/**
 * @file    document.c
 * @brief   Collection loading and per-document metadata (see document.h).
 *
 * @module  Documents & Text Processing (Member B)
 */
#include "document.h"
#include "platform.h"
#include "util.h"
#include "common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_MAX 80

/* ======================================================================
 *  Small text helpers
 * ====================================================================== */

/* Case-insensitive search for `needle` inside hay[0..len). */
static const char *ci_find(const char *hay, size_t len, const char *needle)
{
    size_t n = strlen(needle), i, j;
    if (n == 0 || n > len) return NULL;
    for (i = 0; i + n <= len; i++) {
        for (j = 0; j < n; j++)
            if (tolower((unsigned char)hay[i + j]) != tolower((unsigned char)needle[j]))
                break;
        if (j == n) return hay + i;
    }
    return NULL;
}

/* Finds <tag ...>content</tag>; returns content start and its length. */
static const char *tag_content(const char *s, size_t len, const char *tag, size_t *clen)
{
    char        open[32], close[32];
    const char *a, *b, *end = s + len;
    snprintf(open, sizeof open, "<%s", tag);
    snprintf(close, sizeof close, "</%s>", tag);
    a = ci_find(s, len, open);
    if (!a) return NULL;
    a = memchr(a, '>', (size_t)(end - a));
    if (!a) return NULL;
    a++;
    b = ci_find(a, (size_t)(end - a), close);
    if (!b) b = end;                 /* tolerate a missing close tag */
    *clen = (size_t)(b - a);
    return a;
}

static int is_blank(const char *s, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        if (!isspace((unsigned char)s[i])) return 0;
    return 1;
}

static int looks_binary(const char *s, size_t len)
{
    size_t i, n = len < 4096 ? len : 4096;
    for (i = 0; i < n; i++)
        if (s[i] == '\0') return 1;
    return 0;
}

/* Copies at most `max` visible characters, collapsing runs of whitespace
 * to one space; used to produce clean one-line titles.                  */
static char *collapse_ws(const char *s, size_t len, size_t max)
{
    StrBuf sb;
    size_t i;
    int    pending_space = 0;
    sb_init(&sb);
    for (i = 0; i < len && sb.len < max; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (isspace(ch)) {
            pending_space = sb.len > 0;
        } else {
            if (pending_space) sb_append_char(&sb, ' ');
            pending_space = 0;
            sb_append_char(&sb, (char)ch);
        }
    }
    if (i < len && sb.len >= max) sb_append(&sb, "...");
    return sb_take(&sb);
}

/* Removes <tags> (and <script>/<style> bodies) and decodes a few common
 * entities. Shared by the HTML and TREC readers.                        */
