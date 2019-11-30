#include <stdint.h>
#include <inttypes.h>

#include "mesh.h"

struct host *hosts = NULL;

void dumpMap() {
    struct host *scan;
    printf("Map dump\n");
    
    for (scan = hosts; scan; scan = scan->next) {
        printf("  %012" PRIx64 " = %s\n", scan->mac, ntoa(scan->ip));
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

// Set the IP address of a host (keyed by MAC address) and return 1 if the host map
// has been changed
int setHost(uint64_t mac, uint32_t ip) {
    if (ip == 0) return 0;
    if (ip == -1) return 0;
    if ((ip & 0xFF) == 0x7f) return 0;

    struct host *scan;

    int modified = 0;

    for (scan = hosts; scan; scan = scan->next) {
        if ((scan->ip == ip) && (scan->mac == mac)) {
            return 0;
        }
        if (scan->ip == ip) {
            scan->mac = mac;
            modified++;
#ifdef DEBUG
            printf("Updating MAC address for %08" PRIx32 " to %012" PRIx64 "\n",
                scan->ip, scan->mac);
#endif
        } else if (scan->mac == mac) {
            scan->ip == ip;
            modified++;
#ifdef DEBUG
            printf("Updating IP address for %012" PRIx64 " to %08" PRIx32 "\n",
                scan->mac, scan->ip);
#endif
        }
    }

    if (modified == 0) {
        struct host *newhost = (struct host *)malloc(sizeof(struct host));
        newhost->ip = ip;
        newhost->mac = mac;
        newhost->next = NULL;

#ifdef DEBUG
        printf("Adding new host mapping for %012" PRIx64 " = %08" PRIx32 "\n",
            newhost->mac, newhost->ip);
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
    }

#ifdef DEBUG
    dumpMap();
#endif

    return 1;
}
