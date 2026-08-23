#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/cyw43_driver.h"
#include "pico/aon_timer.h"

#include "hardware/watchdog.h"

#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "lwip/apps/sntp.h"

#include "mbedtls/ssl.h"

#include "communication.h"
#include "storage.h"
#include "secrets.h" //In the repository this is secrets_example.h, modify it and rename it for your own usage!
#include "accelerometer.h"

// Note:
// cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
// You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
// these calls are a no-op and can be omitted, but it is a good practice to use them regardless

#define TIMEOUT_MS  30000            // Default timeout of 30 seconds
#define GITHUB_ADDR "api.github.com" // For DNS lookup and full request

#ifndef GITHUB_CERT
#define GITHUB_CERT     NULL
#define GITHUB_CERT_LEN 0
#endif

#define JSON_BODY_FORMAT "{\"message\":\"New Strike Log\",\"committer\":{\"name\":\"no1\",\"email\":\"odysseus@fakemail.gov\"},\"content\":\"%s\"}"

#define HTTP_REQUEST_FORMAT "PUT /repos/" GITHUB_ACC "/" GITHUB_REPO "/contents/%s HTTP/1.1\r\n" \
                            "Host: " GITHUB_ADDR "\r\n"                                          \
                            "Accept: application/vnd.github+json\r\n"                            \
                            "Content-Type: application/json\r\n"                                 \
                            "Content-Length: %d\r\n"                                             \
                            "Authorization: Bearer " GITHUB_TOKEN "\r\n"                         \
                            "X-GitHub-Api-Version: 2026-03-10\r\n"                               \
                            "User-Agent: " GITHUB_ACC "\r\n"                                     \
                            "Connection: close\r\n"                                              \
                            "\r\n"                                                               \
                            "%s"

// TODO: make all the error message be sent to stderr perhaps?

bool is_wifi_init = false;
bool sntp_state   = false;                 // Whether or not a new timestamp has been set
char __persistent_data(mac_addr[MAC_LEN]); // We don't wanna keep needing WiFi to get the MAC address for naming purposes

static struct altcp_tls_config *tls_config = NULL;

int connect_to_wifi(void) {
    int status;
    if (!is_wifi_init) {                         // Only initialize if not already active
        if ((status = cyw43_arch_init()) != 0) { // RP2 function
            printf("Failure to initialize cyw43_arch with code: %d\n", status);
            return status;
        }
        is_wifi_init = true;
    }

    cyw43_arch_enable_sta_mode(); // RP2 function. Enables WiFi in station mode (client), has no error output
    // For reference, AP is Access Point and for the role of serving, while STA is Station and for the role of being a client to an AP

    printf("Connecting to WiFi...\n");

    if ((status = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, TIMEOUT_MS)) != 0) { // RP2 function
        printf("Failure to connect to WiFi with code: %d\n", status);
        cyw43_arch_disable_sta_mode();
        return status;
    }

    return 0;
}

void disconnect_from_wifi(void) { // Should not be called unless absolutely sure you won't need any cyw libraries until a reset
    // All are RP2 functions
    if (is_wifi_init) {
        cyw43_arch_lwip_begin();
        cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
        cyw43_arch_disable_sta_mode(); // Apparently some routers may be mad if you don't do this
        cyw43_arch_lwip_end();

        uint64_t timeout_us = make_timeout_time_ms(250);
        while (get_absolute_time() < timeout_us) {
            cyw43_arch_poll();
            sleep_ms(10); // Yield to prevent tight-loop lockups
        }

        cyw43_arch_deinit(); // Will cause a hard fault if any cyw libraries are used after being called
        is_wifi_init = false;
        printf("Disconnected from WiFi!\n");
    }
}

int wifi_status(void) { // Returns 3 if running (CYW43_LINK_UP)
    if (!is_wifi_init) {
        return -1;
    }

    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (status != WIFI_IS_CONNECTED) { // RP2 function
        printf("Not connected to WiFi with code: %d\n", status);
        sleep_ms(2000); // Give some time before another attempt at connecting
    }

    return status;
}

