#include "common.h"
#include <stdio.h>

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