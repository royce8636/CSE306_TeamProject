#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
// #include <ncurses.h>
// #include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 8888
#define TARGET_IP "192.168.1.33"

void get_arrows(int client_socket)
{
	struct termios old_term, new_term;
	tcgetattr(STDIN_FILENO, &old_term); // 현재 터미널 attribute
	new_term = old_term;
	new_term.c_lflag &= ~(ICANON | ECHO);		 // 터미널 canonical mode에 둠 (바로바로 input 가져옴)
	tcsetattr(STDIN_FILENO, TCSANOW, &new_term); // 세팅 지금 적용

	int old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK); // read() is non blocking

	char buffer[3] = {0};
	int to_send = -1;

	while (1)
	{
		to_send = -1;
		if (read(STDIN_FILENO, buffer, 3) > 0)
		{
			if (buffer[0] == 27 && buffer[1] == 91)
			{
				switch (buffer[2])
				{
				case 65: // Up arrow
					printf("Up arrow pressed\n");
					to_send = 0;
					break;
				case 66: // Down arrow
					printf("Down arrow pressed\n");
					to_send = 1;
					break;
				case 67: // Right arrow
					printf("Right arrow pressed\n");
					to_send = 2;
					break;
				case 68: // Left arrow
					printf("Left arrow pressed\n");
					to_send = 3;
					break;
				default:
					break;
				}
				if (to_send >= 0)
				{
					if (send(client_socket, &to_send, sizeof(to_send), 0) < 0)
					{
						printf("Error sending data to server\n");
						break;
					}
				}
			}
			memset(buffer, 0, sizeof(buffer));
		}
		usleep(100); // sleep 0.1 millisecond
	}

	// Restore terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
	fcntl(STDIN_FILENO, F_SETFL, old_flags);
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

	server.sin_addr.s_addr = inet_addr(TARGET_IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);

	// Connect to remote server
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}

	puts("Connected\n");

	get_arrows(socket_desc);

	return 0;
}
