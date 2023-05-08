#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include "server.h"

#include "helpers.h"

void socket_add(struct pollfd *pfds, int *fdsnum, int socket) {
	pfds[*fdsnum].fd = socket;
	pfds[*fdsnum].events = POLLIN;
	++(*fdsnum);
}

void socket_delete(struct pollfd *pfds, int *fdsnum, int index) {
	memmove(&pfds[index], &pfds[index + 1], (*fdsnum - index) * sizeof(struct pollfd));
	--(*fdsnum);
}

int handleSTDIN(int tcp, int clients_num, struct client *clients, char buffer[BUFLEN]) {
	memset(buffer, 0, sizeof(&buffer));
	int n = read(STDIN_FILENO, buffer, sizeof(&buffer));
	DIE(n < 0, "read");

	// Check for exit command
	if (strncmp(buffer, "exit", 4) == 0) {
		return 0;
	}
	return 1;
}

void handleUDP(int udp, int clients_num, struct client *clients, socklen_t cli_len,
				struct sockaddr_in udp_addr) {
	// Receive packet
	char buff[1551];
	memset(buff, 0, sizeof(buff));
	int ret = recvfrom(udp, buff, sizeof(buff), 0,
					(struct sockaddr *)&(udp_addr), &cli_len);
	DIE(ret < 0, "recvfrom");
	
	// Initialize tcp packet to send to subscribers of the topic
	struct tcp tcp_packet;
	memset(&tcp_packet, 0, sizeof(struct tcp));
	strcpy(tcp_packet.ip, inet_ntoa(udp_addr.sin_addr));
	tcp_packet.port = htons(udp_addr.sin_port);

	struct udp *udp_packet = (struct udp *)buff;;

	strcpy(tcp_packet.topic, udp_packet->topic);
	tcp_packet.topic[50] = 0;

	if(udp_packet->type == 0) {
		int32_t value = ntohl(*(uint32_t *)(udp_packet->buff + 1));
		int sign = udp_packet->buff[0];
		int32_t num = (sign == 1) ? -value : value;
		strcpy(tcp_packet.type, "INT");
		snprintf(tcp_packet.buff, sizeof(tcp_packet.buff), "%d", num);

	} else if (udp_packet->type == 1) {
		uint16_t value = ntohs(*(uint16_t *)(udp_packet->buff));
		double num1 = value / 100.0;
		strcpy(tcp_packet.type, "SHORT_REAL");
		sprintf(tcp_packet.buff, "%.2f", num1);
	} else if (udp_packet->type == 2) {
		int exponent = udp_packet->buff[5];
		double num1 = ntohl(*(uint32_t *)(udp_packet->buff + 1));
		num1 = num1 / pow(10, exponent);
		strcpy(tcp_packet.type, "FLOAT");
		if (udp_packet->buff[0] == 1)
			num1 = -num1;
		sprintf(tcp_packet.buff, "%lf", num1);

	} else {
		strcpy(tcp_packet.type, "STRING");
		strcpy(tcp_packet.buff, udp_packet->buff);
	}

	// Send the updates to subscribers or store the message
	for (int j = 0; j < clients_num; j++) {
		for (int k = 0; k < clients[j].subscriptions_number; k++) {
			int isSubscribedToTopic = (strcmp(clients[j].subscriptions[k], tcp_packet.topic) == 0);

			if (isSubscribedToTopic && clients[j].isconnected == 1) {
				// Send the packet to connected subscribers
				int ret = send(clients[j].sock, &tcp_packet, sizeof(struct tcp), 0);
				DIE(ret < 0, "send");
			} else if (isSubscribedToTopic && clients[j].isconnected == 0 && clients[j].sf[k] == 1) {
				// Store the packet for disconnected subscribers
				clients[j].pending[clients[j].pending_number++] = tcp_packet;
			}
		}
	}
}

