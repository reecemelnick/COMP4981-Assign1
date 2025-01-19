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

int check_http_format(const char *version, const char *method, const char *uri)
{
    // verify version
    if(strcmp(version, "HTTP/1.1") != 0)
    {
        printf("not correct version\n");
        return -1;
    }

    // verify method
    if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0 && strcmp(method, "POST") != 0)
    {
        printf("Not valid method\n");
        return -1;
    }

    // verify uri
    if(strstr(uri, "..") != NULL)
    {
        printf("invalid uri\n");
        return -1;
    }

    printf("Format is good\n");

    return 0;
}