static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;
    err_t err           = ERR_OK;

    state->complete = true;
    if (state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            printf("Failure to close PCB/TCB, calling abort with code: %d\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) { // This is the one that actually sends the data
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;
    if (err != ERR_OK) {
        printf("Failure to connect with code: %d\n", err);
        return tls_client_close(state);
    }

    printf("Sending request in chunks...\n");
    u16_t request_length = strlen(state->http_request);
    u16_t written        = 0;
    u16_t chunk_size     = 2000; // Safely below MBEDTLS_SSL_OUT_CONTENT_LEN (2048)
    u16_t to_write;              // How many bytes will be written each loop

    while (written < request_length) {
        to_write = request_length - written;
        if (to_write > chunk_size) { // If too big just set to chunk_size
            to_write = chunk_size;
        }
        printf("Writing %d bytes to HTTPS\n", to_write);

        err = altcp_write(state->pcb, state->http_request + written, to_write, TCP_WRITE_FLAG_COPY); // written adds to a pointer location!
        if (err != ERR_OK) {
            printf("Error writing chunk at offset %d, err=%d\n", written, err);
            return tls_client_close(state);
        }
        written += to_write;
    }

    printf("Finished sending all data!\n");
    return ERR_OK;
}

static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) { // lwIP has timedout and needs to be closed
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;
    printf("timed out\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tls_client_close(arg);
}

static void tls_client_err(void *arg, err_t err) { // Doesn't need to free PCB as it's handled automatically by altcp_err()
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;
    printf("tls_client_err %d\n", err);
    if (state) {
        state->pcb      = NULL;
        state->complete = true;
        state->error    = PICO_ERROR_GENERIC;
    }
}

static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;
    if (!p) { // Remote has closed connection so close on our side as well
        printf("Connection closed by Remote, closing client-side connection...\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {         // Ensure the packet isn't empty
        char buf[p->tot_len + 1]; // allocate a buffer to store the incomming packet (in full, since I think RAM can handle it)

        pbuf_copy_partial(p, buf, p->tot_len, 0); // Can just handle an arbitrary pointer, unlike pbuf_copy()

        buf[p->tot_len] = '\0'; // Ensure we have a string terminator as that isn't supplied

        printf("\nnew data received from server:\n-----\n\n%s\n\n-----\n", buf);

        // Check for Github success code
        if (strncmp(buf, "HTTP/", 5) == 0) {
            // Extract the status code ignoring the HTTP/1.x part
            if (sscanf(buf, "HTTP/1.%*d %d", &state->http_state) == 1) {
                if (state->http_state == 201) {
                    printf("Success! GitHub created the file (201 Created).\n");
                } else {
                    printf("Warning: GitHub API returned status %d\n", state->http_state);
                }
            }
        }

        // Needs to be called after user is done reading the data, must specify how many bytes were read (p->tot_len)
        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state) {
    err_t err;
    u16_t port = 443; // Port for HTTPS

    printf("Connecting to GitHub...\n");

    // altcp_connect will return instantly, it does not wait for the connection protocal to finish
    // Instead the callback function supplied as the 4th argument is meant to handle that
    err = altcp_connect(state->pcb, ipaddr, port, tls_client_connected);
    if (err != ERR_OK) {
        fprintf(stderr, "Failure connecting to GitHub with code: %d\n", err);
        tls_client_close(state);
    }
}

static void tls_client_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    if (ipaddr) {
        printf("DNS resolving complete\n");
        tls_client_connect_to_server_ip(ipaddr, (TLS_CLIENT_T *)arg);
    } else {
        printf("error resolving hostname %s\n", hostname);
        tls_client_close(arg);
    }
}

static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)arg;

    cyw43_arch_lwip_begin();
    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY); // lwIP function, creates new PCB/TCB
    cyw43_arch_lwip_end();

    if (!state->pcb) {
        printf("Failure to create new PCB/TCB, not enough memory\n");
        return false;
    }

    // All lwIP functions:
    cyw43_arch_lwip_begin();

    // Specifies the PCB/TCB to be passed to all other callback functions, and the TLS_CLIENT that will be passed to the callbacks
    altcp_arg(state->pcb, state);

    // Sets a callback to handle what occurs after a timeout
    // specifically this is only when the TCP stack is idle, the timer does not count down during
    // It does a "/1000" since the input expects seconds, and "* 2" since it actually polls at half the given interval (for some reason)
    altcp_poll(state->pcb, tls_client_poll, ((state->timeout) / 1000) * 2);

    // Sets a callback to handle new data that arrives
    // It passes the new data via a pbuf that is sent to the callback function
    // It will additionally pass a NULL pbuf to indicate the remote host has closed the connection
    // tls_client_recv should return ERR_OK or ERR_ABRT to indicate it has properly freed the pbuf after closing the connection
    altcp_recv(state->pcb, tls_client_recv);

    // Sets a callback to handle what occurs after a fatal error during the TCP
    altcp_err(state->pcb, tls_client_err);
    cyw43_arch_lwip_end();

    // SNI is needed for host to give us the proper certificate
    // Although we never actually use it for verification ourselves, it's a required part of the TLS handshake
    mbedtls_ssl_set_hostname(altcp_tls_context(state->pcb), hostname);

    printf("resolving %s\n", hostname);

    // More lwIP functions
    cyw43_arch_lwip_begin();

    // Resolves DNS query to server_ip
    //  3rd argument is callback function
    //  4th being the TLS_CLIENT that will be sent as an argument to the callback
    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);

    if (err == ERR_OK) { // hostname is a valid IP address string or the host name is already in the local names table
        tls_client_connect_to_server_ip(&server_ip, state);
    } else if (err != ERR_INPROGRESS) {
        printf("error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }

    cyw43_arch_lwip_end();

    return err == ERR_OK || err == ERR_INPROGRESS;
}

// Returns the state variable on success, NULL otherwise, must be freed!!!
// Currently doesn't spport certificate checking
TLS_CLIENT_T *send_data(const char *path, const int16_t *content) {

    tls_config = altcp_tls_create_config_client(GITHUB_CERT, GITHUB_CERT_LEN); // lwIP function, creates handle
    assert(tls_config);

    TLS_CLIENT_T *state = calloc(1, sizeof(TLS_CLIENT_T)); // calloc initializes the memory to 0

    // We need to turn content into a Base64 encoded string first
    // For each set of samples the 3 axes get turned into 8 characters + null terminator
    char *content_B64 = malloc(8 * STRIKE_SAMPLES + 1);
    if (!content_B64) {
        fprintf(stderr, "Failure to allocate memory for Base64\n");
        free(state);
        altcp_tls_free_config(tls_config);
        return NULL;
    }
    encode_data_to_base64(content, content_B64);

    // snprintf is an insane optimization here, it prints 0 characters to no buffer
    // butttt, it returns how many characters would've been written had it been able to write them all.
    // That plus 1 for the string terminator gets you the length without needing to worry about memory
    int body_len = snprintf(NULL, 0, JSON_BODY_FORMAT, content_B64);
    char *body   = malloc(body_len + 1); //+1 for terminator
    if (!body) {
        fprintf(stderr, "Failure to allocate memory for JSON Body");
        free(state);
        altcp_tls_free_config(tls_config);
        return NULL;
    }
    snprintf(body, body_len + 1, JSON_BODY_FORMAT, content_B64); // Now write the body to the buffer
    free(content_B64);

    // 2. Format complete HTTP request with Content-Length header
    int req_len         = snprintf(NULL, 0, HTTP_REQUEST_FORMAT, path, body_len, body) + 1; // Now get length of full request
    state->http_request = malloc(req_len);
    if (!state->http_request) {
        fprintf(stderr, "Failure to allocate memory for HTTPS Request");
        free(body);
        free(state);
        altcp_tls_free_config(tls_config);
        return NULL;
    }
    snprintf(state->http_request, req_len, HTTP_REQUEST_FORMAT, path, body_len, body);
    free(body); // Done with temporary body buffer

    state->timeout = TIMEOUT_MS;
    if (!tls_client_open(GITHUB_ADDR, state)) { // Cleanup on failure
        free(state->http_request);
        free(state);
        altcp_tls_free_config(tls_config);
        return NULL; // TODO: Replace with corresponding SDK Error enum
    }

    int max_time_ms     = 1000 * 30;                         // Give lwIP 30 seconds to sort itself out
    uint64_t timeout_us = make_timeout_time_ms(max_time_ms); // Get the future time
    while (!state->complete && (get_absolute_time() < timeout_us)) {
        printf("Waiting on HTTPS...\n");
        cyw43_arch_poll();                                          // Required in polling mode, keeps process churning
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000)); // Skips 1s wait if there's work to do
        fflush(stdout);
    }

    if (!state->complete) {
        printf("Error: HTTPS request timed out in polling loop.\n");
    }

    // Cleanup and end request
    free(state->http_request);
    // free(state); Add this if you change your mind
    altcp_tls_free_config(tls_config);
    return state;
}

