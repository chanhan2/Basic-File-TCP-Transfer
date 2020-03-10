/*
    Simple TCP Server
*/

/* external library */
#include "server.h"

int main(int argc, char *argv[]) {
    int sockfd = start_tcp_server(PORT), newsockfd, pid;
    socklen_t clilen;
    struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) perror("ERROR on accept: ");
        pid = fork();
        if (pid < 0) perror("ERROR on fork: ");
        if (pid == 0)  {
            close(sockfd);
            client_request(newsockfd); //relay_message(newsockfd);
            exit(0);
        } else close(newsockfd);
    }

    close(sockfd);
    return 0;
}
