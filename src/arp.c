#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "mesh.h"

struct host hosts[MAX_HOSTS];

void dumpMap() {
    int i;
    printf("Map dump\n");
    
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid == 1) {
            printf("  %012" PRIx64 " = %s:%d\n", hosts[i].mac, ntoa(hosts[i].ip), hosts[i].port);
        }
	}
}

uint32_t getHost(uint64_t address) {
    int i;
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid == 0) {
            continue;
        }

        if (hosts[i].mac == address) {
            return hosts[i].ip;
        }
    }
    return -1;
}

uint16_t getPort(uint64_t address) {
    int i;
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid == 0) {
            continue;
        }

        if (hosts[i].mac == address) {
            return hosts[i].port;
        }
    }
    return -1;
}

// Scan through the hosts table looking for any entries where the ip and port match
// but the MAC address does NOT.  These are redundant entries from some time in
// the past and must be deleted.
void purgeHostsTable(uint64_t mac, uint32_t ip, uint16_t port) {
    int i;

    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid = 0) continue;

        if ((hosts[i].ip == ip) && (hosts[i].port == port) && (hosts[i].mac != mac)) {
#ifdef DEBUG
            printf("Purging %012" PRIx64" at %s:%d\n", hosts[i].mac, ntoa(hosts[i].ip), hosts[i].port);
#endif
            hosts[i].valid = 0;
        }
    }
}

// Set the IP address of a host (keyed by MAC address) and return 1 if the host map
// has been changed
int setHost(uint64_t mac, uint32_t ip, uint16_t port, uint8_t updateIp) {
    if (ip == 0) return 0;
    if (ip == -1) return 0;
    if ((ip & 0xFF) == 0x7f) return 0;

#ifdef DEBUG
    printf("setHost(%012" PRIx64 ", %s, %d, %d)\n",
        mac, ntoa(ip), port, updateIp
    );
#endif

    int i;

    int found = 0;
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid == 0) continue;
        if ((hosts[i].ip == ip) && (hosts[i].mac == mac) && (hosts[i].port == port)) {
            found = 1;
            if (updateIp) {
                hosts[i].seen = time(NULL);
            }
            // Already there, skip.
	    return 0;
        }
        if (hosts[i].mac == mac) {
            if (updateIp == 1) {
                hosts[i].seen = time(NULL);
                hosts[i].ip = ip;
                hosts[i].port = port;
                found = 1;
                return 1;
            }
        }
    }

    if (!found) {
        for (i = 0; i < MAX_HOSTS; i++) {
            if (hosts[i].valid == 0) {
                hosts[i].ip = ip;
                hosts[i].mac = mac;
                hosts[i].port = port;
                hosts[i].valid = 1;
                hosts[i].seen = time(NULL);
                break;
            }
        }
    }

#ifdef DEBUG
    dumpMap();
#endif

    return 1;
}
