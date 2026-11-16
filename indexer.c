#include "indexer.h"

int build_index(Index *idx, Document *docs, int count)
{
    char word[MAX_WORD_LEN];

    for (int i = 0; i < count; i++) {
// register the document first so its id matches its position
        int id = index_add_document(idx, docs[i].path);
        if (id < 0) {
            fprintf(stderr, "Error: could not add '%s' to the index\n", docs[i].path);
            return -1;
        }

// a file that cannot be read stays in the index with 0 terms, so ids do not shift
        char *text = read_whole_file(docs[i].path, NULL);
        if (text == NULL) {
            continue;
        }

// one word at a time
        const char *p = text;
        while (next_token(&p, word)) {
// count every token before adding it, the index checks its frequencies against this
            idx->docs[id].total_terms++;
            if (index_add_term(idx, word, id) != 0) {
                fprintf(stderr, "Error: out of memory while indexing '%s'\n", docs[i].path);
                free(text);
                return -1;
            }
        }
        free(text);

// keep the caller's copy in step with the index
        docs[i].total_terms = idx->docs[id].total_terms;
        docs[i].unique_terms = idx->docs[id].unique_terms;
    }
    return count;
}
