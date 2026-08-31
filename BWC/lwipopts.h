#ifndef _LWIPOPTS_EXAMPLE_COMMONH_H
#define _LWIPOPTS_EXAMPLE_COMMONH_H

// Common settings used in most of the pico_w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for details)

#define NO_SYS          1 // Stating that you are not using an OS and therefore can't use multithreading
#define LWIP_SOCKET     0 // Can't be used without OS
#define MEM_LIBC_MALLOC 0 // MEM_LIBC_MALLOC is incompatible with non polling versions

#define MEM_ALIGNMENT 4
#define MEM_SIZE      32000 // When MEM_LIB_MALLOC is 0 lwIP uses a single static array for operations, this is that array size

#define MEMP_NUM_TCP_SEG           32 * 2 // Increase by 2 to hold our massive HTTTPS request
#define MEMP_NUM_ARP_QUEUE         10
#define PBUF_POOL_SIZE             24
#define LWIP_ARP                   1
#define LWIP_ETHERNET              1
#define LWIP_ICMP                  1
#define LWIP_RAW                   1
#define TCP_WND                    (8 * TCP_MSS)
#define TCP_MSS                    1460
#define TCP_SND_BUF                (8 * TCP_MSS) * 2 // Increase by 2 to hold our massive HTTTPS request. Might be redundant now that we are sending via chunks. TODO: Revisit this later
#define TCP_SND_QUEUELEN           ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK   1
#define LWIP_NETIF_HOSTNAME        1
#define LWIP_NETCONN               0
#define MEM_STATS                  0
#define SYS_STATS                  0
#define MEMP_STATS                 0
#define LINK_STATS                 0
// #define ETH_PAD_SIZE                2
#define LWIP_CHKSUM_ALGORITHM     3
#define LWIP_DHCP                 1
#define LWIP_IPV4                 1
#define LWIP_TCP                  1
#define LWIP_UDP                  1
#define LWIP_DNS                  1
#define LWIP_TCP_KEEPALIVE        1
#define LWIP_NETIF_TX_SINGLE_PBUF 1
#define DHCP_DOES_ARP_CHECK       0
#define LWIP_DHCP_DOES_ACD_CHECK  0

#ifndef NDEBUG
#define LWIP_DEBUG         1
#define LWIP_STATS         1
#define LWIP_STATS_DISPLAY 1
#endif

#define ETHARP_DEBUG     LWIP_DBG_OFF
#define NETIF_DEBUG      LWIP_DBG_OFF
#define PBUF_DEBUG       LWIP_DBG_ON
#define API_LIB_DEBUG    LWIP_DBG_OFF
#define API_MSG_DEBUG    LWIP_DBG_OFF
#define SOCKETS_DEBUG    LWIP_DBG_OFF
#define ICMP_DEBUG       LWIP_DBG_OFF
#define INET_DEBUG       LWIP_DBG_OFF
#define IP_DEBUG         LWIP_DBG_OFF
#define IP_REASS_DEBUG   LWIP_DBG_OFF
#define RAW_DEBUG        LWIP_DBG_OFF
#define MEM_DEBUG        LWIP_DBG_ON
#define MEMP_DEBUG       LWIP_DBG_OFF
#define SYS_DEBUG        LWIP_DBG_OFF
#define TCP_DEBUG        LWIP_DBG_ON
#define TCP_INPUT_DEBUG  LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG LWIP_DBG_OFF
#define TCP_RTO_DEBUG    LWIP_DBG_OFF
#define TCP_CWND_DEBUG   LWIP_DBG_OFF
#define TCP_WND_DEBUG    LWIP_DBG_OFF
#define TCP_FR_DEBUG     LWIP_DBG_OFF
#define TCP_QLEN_DEBUG   LWIP_DBG_OFF
#define TCP_RST_DEBUG    LWIP_DBG_OFF
#define UDP_DEBUG        LWIP_DBG_OFF
#define TCPIP_DEBUG      LWIP_DBG_OFF
#define PPP_DEBUG        LWIP_DBG_OFF
#define SLIP_DEBUG       LWIP_DBG_OFF
#define DHCP_DEBUG       LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */

#define LWIP_ALTCP             1 // Needed for proper usage of Application Layer TCP, see: https://www.nongnu.org/lwip/2_1_x/group__altcp__api.html
#define LWIP_ALTCP_TLS         1 // Needed for proper usage of TLS
#define LWIP_ALTCP_TLS_MBEDTLS 1 // Needed for proper usage of mbedTLS

// Hardcode the number of TPC, ALTCP, and TLS PCBs allowed, lwip default options can lead to 0 TLS PCBs being allowed due to division errors
#define MEMP_NUM_TCP_PCB         2
#define MEMP_NUM_ALTCP_PCB       2
#define MEMP_NUM_ALTCP_TLS_STATE 1

#define MEMP_NUM_SYS_TIMEOUT             (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1) // For some reason needed?
#define SNTP_MAX_SERVERS                 LWIP_DHCP_MAX_NTP_SERVERS           // Default setting of 1
#define SNTP_GET_SERVERS_FROM_DHCP       LWIP_DHCP_GET_NTP_SRV               // Default setting of 0
#define SNTP_SERVER_DNS                  1                                   // Allows setting of server via DNS (via SNTP_SERVER_ADDRESS)
#define SNTP_SERVER_ADDRESS              "pool.ntp.org"                      // See website for more information
#define SNTP_PORT                        LWIP_IANA_PORT_SNTP                 // Default of 123, standard port for NTP
#define SNTP_CHECK_RESPONSE              0                                   // Turns off sanity checks, is default
#define SNTP_COMP_ROUNDTRIP              0                                   // Turns off roundtrip calculation
#define SNTP_STARTUP_DELAY               0                                   // Disables start-up delay
#define SNTP_RECV_TIMEOUT                15000                               // How long to wait before retrying request, default is 15 seconds and specification says to not go below that
#define SNTP_RETRY_TIMEOUT_EXP           1                                   // RFC standard defines it best to increase the retry time for each retry, this enables said feature
#define SNTP_RETRY_TIMEOUT               SNTP_RECV_TIMEOUT                   // Default retry timeout, this is doubled each time until the SNTP_RETRY_TIMEOUT_MAX
#define SNTP_RETRY_TIMEOUT_MAX           (SNTP_RETRY_TIMEOUT * 10)           // Default value, max that a retry can take
#define SNTP_UPDATE_DELAY                3600000                             // How often to update timestamp, not important for our purposes, default is 1 hour
#define SNTP_MONITOR_SERVER_REACHABILITY 1                                   // Keeps an internal register of server reachability per RFC standards

//* configure SNTP to use our callback functions for setting the system time
#define SNTP_SET_SYSTEM_TIME_US(sec, us) sntp_set_system_time_us(sec, us)

//* declare our callback functions
#include <stdint.h>
void sntp_set_system_time_us(unsigned int sec, unsigned int us);