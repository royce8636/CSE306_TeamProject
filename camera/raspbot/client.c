#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#define WIDTH 640
#define HEIGHT 480
#define BUFFER_SIZE (WIDTH * HEIGHT * 5)
#define PORT 8888
#define IP_ADDR "172.20.10.2"
int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    // Create a window for displaying the video
    SDL_Window *window = SDL_CreateWindow("Live Video", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // Create a renderer for the window
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Create a texture for displaying the video frames
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Failed to create socket");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Connect to the server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);  // Use the same port as the server
    if (inet_pton(AF_INET, IP_ADDR, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Failed to connect to the server");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Buffer for receiving video frames
    unsigned char buffer[BUFFER_SIZE];
    // Main loop
    bool quit = false;
    int rec_n = 0;
    while (!quit) {
    	printf("Receiving %d\r", rec_n);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }
        // Receive a video frame from the server
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            perror("Failed to receive video frame");
            break;
        }
        // Update the texture with the new frame data
        SDL_UpdateTexture(texture, NULL, buffer, WIDTH * 3);
        // Clear the renderer
        SDL_RenderClear(renderer);
        // Copy the texture to the renderer
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        // Render the updated frame
        SDL_RenderPresent(renderer);
        // Delay to control frame rate (adjust as needed)
        SDL_Delay(30);
	rec_n += 1;
    }
    // Clean up resources
    close(clientSocket);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
