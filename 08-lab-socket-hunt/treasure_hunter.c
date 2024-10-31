#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>  // Include this header for close()
//1823730042
#define USERID 1823730042
#define TREASURE_BUFFER_SIZE 1024

#include "sockhelper.h"

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s server port level seed\n", argv[0]);
        return 1;
    }

    char *server = argv[1];
    char *port_str = argv[2];
    char *level_str = argv[3];
    char *seed_str = argv[4];

    //int port = atoi(port_str);
    int level = atoi(level_str);
    int seed = atoi(seed_str);

    // Initial request message
    unsigned char message[8] = {0};
    message[0] = 0;
    message[1] = (unsigned char)level;

    unsigned int userid_nbo = htonl(USERID);
    memcpy(&message[2], &userid_nbo, sizeof(userid_nbo));

    unsigned short seed_nbo = htons((unsigned short)seed);
    memcpy(&message[6], &seed_nbo, sizeof(seed_nbo));

    // Server address setup
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int s = getaddrinfo(server, port_str, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_storage remote_addr_ss;
    struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        memcpy(remote_addr, rp->ai_addr, sizeof(struct sockaddr_storage));
        if (sendto(sock, message, sizeof(message), 0, remote_addr, rp->ai_addrlen) != -1) {
            break;  // Successfully sent
        }
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not send message\n");
        return 1;
    }

    freeaddrinfo(res);

    // Initialize treasure buffer and tracking variables
    char treasure[TREASURE_BUFFER_SIZE] = {0};
    int treasure_len = 0;

    // Loop to collect treasure chunks
    while (1) {
        unsigned char response[256];
        socklen_t addr_len = sizeof(struct sockaddr_storage);
        ssize_t response_len = recvfrom(sock, response, sizeof(response), 0, remote_addr, &addr_len);
        
        if (response_len == -1) {
            perror("recvfrom");
            close(sock);
            return 1;
        }

        unsigned char chunklen = response[0];
        if (chunklen == 0) {
            break;  // Entire treasure received
        } else if (chunklen > 127) {
            fprintf(stderr, "Error detected: %x\n", chunklen);
            close(sock);
            return 1;
        }

        // Append received chunk to treasure
        if (treasure_len + chunklen < TREASURE_BUFFER_SIZE) {
            memcpy(treasure + treasure_len, &response[1], chunklen);
            treasure_len += chunklen;
        } else {
            fprintf(stderr, "Treasure exceeds buffer size\n");
            close(sock);
            return 1;
        }

        // Extract and increment nonce for follow-up
        unsigned int nonce;
        memcpy(&nonce, &response[chunklen + 4], sizeof(nonce));
        nonce = ntohl(nonce) + 1;
        unsigned int new_nonce_network_order = htonl(nonce);

        // Build and send follow-up request with new nonce
        unsigned char follow_up_request[4];
        memcpy(follow_up_request, &new_nonce_network_order, sizeof(new_nonce_network_order));

        if (sendto(sock, follow_up_request, sizeof(follow_up_request), 0, remote_addr, addr_len) == -1) {
            perror("sendto");
            close(sock);
            return 1;
        }
    }

    close(sock);

    // Print the complete treasure
    printf("%s\n", treasure);
    return 0;
}


void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
	fflush(stdout);
}
