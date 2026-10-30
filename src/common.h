/**
 * @file    common.h
 * @brief   Project-wide constants and small shared types.
 *
 * Every module includes this header, so it must stay tiny and must not
 * include any other project header (avoids circular includes).
 *
 * @module  Core (Team Leader)
 */
#ifndef DS_COMMON_H
#define DS_COMMON_H

#include <stddef.h>

#define DS_NAME    "DocSearch"
#define DS_VERSION "1.0.0"

/* Longest term kept by the tokenizer, including the terminating '\0'.
 * Longer tokens are truncated (deterministically) to 63 bytes.          */
#define MAX_TERM_LEN     64
#define MAX_PATH_LEN     1024
#define MAX_LINE_LEN     4096
/* Maximum number of distinct positive terms a query may score on.
 * 64 lets us record "which query terms matched" in one 64-bit mask.     */
#define MAX_QUERY_TERMS  64

/** Relevance scoring model used to rank matching documents. */
typedef enum {
    SCORE_TF = 0,   /* sum of raw term frequencies (coursework baseline) */
    SCORE_TFIDF,    /* lnc.ltc cosine-normalised TF-IDF                  */
    SCORE_BM25      /* Okapi BM25 (default)                              */
} ScoreModel;

/** Data structure used for the term dictionary (term -> postings). */
typedef enum {
    DICT_HASH = 0,  /* chained hash table, O(1) average lookup          */
    DICT_SORTED,    /* sorted array + binary search, O(log n) lookup    */
    DICT_LIST       /* sorted singly linked list, O(n) lookup           */
} DictType;

#endif /* DS_COMMON_H */
