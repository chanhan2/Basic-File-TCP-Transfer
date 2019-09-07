#ifndef _SERVER_
#define _SERVER_

/*
    function prototype for TCP server
*/
void error(const char *msg);
int start_tcp_server(char *port);
void relay_message(int socket);
void closeBufferStream(FILE **p);
void saveFile (int socket);
void packageReply(int socket, char command);
int update_file_permission(char *file, mode_t permission);
int tcp_package(int socket, void *package, size_t length, int flag);
bool send_package(int socket, void *buffer, size_t length);
int symlink_resolve(char *file, char *symlink, int tries);
int link_symlink(char *file, char *symlink, int tries);
int length(char *array);
int compareHash(char *hash_f1, char *hash_f2);

/* external library */
#include "hash.h"
#include "mask.h"

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
} tcp_content;

#endif // _SERVER_
