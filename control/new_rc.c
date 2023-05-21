
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>


void write_i2c_block_data(int file, int addr, int reg, int size, int *data) {
    unsigned char buffer[size + 1];
    buffer[0] = reg;
    for (int i = 0; i < size; i++) {
        buffer[i + 1] = (unsigned char)data[i];
    }
    if (write(file, buffer, size + 1) != size + 1) {
        printf("Failed to write data to I2C device.\n");
        exit(1);
    }
}

int main() {
    int file;
    char *filename = "/dev/i2c-1";
    int addr = 0x16;

    if ((file = open(filename, O_RDWR)) < 0) {
        printf("Failed to open the bus.\n");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        return 1;
    }

    int reg = 0x01;
    int data[4] = {1, 80, 1, 80};
    write_i2c_block_data(file, addr, reg, 4, data);


    close(file);

    return 0;
}
