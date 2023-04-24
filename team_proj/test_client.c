#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <ncurses.h>
#include <readline/history.h>
#include <stdlib.h>

void test(int client_socket)
{
	while (1)
	{
		// Receive user input from terminal
		char input_buffer[1024];
		printf("Enter data to send: ");
		fgets(input_buffer, 1024, stdin);

		// Send data to server
		int bytes_sent = send(client_socket, input_buffer, strlen(input_buffer), 0);
		if (bytes_sent < 0)
		{
			printf("Error sending data to server\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	char *message, server_reply[2000];

	// Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
		printf("Could not create socket");

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	// Connect to remote server
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}

	puts("Connected\n");

	test(socket_desc);

	// Receive a reply from the server
	if (recv(socket_desc, server_reply, 2000, 0) < 0)
	{
		puts("recv failed");
	}
	puts("Reply received\n");
	puts(server_reply);

	return 0;
}
