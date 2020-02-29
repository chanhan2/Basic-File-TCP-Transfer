#ifndef _MASK_
#define _MASK_

/* C library */
#include <string.h>

/*
    function prototype for mask

    ToDo: size is package content byte reference
*/
void encryptContent(char *array, int *s_byte);
void decryptContent(char *array, int s_byte);

#endif // _MASK_
