#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#define PORT 8888
#define SA struct sockaddr

// 여기서 출력된 IP address 보고 client에 TARGET_IP 수정
void ip_print()
{
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            if (strcmp(tmp->ifa_name, "lo") != 0)
            {
                printf("IP address of %s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
}

void write_i2c_block_data(int file, int addr, int reg, int size, int *data)
{
    unsigned char buffer[size + 1];
    buffer[0] = reg;
    for (int i = 0; i < size; i++)
    {
        buffer[i + 1] = (unsigned char)data[i];
    }
    if (write(file, buffer, size + 1) != size + 1)
    {
        printf("Failed to write data to I2C device.\n");
        exit(1);
    }
}

// Function designed to get int and decode them with cases.
void func(int client_socket, int file, int addr)
{
    int num;
    int reg = 0x01;
    int data[4] = {0};
    while (1)
    {
        if (recv(client_socket, &num, sizeof(num), 0) <= 0) // 0이면 disconnected, -1은 잘못됨
        {
            printf("Wrong receive\n");
            break;
        }
        switch (num)
        {
        case 0:
            printf("Arrow up\n");
            int data[4] = {1, 80, 1, 80};
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 1:
            printf("Arrow down\n");
            int data[4] = {0, 80, 0, 80};
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 2:
            printf("Arrow right\n");
            int data[4] = {1, 50, 1, 80};
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 3:
            printf("Arrow left\n");
            int data[4] = {1, 80, 1, 50};
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        }
    }
}

// Driver function
int main()
{
    // CAR CONNECTION
    int file;
    char *filename = "/dev/i2c-1";
    int addr = 0x16;

    if ((file = open(filename, O_RDWR)) < 0)
    {
        printf("Failed to open the bus.\n");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        return 1;
    }

    pritnf("Car connection successful\n");

    // IP CONNECTION
    ip_print();

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0)
    {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    // Function for chatting between client and server
    func(connfd, file, addr);

    // After chatting close the socket
    close(connfd);
    close(sockfd);
}
