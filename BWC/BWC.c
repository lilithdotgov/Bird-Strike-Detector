#include <stdio.h>
#include <dirent.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "hardware/spi.h"
#include "hardware/powman.h"
// #include "pico/status_led.h"

#include "accelerometer.h"
#include "storage.h"
#include "communication.h"

#define MAX_DEBOUNCE_WAIT_MS 60000

uint8_t __persistent_data(data)[STRIKE_SAMPLES * BPS]; // Where strike data is to be stored in ram

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
    // Initialize as much as is strictly necessary to gather data
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
    /*
    bool strike_flag;
    if (gpio_get(PIN_INTR)) { // Grab data if woken up by interrupt. TODO: Make this more robust
        fetch_data(data);
        strike_flag = true;
    } else {           // Not a strike, booting up for first time, run usual set up
        printf(".\n"); // Dummy printf just to give us Serial Monitor access
        fflush(stdout);
        sleep_ms(5000); // Wait 5 seconds to let user open Serial Monitor

        strike_flag = false;

        read_state_on();
        sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

        intr_state_on();
        sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on
    }

    initialize_lfs(); // Initializes the filesystem, we do this last to not delay collecting samples

    if (strike_flag) { // Write data to file
        write_binary_file(generate_temp_bin_name(), data);
    }

    // Regardless of strike or not, check if there are any unsent strikes and attempt to send them
    char **files;
    if (files = find_in_dir(".bin")) {
        for (int i = 0; (i < CONNECT_RETRY) && !wifi_status; i++) { // Attempt multiple times to conect to WiFi
            printf("Wifi connection attempt #%d\n", i);
            connect_to_wifi();
        }

        if (wifi_status()) {
            printf("Successfully connected to Wi-Fi!\n");

            for (int i = 0; files[i] != NULL; i++) {
            }
        }

        disconnect_from_wifi();
    }


    for (int i = 1; gpio_get(PIN_INTR); i++) { // Do not go to sleep unless we have had at least a whole second without an interrupt
        reset_intr_state();
        sleep_ms( ((i * 1000) < MAX_DEBOUNCE_WAIT_MS) ? (i * 1000) : MAX_DEBOUNCE_WAIT_MS );
    }
    reset_intr_state(); // One last reset just in-case
    deepsleep();

    for (;;) { // This code should never run
        printf("End of Code! You should not see this!\n");
        sleep_ms(1000);
    }

    */

    printf(".\n"); // Dummy printf just to give us Serial Monitor access
    fflush(stdout);
    sleep_ms(5000); // Wait 5 seconds to let user open Serial Monitor

    connect_to_wifi();
    set_time();
    disconnect_from_wifi();

    printf("Time = %lld\n", get_time());
    fflush(stdout);
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