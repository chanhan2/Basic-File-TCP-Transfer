#ifndef _EPACKAGE_TCP_
#define _EPACKAGE_TCP_

/* C library */
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

/*
    Function prototype of TCP

    Emit and read/create package content of inode
*/
void operation_error(const char *msg);
void error(int socket, int status, const char *msg);
int connect_tcp(char *host, int port);
char get_reply(int socket);
void end_tcp(int sockfd, user_side user);
//void transmission_request(int check, int sockfd);
void service_error(const char *msg, int socket, user_side user);
void service_failure(const char *msg);
void connection_server_error(const char *msg);
void printUseless(int indent);
void mod_path(const char *dest, const char *file, char *path, int shift);
int file_signature(const char *file, char *dest);
void copy_hash(char *array, const char *hash);
void transfer_file(const char *file, const char *src, const char *dest, struct stat statRes, int socket, int shift);
void tcp_directory(const char *file, const char *src, const char *dest, struct stat statRes, int socket, int shift);
void listdir(int socket, int shift, const char *name, const char *dest);
void relayer(int socket);
char *concat(const char *s1, const char *s2);

#endif // _EPACKAGE_TCP_
