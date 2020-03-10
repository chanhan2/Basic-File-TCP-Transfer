#ifndef _FNSTRUCT_
#define _FNSTRUCT_

/* external library */
#include "hash.h"

/* Package/Data Structures */
typedef enum operation {
    UPLOAD,
    DOWNLOAD
} operation_request;

typedef struct {
    operation_request req;
} request;

typedef struct {
    char client_repo[256];
    char origin[256];
    mode_t permission;
} tcp_repo;

typedef struct {
    char command;
    char file_type;
    char filename[256];
    //char content[256];
    char content;
    char ln_filename[256];
    int content_size;
    char hash[HASH_SIZE];
    struct stat inode_info;
} tcp_content;

#endif // _FNSTRUCT_
