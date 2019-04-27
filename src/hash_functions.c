#include <stdio.h>
#include <stdlib.h>

#include "hash.h"

#ifndef MAX_BLOCK_SIZE
    #define MAX_BLOCK_SIZE 1024
#endif

/*
 * Computes the hash bit-wise on each indivduial char characters
 * given by the user.
 */

char *hash(FILE *f) {
    char *hash_val = malloc(sizeof(char) * MAX_BLOCK_SIZE);
    char c;
    int place = 0;
    while ((c = fgetc(f)) != EOF) hash_val[place++ % HASH_SIZE] ^= c;
    return hash_val;
}