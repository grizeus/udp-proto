#ifndef RECEIVE_FROM_CLIENT_H
#define RECEIVE_FROM_CLIENT_H

#include <netinet/in.h>

char* receive_from_client(int fd, struct sockaddr_in* client_addr);
#endif // !RECEIVE_FROM_CLIENT_H
