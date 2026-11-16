/**
 * @file    platform.h
 * @brief   The only OS-specific code in the project: listing the files in
 *          a folder and creating folders. ISO C has no directory API, so
 *          this module hides the Windows / POSIX differences behind one
 *          small portable interface.
 *
 * @module  Documents & Text Processing (Member B)
 */
#ifndef DS_PLATFORM_H
#define DS_PLATFORM_H

#include <stddef.h>

int  fs_is_dir(const char *path);
int  fs_is_file(const char *path);
long fs_file_size(const char *path);       /* -1 if not readable          */
int  fs_make_dir(const char *path);        /* 1 if it exists afterwards   */

/* Lists regular files under `dir` (recursing into sub-folders when
 * `recursive` is non-zero). Paths are returned sorted (strcmp order) so
 * document IDs are deterministic on every platform.
 * Returns 1 on success; free the list with fs_free_list().               */
int  fs_list_files(const char *dir, int recursive, char ***paths, size_t *count);
void fs_free_list(char **paths, size_t count);

#endif /* DS_PLATFORM_H */
