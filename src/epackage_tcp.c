/* C library */
#include <time.h>
#include <utime.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
/* #include <stdbool.h> // performance of data structure is unknown/untested */
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* external library */
#include "hash.h"
#include "mask.h"
#include "fnstruct.h"
#include "epackage_tcp.h"
#include "spackage_tcp.h"

/* 
    TCP functions to emitting and reading/creating package contents
*/
void operation_error(const char *msg) {
    perror(msg);
    printf("TRANSMIT_ERROR\n");
    exit(1);
}

void error(int socket, int status, const char *msg) {
    tcp_content *package_call;
    if (!(package_call = (tcp_content*)malloc(sizeof(tcp_content) + 1))) {
        perror("Memory allocation overflow error\n");
        exit(status);
    }
    package_call->file_type = 'E';

    tcp_request_error_handler(tcp_package(socket, package_call, sizeof(tcp_content), 0, 0), socket);

    free(package_call);
    perror(msg);
    exit(status);
}

void service_error(const char *msg, int socket, user_side user) {
    printf("%s\n", msg);
    //end_tcp(socket, user);
    close(socket);
    exit(0);
}

void service_failure(const char *msg) {
    printf("%s\n", msg);
    exit(0);
}

int connect_tcp(char *host, int port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno = port/*atoi(port)*/, sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) operation_error("Client opening socket error: ");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) < 1) operation_error("Client inet_pton error: ");

    if ((server = gethostbyname(host)) == NULL) operation_error("Client no such host error: ");

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) operation_error("Client Connection error: ");

    return sockfd;
}

char get_reply(int socket) {
    tpc_reply *package_call;
    if (!(package_call = (tpc_reply*)malloc(sizeof(tpc_reply) + 1))) error(socket, 1, "Memory allocation overflow error\n");

    int nbytes = 0;
    while (((nbytes += recv(socket, package_call, sizeof(tpc_reply), 0)) > 0) && (nbytes != sizeof(tpc_reply)));
    char reply = (nbytes == sizeof(tpc_reply)) ? package_call->reply : '?';
    free(package_call);
    return reply;
}

void end_tcp(int sockfd, user_side user) {
    tcp_content *info;
    if (!(info = (tcp_content*)malloc(sizeof(tcp_content) + 1))) error(sockfd, 1, "Memory allocation overflow error\n");

    strcpy(info->filename, "");
    //strcpy(info.content, "\n");
    info->content = '\n';
    //info->permission = 0000;
    info->file_type = 'q';
    info->command = 'Q';
    strcpy(info->ln_filename, "\0");

    tcp_request_error_handler(tcp_package(sockfd, info, sizeof(tcp_content), 0, 0), sockfd);
    free(info);

    char reply = get_reply(sockfd);
    if (reply == 'D') printf("TRANSMIT_OK\n");
    else if (reply == 'E') printf("TRANSMIT_ERROR:\nFailure to upload files\n");
    else {
        if (user == CLIENT) printf("Lost connection to server...\n");
        if (user == SERVER) printf("Lost connection to client...\n");
    }
}

void printUseless(int indent) {
    printf("%*s%s\n", indent, "", ".");
    printf("%*s%s\n", indent, "", "..");
}

void mod_path(const char *dest, const char *file, char *path, int shift) {
    strcat(strcpy(path, dest), &file[shift]);
}

void copy_hash(char *array, const char *hash) {
    int i;
    for (i = 0; i < HASH_SIZE; array[i] = hash[i], i++);
}

int file_signature(const char *file, char *dest) {
    FILE *fh;
    if (!(fh = open_fd(file, "rb"))) return 0;

    char *file_hash;
    if (!(file_hash = hash(fh))) return 0;
    copy_hash(dest, file_hash);

    close_buffer_stream(&fh);
    free(file_hash);
    return 1;
}

void concat(char *str, const char *s1, const char *s2) {
    strcat(strcpy(str, s1), s2);
}

