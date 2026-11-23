#include <string.h>
#include "common.h"


// Split the search into words
static int query_words(const char *query, char words[][MAX_WORD_LEN], int max_words)
{
    const char *p = query;
    char word[MAX_WORD_LEN];
    int n = 0;

    while (next_token(&p, word)) {
        //skip if already have word
        int dup = 0;
        for (int i = 0; i < n; i++) {
            if (strcmp(words[i], word) == 0) {
                dup = 1;
                break;
            }
        }
        if (dup) {
            continue;
        }

        if (n == max_words) {
            fprintf(stderr, "Warning: query has more than %d words; extra ignored\n",
                    max_words);
            break;
        }

        strcpy(words[n], word);
        n++;
    }
    return n;
}

int linear_scan(const Document *docs, int count, const char *query, int *scores)
{
    char words[MAX_QUERY_WORDS][MAX_WORD_LEN];
    int n = query_words(query, words, MAX_QUERY_WORDS);

    for (int d = 0; d < count; d++) {
        scores[d] = 0;
        if (n == 0) {
            continue;
        }

        
        // read document from disk again
        char *text = read_whole_file(docs[d].path, NULL);
        if (text == NULL) {
            continue;
        }

        // count every token that is equal to the query word
        const char *p = text;
        char word[MAX_WORD_LEN];
        while (next_token(&p, word)) {
            for (int i = 0; i < n; i++) {
                if (strcmp(word, words[i]) == 0) {
                    scores[d]++;
                    break;
                }
            }
        }

        free(text);
    }
    return n;
}