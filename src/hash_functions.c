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
    char *hash_val;
    if (!(hash_val = (char*)malloc(sizeof(char) * MAX_BLOCK_SIZE))) {
    	printf("ERROR: Memory allocation overflow error --- Could not compute hash\n");
    	return NULL;
    }
    for (int i = 0; i < MAX_BLOCK_SIZE; hash_val[i++] = '\0');

    char c;
    int place = 0;
    while ((c = fgetc(f)) != EOF)
    	if (place != HASH_SIZE) hash_val[place++ % HASH_SIZE] ^= c;
    	else hash_val[(place = 0) % HASH_SIZE] ^= c;
    return hash_val;
}
