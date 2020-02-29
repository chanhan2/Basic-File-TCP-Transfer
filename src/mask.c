/* C library */
#include <string.h>

/* external library */
#include "mask.h"

void encryptContent(char *array, int *s_byte) {
    char *secret = __SECRET__;
    int size = strlen(__SECRET__);
    //*s_byte = size;
    *s_byte = -1;
    int i;
    for(i = 0; i < size; i++)
        *array ^= secret[i % 8];
}

void decryptContent(char *array, int s_byte) {
    char *secret = __SECRET__;
    s_byte = strlen(__SECRET__);
    int i;
    for(i = 0; i < s_byte; i++)
        *array ^= secret[i % 8];
}
