#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main() {
    int choice;
    int collection_loaded = 0;
    int index_ready = 0;
    int has_results_or_stats = 0;

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

        if (get_safe_choice(&choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1:
                collection_loaded = 1;
                index_ready = 0;
                printf("Collection loaded successfully.\n");
                break;

            case 2:
                if (!collection_loaded) {
                    printf("Please load a collection first (option 1).\n");
                    break;
                }
                index_ready = 1;
                printf("Index built successfully.\n");
                break;

            case 3:
                if (!collection_loaded || !index_ready) {
                    printf("Collection or index not ready yet.\n");
                    break;
                }
                printf("--- Statistics Shown Here ---\n");
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

            case 0:
                printf("Exiting program...\n");
                break;

            default:
                printf("Unknown option. Choose between 0 and 9.\n");
        }
    } while (choice != 0);

    return 0;
}