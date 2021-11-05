#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>

#include "server_utility.h"
#include "utility.h"

extern int errno;

void serverBindandListen(int server_fd, struct sockaddr_in server_addr) {
    int ports[] = { 8080, 8081, 8082 };
    int *port = ports;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(*port);

    int old_port;
    char confirm;

    while (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        if (errno == EADDRINUSE) {
            old_port = *port;
            if (*port) {
                printf("Port %d is in use, would you like to use port %d?", old_port, *++port);
                changePrintColor("bold-cyan");
                printf(" (y/n) ");
                changePrintColor("white");

                while (confirm != 'y' && confirm != 'n') {
                    confirm = getch();
                }
                if (confirm == 'y') {
                    printf("yes\n");
                    server_addr.sin_port = htons(*port);
                    continue;
                } else {
                    printf("no\n");
                    exit(0);
                }
            }
            printf("Run out of ports\n");
        }
        perror("Server bind error\n");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 10) == -1) {
        perror("Server listen error\n");
        close(server_fd);
        exit(1);
    }

    printf("Server is listening on port %d\n", *port);
}

void responseOk(int fd) {
    write(fd, "HTTP/1.1 200 OK\r\n", 17);
}

void responseWebpage(int fd) {
    FILE *file = fopen("web/index.html", "r");
    char buffer[BUFFER_SIZE];
    size_t file_bytes;
    responseOk(fd);
    write(fd, "Content-Type: text/html\r\n\r\n", 27);
    while(!feof(file)){
        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
        write(fd, buffer, file_bytes);
    }
    fclose(file);
}

void responseFile(int fd, char *path, char *header) {
    FILE *file = fopen(path, "r");
    char buffer[BUFFER_SIZE];
    size_t file_bytes;
    responseOk(fd);
    write(fd, header, strlen(header));
    while(!feof(file)){
        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
        write(fd, buffer, file_bytes);
    }
    fclose(file);
}

void storeHeaderAndBody(char **headerLines, char **bodyLines, char **packetLines) {
    char **arr = packetLines;
    *headerLines = *arr;
    *headerLines++;
    *arr++;
    while (*arr) {
        if (!strstr(*arr, ": ")) {
            break;
        }
        *headerLines = *arr;
        *headerLines++;
        *arr++;
    }
    while (*arr) {
        *bodyLines = *arr;
        *bodyLines++;
        *arr++;
    }
}

void printHeaders(char **headerLines) {
    int i = 0;
    changePrintColor("bold-green");
    printf("Headers: \n");
    changePrintColor("green");
    // while (!i && (!strstr(file_contents, ": "))) {
    //     printf("> %s\n", file_contents);
    //     if (i == 1) {
    //         i = 0;
    //     }
    // }
    while (*headerLines) {
        if (i != 0 && !strstr(*headerLines, ": ")) {
            break;
        }
        printf("> %s\n", *headerLines);
        *headerLines++;
        if (i == 0) {
            i++;
        }
    }
    changePrintColor("white");
}