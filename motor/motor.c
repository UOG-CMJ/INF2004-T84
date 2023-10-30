/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Output PWM signals on pins 0 and 1


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <sys/time.h>

//Left Motor
#define ENA_PIN 10
#define IN1_PIN 11
#define IN2_PIN 12

//Right Motor
#define ENB_PIN 13
#define IN3_PIN 14
#define IN4_PIN 15

//Wheel values
#define CIRCUMFERENCE 21
#define DEFAULT_SPEED 62500
static const float WHEEL_HOLE_DIST = 21/20;



//Initialise the respective gpio pins
void init_gpio() {

    gpio_init(IN1_PIN);
    gpio_init(IN2_PIN);
    gpio_init(IN3_PIN);
    gpio_init(IN4_PIN);
    gpio_set_dir(IN1_PIN, GPIO_OUT);
    gpio_set_dir(IN2_PIN, GPIO_OUT);
    gpio_set_dir(IN3_PIN, GPIO_OUT);
    gpio_set_dir(IN4_PIN, GPIO_OUT);
}

//initialise the motor
void init_motor(uint *slice_num_1, uint *slice_num_2) {

    gpio_set_function(ENA_PIN, GPIO_FUNC_PWM);
    gpio_set_function(ENB_PIN, GPIO_FUNC_PWM);

    pwm_set_clkdiv(*slice_num_1, 100);
    pwm_set_clkdiv(*slice_num_2, 100);

    pwm_set_wrap(*slice_num_1, DEFAULT_SPEED);
    pwm_set_wrap(*slice_num_2, DEFAULT_SPEED);

    pwm_set_chan_level(*slice_num_1, PWM_CHAN_A, DEFAULT_SPEED);
    pwm_set_chan_level(*slice_num_2, PWM_CHAN_B, DEFAULT_SPEED);

    pwm_set_enabled(*slice_num_1, true);
    pwm_set_enabled(*slice_num_2, true);
}

//Move forward
void forward() {

    gpio_put(IN1_PIN, 1);
    gpio_put(IN2_PIN, 0);
    gpio_put(IN3_PIN, 1);
    gpio_put(IN4_PIN, 0);

}

//Move backward
void backwards() {
    gpio_put(IN1_PIN, 0);                                
    gpio_put(IN2_PIN, 1);
    gpio_put(IN3_PIN, 0);
    gpio_put(IN4_PIN, 1);
}

//Turn left 
void left_turn_90(uint *slice_num_1, uint *slice_num_2) {
    //Slow left motor
    pwm_set_chan_level(*slice_num_1, PWM_CHAN_A, DEFAULT_SPEED * 0.2);

    //To change
    sleep_ms(3000);

    //Reset left motor speed
    pwm_set_chan_level(*slice_num_1, PWM_CHAN_A, DEFAULT_SPEED);
}

//Turn right
void right_turn_90(uint *slice_num_1, uint *slice_num_2) {
    //Slow left motor
    pwm_set_chan_level(*slice_num_2, PWM_CHAN_B, DEFAULT_SPEED * 0.2);

    //To change
    sleep_ms(3000);

    //Reset left motor speed
    pwm_set_chan_level(*slice_num_2, PWM_CHAN_B, DEFAULT_SPEED);
}

//Initialise the variables for the speed calculations
long long a_fall = 0;
long long b_fall = 0;
long long speed_in_ms;
long long speed_in_ms_ouput = 0;
float speed;
int count = 0;
float distance = 0;


long long timeInMS(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void gpio_callback(uint gpio, uint32_t events) {
    // Records the T1 and T2 timings of each time when events is low
    a_fall = b_fall;

    b_fall = timeInMS();
    
    speed_in_ms = b_fall - a_fall;
    //count++;
    // if (count == 20){
    //     speed_in_ms_ouput = speed_in_ms;

    //     distance += count * WHEEL_HOLE_DIST;
    //     speed = (WHEEL_HOLE_DIST / (double)speed_in_ms_ouput) * 1000;
    //     printf("Current Distance: %fcm\n", distance);
    //     printf("Current Speed: %fcm/s\n", speed);

    //     count = 0;
    // }
    
    //Calculations for the speed and distance of the robot car
    speed_in_ms_ouput = speed_in_ms;
    speed = (WHEEL_HOLE_DIST / (double)speed_in_ms_ouput) * 1000;
    distance += WHEEL_HOLE_DIST;
    printf("Current Speed: %fcm/s\n", speed);
    printf("Current Distance: %fcm\n", distance);
}



int main() {
    //Initialise variables
    init_gpio();
    stdio_init_all();

    printf("Hello GPIO IRQ\n");
    gpio_set_irq_enabled_with_callback(9, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    //get the slice num
    uint slice_num_1 = pwm_gpio_to_slice_num(ENA_PIN);
    uint slice_num_2 = pwm_gpio_to_slice_num(ENB_PIN);
    init_motor(&slice_num_1, &slice_num_2);
    
    
    while(1){
        backwards();
        
        sleep_ms(3000);

        forward();

        sleep_ms(3000);

        left_turn_90(&slice_num_1, &slice_num_2);

        right_turn_90(&slice_num_1, &slice_num_2);

    }
    
    return 0;
}
