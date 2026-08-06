#include "pico/stdlib.h"

void initialize_lfs(void);

void write_file(char *name, uint8_t *data);

void read_file(char *name);

void list_dir(void);