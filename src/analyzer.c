/**
 * @file    analyzer.c
 * @brief   Tokenizer, stopword filter and normalisation (see analyzer.h).
 *
 * @module  Documents & Text Processing (Member B)
 */
#include "analyzer.h"
#include "stemmer.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A widely used English stopword list (order does not matter: it is
 * sorted at start-up so lookups can use binary search).                 */
static const char *BUILTIN_STOPWORDS[] = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an",
    "and", "any", "are", "as", "at", "be", "because", "been", "before",
    "being", "below", "between", "both", "but", "by", "can", "could", "did",
    "do", "does", "doing", "down", "during", "each", "few", "for", "from",
    "further", "had", "has", "have", "having", "he", "her", "here", "hers",
    "herself", "him", "himself", "his", "how", "i", "if", "in", "into", "is",
    "it", "its", "itself", "just", "may", "me", "might", "more", "most",
    "must", "my", "myself", "no", "nor", "not", "now", "of", "off", "on",
    "once", "only", "or", "other", "our", "ours", "ourselves", "out", "over",
    "own", "same", "shall", "she", "should", "so", "some", "such", "than",
    "that", "the", "their", "theirs", "them", "themselves", "then", "there",
    "these", "they", "this", "those", "through", "to", "too", "under",
    "until", "up", "upon", "very", "was", "we", "were", "what", "when",
    "where", "which", "while", "who", "whom", "why", "will", "with", "would",
    "you", "your", "yours", "yourself", "yourselves"
};

/* ======================================================================
 *  Tokenizer
 * ====================================================================== */
void tokenizer_init(Tokenizer *t, const char *text, size_t len)
{
    t->text  = text;
    t->len   = len;
    t->pos   = 0;
    t->count = 0;
}

/* Returns the number of bytes at s[i] that form a separator (0 if the
 * byte starts a word character). Handles a few UTF-8 punctuation marks. */
static size_t separator_len(const unsigned char *s, size_t i, size_t len)
{
    unsigned char c = s[i];
    if (c < 0x80) return isalnum(c) ? 0 : 1;
    /* U+00A0 no-break space */
    if (c == 0xC2 && i + 1 < len && s[i + 1] == 0xA0) return 2;
    /* U+2000..U+206F general punctuation (dashes, quotes, bullets...) */
    if (c == 0xE2 && i + 2 < len && (s[i + 1] == 0x80 || s[i + 1] == 0x81)) return 3;
    return 0;
}

/* Is there an apostrophe (ASCII ' or UTF-8 U+2019) at s[i]? -> its length */
static size_t apostrophe_len(const unsigned char *s, size_t i, size_t len)
{
    if (s[i] == '\'') return 1;
    if (s[i] == 0xE2 && i + 2 < len && s[i + 1] == 0x80 && (s[i + 2] == 0x99 || s[i + 2] == 0x98))
        return 3;
    return 0;
}

/* Length of the UTF-8 sequence that starts with lead byte c. */
static int utf8_seq_len(unsigned char c)
{
    if (c >= 0xF0) return 4;
    if (c >= 0xE0) return 3;
    if (c >= 0xC0) return 2;
    return 1;
}

int tokenizer_next(Tokenizer *t, char *out, size_t *start, size_t *end)
{
    const unsigned char *s = (const unsigned char *)t->text;
    size_t i = t->pos, n = 0;
    int    copying = 1;

    /* 1. skip separators (but not apostrophes that start a word) */
    while (i < t->len) {
        size_t a = apostrophe_len(s, i, t->len);
        size_t sl = a ? a : separator_len(s, i, t->len);
        if (sl == 0) break;
        i += sl;
    }
    if (i >= t->len) {
        t->pos = i;
        return 0;
    }
    if (start) *start = i;

    /* 2. consume word bytes */
    while (i < t->len) {
        size_t a = apostrophe_len(s, i, t->len);
        if (a) {
            size_t j = i + a;
            /* apostrophe is kept inside a word only if a word byte follows */
            if (n == 0 || j >= t->len || separator_len(s, j, t->len) != 0 ||
                apostrophe_len(s, j, t->len))
                break;
            /* possessive 's at the end of a word: drop it entirely */
            if ((s[j] == 's' || s[j] == 'S') &&
                (j + 1 >= t->len || separator_len(s, j + 1, t->len) != 0)) {
                i = j + 1;
                break;
            }
            i = j;          /* "don't" -> "dont" */
            continue;
        }
        if (separator_len(s, i, t->len) != 0) break;
        if (copying) {
            unsigned char c = s[i];
            if (c >= 0xC0 && n + (size_t)utf8_seq_len(c) > MAX_TERM_LEN - 1)
                copying = 0;                      /* would split a character */
            else if (n >= MAX_TERM_LEN - 1)
                copying = 0;                      /* truncate long tokens    */
            else
                out[n++] = (char)(c < 0x80 ? tolower(c) : c);
        }
        i++;
    }
    out[n] = '\0';
    if (end) *end = i;
    t->pos = i;
    t->count++;
    return 1;
}

