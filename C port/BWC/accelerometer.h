//Pins:
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_INTR 20

#define SPI_PORT spi0

#define STRIKE_SAMPLES 2048 //Number of samples to be collected
#define BPS 6 //Bytes-Per-Sample

void read_state_off(void);

void read_state_on(void);

void intr_state_off(void);

void intr_state_on(void);

void reset_intr_state(void);

uint8_t reading_validation(void);

void fetch_data(uint8_t *);