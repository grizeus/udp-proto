#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#define PORT           53
#define MAX_BUFF_SIZE  1024
#define MAX_DOMAIN_LEN 255

struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;

};

char* extract_domain(const uint8_t* dns_payload, int payload_len) {

    struct DNSHeader* dnsHeader = (struct DNSHeader*)dns_payload;

    if ((ntohs(dnsHeader->flags) & 0x8000) == 0) {
        const uint8_t* question = dns_payload + sizeof(struct DNSHeader);
        char* extracted = malloc(MAX_DOMAIN_LEN);

        int i = 0;
        int domain_ind = 0;

        while (i < payload_len) {

            int label_len = question[i];

            for (int j = 1; j <= label_len; ++j) {
                extracted[domain_ind++] = (unsigned char)question[i + j];
            }

            if (label_len > 0) {
                extracted[domain_ind++] = '.';
            } else {
                break;
            }

            i += label_len + 1;
        }
        
        extracted[domain_ind - 1] = '\0';
        
        return extracted;
    }

    return NULL;
}

int main(int argc, char** argv) {
    int sockfd, dns_sockfd;
    struct sockaddr_in server_addr, client_addr, dns_addr;
    char buffer[MAX_BUFF_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    if ((dns_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("DNS socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&dns_addr, 0, sizeof(dns_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    dns_addr.sin_family = AF_INET;
    dns_addr.sin_addr.s_addr = inet_addr("8.8.8.8"); // Replace with your desired DNS server address
    dns_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Set the socket to non-blocking mode
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        perror("Failed to set non-blocking mode");
        exit(1);
    }

    while (1) {
        socklen_t client_addr_len = sizeof(client_addr);
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (recv_len == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Receive failed");
                exit(1);
            }
            // No data available, continue to the next iteration
            continue;
        }

        struct iphdr *ipHeader = (struct iphdr *)(buffer + client_addr_len);
        uint16_t ipHeaderLen = ipHeader->ihl * 4;
        struct udphdr *udpHeader = (struct udphdr *)(buffer + client_addr_len + ipHeaderLen);
        const uint8_t *dnsPayload = buffer + client_addr_len + ipHeaderLen + sizeof(struct udphdr);
        int payloadLength = ntohs(udpHeader->len) - sizeof(struct udphdr);

        struct DNSHeader *dnsHeader = (struct DNSHeader *)dnsPayload;
        uint16_t queryID = ntohs(dnsHeader->id);

        char *extracted = extract_domain(dnsPayload, payloadLength);
        if (extracted != NULL) {
            printf("DNS query id: %d, domain: %s\n", queryID, extracted);
            free(extracted);
        }

        // Forward the DNS query to the actual DNS server
        sendto(dns_sockfd, buffer, recv_len, 0, (const struct sockaddr *)&dns_addr, sizeof(dns_addr));
    }

    close(sockfd);
    close(dns_sockfd);
    return 0;
}