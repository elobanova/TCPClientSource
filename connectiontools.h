/*
 * tools.h
 *
 *  Created on: Apr 5, 2015
 *      Author: ekaterina
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define GETSOCKOPT_ERROR -3
#define TIMEOUT_RESULT -2
#define ERROR_RESULT -1
#define CONNECTION_CLOSED_ERROR 0
#define TIMEOUT_VALUE_SEC 3
#define TCP_FAST_OPEN false

/**
 * A method for establishing the socket connection and sending a buffer content to the host
 *
 * parameters argv[] should contain the host address and the port number of a server,
 * buffer delivers the data which is supposed to be sent of a size buffer_size
 *
 * returns a socket descriptor socketfd
 */
int setupSocketAndConnect(char *argv[], char *buffer, int buffer_size) {
	//connecion data
	int addrinfo_status;
	int socketopt;
	int connection_status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct timeval timeout;
	int socketfd;
	int bytes_sent;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Expecting a hostname as argv[1] and a port as argv[2], e.g. "laboratory.comsys.rwth-aachen.de" 2345
	addrinfo_status = getaddrinfo(argv[1], argv[2], &hints, &servinfo);
	if (addrinfo_status != 0) {
		fprintf(stderr, "Error occured while calling getaddrinfo: %s.\n", gai_strerror(addrinfo_status));
		return -1;
	}

	socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (socketfd == -1) {
		fprintf(stderr, "Socket not created.\n");
		return -1;
	}

	timeout.tv_sec = TIMEOUT_VALUE_SEC;
	timeout.tv_usec = 0;
	socketopt = setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

	if (socketopt != 0) {
		fprintf(stderr, "Warning, socket option was not set.\n");
	}

	// connect!
	if (TCP_FAST_OPEN) {
		bytes_sent = sendto(socketfd, buffer, buffer_size, MSG_FASTOPEN, servinfo->ai_addr, servinfo->ai_addrlen);
	} else {
		connection_status = connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);
		if (connection_status == -1) {
			close(socketfd);
			fprintf(stderr, "Error on connection.\n");
			return -1;
		}
		bytes_sent = send(socketfd, buffer, buffer_size, 0);
	}
	if (bytes_sent == -1) {
		close(socketfd);
		fprintf(stderr, "No bytes have been sent.\n");
		return -1;
	}

	freeaddrinfo(servinfo); // all done with this structure
	return socketfd;
}

/**
 * A method receiving data from the server with the timeout value extracted from the socket
 *
 * parameters socketfd is a socket descriptor
 * buf is a buffer to place the information of size len sent by the server
 *
 * returns the number of bytes received from the host
 */
int recvtimeout(int socketfd, char *buf, int len) { //TODO: delete
	fd_set fds;
	int select_result;
	int getsockopt_result;
	struct timeval tv;

	// set up the file descriptor set
	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	//retrieve timeout from options
	socklen_t socket_option_timeout_len = sizeof(struct timeval);
	getsockopt_result = getsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, &socket_option_timeout_len);

	if (getsockopt_result != 0) {
		return GETSOCKOPT_ERROR;
	}

	// wait until timeout or data received
	select_result = select(socketfd + 1, &fds, NULL, NULL, &tv);
	if (select_result == 0) {
		return TIMEOUT_RESULT; // timeout
	}
	if (select_result == -1) {
		return ERROR_RESULT; // error
	}

	// data must be here, so do a normal recv()
	return recv(socketfd, buf, len, 0);
}

#endif /* TOOLS_H_ */
