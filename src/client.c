/*
    Simple TCP Client
*/

/* C library */
#include <limits.h>
#include <fcntl.h>
#include <stdbool.h>
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
#include <arpa/inet.h>

/* external library */
#include "client.h"

int tcp_package(int socket, void *package, size_t length, int flag, int type) {
    tcp_content *file_ptr;
    repo_tcp *dir_ptr;
    if (type == 0) file_ptr = (tcp_content*) package;
    else dir_ptr = (repo_tcp*) package;
    while (length > 0 && type == 0) {
        int i = send(socket, file_ptr, length, flag);
        if (i < 1) return -1;
        file_ptr += i, length -= i;
    }
    while (length > 0 && type == 1) {
        int i = send(socket, dir_ptr, length, flag);
        if (i < 1) return -1;
        dir_ptr += i, length -= i;
    }
    return 1;
}

void error(int socket, int status, const char *msg) {
    tcp_content *package_call = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!package_call) error(socket, 1, "Memory allocation overflow error\n");
    package_call->file_type = 'E';
    transmission_error(tcp_package(socket, package_call, sizeof(tcp_content), 0, 0), socket);
    free(package_call);
    perror(msg);
    exit(status);
}

void transmission_error(int check, int sockfd) {
    if (check < 0) {
        printf("Transmission failure - Missing/Currupted Package content\n");
        close(sockfd);
        exit(0);
    }
}

void connection_error(const char *msg) {
    perror(msg);
    exit(0);
}

int connect_tcp(char *host, char *port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno = atoi(port), sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) connection_error("Client opening socket error: ");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) < 1) connection_error("Client inet_pton error: ");

    if ((server = gethostbyname(host)) == NULL) connection_error("Client no such host error: ");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) connection_error("Client Connection error: ");

    return sockfd;
}

char get_server_replay(int socket){
    int nbytes = 0;
    tcp_content *package_call = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!package_call) error(socket, 1, "Memory allocation overflow error\n");
    while (((nbytes += recv(socket, package_call, sizeof(tcp_content), 0)) > 0) && (nbytes != sizeof(tcp_content)));
    return (nbytes == sizeof(tcp_content)) ? package_call->command : '?';
}

void end_tcp(int sockfd) {
    tcp_content *info = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!info) error(sockfd, 1, "Memory allocation overflow error\n");
    strcpy(info->filename, "");
    //strcpy(info.content, "\n");
    info->content = '\n';
    info->permission = 0000;
    info->file_type = 'q';
    info->command = 'Q';
    strcpy(info->ln_filename, "\0");
    transmission_error(tcp_package(sockfd, info, sizeof(tcp_content), 0, 0), sockfd);

    char reply = get_server_replay(sockfd);
    if (reply == 'C') printf("TRANSMIT_OK\n");
    else if (reply == 'E') printf("TRANSMIT_ERROR:\nFailure to successfully upload files");
    else printf("Lost connection to server...\n");
}

void printUseless(int indent) {
    printf("%*s%s\n", indent, "", ".");
    printf("%*s%s\n", indent, "", "..");
}

void mod_path(const char *origin, char *dest, char *file, char *path, int shift) {
    strcpy(path, dest);
    strcat(path, origin);
    strcat(path, &file[shift]);
}

void file_signature(char *file, char *dest) {
    FILE *fh = fopen(file, "rb");
    char *file_hash = hash(fh);
    copyHash(dest, file_hash);
    closeBufferStream(&fh);
    free(file_hash);
}

void copyHash(char *array, char *hash) {
    int i;
    for (i = 0; i < HASH_SIZE; i++)
        array[i] = hash[i];
}

