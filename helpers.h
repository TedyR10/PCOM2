#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare


struct client{
	char id[10];
	char subscriptions[100][51];
	int subscriptions_number;
	int sf[100];
	struct tcp *pending;
	int pending_number;
	int sock;
	uint8_t isconnected;
};

typedef struct packet {
	uint8_t type;
	char topic[51];
	uint8_t sf;
	char buff[1501];
	char ip[16];
	uint16_t port;
} packet;

typedef struct tcp {
	char ip[16];
	uint16_t port;
	char type[11];
	char topic[51];
	char buff[1501];
} tcp;

typedef struct udp {
	char topic[50];
	uint8_t type;
	char buff[1501];
} udp;

#endif