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

// bind the server and let it listen
void serverBindandListen(int server_fd, struct sockaddr_in server_addr) {
    int ports[] = { 8080, 8081, 8082 }, *port = ports, old_port;
    char confirm;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(*port);

    while (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        if (errno == EADDRINUSE) {
            // if the error is caused by the port is in use

            old_port = *port;
            if (*port) {
                // if there is another alternate port, then ask if the user wants to use it
                printf("Port %d is in use, would you like to use port %d?", old_port, *++port);
                changePrintColor("bold-cyan");
                printf(" (y/n) ");
                changePrintColor("white");

                while (confirm != 'y' && confirm != 'n') {
                    // check whether y or n is pressed, and react immediately
                    confirm = getch();
                }
                if (confirm == 'y') {
                    // if yes, set the port to the alternate port and try to bind again
                    printString("yes");
                    server_addr.sin_port = htons(*port);
                    continue;
                } else {
                    // if no, then exit without error
                    printString("no");
                    exit(0);
                }
            }
            printString("Run out of ports");
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

// response status code 200 OK
void responseOk(int fd) {
    write(fd, "HTTP/1.1 200 OK\r\n", 17);
}

// response the default webpage
void responseWebpage(int fd) {
    FILE *file = fopen("web/index.html", "r");
    char buffer[BUFFER_SIZE];
    size_t file_bytes;
    responseOk(fd);
    write(fd, "Content-Type: text/html\r\n\r\n", 27);
    while (!feof(file)){
        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
        write(fd, buffer, file_bytes);
    }
    fclose(file);
}

// response with file
void responseFile(int fd, char *path, char *header) {
    FILE *file = fopen(path, "r");
    char buffer[BUFFER_SIZE];
    size_t file_bytes;
    responseOk(fd);
    write(fd, header, strlen(header));
    while (!feof(file)){
        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
        write(fd, buffer, file_bytes);
    }
    fclose(file);
}

// manipulate packet into headers and body (stored in two arrays of string)
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

// print out the headers
void printHeaders(char **headerLines) {
    int i = 0;
    changePrintColor("bold-green");
    printString("Headers: ");
    changePrintColor("green");
    
    while (*headerLines) {
        if (i != 0 && !strstr(*headerLines, ": ")) {
            break;
        }
        printf("> %s\n", *headerLines);
        fflush(stdout);
        *headerLines++;
        if (i == 0) {
            i++;
        }
    }
    changePrintColor("white");
}