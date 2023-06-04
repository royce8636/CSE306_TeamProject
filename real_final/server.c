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
#include <sys/ioctl.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <pthread.h>

#define PORT 8888
#define SA struct sockaddr
#define Trig 4
#define Echo 5
#define BUZZER_PIN 26

int data[4] = {0, 0, 0, 0}; // adjust speed

void stop_exit()
{
    data[0] = 0;
    data[1] = 0;
    data[2] = 1;
    data[3] = 0;
    write_i2c_block_data(file, addr, reg, 4, data);
    printf("Goodbye!\n");
    exit(0);
}

void sigint_handler(int sig)
{
    stop_exit();
}

void ultraInit(void)
{
    pinMode(Echo, INPUT);
    pinMode(Trig, OUTPUT);
}

int getCM(void)
{
    struct timeval tv1;
    struct timeval tv2;
    long start, stop;
    float dis;

    digitalWrite(Trig, LOW);
    delayMicroseconds(2);

    digitalWrite(Trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig, LOW);

    while (!(digitalRead(Echo) == 1))
        ;
    gettimeofday(&tv1, NULL);

    while (!(digitalRead(Echo) == 0))
        ;
    gettimeofday(&tv2, NULL);

    start = tv1.tv_sec * 1000000 + tv1.tv_usec;
    stop = tv2.tv_sec * 1000000 + tv2.tv_usec;

    dis = (float)(stop - start) / 58.00;
    return (int)dis;
}

void *thread(void *vargp)
{
    while (1)
    {
        int distance = getCM();
        if (distance < 50)
        {
            pwmWrite(BUZZER_PIN, 50); // buzzer sounds
            delay(50);                // Sound duration
            pwmWrite(BUZZER_PIN, 0);  // buzzer quiet.
            delay(20 * distance);     // Delay between sounds, shorter delay when closer
            if (distance < 20)
            {
                data[0] = 0;
                data[1] = 0;
                data[2] = 1;
                data[3] = 0;
            }
        }
        else
        {
            pwmWrite(BUZZER_PIN, 0); // buzzer quiet.
        }
    }
}

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
    while (1)
    {
        write_i2c_block_data(file, addr, reg, 4, data);
        if (recv(client_socket, &num, sizeof(num), 0) <= 0) // 0이면 disconnected, -1은 잘못됨
        {
            printf("Wrong receive\n");
            stop_exit();
            break;
        }
        switch (num)
        {
        case 0:
            printf("Arrow up\n");
            data[0] = 1;
            data[1] = 80;
            data[2] = 1;
            data[3] = 80;
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 1:
            printf("Arrow down\n");
            data[0] = 0;
            data[1] = 80;
            data[2] = 0;
            data[3] = 80;
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 2:
            printf("Arrow right\n");
            data[0] = 1;
            data[1] = 30;
            data[2] = 1;
            data[3] = 80;
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 3:
            printf("Arrow left\n");
            data[0] = 1;
            data[1] = 80;
            data[2] = 1;
            data[3] = 30;
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        case 4:
            printf("Stop\n");
            data[0] = 0;
            data[1] = 0;
            data[2] = 1;
            data[3] = 0;
            write_i2c_block_data(file, addr, reg, 4, data);
            break;
        }
    }
}

// Driver function
int main()
{
    // Set up sigint handler
    signal(SIGINT, sigint_handler);
    // CAR CONNECTION
    if (wiringPiSetup() == -1)
    {
        printf("setup wiringPi failed !");
        exit(1);
    }
    pinMode(BUZZER_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(192);
    pwmSetRange(2000);
    printf("Program is starting ...\n");

    ultraInit();

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

    printf("Car connection successful");

    // IP CONNECTION
    ip_print();

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        stop_exit();
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
        stop_exit();
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        stop_exit();
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0)
    {
        printf("server accept failed...\n");
        stop_exit();
    }
    else
        printf("server accept the client...\n");

    // Function for chatting between client and server
    pthread_t tid;
    pthread_create(&tid, NULL, thread, NULL);
    func(connfd, file, addr);

    // After chatting close the socket
    close(connfd);
    close(sockfd);
}
