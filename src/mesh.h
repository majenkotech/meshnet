#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <netinet/in.h>

#include <stdint.h>

#define MAX_PEERS 100

struct configuration {
    uint32_t ip;                    // tap IP address
    uint32_t netmask;               // tap subnet /xx
    char psk[1024];                 // AES PSK
    uint32_t peerips[MAX_PEERS];    // List of peer IPs
    uint16_t peerports[MAX_PEERS];  // List of peer ports
    uint32_t address;               // Public IP address
    uint32_t announce;              // Announce period
    uint16_t port;                  // Communication UDP port
    uint8_t fork;                   // Should we fork into the backgroun?
};

extern struct configuration config;

struct host {
    uint64_t mac;
    uint32_t ip;
    uint16_t port;
    struct host *next;
};

extern struct host *hosts;

extern void startNetReader();
extern void startTapReader();
extern void startPeriodic();
extern void openTapDevice();
extern void closeTapDevice();
extern int initNetwork();
extern uint32_t aton(const char *);
extern char *ntoa(uint32_t);
extern void announceMe(uint32_t, uint16_t);
extern int setHost(uint64_t, uint32_t, uint16_t, uint8_t);
extern uint32_t getHost(uint64_t);
extern uint16_t getPort(uint64_t);
extern void printMac(uint64_t);
extern int netSend(uint64_t, uint8_t *, unsigned long);
extern void getMyMacAddress();
extern uint64_t str2mac(char *);
extern char *mac2str(uint64_t);
extern int tap_open(char *);
extern int tap_close(int, char *);
extern int tap_read(int, uint8_t *, int);
extern int aes_init(char *);
extern int aes_sendto(int, uint8_t *, int, int, struct sockaddr *, int);
extern int aes_recvfrom(int,uint8_t *,int,int,struct sockaddr *, int *);
extern int hostExists(uint64_t mac);
extern void dumpMap();


extern unsigned long tapdev;
extern char tapname[16];
extern uint64_t myMAC;
extern pthread_t tapReader;
extern pthread_t netReader;

// Data types
#define DT_DATA 0x00
#define DT_COMMAND 0x01
#define DT_PART1 0x02
#define DT_PART2 0x03

// Commands
#define CMD_ANNOUNCE 0x00
#define CMD_HOSTS 0x01
