#include "validate.h"
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

    // verify uri
    if(strstr(uri, "..") != NULL)
    {
        printf("invalid uri\n");
        return -1;
    }

    return 0;
}

int verify_method(const char *method, int newsockfd)
{
    if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        const char *time_buffer;
        struct tm   tm_result;
        char        header[HEADER_SIZE];
        const char *invalid_method_msg;

        get_http_date(&tm_result);

        time_buffer = print_time(tm_result);

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
        return -1;
    }

    return 0;
}

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
