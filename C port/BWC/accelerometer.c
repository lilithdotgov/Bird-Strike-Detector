#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "accelerometer.h"

/*
Documentation: https:www.analog.com/media/en/technical-documentation/data-sheets/adxl343.pdf
Notes:
To read data first bit W should be 1, to write it should be 0
To read/write multiple bytes in one call second bit MB is set to 1, else 0 for a single byte
Chip Select (CS) needs to be set to 0 whenever doing read/write, and set to 1 afterwards
CS pin needs to be high voltage at start, see doc pg. 13
SCK also needs to idle at high, see doc pg. 13
*/

//Important registers:
#define REG_DEVID 0x00 //Used to test that we are reading from correct spot. Check doc pg. 23
#define DEVID 0xE5 //Should recieve this default value when reading the Device ID Register (REG_DEVID)
#define REG_POWER_CTL 0x2D //Power Control Register, used to enable/disable data output
#define REG_DATA_RATE 0x2C //Rate of output data
#define DATA_RATE 0b1101 //0b1101 is 800RATE Data Output Rate, see doc pg. 12 for more ranges
#define REG_DATAX0 0x32 //0x32 to 0x37 contain the data for each axis, each is 2 bytes. Order is x, y, and z
#define REG_INT_ENABLE 0x2E //Interrupt Enable Register
#define INT_ENABLE 0x10 //Enables Activity mode
#define REG_ACT_CTL 0x27 //Enables which axis to be monitored for the interrupt modes. Plus an unused bonus feature, see doc pg. 23
#define ACT_CTL 0b00010000 //Selects which axis to turn on for the interrupt. See doc pg. 23
#define REG_INT_MAP 0x2F //Chooses which pin(s) to use for interrupts
#define INT_MAP 0xEF //Sets activity mode interrupt to pin INT1
#define REG_THRESH_ACT 0x24 //Sets threshold for interrupt to occur. Single unsigned byte. threshold = 62.5mg * THRESH_ACT.
#define THRESH_ACT 0x08 //0x20 = 32, 32 * 62.5mg = 2g 
#define REG_FIFO_MODE 0x38 //Register controlling FIFO modes
#define FIFO_MODE 0x80 //Sets FIFO mode to stream
#define REG_INT_SOURCE 0x30 //Read-only register, shows which interrupts were activated. Reading this resets the interrupt states
#define REG_DATA_FORMAT 0x31 //Used for formatting data, see pg. 26
#define DATA_FORMAT 0b00001010 //Importantly sets the data range, has other functionalities that are unused

//RP2350 runs at 125MHz by default on SDK, so each CPU cycle is 8ns
//Need at least 150ns of wait for the longest SPI action
//Altho SPI hardware should handle that

static inline void reg_write(uint8_t reg, uint8_t data){
    uint8_t msg[2];
    msg[0] = reg; //reg will always have the top 2 bits 0, ensuring write mode with 1 byte
    msg[1] = data;

    gpio_put(PIN_CS, 0);
    busy_wait_at_least_cycles(20);

    spi_write_blocking(SPI_PORT, msg, 2);

    gpio_put(PIN_CS, 1);
    busy_wait_at_least_cycles(20);
}

static inline void reg_read(uint8_t reg, uint8_t *buffer, uint8_t nbytes){
    uint8_t msg[1];
    msg[0] = (0b1 << 7) | (((nbytes > 1) ? 1 : 0) << 6) | reg; //Ensures top bit is 1, and second is 0 or 1

    gpio_put(PIN_CS, 0);
    busy_wait_at_least_cycles(20);

    spi_write_blocking(SPI_PORT, msg, 1);
    spi_read_blocking(SPI_PORT, 0, buffer, nbytes);

    gpio_put(PIN_CS, 1);
    busy_wait_at_least_cycles(20);
}

void read_state_off(void){
    uint8_t power_setting;
    reg_read(REG_POWER_CTL, &power_setting, 1); //Obtain current Power Control setting
    power_setting = power_setting & ~(0b1 << 3); //Change Power Control Measure bit to 0 to end data collecition, see pg. 
    reg_write(REG_POWER_CTL, power_setting);
}
 
void read_state_on(void){    
    reg_write(REG_DATA_RATE, DATA_RATE); //Sets Data Output Rate
    reg_write(REG_DATA_FORMAT, DATA_FORMAT); //Sets Data Format
    
    uint8_t power_setting;
    reg_read(REG_POWER_CTL, &power_setting, 1); //Obtain current Power Control setting
    power_setting = power_setting | (0b1 << 3); //Change Power Control Measure bit to 1 to begin data collecition, see pg. 
    reg_write(REG_POWER_CTL, power_setting);
}
        
void intr_state_off(void){
    reg_write(REG_INT_ENABLE, 0); //A value of 0 disables all interrupts
}        

void intr_state_on(void){
    reg_write(REG_INT_MAP, INT_MAP); //Sets INT pin
    reg_write(REG_THRESH_ACT, THRESH_ACT); //Sets threshold
    reg_write(REG_FIFO_MODE, FIFO_MODE); //Sets FIFO mode
    reg_write(REG_ACT_CTL, ACT_CTL); //Sets axes for monitoring

    reg_write(REG_INT_ENABLE, INT_ENABLE); //Enables interrupts. Must go last, see doc pg. 18
}

void reset_intr_state(void){
    uint8_t foo; //Dummy variable for a buffer
    reg_read(REG_INT_SOURCE, &foo, 1);
}

uint8_t reading_validation(void){
    uint8_t foo;
    reg_read(REG_DEVID, &foo, 1);
    return foo;
}

void fetch_data(uint8_t *buffer){ //Optimized for speed, data needs further handling, used in main loop
    for(int i = 0; i < STRIKE_SAMPLES; i++, buffer += BPS){
        reg_read(REG_DATAX0, buffer, BPS);
    }
}