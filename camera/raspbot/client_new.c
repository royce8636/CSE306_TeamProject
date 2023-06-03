#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>
#include <libswscale/swscale.h>

#define BUFFER_SIZE 12288
#define SERVER_PORT 5000
#define SERVER_IP "172.20.10.5"
#define WIDTH 640
#define HEIGHT 480

int main()
{
    // Create a network socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Failed to create socket");
        return 1;
    }

    // Connect to the server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Failed to connect to server");
        close(sockfd);
        return 1;
    }

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        close(sockfd);
        return 1;
    }

    // Create a window to display the frames
    SDL_Window *window = SDL_CreateWindow("Camera Stream", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        close(sockfd);
        return 1;
    }

    // Create a renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(sockfd);
        return 1;
    }

    // Create a texture to hold the frames
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (texture == NULL)
    {
        printf("Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(sockfd);
        return 1;
    }

    struct SwsContext *swsContext = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_YUYV422, WIDTH, HEIGHT, AV_PIX_FMT_RGB24, 0, NULL, NULL, NULL);
    if (swsContext == NULL)
    {
        printf("Failed to create SwsContext\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(sockfd);
        return 1;
    }

    // Main loop: receive and display frames
    while (1)
    {
        // Receive frame size
        // uint32_t frameSize;
        // if (recv(sockfd, &frameSize, sizeof(frameSize), 0) == -1)
        // {
        //     perror("Failed to receive frame size");
        //     break;
        // }

        uint32_t frameSize = WIDTH * HEIGHT * 2;
        // Allocate buffer for frame data
        uint8_t *buffer = (uint8_t *)malloc(frameSize);
        if (buffer == NULL)
        {
            printf("Failed to allocate buffer\n");
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            close(sockfd);
            return 1;
        }

        // Receive frame data
        ssize_t received = recv(sockfd, buffer, frameSize, 0);
        if (received == -1)
        {
            perror("Failed to receive frame");
            free(buffer);
            break;
        }
        else if (received == 0)
        {
            printf("Connection closed by server\n");
            free(buffer);
            break;
        }

        // Allocate buffer for RGB frame data
        int rgbFrameSize = WIDTH * HEIGHT * 3;
        uint8_t *rgbBuffer = (uint8_t *)malloc(rgbFrameSize);
        if (rgbBuffer == NULL)
        {
            printf("Failed to allocate RGB buffer\n");
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            close(sockfd);
            return 1;
        }
        uint8_t *inputData[1] = {buffer};
        int inputLinesize[1] = {WIDTH * 2};
        sws_scale(swsContext, inputData, inputLinesize, 0, HEIGHT, &rgbBuffer, &rgbFrameSize);

        SDL_Rect dstRect;
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = WIDTH;
        dstRect.h = HEIGHT;

        // Update the texture with the received frame
        SDL_UpdateTexture(texture, NULL, rgbBuffer, WIDTH * 3);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &dstRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);

        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                goto exit;
            }
        }

        free(buffer);
    }

exit:
    // Cleanup and close connections
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    close(sockfd);
    return 0;
}