int walk_path_permission(char *path, int isFile, int socket) {
    printf("------- debug walked path directory permission ------- params ---- path: <%s>, isFile: <%d>\n\n", path, isFile);
    tcp_repo *info_dir = (tcp_repo*)malloc(sizeof(tcp_repo) + 1);
    if (!info_dir) error(socket, 1, "Memory allocation overflow error\n");
    strcpy(info_dir->client_repo, "");
    strcpy(info_dir->origin, "");

    char path_tmp[strlen(path) + 1];
    strcpy(path_tmp, path);
    char *token;
    token = strtok(path_tmp, "/");

    char directory[strlen(path) + 1];
    strcpy(directory, "");
    
    strcat(strcat(directory, token), "/");
    char directory_tmp[strlen(token) + 2];
    if (strcmp(path, "./") != 0 && strcmp(path, "../") != 0 && !isFile) {
        strcat(strcpy(directory_tmp, token), "/");
        token = strtok(NULL, "/");
    }

    printf("start path <%s>\n", path);
    printf("hmmm <%s>\n", directory);
    while(token != NULL) {
        struct stat statRes;
        if (strcmp("./", directory_tmp) && strcmp("../", directory_tmp) && lstat(directory, &statRes) < 0) {
            printf("Could not read directory permission in walked path: %s\n", directory);
            free(info_dir);
            return -1;
        }
        info_dir->permission = statRes.st_mode;
        printf("debug walk_path_permission --- current walked path: <%s>, permission <%03o>\n", directory, statRes.st_mode);
        tcp_request_error_handler(tcp_package(socket, info_dir, sizeof(tcp_repo), 0, 1), socket);

        char reply = get_reply(socket);
        if (reply == 'E' || reply == '?') operation_error("Server could not allocate space for directory\n");

        token = strtok(NULL, "/");
    }
    free(info_dir);
    return 1;
}

void transfer_file(const char *file, const char *src, const char *dest, struct stat statRes, int socket, int shift) {
    //printf("debug transfer_file param --- file: <%s>, src: <%s>, dest: <%s>, socket: <%d>, shift: <%d>\n", file, src, dest, socket, shift);
    /**
        ToDo: Check server status reply before continuing with I/O, followed by package construction and emitting package
    **/

    tcp_content *info;
    if (!(info = (tcp_content*)malloc(sizeof(tcp_content) + 1))) error(socket, 1, "Memory allocation overflow error\n");
    char path_trim[PATH_MAX + 1];
    mod_path(dest, file, path_trim, shift);
    strcpy(info->filename, path_trim);
    //strcpy(info.content, "\n");
    info->content = '\n';
    info->inode_info = statRes;
    //printf("debug prepared file --- file: <%s>, &file[shift]: <%s>, dest: <%s>, info->filename: <%s>\n", file, &file[shift], dest, info->filename);

    int byte_mask = 0;
    if (S_ISLNK(statRes.st_mode)) {
        char buf[PATH_MAX + 1];
        //char base[PATH_MAX + 1];
        char basetest[PATH_MAX + 1];
        realpath(file, buf);
        //char *baseline = realpath("./", base);
        char *baseline2 = realpath(src, basetest);
        char soft_path_trim[PATH_MAX + 1];
        mod_path(dest, &buf[strlen(baseline2)], soft_path_trim, 1);

        info->file_type = '~';
        strcpy(info->ln_filename, soft_path_trim);
        /*
        printf("Base path is -> %s\n", &buf[strlen(baseline)]);
        printf("symlink path is -> %s\n", file);
        printf("Baseline path for file -> %s\n", buf);
        printf("Baseline path for src -> %s\n", baseline2);
        printf("Relative path -> %s\n", &buf[strlen(baseline2)]);
        */

        tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);

        free(info);
        return;
    } else if (S_ISREG(statRes.st_mode)) {
        info->file_type = '_';
        strcpy(info->ln_filename, "");
        if (file_signature(file, info->hash) == 0) {
            printf("Error occured in computing hash for %s file\n", file);
            free(info);
            return;
        }
        tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
        char reply = get_reply(socket);
        if (reply == 'S' || reply == 'E') {
            free(info);
            if (reply == 'E') printf("Error occured in remote for file %s\n", file);
            return;
        } else if (reply == '?') {
            free(info);
            service_failure("Upload Failure.....\n\nTRANSMIT_FAILURE\n");
        }
        byte_mask = byte_sum(info->filename);
    } else {
        free(info);
        printf("An unexpected inode type was seen under the attempt of transferring file, inode is '%s'\n", file);
        return;
    }

    info->file_type = 'f';
    FILE *fs;
    if((fs = open_fd(file, "rb")) == NULL) {
        info->file_type = ' ';
        tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
        error(socket, 1, "ERROR File: Could not access File");
    }

    int ch;
    //while (fgets (info.content, 256, fs) != NULL) {
    while ((ch = fgetc(fs)) != EOF) {
        info->content = ch;
        encrypt_content(&info->content, byte_mask);
        tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
        char reply = get_reply(socket);
        if (reply == 'F' || reply == '?') {
            close_buffer_stream(&fs);
            free(info);
            service_error("Upload Failure.....\n\nTRANSMIT_FAILURE\n", socket, CLIENT);
        }
    }

    info->file_type = ' ';
    tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);

    close_buffer_stream(&fs);
    free(info);
}

