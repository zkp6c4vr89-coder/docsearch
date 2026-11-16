/**
 * @file    analyzer.h
 * @brief   Tokenizer and text normaliser. Documents AND queries both pass
 *          through exactly this code, which guarantees they are normalised
 *          with identical rules.
 *
 * Tokenization rules (deterministic, documented in docs/DESIGN.md):
 *   1. A token is a maximal run of "word bytes": ASCII letters/digits and
 *      non-ASCII bytes (so UTF-8 words such as "café" stay intact).
 *   2. Everything else (spaces, punctuation, hyphens, Unicode punctuation
 *      U+2000-U+206F, no-break space) separates tokens:
 *      "renewable-energy" -> "renewable", "energy".
 *   3. An apostrophe between two letters is removed ("don't" -> "dont"),
 *      and a possessive "'s" is dropped ("prandtl's" -> "prandtl").
 *   4. ASCII letters are lower-cased. Tokens are truncated to 63 bytes
 *      (never in the middle of a UTF-8 character).
 * Normalisation, applied after tokenization:
 *   5. Stopwords (sorted list, binary search) are dropped if enabled.
 *   6. Purely alphabetic tokens are reduced with the Porter stemmer if
 *      enabled ("connections" -> "connect").
 * Token positions count every token, including dropped stopwords, so
 * phrase queries such as "state of the art" keep their real gaps.
 *
 * @module  Documents & Text Processing (Member B)
 */
#ifndef DS_ANALYZER_H
#define DS_ANALYZER_H

#include <stddef.h>
#include "common.h"

typedef struct {
    const char *text;
    size_t      len;
    size_t      pos;      /* current byte offset                        */
    int         count;    /* tokens produced so far (= next position)   */
} Tokenizer;

void tokenizer_init(Tokenizer *t, const char *text, size_t len);
/* Writes the next lower-cased token into out[MAX_TERM_LEN]. start/end
 * (may be NULL) receive its byte range in the text. Returns 0 at end.   */
int  tokenizer_next(Tokenizer *t, char *out, size_t *start, size_t *end);

typedef struct {
    int     use_stopwords;
    int     use_stemming;
    char  **stopwords;       /* sorted, for binary search */
    size_t  num_stopwords;
    int     from_file;       /* 1 if loaded from a file, 0 = built-in list */
} Analyzer;

/* Loads stopwords from `stopword_file` (one per line, '#' comments) or
 * falls back to the built-in English list if the file is missing/empty. */
void analyzer_init(Analyzer *a, int use_stopwords, int use_stemming,
                   const char *stopword_file);
void analyzer_free(Analyzer *a);

int  analyzer_is_stopword(const Analyzer *a, const char *word);
/* Normalises one lower-cased token in place (stopword check + stemming).
 * Returns 0 if the token must be discarded.                             */
int  analyzer_normalize(const Analyzer *a, char *term);
/* Next indexable term: combines tokenizer_next() and normalisation.
 * *position receives the token's position in the full token stream.     */
int  analyzer_next(const Analyzer *a, Tokenizer *t, char *term, int *position,
                   size_t *start, size_t *end);

#endif /* DS_ANALYZER_H */
