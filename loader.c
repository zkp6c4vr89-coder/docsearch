#include "common.h"
#include <stdio.h>
#include <string.h>

char *read_whole_file(const char *path, long *out_size) {

//open the file
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Warning: could not open '%s'\n", path);
    return NULL;
    }
// find its size by jumping to the end
   if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Warning: could not seek in '%s'\n", path);
        fclose(fp);
        return NULL;
    }
    long size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "Warning: could not get size of '%s'\n", path);
        fclose(fp);
        return NULL;
    }
// go back to the start
    rewind(fp);

// allocate size
    char *buffer = malloc((size_t)size +1);
    if (buffer == NULL){
        fprintf(stderr, "Error: out of memory reading '%s'\n", path);
        fclose(fp);
        return NULL;
    }

// read and terminate 
    size_t bytes_read = fread(buffer, 1, (size_t)size, fp);
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Warning: could not read all of '%s'\n", path);
        free(buffer);
        fclose(fp);
        return NULL;
    }
    buffer[bytes_read] = '\0';

// close the file and handover 
    fclose(fp);

    if (out_size != NULL) {
        *out_size = (long)bytes_read;
    }
    return buffer;   /* caller must free() this */
}

Document *load_collection(const char *list_path, int *out_count)
{
    *out_count = 0;

// Open the list of paths
    FILE *list = fopen(list_path, "r");
    if (list == NULL) {
        fprintf(stderr, "Error: could not open '%s'\n", list_path);
        return NULL;
    }

// First pass, Counts most number of documents possible
    char line[MAX_PATH_LEN];
    int max_docs = 0;
    while (fgets(line, sizeof line, list) != NULL) {
        max_docs++;
    }

    if (max_docs == 0) {
        fprintf(stderr, "Error: '%s' is empty\n", list_path);
        fclose(list);
        return NULL;
    }

// allocates space for max number of docs
    Document *docs = malloc((size_t)max_docs * sizeof(Document));
    if (docs == NULL) {
        fprintf(stderr, "Error: out of memory for %d documents\n", max_docs);
        fclose(list);
        return NULL;
    }

// second pass
    rewind(list);

// clean each line
    int count = 0;
    while (fgets(line, sizeof line, list) != NULL) {
// cut off line ending
        line[strcspn(line, "\r\n")] = '\0';

// Skip blanks
        if (line[0] == '\0') {
            continue;
        }

// check id file can be read
        FILE *test = fopen(line, "rb");
        if (test == NULL) {
            fprintf(stderr, "Warning: skipping '%s' (cannot open)\n", line);
            continue;
        }
        fclose(test);

// store in the next free slot & ID == Position
        docs[count].id = count;
        strcpy(docs[count].path, line);
        docs[count].total_terms = 0;
        docs[count].unique_terms = 0;
        count++;
    }

// close the list and report
    fclose(list);
    printf("Loaded %d documents from '%s'\n", count, list_path);

    *out_count = count;
    return docs;   
}