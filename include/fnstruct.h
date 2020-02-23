#ifndef _FNSTRUCT_
#define _FNSTRUCT_

/* Package/Data Structures */
typedef struct {
    char client_repo[256];
    char origin[256];
    mode_t permission;
} repo_tcp;

typedef struct {
    char command;
    char file_type;
    char filename[256];
    //char content[256];
    char content;
    char ln_filename[256];
    int content_size;
    mode_t permission;
    size_t size;
    char hash[HASH_SIZE];
    struct stat inodeInfo;
} tcp_content;

#endif // _FNSTRUCT_
