// Test harness for the tokenizer, analyzer, indexer and index save/load.
// This is the permanent test program, separate from the menu in main.c.
// Build it with `make test` and run it as ./search_test
// The default list file names below are hard-coded on purpose, for testing only.

#include <string.h>
#include "common.h"
#include "indexer.h"
#include "analyzer.h"

#define CHECK_FILE "index_check.tmp"

// the Index holds 10007 bucket pointers, too big to keep on the stack
static Index idx;
static Index loaded;

static void test(const char *text)
{
    const char *p = text;
    char word[MAX_WORD_LEN];

    printf("\"%s\" ->", text);
    while (next_token(&p, word)) {
        printf(" [%s]", word);
    }
    printf("\n");
}

// same text through the analyzer: next_token, then stopwords and stemming
static void test_analyzer(const Analyzer *a, const char *text)
{
    Tokenizer t;
    char term[MAX_WORD_LEN];

    tokenizer_init(&t, text, strlen(text));
    printf("\"%s\" ->", text);
    while (analyzer_next(a, &t, term, NULL, NULL, NULL)) {
        printf(" [%s]", term);
    }
    printf("\n");
}

// loads one list of files, indexes it, then saves and reloads the index
// returns 0 if everything worked
static int run_collection(const char *list_path)
{
    int count = 0;
    int ok = 1;

    printf("\n== %s ==\n", list_path);
    Document *docs = load_collection(list_path, &count);
    if (docs == NULL) {
        return 1;
    }

    index_init(&idx);
    index_init(&loaded);

    if (build_index(&idx, docs, count) < 0) {
        ok = 0;
    }

// only print every document for small collections
    if (count <= 10) {
        for (int i = 0; i < count; i++) {
            printf("  %s: %d total terms, %d unique\n",
                   docs[i].path, docs[i].total_terms, docs[i].unique_terms);
        }
    }
    printf("Indexed %d documents: %ld total terms, %d unique terms, %d postings\n",
           idx.num_docs, index_total_terms(&idx), idx.unique_terms, idx.posting_count);

    if (!index_check_frequency_sum(&idx)) {
        printf("FAIL: frequencies do not add up to the total terms\n");
        ok = 0;
    }

// save, load it back and compare
    if (ok && (index_save(&idx, CHECK_FILE) != 0 || index_load(&loaded, CHECK_FILE) != 0)) {
        printf("FAIL: index save/load\n");
        ok = 0;
    }
    if (ok && (loaded.num_docs != idx.num_docs ||
               loaded.unique_terms != idx.unique_terms ||
               loaded.posting_count != idx.posting_count ||
               index_total_terms(&loaded) != index_total_terms(&idx))) {
        printf("FAIL: loaded index is different from the saved one\n");
        ok = 0;
    }
    if (ok) {
        printf("Index save -> load OK\n");
    }
    remove(CHECK_FILE);

    index_free(&idx);
    index_free(&loaded);
    free(docs);
    return ok ? 0 : 1;
}

int main(int argc, char *argv[])
{
    int failed = 0;

// usage: ./search_test              tokenizer tests, then the default lists below
//        ./search_test <list> ...   tokenizer tests, then each list of file paths given
    printf("== tokenizer ==\n");
    test("Hello, World!");
    test("renewable-energy");
    test("   ");
    test("");
    test("don't");
    test("word");
    test("Solar power, widely-used!");

    printf("\n== analyzer (stopwords + stemming) ==\n");
    Analyzer a;
    analyzer_init(&a, 1, 1, "data/stopwords.txt");
    test_analyzer(&a, "Solar power, widely-used!");
    test_analyzer(&a, "The connections are connected");
    analyzer_free(&a);

    for (int i = 1; i < argc; i++) {
        failed += run_collection(argv[i]);
    }

// no lists given: use the default ones that exist on this machine
    if (argc == 1) {
        const char *defaults[] = { "files_sample.txt", "files_cran.txt" };
        for (int i = 0; i < 2; i++) {
            FILE *fp = fopen(defaults[i], "r");
            if (fp == NULL) {
                printf("\n(no %s here, skipped)\n", defaults[i]);
                continue;
            }
            fclose(fp);
            failed += run_collection(defaults[i]);
        }
    }
    return failed ? 1 : 0;
}
