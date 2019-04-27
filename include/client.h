#ifndef _CLIENT_
#define _CLIENT_

/*
    function prototype for TCP client
*/
void error(int socket, int status, const char *msg);
void transmission_error(int check, int sockfd);
void connection_error(const char *msg);
void end_tcp(int sockfd);
void printUseless(int indent);
void mod_path(const char *origin, char *dest, char *file, char *path, int shift);
void copyHash(char *array, char *hash);
void closeBufferStream(FILE **p);
int tcp_package(int socket, void *package, size_t length, int flag, int type);
void transfer_file(char *file, const char *origin, const char *src, char *dest, mode_t permission, int socket, int shift);
void tcp_directory(char *file, const char *origin, const char *src, char *dest, mode_t permission, int isLink, int socket, int shift);
void listdir(int socket, int shift, const char *origin, const char *name, char *dest, int indent);
void relayer(int socket);
char *concat(const char *s1, const char *s2);
int connect_tcp(char *host, char *port);

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

#endif // _CLIENT_
