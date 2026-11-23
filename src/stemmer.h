/**
 * @file    stemmer.h
 * @brief   Porter (1980) suffix-stripping stemmer.
 *
 * Reduces English words to a common stem so that, for example,
 * "connect", "connected", "connecting" and "connections" all become
 * "connect". Implemented from the published algorithm:
 *   M.F. Porter, "An algorithm for suffix stripping", Program 14(3), 1980.
 *
 * @module  Documents & Text Processing (Member B)
 */
#ifndef DS_STEMMER_H
#define DS_STEMMER_H

/* Stems a lower-case ASCII word in place. Returns the new length.
 * Words of length <= 2 are left unchanged.                              */
int porter_stem(char *word);

#endif /* DS_STEMMER_H */
