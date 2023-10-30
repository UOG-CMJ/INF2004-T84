#include "pico/stdlib.h"             // Include the Pico standard library.
#include "hardware/i2c.h"            // Include the I2C hardware library.
#include <stdio.h>                   // Include the standard I/O library.
#include <math.h>                   // Include the math library.

#define I2C_PORT i2c0                 // Define the I2C port to be used.

#define ACCELEROMETER_ADDRESS 0x19    // Define the I2C address for the accelerometer.
#define MAGNETOMETER_ADDRESS 0x1E     // Define the I2C address for the magnetometer.

// Define register addresses for the accelerometer.
#define CTRL_REG1_ACCELEROMETER 0x20
#define ACCELEROMETER_X_LSB 0x28
#define ACCELEROMETER_X_MSB 0x29
#define ACCELEROMETER_Y_LSB 0x2A
#define ACCELEROMETER_Y_MSB 0x2B
#define ACCELEROMETER_Z_LSB 0x2C
#define ACCELEROMETER_Z_MSB 0x2D

// Define register addresses for the magnetometer.
#define CRA_REG_MAGNETOMETER 0x00
#define MR_REG_MAGNETOMETER 0x02
#define MAGNETOMETER_X_MSB 0x03
#define MAGNETOMETER_X_LSB 0x04
#define MAGNETOMETER_Z_MSB 0x05
#define MAGNETOMETER_Z_LSB 0x06
#define MAGNETOMETER_Y_MSB 0x07
#define MAGNETOMETER_Y_LSB 0x08

#define SDA_PIN 0                    // Define the SDA pin for I2C communication.
#define SCL_PIN 1                    // Define the SCL pin for I2C communication.

void calculateCompassBearing(int16_t x, int16_t y) {
    // Calculate the compass bearing from magnetometer data.
    double heading = atan2((double)y, (double)x);
    double heading_degrees = heading * (180.0 / M_PI);

    if (heading_degrees < 0.0) {
        heading_degrees += 360.0;
    }

    printf("Compass Bearing: %.2f degrees\n", heading_degrees);
}

void calculateAcceleration(int16_t x, int16_t y, int16_t z) {
    // Calculate acceleration from accelerometer data.
    double g = 9.81;
    double acc_x = (double)x * (g / 16384.0);
    double acc_y = (double)y * (g / 16384.0);
    double acc_z = (double)z * (g / 16384.0);

    printf("Acceleration (X, Y, Z): %.2f m/s^2, %.2f m/s^2, %.2f m/s^2\n", acc_x, acc_y, acc_z);
}

void initializeI2C() {
    // Initialize I2C communication.
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    i2c_set_slave_mode(I2C_PORT, false, 0);
}

void writeI2CRegister(uint8_t device_address, uint8_t register_address, uint8_t value) {
    // Write data to an I2C register.
    uint8_t data[2] = {register_address, value};
    i2c_write_blocking(I2C_PORT, device_address, data, 2, true);
}

uint8_t readI2CRegister(uint8_t device_address, uint8_t register_address) {
    // Read data from an I2C register.
    uint8_t data;
    i2c_write_blocking(I2C_PORT, device_address, &register_address, 1, true);
    i2c_read_blocking(I2C_PORT, device_address, &data, 1, false);
    return data;
}

void initalize_acc() {
    // Configure the accelerometer.
    writeI2CRegister(ACCELEROMETER_ADDRESS, CTRL_REG1_ACCELEROMETER, 0x5F);
}

void read_acc(int16_t* x, int16_t* y, int16_t* z) {
    // Read accelerometer data from the specified registers.
    *x = (uint16_t)((readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_X_MSB) << 8) | readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_X_LSB));
    *y = (uint16_t)((readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_Y_MSB) << 8) | readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_Y_LSB));
    *z = (uint16_t)((readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_Z_MSB) << 8) | readI2CRegister(ACCELEROMETER_ADDRESS, ACCELEROMETER_Z_LSB));
}

void initalize_mag() {
    // Configure the magnetometer.
    writeI2CRegister(MAGNETOMETER_ADDRESS, MR_REG_MAGNETOMETER, CRA_REG_MAGNETOMETER);
}

void read_mag(int16_t* x, int16_t* y, int16_t* z) {
    // Read magnetometer data from the specified registers.
    *x = (uint16_t)((readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_X_MSB) << 8) | readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_X_LSB));
    *y = (uint16_t)((readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_Y_MSB) << 8) | readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_Y_LSB));
    *z = (uint16_t)((readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_Z_MSB) << 8) | readI2CRegister(MAGNETOMETER_ADDRESS, MAGNETOMETER_Z_LSB));
}

int main() {
    stdio_init_all();             // Initialize standard I/O.
    initializeI2C();              // Initialize I2C communication.
    initalize_acc();              // Configure the accelerometer.
    initalize_mag();              // Configure the magnetometer.

    while (1) {
        int16_t acc_x, acc_y, acc_z;
        int16_t mag_x, mag_y, mag_z;

        read_acc(&acc_x, &acc_y, &acc_z);
        read_mag(&mag_x, &mag_y, &mag_z);

        // Calculate and print compass bearing.
        calculateCompassBearing(mag_x, mag_y);

        // Uncomment the following lines to print accelerometer and magnetometer data.
        //calculateAcceleration(acc_x, acc_y, acc_z);
        //printf("Accelerometer Data: (X = %d, Y = %d, Z = %d)\n", acc_x, acc_y, acc_z);
        //printf("Magnetometer Data: (X = %d, Y = %d, Z = %d)\n", mag_x, mag_y, mag_z);

        sleep_ms(500);              // Sleep for 500 milliseconds.
    }

    return 0;                       // Return 0 to indicate successful program execution.
}
