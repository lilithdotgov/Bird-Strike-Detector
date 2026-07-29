#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "accelerometer.h"

uint8_t data[STRIKE_SAMPLES * BPS];

int main(){
    stdio_init_all();

    gpio_init(PIN_INTR);
    gpio_set_dir(PIN_INTR, GPIO_IN);
    
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_pulls(PIN_CS, true, false);
    gpio_put(PIN_CS, 1); //Must occur first before setting polarity and phase of SPI
    
    spi_init(SPI_PORT, 2000000); //2Mhz baudrate, see doc pg. 13 for info on appropriate ranges 

    spi_set_format(SPI_PORT, 8, 1, 1, SPI_MSB_FIRST); //sets polarity and phase to 1 as required, see doc pg. 13
    //Since pins on the board have specific SPI functions, you need not specify the role of each pin, it just works 

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    //gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

    


    read_state_on();
    sleep_ms(100); //ADXL343 needs ~1.4ms to turn read state on, this is overkill just for demonstration
    
    fetch_data(data);
    
    for(int i = 0; i < STRIKE_SAMPLES * BPS; i += BPS){
        sleep_ms(1000);
        
        // Combine LSB (data[i]) and MSB (data[i+1]), then cast to signed 16-bit integer
        int16_t x = (int16_t)((data[i+1] << 8) | data[i]);
        int16_t y = (int16_t)((data[i+3] << 8) | data[i+2]);
        int16_t z = (int16_t)((data[i+5] << 8) | data[i+4]);
        
        printf("X: %d\t Y: %d\t Z: %d\n", x, y, z);
    }
    
    
    
    while(true){
        sleep_ms(1000);
        printf("Response: %d\n", reading_validation());
    }

}