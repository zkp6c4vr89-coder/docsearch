/**
 * @file    config.c
 * @brief   Configuration defaults, parsing and saving (see config.h).
 *
 * File format (docsearch.conf):
 *     # comment
 *     key = value
 * Unknown keys and invalid values are reported and ignored, so a broken
 * config file can never crash the program.
 *
 * @module  Core (Team Leader)
 */
#include "config.h"
#include "util.h"

#include <string.h>

void config_defaults(Config *cfg)
{
    memset(cfg, 0, sizeof *cfg);
    str_copy(cfg->data_dir, sizeof cfg->data_dir, "data/cranfield/cran.all.1400.xml");
    str_copy(cfg->stopword_file, sizeof cfg->stopword_file, "data/stopwords.txt");
    cfg->recursive     = 1;
    cfg->use_stopwords = 1;
    cfg->use_stemming  = 1;
    cfg->dict_type     = DICT_HASH;
    cfg->model         = SCORE_BM25;
    cfg->bm25_k1       = 1.2;
    cfg->bm25_b        = 0.75;
    cfg->top_k         = 10;
    cfg->max_wildcard_expansions = 50;
    cfg->show_snippets = 1;
    cfg->snippet_words = 24;
#ifdef _WIN32
    cfg->color         = 0;  /* legacy Windows consoles show escape codes */
#else
    cfg->color         = 1;
#endif
    str_copy(cfg->output_dir,  sizeof cfg->output_dir,  "output");
    str_copy(cfg->index_file,  sizeof cfg->index_file,  "output/index.dsx");
    str_copy(cfg->report_file, sizeof cfg->report_file, "output/report.txt");
    str_copy(cfg->results_csv, sizeof cfg->results_csv, "output/search_results.csv");
    str_copy(cfg->queries_file, sizeof cfg->queries_file, "data/cranfield/cran.qry.xml");
    str_copy(cfg->qrels_file,   sizeof cfg->qrels_file,   "data/cranfield/cranqrel.trec.txt");
    cfg->eval_k   = 10;
    cfg->gui_port = 8080;
}

/* ---------- enum <-> string ---------- */
const char *score_model_name(ScoreModel m)
{
    switch (m) {
    case SCORE_TF:    return "tf";
    case SCORE_TFIDF: return "tfidf";
    case SCORE_BM25:  return "bm25";
    }
    return "?";
}

int score_model_parse(const char *s, ScoreModel *out)
{
    if (str_casecmp(s, "tf") == 0)                                   *out = SCORE_TF;
    else if (str_casecmp(s, "tfidf") == 0 || str_casecmp(s, "tf-idf") == 0) *out = SCORE_TFIDF;
    else if (str_casecmp(s, "bm25") == 0)                            *out = SCORE_BM25;
    else return 0;
    return 1;
}

const char *dict_type_name(DictType t)
{
    switch (t) {
    case DICT_HASH:   return "hash";
    case DICT_SORTED: return "sorted";
    case DICT_LIST:   return "list";
    }
    return "?";
}

int dict_type_parse(const char *s, DictType *out)
{
    if (str_casecmp(s, "hash") == 0)        *out = DICT_HASH;
    else if (str_casecmp(s, "sorted") == 0) *out = DICT_SORTED;
    else if (str_casecmp(s, "list") == 0)   *out = DICT_LIST;
    else return 0;
    return 1;
}

static int parse_bool(const char *v, int *out)
{
    if (!str_casecmp(v, "1") || !str_casecmp(v, "on") || !str_casecmp(v, "yes") || !str_casecmp(v, "true"))
        *out = 1;
    else if (!str_casecmp(v, "0") || !str_casecmp(v, "off") || !str_casecmp(v, "no") || !str_casecmp(v, "false"))
        *out = 0;
    else
        return 0;
    return 1;
}

