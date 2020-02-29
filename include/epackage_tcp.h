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
void connection_error(const char *msg);
void error(int socket, int status, const char *msg);
int connect_tcp(char *host, int port);
char get_replay(int socket);
void end_tcp(int sockfd);
void transmission_error(int check, int sockfd);
void service_error(const char *msg);
void connection_server_error(const char *msg);
void printUseless(int indent);
void mod_path(const char *origin, char *dest, char *file, char *path, int shift);
void file_signature(char *file, char *dest);
void copyHash(char *array, char *hash);
void transfer_file(char *file, const char *origin, const char *src, char *dest, struct stat statRes, int socket, int shift);
void tcp_directory(char *file, const char *origin, const char *src, char *dest, struct stat statRes, int socket, int shift);
void listdir(int socket, int shift, const char *origin, const char *name, char *dest);
void relayer(int socket);
char *concat(const char *s1, const char *s2);

#endif // _EPACKAGE_TCP_
