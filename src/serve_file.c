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

#define BUFFER_SIZE 8192
#define FILE_NOT_FOUND 404
#define PERMISSION_DENIED 403
#define HEADER_SIZE 512

// read file and write contents to client socket
int read_file(const char *filepath, int client_socket)
{
    int     filefd;                      // file descriptor for file to be served
    char    file_buffer[BUFFER_SIZE];    // buffer for file contents
    ssize_t bytes_read;                  // bytes read in read call

    filefd = open(filepath, O_RDONLY | O_CLOEXEC);    // open file and assign file descriptor
    if(filefd < 0)
    {
        perror("opening file");
        return -1;
    }

    // read all data from file and write to client socket
    while((bytes_read = read(filefd, file_buffer, sizeof(file_buffer))) > 0)
    {
        write(client_socket, file_buffer, (size_t)bytes_read);
    }

    close(filefd);

    return 0;
}

void form_success_response(int newsockfd, int content_length, const char *content_type)
{
    const char *time_buffer;            // buffer to store readable time
    struct tm   tm_result;              // time structure
    char        header[HEADER_SIZE];    // buffer to hold contents of response

    get_http_date(&tm_result);    // get current time

    time_buffer = format_time(tm_result);    // format time to human readable string

    // format response header for status 200 OK
    snprintf(header,
             sizeof(header),
             "HTTP/1.0 200 OK\r\n"
             "Server: HTTPServer/1.0\r\n"
             "Date: %s\r\n"
             "Connection: close\r\n"
             "Content-Length: %d\r\n"
             "Content-Type: %s\r\n\r\n",
             time_buffer,
             content_length,
             content_type);

    write(newsockfd, header, strlen(header));    // send response to client

    printf("%s\n", header);
}

// returns content length of file
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

// returns content type by finding pointer to last occurence of '.'.
const char *get_content_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if(!ext)
    {
        return "application/octet-stream";
    }

    if(strcmp(ext, ".html") == 0)
    {
        return "text/html";
    }
    if(strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }
    if(strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }
    if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    if(strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }
    if(strcmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }
    if(strcmp(ext, ".swf") == 0)
    {
        return "application/x-shockwave-flash";
    }

    return "application/octet-stream";
}

void form_get_error(int status_code, int newsockfd, const char *method)
{
    const char *time_buffer;            // buffer to store readable time
    struct tm   tm_result;              // time structure
    char        header[HEADER_SIZE];    // buffer to hold contents of response

    get_http_date(&tm_result);    // get current time

    time_buffer = format_time(tm_result);    // format time to human readable string

    if(status_code == FILE_NOT_FOUND)
    {
        // format response header for file not found
        snprintf(header,
                 sizeof(header),
                 "HTTP/1.0 404 Not Found\r\n"
                 "Server: HTTPServer/1.0\r\n"
                 "Date: %s\r\n"
                 "Connection: close\r\n\r\n",
                 time_buffer);

        write(newsockfd, header, strlen(header));    // write header to socket

        // if method is GET, also write body
        if(strcmp(method, "GET") == 0)
        {
            const char *not_found_page;
            not_found_page = "<html><body><h1>404 Not Found</h1></body></html>";
            write(newsockfd, not_found_page, strlen(not_found_page));
        }
    }
    else if(status_code == PERMISSION_DENIED)
    {
        // format response header for file not found
        snprintf(header,
                 sizeof(header),
                 "HTTP/1.0 403 Forbidden\r\n"
                 "Server: HTTPServer/1.0\r\n"
                 "Date: %s\r\n"
                 "Connection: close\r\n\r\n",
                 time_buffer);

        write(newsockfd, header, strlen(header));

        // if method is GET, also write body
        if(strcmp(method, "GET") == 0)
        {
            const char *no_access_page;

            no_access_page = "<html><body><h1>403 Forbidden</h1></body></html>";
            write(newsockfd, no_access_page, strlen(no_access_page));
        }
    }

    printf("%s\n", header);
}
