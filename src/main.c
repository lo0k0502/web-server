#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "server_utility.h"
#include "utility.h"

#define PACKET_FILE_NAME "file"

static void sigchld_handler() {
    pid_t PID;
    int status;

    while (PID = waitpid(-1, &status, WNOHANG) > 0) {
        changePrintColor("bold-yellow");
        printf("Child process %d terminated.\n", PID);
        changePrintColor("white");
    }

    signal(SIGCHLD, sigchld_handler);
}

int main (int argc, char **argv) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int server_fd, client_fd, i, on = 1, readState = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE], *packet, *ptr;
    FILE *file, *store;

    // handle zombie process
    signal(SIGCHLD, sigchld_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Server socket error\n");
        exit(1);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    serverBindandListen(server_fd, server_addr);

    for (;;) {

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
            
            char **packetLines;
            char **firstLineItems;
            char *method;
            char *uri;
            char **contentTypeLineItems;

            // read the packet and manipulate
            readState = read(client_fd, buffer, BUFFER_SIZE - 1);
            packet = buffer;
            packetLines = strsplit(packet, "\r\n");
            firstLineItems = strsplit(packetLines[0], " ");
            method = firstLineItems[0];
            uri = firstLineItems[1];

            // while (readState == BUFFER_SIZE - 1) {
            //     readState = read(client_fd, buffer, BUFFER_SIZE - 1);
            //     fwrite(buffer, sizeof(char), BUFFER_SIZE - 1, file);
            // }

            printHeaders(packetLines);

            // manipulate post request
            if (!strcmp(method, "POST")) {
                char *content_type;

                // check the content type
                for (i = 0; 1; i++) {
                    if (!strncmp(packetLines[i], "Content-Type", 12)) {
                        contentTypeLineItems = strsplit(packetLines[i], " ");
                        content_type = contentTypeLineItems[1];
                        break;
                    }
                }

                char destFilePath[] = "uploads/"; // the file path which will be stored (must be in uploads folder)

                if (!strcmp(content_type, "text/html")) {
                    // if it's a html, find the title in the file to be the file name, then store the html file
                    int title_length;
                    char *content;
                    
                    content = strstr(packet, "<!DOCTYPE html>");
                    ptr = (strstr(content, "<title>")) + 7;
                    title_length = strcspn(ptr, "<");

                    char title[title_length];
                    memset(title, '\0', title_length + 1);
                    strncpy(title, ptr, title_length);

                    strcat(destFilePath, title);
                    strcat(destFilePath, ".html");
                    store = fopen(destFilePath, "w");
                    if (store == NULL) {
                        perror("Store file error");
                        exit(1);
                    }

                    fwrite(content, sizeof(char), strlen(content), store);

                    fclose(store);
                } else if (!strncmp(content_type, "multipart/form-data", 19)) {
                    // if it's a form, find the boundary to find the position of the data
                    char *form_body;
                    char **formBodyItems;
                    char file_name[10];

                    content_type = stringBefore(content_type, ';');
                    ptr = stringAfter(contentTypeLineItems[2], "=");

                    char boundary[strlen(ptr) + 2];

                    memset(boundary, '\0', strlen(ptr) + 3);
                    strcat(boundary, "--");
                    strcat(boundary, ptr);
                    form_body = strstr(packet, boundary);
                    formBodyItems = strsplit(form_body, "\r\n");

                    // manipulate the form data (only for html file now)

                    // manipulate the form data headers
                    while (*formBodyItems) {
                        if (!strncmp(*formBodyItems, "Content-Disposition", 19)) {
                            // store its file name in the Content-Disposition line
                            ptr = (strstr(*formBodyItems, "filename=\"")) + 10;
                            memset(file_name, '\0', strlen(ptr));
                            while (*ptr) {
                                if (*ptr == '\"') {
                                    break;
                                }
                                strcatChar(file_name, *ptr);
                                *ptr++;
                            }
                        } else if (!strncmp(*formBodyItems, "Content-Type", 12)) {
                            // store the content type of the form data
                            ptr = *formBodyItems + 14;
                            break;
                        }
                        *formBodyItems++;
                    }
                    if (!strcmp(ptr, "text/html")) {
                        // if the content type of the form data is html

                        strcat(destFilePath, file_name);
                        store = fopen(destFilePath, "w");
                        if (store == NULL) {
                            perror("Store file error");
                            exit(1);
                        }

                        // find the beginning of the html file (which is <!DOCTYPE html>)
                        while (*formBodyItems) {
                            if (!strncmp(*formBodyItems, "<!DOCTYPE html>", 15)) {
                                break;
                            }
                            *formBodyItems++;
                        }

                        // store the html file line by line until the ending boundary
                        while (*formBodyItems) {
                            if (!strncmp(*formBodyItems, boundary, strlen(boundary))) {
                                break;
                            }
                            fwrite(strcat(*formBodyItems, "\r\n"), sizeof(char), strlen(*formBodyItems) + 2, store);
                            *formBodyItems++;
                        }

                        fclose(store);
                    }
                }

                // response the default webpage
                responseWebpage(client_fd);
            }

            // manipulate get request
            if (!strcmp(method, "GET")) {

                if (!strcmp(uri, "/favicon.ico")) {
                    // response the favicon
                    responseFile(client_fd, "assets/favicon.ico", "Content-Type: image/x-icon\r\n\r\n");
                } else if (!strcmp(uri, "/732a5748618c3f9083dc35e3edbd95034ae63e7a.png")) {
                    // response the specific image
                    responseFile(client_fd, "assets/732a5748618c3f9083dc35e3edbd95034ae63e7a.png", "Content-Type: image/png\r\n\r\n");
                } else if (!strcmp(uri, "/script.js")) {
                    // response the script.js file
                    responseFile(client_fd, "web/script.js", "Content-Type: text/javascript\r\n\r\n");
                } else {
                    // response the default webpage
                    responseWebpage(client_fd);
                }
            }

            close(client_fd);
            printString("Connection closed");
            exit(0);
        }
        close(client_fd);// parent process
    }

    return 0;
}