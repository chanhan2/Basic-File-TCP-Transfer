/*
    Simple TCP Client
*/

/* macro */
#define __SECRET__ "d4a885a0a9390eb86edec9653cc7dae57a1520968fa89c57c33da4a50e2312f192c3e18f31edd2e3668e19e8e3a0e3effab1402653d40c07f52b2fbc9506ce06"

#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

/* function prototype */
void error(const char *msg);
void transmition_error(int check, int sockfd);
void end_tcp(int sockfd);
void printUseless(int indent);
void encryptContent(char *array, int *content_size);
void decryptContent(char *array, int size);
void transfer_file(char *file, mode_t permission, int isLink, int socket);
void tcp_directory(char *file, mode_t permission, int isLink, int socket);
void listdir(int socket, const char *name, int indent);
void relayer(int socket);
char *concat(const char *s1, const char *s2);
int connect_tcp(char *host, char *port);
int getSize(char *array);

typedef struct {
    char file_type;
    char filename[256];
    //char content[256];
    char content;
    char ln_filename[256];
    int content_size;
    mode_t permission;
} TCP_Content;

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void transmition_error(int check, int sockfd) {
    if (check < 0) {
        perror("Transmition failure ");
        close(sockfd);
        exit(0);
    }
}

int connect_tcp(char *host, char *port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno, sockfd;

    portno = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(host);

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) error("ERROR connecting");

    return sockfd;
}

void end_tcp(int sockfd) {
    TCP_Content info;
    strcpy(info.filename, "");
    //strcpy(info.content, "\n");
    info.content = '\n';
    info.permission = 0000;
    info.file_type = 'q';
    strcpy(info.ln_filename, "\0");
    int n = send(sockfd, (TCP_Content *)&info, sizeof(TCP_Content), 0);
    transmition_error(n, sockfd);
}

void printUseless(int indent) {
    printf("%*s%s\n", indent, "", ".");
    printf("%*s%s\n", indent, "", "..");
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

void transfer_file(char *file, mode_t permission, int isLink, int socket) {
    FILE *fs = fopen(file, "rb");
    if(fs == NULL) {
        printf("ERROR: File %s not found.\n", file);
        exit(1);
    }

    TCP_Content info;
    strcpy(info.filename, file);
    //strcpy(info.content, "\n");
    info.content = '\n';
    info.permission = permission;
    if (isLink == 1) {
        info.file_type = '\0';
        char buf[PATH_MAX + 1];
        char base[PATH_MAX + 1];
        realpath(file, buf);
        char *baseline = realpath("./", base);
        strcpy(info.ln_filename, &buf[strlen(baseline)]);
        printf("Base path is -> %s\n", &buf[strlen(baseline)]);
        printf("symlink path is -> %s\n", file);
    } else {
        info.file_type = '_';
        strcpy(info.ln_filename, "\0");
    }

    int n = send(socket, (TCP_Content *)&info, sizeof(TCP_Content), 0);
    transmition_error(n, socket);

    info.file_type = 'f';
    int ch;
    //while (fgets (info.content, 256, fs) != NULL) {
    while ((ch = fgetc(fs)) != EOF) {
        //encryptContent(info.content, &info.content_size);
        info.content = ch;
        n = send(socket, (TCP_Content *)&info, sizeof(TCP_Content), 0);
        transmition_error(n, socket);
    }
    info.file_type = ' ';
    send(socket, (TCP_Content *)&info, sizeof(TCP_Content), 0);
    transmition_error(n, socket);
    fclose(fs);
}

void tcp_directory(char *file, mode_t permission, int isLink, int socket) {
    TCP_Content info;
    strcpy(info.filename, file);
    //strcpy(info.content, "\n");
    info.content = '\n';
    info.permission = permission;
    if (isLink == 1) {
        info.file_type = '\0';
        char buf[PATH_MAX + 1];
        char base[PATH_MAX + 1];
        realpath(file, buf);
        char *baseline = realpath("./", base);
        strcpy(info.ln_filename, &buf[strlen(baseline)]);
        printf("Base path is -> %s\n", &buf[strlen(baseline)]);
        printf("symlink path is -> %s\n", file);
    } else {
        info.file_type = 'd';
        strcpy(info.ln_filename, "\0");
    }

    int n = send(socket, (TCP_Content *)&info, sizeof(TCP_Content), 0);
    transmition_error(n, socket);
}

void listdir(int socket, const char *name, int indent) {
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(name))) return;
    printUseless(indent);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            char *file = concat(name, entry->d_name);
            struct stat statRes;

            if (lstat(file, &statRes) < 0) {
                printf("ERROR: File stat is cannot be accessed for file %s.\n", file);
                continue;
            }

            if (S_ISLNK(statRes.st_mode) || !S_ISREG(statRes.st_mode)) continue;
            transfer_file(file, statRes.st_mode, 0, socket);
            printf("%*s%s\n", indent, "", file);
        }
    }

    if (!(dir = opendir(name))) return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            char *file = concat(name, entry->d_name);
            struct stat statRes;

            if (lstat(file, &statRes) < 0) {
                printf("ERROR: File stat is cannot be accessed for file %s.\n", file);
                continue;
            }

            if (!S_ISLNK(statRes.st_mode) || S_ISREG(statRes.st_mode)) continue;
            printf("%*s~%s\n", indent, "", file);
            transfer_file(file, statRes.st_mode, 1, socket);
        }
    }

    if (!(dir = opendir(name))) return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char *t = concat(name, entry->d_name);
            char *t_tag = concat(t, "/");
            struct stat statRes;

            if (lstat(t, &statRes) < 0) {
                printf("ERROR: F stat is cannot be accessed for directory %s.\n", t);
                continue;
            }
            if (S_ISLNK(statRes.st_mode) || !S_ISDIR(statRes.st_mode)) continue;
            tcp_directory(t, statRes.st_mode, 0, socket);

            printf("%*s%s\n", indent, "", t_tag);
            listdir(socket, t_tag, indent + 2);
        }
    }

    if (!(dir = opendir(name))) return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char *t = concat(name, entry->d_name);
            char *t_tag = concat(t, "/");
            struct stat statRes;

            if (lstat(t, &statRes) < 0) {
                printf("ERROR: F stat is cannot be accessed for directory %s.\n", t);
                continue;
            }
            if (S_ISLNK(statRes.st_mode) || !S_ISDIR(statRes.st_mode)) continue;
            tcp_directory(t, statRes.st_mode, 1, socket);
            printf("%*s~%s\n", indent, "", t_tag);
        }
    }

    closedir(dir);
}

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void relayer(int socket) {
    int n;
    char buffer[256];

    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = send(socket,buffer,strlen(buffer), 0);
    n = send(socket,"endo\n",strlen("endo\n"), 0);
    if (n < 0) error("ERROR writing to socket");
    bzero(buffer,256);

    int count = 0, total = 0;
    while ((count = recv(socket, &buffer[total], sizeof buffer - count, 0)) > 0) {
        total += count;
        printf("Message has %d bytes and the content is: %s \n", count, buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"usage %s hostname port <directory_path>\n", argv[0]);
        exit(0);
    }

    char *e = strrchr(argv[3], '/');
    int index = (int)(e - argv[3]);
    if ((strlen(argv[3]) != (index + 1)) || !e) {
        printf("Incomplete path... Usage: requires a relative or absolute path\n");
        return -1;
    }
    int sockfd = connect_tcp(argv[1], argv[2]);
    listdir(sockfd, argv[3], 0);
    end_tcp(sockfd);
    //relayer(sockfd);

    close(sockfd);
    return 0;
}
