#include "send_to_client.h"
#include <string.h>

void send_to_client(int fd, const char* msg, struct sockaddr_in* client_addr) {

    sendto(fd, msg, strlen(msg), 0, (const struct sockaddr*)client_addr, sizeof(*client_addr));
}
