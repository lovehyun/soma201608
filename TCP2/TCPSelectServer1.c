#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

#define SOCK_ERR -1
#define BUFSIZE 100
#define NUMBER_OF_CLIENTS_TO_QUEUE 5
#define PORT 43210 //change if 43210 already in use

#define MAXDATASOCKETS 20

int dataSocketArray[MAXDATASOCKETS];

void abortt(const char * msg) {
	printf("\nFATAL ERROR: %s\n", msg);
	exit(1);
}

int respond_to_data(int data_socket) {
	char msgbuffer[400];

	// CONDUCT CONVERSATION
	int nReceived = -1;
	char buf[BUFSIZE];
	printf("About to receive on handle %d\n", data_socket);
	nReceived = recv(data_socket, buf, BUFSIZE - 2, 0);
	printf("nReceived=%d\n", nReceived);
	if (nReceived > 0) {
		buf[nReceived] = '\0';
		sprintf(msgbuffer, "fromserver#%d=>", data_socket);
		send(data_socket, msgbuffer, strlen(msgbuffer), 0);
		send(data_socket, buf, strlen(buf), 0);
		printf("Data socket %d said: %s\n", data_socket, buf);
	} else if (nReceived == 0) {
		printf("Data socket %d disconnected.\n", data_socket);
		nReceived = recv(data_socket, buf, BUFSIZE - 2, 0);
	} else {
		printf("Recv() error on socket %d\n", data_socket);
	}
	return (nReceived);
}

int calc_maxfd(int *arr, int listener_fd) {
	int i;
	int maxfd = listener_fd;
	for (i = 0; i < MAXDATASOCKETS; i++)
		if (arr[i] > maxfd)
			maxfd = arr[i];
	return (maxfd);
}

int main(int argc, char* argv[]) {
	int data_socket;
	printf("Starting server\n");

	// MAKE A SOCKET
	printf("Making a socket.\n");
	int listener_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listener_socket == SOCK_ERR) {
		abortt("Could not make a socket!");
	}
	printf("Listener socket handle=%d\n", listener_socket);

	// MAKE IT RERUN INSTANTLY
	int flag = 1;
	if (setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &flag,
			sizeof(flag)) < 0) {
		abortt("Could not set socket option SO_REUSEADDR!");
	}

	// DECLARE AND LOAD sockaddr_in STRUCT
	printf("Declaring and loading sockaddr_in struct\n");
	struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;
	printf("sockaddr_in struct declared and loaded.\n");

	// BIND TO PORT
	printf("Binding socket %d to port %d\n", listener_socket, PORT);
	if (bind(listener_socket, (struct sockaddr*) &addr,
			sizeof(addr)) == SOCK_ERR) {
		abortt("Could not bind listener socket to port");
	}
	printf("Bind of socket %d to port %d successful\n", listener_socket, PORT);

	// CONVERT SOCKET TO PASSIVE LISTENER
	if (listen(listener_socket, NUMBER_OF_CLIENTS_TO_QUEUE) == SOCK_ERR) {
		abortt("The listen() call failed");
	}
	printf("Listener port %d bound to port %d now listening.\n",
			listener_socket, PORT);

	// PREVENT CHILD PROCESSES FROM BECOMING ZOMBIES
	signal(SIGCHLD, SIG_IGN);

	// INITIALIZE FOR THE SELECT LOOP
	int maxfd = listener_socket;
	fd_set set;
	int i;
	for (i = 0; i < MAXDATASOCKETS; i++) {
		dataSocketArray[i] = 0;
	}

	// REPEATEDLY RUN SELECT
	while (1) {
		struct timeval timeout = { 5, 0 }; // 5 second timeout
		FD_ZERO(&set);
		for (i = 0; i < MAXDATASOCKETS; i++)
			if (dataSocketArray[i] != 0)
				FD_SET(dataSocketArray[i], &set);
		FD_SET(listener_socket, &set);

		// DIAGNOSTIC PRINT FOLLOWS, COMMENT IN FOR DEBUGGING
		// printf("dia about to run select(), maxfd=%d, listener_socket=%d\n", maxfd, listener_socket);

		// CHECK READY SOCKETS
		int numReady = select(maxfd + 1, &set, NULL, NULL, &timeout);
		if (numReady < 0)
			perror("select returns negative");
		printf("dia after select, numReady=%d, maxfd=%d, set=:\n", numReady,
				maxfd);
		for (i = 0; i <= maxfd; i++) {
			printf("Socket %d is %d\n", i, FD_ISSET(i, &set));
		}
		if (numReady < 0)
			abortt("Error on select()");
		if (numReady > 0) {
			// FIND NEXT 0 IN dataSocketArray
			int firstZeroDataSocket;
			for (firstZeroDataSocket = 0; firstZeroDataSocket < MAXDATASOCKETS;
					firstZeroDataSocket++)
				if (dataSocketArray[firstZeroDataSocket] == 0)
					break;
			if (firstZeroDataSocket == MAXDATASOCKETS)
				abortt("Maximum data sockets reached!"); // Change this for production code

			// CHECK LISTENER SOCKET
			if (FD_ISSET(listener_socket, &set)) {
				// DERIVE NEW DATA SOCKET
				int sockaddrlen = sizeof(struct sockaddr_in);
				data_socket = accept(listener_socket, (struct sockaddr*) &addr,
						(socklen_t *) &sockaddrlen);
				if (data_socket == SOCK_ERR) {
					abortt("Call to accept() failed");
				}
				printf("New client. Data socket handle=%d.\n", data_socket);
				if (data_socket > maxfd)
					maxfd = data_socket;
				FD_SET(data_socket, &set);
				dataSocketArray[firstZeroDataSocket] = data_socket;
			} // end of checking listener socket
			else {
				// SERVE ALL READY DATA SOCKETS
				int i, j;
				for (i = 0; i <= maxfd; i++) {
					if (i != listener_socket) {
						if (FD_ISSET(i, &set)) {
							if (!respond_to_data(i)) {
								FD_CLR(i, &set);
								for (j = 0; j < MAXDATASOCKETS; j++)
									if (dataSocketArray[j] == i)
										dataSocketArray[j] = 0;
								if (i == maxfd)
									maxfd = calc_maxfd(dataSocketArray,
											listener_socket);
							}
						}
					}
				} // end of serving all ready data sockets

				printf("Array=");
				for (i = 0; i < MAXDATASOCKETS; i++)
					printf("|%d|", dataSocketArray[i]);
				printf("\n");
			}
		} // end of sockets ready test

	} // end of select() loop

	return (0);
}
