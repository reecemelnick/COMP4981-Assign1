#include "get_time.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>

#define TIME_BUFFER 64

void get_http_date(struct tm *result)
{
    time_t now = time(NULL);
    if(localtime_r(&now, result) == NULL)
    {
        perror("localtime_r");
    }
    // (buffer, BUFFER_SIZE, "%a, %d %b %Y %H:%M:%S GMT", &);
}

char *print_time(struct tm tm_result)
{
    char *timestamp = malloc(TIME_BUFFER);
    if(timestamp == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    if(strftime(timestamp, TIME_BUFFER, "%a, %d %b %Y %H:%M:%S GMT", &tm_result) == 0)
    {
        fprintf(stderr, "strftime failed\n");
        free(timestamp);
        return NULL;
    }

    return timestamp;
}
