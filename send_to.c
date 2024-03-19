#include <string.h>

#include "send_to.h"

ssize_t send_to(int fd, const char* msg, ssize_t msg_len, struct sockaddr_in* addr) {

    return sendto(fd, msg, msg_len, 0, (const struct sockaddr*)addr, sizeof(*addr));
}
