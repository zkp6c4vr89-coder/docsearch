#include <string.h>
#include "../common.h"

int main(int argc, char *argv[])
{
// takes input file and output folder from the command line
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <cran.all.1400.xml> <output_folder>\n", argv[0]);
        return 1;
    }

// load the xml into memory
    char *xml = read_whole_file(argv[1], NULL);
    if (xml == NULL) {
        return 1;
    }

    int written = 0;
    char *p = xml;

// jumps to each document ID number
    while ((p = strstr(p, "<docno>")) != NULL) {
        p += strlen("<docno>");
        int docno = (int)strtol(p, NULL, 10);

// if document has no end tag then it stops
        char *doc_end = strstr(p, "</doc>");
        if (doc_end == NULL) {
            fprintf(stderr, "Warning: doc %d has no </doc>, stopping\n", docno);
            break;
        }

// find text in document
        char *text_start = strstr(p, "<text>");
        char *text_end = NULL;
        if (text_start != NULL && text_start < doc_end) {
            text_start += strlen("<text>");
            text_end = strstr(text_start, "</text>");
        }

// build the output name
        char out_path[MAX_PATH_LEN];
        snprintf(out_path, sizeof out_path, "%s/%04d.txt", argv[2], docno);

        FILE *out = fopen(out_path, "w");
        if (out == NULL) {
            fprintf(stderr, "Error: cannot create '%s'\n", out_path);
            free(xml);
            return 1;
        }

// write the text or leave the file empty
        if (text_end != NULL && text_end < doc_end) {
            fwrite(text_start, 1, (size_t)(text_end - text_start), out);
            fputc('\n', out);
        } else {
            fprintf(stderr, "Warning: doc %d has no text, writing empty file\n", docno);
        }
        fclose(out);
        written++;

// continue searching
        p = doc_end;
    }

    free(xml);
    printf("Wrote %d documents to '%s'\n", written, argv[2]);
    return 0;
}