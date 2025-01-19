#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void get_http_date(struct tm *result);

int initialize_socket(void);

int accept_clients(int server_sock, struct sockaddr_in host_addr, socklen_t host_addrlen);

int read_file(const char *filepath, int client_socket);

void handle_sigint(int signum);
int  set_socket_non_blocking(int sockfd);
