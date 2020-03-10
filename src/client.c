/*
    Simple TCP Client
*/

/* external library */
#include "client.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,"usage %s hostname <target_directory_path> <dest_directory_path>\n", argv[0]);
        exit(0);
    }

    struct stat statRes;
    if (lstat(argv[2], &statRes) < 0) {
        printf("ERROR: stat cannot access inode %s.\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[2], "/") == 0 && strcmp(argv[4], "upload") == 0) {
        printf("\n\nThis DOES NOT and WILL NOT EVER support your whole god damn storage starting at root...\n");
        printf("\n\nIf you actually require this you're clearly doing something wrong here...\n\n");
        return 0;
    }

    int sockfd = connect_tcp(argv[1], PORT);
    if (strcmp(argv[4], "download") == 0) {
        if (get_replay(sockfd) == 'F') {
            service_error("Download Failure.....\n\nTRANSMIT_FAILURE\n", sockfd);
            return 0;
        }
    } else if (strcmp(argv[4], "upload") == 0) {
        char ref_path[PATH_MAX + 1];
        if (argv[2][0] != '.' && argv[2][0] == '/') {
            strcpy(ref_path, argv[2]);
        } else if (argv[2][0] != '.' && argv[2][1] != '/') {
            strcpy(ref_path, "./");
            strcat(ref_path, argv[2]);
        } else strcpy(ref_path, argv[2]);
        if (ref_path[strlen(ref_path) - 1] != '/') strcat(ref_path, "/");

        char package_path[PATH_MAX + 1];
        strcpy(package_path, argv[3]);
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

        char replay;
        request *client_req = (request*)malloc(sizeof(request) + 1);
        if (!client_req) error(sockfd, 1, "Memory allocation overflow error\n");
        client_req->req = UPLOAD;
        transmission_error(tcp_package(sockfd, client_req, sizeof(request), 0, 1), sockfd);
        free(client_req);
        replay = get_replay(sockfd);
        if (replay == 'E' || replay == '?') operation_error("Server could not allocate space for directory: \n");

        tcp_repo *info_dir = (tcp_repo*)malloc(sizeof(tcp_repo) + 1);
        if (!info_dir) error(sockfd, 1, "Memory allocation overflow error\n");
        strcpy(info_dir->client_repo, package_path);
        strcpy(info_dir->origin, origin);
        info_dir->permission = 0777;
        transmission_error(tcp_package(sockfd, info_dir, sizeof(tcp_repo), 0, 1), sockfd);
        free(info_dir);

        replay = get_replay(sockfd);
        if (replay == 'E' || replay == '?') operation_error("Server could not allocate space for directory: \n");

        listdir(sockfd, index + 1, origin, ref_path, package_path);
    }
    end_tcp(sockfd);
    //relayer(sockfd);
    close(sockfd);
    return 0;
    
}
