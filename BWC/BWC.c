#include <stdio.h>
#include <dirent.h>

#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/status_led.h"
#include "hardware/spi.h"
#include "hardware/powman.h"

#include "accelerometer.h"
#include "storage.h"
#include "communication.h"

uint8_t __persistent_data(data)[STRIKE_SAMPLES * BPS];

// Following is a dummy variable for debug purposes
static uint32_t __persistent_data(run_count); // Un-Initialized static variables guarenteed to be 0

void deepsleep(void) {
    // Specify which domains to go to *sleep* by adding all bitflags then removing sleeping domains
    // We want SRAM0 to be awake for persistant memory so we don't remove it
    // XIP is more energy efficient, but unforunately it seems to be a hassle to move everything from SRAM0
    pstate_bitset_t pstate = pstate_bitset_all();
    pstate_bitset_remove(&pstate, POWMAN_POWER_DOMAIN_SRAM_BANK1);
    pstate_bitset_remove(&pstate, POWMAN_POWER_DOMAIN_SWITCHED_CORE);
    pstate_bitset_remove(&pstate, POWMAN_POWER_DOMAIN_XIP_CACHE);

    low_power_pstate_until_gpio_pin_state(PIN_INTR, false, true, &pstate, NULL);
}

int main() {
    stdio_init_all();

    gpio_init(PIN_INTR);
    gpio_set_dir(PIN_INTR, GPIO_IN);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_pulls(PIN_CS, true, false);
    gpio_put(PIN_CS, 1); // Must occur first before setting polarity and phase of SPI

    spi_init(SPI_PORT, 2000000); // 2Mhz baudrate, see doc pg. 13 for info on appropriate ranges

    spi_set_format(SPI_PORT, 8, 1, 1, SPI_MSB_FIRST); // sets polarity and phase to 1 as required, see doc pg. 13
    // Since pins on the board have specific SPI functions, you need not specify the role of each pin, it just works

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

    if (gpio_get(PIN_INTR)) { // Grab data if woken up by interrupt. TODO: Make this more robust
        fetch_data(data);
    }

    initialize_lfs(); // Initializes the filesystem, we do this last to not delay collecting samples

    printf(".\n"); // Dummy printf just to give us Serial Monitor access, disable when no longer needed for testing
    fflush(stdout);
    sleep_ms(5000);

    if (gpio_get(PIN_INTR)) { // Write data to file
        write_file("/test.txt", data);
    }

    read_state_on();
    sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

    intr_state_on();
    sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

    if (gpio_get(PIN_INTR)) {
        // reset_intr_state();
    }

    if (!gpio_get(PIN_INTR)) {
        // deepsleep();
    }
    // No code should ever run past the low power call when the if-statement is false

    // list_dir();

    // read_file("/test.txt");

    if (connect_to_wifi() == 0) {
        printf("Successfully connected to Wi-Fi!\n");
        send_data("test2", "bXkgbmV3IGZpbGUgY29udGVudHM=");

        // Optional: Disconnect when done to save power
        disconnect_from_wifi();
    }

    for (;;) {
        printf("End of Code!\n");
        fflush(stdout);
        sleep_ms(1000);
    }
}

/*
   hard_assert(status_led_init());
   if (run_count > 0) {
       for (int i = 0; i < run_count; i++) {
           status_led_set_state(true);
           sleep_ms(1000 / run_count);
           status_led_set_state(false);
           sleep_ms(100 / run_count);
       }
   }
   */

/*
Notes on LEDs (specifically status_led_init):
Initialize the status LED(s) and the resources they need before use. On some devices (e.g. Pico W, Pico 2 W) accessing
the status LED requires talking to the WiFi chip, which requires an async_context. This method will create an
async_context for you.
However an application should only use a single async_context instance to talk to the WiFi chip. If the application
already has an async context (e.g. created by cyw43_arch_init) you should use status_led_init_with_context instead and
pass it the async_context already created by your application
*/