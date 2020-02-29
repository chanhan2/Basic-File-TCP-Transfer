#ifndef _SPACKAGE_TCP_
#define _SPACKAGE_TCP_

/* C library */
#include <sys/stat.h>

/* external library */
#include "fnstruct.h"

/*
    Function prototype of TCP

    Recieve and save package content of inode
*/
void connection_error(const char *msg);
int start_tcp_server(int port);
void transmission_error(int check, int sockfd);
int tcp_package(int socket, void *package, size_t length, int flag, int type);
tcp_content *get_package_content_replay(int socket, tcp_content *package_reply);
repo_tcp *get_package_repo_replay(int socket, repo_tcp *package_reply);
void relay_message(int socket);
void closeBufferStream(FILE **p);
void directory_storage(int socket);
void saveFile (int socket);
void packageReply(int socket, char command);
int update_file_permission(char *file, mode_t permission);
int send_package(int socket, void *buffer, size_t length);
int symlink_resolve(char *file, char *symlink, int tries);
int link_symlink(char *file, char *symlink, int tries);
void updateInodeMetadata(struct stat info, const char *filename);
int length(char *array);
int compareHash(char *hash_f1, char *hash_f2);

#endif // _SPACKAGE_TCP_
