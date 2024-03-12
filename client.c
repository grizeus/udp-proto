#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>

#define PORT    53
#define MAXLINE 1024
#define NUM_THREADS 4

void* send_thread(void* arg) {

    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed\n");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    int n;
    socklen_t len = sizeof(servaddr);

    while (1) {

        unsigned char query[] = {
            0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x07, 0x67, 0x6f, 0x6f,
            0x67, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00,
            0x00, 0x01, 0x00, 0x01
        };
        size_t query_len = sizeof(query);

        sendto(sockfd, query, query_len, 0,
               (const struct sockaddr*)&servaddr, sizeof(servaddr));

        printf("DNS query sent.\n");

        n = recvfrom(sockfd, buffer, MAXLINE, 0,
                     (struct sockaddr*)&servaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
        } else {
            perror("Receive failed");
        }

        usleep(2000000);
    }

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {

    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        if(pthread_create(&threads[i], NULL, send_thread, NULL) != 0) {
            perror("Thread creatiom failed\n");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed\n");
            exit(1);
        }
    }

    return 0;
}