// callback for lwIP/SNTP to set the aon_timer to UTC
// we configure SNTP to call this function when it receives a valid NTP timestamp
// (see lwipopts.h)
void sntp_set_system_time_us(unsigned int sec, unsigned int us) {
    static struct timespec ntp_ts;
    ntp_ts.tv_sec  = sec;
    ntp_ts.tv_nsec = us * 1000;

    if (aon_timer_is_running()) { // AON already running, set to new timestamp
        aon_timer_set_time(&ntp_ts);
        sntp_state = true;
        printf("Updated time!\n");
    } else { // AON is not on yet so set starting time to the new timestamp
        aon_timer_start(&ntp_ts);
        sntp_state = true;
        printf("Set time via SNTP!\n");
    }
}

void set_time(void) {                        // Function user calls to setup the AON time via SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // Needs to be called before sntp_init()
    sntp_init();

    int max_time_ms     = 1000 * 60 * 5;                     // Give SNTP 5 minutes to sort itself out
    uint64_t timeout_us = make_timeout_time_ms(max_time_ms); // Get the future time
    while (!sntp_state && (get_absolute_time() < timeout_us)) {
        printf("Waiting on SNTP...\n");
        cyw43_arch_poll();                                          // Required in polling mode, keeps process churning
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000)); // Skips 1s wait if there's work to do
        fflush(stdout);
    }

    sntp_stop();

    // If all fails and we still cannot get the system time, we cannot set it to default to some number
    // Instead set a timer to wake up and reset from in 5 minutes
    if (!aon_timer_is_running()) {
        printf("set_time: Failed completely to get SNTP on boot, restarting and trying again in 5 minutes...\n");
        fflush(stdout);
        sleep_ms(1000 * 60 * 6);

        // Reset:
        watchdog_enable(1, 1);
        while (1);
    }
}

