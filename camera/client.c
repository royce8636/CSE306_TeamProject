#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <jpeglib.h>
#include <linux/videodev2.h>

#define WIDTH 640
#define HEIGHT 480
#define QUALITY 90
#define PORT 8080

int main()
{
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        exit(1);
    }

    struct v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = WIDTH;
    format.fmt.pix.height = HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
    {
        perror("ioctl");
        exit(1);
    }

    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("ioctl");
        exit(1);
    }

    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
    {
        perror("ioctl");
        exit(1);
    }

    void *ptr = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) < 0)
    {
        perror("ioctl");
        exit(1);
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(PORT);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    struct jpeg_compress_struct cinfo = {0};
    struct jpeg_error_mgr jerr = {0};
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    cinfo.image_width = WIDTH;
    cinfo.image_height = HEIGHT;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, QUALITY, TRUE);

    uint8_t *outbuf = malloc(WIDTH * HEIGHT * 3);
    uint8_t *tmpbuf = malloc(WIDTH * HEIGHT * 2);
    while (1)
    {
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
        {
            perror("ioctl");
            exit(1);
        }
        memcpy(tmpbuf, ptr, WIDTH * HEIGHT * 2);
        for (int i = 0; i < WIDTH * HEIGHT; i++)
        {
            outbuf[i * 3 + 0] = tmpbuf[i * 2 + 1];
            outbuf[i * 3 + 1] = tmpbuf[i * 2 + 0];
            outbuf[i * 3 + 2] = 0;
        }
        jpeg_mem_dest(&cinfo, &outbuf, &buf.bytesused);
        jpeg_start_compress(&cinfo, TRUE);
        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] = &outbuf[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);
        if (send(sockfd, outbuf, buf.bytesused, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("ioctl");
            exit(1);
        }
    }
    close(sockfd);
    jpeg_destroy_compress(&cinfo);
    munmap(ptr, buf.length);
    close(fd);
    return 0;
}
