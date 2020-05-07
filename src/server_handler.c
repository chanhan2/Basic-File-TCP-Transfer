/* external library */
#include "server.h"

void client_request(int socket) {
    request *client_tcp_req = (request*)malloc(sizeof(request) + 1);
    if (!client_tcp_req) {
        package_reply(socket, 'E');
        perror("Memory allocation overflow error: \n");
        return;
    }

    request *req;
    if (!(req = get_package_request_replay(socket, client_tcp_req))) {
        free(client_tcp_req);
        operation_error("Could not recieve message request from client: \n");
    }

    operation_request operation = req->req;
    free(client_tcp_req);
    if (operation == UPLOAD) {
        package_reply(socket, 'R');
        save_file(socket, (user_side)SERVER);
    } else if (operation == DOWNLOAD) {
        package_reply(socket, 'R');

        tcp_repo *client_tcp_req_repo = (tcp_repo*)malloc(sizeof(tcp_repo) + 1);
        if (!client_tcp_req_repo) {
            package_reply(socket, 'E');
            perror("Memory allocation overflow error: \n");
            return;
        }

        if (!get_package_repo_replay(socket, client_tcp_req_repo)) {
            free(client_tcp_req_repo);
            service_error("Could not recieve message request from client: \n", socket, SERVER);
        }

        struct stat statRes;
        if (lstat(client_tcp_req_repo->client_repo, &statRes) < 0) {
            printf("ERROR: stat cannot access inode %s\n", client_tcp_req_repo->client_repo);
            error(socket, 1, "ERROR: stat cannot access inode for client pull repo requestion");
        }
        client_tcp_req_repo->permission = statRes.st_mode;
        transmission_request(tcp_package(socket, client_tcp_req_repo, sizeof(tcp_repo), 0, 1), socket);

        char replay = get_reply(socket);
        if (replay == 'E' || replay == '?') operation_error("Client side error during pull request: \n");
        //printf("debug --- client_tcp_req_repo->idx: <%d>, client_tcp_req_repo->client_repo: <%s>, client_tcp_req_repo->origin: <%s>\n", client_tcp_req_repo->idx, client_tcp_req_repo->client_repo, client_tcp_req_repo->origin);
        listdir(socket, client_tcp_req_repo->idx, client_tcp_req_repo->client_repo, client_tcp_req_repo->origin);

        free(client_tcp_req_repo);

        end_tcp(socket, SERVER);
        /* ToDo: Finalize client download (pull repo request) */
    } else {
        printf("Something went horribly wrong with the attempt for the client request...\n");
        package_reply(socket, 'F');
        return;
    }
    printf("*\n*\n*\nCompleted client request\n\n");
}