/* ---------- table-driven setter ---------- */
int config_set(Config *cfg, const char *key, const char *value)
{
    long   l;
    double d;

#define SET_STR(name) \
    if (str_casecmp(key, #name) == 0) { str_copy(cfg->name, sizeof cfg->name, value); return 1; }
#define SET_BOOL(name) \
    if (str_casecmp(key, #name) == 0) return parse_bool(value, &cfg->name);
#define SET_INT(name, lo, hi) \
    if (str_casecmp(key, #name) == 0) { \
        if (!parse_long(value, lo, hi, &l)) { return 0; } \
        cfg->name = (int)l; \
        return 1; \
    }
#define SET_DBL(name, lo, hi) \
    if (str_casecmp(key, #name) == 0) { \
        if (!parse_double(value, lo, hi, &d)) { return 0; } \
        cfg->name = d; \
        return 1; \
    }

    SET_STR(data_dir)
    SET_STR(stopword_file)
    SET_STR(output_dir)
    SET_STR(index_file)
    SET_STR(report_file)
    SET_STR(results_csv)
    SET_STR(queries_file)
    SET_STR(qrels_file)
    SET_BOOL(recursive)
    SET_BOOL(use_stopwords)
    SET_BOOL(use_stemming)
    SET_BOOL(show_snippets)
    SET_BOOL(color)
    SET_INT(top_k, 1, 1000)
    SET_INT(max_wildcard_expansions, 1, 1000)
    SET_INT(snippet_words, 5, 200)
    SET_INT(eval_k, 1, 1000)
    SET_INT(gui_port, 1024, 65535)
    SET_DBL(bm25_k1, 0.0, 10.0)
    SET_DBL(bm25_b, 0.0, 1.0)
    if (str_casecmp(key, "model") == 0)     return score_model_parse(value, &cfg->model);
    if (str_casecmp(key, "dict_type") == 0) return dict_type_parse(value, &cfg->dict_type);
    return 0;

#undef SET_STR
#undef SET_BOOL
#undef SET_INT
#undef SET_DBL
}

int config_load(Config *cfg, const char *path, FILE *log)
{
    FILE *fp = fopen(path, "r");
    char  line[MAX_LINE_LEN];
    int   lineno = 0;

    if (!fp) return 0;
    while (read_line(fp, line, sizeof line)) {
        char *eq, *key, *val;
        lineno++;
        str_trim(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;
        eq = strchr(line, '=');
        if (!eq) {
            if (log) fprintf(log, "  config %s:%d: missing '=' (ignored)\n", path, lineno);
            continue;
        }
        *eq = '\0';
        key = str_trim(line);
        val = str_trim(eq + 1);
        if (!config_set(cfg, key, val) && log)
            fprintf(log, "  config %s:%d: invalid setting '%s = %s' (ignored)\n",
                    path, lineno, key, val);
    }
    fclose(fp);
    return 1;
}

int config_save(const Config *cfg, const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return 0;
    fprintf(fp, "# %s configuration file (key = value). Lines starting with # are comments.\n", DS_NAME);
    config_print(cfg, fp);
    fclose(fp);
    return 1;
}

void config_print(const Config *cfg, FILE *out)
{
    fprintf(out, "data_dir                = %s\n", cfg->data_dir);
    fprintf(out, "recursive               = %d\n", cfg->recursive);
    fprintf(out, "stopword_file           = %s\n", cfg->stopword_file);
    fprintf(out, "use_stopwords           = %d\n", cfg->use_stopwords);
    fprintf(out, "use_stemming            = %d\n", cfg->use_stemming);
    fprintf(out, "dict_type               = %s\n", dict_type_name(cfg->dict_type));
    fprintf(out, "model                   = %s\n", score_model_name(cfg->model));
    fprintf(out, "bm25_k1                 = %.2f\n", cfg->bm25_k1);
    fprintf(out, "bm25_b                  = %.2f\n", cfg->bm25_b);
    fprintf(out, "top_k                   = %d\n", cfg->top_k);
    fprintf(out, "max_wildcard_expansions = %d\n", cfg->max_wildcard_expansions);
    fprintf(out, "show_snippets           = %d\n", cfg->show_snippets);
    fprintf(out, "snippet_words           = %d\n", cfg->snippet_words);
    fprintf(out, "color                   = %d\n", cfg->color);
    fprintf(out, "output_dir              = %s\n", cfg->output_dir);
    fprintf(out, "index_file              = %s\n", cfg->index_file);
    fprintf(out, "report_file             = %s\n", cfg->report_file);
    fprintf(out, "results_csv             = %s\n", cfg->results_csv);
    fprintf(out, "queries_file            = %s\n", cfg->queries_file);
    fprintf(out, "qrels_file              = %s\n", cfg->qrels_file);
    fprintf(out, "eval_k                  = %d\n", cfg->eval_k);
    fprintf(out, "gui_port                = %d\n", cfg->gui_port);
}