uint64_t get_time(void) {          // Function user calls to recieve current time
    if (!aon_timer_is_running()) { // Ensures we aren't getting a nonsense timestamp
        if (wifi_status() != WIFI_IS_CONNECTED) {
            for (int i = 0; (i < CONNECT_RETRY) && (wifi_status() != WIFI_IS_CONNECTED); i++) { // Attempt multiple times to conect to WiFi
                printf("Wifi connection attempt #%d\n", i);
                fflush(stdout);
                connect_to_wifi();
            }
        }

        if (wifi_status() == WIFI_IS_CONNECTED) {
            set_time();
        }
    }

    // If all fails and we still cannot get the system time, we cannot set it to default to some number
    // Instead set a timer to wake up and reset from in 5 minutes
    if (!aon_timer_is_running()) {
        printf("get_time: Failed completely to get SNTP on boot, restarting and trying again in 5 minutes...\n");
        fflush(stdout);
        sleep_ms(1000 * 60 * 6);

        // Reset:
        watchdog_enable(1, 1);
        while (1);
    }

    struct timespec current_time;

    aon_timer_get_time(&current_time);
    return current_time.tv_sec;
}

void set_mac(void) {    // To be called to store MAC while WiFi is up, can't use cyw function otherwise
    uint8_t raw_mac[6]; // Below function only returns 6 bytes of MAC data and needs to be formatted
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, raw_mac);
    snprintf(mac_addr, MAC_LEN, "%02X-%02X-%02X-%02X-%02X-%02X", raw_mac[0], raw_mac[1], raw_mac[2], raw_mac[3], raw_mac[4], raw_mac[5]);
}

void get_mac(char *mac_buff) {
    if (mac_buff) {
        strcpy(mac_buff, mac_addr);
    }
}