#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/if_packet.h>

#include "receive_from_client.h"
#include "send_to_client.h"

#define DNS_PORT       53
#define PORT           8888
#define MAX_DOMAIN_LEN 255

struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

char* extract_domain(char* buffer) {

    struct iphdr *ipHeader = (struct iphdr *)buffer;
    uint16_t ipHeaderLen = ipHeader->ihl * 4;
    struct udphdr *udpHeader = (struct udphdr *)(buffer + ipHeaderLen);
    const uint8_t *dns_payload = buffer + ipHeaderLen + sizeof(struct udphdr);
    int payload_len = ntohs(udpHeader->len) - sizeof(struct udphdr);

    struct DNSHeader* dnsHeader = (struct DNSHeader*)dns_payload;
    uint16_t queryID = ntohs(dnsHeader->id);

    if ((ntohs(dnsHeader->flags) & 0x8000) == 0) {
        const uint8_t* question = dns_payload + sizeof(struct DNSHeader);
        char* extracted = malloc(MAX_DOMAIN_LEN + 1);
        if (extracted == NULL) {
            perror("Memory allocation failed");
            return NULL;
        }

        int domain_ind = 0;
        int i = 0;
        while (i < payload_len) {
            int label_len = question[i];

            if (domain_ind + label_len + 1 > MAX_DOMAIN_LEN) {
                fprintf(stderr, "Domain length exceeds maximum allowed\n");
                free(extracted);
                return NULL;
            }

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

        if (domain_ind > 0) {
            extracted[domain_ind - 1] = '\0'; // Null-terminate the string
        } else {
            fprintf(stderr, "No domain extracted\n");
            free(extracted);
            return NULL;
        }

        return extracted;
    }

    return NULL;
}

int main(int argc, char** argv) {
    int sockfd, dns_sockfd;
    struct sockaddr_in server_addr, client_addr, dns_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    if ((dns_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("DNS socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&dns_addr, 0, sizeof(dns_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // server_addr.sin_addr.s_addr = inet_addr("192.168.10.1");
    server_addr.sin_port = htons(PORT);
    // inet_pton(AF_INET, "192.168.10.1", &server_addr.sin_addr);

    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, "8.8.8.8", &dns_addr.sin_addr); // Replace with your desired DNS server address

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

    char buffer[MAX_BUFF_SIZE];

    while (1) {
        
        char* received_msg = receive_from_client(sockfd, &client_addr);
        // if nothing to receive, skip this iteration
        if (received_msg == NULL)
            continue;
        
        char* extracted = extract_domain(received_msg);
        if (extracted != NULL) {
            printf("Received DNS query with domain: %s\n", extracted);
            free(extracted);
        }

        const char answer[6] = "Roger";
        send_to_client(sockfd, answer, &client_addr);
        // Forward the DNS query to the actual DNS server
        sendto(dns_sockfd, received_msg, strlen(received_msg), 0, (const struct sockaddr *)&dns_addr, sizeof(dns_addr));
        free(received_msg);

        printf("DNS query sent to upstream.\n");

        // Receive the response
        socklen_t addr_len = sizeof(dns_addr);
        int recv_len = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr *)&dns_addr, &addr_len);
        if (recv_len < 0) {
            perror("Recvfrom failed");
            exit(EXIT_FAILURE);
        }

        // Print the response
        printf("Received response from DNS server:");
        for (int i = 0; i < recv_len; ++i) {
            printf("%02x ", buffer[i] & 0xFF);
        }
        printf("\n");
    }

    close(sockfd);
    close(dns_sockfd);
    return 0;
}