void tcp_directory(const char *file, const char *src, const char *dest, struct stat statRes, int socket, int shift) {
    /**
        ToDo: Check server status reply before continuing with I/O, followed by package construction and emitting package
    **/

    tcp_content *info;
    if (!(info = (tcp_content*)malloc(sizeof(tcp_content) + 1))) error(socket, 1, "Memory allocation overflow error\n");
    char path_trim[strlen(dest) + strlen((&file[shift])) + 1];
    mod_path(dest, file, path_trim, shift);
    strcpy(info->filename, path_trim);
    //strcpy(info.content, "\n");
    info->content = '\n';
    info->inode_info = statRes;

    if (S_ISLNK(statRes.st_mode)) {
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
    } else if (S_ISDIR(statRes.st_mode)) {
        info->file_type = 'd';
        strcpy(info->ln_filename, "");
    } else {
        free(info);
        printf("An unexpected inode type was seen under the attempt of transferring directory, inode is '%s'\n", file);
        return;
    }

    tcp_request_error_handler(tcp_package(socket, info, sizeof(tcp_content), 0, 0), socket);
    free(info);

    char replay = get_reply(socket);
    if (replay == 'E') service_error("Cannot transfer directory to current directory due to service error or terminated service\n", socket, CLIENT);
    else if (replay == '?') service_error("Cannot transfer directory to current directory due to terminated server communication\n", socket, CLIENT);
}

void listdir(int socket, int shift, const char *name, const char *dest) {
    //printf("debug listdir param --- socket: <%d> shift: <%d> name: <%s> dest: <%s>\n", socket, shift, name, dest);
    static int indent = 0;
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name))) return;
    if (name[0] == '.' && name[1] == '/') printf("%*s%s\n", indent, "", (strcmp(name, "./") == 0) ? name : &name[2]);
    else if (name[0] == '.' && name[1] == '.') printf("%*s%s\n", indent, "", (strcmp(name, "../") == 0) ? name : &name[3]);
    else printf("%*s%s\n", indent, "", name);
    indent += 2, printUseless(indent);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            char file[strlen(name) + strlen(entry->d_name) + 1];
            concat(file, name, entry->d_name);
            struct stat statRes;
            if (lstat(file, &statRes) < 0) {
                printf("ERROR: stat is cannot be accessed for file %s.\n", file);
                continue;
            }

            //printf("debug --- file: <%s>, name: <%s>, entry->d_name: <%s>\n", file, name, entry->d_name);
            char *loc = strchr(file, '/');
            if (!(S_ISLNK(statRes.st_mode) || !S_ISREG(statRes.st_mode))) {
                transfer_file(file, name, dest, statRes, socket, shift);
                if ((int)(loc - file) == 1 /*file[1] == '/'*/) printf("%*s%s\n", indent, "", &file[2]);
                else if ((int)(loc - file) == 2 /*file[1] == '.'*/) printf("%*s%s\n", indent, "", &file[3]);
                else printf("%*s%s\n", indent, "", file);
            } else if (!(!S_ISLNK(statRes.st_mode) || S_ISREG(statRes.st_mode))) {
                transfer_file(file, name, dest, statRes, socket, shift);
                if ((int)(loc - file) == 1 /*file[1] == '/'*/) printf("%*s%s~%s\n", indent, "", &name[2], entry->d_name /*&file[2]*/);
                else if ((int)(loc - file) == 2 /*file[1] == '.'*/) printf("%*s%s~%s\n", indent, "", &name[3], entry->d_name /*&file[2]*/);
                else printf("%*s%s~%s\n", indent, "", name, entry->d_name /*&file[2]*/);
            }
        }
    }
    closedir(dir);

    if (!(dir = opendir(name))) return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            char t[strlen(name) + strlen(entry->d_name) + 1];
            concat(t, name, entry->d_name);
            char t_tag[strlen(t) + strlen("/") + 1];
            concat(t_tag, t, "/");

            struct stat statRes;
            if (lstat(t, &statRes) < 0) {
                printf("ERROR: stat is cannot be accessed for directory %s.\n", t);
                continue;
            }

            if (!(S_ISLNK(statRes.st_mode) || !S_ISDIR(statRes.st_mode))) {
                tcp_directory(t, name, dest, statRes, socket, shift);
                listdir(socket, shift, t_tag, dest);
            } else if (!(!S_ISLNK(statRes.st_mode) || S_ISDIR(statRes.st_mode))) {
                tcp_directory(t, name, dest, statRes, socket, shift);
                listdir(socket, shift, t_tag, dest);
            }
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
