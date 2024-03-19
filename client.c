#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>

#define PORT          8888
#define MAX_BUFF_SIZE 1024
#define NUM_THREADS   4

struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

void* send_thread(void* arg) {


    int sockfd;
    char buffer[MAX_BUFF_SIZE];
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    while (1) {
            unsigned char query[] = {
            0x26, 0xa9, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x03, 0x77, 0x77, 0x77, 0x06, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
            0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01
        };

        if (sendto(sockfd, query, sizeof(query), 0,
                   (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            perror("Sendto failed");
            exit(1);
        }

        printf("DNS query sent to proxy.\n");

        socklen_t addr_len = sizeof(servaddr);
        int recv_len = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0,
                                (struct sockaddr*)&servaddr, &addr_len);
        if (recv_len > 0) {
            printf("Server: %s\n", buffer);
        } else {
            perror("Receive failed");
        }

        usleep(10000);
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, send_thread, NULL) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed");
            exit(1);
        }
    }

    return 0;
}