void handleTCP(int tcp, socklen_t cli_len, struct sockaddr_in cli_addr, 
				struct pollfd *pfds, int *fdsnum, int *clients_num, 
				struct client *clients, int ret) {
	
	// Accept the connection from subscriber
	cli_len = sizeof(struct sockaddr);
	int newtcp = accept(tcp, (struct sockaddr *)&cli_addr, &cli_len);
	DIE(newtcp < 0, "accept");

	// Receive the client id
	char client_id[10];
	memset(client_id, 0, 10);
	int n = recv(newtcp, client_id, 10, 0);
	DIE(n < 0, "recv");

	// Check if he has previously logged in
	int found_client = -1;
	for (int k = 0; k < *clients_num; k++) {
		if (strcmp(clients[k].id, client_id) == 0) {
			found_client = k;
			break;
		}
	}

	// Add him in our array and add his socket to our fds
	if (found_client == -1) {
		socket_add(pfds, fdsnum, newtcp);

		strcpy(clients[*clients_num].id, client_id);
		clients[*clients_num].isconnected = 1;
		clients[*clients_num].subscriptions_number = 0;
		clients[*clients_num].sock = newtcp;
		clients[*clients_num].pending = malloc(1000 * sizeof(struct tcp));
		clients[*clients_num].pending_number = 0;
		(*clients_num)++;
		printf("New client %s connected from %s:%d\n", client_id,
				inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	} else {
		// If he's already connected, close the new connection
		if (clients[found_client].isconnected == 1) {
			close(newtcp);
			printf("Client %s already connected.\n", client_id);
		} else {
			// Add the new connection
			socket_add(pfds, fdsnum, newtcp);
			clients[found_client].sock = newtcp;
			clients[found_client].isconnected = 1;

			// Send pending packets if available
			for (int k = 0; k < clients[found_client].pending_number; k++) {
				struct tcp pendingPacket = clients[found_client].pending[k];
				int ret = send(clients[found_client].sock, &pendingPacket, sizeof(struct tcp), 0);
				DIE(ret < 0, "send");
			}
			clients[found_client].pending_number = 0;

			printf("New client %s connected from %s:%d\n", client_id,
					inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		}
	}
}

void response_subscriber(int *clients_num, struct client *clients,
							struct pollfd *pfds, int *fdsnum, int i, int idx) {

	// Receive message from subscriber
	char buff[sizeof(struct packet)];
	memset(buff, 0, sizeof(struct packet));
	int n = recv(i, buff, sizeof(struct packet), 0);
	DIE(n < 0, "recv");

	struct packet *msg = (struct packet *) buff;

	// Check the socket
	int index = -1;
	for (int k = 0; k < *clients_num; k++) {
		if (clients[k].sock == i) {
			index = k;
			break;
		}
	}
	if (n == 0) {
		// The connection is no longer available
		printf("Client %s disconnected.\n", clients[index].id);
		socket_delete(pfds, fdsnum, i);
		close(i);
		clients[index].isconnected = 0;
		clients[index].sock = -1;
	} else {
		if (index >= 0) {
			// Check if he wants to subscribe, unsubscribe or exit
			if (msg->type == 0) {
				int subscribed = -1;

				for(int k = 0; k < clients[index].subscriptions_number; k++) {
					if (strcmp(clients[index].subscriptions[k], msg->topic) == 0) {
						subscribed = k;
						// Already subscribed
						break;
					}
				}

				if (subscribed == -1) {
					clients[index].sf[clients[index].subscriptions_number] = msg->sf;
					strcpy(clients[index]
								.subscriptions[clients[index].subscriptions_number],
							msg->topic);
					clients[index].subscriptions_number++;
				}
			}
			if (msg->type == 1) {

				int subscribed = -1;

				for(int k = 0; k < clients[index].subscriptions_number; k++) {
					if (strcmp(clients[index].subscriptions[k], msg->topic) == 0) {
						subscribed = k;
						break;
					}
				}

				if (subscribed > -1) {
					while (subscribed < clients[index].subscriptions_number - 1) {
						clients[index].sf[subscribed] = clients[index].sf[subscribed + 1];
						strcpy(clients[index].subscriptions[subscribed],
							clients[index].subscriptions[subscribed + 1]);
						subscribed++;
					}
					clients[index].subscriptions_number--;
				}
			}
			else if (msg->type == 2) {
				for (int j = 0; j < *clients_num; j++) {
					if(clients[j].sock == i) {
						clients[j].sock = -1;
						clients[j].isconnected = 0;
						socket_delete(pfds, fdsnum, idx);
						printf("Client %s disconnected.\n", clients[j].id);
						close(i);
						break;
					}
				}
			}
		}
	}
}

void init(struct pollfd *pfds, int *fdsnum, int *tcp, int *port,
			struct sockaddr_in serv_addr, struct sockaddr_in udp_addr,
			char *portnumber, int *udp) {
	
	// Tcp initialization
	*tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(*tcp < 0, "socket");

	// Udp initialization
	*udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(*udp < 0, "udp socket");

	*port = atoi(portnumber);
	DIE(*port == 0, "atoi");

	// Setup the server
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(*port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind the sockets
	int ret = bind(*tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(*port);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(*udp, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	// disable nagle
	int nagle = 1;
	ret = setsockopt(*tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle, sizeof(int));
	DIE(ret < 0, "nagle");

	// Listen on the port
	int listen_tcp = listen(*tcp, MAX_CLIENTS);
	DIE(listen_tcp < 0, "listen");

	// Add the sockets
	socket_add(pfds, fdsnum, STDIN_FILENO);
	socket_add(pfds, fdsnum, *udp);
	socket_add(pfds, fdsnum, *tcp);
}

void closeAll(struct pollfd *pfds, int *fdsnum, struct client *clients, 
				int clients_num) {
	// Close all sockets
	for (int i = 0; i < *fdsnum; i++) {
		close(pfds[i].fd);
	}

	// Free everything
	for (int i = 0; i < clients_num; i++) {
		free(clients[i].pending);
	}
	free(clients);
}

void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int tcp, udp, port;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, udp_addr;
	struct client *clients = malloc(1000 * sizeof(struct client));
	int clients_num = 0;
	socklen_t cli_len = 0;

	struct pollfd pfds[100];
	int fdsnum = 0;

	if (argc < 2) {
		usage(argv[0]);
	}

	init(pfds, &fdsnum, &tcp, &port, serv_addr, udp_addr, argv[1], &udp);

	while (1) {
		int ret = poll(pfds, fdsnum, -1);
		DIE(ret < 0, "poll");

		if (pfds[0].revents & POLLIN) {
			if (handleSTDIN(tcp, clients_num, clients, buffer) == 0)
				break;
		}

		for (int i = 3; i < fdsnum; i++) {
			if (pfds[i].revents & POLLIN)
				response_subscriber(&clients_num, clients, pfds, &fdsnum, pfds[i].fd, i);
		}

		if (pfds[1].revents & POLLIN) {
			handleUDP(udp, clients_num, clients, cli_len, udp_addr);
		}

		if (pfds[2].revents & POLLIN) {
			handleTCP(tcp, cli_len, cli_addr, pfds, &fdsnum, &clients_num, clients,
					ret);
		}

		
	}
	
	closeAll(pfds, &fdsnum, clients, clients_num);

	return 0;
}