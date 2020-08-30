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
#include "spackage_tcp.h"
#include "epackage_tcp.h"

/* 
    TCP functions for recieving and collecting package contents
*/
int start_tcp_server(int port) {
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
    portno = port; //atoi(port/*argv[1]*/);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    printf("Listening on %d\n\n", portno);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind: ");
        exit(1);
    }

    if (listen(sockfd, 5) == -1) {
        perror("listen: ");
        exit(1);
    }
    return sockfd;
}

void tcp_request_error_handler(int check, int sockfd) {
    if (check < 0) {
        printf("Transmission failure - Missing/Currupted Package content\n");
        close(sockfd);
        exit(0);
    }
}

/* need to check for transmission -- buggy */
int tcp_package(int socket, void *package, size_t length, int flag, int type) {
    tcp_content *file_ptr;
    tcp_repo *dir_ptr;
    request *req_ptr;
    if (type == 0) file_ptr = (tcp_content*)package;
    else if (type == 1) dir_ptr = (tcp_repo*)package;
    else req_ptr = (request*)package;
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
    while (length > 0 && type == 2) {
        int i = send(socket, req_ptr, length, flag);
        if (i < 1) return -1;
        dir_ptr += i, length -= i;
    }
    return 1;
}

tcp_content *get_package_content_replay(int socket, tcp_content *package_reply) {
    if (!package_reply) operation_error("Memory allocation overflow error\n");

    int nbytes = 0;
    while (((nbytes += recv(socket, package_reply, sizeof(tcp_content), 0)) > 0) && (nbytes != sizeof(tcp_content)));
    return (nbytes == sizeof(tcp_content)) ? package_reply : NULL;
}

tcp_repo *get_package_repo_replay(int socket, tcp_repo *package_reply) {
    if (!package_reply) operation_error("Memory allocation overflow error\n");

    int nbytes = 0;
    while (((nbytes += recv(socket, package_reply, sizeof(tcp_repo), 0)) > 0) && (nbytes != sizeof(tcp_repo)));
    return (nbytes == sizeof(tcp_repo)) ? package_reply : NULL;
}

request *get_package_request_replay(int socket, request *package_reply) {
    if (!package_reply) operation_error("Memory allocation overflow error\n");

    int nbytes = 0;
    while (((nbytes += recv(socket, package_reply, sizeof(request), 0)) > 0) && (nbytes != sizeof(request)));
    return (nbytes == sizeof(request)) ? package_reply : NULL;
}

int length(char *array) {
    int size = 0;
    while (array && array[size] != '\0' && array[size + 1] != '\0') size++;
    return size;
}

int compare_hash(char *hash_f1, char *hash_f2) {
    int i;
    for (i = 0; hash_f1[i] == hash_f2[i] && i < HASH_SIZE; i++);
    return i == HASH_SIZE;
}

void package_reply(int socket, const char reply) {
    /* Label representation
     * ? - unknown/interuption flag
     * E - error instruction
     * S - skip instruction
     * N - next instruction
     * R - ready instruction
     * F - file write/update instruction
     * C - create new (root or sub) repo
     * D - done intstruction
     */
    tpc_reply *package_call;
    if (!(package_call = (tpc_reply*)malloc(sizeof(tpc_reply) + 1))) {
        perror("Memory allocation overflow error: \n");
        return;
    }

    package_call->reply = reply;
    tcp_request_error_handler(tcp_package(socket, package_call, sizeof(tpc_reply), 0, 0), socket);
    free(package_call);
}

void close_buffer_stream(FILE **p) {
    if (fclose(*p)) printf("ERROR: could not close file\n");
    *p = NULL;
}

FILE *open_fd(const char *file, const char *flag) {
    FILE *fd;
    if (!(fd = fopen(file, flag))) {
        printf("ERROR: Could not open File %s for access\n", file);
        //close_buffer_stream(&fd);
    }
    return fd;
}

int update_file(const char content, FILE *inodeFd) {
    //fputs(info->content, inode);
    if (fputc(content, inodeFd) == EOF) {
        close_buffer_stream(&inodeFd);
        return 0;
    }
    return 1;
}

int update_file_permission(const char *file, mode_t permission) {
    if (chmod(file, permission) < 0) {
        printf("Could not update permissions for file '%s'\n", file);
        perror("Following error occurred: ");
        return 0;
    }
    return 1;
}

int symlink_resolve(const char *file, const char *symlink, int tries) {
    if (remove(symlink) == 0) {
        if (link_symlink(file, symlink, tries) == 0) {
            unlink(file);
            printf("Could not resolve symlink() for file %s\n", symlink);
            return 0;
        } else printf("'%s' file deleted successfully.\n", symlink);
    } else {
        printf("Unable to delete the linker file\n");
        perror("Following error occurred: ");
        return 0;
    }
    return 1;
}

