#include <stdint.h>
#include <inttypes.h>

#include "mesh.h"

struct host *hosts = NULL;

void dumpMap() {
    struct host *scan;
    printf("Map dump\n");
    
    for (scan = hosts; scan; scan = scan->next) {
        printf("  %012" PRIx64 " = %s:%d\n", scan->mac, ntoa(scan->ip), scan->port);
	}
}

uint32_t getHost(uint64_t address) {
    struct host *scan;

    for (scan = hosts; scan; scan = scan->next) {
        if (scan->mac == address) {
            return scan->ip;
        }
    }
    return -1;
}

uint16_t getPort(uint64_t address) {
    struct host *scan;

    for (scan = hosts; scan; scan = scan->next) {
        if (scan->mac == address) {
            return scan->port;
        }
    }
    return -1;
}

// Set the IP address of a host (keyed by MAC address) and return 1 if the host map
// has been changed
int setHost(uint64_t mac, uint32_t ip, uint16_t port, uint8_t updateIp) {
    if (ip == 0) return 0;
    if (ip == -1) return 0;
    if ((ip & 0xFF) == 0x7f) return 0;

#ifdef DEBUG
    printf("setHost(0x%012" PRIx64 ", 0x%08" PRIx32 ", %d, %d)\n",
        mac, ip, port, updateIp
    );
#endif

    struct host *scan;

    for (scan = hosts; scan; scan = scan->next) {
        if ((scan->ip == ip) && (scan->mac == mac) && (scan->port == port)) {
            printf("Found dupliacte\n");
            return 0;
        }
        if (scan->mac == mac) {
#ifdef DEBUG
            printf("Updating IP address for %012" PRIx64 " from %s:%d\n",
                scan->mac, ntoa(scan->ip), scan->port);
#endif
            if (updateIp == 1) {
                scan->ip == ip;
                scan->port = port;
            } else {
                return 0;
            }
#ifdef DEBUG
            printf("          to %s:%d\n",
                ntoa(scan->ip), scan->port);
#endif
            return 1;
        }
    }

    struct host *newhost = (struct host *)malloc(sizeof(struct host));
    newhost->ip = ip;
    newhost->mac = mac;
    newhost->port = port;
    newhost->next = NULL;

#ifdef DEBUG
    printf("Adding new host mapping for %012" PRIx64 " = %s:%d\n",
        newhost->mac, ntoa(newhost->ip), newhost->port);
#endif

    if (hosts == NULL) {
        hosts = newhost;
    } else {
        for (scan = hosts; scan; scan = scan->next) {
            if (scan->next == NULL) {
                scan->next = newhost;
                break;
            }
        }
    }

#ifdef DEBUG
    dumpMap();
#endif

    return 1;
}
