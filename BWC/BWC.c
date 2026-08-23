#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "hardware/spi.h"
#include "hardware/powman.h"
#include "pico/status_led.h"
#include "pico/cyw43_arch.h" //only needed for LED

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

// TODO: Have a watchdog or smthn for connection attempts. Seems like it can hang...
int main() {
    // Initialize as much as is strictly necessary to gather data
    stdio_init_all();

    gpio_init(PIN_INTR);
    gpio_set_dir(PIN_INTR, GPIO_IN);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_pulls(PIN_CS, true, false);
    gpio_put(PIN_CS, 1); // Must occur first before setting polarity and phase of SPI

    spi_init(SPI_PORT, 2000000); // 2Mhz baudrate for 3200 and 1600, else 400k for 800, see doc pg. 13 for info on appropriate ranges

    spi_set_format(SPI_PORT, 8, 1, 1, SPI_MSB_FIRST); // sets polarity and phase to 1 as required, see doc pg. 13
    // Since pins on the board have specific SPI functions, you need not specify the role of each pin, it just works

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    bool strike_flag;
    if (gpio_get(PIN_INTR) && run_count) { // Grab data if woken up by interrupt and isn't first run to ensure no floating values
        fetch_data(data);
        strike_flag = true;
    } else {
        printf(".\n"); // Dummy printf just to give us Serial Monitor access
        fflush(stdout);
        sleep_ms(5000); // Wait 5 seconds to let user open Serial Monitor
        strike_flag = false;
    }

    // If no USB host is connected (e.g., on battery power), disable USB stdout completely.
    // This makes all printf/fflush(stdout) statements into no-ops (supposedly, check docs later...)
    if (!stdio_usb_connected()) {
        // stdio_set_driver_enabled(&stdio_usb, false);
    }

    if (strike_flag == false) { // Not a strike, booting up for first time, run usual set up

        strike_flag = false;

        for (int i = 0; (i < CONNECT_RETRY) && (wifi_status() != WIFI_IS_CONNECTED); i++) { // Attempt multiple times to connect to WiFi
            printf("Wifi connection attempt #%d\n", i);
            fflush(stdout);
            connect_to_wifi();
        }

        set_time();

        printf("Time = %lld\n", get_time());
        fflush(stdout);

        set_mac(); // Must be called at the start to establish MAC address for future

        read_state_on();
        sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

        intr_state_on();
        sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on
    }

    initialize_lfs(); // Initializes the filesystem, we do this last to not delay collecting samples

    if (strike_flag) { // Write data to file
        char *name = generate_bin_name();
        write_binary_file(name, data);
        free(name);
    }

    // Regardless of strike or not, check if there are any unsent strikes and attempt to send them
    // First show the user what files are currently available
    print_dir();
    char *files[MAX_DIR_SIZE];
    if (find_in_dir(".bin", files) == 0 && files[0] != NULL) {                              // Ensure find_in_dir ran correctly and there are ".bin" files
        for (int i = 0; (i < CONNECT_RETRY) && (wifi_status() != WIFI_IS_CONNECTED); i++) { // Attempt multiple times to conect to WiFi
            printf("Wifi connection attempt #%d\n", i);
            fflush(stdout);
            connect_to_wifi();
        }

        if (wifi_status() == WIFI_IS_CONNECTED) {
            printf("Successfully connected to Wi-Fi!\n");
            int16_t *buf_data = malloc(STRIKE_SAMPLES * 3 * sizeof(int16_t)); // 3 for each axis, needs to be malloc'ed because too large for stack

            if (buf_data == NULL) {
                printf("Failed to allocate memory for buf_data, files will be locally stored and sent another time\n");

            } else {
                TLS_CLIENT_T *state;
                for (int i = 0; files[i] != NULL; i++) {  // Iterate for each ".bin" file
                    read_binary_file(files[i], buf_data); // Get the data
                    state = send_data(files[i], buf_data);

                    if (state->http_state == GITHUB_SUCCESS_CODE) { // If data sent correct delete the file
                        printf("Successfully sent file:\t%s!\n", files[i]);
                        remove(files[i]);
                    }
                    free(state);
                    free(files[i]);
                }
            }

            if (buf_data != NULL) { // Free buffer if not already freed
                free(buf_data);
            }

        } else {
            printf("Could not connect to Wi-Fi, files will be locally stored and sent another time\n");
            for (int i = 0; files[i] != NULL; i++) { // Clean up memory even on failure!
                free(files[i]);
            }
        }
        // Show which are left over
        printf("Change in files:\n");
        print_dir();
    }

    printf("Strike count = %d\n", run_count++);
    printf("Going to deepsleep after disconnecting WiFi and debouncing...\n");

    // Now it is safe to disconnect from WiFi since we will no longer use any cyw libraries
    // NOTE: YOU CANNOT RECIEVE FURTHER STDOUT CALLS FROM THE USB AFTER DISCONNECTING!!!!
    // ANY ATTEMPTS TO CALL PRINTF WILL APPEAR AS IF THEY FAILED!!!
    // Hence all our last prints will be made before this, even if a little awkward...
    disconnect_from_wifi();

    //-------------------------------------------------------
    // Copy-and-paste code for LED debugging
    // status_led_init(); // If running outside WiFi
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(1000);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(1000);
    //-------------------------------------------------------

    for (int i = 1; gpio_get(PIN_INTR); i++) { // Do not go to sleep unless we can ensure there is no further bouncing
        reset_intr_state();
        sleep_ms(((i * 1000) < MAX_DEBOUNCE_WAIT_MS) ? (i * 1000) : MAX_DEBOUNCE_WAIT_MS);
    }
    reset_intr_state(); // One last reset just in-case
    deepsleep();

    for (;;) { // This code should never run
        printf("End of Code! You should not see this!\n");
        sleep_ms(1000);
    }
}

/*
Notes on LEDs (specifically status_led_init):
Initialize the status LED(s) and the resources they need before use. On some devices (e.g. Pico W, Pico 2 W) accessing
the status LED requires talking to the WiFi chip, which requires an async_context. This method will create an
async_context for you.
However an application should only use a single async_context instance to talk to the WiFi chip. If the application
already has an async context (e.g. created by cyw43_arch_init) you should use status_led_init_with_context instead and
pass it the async_context already created by your application
*/

/*
    //To be placed after gpio setup
    read_state_on();
    sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

    intr_state_on();
    sleep_ms(10); // ADXL343 needs at least ~1.4ms to turn setting on

    initialize_lfs();

    printf(".\n"); // Dummy printf just to give us Serial Monitor access
    fflush(stdout);
    sleep_ms(3000);
    printf("3\n");
    sleep_ms(1000);
    printf("2\n");
    sleep_ms(1000);
    printf("1\n");
    sleep_ms(1000);
    printf("Go!\n");
    sleep_ms(250);

    fetch_data(data);
    char *name = "test.txt";
    write_binary_file(name, data);
    sleep_ms(250);
    print_dir();
    print_file(name);
    */
/*
//-------------------------------------------------------
// Copy-and-paste code for LED debugging
// status_led_init(); // If running outside WiFi
status_led_init_with_context(cyw43_arch_async_context()); // If running within WiFi
status_led_set_state(true);
sleep_ms(1000);
status_led_set_state(false);
sleep_ms(1000);
status_led_deinit();
sleep_ms(1000);
//-------------------------------------------------------

//-------------------------------------------------------
// Copy-and-paste code for LED debugging
// status_led_init(); // If running outside WiFi
cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
sleep_ms(1000);
cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
sleep_ms(1000);
//-------------------------------------------------------
*/