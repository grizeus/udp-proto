#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/if_packet.h>

#include "receive_from.h"
#include "send_to.h"
#include "extract_dns.h"

#define DNS_PORT       53
#define PORT           8888

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
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

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
        ssize_t recv_cl_len;
        char* recv_msg = receive_from(sockfd, &client_addr, &recv_cl_len);
        // if nothing to receive, skip this iteration
        if (recv_msg == NULL) {
            usleep(10000);
            continue;
        }
        
        printf("Received from client:");
        for (int i = 0; i < recv_cl_len; ++i) {
            printf("%02x ", recv_msg[i] & 0xFF);
        }
        putchar('\n');
        
        char* extracted = extract_domain(recv_msg, recv_cl_len);
        if (extracted != NULL) {
            printf("Received DNS query with domain: %s\n", extracted);
            free(extracted);
        }

        // Forward the DNS query to the actual DNS server
        send_to(dns_sockfd, recv_msg, recv_cl_len, &dns_addr);
        free(recv_msg);

        printf("DNS query sent to upstream.\n");

        // Receive the response
        ssize_t recv_up_len;
        char* upstream_resp = receive_from(dns_sockfd, &dns_addr, &recv_up_len);
        if (recv_up_len < 0) {
            perror("Recvfrom failed");
            exit(EXIT_FAILURE);
        } else {
            const char answer[6] = "Roger";
            send_to(sockfd, answer, sizeof(answer), &client_addr);
        }

        // Print the response
        printf("Received response from DNS server:");
        for (int i = 0; i < recv_up_len; ++i) {
            printf("%02x ", upstream_resp[i] & 0xFF);
        }
        printf("\n");

        usleep(10000);
    }

    close(sockfd);
    close(dns_sockfd);
    return 0;
}
