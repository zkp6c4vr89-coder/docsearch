/**
 * @file    util.c
 * @brief   Implementation of the shared utilities declared in util.h.
 *
 * @module  Core (Team Leader)
 */
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ======================================================================
 *  Tracked memory allocation
 * ----------------------------------------------------------------------
 *  Layout of every block:   [ MemHeader | user bytes ... ]
 *  The header is a union with max_align_t so the user pointer keeps the
 *  strictest alignment malloc() would have given.
 * ====================================================================== */
typedef union {
    size_t      size;
    max_align_t align_;
} MemHeader;

static size_t g_mem_current = 0;
static size_t g_mem_peak    = 0;
static size_t g_mem_blocks  = 0;

static void out_of_memory(size_t size)
{
    fprintf(stderr, "Fatal: out of memory (requested %lu bytes)\n",
            (unsigned long)size);
    exit(EXIT_FAILURE);
}

static void mem_account_add(size_t size)
{
    g_mem_current += size;
    g_mem_blocks++;
    if (g_mem_current > g_mem_peak) g_mem_peak = g_mem_current;
}

void *xmalloc(size_t size)
{
    MemHeader *h = (MemHeader *)malloc(sizeof(MemHeader) + size);
    if (!h) out_of_memory(size);
    h->size = size;
    mem_account_add(size);
    return h + 1;
}

void *xcalloc(size_t count, size_t size)
{
    size_t total;
    void  *p;
    if (size != 0 && count > (size_t)-1 / size) out_of_memory((size_t)-1);
    total = count * size;
    p = xmalloc(total);
    memset(p, 0, total);
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    MemHeader *old, *h;
    size_t     old_size;
    if (!ptr) return xmalloc(size);
    old      = (MemHeader *)ptr - 1;
    old_size = old->size;
    h = (MemHeader *)realloc(old, sizeof(MemHeader) + size);
    if (!h) out_of_memory(size);
    h->size = size;
    g_mem_current = g_mem_current - old_size + size;
    if (g_mem_current > g_mem_peak) g_mem_peak = g_mem_current;
    return h + 1;
}

void xfree(void *ptr)
{
    MemHeader *h;
    if (!ptr) return;
    h = (MemHeader *)ptr - 1;
    g_mem_current -= h->size;
    g_mem_blocks--;
    free(h);
}

char *xstrdup(const char *s)
{
    return xstrndup(s, strlen(s));
}

char *xstrndup(const char *s, size_t n)
{
    char *p = (char *)xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

size_t mem_current(void)     { return g_mem_current; }
size_t mem_peak(void)        { return g_mem_peak; }
void   mem_reset_peak(void)  { g_mem_peak = g_mem_current; }
size_t mem_live_blocks(void) { return g_mem_blocks; }

/* ======================================================================
 *  Timing
 * ----------------------------------------------------------------------
 *  timespec_get() is standard C11 and gives wall-clock time with
 *  nanosecond resolution. clock() is the C89 fallback (CPU time).
 * ====================================================================== */
double time_now_ms(void)
{
#if defined(TIME_UTC)
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) == TIME_UTC)
        return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
#endif
    return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

/* ======================================================================
 *  Input helpers
 * ====================================================================== */
int read_line(FILE *in, char *buf, size_t size)
{
    size_t len;
    if (size == 0) return 0;
    if (!fgets(buf, (int)size, in)) {
        buf[0] = '\0';
        return 0;
    }
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        /* line longer than the buffer: discard the rest of it */
        int c;
        while ((c = fgetc(in)) != EOF && c != '\n') { /* skip */ }
    }
    if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0'; /* Windows CRLF */
    return 1;
}

int parse_long(const char *s, long min, long max, long *out)
{
    char *end;
    long  v;
    if (!s) return 0;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0' || v < min || v > max) return 0;
    *out = v;
    return 1;
}

int parse_double(const char *s, double min, double max, double *out)
{
    char  *end;
    double v;
    if (!s) return 0;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    errno = 0;
    v = strtod(s, &end);
    if (errno != 0) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0' || v < min || v > max) return 0;
    *out = v;
    return 1;
}

/* ======================================================================
 *  String helpers
 * ====================================================================== */
char *str_trim(char *s)
{
    char  *start = s;
    size_t len;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

int str_casecmp(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int str_ends_with_ci(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    if (lx > ls) return 0;
    return str_casecmp(s + ls - lx, suffix) == 0;
}

void str_copy(char *dst, size_t size, const char *src)
{
    size_t n;
    if (size == 0) return;
    n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void str_tolower(char *s)
{
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* ======================================================================
 *  StrBuf: amortised O(1) append by doubling capacity
 * ====================================================================== */
void sb_init(StrBuf *sb)  { sb->data = NULL; sb->len = 0; sb->cap = 0; }
void sb_free(StrBuf *sb)  { xfree(sb->data); sb_init(sb); }
void sb_clear(StrBuf *sb) { sb->len = 0; if (sb->data) sb->data[0] = '\0'; }

static void sb_reserve(StrBuf *sb, size_t extra)
{
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) return;
    if (sb->cap == 0) sb->cap = 64;
    while (sb->cap < need) sb->cap *= 2;
    sb->data = (char *)xrealloc(sb->data, sb->cap);
}

void sb_append_n(StrBuf *sb, const char *s, size_t n)
{
    sb_reserve(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_append(StrBuf *sb, const char *s) { sb_append_n(sb, s, strlen(s)); }

void sb_append_char(StrBuf *sb, char c) { sb_append_n(sb, &c, 1); }

void sb_appendf(StrBuf *sb, const char *fmt, ...)
{
    va_list ap;
    int     n;
    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);   /* C99: measure first */
    va_end(ap);
    if (n < 0) return;
    sb_reserve(sb, (size_t)n);
    va_start(ap, fmt);
    vsnprintf(sb->data + sb->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    sb->len += (size_t)n;
}

char *sb_take(StrBuf *sb)
{
    char *d = sb->data;
    if (!d) d = xstrdup("");
    sb_init(sb);
    return d;
}

/* ======================================================================
 *  Files & paths
 * ====================================================================== */
char *read_file(const char *path, size_t *len_out)
{
    FILE  *fp;
    long   size;
    char  *buf;
    size_t got;

    fp = fopen(path, "rb");
    if (!fp) return NULL;
    /* fseek/ftell give the size so we can allocate exactly once */
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    buf = (char *)xmalloc((size_t)size + 1);
    got = fread(buf, 1, (size_t)size, fp);
    if (ferror(fp)) {
        xfree(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    buf[got] = '\0';
    if (len_out) *len_out = got;
    return buf;
}

const char *path_basename(const char *path)
{
    const char *p = path, *base = path;
    for (; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    return base;
}

void path_join(char *out, size_t size, const char *dir, const char *name)
{
    size_t n = strlen(dir);
    if (n == 0)
        snprintf(out, size, "%s", name);
    else if (dir[n - 1] == '/' || dir[n - 1] == '\\')
        snprintf(out, size, "%s%s", dir, name);
    else
        snprintf(out, size, "%s/%s", dir, name);
}

/* ======================================================================
 *  Hashing & PRNG
 * ====================================================================== */
uint32_t hash_fnv1a(const void *data, size_t len, uint32_t seed)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t h = seed;
    size_t   i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;           /* FNV 32-bit prime */
    }
    return h;
}

uint32_t hash_str(const char *s)
{
    uint32_t h = FNV_OFFSET_BASIS;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}

uint32_t rng_next(uint32_t *state)
{
    uint32_t x = *state ? *state : 2463534242u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}
