#ifndef _MASK_
#define _MASK_

/* macro */
#define __SECRET__ "d4a885a0a9390eb86edec9653cc7dae57a1520968fa89c57c33da4a50e2312f192c3e18f31edd2e3668e19e8e3a0e3effab1402653d40c07f52b2fbc9506ce06"

/*
    function prototype for mask

    todo: size is package content byte reference
*/
void encryptContent(char *array, int *s_byte);
void decryptContent(char *array, int s_byte);

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

#endif // _MASK_
