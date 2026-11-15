/**
 * @file    platform.c
 * @brief   Directory listing / folder creation for Windows and POSIX.
 *
 * @module  Documents & Text Processing (Member B)
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L   /* opendir/stat/mkdir under -std=c11 */
#endif

#include "platform.h"
#include "util.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/types.h>
#endif

int fs_is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

int fs_is_file(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFREG;
}

long fs_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long)st.st_size;
}

int fs_make_dir(const char *path)
{
    if (fs_is_dir(path)) return 1;
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
    return fs_is_dir(path);
}

/* ---------- growable list of path strings ---------- */
typedef struct {
    char  **items;
    size_t  count;
    size_t  cap;
} PathList;

static void pl_push(PathList *pl, const char *path)
{
    if (pl->count == pl->cap) {
        pl->cap   = pl->cap ? pl->cap * 2 : 64;
        pl->items = (char **)xrealloc(pl->items, pl->cap * sizeof *pl->items);
    }
    pl->items[pl->count++] = xstrdup(path);
}

static int cmp_str_ptr(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* Hidden files (".git", ".DS_Store") are never documents. */
static int is_hidden(const char *name) { return name[0] == '.'; }

static int list_rec(const char *dir, int recursive, PathList *pl, int depth)
{
    char full[MAX_PATH_LEN];
    if (depth > 32) return 1;                   /* guard against loops */
#ifdef _WIN32
    {
        char             pattern[MAX_PATH_LEN];
        WIN32_FIND_DATAA fd;
        HANDLE           h;
        path_join(pattern, sizeof pattern, dir, "*");
        h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) return 0;
        do {
            if (is_hidden(fd.cFileName)) continue;
            path_join(full, sizeof full, dir, fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (recursive) list_rec(full, recursive, pl, depth + 1);
            } else {
                pl_push(pl, full);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    {
        DIR           *d = opendir(dir);
        struct dirent *e;
        if (!d) return 0;
        while ((e = readdir(d)) != NULL) {
            if (is_hidden(e->d_name)) continue;
            path_join(full, sizeof full, dir, e->d_name);
            if (fs_is_dir(full)) {
                if (recursive) list_rec(full, recursive, pl, depth + 1);
            } else if (fs_is_file(full)) {
                pl_push(pl, full);
            }
        }
        closedir(d);
    }
#endif
    return 1;
}

int fs_list_files(const char *dir, int recursive, char ***paths, size_t *count)
{
    PathList pl = { NULL, 0, 0 };
    if (!list_rec(dir, recursive, &pl, 0)) {
        fs_free_list(pl.items, pl.count);
        return 0;
    }
    /* Sorting makes document IDs identical across operating systems,
     * which readdir()/FindNextFile() do not guarantee.                   */
    if (pl.count > 1) qsort(pl.items, pl.count, sizeof *pl.items, cmp_str_ptr);
    *paths = pl.items;
    *count = pl.count;
    return 1;
}

void fs_free_list(char **paths, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) xfree(paths[i]);
    xfree(paths);
}
