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

static sig_atomic_t volatile g_running = 1;    // NOLINT // controls accept loop

// sets g_running to 0 to exit infinite accept loop
void handle_sigint(int signum)
{
    if(signum == SIGINT)
    {
        g_running = 0;
    }
}

// socket, bind, listen
int initialize_socket(void)
{
    const uint16_t PORT = 8080;    // Port to bind to

    struct sockaddr_in host_addr;       // IPv4 socket structure for server
    socklen_t          host_addrlen;    // length of the sockaddr struct

    // create tcp socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT
    if(sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    host_addrlen = sizeof(host_addr);

    host_addr.sin_family      = AF_INET;        // AF_INET for IPv4
    host_addr.sin_port        = htons(PORT);    // Port in network byte order
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to specified ip and port
    if(bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0)
    {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // sets socket to listening with possiblilty of max number of connections in queue
    if(listen(sockfd, SOMAXCONN) != 0)
    {
        perror("listen");
        close(sockfd);
        return -1;
    }

    // calls function to accept clients
    if(accept_clients(sockfd, host_addr, host_addrlen) < 0)
    {
        perror("Accept clients");
        return -1;
    }

    return sockfd;
}

// sets file descriptor to non-blocking mode
int set_socket_non_blocking(int sockfd)
{
    // fetch current flags
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl(F_GETFL)");
        return -1;
    }

    // set nonblock flag
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl(F_SETFL)");
        return -1;
    }

    return 0;
}

int accept_clients(int server_sock, struct sockaddr_in host_addr, socklen_t host_addrlen)
{
    pthread_t thread_id;    // thread struct that reperesents does new connection work
    int      *new_fd;       // pointer to new socket file descriptor

    static struct timespec ts = {0, 100000000};    // NOLINT // timeout interval of 100 ms

    signal(SIGINT, &handle_sigint);    // sets signal handler for CTRL-C

    if(set_socket_non_blocking(server_sock) < 0)
    {
        return -1;
    }

    // main loop for accepting clients
    while(g_running)
    {
        // accept new connection
        int newsockfd = accept(server_sock, (struct sockaddr *)&host_addr, &host_addrlen);

        if(newsockfd < 0)
        {
            if(errno == EAGAIN)
            {
                nanosleep(&ts, NULL);    // timeout between failed accepts
                continue;
            }
            perror("accept");
            break;
        }

        new_fd = (int *)malloc(sizeof(int));    // create copy of file descriptor
        if(new_fd == NULL)
        {
            perror("malloc failed");
            close(newsockfd);
            continue;
        }

        *new_fd = newsockfd;

        // spawn thread to handle client
        if(pthread_create(&thread_id, NULL, handle_client, (void *)new_fd) != 0)
        {
            perror("pthread_create");
            close(newsockfd);
            continue;
        }

        pthread_detach(thread_id);    // detach thread from main thread
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
