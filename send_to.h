#ifndef SEND_TO_CLIENT_H
#define SEND_TO_CLIENT_H

#include <netinet/in.h>

ssize_t send_to(int fd, const char* msg, ssize_t msg_len, struct sockaddr_in* addr);
#endif // !SEND_TO_CLIENT_H
