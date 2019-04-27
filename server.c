/*
    TCP Server
*/

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

#define __SECRET__ "d4a885a0a9390eb86edec9653cc7dae57a1520968fa89c57c33da4a50e2312f192c3e18f31edd2e3668e19e8e3a0e3effab1402653d40c07f52b2fbc9506ce06"

/* function prototype */
void relay_message(int socket);
bool send_package(int socket, void *buffer, size_t length);
int symlink_resolve(char *file, char *symlink, int tries);
int link_symlink(char *file, char *symlink, int tries);
int length(char *array);

typedef struct {
    char file_type;
    char filename[256];
    char content;
    char ln_filename[256];
    int content_size;
    mode_t permission;
} TCP_Content;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int length(char *array) {
    int size = 0;
    while(array[size] != '\0') {
        size++;
    }
    return size;
}

void encryptContent(char *array, int *content_size) {
    char *secret = __SECRET__;
    int size = strlen(array);
    *content_size = size;
    int i;
    for(i = 0; i < size; i++)
        array[i] ^= secret[i % 8];
}

void decryptContent(char *array, int size) {
    char *secret = __SECRET__;
    int i;
    for(i = 0; i < size; i++)
        array[i] ^= secret[i % 8];
}

int symlink_resolve(char *file, char *symlink, int tries) {
    if (remove(symlink) == 0) {
        if (link_symlink(file, symlink, tries) == 0) {
            unlink(file);
            printf("Could not resolve symlink() for file %s\n", symlink);
            return 0;
        } else {
            printf("%s file deleted successfully.\n", symlink);
        }
    } else {
        printf("Unable to delete the file\n");
        perror("Following error occurred");
        return 0;
    }
    return 1;
}

int link_symlink(char *pathname, char *slink, int tries) {
    if (tries <= 0) return 0;
    if (symlink(pathname, slink) != 0) {
        unlink(pathname);
        printf("symlink() error happended...\n Attempting quick fix...");
        return symlink_resolve(pathname, slink, tries-1);
    }
    return 1;
}

bool send_package(int socket, void *buffer, size_t length){
    char *ptr = (char*) buffer;
    while (length > 0) {
        int i = send(socket, ptr, length, 0);
        if (i < 1) return false;
        ptr += i;
        length -= i;
    }
    return true;
}

void relay_message (int socket) {
    char buffer[256];
    bzero(buffer,256);
    int count = 0, total = 0;

    while (((count = recv(socket, &buffer[total], (sizeof(buffer)) - count, 0)) > 0)) {
        printf("Message has %d bytes and the content is: %s \n", count, &buffer[total]);
        total += count;
        if (!(strcmp(&buffer[total], "endo") == 0)) break;
    }

    if (count < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);

    int i;
    for (i = 0; i < 5; i++) {
        bool b;
        if (i != 4) {
            b = send_package(socket,"I got your message",18);
        } else {
            b = send_package(socket,"Final ending...",15);
        }
        if (!b) error("ERROR writing to socket");
        printf("something happened\n");
    }
}

void saveFile (int socket) {
    TCP_Content info;
    int count = 0, total = 0;
    FILE *t;

    while (((count = recv(socket, (TCP_Content *)&info, sizeof(TCP_Content), 0)) > 0)) {
        total += count;
        if (info.file_type == '_') {
            t = fopen(info.filename, "w+b");
            if(t == 0) {
                printf("rip in aids my friend\n");
                printf("Error on file -> %s\n", info.filename);
            }
            if (chmod(info.filename, info.permission) < 0) {
                printf("rip in aids again my friend\n");
                printf("Error on file -> %s\n", info.filename);
            }
        } else if (info.file_type == 'd') {
            struct stat st;
            if (stat(info.filename, &st) == -1) {
                mkdir(info.filename, info.permission);
            }
        } else if (info.file_type == ' ') {
            if (strcmp(info.ln_filename, "\0") != 0) {
                if (link_symlink(info.ln_filename, info.filename, 3) == 1) {
                    printf("Linked %s to %s as a symlink\n", info.filename, info.ln_filename);
                } else printf("Could not linked %s to %s as a symlink\n", info.filename, info.ln_filename);
            } else fclose(t);
            printf("Finished file transfer for %s\n", info.filename);
        } else {
            //decryptContent(info.content, info.content_size);
            //fputs(info.content, t);
            fputc(info.content, t);
        }
    }
    if (count < 0) error("ERROR reading from socket");
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) perror("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) perror("ERROR on accept");
        pid = fork();
        if (pid < 0) perror("ERROR on fork");
        if (pid == 0)  {
            close(sockfd);
            saveFile(newsockfd); //relay_message(newsockfd);
            exit(0);
        } else close(newsockfd);
    }
    close(sockfd);
    return 0;
}
