/**
 * @file    stemmer.c
 * @brief   Porter stemmer implementation (see stemmer.h).
 *
 * Terminology from the paper:
 *   - A consonant is a letter other than a,e,i,o,u, and other than 'y'
 *     preceded by a consonant.
 *   - Any word has the form [C](VC)^m[V]; m is the "measure" of the stem.
 *   - *v*  : the stem contains a vowel.
 *   - *d   : the stem ends with a double consonant.
 *   - *o   : the stem ends consonant-vowel-consonant, where the final
 *            consonant is not w, x or y (e.g. -wil, -hop).
 * The word is held in b[0..k]; j marks the end of the stem when a suffix
 * has been matched by ends().
 *
 * @module  Documents & Text Processing (Member B)
 */
#include "stemmer.h"

#include <string.h>

typedef struct {
    char *b;   /* the word buffer               */
    int   k;   /* index of the last character   */
    int   j;   /* end of stem after ends()      */
} Stem;

/* 1 if b[i] is a consonant. */
static int cons(const Stem *z, int i)
{
    switch (z->b[i]) {
    case 'a': case 'e': case 'i': case 'o': case 'u':
        return 0;
    case 'y':
        return i == 0 ? 1 : !cons(z, i - 1);
    default:
        return 1;
    }
}

/* Measure m of b[0..j]: number of VC sequences. */
static int measure(const Stem *z)
{
    int n = 0, i = 0, j = z->j;
    for (;;) {                       /* skip initial consonants */
        if (i > j) return n;
        if (!cons(z, i)) break;
        i++;
    }
    i++;
    for (;;) {
        for (;;) {                   /* skip vowels */
            if (i > j) return n;
            if (cons(z, i)) break;
            i++;
        }
        i++;
        n++;                         /* completed one VC */
        for (;;) {                   /* skip consonants */
            if (i > j) return n;
            if (!cons(z, i)) break;
            i++;
        }
        i++;
    }
}

/* *v* : b[0..j] contains a vowel. */
static int vowel_in_stem(const Stem *z)
{
    int i;
    for (i = 0; i <= z->j; i++)
        if (!cons(z, i)) return 1;
    return 0;
}

/* *d : b[i-1..i] is a double consonant. */
static int double_cons(const Stem *z, int i)
{
    if (i < 1 || z->b[i] != z->b[i - 1]) return 0;
    return cons(z, i);
}

/* *o : b[i-2..i] is consonant-vowel-consonant and b[i] is not w, x, y. */
static int cvc(const Stem *z, int i)
{
    char ch;
    if (i < 2 || !cons(z, i) || cons(z, i - 1) || !cons(z, i - 2)) return 0;
    ch = z->b[i];
    return !(ch == 'w' || ch == 'x' || ch == 'y');
}

/* Does b[0..k] end with s? If so, j = end of the remaining stem. */
static int ends(Stem *z, const char *s)
{
    int len = (int)strlen(s);
    if (len > z->k + 1) return 0;
    if (memcmp(z->b + z->k - len + 1, s, (size_t)len) != 0) return 0;
    z->j = z->k - len;
    return 1;
}

/* Replace b[j+1..k] by s and adjust k. */
static void set_to(Stem *z, const char *s)
{
    int len = (int)strlen(s);
    memmove(z->b + z->j + 1, s, (size_t)len);
    z->k = z->j + len;
}

/* Replace the suffix only if the stem's measure m > 0. */
static void replace_m0(Stem *z, const char *s)
{
    if (measure(z) > 0) set_to(z, s);
}

/* Step 1ab: plurals and -ed / -ing.
 *   caresses -> caress, ponies -> poni, cats -> cat
 *   agreed -> agree, plastered -> plaster, motoring -> motor, hopping -> hop */
static void step1ab(Stem *z)
{
    char *b = z->b;
    if (b[z->k] == 's') {
        if (ends(z, "sses"))              z->k -= 2;
        else if (ends(z, "ies"))          set_to(z, "i");
        else if (b[z->k - 1] != 's')      z->k--;
    }
    if (ends(z, "eed")) {
        if (measure(z) > 0) z->k--;
    } else if ((ends(z, "ed") || ends(z, "ing")) && vowel_in_stem(z)) {
        z->k = z->j;
        if (ends(z, "at"))       set_to(z, "ate");
        else if (ends(z, "bl"))  set_to(z, "ble");
        else if (ends(z, "iz"))  set_to(z, "ize");
        else if (double_cons(z, z->k)) {
            char ch;
            z->k--;
            ch = b[z->k];
            if (ch == 'l' || ch == 's' || ch == 'z') z->k++;
        } else if (measure(z) == 1 && cvc(z, z->k)) {
            set_to(z, "e");
        }
    }
}

/* Step 1c: terminal y -> i when there is another vowel (happy -> happi). */
static void step1c(Stem *z)
{
    if (ends(z, "y") && vowel_in_stem(z)) z->b[z->k] = 'i';
}

/* Step 2: double suffixes -> single ones (m > 0).
 *   relational -> relate, conditional -> condition, digitizer -> digitize */
