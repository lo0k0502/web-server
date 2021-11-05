#ifndef SERVER_UTILITY_H
#define SERVER_UTILITY_H

#define BUFFER_SIZE 2048

void serverBindandListen(int server_fd, struct sockaddr_in server_addr);
void responseOk(int fd);
void responseWebpage(int fd);
void responseFile(int fd, char *path, char *header);
void storeHeaderAndBody(char **headerLines, char **bodyLines, char **packetLines);
void printHeaders(char **headerLines);

#endif