char *concat(const char *s1, const char *s2) {
    char *result = (char*)malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void closeBufferStream(FILE **p) {
    fclose(*p), *p = NULL;
}

void transfer_file(char *file, const char *origin, const char *src, char *dest, mode_t permission, int socket, int shift) {
    struct stat statRes;
    if (lstat(file, &statRes) < 0) {
        printf("ERROR: File stat is cannot be accessed for file %s.\n", file);
        return;
    }

    int isLink;
    if (S_ISLNK(statRes.st_mode)) isLink = 1;
    if (S_ISREG(statRes.st_mode)) isLink = 0;

    tcp_content *info = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!info) error(socket, 1, "Memory allocation overflow error\n");
    char path_trim[PATH_MAX + 1];
    mod_path(origin, dest, file, path_trim, shift);
    strcpy(info->filename, path_trim);
    //strcpy(info.content, "\n");
    info->content = '\n';
    info->permission = permission;
    info->size = statRes.st_size;

    if (isLink == 1) {
        char buf[PATH_MAX + 1];
        //char base[PATH_MAX + 1];
        char basetest[PATH_MAX + 1];
        realpath(file, buf);
        //char *baseline = realpath("./", base);
        char *baseline2 = realpath(src, basetest);
        char soft_path_trim[PATH_MAX + 1];
        mod_path(origin, dest, &buf[strlen(baseline2)], soft_path_trim, 1);

        info->file_type = '~';
        strcpy(info->ln_filename, soft_path_trim);
        /*
        printf("Base path is -> %s\n", &buf[strlen(baseline)]);
        printf("symlink path is -> %s\n", file);
        printf("Baseline path for file -> %s\n", buf);
        printf("Baseline path for src -> %s\n", baseline2);
        printf("Relative path -> %s\n", &buf[strlen(baseline2)]);
        */
        transmission_error(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
    } else {
        info->file_type = '_';
        strcpy(info->ln_filename, "");
        file_signature(file, info->hash);
        transmission_error(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
        if (get_server_replay(socket) == 'S') return;
    }

    info->file_type = 'f';
    FILE *fs = fopen(file, "rb");
    if(fs == NULL) {
        printf("ERROR: File %s not found.\n", file);
        error(socket, 1, "ERROR File: ");
    }

    int ch;
    //while (fgets (info.content, 256, fs) != NULL) {
    while ((ch = fgetc(fs)) != EOF) {
        info->content = ch;
        encryptContent(&info->content, &info->content_size);
        transmission_error(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
    }
    info->file_type = ' ';
    transmission_error(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
    closeBufferStream(&fs);
    free(info);
}

void tcp_directory(char *file, const char *origin, const char *src, char *dest, mode_t permission, int isLink, int socket, int shift) {
    tcp_content *info = (tcp_content*)malloc(sizeof(tcp_content) + 1);
    if (!info) error(socket, 1, "Memory allocation overflow error\n");
    char path_trim[256];
    mod_path(origin, dest, file, path_trim, shift);
    strcpy(info->filename, path_trim);
    //strcpy(info.content, "\n");
    info->content = '\n';
    info->permission = permission;

    if (isLink == 1) {
        info->file_type = '~';
        char buf[PATH_MAX + 1];
        char base[PATH_MAX + 1];
        char basetest[PATH_MAX + 1];
        realpath(file, buf);
        char *baseline = realpath("./", base);
        char *baseline2 = realpath(src, basetest);
        strcpy(info->ln_filename, &buf[strlen(baseline2)]);
        printf("Base path is -> %s\n", &buf[strlen(baseline)]);
        printf("symlink path is -> %s\n", file);
        printf("Baseline path for file -> %s\n", buf);
        printf("Baseline path for src -> %s\n", baseline2);
        printf("Relative path -> %s\n", &buf[strlen(baseline2)]);
    } else {
        info->file_type = 'd';
        strcpy(info->ln_filename, "\0");
    }
    transmission_error(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
    free(info);
}

void listdir(int socket, int shift, const char *origin, const char *name, char *dest) {
    static int indent = 0;
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
                free(file);
                continue;
            }

            if (!(S_ISLNK(statRes.st_mode) || !S_ISREG(statRes.st_mode))) {
                transfer_file(file, origin, name, dest, statRes.st_mode, socket, shift);
                printf("%*s%s\n", indent, "", &file[2]);
            } else if (!(!S_ISLNK(statRes.st_mode) || S_ISREG(statRes.st_mode))) {
                printf("%*s~%s\n", indent, "", &file[2]);
                transfer_file(file, origin, name, dest, statRes.st_mode, socket, shift);
            }
            free(file);
        }
    }
    closedir(dir);

    indent += 2;
    if (!(dir = opendir(name))) return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            char *t = concat(name, entry->d_name);
            char *t_tag = concat(t, "/");
            struct stat statRes;
            if (lstat(t, &statRes) < 0) {
                printf("ERROR: F stat is cannot be accessed for directory %s.\n", t);
                free(t), free(t_tag);
                continue;
            }

            if (!(S_ISLNK(statRes.st_mode) || !S_ISDIR(statRes.st_mode))) {
                tcp_directory(t, origin, name, dest, statRes.st_mode, 0, socket, shift);
                printf("%*s%s\n", indent, "", &t_tag[2]);
                listdir(socket, shift, origin, t_tag, dest);
            } else if (!(!S_ISLNK(statRes.st_mode) || S_ISDIR(statRes.st_mode))) {
                tcp_directory(t, origin, name, dest, statRes.st_mode, 1, socket, shift);
                printf("%*s~%s\n", indent, "", &t_tag[2]);
            }
            free(t), free(t_tag);
        }
    }
    closedir(dir); indent -= 2;
}

