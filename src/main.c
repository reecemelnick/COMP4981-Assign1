#include "main.h"
#include "serve_file.h"
#include "validate.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
sig_atomic_t static volatile g_running = 1;                 // NOLINT
static struct timespec ts              = {0, 100000000};    // NOLINT

// #define PUBLIC_DIR "../public"

void get_http_date(struct tm *result)
{
    time_t now = time(NULL);
    if(localtime_r(&now, result) == NULL)
    {
        perror("localtime_r");
    }
    // (buffer, BUFFER_SIZE, "%a, %d %b %Y %H:%M:%S GMT", &);
}

void handle_sigint(int signum)
{
    if(signum == SIGINT)
    {
        g_running = 0;
    }
}

int initialize_socket(void)
{
    const int          PORT = 8080;
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
    const int          DATE_NUM = 1900;
    struct sockaddr_in client_addr;
    socklen_t          client_addrlen;
    ssize_t            valread;
    // ssize_t            valwrite;
    int       sockn;
    int       validate_result;
    char      buffer[BUFFER_SIZE];
    struct tm tm_result;
    char      method[BUFFER_SIZE];
    char      uri[BUFFER_SIZE];
    char      version[BUFFER_SIZE];
    // const char resp[] = "HTTP/1.0 200 OK\r\n"
    //                     "Server: webserver-c\r\n"
    //                     "Content-type: text/html\r\n\r\n"
    //                     "<html>hello, world</html>\r\n";

    char        filepath[BUFFER_SIZE];
    struct stat file_stat;

    client_addrlen = sizeof(client_addr);

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

        sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if(sockn < 0)
        {
            perror("sockname");
            continue;
        }

        valread = read(newsockfd, buffer, (size_t)BUFFER_SIZE);
        if(valread < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                nanosleep(&ts, NULL);
                continue;
            }
            perror("read");
            continue;
        }

        sscanf(buffer, "%15s %255s %15s", method, uri, version);
        printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, uri, version);
        get_http_date(&tm_result);

        // <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
        printf("%d/%d/%d, %d:%d:%d\n", tm_result.tm_mday, tm_result.tm_mon + 1, tm_result.tm_year + DATE_NUM, tm_result.tm_hour, tm_result.tm_min, tm_result.tm_sec);

        validate_result = check_http_format(version, method, uri);
        if(validate_result == -1)
        {
            perror("validate result");
            break;
        }

        if(strcmp(uri, "/") == 0)
        {
            snprintf(uri, sizeof(uri), "/index.html");
        }

        snprintf(filepath, sizeof(filepath), "../public%s", uri);

        if(stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
        {
            printf("file exists\n");
            if(read_file(filepath, newsockfd) < 0)
            {
                perror("read file");
                break;
            }
        }
        else
        {
            printf("file does NOT exist\n");
            printf("%s\n", filepath);
        }

        close(newsockfd);
        printf("closing connection\n");
    }

    printf("done in loop\n");

    return server_sock;
}

int main(void)
{
    int sockfd = initialize_socket();

    close(sockfd);
    return 0;
}
