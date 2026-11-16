#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "indexer.h"

/* collections this small get one line per document in the statistics */
#define SMALL_COLLECTION 10

int get_safe_choice(int *choice) {
    char buffer[256];
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0; 
    }
    
    if (strchr(buffer, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    
    if (endptr == buffer) return -1;
    
    while (*endptr != '\0') {
        if (*endptr != '\n' && *endptr != ' ' && *endptr != '\r' && *endptr != '\t') {
            return -1;
        }
        endptr++;
    }
    
    *choice = (int)val;
    return 1;
}

/* Asks the user for a file name. Shared by options 1, 8 and 9.
   Returns 1 if a name was entered, 0 if the line was empty or input ended. */
int ask_file_name(const char *prompt, char *name, size_t size) {
    printf("%s", prompt);

    if (fgets(name, (int)size, stdin) == NULL) {
        name[0] = '\0';
        return 0;
    }

    if (strchr(name, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }

    name[strcspn(name, "\r\n")] = '\0';
    return name[0] != '\0';
}

int main(void) {
    int choice = -1;
    int collection_loaded = 0;
    int index_ready = 0;
    int has_results_or_stats = 0;

    Document *docs = NULL;
    int doc_count = 0;
    Index idx;
    char file_name[MAX_PATH_LEN];

    /* the index is always initialised, so index_free and index_load are safe at any time */
    index_init(&idx);

    do {
        printf("\n===== Document Search =====\n");
        printf("1. Load Collection\n");
        printf("2. Build Index\n");
        printf("3. Show Statistics\n");
        printf("4. Search\n");
        printf("5. View Document\n");
        printf("6. Set Number of Results\n");
        printf("7. Save Report\n");
        printf("8. Save Index\n");
        printf("9. Load Index\n");
        printf("0. Exit\n");
        printf("Choice: ");

        int status = get_safe_choice(&choice);
        if (status == 0) {
            printf("End of input. Exiting.\n");
            break;
        }
        if (status != 1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1:
                if (!ask_file_name("List file (one document path per line): ", file_name, sizeof(file_name))) {
                    printf("No file name entered.\n");
                    break;
                }

                /* throw away the old collection and its index before loading a new one */
                free(docs);
                docs = NULL;
                doc_count = 0;
                index_free(&idx);
                collection_loaded = 0;
                index_ready = 0;
                has_results_or_stats = 0;

                docs = load_collection(file_name, &doc_count);
                if (docs == NULL || doc_count == 0) {
                    free(docs);
                    docs = NULL;
                    doc_count = 0;
                    printf("Could not load a collection from %s\n", file_name);
                    break;
                }
                collection_loaded = 1;
                printf("Collection loaded successfully.\n");
                break;

            case 2:
                if (!collection_loaded) {
                    printf("Please load a collection first (option 1).\n");
                    break;
                }
                index_free(&idx);
                index_init(&idx);
                index_ready = 0;
                {
                    int indexed = build_index(&idx, docs, doc_count);
                    if (indexed < 0) {
                        index_free(&idx);
                        printf("Error building index.\n");
                        break;
                    }
                    index_ready = 1;
                    printf("Index built successfully (%d documents indexed).\n", indexed);
                }
                break;

            case 3:
                if (!index_ready) {
                    printf("Please build or load an index first.\n");
                    break;
                }
                printf("--- Statistics ---\n");
                printf("Documents:    %d\n", idx.num_docs);
                printf("Total terms:  %ld\n", index_total_terms(&idx));
                printf("Unique terms: %d\n", idx.unique_terms);
                printf("Postings:     %d\n", idx.posting_count);

                if (idx.num_docs <= SMALL_COLLECTION) {
                    for (int i = 0; i < idx.num_docs; i++) {
                        printf("  %s: %d total terms, %d unique\n", idx.docs[i].path,
                               idx.docs[i].total_terms, idx.docs[i].unique_terms);
                    }
                }
                {
                    int empty_docs = 0;
                    for (int i = 0; i < idx.num_docs; i++) {
                        if (idx.docs[i].total_terms == 0) {
                            if (empty_docs == 0) {
                                printf("Documents with 0 terms:\n");
                            }
                            printf("  %s\n", idx.docs[i].path);
                            empty_docs++;
                        }
                    }
                    if (empty_docs == 0) {
                        printf("Documents with 0 terms: none\n");
                    } else {
                        printf("(%d documents with 0 terms)\n", empty_docs);
                    }
                }
                has_results_or_stats = 1;
                break;

            case 4:
                if (!index_ready) {
                    printf("Please build or load an index first.\n");
                    break;
                }
                printf("Performing search...\n");
                has_results_or_stats = 1;
                break;

            case 5:
                if (!collection_loaded) {
                    printf("Please load a collection first.\n");
                    break;
                }
                printf("View document logic goes here...\n");
                break;

            case 7:
                if (!has_results_or_stats) {
                    printf("Nothing to save yet.\n");
                    break;
                }
                {
                    FILE *f = fopen("report.txt", "w");
                    if (f != NULL) {
                        fprintf(f, "Report data saved.\n");
                        fclose(f);
                        printf("Report saved to report.txt\n");
                    } else {
                        printf("Error saving report.\n");
                    }
                }
                break;

            case 6:
                printf("Set number of results logic goes here...\n");
                break;

            case 8:
                if (!index_ready) {
                    printf("Please build or load an index first.\n");
                    break;
                }
                if (!ask_file_name("Save index to file: ", file_name, sizeof(file_name))) {
                    printf("No file name entered.\n");
                    break;
                }
                if (index_save(&idx, file_name) != 0) {
                    printf("Error saving index.\n");
                    break;
                }
                printf("Index saved to %s\n", file_name);
                break;

            case 9:
                if (!ask_file_name("Load index from file: ", file_name, sizeof(file_name))) {
                    printf("No file name entered.\n");
                    break;
                }
                /* on failure index_load leaves the current index as it was */
                if (index_load(&idx, file_name) != 0) {
                    printf("Error loading index.\n");
                    break;
                }
                index_ready = 1;
                printf("Index loaded successfully (%d documents).\n", idx.num_docs);
                break;

            case 0:
                printf("Exiting program...\n");
                break;

            default:
                printf("Unknown option. Choose between 0 and 9.\n");
        }
    } while (choice != 0);

    /* reached from option 0 and from end of input */
    free(docs);
    index_free(&idx);

    return 0;
}
