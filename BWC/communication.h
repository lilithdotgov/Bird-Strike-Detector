#define CONNECT_RETRY 10 // Number of times to attempt a connection before giving up

int connect_to_wifi(void);

void disconnect_from_wifi(void);

int wifi_status(void);

int send_data(const char *path, const char *content);

void set_time(void);

uint64_t get_time(void);