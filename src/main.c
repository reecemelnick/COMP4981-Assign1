#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(void)
{
    const int          PORT = 8080;
    char               buffer[BUFFER_SIZE];
    const int          LOOP = 10;
    struct sockaddr_in host_addr;
    socklen_t          host_addrlen;
    struct sockaddr_in client_addr;
    socklen_t          client_addrlen;
    ssize_t            valread;
    ssize_t            valwrite;
    int                sockn;
    char               method[BUFFER_SIZE];
    char               uri[BUFFER_SIZE];
    char               version[BUFFER_SIZE];
    const char         resp[] = "HTTP/1.0 200 OK\r\n"
                                "Server: webserver-c\r\n"
                                "Content-type: text/html\r\n\r\n"
                                "<html>hello, world</html>\r\n";

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

    client_addrlen = sizeof(client_addr);

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

    for(int i = 0; i < LOOP; i++)
    {
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr, &host_addrlen);

        if(newsockfd < 0)
        {
            perror("accept");
            continue;
        }

        printf("connection accepted");

        sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if(sockn < 0)
        {
            perror("sockname");
            continue;
        }

        valread = read(newsockfd, buffer, (size_t)BUFFER_SIZE);
        if(valread < 0)
        {
            perror("read");
            continue;
        }

        sscanf(buffer, "%15s %255s %15s", method, uri, version);
        printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, version, uri);

        valwrite = write(newsockfd, resp, strlen(resp));
        if(valwrite < 0)
        {
            perror("write");
            continue;
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
