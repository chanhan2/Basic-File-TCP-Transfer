/*
    Simple TCP Server
*/

/* C library */
#include <limits.h>
#include <fcntl.h>
/* #include <stdbool.h> // performance of data structure is unknown/untested */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

/* external library */
#include "server.h"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void transmission_error(int check, int sockfd) {
    if (check < 0) {
        printf("Transmission failure - Missing/Currupted Package content\n");
        close(sockfd);
        exit(0);
    }
}

int start_tcp_server(char *port) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("ERROR opening socket: ");
        exit(1);
    }

    int on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        perror("setsockopt -- REUSEADDR: ");
        exit(1);
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));
    portno = atoi(port/*argv[1]*/);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    printf("Listening on %d\n\n", portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind: ");
        exit(1);
    }

    if (listen(sockfd, 5) == -1) {
        perror("listen: ");
        exit(1);
    }
    return sockfd;
}

tcp_content *get_package_content_replay(int socket, tcp_content *package_reply) {
    int nbytes = 0;
    if (!package_reply) error("Memory allocation overflow error\n");
    while (((nbytes += recv(socket, package_reply, sizeof(tcp_content), 0)) > 0) && (nbytes != sizeof(tcp_content)));
    return (nbytes == sizeof(tcp_content)) ? package_reply : NULL;
}

repo_tcp *get_package_repo_replay(int socket, repo_tcp *package_reply) {
    int nbytes = 0;
    if (!package_reply) error("Memory allocation overflow error\n");
    while (((nbytes += recv(socket, package_reply, sizeof(repo_tcp), 0)) > 0) && (nbytes != sizeof(repo_tcp)));
    return (nbytes == sizeof(repo_tcp)) ? package_reply : NULL;
}

int length(char *array) {
    int size = 0;
    while(array && array[size] != '\0') size++;
    return size;
}

int compareHash(char *hash_f1, char *hash_f2) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) if (hash_f1[i] != hash_f2[i]) break;
    return i == HASH_SIZE;
}

void packageReply(int socket, char command) {
    tcp_content *package_call = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!package_call) {
        perror("Memory allocation overflow error: \n");
        return;
    }
    package_call->command = command;
    transmission_error(tcp_package(socket, package_call, sizeof(tcp_content), 0), socket);
    free(package_call);
}

int update_file_permission(char *file, mode_t permission) {
    if (chmod(file, permission) < 0) {
        printf("rip in aids again my friend\n");
        printf("Error on file -> %s\n", file);
        return 0;
    }
    return 1;
}

int symlink_resolve(char *file, char *symlink, int tries) {
    if (remove(symlink) == 0) {
        if (link_symlink(file, symlink, tries) == 0) {
            unlink(file);
            printf("Could not resolve symlink() for file %s\n", symlink);
            return 0;
        } else printf("%s file deleted successfully.\n", symlink);
    } else {
        printf("Unable to delete the file\n");
        perror("Following error occurred\n");
        return 0;
    }
    return 1;
}

int link_symlink(char *pathname, char *slink, int tries) {
    if (tries <= 0) return 0;
    if (symlink(pathname, slink) != 0) {
        printf("symlink() error happended...\n Attempting quick fix...");
        return symlink_resolve(pathname, slink, tries-1);
    }
    return 1;
}

/* need to check for transmission -- buggy */
int tcp_package(int socket, void *package, size_t length, int flag) {
    tcp_content *ptr = (tcp_content*) package;
    while (length > 0) {
        int i = send(socket, ptr, length, flag);
        if (i < 1) return -1;
        ptr += i, length -= i;
    }
    return 1;
}

int send_package(int socket, void *buffer, size_t length) {
    char *ptr = (char*) buffer;
    while (length > 0) {
        int i = send(socket, ptr, length, 0);
        if (i < 1) return 0;
        ptr += i, length -= i;
    }
    return 1;
}

void relay_message (int socket) {
    char buffer[256];
    bzero(buffer,256);
    int count = 0, total = 0;

    while (((count = recv(socket, &buffer[total], (sizeof(buffer)) - count, 0)) > 0)) {
        printf("Message has %d bytes and the content is: %s \n", count, &buffer[total]);
        total += count;
        if (!(strcmp(&buffer[total], "endo") == 0) && (total == sizeof(buffer))) break;
    }

    if (count < 0) error("ERROR reading from socket: ");
    printf("Here is the message: %s\n",buffer);

    int i;
    for (i = 0; i < 5; i++) {
        int b = (i != 4) ? send_package(socket,"I got your message",18) : send_package(socket,"Final ending...",15);
        if (!b) error("ERROR writing to socket: ");
        printf("something happened\n");
    }
}

void closeBufferStream(FILE **p) {
    fclose(*p), *p = NULL;
}

