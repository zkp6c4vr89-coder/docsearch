#include <ctype.h>
#include "common.h"

int next_token(const char **pos, char *word)
{
    
    const char *p = *pos;

//skip everything that isn't a letter or digit 
    while (*p != '\0' && !isalnum((unsigned char)*p)) {
        p++;
    }

//reached the end with no word found 
    if (*p == '\0') {
        *pos = p;
        return 0;
    }

 //copy letters and digits all lowercased into the word buffer */
    int len = 0;
    while (isalnum((unsigned char)*p)) {
        if (len < MAX_WORD_LEN - 1) {
            word[len] = (char)tolower((unsigned char)*p);
            len++;
        }
        p++;   /* always move on, even when the word is too long to store */
    }

//End the word 
    word[len] = '\0';

    
    *pos = p;
    return 1;
}