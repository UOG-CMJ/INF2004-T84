
/* IR Sensor: Detect line for line following and measure pulse width for barcode.
 * High value (1) means detected line/barcode, Low value (0) means no line/barcode
 */

/* Feed the GP2 into the ADC at GP26 */
#include <sys/time.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/adc.h>

// define PWM and ADC pins
#define ADC_PIN 0

static volatile long long start_time;
static volatile long long end_time;

bool state = false;

long long timeInMS(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

//Interrupt to calculate the width of the black line
void irline_callback(uint gpio, uint32_t events)
{
    // store black lines samples
    if (events == GPIO_IRQ_EDGE_RISE && state == true | events == GPIO_IRQ_EDGE_FALL && state == false)
    {
        return;
    }

    //Records the time when hitting the black line
    if (events == GPIO_IRQ_EDGE_RISE && state == false)
    {
        state = true;
        printf("Black detected!\n");
        start_time = getTimeMS();
        return;
    }
    
    //Records the time when hitting the white area after hitting the black line
    //Calculates the width of the bar code line by multiplying speed and time
    if (events == GPIO_IRQ_EDGE_FALL && state == true)
    {
        state = false;
        printf("White detected!\n");
        end_time = getTimeMS();
        long long time_taken_ms = end_time - start_time;
        printf("Time: %lldms\n", time_taken_ms);
        double width = ((double)time_taken_ms/1000) * 10;

        printf("Width: %fcm\n\n", width);
        return;
    }
}

/*
 * Initialise the ADC and GPIO pins
 */
void init_adc()
{
    stdio_init_all();
    adc_init();
    gpio_init(ADC_PIN);
}

int main()
{
    init_adc();

    //Initialise interrupt when hit black line
    gpio_set_irq_enabled_with_callback(ADC_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &irline_callback);

    while (1)
    {
        tight_loop_contents();
    }

    return 0;
}