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
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <libgen.h>

#define PORT 8888
#define TARGET_IP "172.20.10.6"

int sockfd;
int old_flags = 0;
struct termios old_term;


// [Error Handler] CTRL C로 프로그램 끝내면 socket다 닫고 끝내기
void sigint_handler(int sig)
{
	printf("Ending client\n");
	close(sockfd);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // terminal 다시 원상복구
	fcntl(STDIN_FILENO, F_SETFL, old_flags);
	exit(0);
}

void* linux_like_control(){
	FILE* file;
	DIR *dir;
	struct dirent *entry;
	while(1){
		int to_send = -1;
		char string[100]; // Declare a character array to store the input string
    	fgets(string, sizeof(string),stdin);

		if(strncmp(string,"./left",6) == 0){
			to_send = 2;	
		}
		else if(strncmp(string,"./right",7) == 0){
			to_send = 1;
		}
		else if(strncmp(string,"./forward",9) == 0){
			to_send = 0;
		}
		else if(strncmp(string,"./back",6) == 0){
			to_send = 3;
		}
		if(strncmp(string,"./stop",6) == 0){
			to_send = 4;
		}
		if (to_send >= 0){
			if (send(sockfd, &to_send, sizeof(to_send), 0) < 0)
			{
				printf("Error sending data to server\n");						
			}
			continue;
		}

		if(strncmp(string,"ls",2) == 0){
			dir = opendir(".");
			if(dir == NULL){
				perror("Unable to open directory");
			}
			while ((entry = readdir(dir)) != NULL) {
        		printf("%s ", entry->d_name);
    		}
			closedir(dir);
			printf("\n");
			continue;
		}
		char str1[50], str2[50];

		if(sscanf(string,"%s %s", str1, str2) == 2) {
			if(strcmp(str1,"mkdir") == 0){
				int status = mkdir(str2,0777);
				if(strcmp(str2,"move") == 0){
                    chdir(str2);
                    file = fopen("left","w");
                    fclose(file);
                    file = fopen("right","w");
                    fclose(file);
                    file = fopen("foward","w");
                    fclose(file);
                    file = fopen("back","w");
                    fclose(file);
                    file = fopen("stop","w");
                    fclose(file);
					chdir("..");
                }
			}
			else if(strcmp(str1,"cd") == 0){
				if(strcmp(str2,"parent") == 0){
                    chdir("..");
                    continue;
                }
				int status = chdir(str2);
			}
		}
		else{
			printf("Invalid input format.\n");
		}
	}
}



void get_arrows(int client_socket)
{
	struct termios new_term;
	tcgetattr(STDIN_FILENO, &old_term); // 현재 터미널 attribute
	new_term = old_term;
	new_term.c_lflag &= ~(ICANON | ECHO);		 // 터미널 canonical mode에 둠 (바로바로 input 가져옴)
	tcsetattr(STDIN_FILENO, TCSANOW, &new_term); // 세팅 지금 적용

	old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
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
			else if(buffer[0]==32)
			{
				printf("Space pressed\n");
				to_send=4;
				if (to_send >= 0)
				{
					if (send(client_socket, &to_send, sizeof(to_send), 0) < 0)
					{
						printf("Error sending data to server\n");
						break;
					}
				}
			}
		}
		memset(buffer, 0, sizeof(buffer));
		
	
		usleep(100); // sleep 0.1 millisecond
	}
	// Restore terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
	fcntl(STDIN_FILENO, F_SETFL, old_flags);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sigint_handler);
	pthread_t thread_id;
	struct sockaddr_in server;
	char *message, server_reply[2000];
	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		printf("Could not create socket");

	server.sin_addr.s_addr = inet_addr(TARGET_IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);

	// Connect to remote server
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}

	puts("Connected\n");
	int status = pthread_create(&thread_id, NULL, linux_like_control, NULL);
	if (status != 0) {
        printf("Failed to create thread.\n");
        return 1;
    }
	get_arrows(sockfd);

	return 0;
}
