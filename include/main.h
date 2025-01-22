#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int initialize_socket(void);

int accept_clients(int server_sock, struct sockaddr_in host_addr, socklen_t host_addrlen);

void handle_sigint(int signum);

int set_socket_non_blocking(int sockfd);

void *thread_function(void *arg);
