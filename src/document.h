/**
 * @file    document.h
 * @brief   In-memory document collection and text clean-up helpers.
 *
 * There is ONE Document struct in the project, defined in common.h, and
 * ONE way of finding the documents: load_collection() in loader.c reads a
 * list of file paths (Cranfield is first split into one file per document
 * by tools/split_cranfield). This module only keeps documents in a
 * growable array and caches their text and title.
 *
 * Document text is cached in memory; if it is released, collection_text()
 * re-reads the file from disk.
 *
 * @module  Documents & Text Processing (Member B)
 */
#ifndef DS_DOCUMENT_H
#define DS_DOCUMENT_H

#include "../common.h"

typedef struct {
    Document *docs;
    int       count;
    int       capacity;
} Collection;

void        collection_init(Collection *c);
void        collection_free(Collection *c);

/* Appends a document; the strings are copied. `title` may be NULL, and
 * `text` may be NULL (lazy loading). Returns the new document id, or -1
 * if `path` is too long for Document.path.                               */
int         collection_add(Collection *c, const char *path, const char *title,
                           const char *text, size_t text_len);

/* Returns the document text, loading it from disk if needed (NULL on
 * failure, e.g. the file was deleted).                                   */
const char *collection_text(Collection *c, int doc_id, size_t *len_out);
void        collection_release_text(Collection *c);  /* frees all caches */

/* Linear search by exact path; -1 if not found. */
int         collection_find(const Collection *c, const char *path);

/* Extracts the clean text of one TREC <DOC> block (TITLE + TEXT, markup
 * removed). docno/title (may be NULL) receive malloc'd copies.           */
char       *trec_extract_text(const char *block, size_t len, char **docno,
                              char **title, size_t *out_len);

#endif /* DS_DOCUMENT_H */