void relayer(int socket) {
    char buffer[256];

    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    if (send(socket,buffer,strlen(buffer), 0) < 0) error(socket, 1, "ERROR writing to socket: ");
    if (send(socket,"endo\n",strlen("endo\n"), 0) < 0) error(socket, 1, "ERROR writing to socket: ");
    bzero(buffer,256);

    int count = 0, total = 0;
    while ((count = recv(socket, &buffer[total], sizeof buffer - count, 0)) > 0) {
        total += count;
        printf("Message has %d bytes and the content is: %s \n", count, buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,"usage %s hostname port <target_directory_path> <dest_directory_path>\n", argv[0]);
        exit(0);
    }

    char ref_path[PATH_MAX + 1];
    strcpy(ref_path, argv[3]);
    if (ref_path[strlen(ref_path) - 1] != '/') strcat(ref_path, "/");

    char package_path[PATH_MAX + 1];
    strcpy(package_path, argv[4]);
    if (package_path[strlen(package_path) - 1] != '/') strcat(package_path, "/");

    char *e = strrchr(ref_path, '/');
    int index = (int)(e - ref_path);
    char *e2 = strrchr(package_path, '/');
    int index2 = (int)(e2 - package_path);
    if ((strlen(ref_path) != (index + 1)) || (strlen(package_path) != (index2 + 1)) || !e || !e2) {
        printf("Incomplete path or invalid path... Usage: requires a relative or absolute path\n");
        return -1;
    }

    char origin[PATH_MAX + 1];
    realpath(ref_path, origin);
    char *e3 = strrchr(origin, '/');
    int index3 = (int)(e3 - origin);
    strcpy(origin, &origin[index3 + 1]);
    strcat(origin, "/");

    int sockfd = connect_tcp(argv[1], argv[2]);
    repo_tcp *info_dir = (repo_tcp*)malloc(sizeof(repo_tcp) + 1);
    if (!info_dir) error(sockfd, 1, "Memory allocation overflow error\n");
    strcpy(info_dir->client_repo, package_path);
    strcpy(info_dir->origin, origin);
    info_dir->permission = 0777;
    transmission_error(tcp_package(sockfd, info_dir, sizeof(repo_tcp), 0, 1), sockfd);
    free(info_dir);

    char replay = get_server_replay(sockfd);
    if (replay == 'E' || replay == '?') connection_error("Server could not allocate space for directory: \n");

    listdir(sockfd, index + 1, origin, ref_path, package_path);
    end_tcp(sockfd);
    //relayer(sockfd);

    close(sockfd);
    return 0;
}