void directory_storage(int socket) {
    repo_tcp *client_repo_tcp = (repo_tcp*)malloc(sizeof(repo_tcp) + 1);
    if (!client_repo_tcp) {
        packageReply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return;
    }

    if (!get_package_repo_replay(socket, client_repo_tcp)) {
        error("Could not recieve message request from client: ");
    } else {
        struct stat st;
        if (stat(client_repo_tcp->client_repo, &st) == -1) {
            printf("Creating new server side client directory '%s'\n", client_repo_tcp->client_repo);
            if (mkdir(client_repo_tcp->client_repo, client_repo_tcp->permission) == -1) { 
                packageReply(socket, 'E');
                perror("Could not create directory: \n");
                return;
            }
        } else printf("Updating server side client directory '%s'\n", client_repo_tcp->client_repo);
        char origin_repo[PATH_MAX + 1];
        strcpy(origin_repo, client_repo_tcp->client_repo);
        strcat(origin_repo, client_repo_tcp->origin);
        if (stat(origin_repo, &st) == -1) {
            printf("Creating new server side directory '%s' in '%s'\n\n", client_repo_tcp->origin, client_repo_tcp->client_repo);
            printf("Transferring '%s' to '%s'\n", client_repo_tcp->origin, client_repo_tcp->client_repo);
            if (mkdir(origin_repo, client_repo_tcp->permission) == -1) {
                packageReply(socket, 'E');
                perror("Could not create directory: \n");
                return;
            }
        } else printf("Updating server side directory on '%s'\n\n", origin_repo);
        packageReply(socket, 'S');
    }
    free(client_repo_tcp);
}

void saveFile (int socket) {
    directory_storage(socket);
    tcp_content *info;
    if (!(info = (tcp_content*)malloc(sizeof(tcp_content) + 1))) {
        packageReply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return;
    }

    FILE *t = NULL;
    while ((info->command != 'Q')) {
        if (!get_package_content_replay(socket, info) || info->file_type == 'E') {
            printf("Connection with client is terminated, due to some interruption.\n");
            break;
        }
        if (info->file_type == '_') {
            struct stat statRes;
            if (lstat(info->filename, &statRes) == 0) {
                FILE *fp = fopen(info->filename, "rb");
                char *file_hash = hash(fp);
                closeBufferStream(&fp);
                if (compareHash(info->hash, file_hash) && (info->size == statRes.st_size)) {
                    printf("skip package update for file '%s'\n", info->filename);
                    packageReply(socket, 'S');
                    continue;
                } else {
                    printf("Writing/Updating package for storage '%s'\n", info->filename);
                    packageReply(socket, 'T');
                }
                free(file_hash);
            } else packageReply(socket, 'T');
            if (!t) t = fopen(info->filename, "w+b");
            if(t == 0) {
                printf("rip in aids my friend\n");
                printf("Error on file -> %s\n", info->filename);
            }
            if (update_file_permission(info->filename, info->permission) == 0) break;
        } else if (info->file_type == 'd') {
            struct stat st;
            if (stat(info->filename, &st) == -1) {
                if (mkdir(info->filename, info->permission) == -1) {
                    printf("Could not transfer directory '%s'\n", info->filename);
                    packageReply(socket, 'E');
                    continue;
                }
                printf("Transferring directory '%s'\n", info->filename);
                packageReply(socket, 'S');
            } else {
                printf("Updating directory '%s'\n", info->filename);
                packageReply(socket, 'S');
            }
        } else if (info->file_type == '~') {
            printf("Linking %s to %s as a symlink\n", info->filename, info->ln_filename);
            if (link_symlink(info->ln_filename, info->filename, 3) == 1) {
                printf("Linked %s to %s as a symlink\n", info->filename, info->ln_filename);
            } else printf("Could not linked %s to %s as a symlink\n", info->filename, info->ln_filename);
        } else if (info->file_type == ' ') {
            closeBufferStream(&t);
            printf("Finished file transfer for %s\n", info->filename);
        } else if (info->file_type == 'f') {
            decryptContent(&info->content, info->content_size);
            //fputs(info->content, t);
            fputc(info->content, t);
        }
    }
    if (info->command == 'Q') {
        printf("*\n*\n*\nCompleted client request\n\n");
        packageReply(socket, 'C');
    } else {
        printf("TRANSMIT_ERROR\n\n");
        packageReply(socket, 'E');
    }
    free(info);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    int sockfd = start_tcp_server(argv[1]), newsockfd, pid;
    socklen_t clilen;
    struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) perror("ERROR on accept: ");
        pid = fork();
        if (pid < 0) perror("ERROR on fork: ");
        if (pid == 0)  {
            close(sockfd);
            saveFile(newsockfd); //relay_message(newsockfd);
            exit(0);
        } else close(newsockfd);
    }
    close(sockfd);
    return 0;
}
