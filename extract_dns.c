#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include "extract_dns.h"

#define MAX_DOMAIN_LEN 255

char* extract_domain(char* buffer, int payload_len) {

    const uint8_t* dns_payload = buffer;

    struct DNSHeader* dnsHeader = (struct DNSHeader*)dns_payload;
    uint16_t queryID = ntohs(dnsHeader->id);

    if ((ntohs(dnsHeader->flags) & 0x8000) == 0) {
        const uint8_t* question = dns_payload + sizeof(struct DNSHeader);
        char* extracted = malloc(payload_len + 1);
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
