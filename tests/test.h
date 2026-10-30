/**
 * @file    test.h
 * @brief   A minimal unit-test framework (no external libraries).
 *
 * CHECK(cond)              passes if cond is true
 * CHECK_INT(actual, exp)   integer equality, prints both values on failure
 * CHECK_STR(actual, exp)   string equality
 * CHECK_NEAR(a, b, eps)    floating-point equality within eps
 * Each failure prints file:line so it can be located immediately.
 *
 * @module  Core (Team Leader)
 */
#ifndef DS_TEST_H
#define DS_TEST_H

#include <math.h>
#include <stdio.h>
#include <string.h>

extern int g_checks;
extern int g_failures;

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failures++; \
    printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_INT(actual, expected) do { long a_ = (long)(actual), e_ = (long)(expected); \
    g_checks++; if (a_ != e_) { g_failures++; \
    printf("    FAIL %s:%d: %s = %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_, e_); } } while (0)

#define CHECK_STR(actual, expected) do { const char *a_ = (actual), *e_ = (expected); \
    g_checks++; if (!a_ || strcmp(a_, e_) != 0) { g_failures++; \
    printf("    FAIL %s:%d: %s = \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual, \
           a_ ? a_ : "(null)", e_); } } while (0)

#define CHECK_NEAR(actual, expected, eps) do { double a_ = (actual), e_ = (expected); \
    g_checks++; if (fabs(a_ - e_) > (eps)) { g_failures++; \
    printf("    FAIL %s:%d: %s = %.6f, expected %.6f\n", __FILE__, __LINE__, #actual, a_, e_); } } while (0)

/* Test suites, one per module owner. */
void suite_core(void);      /* Member A: util, config, report, web helpers */
void suite_text(void);      /* Member B: loader, tokenizer, stemmer, bench */
void suite_index(void);     /* Member C: dictionaries, index, persistence, eval */
void suite_search(void);    /* Member D: query, ranking, search, snippets  */

#define FIXTURES "tests/fixtures/"

#endif /* DS_TEST_H */
