#include "receive_from_client.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_BUFF_SIZE  2048

char* receive_from_client(int fd, struct sockaddr_in* client_addr) {

    char buffer[MAX_BUFF_SIZE];

    socklen_t client_addr_len = sizeof(*client_addr);
    ssize_t recv_len = recvfrom(fd, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr*)client_addr, &client_addr_len);

    if (recv_len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Receive failed");
        }
        return NULL;
    }

    printf("Received from %s:%d\n", inet_ntoa(client_addr->sin_addr), htons(client_addr->sin_port));

    char* msg = malloc(recv_len + 1);
    memcpy(msg, buffer, recv_len);
    msg[recv_len] = '\0';

    return msg;
}
