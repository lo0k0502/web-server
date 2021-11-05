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

void printString(char *str) {
    printf("%s\n", str);
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

int main (int argc, char **argv) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int ports[] = { 8080, 8081, 8082 };
    int *port = ports;
    int server_fd, client_fd;
    char buffer[BUFFER_SIZE];
    char *ptr;
    char separate[] = "\r\n";
    int fdimg;
    int i;
    FILE *file;
    FILE *store;
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
            printf("Got client connection!\n - connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            changePrintColor("white");

            memset(buffer, 0, BUFFER_SIZE);
            
            int readState = BUFFER_SIZE - 1;
            file = fopen(PACKET_FILE_NAME, "w");
            if (!file) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            
            char **headerLines;
            char **firstLineItems;
            char *method;
            char *uri;
            char **contentTypeLineItems;
            readState = read(client_fd, buffer, BUFFER_SIZE - 1);
            fwrite(buffer, sizeof(char), BUFFER_SIZE - 1, file);
            headerLines = strsplit(buffer, "\r\n");
            firstLineItems = strsplit(headerLines[0], " ");
            method = firstLineItems[0];
            uri = firstLineItems[1];

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
            i = 1;
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
            if (!strcmp(method, "POST")) {
                char *content_type;
                char *boundary;

                printf("Posting\n");

                for (i = 0; 1; i++) {
                    if (!strncmp(headerLines[i], "Content-Type", 12)) {
                        printString(headerLines[i]);
                        contentTypeLineItems = strsplit(headerLines[i], " ");
                        content_type = contentTypeLineItems[1];
                        break;
                    }
                }
                if (!strcmp(content_type, "text/html")) {
                    int title_length;
                    char *content;
                    char *file_name;
                    char destFilePath[] = "uploads/";
                    file = fopen(PACKET_FILE_NAME, "r");

                    file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                    content = strstr(buffer, "<!DOCTYPE html>");
                    ptr = strstr(content, "<title>");
                    ptr += 7;
                    title_length = strcspn(ptr, "<");

                    char title[title_length];
                    memset(title, '\0', title_length + 1);
                    strncpy(title, ptr, title_length);

                    strcat(destFilePath, title);
                    strcat(destFilePath, ".html");
                    store = fopen(destFilePath, "w");
                    fwrite(content, sizeof(char), strlen(content), store);

                    while (!feof(file)) {
                        file_bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);
                        fwrite(buffer, sizeof(char), strlen(buffer), store);
                    }
                    fclose(file);
                    fclose(store);
                } else if (!strncmp(content_type, "multipart/form-data", 19)) {
                    printf("Posted form\n");
                    content_type = stringBefore(content_type, ';');
                    printString(content_type);
                    boundary = stringAfter(contentTypeLineItems[2], "=");
                    printString(boundary);
                }
                responseWebpage(client_fd);
            }

            // get
            if (!strcmp(method, "GET")) {

                printf("Getting\n");

                if (!strcmp(uri, "/favicon.ico")) {
                    responseFile(client_fd, "assets/favicon.ico", "Content-Type: image/x-icon\r\n\r\n");
                } else if (!strcmp(uri, "/732a5748618c3f9083dc35e3edbd95034ae63e7a.png")) {
                    responseFile(client_fd, "assets/732a5748618c3f9083dc35e3edbd95034ae63e7a.png", "Content-Type: image/png\r\n\r\n");
                } else if (!strcmp(uri, "/script.js")) {
                    responseFile(client_fd, "web/script.js", "Content-Type: text/javascript\r\n\r\n");
                } else {
                    responseWebpage(client_fd);
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