int link_symlink(const char *pathname, const char *slink, int tries) {
    if (tries <= 0) return 0;
    if (symlink(pathname, slink) != 0) {
        printf("symlink() error happended...\n Attempting quick fix...");
        return symlink_resolve(pathname, slink, tries - 1);
    }
    return 1;
}

void update_inode_meta_data(struct stat info, const char *filename) {
    if (S_ISLNK(info.st_mode)) {
        printf("No need to update meta-data inode times for link: '%s'\n", filename);
        return;
    }

    struct utimbuf inode_info;
    inode_info.modtime = info.st_mtime;
    inode_info.actime = info.st_atime;

    if (utime(filename, &inode_info) == 0) printf("Successfully updated meta-data inode times for '%s'\n", filename);
    else printf("Failure upon updating meta-data inode times for '%s'\n", filename);
}

int create_repo_directory(const char *path, const char *storage_path, int socket, user_side user) {
    printf("param create_repo_directory --- path: <%s>, storage_path: <%s>\n", path, storage_path);
    /**
        ToDo: - Update corresponding created directory permissions with respected local to remote (debug memory leaks)
              - Error handling
    **/

    tcp_repo *client_tcp_repo;
    if (!(client_tcp_repo = (tcp_repo*)malloc(sizeof(tcp_repo) + 1))) {
        package_reply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return -1;
    }

    char path_tmp[strlen(path) + 1];
    strcpy(path_tmp, path);
    char *token;
    token = strtok(path_tmp, "/");

    char directory[strlen(path) + 1];
    strcpy(directory, "");

    /* parse the base-path of remote */
    strcat(strcat(directory, token), "/");
    char directory_tmp[strlen(token) + 2];
    strcat(strcpy(directory_tmp, token), "/");
    token = strtok(NULL, "/");

    while(token != NULL) {
        strcat(strcat(directory, token), "/");
        char directory_tmp[strlen(token) + 2];
        strcat(strcpy(directory_tmp, token), "/");
        printf("debug created directory ------- %s\n", directory);

        if (!get_package_repo_replay(socket, client_tcp_repo)) {
            free(client_tcp_repo);
            printf("Could not recieve message request from %s\n", (user == CLIENT) ? "client" : "server");
            return -1;
        }

        struct stat statRes;
        if (/*strcmp(storage_path, directory_tmp) &&*/ strcmp("./", directory_tmp) && strcmp("../", directory_tmp) && lstat(directory, &statRes) < 0 && mkdir(directory, client_tcp_repo->permission) == -1) {
            printf("Could not create %s directory\n", directory);
            return -1;
        }
        token = strtok(NULL, "/");
        package_reply(socket, 'R');
    }
    free(client_tcp_repo);
    return 0;
}

int directory_storage(int socket, user_side user) {
    tcp_repo *client_tcp_repo;
    if (!(client_tcp_repo = (tcp_repo*)malloc(sizeof(tcp_repo) + 1))) {
        package_reply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return -1;
    }

    if (!get_package_repo_replay(socket, client_tcp_repo)) {
        free(client_tcp_repo);
        printf("Could not recieve message request from %s\n", (user == CLIENT) ? "client" : "server");
        return -1;
    } else {
        printf("debug _recieved tcp repo data_ --- client_tcp_repo->client_repo: <%s>, client_tcp_repo->origin: <%s>, client_tcp_repo->permission <%03o>\n", client_tcp_repo->client_repo, client_tcp_repo->origin, client_tcp_repo->permission);
        int path_len = (user == SERVER) ? strlen(client_tcp_repo->client_repo) : strlen(client_tcp_repo->origin);
        char repo_storage[path_len + 1];
        if (user == SERVER) {
            strcpy(repo_storage, client_tcp_repo->client_repo);
            if (strcmp(client_tcp_repo->origin, "/") != 0) strcat(repo_storage, client_tcp_repo->origin);
            printf("Creating new server side directory '%s'\n", repo_storage);
        } else if (user == CLIENT) {
            strcpy(repo_storage, client_tcp_repo->origin);
            printf("Pulling repo directory '%s'\n", repo_storage);
        }
        printf("debug --- repo_storage: <%s>, client_tcp_repo->client_repo: <%s>, client_tcp_repo->permission) <%03o>\n", repo_storage, client_tcp_repo->client_repo, client_tcp_repo->permission);
        struct stat st;
        if (stat(repo_storage, &st) == -1) {
            package_reply(socket, 'C');
            if (create_repo_directory(repo_storage, client_tcp_repo->client_repo, socket, user) == -1) { 
                package_reply(socket, 'E');
                free(client_tcp_repo);
                perror("Could not create directory: \n");
                return -1;
            }
        } else {
            if (user == SERVER) printf("Updating server side client directory '%s'\n\n", repo_storage);
            else if (user == CLIENT) printf("Updating client side directory '%s'\n\n", repo_storage);
            package_reply(socket, 'N');
        }
        package_reply(socket, 'R');
    }
    free(client_tcp_repo);
    return 1;
}