/* ======================================================================
 *  Stopwords
 * ====================================================================== */
static int cmp_cstr(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static void add_stopword(Analyzer *a, const char *w, size_t *cap)
{
    if (a->num_stopwords == *cap) {
        *cap = *cap ? *cap * 2 : 128;
        a->stopwords = (char **)xrealloc(a->stopwords, *cap * sizeof *a->stopwords);
    }
    a->stopwords[a->num_stopwords++] = xstrdup(w);
}

void analyzer_init(Analyzer *a, int use_stopwords, int use_stemming,
                   const char *stopword_file)
{
    size_t cap = 0, i, w;
    FILE  *fp = NULL;

    memset(a, 0, sizeof *a);
    a->use_stopwords = use_stopwords;
    a->use_stemming  = use_stemming;

    if (stopword_file && *stopword_file) fp = fopen(stopword_file, "r");
    if (fp) {
        char line[256];
        while (read_line(fp, line, sizeof line)) {
            str_trim(line);
            if (line[0] == '\0' || line[0] == '#') continue;
            str_tolower(line);
            add_stopword(a, line, &cap);
        }
        fclose(fp);
        a->from_file = a->num_stopwords > 0;
    }
    if (a->num_stopwords == 0) {
        size_t n = sizeof BUILTIN_STOPWORDS / sizeof BUILTIN_STOPWORDS[0];
        for (i = 0; i < n; i++) add_stopword(a, BUILTIN_STOPWORDS[i], &cap);
    }
    qsort(a->stopwords, a->num_stopwords, sizeof *a->stopwords, cmp_cstr);
    /* remove duplicates so the list is a proper set */
    for (i = 1, w = 1; i < a->num_stopwords; i++) {
        if (strcmp(a->stopwords[i], a->stopwords[w - 1]) == 0) xfree(a->stopwords[i]);
        else a->stopwords[w++] = a->stopwords[i];
    }
    if (a->num_stopwords > 0) a->num_stopwords = w;
}

void analyzer_free(Analyzer *a)
{
    size_t i;
    for (i = 0; i < a->num_stopwords; i++) xfree(a->stopwords[i]);
    xfree(a->stopwords);
    memset(a, 0, sizeof *a);
}

/* Classic iterative binary search: O(log n) comparisons. */
int analyzer_is_stopword(const Analyzer *a, const char *word)
{
    size_t lo = 0, hi = a->num_stopwords;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int    c   = strcmp(word, a->stopwords[mid]);
        if (c == 0) return 1;
        if (c < 0) hi = mid;
        else       lo = mid + 1;
    }
    return 0;
}

/* ======================================================================
 *  Normalisation
 * ====================================================================== */
static int is_ascii_alpha_word(const char *s)
{
    for (; *s; s++)
        if (*s < 'a' || *s > 'z') return 0;
    return 1;
}

int analyzer_normalize(const Analyzer *a, char *term)
{
    if (term[0] == '\0') return 0;
    if (a->use_stopwords && analyzer_is_stopword(a, term)) return 0;
    if (a->use_stemming && is_ascii_alpha_word(term)) porter_stem(term);
    return term[0] != '\0';
}

int analyzer_next(const Analyzer *a, Tokenizer *t, char *term, int *position,
                  size_t *start, size_t *end)
{
    while (tokenizer_next(t, term, start, end)) {
        int pos = t->count - 1;
        if (analyzer_normalize(a, term)) {
            if (position) *position = pos;
            return 1;
        }
    }
    return 0;
}
