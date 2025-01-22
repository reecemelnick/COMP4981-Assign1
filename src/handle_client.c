#include "handle_client.h"
#include "get_time.h"
#include "serve_file.h"
#include "validate.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define OK_STATUS 200

void *handle_client(void *arg)
{
    int                newsockfd;
    struct sockaddr_in client_addr;
    socklen_t          client_addrlen;
    int                sockn;
    ssize_t            valread;
    char               buffer[BUFFER_SIZE];
    char               method[BUFFER_SIZE];
    char               uri[BUFFER_SIZE];
    char               version[BUFFER_SIZE];
    struct tm          tm_result;
    int                validate_result;
    char               filepath[BUFFER_SIZE];
    int                flags;
    int                file_status;
    const char        *time_buffer;
    int                content_length;

    newsockfd = *((int *)arg);

    free(arg);

    client_addrlen = sizeof(client_addr);

    // making sure read is blocking
    flags = fcntl(newsockfd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if(fcntl(newsockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl F_SETFL");
    }

    sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if(sockn < 0)
    {
        perror("sockname");
        return NULL;
    }

    valread = read(newsockfd, buffer, (size_t)BUFFER_SIZE);
    if(valread < 0)
    {
        perror("read");
        return NULL;
    }

    printf("REQUEST\n\n");

    sscanf(buffer, "%15s %255s %15s", method, uri, version);
    printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, uri, version);
    get_http_date(&tm_result);

    time_buffer = print_time(tm_result);

    printf("%s\n", time_buffer);

    printf("\n\nRESPONSE\n\n");

    validate_result = check_http_format(version, uri);
    if(validate_result == -1)
    {
        perror("validate result");
        return NULL;
    }

    if(verify_method(method, newsockfd) != 0)
    {
        close(newsockfd);
        return NULL;
    }

    if(strcmp(uri, "/") == 0)
    {
        snprintf(uri, sizeof(uri), "/index.html");
    }

    if(strchr(uri, '.') == NULL)
    {
        strncat(uri, ".html", sizeof(uri) - strlen(uri) - 1);
    }

    snprintf(filepath, sizeof(filepath), "../public%s", uri);

    file_status = check_file_status(filepath);

    content_length = get_file_size(filepath);

    if(file_status == OK_STATUS)
    {
        form_success_response(newsockfd, content_length);

        if(read_file(filepath, newsockfd, method) < 0)
        {
            perror("read file");
            return NULL;
        }
    }
    else
    {
        form_get_error(file_status, newsockfd);
    }

    close(newsockfd);

    return 0;
}