static char *strip_markup(const char *s, size_t len, size_t *out_len)
{
    StrBuf sb;
    size_t i = 0;
    sb_init(&sb);
    while (i < len) {
        if (s[i] == '<') {
            const char *skip_to = NULL;
            if (len - i > 7 && ci_find(s + i, 7, "<script")) skip_to = "</script>";
            else if (len - i > 6 && ci_find(s + i, 6, "<style")) skip_to = "</style>";
            if (skip_to) {
                const char *e = ci_find(s + i, len - i, skip_to);
                i = e ? (size_t)(e - s) + strlen(skip_to) : len;
            } else {
                const char *e = memchr(s + i, '>', len - i);
                i = e ? (size_t)(e - s) + 1 : len;
            }
            sb_append_char(&sb, ' ');          /* tags separate words */
        } else if (s[i] == '&') {
            static const struct { const char *ent; char ch; } map[] = {
                { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' },
                { "&quot;", '"' }, { "&#39;", '\'' }, { "&apos;", '\'' }, { "&nbsp;", ' ' }
            };
            size_t k, n_map = sizeof map / sizeof map[0];
            for (k = 0; k < n_map; k++) {
                size_t el = strlen(map[k].ent);
                if (len - i >= el && strncmp(s + i, map[k].ent, el) == 0) {
                    sb_append_char(&sb, map[k].ch);
                    i += el;
                    break;
                }
            }
            if (k == n_map) sb_append_char(&sb, s[i++]);
        } else {
            sb_append_char(&sb, s[i++]);
        }
    }
    if (out_len) *out_len = sb.len;
    return sb_take(&sb);
}

/* Title of a plain-text file: an e-mail style "Subject:" line in the
 * first 30 lines (common in newsgroup corpora), else the first
 * non-blank line.                                                       */
static char *plain_title(const char *text, size_t len)
{
    const char *line = text, *end = text + len, *first = NULL;
    size_t      first_len = 0;
    int         n;
    for (n = 0; n < 30 && line < end; n++) {
        const char *nl = memchr(line, '\n', (size_t)(end - line));
        size_t      ll = nl ? (size_t)(nl - line) : (size_t)(end - line);
        if (ll > 8 && ci_find(line, 8, "subject:") == line)
            return collapse_ws(line + 8, ll - 8, TITLE_MAX);
        if (!first && !is_blank(line, ll)) { first = line; first_len = ll; }
        if (!nl) break;
        line = nl + 1;
    }
    if (first) return collapse_ws(first, first_len, TITLE_MAX);
    return xstrdup("(untitled)");
}

/* ======================================================================
 *  TREC block parsing
 * ====================================================================== */
char *trec_extract_text(const char *block, size_t len, char **docno,
                        char **title, size_t *out_len)
{
    const char *p;
    size_t      plen;
    const char *title_s = tag_content(block, len, "title", &plen);
    size_t      title_len = title_s ? plen : 0;
    const char *text_s;
    size_t      text_len = 0;
    StrBuf      sb;
    char       *clean;

    if (docno) {
        p = tag_content(block, len, "docno", &plen);
        *docno = p ? collapse_ws(p, plen, 64) : NULL;
    }
    if (title) *title = title_s ? collapse_ws(title_s, title_len, TITLE_MAX) : NULL;

    text_s = tag_content(block, len, "text", &text_len);
    sb_init(&sb);
    if (title_s || text_s) {
        /* Index TITLE + TEXT. Other fields (author, bib...) are metadata. */
        if (title_s) sb_append_n(&sb, title_s, title_len);
        if (title_s && text_s) sb_append_char(&sb, '\n');
        if (text_s) sb_append_n(&sb, text_s, text_len);
    } else {
        /* No recognised fields: index the whole block minus the DOCNO. */
        sb_append_n(&sb, block, len);
        p = tag_content(sb.data, sb.len, "docno", &plen);
        if (p) memset((char *)p, ' ', plen);
    }
    /* Fields may still contain nested markup or entities. */
    clean = strip_markup(sb.data ? sb.data : "", sb.len, out_len);
    sb_free(&sb);
    return clean;
}

/* ======================================================================
 *  Collection management
 * ====================================================================== */
void collection_init(Collection *c)
{
    memset(c, 0, sizeof *c);
}

void collection_free(Collection *c)
{
    int i;
    for (i = 0; i < c->count; i++) {
        Document *d = &c->docs[i];
        xfree(d->name);
        xfree(d->path);
        xfree(d->title);
        xfree(d->text);
    }
    xfree(c->docs);
    collection_init(c);
}

int collection_add(Collection *c, const char *name, const char *path,
                   const char *title, DocFormat fmt, long offset,
                   long length, const char *text, size_t text_len)
{
    Document *d;
    if (c->count == c->capacity) {
        c->capacity = c->capacity ? c->capacity * 2 : 64;
        c->docs = (Document *)xrealloc(c->docs, (size_t)c->capacity * sizeof *c->docs);
    }
    d = &c->docs[c->count];
    memset(d, 0, sizeof *d);
    d->id       = c->count;
    d->name     = xstrdup(name);
    d->path     = xstrdup(path);
    d->title    = xstrdup(title ? title : "(untitled)");
    d->format   = fmt;
    d->offset   = offset;
    d->length   = length;
    if (text) {
        d->text     = xstrndup(text, text_len);
        d->text_len = text_len;
    }
    return c->count++;
}

/* Name shown to the user: path relative to the folder that was loaded,
 * with '/' separators on every OS.                                      */
static void relative_name(char *out, size_t size, const char *path, const char *root)
{
    size_t rl = root ? strlen(root) : 0;
    char  *p;
    if (root && rl > 0 && strncmp(path, root, rl) == 0) {
        path += rl;
        while (*path == '/' || *path == '\\') path++;
    } else {
        path = path_basename(path);
    }
    str_copy(out, size, path);
    for (p = out; *p; p++)
        if (*p == '\\') *p = '/';
}

typedef enum { EXT_PLAIN, EXT_TREC, EXT_HTML, EXT_OTHER } ExtKind;

static ExtKind classify_ext(const char *path)
{
    const char *base = path_basename(path);
    const char *dot  = strrchr(base, '.');
    if (!dot) return EXT_PLAIN;          /* e.g. 20 Newsgroups files */
    if (str_ends_with_ci(base, ".txt") || str_ends_with_ci(base, ".text"))
        return EXT_PLAIN;
    if (str_ends_with_ci(base, ".xml") || str_ends_with_ci(base, ".trec") ||
        str_ends_with_ci(base, ".sgml") || str_ends_with_ci(base, ".sgm"))
        return EXT_TREC;
    if (str_ends_with_ci(base, ".html") || str_ends_with_ci(base, ".htm"))
        return EXT_HTML;
    /* names such as "doc.1" or "53120" style numeric suffixes */
    if (isdigit((unsigned char)dot[1])) return EXT_PLAIN;
    return EXT_OTHER;
}

/* Parses every <DOC>...</DOC> block of a TREC file. */
static int load_trec(Collection *c, const char *path, const char *buf,
                     size_t len, FILE *log)
{
    const char *p = buf, *end = buf + len;
    int         added = 0, seq = 0;
    while (p < end) {
        const char *s = ci_find(p, (size_t)(end - p), "<doc>");
        const char *e;
        char       *docno = NULL, *title = NULL, *text;
        size_t      tlen;
        if (!s) break;
        e = ci_find(s, (size_t)(end - s), "</doc>");
        e = e ? e + 6 : end;
        seq++;
        text = trec_extract_text(s, (size_t)(e - s), &docno, &title, &tlen);
        if (is_blank(text, tlen)) {
            c->skipped_empty++;
            if (log) fprintf(log, "  warning: %s document #%d is empty (skipped)\n",
                             path_basename(path), seq);
        } else {
            char fallback[64];
            snprintf(fallback, sizeof fallback, "%s#%d", path_basename(path), seq);
            collection_add(c, docno && *docno ? docno : fallback, path,
                           title && *title ? title : NULL, DOC_TREC,
                           (long)(s - buf), (long)(e - s), text, tlen);
            added++;
        }
        xfree(text);
        xfree(docno);
        xfree(title);
        p = e;
    }
    return added;
}

static int load_file(Collection *c, const char *path, const char *root, FILE *log)
{
    ExtKind kind = classify_ext(path);
    char   *buf;
    size_t  len;
    char    name[MAX_PATH_LEN];
    int     added = 0;

    c->files_scanned++;
    if (kind == EXT_OTHER) {
        c->skipped_other++;
        return 0;
    }
    buf = read_file(path, &len);
    if (!buf) {
        c->skipped_unreadable++;
        if (log) fprintf(log, "  warning: cannot read '%s' (skipped)\n", path);
        return 0;
    }
    c->total_bytes += (long)len;
    if (is_blank(buf, len)) {
        c->skipped_empty++;
        if (log) fprintf(log, "  warning: '%s' is empty (skipped)\n", path_basename(path));
    } else if (looks_binary(buf, len)) {
        c->skipped_other++;
        if (log) fprintf(log, "  warning: '%s' is not a text file (skipped)\n", path_basename(path));
    } else if (kind == EXT_TREC || ci_find(buf, len < 1024 ? len : 1024, "<doc>")) {
        added = load_trec(c, path, buf, len, log);
        if (added == 0 && log)
            fprintf(log, "  warning: no <DOC> blocks found in '%s'\n", path_basename(path));
    } else {
        char  *title;
        relative_name(name, sizeof name, path, root);
        if (kind == EXT_HTML) {
            size_t      clen, tl;
            char       *clean = strip_markup(buf, len, &clen);
            const char *t     = tag_content(buf, len, "title", &tl);
            if (t) {                       /* decode entities in the title too */
                size_t dl;
                char  *decoded = strip_markup(t, tl, &dl);
                title = collapse_ws(decoded, dl, TITLE_MAX);
                xfree(decoded);
            } else {
                title = plain_title(clean, clen);
            }
            collection_add(c, name, path, title, DOC_HTML, 0, (long)len, clean, clen);
            xfree(clean);
        } else {
            title = plain_title(buf, len);
            collection_add(c, name, path, title, DOC_PLAIN, 0, (long)len, buf, len);
        }
        xfree(title);
        added = 1;
    }
    if (added > 0) c->files_loaded++;
    xfree(buf);
    return added;
}

int collection_load(Collection *c, const char *path, int recursive, FILE *log)
{
    int before = c->count;
    str_copy(c->source, sizeof c->source, path);
    if (fs_is_dir(path)) {
        char  **files;
        size_t  n, i;
        if (!fs_list_files(path, recursive, &files, &n)) return -1;
        for (i = 0; i < n; i++) load_file(c, files[i], path, log);
        fs_free_list(files, n);
    } else if (fs_is_file(path)) {
        load_file(c, path, NULL, log);
    } else {
        return -1;
    }
    return c->count - before;
}

const char *collection_text(Collection *c, int doc_id, size_t *len_out)
{
    Document *d;
    FILE     *fp;
    char     *raw;
    size_t    got;

    if (doc_id < 0 || doc_id >= c->count) return NULL;
    d = &c->docs[doc_id];
    if (!d->text) {
        /* Lazy load: jump straight to the stored byte offset. */
        fp = fopen(d->path, "rb");
        if (!fp) return NULL;
        if (fseek(fp, d->offset, SEEK_SET) != 0) { fclose(fp); return NULL; }
        raw = (char *)xmalloc((size_t)d->length + 1);
        got = fread(raw, 1, (size_t)d->length, fp);
        fclose(fp);
        raw[got] = '\0';
        if (d->format == DOC_TREC) {
            d->text = trec_extract_text(raw, got, NULL, NULL, &d->text_len);
            xfree(raw);
        } else if (d->format == DOC_HTML) {
            d->text = strip_markup(raw, got, &d->text_len);
            xfree(raw);
        } else {
            d->text     = raw;
            d->text_len = got;
        }
    }
    if (len_out) *len_out = d->text_len;
    return d->text;
}

void collection_release_text(Collection *c)
{
    int i;
    for (i = 0; i < c->count; i++) {
        xfree(c->docs[i].text);
        c->docs[i].text     = NULL;
        c->docs[i].text_len = 0;
    }
}

int collection_find(const Collection *c, const char *name)
{
    int i;
    for (i = 0; i < c->count; i++)
        if (strcmp(c->docs[i].name, name) == 0) return i;
    return -1;
}