void save_file(int socket, user_side user) {
    if (directory_storage(socket, user) != 1) {
        printf("TRANSMIT_ERROR\n\n");
        package_reply(socket, 'E');
        exit(0);
    }

    tcp_content *info;
    if (!(info = (tcp_content*)malloc(sizeof(tcp_content) + 1))) {
        package_reply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return;
    }

    int byte_mask;
    FILE *inode = NULL;
    while ((info->command != 'Q')) {
        if (!get_package_content_replay(socket, info) || info->file_type == 'E') {
            printf("Connection with client is terminated, due to some interruption.\n");
            break;
        }
        if (info->file_type == '_') {
            struct stat statRes;
            if (lstat(info->filename, &statRes) == 0) {
                FILE *fp = open_fd(info->filename, "rb");
                if (!fp) {
                    package_reply(socket, 'E');
                    continue;
                }
                char *file_hash;
                if (!(file_hash = hash(fp))) {
                    printf("Could not compute file hash for '%s'\n", info->filename);
                    package_reply(socket, 'S');
                }
                close_buffer_stream(&fp);
                if (compare_hash(info->hash, file_hash) && (info->inode_info.st_size == statRes.st_size)) {
                    printf("skip package update for file '%s'\n", info->filename);
                    package_reply(socket, 'S');
                    continue;
                } else printf("Writing/Updating package for storage '%s'\n", info->filename);
                free(file_hash);
            }
            if (!inode) inode = open_fd(info->filename, "w+b");
            if(!inode) {
                package_reply(socket, 'E');
                continue;
            }
            byte_mask = byte_sum(info->filename);
            if (update_file_permission(info->filename, info->inode_info.st_mode) == 0) break;
            package_reply(socket, 'T');
        } else if (info->file_type == 'd') {
            struct stat st;
            if (stat(info->filename, &st) == -1) {
                if (mkdir(info->filename, info->inode_info.st_mode) == -1) {
                    printf("Could not allocate directory space for '%s'\n", info->filename);
                    package_reply(socket, 'E');
                    continue;
                }
                printf("Transferring directory '%s'\n", info->filename);
            } else printf("Updating directory '%s'\n", info->filename);
            package_reply(socket, 'R');
        } else if (info->file_type == '~') {
            printf("Linking %s to %s as a symlink\n", info->filename, info->ln_filename);
            if (link_symlink(info->ln_filename, info->filename, 3) == 1) {
                printf("Linked %s to %s as a symlink\n", info->filename, info->ln_filename);
            } else printf("Could not linked %s to %s as a symlink\n", info->filename, info->ln_filename);
        } else if (info->file_type == ' ') {
            close_buffer_stream(&inode);
            update_inode_meta_data(info->inode_info, info->filename);
            byte_mask = 0;
            printf("Finished file transfer for '%s'\n", info->filename);
        } else if (info->file_type == 'f') {
            decrypt_content(&info->content, byte_mask);
            if (update_file(info->content, inode) == 0) {
                package_reply(socket, 'F');
                printf("Could not write to file '%s' ", info->filename);
                perror("fputc(...) raised an error: ");
                break;
            }
            package_reply(socket, 'R');
        }
    }
    char last_commnad = info->command;
    free(info);
    if (last_commnad == 'Q') {
    	printf("TRANSMIT_OK\n");
        package_reply(socket, 'D');
    } else {
        printf("TRANSMIT_ERROR\n\n");
        package_reply(socket, 'E');
        exit(0);
    }
}

int send_package(int socket, void *buffer, size_t length) {
    char *ptr = (char*)buffer;
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

    if (count < 0) operation_error("ERROR reading from socket: ");
    printf("Here is the message: %s\n",buffer);

    int i;
    for (i = 0; i < 5; i++) {
        int b = (i != 4) ? send_package(socket,"I got your message",18) : send_package(socket,"Final ending...",15);
        if (!b) operation_error("ERROR writing to socket: ");
        printf("something happened\n");
    }
}
