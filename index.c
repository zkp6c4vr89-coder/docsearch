// index code (Zoyal Ahmed Khan)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define HEADERLINE "DOCSEARCH_INDEX 1\n"
#define MAX_DOCS 1000000

static char *copy_string(const char *s)
{
    char *c = malloc(strlen(s) + 1);
    if (c != NULL) {
        strcpy(c, s);
    }
    return c;
}

static unsigned int hash_word(const char *word) //hash function to turn word into the bucket number
{
    unsigned long total = 0;
    for(const char *p = word; *p != '\0'; p++){
        total = (total*31) + (unsigned char)*p;
    }
    return (unsigned int)(total % NUM_BUCKETS);
}

void index_init(Index *idx) //initialzing the index
{
    for(int i = 0; i < NUM_BUCKETS; i++){
        (*idx).buckets[i] = NULL;
    }
    (*idx).unique_terms = 0;
    (*idx).posting_count = 0;
    (*idx).docs = NULL;
    (*idx).num_docs = 0;
    (*idx).doc_capacity = 0;
}

int index_add_document(Index *idx, const char *path) 
{
    if((*idx).num_docs == (*idx).doc_capacity){
        int new_cap;
        if ((*idx).doc_capacity == 0) {
            new_cap = 16;
        } else {
            new_cap = (*idx).doc_capacity * 2;
        }
        Document *bigger = realloc((*idx).docs, new_cap * sizeof(Document));
        if(bigger == NULL){
            return -1;
        }
        (*idx).docs = bigger;
        (*idx).doc_capacity = new_cap;
    }
    Document *d = &(*idx).docs[(*idx).num_docs];
    (*d).id = (*idx).num_docs;
    (*d).total_terms = 0;
    (*d).unique_terms = 0;
    if (strlen(path) >= MAX_PATH_LEN){
        return -1;
    }
    strcpy((*d).path, path);
    (*idx).num_docs++;
    return (*d).id;
}

Term *index_lookup(const Index *idx, const char *word) //finding the word in the index
{
    unsigned int b = hash_word(word); //finding the bucket number for the word
    for (Term *t = (*idx).buckets[b]; t != NULL; t = (*t).next){ // finding the word in the buckets list
        if (strcmp((*t).text, word) == 0){
            return t; 
        }
    }
    return NULL; //word is not in the index
}

static Term *create_term(Index *idx, const char *word) //creating new term and putting it at the start of the list inside the bucket
{
    Term *t = malloc(sizeof(Term));
    if(t == NULL){
        return NULL;
    }
    (*t).text = copy_string(word);
    if ((*t).text == NULL) {
        free(t);
        return NULL;
    }
    (*t).first = NULL;
    (*t).last = NULL;
    (*t).doc_count = 0;

    unsigned int b = hash_word(word);
    (*t).next = (*idx).buckets[b]; //new term points at old first term
    (*idx).buckets[b] = t; //bucket points at new term as new term is now the starting term
    (*idx).unique_terms++;
    return t;
}

static Posting *append_posting(Index *idx, Term *t, int doc_id, int freq) //using last pointer to attach new posting to end of terms list
{
    Posting *p = malloc(sizeof(Posting));
    if (p == NULL){
        return NULL;
    }
    (*p).doc_id = doc_id;
    (*p).freq = freq;
    (*p).next = NULL; //to show end of list to prevent crashing when going through the list
    if ((*t).last == NULL){
        (*t).first = p; 
    } else {
        (*(*t).last).next = p;
    }
    (*t).last = p;
    (*t).doc_count++;
    (*idx).posting_count++;
    return p;
}

int index_add_term(Index *idx, const char *word, int doc_id) //adding term to index
{
    if (doc_id < 0 || doc_id >= (*idx).num_docs){
        return -1;
    }
    Term *t = index_lookup(idx, word); 
    if (t==NULL){ 
        t=create_term(idx,word); 
        if (t==NULL){
            return -1;
        }
    }
    
    if ((*t).last != NULL && (*(*t).last).doc_id == doc_id) { //checking if term is found in the doc
        (*(*t).last).freq++;
        return 0; //term found so we stop here
    }

    if (append_posting(idx, t, doc_id, 1) == NULL){ 
        return -1;
    }
    (*idx).docs[doc_id].unique_terms++;
    return 0;
}

long index_total_terms(const Index *idx) 
{
    long sum = 0;
    for (int i = 0; i< (*idx).num_docs; i++){
        sum += (*idx).docs[i].total_terms;
    }
    return sum;
}

int index_check_frequency_sum(const Index *idx) //checking postings with total words to check for words lost or counted twice
{
    long sum = 0;
    for (int b = 0; b< NUM_BUCKETS; b++){ //go through every bucket
        for (Term *t = (*idx).buckets[b]; t != NULL; t = (*t).next){ //go through the list in the bucket
            for (Posting *p = (*t).first; p != NULL; p = (*p).next){ //go through list of postings for the term
                sum += (*p).freq; //how many times the term was in the doc
            }
        }
    }
    return sum == index_total_terms(idx);
}

void index_free(Index *idx) //freeing the memory malloc takes
{
    for (int b = 0; b <NUM_BUCKETS; b++) {
        Term *t = (*idx).buckets[b];
        while (t != NULL) {
            Posting *p = (*t).first;
            while (p != NULL) {
                Posting *next_p = (*p).next;
                free(p);
                p = next_p;
            }
            Term *next_t = (*t).next;
            free((*t).text);
            free(t);
            t = next_t;
        }
        (*idx).buckets[b] = NULL;
    }
    free((*idx).docs); //reset to empty state
    (*idx).docs = NULL;
    (*idx).num_docs = 0;
    (*idx).doc_capacity=0;
    (*idx).unique_terms = 0;
    (*idx).posting_count= 0;
}

