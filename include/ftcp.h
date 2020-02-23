#ifndef _TCP_
#define _TCP_

/* C library */
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
/* #include <stdbool.h> // performance of data structure is unknown/untested */
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* external library */
#include "hash.h"
#include "mask.h"
#include "fnstruct.h"

#include "spackage_tcp.h"
#include "epackage_tcp.h"

#endif // _TCP_
