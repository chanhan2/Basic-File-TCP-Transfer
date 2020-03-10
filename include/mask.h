#ifndef _MASK_
#define _MASK_

/* C library */
#include <string.h>

/*
    function prototype for mask
*/
int byte_sum(const char *byte);
void encrypt_content(char *array, const int s_byte);
void decrypt_content(char *array, const int s_byte);

#endif // _MASK_
