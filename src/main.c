#include "main.h"
#include "handle_client.h"
#include "serve_file.h"
#include "validate.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

sig_atomic_t static volatile g_running = 1;                 // NOLINT
static struct timespec ts              = {0, 100000000};    // NOLINT

void handle_sigint(int signum)
{
    if(signum == SIGINT)
    {
        g_running = 0;
    }
}

int initialize_socket(void)
{
    const int PORT = 8080;
    // const char        *IP   = "192.168.0.161";
    struct sockaddr_in host_addr;
    socklen_t          host_addrlen;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT
    if(sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    printf("Socket created successfully.\n");

    host_addrlen = sizeof(host_addr);

    host_addr.sin_family      = AF_INET;
    host_addr.sin_port        = htons(PORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // host_addr.sin_addr.s_addr = inet_addr(IP);

    if(bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0)
    {
        perror("bind");
        close(sockfd);
        return -1;
    }

    printf("socket was bound successfully\n");

    if(listen(sockfd, SOMAXCONN) != 0)
    {
        perror("listen");
        close(sockfd);
        return -1;
    }

    printf("server listening for connections\n");

    if(accept_clients(sockfd, host_addr, host_addrlen) < 0)
    {
        perror("Accept clients");
        return -1;
    }

    return sockfd;
}

int set_socket_non_blocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl(F_SETFL)");
        return -1;
    }

    return 0;
}

int accept_clients(int server_sock, struct sockaddr_in host_addr, socklen_t host_addrlen)
{
    pthread_t thread_id;
    int      *new_fd;

    signal(SIGINT, &handle_sigint);

    if(set_socket_non_blocking(server_sock) < 0)
    {
        return -1;
    }

    while(g_running)
    {
        int newsockfd = accept(server_sock, (struct sockaddr *)&host_addr, &host_addrlen);

        if(newsockfd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                nanosleep(&ts, NULL);
                continue;
            }
            perror("accept");
            break;
        }

        new_fd = malloc(sizeof(int));

        *new_fd = newsockfd;

        if(pthread_create(&thread_id, NULL, handle_client, (void *)new_fd) != 0)
        {
            perror("pthread_create");
            continue;
        }

        pthread_detach(thread_id);
    }

    printf("done in loop\n");

    return server_sock;
}

int main(void)
{
    int sockfd = initialize_socket();

    close(sockfd);

    printf("exiting application\n");
    return 0;
}
