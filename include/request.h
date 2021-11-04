#ifndef REQUEST_H
#define REQUEST_H

typedef struct request {
    char *method;
    char *uri;
    char *clientIP;
    int clientPort;
    char **headers;
    char *path;
    char *body;
} Request;

Request *newRequest(char *packet, struct sockaddr_in *sin);

#endif