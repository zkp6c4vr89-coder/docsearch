/**
 * @file    config.h
 * @brief   Run-time configuration loaded from a simple "key = value" file
 *          (docsearch.conf) and editable from the Settings menu.
 *
 * @module  Core (Team Leader)
 */
#ifndef DS_CONFIG_H
#define DS_CONFIG_H

#include <stdio.h>
#include "common.h"

typedef struct {
    /* input */
    char   data_dir[MAX_PATH_LEN];      /* folder or TREC file to load     */
    char   stopword_file[MAX_PATH_LEN]; /* optional; built-in list if ""   */
    int    recursive;                   /* scan sub-folders                */
    /* text processing */
    int    use_stopwords;
    int    use_stemming;
    /* indexing & ranking */
    DictType   dict_type;
    ScoreModel model;
    double bm25_k1;
    double bm25_b;
    int    top_k;
    int    max_wildcard_expansions;
    /* presentation */
    int    show_snippets;
    int    snippet_words;
    int    color;                       /* ANSI highlight in console       */
    /* output files */
    char   output_dir[MAX_PATH_LEN];
    char   index_file[MAX_PATH_LEN];
    char   report_file[MAX_PATH_LEN];
    char   results_csv[MAX_PATH_LEN];
    /* evaluation (Cranfield-style query set + relevance judgements) */
    char   queries_file[MAX_PATH_LEN];
    char   qrels_file[MAX_PATH_LEN];
    int    eval_k;
    /* web GUI */
    int    gui_port;
} Config;

void        config_defaults(Config *cfg);
/* Returns 1 on success, 0 if the file is missing; bad lines are reported
 * to `log` (may be NULL) and skipped, they never abort loading.          */
int         config_load(Config *cfg, const char *path, FILE *log);
int         config_save(const Config *cfg, const char *path);
/* Applies one "key=value" setting. Returns 1 if accepted.                */
int         config_set(Config *cfg, const char *key, const char *value);
void        config_print(const Config *cfg, FILE *out);

const char *score_model_name(ScoreModel m);
int         score_model_parse(const char *s, ScoreModel *out);
const char *dict_type_name(DictType t);
int         dict_type_parse(const char *s, DictType *out);

#endif /* DS_CONFIG_H */
