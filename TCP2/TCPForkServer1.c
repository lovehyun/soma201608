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

#define SOCK_ERR -1
#define BUFSIZE 100
#define NUMBER_OF_CLIENTS_TO_QUEUE 5
#define PORT 43210 //change if 43210 already in use

void abortt(const char * msg) {
	printf("\nFATAL ERROR: %s\n", msg);
	exit(1);
}

void trade_data(int data_socket) {
	char msgbuffer[400];

	// CONDUCT CONVERSATION
	int nReceived = -1;
	while (nReceived) {
		char buf[BUFSIZE];
		nReceived = recv(data_socket, buf, BUFSIZE - 2, 0);
		printf("nReceived=%d\n", nReceived);
		if (nReceived == 0)
			continue;
		buf[nReceived] = '\0';
		sprintf(msgbuffer, "fromserver#%d=>", data_socket);
		send(data_socket, msgbuffer, strlen(msgbuffer), 0);
		send(data_socket, buf, strlen(buf), 0);
	}
}

int main(int argc, char* argv[]) {
	pid_t pid;
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

	while (1) {
		printf("Looking to accept a client connection\n");
		int sockaddrlen = sizeof(struct sockaddr_in);
		data_socket = accept(listener_socket, (struct sockaddr*) &addr,
				(socklen_t *) &sockaddrlen);
		if (data_socket == SOCK_ERR) {
			abortt("Call to accept() failed");
		}
		printf("Accepted a client, data socket handle=%d.\n", data_socket);
		pid = fork();
		if (pid == 0) { //child process
			trade_data(data_socket);
			exit(0);
		} else if (pid < 0) { //fork error
			abortt("Failed to fork!");
		} else { //parent
			printf("Successfully forked process %d to handle data socket %d\n",
					pid, data_socket);
		}
	}
	return (0);
}
