/**
 * @file    util.h
 * @brief   Shared utilities: tracked memory allocation, timing, safe input,
 *          string helpers, a growable string buffer, hashing and a
 *          deterministic pseudo-random number generator.
 *
 * Memory tracking: every module allocates through xmalloc()/xcalloc()/
 * xrealloc()/xfree(). Each block carries a small hidden header recording
 * its size, which lets the program report current and peak heap usage
 * (needed for the memory benchmarks) using only standard C.
 *
 * @module  Core (Team Leader)
 */
#ifndef DS_UTIL_H
#define DS_UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/* ---------- tracked memory ---------- */
void  *xmalloc(size_t size);
void  *xcalloc(size_t count, size_t size);
void  *xrealloc(void *ptr, size_t size);
void   xfree(void *ptr);
char  *xstrdup(const char *s);
char  *xstrndup(const char *s, size_t n);

size_t mem_current(void);     /* bytes currently allocated               */
size_t mem_peak(void);        /* high-water mark since last reset         */
void   mem_reset_peak(void);  /* set peak = current                       */
size_t mem_live_blocks(void); /* number of live allocations (leak check)  */

/* ---------- timing ---------- */
double time_now_ms(void);     /* wall-clock milliseconds (monotonic enough) */

/* ---------- input ---------- */
/* Reads one line (without the newline). Over-long lines are truncated and
 * the remainder discarded. Returns 1 on success, 0 on EOF/error.          */
int    read_line(FILE *in, char *buf, size_t size);
/* Strict integer/double parsing: whole string must be a number in range. */
int    parse_long(const char *s, long min, long max, long *out);
int    parse_double(const char *s, double min, double max, double *out);

/* ---------- strings ---------- */
char  *str_trim(char *s);                         /* in place, returns s  */
int    str_casecmp(const char *a, const char *b); /* ASCII, like strcasecmp */
int    str_ends_with_ci(const char *s, const char *suffix);
void   str_copy(char *dst, size_t size, const char *src); /* always NUL-terminates */
void   str_tolower(char *s);

/* ---------- growable string buffer ---------- */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} StrBuf;

void   sb_init(StrBuf *sb);
void   sb_free(StrBuf *sb);
void   sb_clear(StrBuf *sb);
void   sb_append(StrBuf *sb, const char *s);
void   sb_append_n(StrBuf *sb, const char *s, size_t n);
void   sb_append_char(StrBuf *sb, char c);
void   sb_appendf(StrBuf *sb, const char *fmt, ...);
char  *sb_take(StrBuf *sb);   /* returns data (caller frees), resets sb */

/* ---------- files & paths ---------- */
/* Reads a whole file into a NUL-terminated buffer. Returns NULL on error. */
char  *read_file(const char *path, size_t *len_out);
const char *path_basename(const char *path);
void   path_join(char *out, size_t size, const char *dir, const char *name);

/* ---------- hashing & random ---------- */
uint32_t hash_fnv1a(const void *data, size_t len, uint32_t seed);
uint32_t hash_str(const char *s);
#define  FNV_OFFSET_BASIS 2166136261u

/* xorshift32: deterministic across compilers/platforms (unlike rand()). */
uint32_t rng_next(uint32_t *state);

#endif /* DS_UTIL_H */
