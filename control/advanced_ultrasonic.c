#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define Trig    4
#define Echo    5
#define BUZZER_PIN 26

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
    if(wiringPiSetup() == -1){
        printf("setup wiringPi failed !");
        exit(1);
    }
    pinMode(BUZZER_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(192);
    pwmSetRange(2000);
    printf("Program is starting ...\n");

    ultraInit();

    while(1){        
        int distance = getCM();
        if(distance < 50){
            pwmWrite(BUZZER_PIN, 50); //buzzer sounds
            delay(50); // Sound duration
            pwmWrite(BUZZER_PIN, 0); //buzzer quiet.
            delay(20 * distance); // Delay between sounds, shorter delay when closer
        }
        else{
            pwmWrite(BUZZER_PIN, 0); //buzzer quiet.
        }
    }
    return 0;
}
