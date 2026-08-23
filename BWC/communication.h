#define CONNECT_RETRY       10  // Number of times to attempt a connection before giving up
#define MAC_LEN             18  // Number of characters needed to store a MAC address + terminator
#define MAX_DIR_SIZE        200 // Likely can't hold even 100 samples, so this should suffice
#define WIFI_IS_CONNECTED   3   // This is equal to CYW43_LINK_UP
#define GITHUB_SUCCESS_CODE 201 // See https://docs.github.com/en/rest/repos/contents?apiVersion=2026-03-10#create-or-update-file-contents

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    char *http_request;
    int timeout;
    int http_state;
} TLS_CLIENT_T;

int connect_to_wifi(void);

void disconnect_from_wifi(void);

int wifi_status(void);

TLS_CLIENT_T *send_data(const char *path, const int16_t *content);

void set_time(void);

uint64_t get_time(void);

void set_mac(void);

void get_mac(char *mac_buff);