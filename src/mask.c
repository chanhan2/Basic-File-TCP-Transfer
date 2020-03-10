/* C library */
#include <string.h>
#include <stdio.h>

/* external library */
#include "mask.h"

int byte_sum(const char *byte) {
    if (*byte == '\0') return 0;

    int byteTotal = DEFAULT_KEY;
    int i;
    for (i = 0; *(byte + i) != '\0'; byteTotal ^= (int)(*(byte + i++)));
    return byteTotal % strlen(__SECRET__);
}

void encrypt_content(char *array, const int s_byte) {
    int i;
    for(i = 0; i < MASKLEN; *array ^= __SECRET__[i++ % s_byte]);
}

void decrypt_content(char *array, const int s_byte) {
    int i;
    for(i = 0; i < MASKLEN; *array ^= __SECRET__[i++ % s_byte]);
}
