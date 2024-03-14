#ifndef SEND_TO_CLIENT_H
#define SEND_TO_CLIENT_H

#include <netinet/in.h>

void send_to_client(int fd, const char* msg, struct sockaddr_in* client_addr);
#endif // !SEND_TO_CLIENT_H
