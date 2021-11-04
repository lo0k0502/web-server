#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "utility.h"

#define BUFFER_SIZE 2048
#define HTTP_200_RESPONSE "HTTP/1.1 200 OK\r\n"
#define PACKET_FILE_NAME "file"

extern int errno;

static void sigchld_handler() {
    pid_t PID;
    int status;

    while (PID = waitpid(-1, &status, WNOHANG) > 0) {
        changePrintColor("bold-yellow");
        printf("Child process %d terminated.\n", PID);
        changePrintColor("white");
    }

    /* Re-install handler */
    signal(SIGCHLD, sigchld_handler);
}

int main (int argc, char **argv) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int ports[] = { 8080, 8081, 8082 };
    int *port = ports;
    int server_fd, client_fd;
    char buffer[BUFFER_SIZE];
    char separate[] = "\r\n";
    int fdimg;
    FILE *file;
    size_t file_bytes;
    ssize_t state;
    int on = 1;

    signal(SIGCHLD, sigchld_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Server socket error\n");
        exit(1);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

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
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd == -1) {
            perror("Can't connect to client\n");
            continue;
        }

        if (!fork()) {
            close(server_fd);// child process

            changePrintColor("bold-yellow");
            printf("Got client connection!\n");
            printf(" - connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            changePrintColor("white");

            memset(buffer, 0, BUFFER_SIZE);
            
            int readState = BUFFER_SIZE - 1;
            file = fopen(PACKET_FILE_NAME, "w");
            if (!file) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            while (readState == BUFFER_SIZE - 1) {
                readState = read(client_fd, buffer, BUFFER_SIZE - 1);
                fwrite(buffer, sizeof(char), BUFFER_SIZE - 1, file);
            }
            fclose(file);

            file = fopen(PACKET_FILE_NAME, "r");
            
            struct stat sb;
            if (stat(PACKET_FILE_NAME, &sb) == -1){
                perror("stat");
                exit(EXIT_FAILURE);
            }

            char *file_contents = malloc(sb.st_size);
            int i = 1;
            changePrintColor("bold-green");
            printf("Headers: \n");
            changePrintColor("green");
            while (fscanf(file, "%[^\n] ", file_contents) != EOF) {
                if (!i && (!strstr(file_contents, ": ") && strncmp(file_contents, "-----", 5))) {
                    break;
                }
                printf("> %s\n", file_contents);
                if (i == 1) {
                    i = 0;
                }
            }
            changePrintColor("white");
            free(file_contents);
            fclose(file);

            // post
            if (!strncmp(buffer, "POST", 4)) {

                printf("Posting\n");

                if (!strncmp(buffer, "POST / ", 7)) {
                    file = fopen("web/index.html", "r");
                    write(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n", 44);
                    while(!feof(file)){
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        write(client_fd, buffer, file_bytes);
                    }
                    fclose(file);
                }
            }

            // get
            if (!strncmp(buffer, "GET", 3)) {

                printf("Getting\n");

                if (!strncmp(buffer, "GET /favicon.ico", 16)) {
                    file = fopen("assets/favicon.ico", "r");
                    write(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n\r\n", 47);
                    while(!feof(file)){
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        write(client_fd, buffer, file_bytes);
                    }
                    fclose(file);
                }
                else if (!strncmp(buffer, "GET /732a5748618c3f9083dc35e3edbd95034ae63e7a.png", 49)) {
                    file = fopen("assets/732a5748618c3f9083dc35e3edbd95034ae63e7a.png", "r");
                    write(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n\r\n", 44);
                    while(!feof(file)){
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        write(client_fd, buffer, file_bytes);
                    }
                    fclose(file);
                }
                else if (!strncmp(buffer, "GET /script.js", 14)) {
                    file = fopen("web/script.js", "r");
                    write(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript\r\n\r\n", 50);
                    while(!feof(file)){
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        write(client_fd, buffer, file_bytes);
                    }
                    fclose(file);
                }
                else {
                    file = fopen("web/index.html", "r");
                    write(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n", 44);
                    while(!feof(file)){
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        write(client_fd, buffer, file_bytes);
                    }
                    fclose(file);
                }
            }

            // remove("file");
            close(client_fd);
            printf("Connection closed\n");
            exit(0);
        }
        close(client_fd);// parent process
    }

    return 0;
}