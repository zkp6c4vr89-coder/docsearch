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

#endif

