#include "validate.h"
#include "get_time.h"
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

#define OK_STATUS 200
#define FILE_NOT_FOUND 404
#define PERMISSION_DENIED 403
#define HEADER_SIZE 512

int check_http_format(const char *version, const char *uri)
{
    // verify version
    if(strcmp(version, "HTTP/1.1") != 0)
    {
        printf("not correct version\n");
        return -1;
    }

    printf("%s\n", uri);

    // verify uri to prevent backwards traveral through directories
    if(strstr(uri, "..") != NULL)
    {
        return PERMISSION_DENIED;
    }

    return 0;
}

// check if method is valid, if not send error responce
int verify_method(const char *method, int newsockfd)
{
    if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        char       *time_buffer;            // buffer for time
        struct tm   tm_result;              // time struct
        char        header[HEADER_SIZE];    // buffer to store header in
        const char *invalid_method_msg;     // string for invalid method message

        get_http_date(&tm_result);    // get current time

        time_buffer = format_time(tm_result);    // format time to human readable string

        // format header for 405 not allowed
        snprintf(header,
                 sizeof(header),
                 "HTTP/1.0 405 Method Not Allowed\r\n"
                 "Server: HTTPServer/1.0\r\n"
                 "Date: %s\r\n"
                 "Connection: close\r\n\r\n",
                 time_buffer);

        invalid_method_msg = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        write(newsockfd, header, strlen(header));
        write(newsockfd, invalid_method_msg, strlen(invalid_method_msg));
        printf("%s\n", header);
        free(time_buffer);
        return -1;
    }

    return 0;
}

// check if requested resource is a directory using stat
int is_directory(const char *filepath)
{
    struct stat file_stat;

    if(stat(filepath, &file_stat) == -1)
    {
        perror("stat");
        return -1;
    }

    if(S_ISDIR(file_stat.st_mode))
    {
        return 0;
    }

    return -1;
}

// check if requested resource exists and has appropriate permission
int check_file_status(char *filepath)
{
    struct stat file_stat;
    int         status_code;

    if(stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
    {
        status_code = OK_STATUS;
    }
    else if(access(filepath, F_OK) == -1)
    {
        status_code = FILE_NOT_FOUND;
    }
    else
    {
        status_code = PERMISSION_DENIED;
    }

    return status_code;
}
