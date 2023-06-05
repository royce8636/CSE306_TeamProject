#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>

#define SERVER_PORT 8888
#define NUM_BUFFERS 1

struct buffer
{
    void *start;
    size_t length;
};

int main()
{
    const char *device = "/dev/video1";
    int width = 1280;
    int height = 800;

    // Open the video device
    int fd = open(device, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open device");
        return 1;
    }

    // Set camera format and resolution
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("Failed to set format");
        close(fd);
        return 1;
    }

    // Create a network socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Failed to create socket");
        close(fd);
        return 1;
    }

    // Bind the socket to the server port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Failed to bind socket");
        close(fd);
        close(sockfd);
        return 1;
    }

    // Listen for client connections
    if (listen(sockfd, 1) == -1)
    {
        perror("Failed to listen for connections");
        close(fd);
        close(sockfd);
        return 1;
    }

    // Accept a client connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sockfd == -1)
    {
        perror("Failed to accept connection");
        close(fd);
        close(sockfd);
        return 1;
    }

    // Request and memory map video buffers
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = NUM_BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        perror("Failed to request buffers");
        close(fd);
        close(sockfd);
        close(client_sockfd);
        return 1;
    }

    struct buffer *frame_buffers = (struct buffer *)calloc(req.count, sizeof(struct buffer));

    for (int i = 0; i < req.count; ++i)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            perror("Failed to query buffer");
            free(frame_buffers);
            close(fd);
            close(sockfd);
            close(client_sockfd);
            return 1;
        }

        frame_buffers[i].length = buf.length;
        frame_buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (frame_buffers[i].start == MAP_FAILED)
        {
            perror("Failed to map buffer");
            free(frame_buffers);
            close(fd);
            close(sockfd);
            close(client_sockfd);
            return 1;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            perror("Failed to enqueue buffer");
            free(frame_buffers);
            close(fd);
            close(sockfd);
            close(client_sockfd);
            return 1;
        }
    }

    // Start capturing frames
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        perror("Failed to start capturing");
        free(frame_buffers);
        close(fd);
        close(sockfd);
        close(client_sockfd);
        return 1;
    }

    // Allocate a buffer for frame data
    void *buffer = malloc(width * height * 2);

    // Main loop: capture and send frames
    while (1)
    {
        // Dequeue a buffer for capturing
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
        {
            perror("Failed to dequeue buffer");
            break;
        }

        // Copy the frame data to the buffer
        memcpy(buffer, frame_buffers[buf.index].start, buf.bytesused);

        uint32_t frameSize = buf.bytesused;
        ssize_t bytesSent = send(client_sockfd, &frameSize, sizeof(frameSize), 0);

        // Send the frame to the client
        if (send(client_sockfd, buffer, buf.bytesused, 0) == -1)
        {
            perror("Failed to send frame");
            break;
        }

        // Requeue the buffer for the next capture
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
        {
            perror("Failed to requeue buffer");
            break;
        }
    }

    // Stop capturing frames
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
    {
        perror("Failed to stop capturing");
    }

    // Cleanup and close connections
    for (int i = 0; i < req.count; ++i)
    {
        munmap(frame_buffers[i].start, frame_buffers[i].length);
    }
    free(frame_buffers);
    close(fd);
    close(sockfd);
    close(client_sockfd);
    free(buffer);
    return 0;
}
