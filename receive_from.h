#ifndef RECEIVE_FROM_CLIENT_H
#define RECEIVE_FROM_CLIENT_H

#include <netinet/in.h>
#include <sys/types.h>

#define MAX_BUFF_SIZE  2048

char* receive_from(int fd, struct sockaddr_in* client_addr, ssize_t* recv_len);
#endif // !RECEIVE_FROM_CLIENT_H
