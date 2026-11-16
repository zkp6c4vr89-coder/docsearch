#include <ctype.h>
#include <string.h>
#include "common.h"

// memory helpers, stop the program if there is no memory left

void *xmalloc(size_t size)
{
    // malloc(0) may return NULL, so always ask for at least 1 byte
    void *p = malloc(size > 0 ? size : 1);
    if (p == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size > 0 ? size : 1);
    if (p == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    return p;
}

char *xstrndup(const char *s, size_t n)
{
    char *copy = xmalloc(n + 1);
    memcpy(copy, s, n);
    copy[n] = '\0';
    return copy;
}

char *xstrdup(const char *s)
{
    return xstrndup(s, strlen(s));
}

void xfree(void *ptr)
{
    free(ptr);   /* free(NULL) is allowed */
}

// growable string buffer

void sb_init(StrBuf *sb)
{
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_append_n(StrBuf *sb, const char *s, size_t n)
{
    // +1 keeps room for the '\0'
    size_t needed = sb->len + n + 1;
    if (needed > sb->cap) {
        size_t new_cap = sb->cap > 0 ? sb->cap * 2 : 64;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        sb->data = xrealloc(sb->data, new_cap);
        sb->cap = new_cap;
    }
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_append(StrBuf *sb, const char *s)
{
    sb_append_n(sb, s, strlen(s));
}

void sb_append_char(StrBuf *sb, char c)
{
    sb_append_n(sb, &c, 1);
}

char *sb_take(StrBuf *sb)
{
    // an empty buffer still gives back a real (empty) string
    char *result = sb->data != NULL ? sb->data : xstrdup("");
    sb_init(sb);
    return result;
}

void sb_free(StrBuf *sb)
{
    free(sb->data);
    sb_init(sb);
}

// string helpers

void str_copy(char *out, size_t size, const char *src)
{
    if (size == 0) {
        return;
    }
    size_t n = strlen(src);
    if (n > size - 1) {
        n = size - 1;
    }
    memcpy(out, src, n);
    out[n] = '\0';
}

void str_trim(char *s)
{
    // skip spaces at the front
    char *start = s;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // cut spaces at the back
    size_t len = strlen(start);
    while (len > 0 && isspace((unsigned char)start[len - 1])) {
        len--;
    }

    memmove(s, start, len);
    s[len] = '\0';
}

void str_tolower(char *s)
{
    for (; *s != '\0'; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

int str_ends_with_ci(const char *s, const char *suffix)
{
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > s_len) {
        return 0;
    }
    s += s_len - suffix_len;
    for (size_t i = 0; i < suffix_len; i++) {
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)suffix[i])) {
            return 0;
        }
    }
    return 1;
}

// path helpers

void path_join(char *out, size_t size, const char *dir, const char *name)
{
    size_t dir_len = strlen(dir);
    // no extra separator if dir is empty or already ends with one ('/' works on Windows too)
    if (dir_len == 0 || dir[dir_len - 1] == '/' || dir[dir_len - 1] == '\\') {
        snprintf(out, size, "%s%s", dir, name);
    } else {
        snprintf(out, size, "%s/%s", dir, name);
    }
}

const char *path_basename(const char *path)
{
    const char *base = path;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

// file helpers

char *read_file(const char *path, size_t *out_len)
{
    // same job as read_whole_file in loader.c, only the size type differs
    long size = 0;
    char *buffer = read_whole_file(path, &size);
    if (buffer == NULL) {
        return NULL;
    }
    if (out_len != NULL) {
        *out_len = (size_t)size;
    }
    return buffer;
}

int read_line(FILE *fp, char *buf, size_t size)
{
    if (fgets(buf, (int)size, fp) == NULL) {
        return 0;
    }

    size_t len = strcspn(buf, "\r\n");
    if (buf[len] == '\0' && len == size - 1) {
        // line was longer than the buffer, throw away the rest of it
        int c;
        while ((c = fgetc(fp)) != EOF && c != '\n') {
        }
    }
    buf[len] = '\0';
    return 1;
}
