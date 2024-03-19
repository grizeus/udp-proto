#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "receive_from.h"

char* receive_from(int fd, struct sockaddr_in* client_addr, ssize_t* recv_len) {

    char buffer[MAX_BUFF_SIZE];

    socklen_t client_addr_len = sizeof(*client_addr);
    *recv_len = recvfrom(fd, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr*)client_addr, &client_addr_len);

    if (*recv_len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Receive failed");
        }
        return NULL;
    }

    printf("Received from %s:%d\n", inet_ntoa(client_addr->sin_addr), htons(client_addr->sin_port));

    char* msg = malloc(*recv_len);
    memcpy(msg, buffer, *recv_len);

    return msg;
}
