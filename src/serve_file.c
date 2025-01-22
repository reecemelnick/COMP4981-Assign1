#include "serve_file.h"
#include "get_time.h"
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
#define FILE_NOT_FOUND 404
#define PERMISSION_DENIED 403
#define HEADER_SIZE 512

int read_file(const char *filepath, int client_socket, const char *method)
{
    int     filefd;
    char    file_buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    if(strcmp(method, "HEAD") == 0)
    {
        return 0;
    }

    filefd = open(filepath, O_RDONLY | O_CLOEXEC);
    if(filefd < 0)
    {
        perror("opening file");
        return -1;
    }

    while((bytes_read = read(filefd, file_buffer, sizeof(file_buffer))) > 0)
    {
        write(client_socket, file_buffer, (size_t)bytes_read);
    }

    close(filefd);

    return 0;
}

void form_success_response(int newsockfd, int content_length)
{
    const char *time_buffer;
    struct tm   tm_result;
    char        header[HEADER_SIZE];

    get_http_date(&tm_result);

    time_buffer = print_time(tm_result);

    snprintf(header,
             sizeof(header),
             "HTTP/1.0 200 OK\r\n"
             "Server: HTTPServer/1.0\r\n"
             "Date: %s\r\n"
             "Connection: close\r\n"
             "Content-Length: %d\r\n"
             "Content-Type: text/html\r\n\r\n",
             time_buffer,
             content_length);

    write(newsockfd, header, strlen(header));

    printf("%s\n", header);
}

int get_file_size(const char *filepath)
{
    struct stat file_stat;

    if(stat(filepath, &file_stat) == 0)
    {
        return (int)file_stat.st_size;
    }

    perror("stat");
    return -1;
}

void form_get_error(int status_code, int newsockfd)
{
    const char *time_buffer;
    struct tm   tm_result;
    char        header[HEADER_SIZE];

    get_http_date(&tm_result);

    time_buffer = print_time(tm_result);

    if(status_code == FILE_NOT_FOUND)
    {
        const char *not_found_page;

        snprintf(header,
                 sizeof(header),
                 "HTTP/1.0 404 Not Found\r\n"
                 "Server: HTTPServer/1.0\r\n"
                 "Date: %s\r\n"
                 "Connection: close\r\n\r\n",
                 time_buffer);

        not_found_page = "<html><body><h1>404 Not Found</h1></body></html>";
        write(newsockfd, header, strlen(header));
        write(newsockfd, not_found_page, strlen(not_found_page));
    }
    else if(status_code == PERMISSION_DENIED)
    {
        const char *no_access_page;

        snprintf(header,
                 sizeof(header),
                 "HTTP/1.0 403 Forbidden\r\n"
                 "Server: HTTPServer/1.0\r\n"
                 "Date: %s\r\n"
                 "Connection: close\r\n\r\n",
                 time_buffer);

        no_access_page = "<html><body><h1>403 Forbidden</h1></body></html>";
        write(newsockfd, header, strlen(header));
        write(newsockfd, no_access_page, strlen(no_access_page));
    }

    printf("%s\n", header);
}
