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
#define REQUEST_SIZE 512
#define OK_STATUS 200

void *handle_client(void *arg)
{
    int                newsockfd;                // client socket file descriptor
    struct sockaddr_in client_addr = {0};        // socket structure for client connection
    socklen_t          client_addrlen;           // length of client socket structure
    int                sockn;                    // result of calling getsockname
    ssize_t            valread;                  // number of bytes read from client socket
    char               buffer[BUFFER_SIZE];      // buffer for reading from client
    char               method[REQUEST_SIZE];     // string for request method
    char               uri[REQUEST_SIZE];        // string for request resource path
    char               version[REQUEST_SIZE];    // string for request protocol version
    // struct tm          tm_result;                // struct to represent timestamp
    int  validate_result;          // result of validating http request format
    char filepath[BUFFER_SIZE];    // filepath of the requested resource
    int  flags;                    // flags of newsock fd
    int  file_status;              // status code of request
    // char              *time_buffer;              // string to hold timestamp struct

    newsockfd = *((int *)arg);    // cast void pointer back to int

    free(arg);    // free copy of file descriptor

    client_addrlen = sizeof(client_addr);

    flags = fcntl(newsockfd, F_GETFL, 0);    // get flags of newsockfd
    flags &= ~O_NONBLOCK;                    // making sure read is blocking
    if(fcntl(newsockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl F_SETFL");
    }

    // store IP and port in client socket structure
    sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if(sockn < 0)
    {
        perror("sockname");
        close(newsockfd);
        return NULL;
    }

    valread = read(newsockfd, buffer, (size_t)BUFFER_SIZE);
    if(valread < 0)
    {
        perror("read");
        close(newsockfd);
        return NULL;
    }

    printf("REQUEST\n\n");

    sscanf(buffer, "%15s %255s %15s", method, uri, version);                                                               // parse request into buffer
    printf("[%s:%u]\n %s %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, uri, version);    // prints ip in human readable form, port in host byte order, request method, uri, version

    // TODO: is this block needed? Causing memory leak if rapidly sending requests
    // get date and print to server
    // get_http_date(&tm_result);
    // time_buffer = format_time(tm_result);

    // printf("%s\n", time_buffer);

    // free(time_buffer);
    // TODO: ^^^

    printf("\n\nRESPONSE\n\n");

    // validate that version and uri are valid
    validate_result = check_http_format(version, uri);
    if(validate_result == 0)
    {
        int *is_dir;    // result if uri is directory or not

        // validate the method
        if(verify_method(method, newsockfd) != 0)
        {
            close(newsockfd);
            return NULL;
        }

        snprintf(filepath, sizeof(filepath), "../public%s", uri);    // safely parse file path using hardcoded root directory

        is_dir = (int *)malloc(sizeof(int));
        if(is_dir == NULL)
        {
            perror("malloc fail");
            return NULL;
        }

        *is_dir = is_directory(filepath);    // check if directory is requested

        // if directory is requested, append index.html to path by default, update filepath
        if(*is_dir == 0)
        {
            strncat(uri, "/index.html", sizeof(uri) - strlen(uri) - 1);
            snprintf(filepath, sizeof(filepath), "../public%s", uri);    // safely parse file path using hardcoded root directory
        }

        // if resource has no extention, default to html, update filepath
        if(strchr(uri, '.') == NULL)
        {
            strncat(uri, ".html", sizeof(uri) - strlen(uri) - 1);
            snprintf(filepath, sizeof(filepath), "../public%s", uri);    // safely parse file path using hardcoded root directory
        }

        file_status = check_file_status(filepath);    // get status code

        free(is_dir);
    }
    else
    {
        file_status = validate_result;
    }

    if(file_status == OK_STATUS)
    {
        int         content_length;    // length of requested resource
        const char *content_type;      // type of requested resource

        content_length = get_file_size(filepath);       // get content length of file
        content_type   = get_content_type(filepath);    // get content tyoe of file

        form_success_response(newsockfd, content_length, content_type);

        if(strcmp(method, "GET") == 0)
        {
            // read requested resource
            if(read_file(filepath, newsockfd) < 0)
            {
                perror("read file");
                close(newsockfd);
                return NULL;
            }
        }
    }
    else
    {
        printf("STATUS: %d\n", file_status);
        form_get_error(file_status, newsockfd, method);    // if not OK, send error response
    }

    close(newsockfd);

    return 0;
}
