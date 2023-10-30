// This example uses the ultrasonic script to get centimeters and writes it to UART

#include "pico/stdlib.h"
#include <stdio.h>
#include "ultrasonic.h"
#include "hardware/uart.h"

uint trigPin = 0;
uint echoPin = 1;
int cm;

int main()
{
    stdio_init_all();
    setupUltrasonicPins(trigPin, echoPin);
    while (true)
    {
        cm = getCm(trigPin, echoPin);
        if (cm < 10){
            printf("Too near!\n");
        } else {
            printf("%d cm\n", cm);
        }
        
        sleep_ms(100);
    }
}