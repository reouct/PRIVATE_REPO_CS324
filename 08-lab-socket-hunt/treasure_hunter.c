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

#include "sockhelper.h"

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
        if (argc != 5) {
        fprintf(stderr, "Usage: %s server port level seed\n", argv[0]);
        return 1;
    }

    // Command-Line Arguments
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
    // Check point 1

    // Initial Request
    unsigned char message[8] = {0};
    message[0] = 0;
    message[1] = (unsigned char)level;

    unsigned int userid_nbo = htonl(USERID);
    memcpy(&message[2], &userid_nbo, sizeof(userid_nbo));

    unsigned short seed_nbo = htons((unsigned short)seed);
    memcpy(&message[6], &seed_nbo, sizeof(seed_nbo));

    // Print the message buffer
    print_bytes(message, 8);
	// Check point 2 

	// Convert port to string
    char port_str_conv[6];
    sprintf(port_str_conv, "%d", port);

    // Get server address information
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP

    int s = getaddrinfo(server, port_str_conv, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return 1;
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    // Send message to server
    struct sockaddr_storage remote_addr_ss;
    struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        memcpy(remote_addr, rp->ai_addr, sizeof(struct sockaddr_storage));
        if (sendto(sock, message, sizeof(message), 0, remote_addr, rp->ai_addrlen) == -1) {
            perror("sendto");
            continue;
        }
        break;  // Successfully sent
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not send message\n");
        return 1;
    }

    // Receive response from server
    unsigned char response[256];
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    ssize_t response_len = recvfrom(sock, response, sizeof(response), 0, remote_addr, &addr_len);
    if (response_len == -1) {
        perror("recvfrom");
        return 1;
    }

    // Print the size of the response and its contents
    printf("Received %zd bytes from server\n", response_len);
    print_bytes(response, response_len);

    freeaddrinfo(res);
    close(sock);
	// Checkpoint 3

    // Extract chunk length
    unsigned char chunklen = response[0];

    if (chunklen == 0) {
        printf("Hunt is over.\n");
        return 0;
    } else if (chunklen > 127) {
        printf("Error detected: %x\n", chunklen);
        return 1;
    }

    // Extract treasure chunk
    char chunk[128];
    memcpy(chunk, &response[1], chunklen);
    chunk[chunklen] = '\0';  // Null-terminate the chunk

    // Extract op-code
    unsigned char opcode = response[chunklen + 1];

    // Extract op-param
    unsigned short opparam;
    memcpy(&opparam, &response[chunklen + 2], sizeof(opparam));
    opparam = ntohs(opparam);  // Convert from network byte order to host byte order

    // Extract nonce
    unsigned int nonce;
    memcpy(&nonce, &response[chunklen + 4], sizeof(nonce));
    nonce = ntohl(nonce);  // Convert from network byte order to host byte order

    // Print extracted values
    printf("%x\n", chunklen);
    printf("%s\n", chunk);
    printf("%x\n", opcode);
    printf("%x\n", opparam);
    printf("%x\n", nonce);

	// Increment nonce
    unsigned int new_nonce = nonce + 1;

    // Convert new nonce to network byte order
    unsigned int new_nonce_network_order = htonl(new_nonce);

    // Build follow-up request
    unsigned char follow_up_request[4];
    memcpy(follow_up_request, &new_nonce_network_order, sizeof(new_nonce_network_order));

    // Print follow-up request
    printf("Follow-up request:\n");
    print_bytes(follow_up_request, sizeof(follow_up_request));

    // Send follow-up request
    ssize_t sent_len = sendto(sock, follow_up_request, sizeof(follow_up_request), 0, (struct sockaddr *)&remote_addr, addr_len);
    if (sent_len == -1) {
        perror("sendto");
        return 1;
    }

    // Receive next directions response
    response_len = recvfrom(sock, response, sizeof(response), 0, (struct sockaddr *)&remote_addr, &addr_len);
    if (response_len == -1) {
        perror("recvfrom");
        return 1;
    }

    // Print the size of the response and its contents
    printf("Received %zd bytes from server\n", response_len);
    print_bytes(response, response_len);

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
