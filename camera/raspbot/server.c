#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define WIDTH 640
#define HEIGHT 480
#define BUFFER_SIZE (WIDTH * HEIGHT * 3)
#define PORT 8888
int main() {
    // Create pipe for communication with raspivid
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Failed to create pipe");
        return 1;
    }

    // Fork a child process to run raspivid
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork process");
        return 1;
    }

    if (pid == 0) {
        // Child process - Run raspivid to capture video
        close(pipefd[0]);  // Close the read end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to the pipe
        dup2(pipefd[1], STDERR_FILENO);
	close(pipefd[1]);  // Close the write end of the pipe

        // Execute raspivid command
        execlp("libcamera-vid", "libcamera-vid", "-t", "0", "--height=480", "--width=640", NULL);
        perror("Failed to execute raspivid");
        exit(1);
    }

    // Parent process - Video streaming server
    close(pipefd[1]);  // Close the write end of the pipe

    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Failed to create socket");
        close(pipefd[0]);  // Close the read end of the pipe
        return 1;
    }

    // Bind socket to a port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);  // Choose a suitable port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Failed to bind socket");
        close(pipefd[0]);  // Close the read end of the pipe
        close(serverSocket);
        return 1;
    }

    // Listen for client connections
    if (listen(serverSocket, 1) == -1) {
        perror("Failed to listen for connections");
        close(pipefd[0]);  // Close the read end of the pipe
        close(serverSocket);
        return 1;
    }

    // Accept client connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == -1) {
        perror("Failed to accept client connection");
        close(pipefd[0]);  // Close the read end of the pipe
        close(serverSocket);
        return 1;
    }

    // Streaming loop
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    int sent = 0;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
       printf("sending %d\r", sent);
            // Send the video frame to the client
        ssize_t bytesSent = send(clientSocket, buffer, bytesRead, 0);
        if (bytesSent != bytesRead) {
            perror("Failed to send video frame");
            break;
        }
	sent += 1;
    }

    // Clean up resources
    close(clientSocket);
    close(serverSocket);
    close(pipefd[0]);  // Close the read end of the pipe

    // Wait for the child process (raspivid) to exit
    int status;
    waitpid(pid, &status, 0);

    return 0;
}

