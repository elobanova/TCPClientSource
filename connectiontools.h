/*
 * tools.h
 *
 *  Created on: Apr 5, 2015
 *      Author: ekaterina
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

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
 * returns a socket descriptor socketfd or ERROR_RESULT when an error occurs
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
		return ERROR_RESULT;
	}

	socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (socketfd == -1) {
		fprintf(stderr, "Socket not created.\n");
		return ERROR_RESULT;
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
			return ERROR_RESULT;
		}
		bytes_sent = send(socketfd, buffer, buffer_size, 0);
	}
	if (bytes_sent == -1) {
		close(socketfd);
		fprintf(stderr, "No bytes have been sent.\n");
		return ERROR_RESULT;
	}

	freeaddrinfo(servinfo); // all done with this structure
	return socketfd;
}

#endif /* TOOLS_H_ */
