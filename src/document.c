/**
 * @file    document.c
 * @brief   In-memory document collection and text clean-up helpers
 *          (see document.h). Finding the files is done by load_collection()
 *          in loader.c; this module no longer loads folders itself.
 *
 * @module  Documents & Text Processing (Member B)
 */
#include "document.h"

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
        xfree(d->title);
        xfree(d->text);
    }
    xfree(c->docs);
    collection_init(c);
}

int collection_add(Collection *c, const char *path, const char *title,
                   const char *text, size_t text_len)
{
    Document *d;
    if (strlen(path) >= MAX_PATH_LEN) return -1;   /* would not fit in Document.path */
    if (c->count == c->capacity) {
        c->capacity = c->capacity ? c->capacity * 2 : 64;
        c->docs = (Document *)xrealloc(c->docs, (size_t)c->capacity * sizeof *c->docs);
    }
    d = &c->docs[c->count];
    memset(d, 0, sizeof *d);
    d->id    = c->count;
    str_copy(d->path, sizeof d->path, path);
    d->title = xstrdup(title ? title : "(untitled)");
    if (text) {
        d->text     = xstrndup(text, text_len);
        d->text_len = text_len;
    }
    return c->count++;
}

const char *collection_text(Collection *c, int doc_id, size_t *len_out)
{
    Document *d;

    if (doc_id < 0 || doc_id >= c->count) return NULL;
    d = &c->docs[doc_id];
    if (!d->text) {
        /* Lazy load: one document is one whole file. */
        d->text = read_file(d->path, &d->text_len);
        if (!d->text) return NULL;
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

int collection_find(const Collection *c, const char *path)
{
    int i;
    for (i = 0; i < c->count; i++)
        if (strcmp(c->docs[i].path, path) == 0) return i;
    return -1;
}
