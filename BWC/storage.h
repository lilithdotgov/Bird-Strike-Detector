#include "pico/stdlib.h"

void initialize_lfs(void);

void write_file(char *name, uint8_t *data);

void read_file(char *name);

void write_binary_file(char *name, uint8_t *data);

void read_binary_file(char *name, int16_t *buf);

void print_file(char *name);

void print_binary_file(char *name);

void print_dir(void);

int find_in_dir(char *str, char **return_files);

char *generate_bin_name(void);

void encode_data_to_base64(const int16_t *data, char *data_B64);