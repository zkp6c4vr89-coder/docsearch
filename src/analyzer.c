/**
 * @file    analyzer.c
 * @brief   Tokenizer, stopword filter and normalisation (see analyzer.h).
 *
 * @module  Documents & Text Processing (Member B)
 */
#include "analyzer.h"
#include "stemmer.h"

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

/* Splitting text into words is done by next_token() (tokenizer.c), the one
 * tokenizer of the project. This wrapper only remembers where it is in the
 * text, counts the tokens and reports each token's byte range.
 * The text must be '\0' terminated, because next_token() stops there.     */
int tokenizer_next(Tokenizer *t, char *out, size_t *start, size_t *end)
{
    const char *p;
    size_t      s, e;

    if (t->pos >= t->len) return 0;
    p = t->text + t->pos;
    if (!next_token(&p, out)) {
        t->pos = (size_t)(p - t->text);
        return 0;
    }
    /* next_token() leaves p just after the word; walk back to its first
     * character (strlen(out) would be wrong for a truncated long word).  */
    e = (size_t)(p - t->text);
    s = e;
    while (s > t->pos && isalnum((unsigned char)t->text[s - 1])) s--;

    if (start) *start = s;
    if (end) *end = e;
    t->pos = e;
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
