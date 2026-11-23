// metrics code (Zoyal Ahmed Khan)

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int qrels_load(QrelList *q, const char *filename){
    (*q).items = NULL;
    (*q).count = 0;
    FILE *f = fopen(filename, "r");
    if(f==NULL){
        fprintf(stderr, "error: cannot open file '%s'\n", filename);
        return -1;
    }
    int capacity = 0;
    char line[256];
    while(fgets(line, sizeof line, f) != NULL){
        int a,b,c,d;
        int n = sscanf(line, "%d %d %d %d", &a, &b, &c, &d);
        Qrel r;
        if(n==4){
            r.query = a;
            r.doc = c;
            r.rel = d;
        } 
        else if (n==3){
            r.query = a;
            r.doc = b;
            r.rel = c;
        }
        else continue; //to skip blank lines
        if((*q).count == capacity){ //make array bigger if full
            if(capacity == 0){
                capacity = 256;
            }
            else{
                capacity = capacity*2;
            }
            Qrel *bigger = realloc((*q).items, capacity*sizeof(Qrel));
            if (bigger ==NULL){
                fclose(f); 
                qrels_free(q);
                return -1;      
            }
            (*q).items = bigger;
        }
        (*q).items[(*q).count++] = r;
    }
    fclose(f);
    return 0;
}

int qrels_relevant_docs(const QrelList *q, int query, int *out, int max){ //returns how many relevant docs were found for the query
    int n=0;
    for(int i=0; i< (*q).count && n<max; i++){
        if ((*q).items[i].query == query && (*q).items[i].rel >= MIN_RELEVANCE){
            out[n++]=(*q).items[i].doc;
        }
    }
    return n;
}

void qrels_free(QrelList *q){ //freeing
    free((*q).items);
    (*q).items = NULL;
    (*q).count = 0;
}

static int is_relevant(int doc, const int *relevant, int n_relevant){ //checking if the doc is in the relevant list
    for (int i=0; i< n_relevant; i++){
        if(relevant[i] ==doc){
            return 1;
        }
    }
    return 0;
}

double precision_at_k(const int *results, int n_results, int k, const int *relevant, int n_relevant){ //checking how much of the top k is relevant, basically a precision check
    if(k<=0){
        return 0.0;
    }
    int found = 0;
    for (int i=0 ; i<k && i< n_results; i++){
        if(is_relevant(results[i], relevant, n_relevant)){
            found++;
        }
    }
    return (double)found/k;
}

double recall_at_k(const int *results, int n_results, int k, const int *relevant, int n_relevant){ //checking how many of the total relevant docs are in k
    if(n_relevant == 0) return 0.0;
    int found = 0;
    for(int i = 0; i < k && i< n_results; i++){
        if (is_relevant(results[i], relevant, n_relevant)){
            found++;
        }
    }
    return (double)found/n_relevant;
}

double  reciprocal_rank(const int *results, int n_results, const int *relevant, int n_relevant){ // checks and ranks how far down the first relevant answer is
    for(int i=0; i<n_results; i++){
        if(is_relevant(results[i], relevant, n_relevant)){
            return 1.0/(i+1);
        }
    }
    return 0.0;
}