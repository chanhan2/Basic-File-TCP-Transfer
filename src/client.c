/*
    Simple TCP Client
*/

/* external library */
#include "client.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,"usage %s <hostname> <target_directory_path> <dest_directory_path> <download|upload>\n", argv[0]);
        exit(0);
    }

    if (strcmp(argv[4], "upload") != 0 && strcmp(argv[4], "download") != 0) {
        fprintf(stderr,"Invalid 'request' arguement or the 'request' specified is not supported.\n\nCould not follow and execute the request: '%s' .\n\nCurrent requests are 'download' and 'upload'\n\n download or upload?\n", argv[4]);
        exit(0);
    }

    struct stat statRes;
    if (lstat(argv[2], &statRes) < 0 && strcmp(argv[4], "upload") == 0) {
        printf("ERROR: stat cannot access inode %s\n", argv[2]);
        exit(0);
    }

    if (strlen(argv[3]) > 2 && argv[3][1] == '.' && argv[3][2] == '/') {
        fprintf(stderr,"Invalid <target_directory_path>\n\n<target_directory_path> cannot have indirect parent directory access\n");
        exit(0);
    }

    if (strcmp(argv[2], "/") == 0 && strcmp(argv[4], "upload") == 0) {
        printf("%s%s", __UNSUPPORT_MESSAGE__, __UNSUPPORT_MESSAGE_REASON__);
        //exit(0); // <- initially kept as this was not part of design
    }

    char package_path[PATH_MAX + 1];
    strcpy(package_path, argv[3]);
    if (package_path[strlen(package_path) - 1] != '/') strcat(package_path, "/");

    char ref_path[PATH_MAX + 1];
    char origin[PATH_MAX + 1];
    if (S_ISDIR(statRes.st_mode)) {
        if (strlen(argv[2]) > 1 && argv[2][0] != '.' && argv[2][0] == '/') {
            strcpy(ref_path, argv[2]);
        } else if (strlen(argv[2]) > 1 && argv[2][0] != '.' && argv[2][1] != '/') {
            strcat(strcpy(ref_path, "./"), argv[2]);
        } else strcpy(ref_path, argv[2]);
        if (ref_path[strlen(ref_path) - 1] != '/') strcat(ref_path, "/");

        char *e = strrchr(ref_path, '/');
        char *e2 = strrchr(package_path, '/');
        if (!e || !e2 || (strlen(ref_path) != ((int)(e - ref_path) + 1)) || (strlen(package_path) != ((int)(e2 - package_path) + 1))) {
            printf("Incomplete path or invalid path... Usage: requires a relative or absolute path\n");
            return -1;
        }

        // ToDo: retains respective permissions and parent directory structure
        // option 1: --- save file with respect to relative path in remove directory
        /*strcpy(origin, strcpy(ref_path, argv[2]));
        strcpy(ref_path, origin);*/

        // option 2 (default): --- save directory in remote specified directory location
        realpath(ref_path, origin);
        strcat(strcpy(origin, &origin[(int)(strrchr(origin, '/') - origin) + 1]), "/");
    } else {
        strcpy(origin, strcpy(ref_path, argv[2]));
        char working_path[strlen(origin) + 3];
        char relative_path[strlen(origin) + 3];
        char *e3 = strchr(origin, '/');
        strcpy(origin, strcat(strcpy(working_path, "./"), strcpy(relative_path, &origin[(e3 != NULL) ? (int)(e3 - origin) + 1 : 0])));

        // ToDo: retains respective permissions and parent directory structure
        // option 1 (default): --- save file with respect to relative path in remove directory
        origin[(int)((strrchr(origin, '/')) - origin) + 1] = '\0';
        strcpy(ref_path, origin);

        // option 2: --- save file in remote specified directory location
        /*strcpy(origin, "./");
        if (!strrchr(relative_path, '/')) strcpy(ref_path, "./");
        else {
            strcpy(ref_path, working_path);
            ref_path[(int)((e3 = strrchr(ref_path, '/')) - ref_path) + 1] = '\0';
        }*/
    }
    printf("debug --- origin: <%s>, ref_path <%s>\n", origin, ref_path);

    int sockfd = connect_tcp(argv[1], PORT);

    char reply;
    request *client_req = (request*)malloc(sizeof(request) + 1);
    if (!client_req) error(sockfd, 1, "Memory allocation overflow error\n");

    if (strcmp(argv[4], "download") == 0) client_req->req = DOWNLOAD;
    else if (strcmp(argv[4], "upload") == 0) client_req->req = UPLOAD;

    tcp_request_error_handler(tcp_package(sockfd, client_req, sizeof(request), 0, 1), sockfd);
    free(client_req);
    reply = get_reply(sockfd);

    if (strcmp(argv[4], "download") == 0) {
        if (reply == 'F' || reply == '?') service_error("Download Failure.....\n\nTRANSMIT_FAILURE\n", sockfd, CLIENT);
        // ToDo:
        // Handling on download specified file

        char client_repo_dir[strlen(package_path) + strlen(ref_path) + 1];
        char chr = package_path[strlen(package_path) - 1];
        int chr_location = strlen(package_path) - 1;
        char *e_loc;
        strcpy(client_repo_dir, ref_path);
        if (!(e_loc = strrchr(package_path, '/'))) {
            package_path[chr_location] = '\0';
            strcat(client_repo_dir, &package_path[(int)(strrchr(package_path, '/') - package_path) + 1]);
            package_path[chr_location] = chr;
            strcat(client_repo_dir, "/");
        }

        tcp_repo *info_dir = (tcp_repo*)malloc(sizeof(tcp_repo) + 1);
        if (!info_dir) error(sockfd, 1, "Memory allocation overflow error\n");
        strcpy(info_dir->origin, client_repo_dir);
        strcpy(info_dir->client_repo, package_path);

        info_dir->idx = strlen(package_path);
        //info_dir->offset_idx_len = atoi(argv[5]);
        tcp_request_error_handler(tcp_package(sockfd, info_dir, sizeof(tcp_repo), 0, 1), sockfd);
        free(info_dir);

        if ((reply = get_reply(sockfd)) == 'M') service_error("Remote repo does not exist\n", sockfd, CLIENT);
        else if (reply == '?') service_error("Download Failure.....\n\nTRANSMIT_FAILURE\n", sockfd, CLIENT);

        save_file(sockfd, (user_side)CLIENT);
    } else if (strcmp(argv[4], "upload") == 0) {
        if (reply == 'E' || reply == '?') operation_error("Server could not allocate space for directory\n");

        tcp_repo *info_dir = (tcp_repo*)malloc(sizeof(tcp_repo) + 1);
        if (!info_dir) error(sockfd, 1, "Memory allocation overflow error\n");
        strcpy(info_dir->client_repo, package_path);
        strcpy(info_dir->origin, (S_ISDIR(statRes.st_mode)) ? origin : &origin[2]);
        info_dir->permission = 0000;
        //printf("debug _send tcp repo data_ info_dir --- info_dir->client_repo: <%s>, info_dir->origin: <%s>\n", info_dir->client_repo, info_dir->origin);
        tcp_request_error_handler(tcp_package(sockfd, info_dir, sizeof(tcp_repo), 0, 1), sockfd);

        reply = get_reply(sockfd);
        if (reply != 'N' && !S_ISDIR(statRes.st_mode) && walk_path_permission((S_ISDIR(statRes.st_mode)) ? origin : &origin[2], 1, sockfd) < 0) operation_error("Could not access local directory meta-data\n");
        else if (reply != 'N' && S_ISDIR(statRes.st_mode)) {
            if (strcmp(argv[2], "./") != 0 && strcmp(argv[2], "../") != 0 && walk_path_permission((S_ISDIR(statRes.st_mode)) ? origin : &origin[0], 0, sockfd) < 0) operation_error("Could not access local directory meta-data\n");
            else {
                if (walk_path_permission(ref_path, 0, sockfd) < 0) operation_error("Could not access local directory meta-data\n");
            }
        }
        free(info_dir);

        reply = get_reply(sockfd);
        if (reply == 'E' || reply == '?') operation_error("Server could not allocate space for directory: \n");
        //printf("debug --- ref_path <%s>, package_path <%s>\n", ref_path, package_path);
        //printf("debug transfer_file param input --- argv[2]: <%s>, strlen(ref_path): <%lu> \n", argv[2], (argv[2][0] == '.' && argv[2][1] == '/') ? strlen(ref_path) : ((argv[2][0] == '.' && argv[2][1] == '.') ? strlen(ref_path) + 1 : 0));
        if (S_ISDIR(statRes.st_mode)) listdir(sockfd, strlen(ref_path), ref_path, (strcmp(origin, "/") == 0) ? package_path : strcat(package_path, origin));
        else transfer_file(argv[2], origin, strcat(package_path, &origin[2]), statRes, sockfd, (argv[2][0] == '.' && argv[2][1] == '/') ? strlen(ref_path) : ((argv[2][0] == '.' && argv[2][1] == '.') ? strlen(ref_path) + 1 : 0));

        end_tcp(sockfd, CLIENT);
        //relayer(sockfd);
    }
    close(sockfd);
    return 0;
}
