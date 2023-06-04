#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <jpeglib.h>
#include <errno.h>
#include <time.h>

#define WIDTH 1280
#define HEIGHT 800
#define PORT 8888
#define IP_ADDR "172.20.10.11"

void destroy_all(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Live Video", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture)
    {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Failed to create socket");
        destroy_all(window, renderer, texture);
        SDL_Quit();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDR, &(serverAddr.sin_addr)) <= 0)
    {
        perror("Invalid server IP address");
        destroy_all(window, renderer, texture);
        SDL_Quit();
        return 1;
    }
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Failed to connect to the server");
        destroy_all(window, renderer, texture);
        SDL_Quit();
        return 1;
    }

    printf("Server connected\n");

    int frame_cnt = 0;

    bool quit = false;
    uint32_t start_time = SDL_GetTicks();
    uint32_t prev_time = start_time;
    uint32_t cur_time;

    while (!quit)
    {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
                break;
            }
        }
        uint32_t frameSize = 0;
        ssize_t bytesRead = recv(clientSocket, &frameSize, sizeof(frameSize), 0);

        unsigned char *buffer = (unsigned char *)malloc(frameSize);
        if (!buffer)
        {
            perror("Failed to allocate memory for the buffer");
        }

        ssize_t totalBytesReceived = 0;
        while (totalBytesReceived < frameSize)
        {
            bytesRead = recv(clientSocket, buffer + totalBytesReceived, frameSize - totalBytesReceived, 0);
            if (bytesRead == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                else
                {
                    perror("Failed to receive frame data");
                    free(buffer);
                    break;
                }
            }
            else if (bytesRead == 0)
            {
                free(buffer);
                break;
            }
            totalBytesReceived += bytesRead;
        }

        jpeg_mem_src(&cinfo, buffer, frameSize);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        unsigned char *rgbBuffer = (unsigned char *)malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
        if (!rgbBuffer)
        {
            perror("Failed to allocate memory for RGB buffer");
            break;
        }

        JSAMPROW row_pointer[1];
        while (cinfo.output_scanline < cinfo.output_height)
        {
            row_pointer[0] = &rgbBuffer[cinfo.output_scanline * cinfo.output_width * cinfo.output_components];
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        SDL_UpdateTexture(texture, NULL, rgbBuffer, cinfo.output_width * cinfo.output_components);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        free(rgbBuffer);
        free(buffer);

        frame_cnt++;
        cur_time = SDL_GetTicks();
        double elapsedSeconds = (double)(cur_time - prev_time) / 1000.0;
        if (elapsedSeconds >= 5.0)
        {
            double fps = frame_cnt / elapsedSeconds;
            printf("FPS: %.2f\n", fps);
            frame_cnt = 0;
            prev_time = cur_time;
        }
    }

    close(clientSocket);
    destroy_all(window, renderer, texture);
    SDL_Quit();

    return 0;
}
