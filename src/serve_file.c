#include "serve_file.h"
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

int read_file(const char *filepath, int client_socket)
{
    int     filefd;
    char    file_buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    const char *response_header = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html\r\n"
                                  "Connection: close\r\n\r\n";

    write(client_socket, response_header, strlen(response_header));

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
