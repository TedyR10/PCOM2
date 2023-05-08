#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H 1
#include "helpers.h"

/**
 * @brief This function initializes the sockets and sends the client id to the
 * server
 * 
 * @param pfds poll function
 * @param fdsnum number of fds
 * @param tcp tcp socket
 * @param serv_addr server address
 * @param arg1 client id
 * @param arg2 server ip
 * @param arg3 server port
 */
void init(struct pollfd *pfds, int *fdsnum, int *tcp,
			struct sockaddr_in serv_addr, char *arg1, char *arg2, char *arg3);

/**
 * @brief This function handles arguments errors
 * 
 * @param file file
 */
void usage(char *file);

/**
 * @brief This function displays the contents of a tcp packet
 * 
 * @param pack tcp packet
 */
void display_msg(struct tcp *pack);

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


#endif