int index_save(const Index *idx, const char *filename) //saving index into a file
{
    FILE *f = fopen(filename, "w");
    if (f==NULL) {
        fprintf(stderr, "Error! Cant open '%s' for writing", filename);
        return -1;
    }
    fputs(HEADERLINE, f);
    fprintf(f, "documents %d\n", (*idx).num_docs);
    for(int i= 0; i< (*idx).num_docs; i++){
        const Document *d = &(*idx).docs[i];
        fprintf(f, "%d %d %d %s\n", (*d).id, (*d).total_terms, (*d).unique_terms, (*d).path);
    }
    fprintf(f, "terms %d\n", (*idx).unique_terms);
    for(int b = 0; b < NUM_BUCKETS; b++){
        for (Term *t = (*idx).buckets[b]; t != NULL; t = (*t).next){
            fprintf(f, "%s %d", (*t).text, (*t).doc_count);
            for (Posting *p = (*t).first; p != NULL; p = (*p).next) {
                fprintf(f, " %d %d", (*p).doc_id, (*p).freq);
            }
            fputc('\n', f);
        }
    }
    int failed = ferror(f); //checking if inputing into file succeeded
    if(fclose(f) != 0) {
        failed = 1;
    }
    if (failed){
        fprintf(stderr, "Error! Failed at inputing to file '%s'\n", filename);
        return -1;
    }
    return 0;
}

static int read_index_file(FILE *f, Index *tmp, const char **error){ //loading into temp index and also checking for errors
    char line[2048];
    int num_docs, num_terms;
    if (fgets(line, sizeof line, f) == NULL || strcmp(line, HEADERLINE) != 0){ //rejecting empty or unrelated files
        *error = "error: not valid index file";
        return -1;
    }
    if (fscanf(f, "documents %d", &num_docs)!= 1 || num_docs <0 || num_docs > MAX_DOCS){ //reading failed or doc count was negative or too big
        *error = "error: doc count";
        return -1;
    }
    for(int i=0; i< num_docs; i++){
        int id, total, unique;
        if(fscanf(f, "%d %d %d ", &id, &total, &unique) !=3 || id != i || total <0 || unique<0){ //doc line in wrong format or id not in order or counts for total and unique are negative
            *error = "error: doc line";
            return -1;
        }
        if(fgets(line, sizeof line, f) == NULL){ //file was cut off
            *error = "error: file cut off";
            return -1;
        }
        line[strcspn(line, "\r\n")] = '\0'; //striping new line
        if(index_add_document(tmp, line) < 0){ //malloc failed
            *error = "error: out of memory";
            return -1;
        }
        (*tmp).docs[i].total_terms = total;
        (*tmp).docs[i].unique_terms = unique;
    }
    if(fscanf(f, " terms %d", &num_terms) != 1 || num_terms <0){ //term count iss there and is not negative
        *error = "error: term count";
        return -1;
    }
    long freq_sum = 0;
    for (int i = 0; i < num_terms; i++){
        char word [MAX_WORD + 1];
        int n;
        if (fscanf(f, "%255s %d", word, &n) != 2 || n<1 || n> num_docs){ //255 limit so word doesnt overflow
            *error = "error: term line or file cut off";
            return -1;
        }
        if(index_lookup(tmp, word) != NULL){
            *error = "error: duplicate term";
            return -1;
        }
        Term *t = create_term(tmp, word);
        if (t == NULL) { //malloc failed
            *error = "error: out of memory";
            return -1;
        }
        int prev_doc = -1;
        for (int j = 0; j < n; j++){
            int d, freq;
            if (fscanf(f, "%d %d", &d, &freq) != 2){ //posting should be 2 numbers, if not it is damaged or file was cut off
                *error = "error; posting or file cut off";
                return -1;
            }
            if (d <0 || d>= num_docs || d<= prev_doc || freq <1){ //checking ids in range, id in order, freq atleast 1,
                *error = "error: invalid doc ID or frequency";
                return -1;
            }
            if(append_posting(tmp, t, d, freq) == NULL){ //if null then malloc failed
                *error = "error; out of memory";
                return -1;
            }
            prev_doc = d;
            freq_sum += freq;
        }
    }
    if ((*tmp).unique_terms != num_terms || freq_sum != index_total_terms(tmp)) { //check if number of terms and freq match the docs
        *error = "error: terms/freq doesnt match file";
        return -1;
    }
    return 0;
}

int index_load(Index *idx, const char *filename) 
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open '%s'\n", filename);
        return -1;
    }
    Index *tmp = malloc(sizeof(Index)); //writing into temp index first
    if (tmp == NULL) { //if malloc fails we close the file
        fclose(f);
        return -1;
    }
    index_init(tmp);

    const char *error = NULL;
    int result = read_index_file(f, tmp, &error);
    fclose(f);

    if (result != 0) { //there was a problem with the file
        fprintf(stderr, "error loading '%s': %s\n", filename, error);
        index_free(tmp);
        free(tmp);
        return -1;
    }
    index_free(idx); 
    *idx = *tmp; //loading all into main index because now we know it is safe to do so
    free(tmp);
    return 0;

}