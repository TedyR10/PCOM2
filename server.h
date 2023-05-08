#ifndef _SERVER_H
#define _SERVER_H 1
#include <sys/socket.h>
#include "helpers.h"
#define BUFLEN 256

/**
 * @brief This function deletes a socket from pfds
 * 
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param index fd to delete
 */
void socket_delete(struct pollfd *pfds, int *fdsnum, int index);

/**
 * @brief This function adds a socket to pfds
 * 
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param socket socket to add
 */
void socket_add(struct pollfd *pfds, int *fdsnum, int socket);

/**
 * @brief This function handles inputs from STDIN
 * 
 * @param tcp tcp socket
 * @param clients_num number of clients
 * @param clients array of clients
 * @param buffer buffer
 */
int handleSTDIN(int tcp, int clients_num, struct client *clients, char buffer[BUFLEN]);

/**
 * @brief This function handles udp connections
 * 
 * @param udp udp socket
 * @param clients_num number of clients
 * @param clients array of clients
 * @param cli_len client address length
 * @param udp_addr udp address
 */
void handleUDP(int udp, int clients_num, struct client *clients, socklen_t cli_len,
				struct sockaddr_in udp_addr);

/**
 * @brief This function handles tcp connections
 * 
 * @param tcp tcp socket
 * @param cli_len client address length
 * @param cli_addr udp address
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param socket socket to add
 * @param clients_num number of clients
 * @param clients array of clients
 * @param ret return 
 */
void handleTCP(int tcp, socklen_t cli_len, struct sockaddr_in cli_addr, 
				struct pollfd *pfds, int *fdsnum, int *clients_num, 
				struct client *clients, int ret);

/**
 * @brief This function handles responses from subscribers
 * 
 * @param clients_num number of clients
 * @param clients array of clients
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param i socket number 
 */
void response_subscriber(int *clients_num, struct client *clients,
							struct pollfd *pfds, int *fdsnum, int i, int idx);

/**
 * @brief This function initializes the sockets and gets the server running
 * 
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param tcp tcp socket
 * @param port server port
 * @param serv_addr server address
 * @param udp_addr udp address
 * @param portnumber port from argv
 * @param udp udp socket
 */
void init(struct pollfd *pfds, int *fdsnum, int *tcp, int *port,
			struct sockaddr_in serv_addr, struct sockaddr_in udp_addr,
			char *portnumber, int *udp);

/**
 * @brief This function closes the sockets and frees the clients array
 * 
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param clients clients array
 * @param clients_num 
 */
void closeAll(struct pollfd *pfds, int *fdsnum, struct client *clients, 
                int clients_num);

#endif
