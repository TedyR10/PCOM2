#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include "subscriber.h"

#include "helpers.h"

void socket_add(struct pollfd *pfds, int *fdsnum, int socket) {
	pfds[*fdsnum].fd = socket;
	pfds[*fdsnum].events = POLLIN;
	++(*fdsnum);
}

void socket_delete(struct pollfd *pfds, int *fdsnum, int index) {
	memmove(&pfds[index + 1], &pfds[index], (*fdsnum - index) * sizeof(struct pollfd));
	--(*fdsnum);
}

void display_msg(struct tcp *pack) {
	printf("%s:%u - %s - %s - %s\n", pack->ip, pack->port,
				pack->topic, pack->type, pack->buff);
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

void init(struct pollfd *pfds, int *fdsnum, int *tcp,
			struct sockaddr_in serv_addr, char *arg1, char *arg2, char *arg3) {
	
	// Initialize tcp socket
	*tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp < 0, "tcp socket");


	// Initialize server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(arg3));
	int ret = inet_aton(arg2, &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	// disable nagle
	int nagle = 1;
	ret = setsockopt(*tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle, 4);
	DIE(ret < 0, "nagle");

	// Connect to the server
	ret = connect(*tcp, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// Send client id
	int n = send(*tcp, arg1, 10, 0);
	DIE(n < 0, "send");

	// Add the sockets
	socket_add(pfds, fdsnum, STDIN_FILENO);
	socket_add(pfds, fdsnum, *tcp);
}
int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int tcp;
    struct sockaddr_in serv_addr;

    struct pollfd pfds[100];
    int fdsnum = 0;

    if (argc < 4) {
        usage(argv[0]);
    }

    // Initialize everything
    init(pfds, &fdsnum, &tcp, serv_addr, argv[1], argv[2], argv[3]);

    while (1) {
        int ret = poll(pfds, fdsnum, -1);
        DIE(ret < 0, "poll");

        // Check if input is available from stdin
        if (pfds[0].revents & POLLIN) {
            struct packet pack;
            char buffer[100];
            memset(buffer, 0, 100);
            fgets(buffer, 100, stdin);
            memset(&pack, 0, sizeof(struct packet));
            char *token = strtok(buffer, " ");

            // Check if the input command is "exit"
            if (strncmp(buffer, "exit", 4) == 0) {
                pack.type = 2;
                int ret = send(tcp, &pack, sizeof(struct packet), 0);
                DIE(ret < 0, "send");
                break;  // Exit the while loop
            }

            // Check if the input command is "subscribe"
            else if (strcmp(token, "subscribe") == 0) {
                pack.type = 0;
                token = strtok(NULL, " ");
                strncpy(pack.topic, token, sizeof(pack.topic) - 1);
    			pack.topic[sizeof(pack.topic) - 1] = '\0';
                token = strtok(NULL, " ");
                pack.sf = token[0] - '0';
                int ret = send(tcp, &pack, sizeof(struct packet), 0);
                DIE(ret < 0, "send");
                printf("Subscribed to topic.\n");
            }

            // Check if the input command is "unsubscribe"
            else if (strcmp(token, "unsubscribe") == 0) {
                pack.type = 1;
                token = strtok(NULL, " ");
                strncpy(pack.topic, token, sizeof(pack.topic) - 1);
    			pack.topic[sizeof(pack.topic) - 1] = '\0';
                int ret = send(tcp, &pack, sizeof(struct packet), 0);
                DIE(ret < 0, "send");
                printf("Unsubscribed to topic.\n");
            }
        }

        // Check if data is available to receive from the TCP socket
        else if (pfds[1].revents & POLLIN) {
            struct tcp pack;
			memset(&pack, 0, sizeof(struct tcp));

			int ret = recv(tcp, &pack, sizeof(struct tcp), 0);
			DIE(ret < 0, "receive");

			if (ret == 0) {
				break;
			}

			display_msg(&pack);
        }
    }

    close(tcp);

    return 0;
}
