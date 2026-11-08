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

#endif