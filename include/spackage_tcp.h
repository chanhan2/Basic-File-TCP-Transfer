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
tcp_repo *get_package_repo_replay(int socket, tcp_repo *package_reply);
request *get_package_request_replay(int socket, request *package_reply);
void relay_message(int socket);
void close_buffer_stream(FILE **p);
void directory_storage(int socket);
void save_file (int socket);
void client_request(int socket);
void package_reply(int socket, const char command);
int update_file(const char content, FILE *inodeFd);
int update_file_permission(const char *file, mode_t permission);
int send_package(int socket, void *buffer, size_t length);
int symlink_resolve(const char *file, const char *symlink, int tries);
int link_symlink(const char *file, const char *symlink, int tries);
void update_inode_meta_data(struct stat info, const char *filename);
int length(char *array);
int compare_hash(char *hash_f1, char *hash_f2);

#endif // _SPACKAGE_TCP_