static void step2(Stem *z)
{
    if (z->k < 1) return;
    switch (z->b[z->k - 1]) {
    case 'a':
        if (ends(z, "ational")) { replace_m0(z, "ate");  break; }
        if (ends(z, "tional"))  { replace_m0(z, "tion"); break; }
        break;
    case 'c':
        if (ends(z, "enci")) { replace_m0(z, "ence"); break; }
        if (ends(z, "anci")) { replace_m0(z, "ance"); break; }
        break;
    case 'e':
        if (ends(z, "izer")) { replace_m0(z, "ize"); break; }
        break;
    case 'l':
        if (ends(z, "bli"))   { replace_m0(z, "ble"); break; }
        if (ends(z, "alli"))  { replace_m0(z, "al");  break; }
        if (ends(z, "entli")) { replace_m0(z, "ent"); break; }
        if (ends(z, "eli"))   { replace_m0(z, "e");   break; }
        if (ends(z, "ousli")) { replace_m0(z, "ous"); break; }
        break;
    case 'o':
        if (ends(z, "ization")) { replace_m0(z, "ize"); break; }
        if (ends(z, "ation"))   { replace_m0(z, "ate"); break; }
        if (ends(z, "ator"))    { replace_m0(z, "ate"); break; }
        break;
    case 's':
        if (ends(z, "alism"))   { replace_m0(z, "al");  break; }
        if (ends(z, "iveness")) { replace_m0(z, "ive"); break; }
        if (ends(z, "fulness")) { replace_m0(z, "ful"); break; }
        if (ends(z, "ousness")) { replace_m0(z, "ous"); break; }
        break;
    case 't':
        if (ends(z, "aliti"))  { replace_m0(z, "al");  break; }
        if (ends(z, "iviti"))  { replace_m0(z, "ive"); break; }
        if (ends(z, "biliti")) { replace_m0(z, "ble"); break; }
        break;
    case 'g':
        if (ends(z, "logi")) { replace_m0(z, "log"); break; }
        break;
    default:
        break;
    }
}

/* Step 3: -ic-, -full, -ness etc. (m > 0).
 *   triplicate -> triplic, hopeful -> hope, goodness -> good */
static void step3(Stem *z)
{
    switch (z->b[z->k]) {
    case 'e':
        if (ends(z, "icate")) { replace_m0(z, "ic"); break; }
        if (ends(z, "ative")) { replace_m0(z, "");   break; }
        if (ends(z, "alize")) { replace_m0(z, "al"); break; }
        break;
    case 'i':
        if (ends(z, "iciti")) { replace_m0(z, "ic"); break; }
        break;
    case 'l':
        if (ends(z, "ical")) { replace_m0(z, "ic"); break; }
        if (ends(z, "ful"))  { replace_m0(z, "");   break; }
        break;
    case 's':
        if (ends(z, "ness")) { replace_m0(z, ""); break; }
        break;
    default:
        break;
    }
}

/* Step 4: remove -ant, -ence, -ment ... when m > 1.
 *   revival -> reviv, allowance -> allow, adoption -> adopt */
static void step4(Stem *z)
{
    if (z->k < 1) return;
    switch (z->b[z->k - 1]) {
    case 'a': if (ends(z, "al")) break; return;
    case 'c': if (ends(z, "ance") || ends(z, "ence")) break; return;
    case 'e': if (ends(z, "er")) break; return;
    case 'i': if (ends(z, "ic")) break; return;
    case 'l': if (ends(z, "able") || ends(z, "ible")) break; return;
    case 'n':
        if (ends(z, "ant") || ends(z, "ement") || ends(z, "ment") || ends(z, "ent")) break;
        return;
    case 'o':
        if (ends(z, "ion") && z->j >= 0 && (z->b[z->j] == 's' || z->b[z->j] == 't')) break;
        if (ends(z, "ou")) break;
        return;
    case 's': if (ends(z, "ism")) break; return;
    case 't': if (ends(z, "ate") || ends(z, "iti")) break; return;
    case 'u': if (ends(z, "ous")) break; return;
    case 'v': if (ends(z, "ive")) break; return;
    case 'z': if (ends(z, "ize")) break; return;
    default:  return;
    }
    if (measure(z) > 1) z->k = z->j;
}

/* Step 5: tidy up a final -e and -ll.
 *   probate -> probat, rate -> rate, controll -> control */
static void step5(Stem *z)
{
    z->j = z->k;
    if (z->b[z->k] == 'e') {
        int a = measure(z);
        if (a > 1 || (a == 1 && !cvc(z, z->k - 1))) z->k--;
    }
    if (z->b[z->k] == 'l' && double_cons(z, z->k) && measure(z) > 1) z->k--;
}

int porter_stem(char *word)
{
    Stem z;
    int  len = (int)strlen(word);
    if (len <= 2) return len;     /* the algorithm leaves short words alone */
    z.b = word;
    z.k = len - 1;
    z.j = 0;
    step1ab(&z);
    if (z.k > 0) {
        step1c(&z);
        step2(&z);
        step3(&z);
        step4(&z);
        step5(&z);
    }
    word[z.k + 1] = '\0';
    return z.k + 1;
}
