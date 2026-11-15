/**
 * @file    document.h
 * @brief   Document collection: loading .txt / TREC (.xml/.trec) / HTML
 *          files, assigning document IDs and keeping per-document metadata.
 *
 * Supported input formats (detected automatically):
 *   - Plain text (.txt, .text or no extension): one file = one document.
 *   - TREC / SGML (.xml, .trec, .sgml, or any file whose content starts
 *     with <DOC>): one file holds many <DOC>...</DOC> blocks. The DOCNO
 *     becomes the document name, TITLE + TEXT are indexed.
 *   - HTML (.html, .htm): tags are stripped, <title> becomes the title.
 *
 * Document text is cached in memory after loading; if it is released (or
 * the collection came from a saved index) collection_text() re-reads just
 * that document from disk using fseek() to its stored byte offset.
 *
 * @module  Documents & Text Processing (Member B)
 */
#ifndef DS_DOCUMENT_H
#define DS_DOCUMENT_H

#include <stdio.h>

typedef enum { DOC_PLAIN = 0, DOC_TREC = 1, DOC_HTML = 2 } DocFormat;

typedef struct {
    int       id;          /* 0-based, equals position in Collection.docs  */
    char     *name;        /* file name (relative to root) or TREC DOCNO   */
    char     *path;        /* source file on disk                          */
    char     *title;       /* short human-readable title                   */
    DocFormat format;
    long      offset;      /* byte offset of this document in `path`       */
    long      length;      /* byte length of the raw document in `path`    */
    char     *text;        /* cached clean text, NULL if not loaded        */
    size_t    text_len;
    /* statistics filled in by the indexer */
    int       num_tokens;  /* all tokens, including stopwords              */
    int       num_terms;   /* indexed terms (document length for ranking)  */
    int       num_unique;  /* distinct indexed terms                       */
} Document;

typedef struct {
    Document *docs;
    int       count;
    int       capacity;
    /* load report */
    int       files_scanned;
    int       files_loaded;
    int       skipped_empty;
    int       skipped_unreadable;
    int       skipped_other;   /* unsupported extension / binary file   */
    long      total_bytes;
    char      source[1024];
} Collection;

void        collection_init(Collection *c);
void        collection_free(Collection *c);

/* Loads a folder (optionally recursive) or a single file and appends its
 * documents. Problems are reported to `log` (may be NULL). Returns the
 * number of documents added, or -1 if `path` does not exist.             */
int         collection_load(Collection *c, const char *path, int recursive, FILE *log);

/* Appends a document; takes ownership of nothing (strings are copied).
 * `text` may be NULL (lazy loading). Returns the new document id.        */
int         collection_add(Collection *c, const char *name, const char *path,
                           const char *title, DocFormat fmt, long offset,
                           long length, const char *text, size_t text_len);

/* Returns the document text, loading it from disk if needed (NULL on
 * failure, e.g. the file was deleted after the index was saved).         */
const char *collection_text(Collection *c, int doc_id, size_t *len_out);
void        collection_release_text(Collection *c);  /* frees all caches */

/* Linear search by exact name; -1 if not found. */
int         collection_find(const Collection *c, const char *name);

/* Exposed for unit tests: extracts the clean text of one TREC block. */
char       *trec_extract_text(const char *block, size_t len, char **docno,
                              char **title, size_t *out_len);

#endif /* DS_DOCUMENT_H */
