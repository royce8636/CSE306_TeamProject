#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <jpeglib.h>

#define PORT 8080
#define BUFSIZE 65536

int main()
{
    struct sockaddr_in server_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        exit(1);
    }

    if (listen(sock, 5) < 0)
    {
        perror("Error listening on socket");
        exit(1);
    }

    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);
    int conn = accept(sock, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
    if (conn < 0)
    {
        perror("Error accepting connection");
        exit(1);
    }

    int width = 640;
    int height = 480;
    SDL_Window *window = SDL_CreateWindow("Video Stream", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_Rect rect = {0, 0, width, height};

    struct
    {
        uint8_t *data;
        uint32_t size;
        uint32_t bytesused;
    } buf;

    uint8_t inbuf[BUFSIZE];
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    while (1)
    {
        int recv_len = recv(conn, inbuf, sizeof(inbuf), 0);
        if (recv_len < 0)
        {
            printf("Error receiving frame data: %s\n", strerror(errno));
            break;
        }

        if (recv_len == 0)
        {
            printf("Connection closed by peer\n");
            break;
        }

        buf.data = inbuf;
        buf.size = recv_len;
        buf.bytesused = recv_len;

        jpeg_mem_src(&cinfo, buf.data, buf.size);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        uint8_t *pixels = malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            uint8_t *row_pointer = pixels + cinfo.output_scanline * cinfo.output_width * cinfo.output_components;
            jpeg_read_scanlines(&cinfo, &row_pointer, 1);
        }

        jpeg_finish_decompress(&cinfo);

        SDL_UpdateTexture(texture, NULL, pixels, cinfo.output_width);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);

        free(pixels);
    }

    jpeg_destroy_decompress(&cinfo);

    // Cleanup resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
