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

#define PORT    53
#define MAXLINE 1024
#define NUM_THREADS 4

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
            0x02, 0x9a, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x03, 0x77, 0x77, 0x77,
            0x06, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00,
            0x00, 0x10, 0x00, 0x01
        };

        struct iphdr ipHeader;
        ipHeader.ihl = 5;
        ipHeader.version = 4; // IPv4
        ipHeader.tos = 0; // Type of Service
        ipHeader.tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct DNSHeader) + sizeof(query);
        ipHeader.id = htons(1234); // Random ID
        ipHeader.frag_off = 0; // Fragment Offset
        ipHeader.ttl = 64; // Time-to-Live
        ipHeader.protocol = IPPROTO_UDP; // UDP protocol
        ipHeader.check = 0; // Checksum will be computed automatically
        ipHeader.saddr = inet_addr("192.168.1.1"); // Source IP
        ipHeader.daddr = inet_addr("8.8.8.8"); // Destination IP

        struct udphdr udpHeader;
        udpHeader.source = htons(12345); // Source Port
        udpHeader.dest = htons(53); // Destination Port
        udpHeader.len = htons(sizeof(struct udphdr) + sizeof(struct DNSHeader) + sizeof(query)); // Length of UDP header + data
        udpHeader.check = 0; // Checksum will be computed automatically

        char packet[sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct DNSHeader) + sizeof(query)];

        // Copy headers and query into the packet buffer
        memcpy(packet, &ipHeader, sizeof(struct iphdr));
        memcpy(packet + sizeof(struct iphdr), &udpHeader, sizeof(struct udphdr));
        memcpy(packet + sizeof(struct iphdr) + sizeof(struct udphdr), query, sizeof(query));

        sendto(sockfd, packet, sizeof(packet), 0,
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

        usleep(200000);
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        if(pthread_create(&threads[i], NULL, send_thread, NULL) != 0) {
            perror("Thread creation failed\n");
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