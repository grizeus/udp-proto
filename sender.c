#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define DNS_PORT 53

// DNS header structure
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUF_SIZE];

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address setup
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, "8.8.8.8", &servaddr.sin_addr); // Google's public DNS server

    // Server address setup
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, "8.8.8.8", &servaddr.sin_addr); // Google's public DNS server

    // // Prepare DNS query
    // struct DNSHeader *dns = (struct DNSHeader *)buffer;
    // dns->id = htons(0x1234); // Random ID
    // dns->flags = htons(0x0100); // Standard query
    // dns->qdcount = htons(0x0001); // One question
    // dns->ancount = 0;
    // dns->nscount = 0;
    // dns->arcount = 0;

    // // Add query string
    // strcpy(buffer + sizeof(struct DNSHeader), "\x06google\x03com\x00"); // google.com

    // // Send the DNS query
    // if (sendto(sockfd, buffer, sizeof(struct DNSHeader) + strlen("\x06google\x03com\x00") + 1, 0,
    //            (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    //     perror("Sendto failed");
    //     exit(EXIT_FAILURE);
    // }

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
    ipHeader.saddr = inet_addr("172.31.15.255"); // Source IP
    ipHeader.daddr = inet_addr("1.1.1.1"); // Destination IP

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


    // Send the DNS query
    if (sendto(sockfd, packet, sizeof(packet), 0,
               (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Sendto failed");
        exit(EXIT_FAILURE);
    }

    printf("DNS query sent.\n");

    // Receive the response
    socklen_t addr_len = sizeof(servaddr);
    int recv_len = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&servaddr, &addr_len);
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

    close(sockfd);
    return 0;
}
