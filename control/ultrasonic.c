#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define Trig    4
#define Echo    5

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

    while(!(digitalRead(Echo) == 1));
    gettimeofday(&tv1, NULL);

    while(!(digitalRead(Echo) == 0));
    gettimeofday(&tv2, NULL);

    start = tv1.tv_sec * 1000000 + tv1.tv_usec;
    stop  = tv2.tv_sec * 1000000 + tv2.tv_usec;

    dis = (float)(stop - start) / 58.00;
    return (int)dis;
}

int main(void)
{
    printf("Program is starting ...\n");

    if(wiringPiSetup() == -1){
        printf("setup wiringPi failed !");
        exit(1);
    }

    ultraInit();

    while(1){
        printf("The distance is : %d cm\n", getCM());
        delay(100);
    }

    return 0;
}
