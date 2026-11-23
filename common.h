#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

/* Longest word kept: 63 characters + ('\0')*/
#define MAX_WORD_LEN 64

/* Longest file path  */
#define MAX_PATH_LEN 512

/* One record per document in the collection */
typedef struct {
    int  id;             /* position in the array*/
    char path[MAX_PATH_LEN];
    int  total_terms;    /* every token counted */
    int  unique_terms;   /* distinct words*/
    char  *title;        /* short title, NULL if not set (src/document.c) */
    char  *text;         /* cached text, NULL if not loaded (src/document.c) */
    size_t text_len;     /* length of text in bytes */
} Document;

/*loader.c (Sreenandhu Siju Nair)*/
char *read_whole_file(const char *path, long *out_size);
Document *load_collection(const char *list_path, int *out_count);
int next_token(const char **pos, char *word);


// index header (Zoyal Ahmed Khan)
#define NUM_BUCKETS 10007 //prime number so words spread more evenly and less collisions 
#define MAX_WORD 255

typedef struct Posting //posting basically says "this term appears in this document how many times"
{
    int doc_id;
    int freq;
    struct Posting *next;
} 
Posting;

typedef struct Term //term is one word, and all the documents it is appearing in
{
    char *text;
    int doc_count; 
    Posting *first; 
    Posting *last; 
    struct Term *next; 
} 
Term;

typedef struct //the whole index
{
    Term *buckets[NUM_BUCKETS];
    int unique_terms; 
    int posting_count; 
    Document *docs; 
    int num_docs;
    int doc_capacity;
} 
Index;

void index_init(Index *idx); //empty index
int index_add_document(Index *idx, const char *path);
int index_add_term(Index *idx, const char *word, int doc_id);
long index_total_terms(const Index *idx);
Term *index_lookup(const Index *idx, const char *word);
void index_free(Index *idx);
int index_check_frequency_sum(const Index *idx);
int index_save(const Index *idx, const char *filename);
int index_load(Index *idx, const char *filename);

/* util.c - small helpers shared by every module */

/* memory: these never return NULL, they print an error and exit instead */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);   /* copies n bytes and adds '\0' */
void  xfree(void *ptr);

/* growable string buffer, data is always '\0' terminated once something is added */
typedef struct {
    char  *data;   /* NULL while the buffer is empty */
    size_t len;    /* bytes used, not counting the '\0' */
    size_t cap;    /* bytes allocated */
} StrBuf;

void  sb_init(StrBuf *sb);
void  sb_append_char(StrBuf *sb, char c);
void  sb_append(StrBuf *sb, const char *s);
void  sb_append_n(StrBuf *sb, const char *s, size_t n);
char *sb_take(StrBuf *sb);   /* hands the string to the caller (must free), buffer becomes empty */
void  sb_free(StrBuf *sb);

/* strings */
void str_copy(char *out, size_t size, const char *src);   /* always terminates, cuts off if too long */
void str_trim(char *s);                                    /* removes spaces at both ends, in place */
void str_tolower(char *s);                                 /* ASCII letters only, in place */
int  str_ends_with_ci(const char *s, const char *suffix);  /* ignores case, 1 = yes */

/* paths */
void        path_join(char *out, size_t size, const char *dir, const char *name);
const char *path_basename(const char *path);   /* part after the last '/' or '\' */

/* files */
char *read_file(const char *path, size_t *out_len);   /* whole file, caller must free, NULL on failure */
int   read_line(FILE *fp, char *buf, size_t size);    /* one line without the line ending, 0 at end of file */

#endif

// Most distinct words we keep from one query
#define MAX_QUERY_WORDS 32

// linear.c 
int linear_scan(const Document *docs, int count, const char *query, int *scores);
