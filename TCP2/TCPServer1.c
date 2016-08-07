#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SOCK_ERR -1
#define BUFSIZE 100
#define NUMBER_OF_CLIENTS_TO_QUEUE 5
#define PORT 43210 //change if 43210 already in use

void abortt(const char * msg) {
	printf("\nFATAL ERROR: %s\n", msg);
	exit(1);
}

int main(int argc, char* argv[]) {
	printf("Starting server\n");

	// MAKE A SOCKET
	printf("Making a socket.\n");
	int listener_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listener_socket == SOCK_ERR) {
		abortt("Could not make a socket!");
	}
	printf("Listener socket handle=%d\n", listener_socket);

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

	// ACCEPT A CLIENT CONNECTION
	printf("Looking to accept a client connection\n");
	int sockaddrlen = sizeof(struct sockaddr_in);
	int data_socket = accept(listener_socket, (struct sockaddr*) &addr,
			(socklen_t *) &sockaddrlen);
	if (data_socket == SOCK_ERR) {
		abortt("Call to accept() failed");
	}
	printf("Accepted a client, data socket handle=%d.\n", data_socket);

#if 0
	// SEND STRING TO CLIENT
	printf("Sending string to client\n");
	char * sendstring = "Please type something then press Enter...\n";
	send(data_socket, sendstring, strlen(sendstring), 0);
	printf("Sent string to client\n");

	// RECEIVE FROM CLIENT
	char buf[BUFSIZE];
	printf("Receiving string from client\n");
	int nReceived = -1;
	nReceived = recv(data_socket, buf, BUFSIZE - 2, 0);
	printf("Received %d characters from client\n", nReceived);
	buf[nReceived] = '\0';
	printf("Received string=>%s<=\n", buf);
#endif

	// CONDUCT CONVERSATION
	while (1) {
		char buf[BUFSIZE];
		int nReceived = -1;
		nReceived = recv(data_socket, buf, BUFSIZE - 2, 0);
		printf("nReceived=%d\n", nReceived);
		buf[nReceived] = '\0';
		send(data_socket, "fromserver=>", strlen("fromserver=>"), 0);
		send(data_socket, buf, strlen(buf), 0);
	}

	return (0);
}
