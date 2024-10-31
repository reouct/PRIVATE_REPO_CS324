// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823730042

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

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

    int port = atoi(port_str);
    int level = atoi(level_str);
    int seed = atoi(seed_str);

    printf("Server: %s\n", server);
    printf("Port: %d\n", port);
    printf("Level: %d\n", level);
    printf("Seed: %d\n", seed);

	// Initial request
    unsigned char message[8];
    message[0] = 0;
    message[1] = (unsigned char)level;

    unsigned int userid_n = htonl(USERID);
    memcpy(&message[2], &userid_n, sizeof(userid_n));

    unsigned short seed_n = htons((unsigned short)seed);
    memcpy(&message[6], &seed_n, sizeof(seed_n));

    print_bytes(message, 8);

	// Check point 2 
	struct addrinfo hints, *res, *rp;
    int sockfd;
    char port_str_conv[6];

	sprintf(port_str_conv, "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server, port_str_conv, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        struct sockaddr_storage remote_addr_ss;
        struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
        memcpy(remote_addr, rp->ai_addr, sizeof(struct sockaddr_storage));

        if (sendto(sockfd, message, sizeof(message), 0, remote_addr, rp->ai_addrlen) == -1) {
            perror("sendto");
            close(sockfd);
            continue;
        }

        unsigned char response[256];
        socklen_t addr_len = sizeof(struct sockaddr_storage);
        int response_len = recvfrom(sockfd, response, sizeof(response), 0, remote_addr, &addr_len);
        if (response_len == -1) {
            perror("recvfrom");
            close(sockfd);
            continue;
        }

        printf("Received %d bytes from server\n", response_len);
        print_bytes(response, response_len);

        close(sockfd);
        break;
    }

	

    freeaddrinfo(res);

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
