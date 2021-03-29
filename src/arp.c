#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "mesh.h"

struct host hosts[MAX_HOSTS];

void dumpMap() {
    if (config.debug == 0) return;
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
    dbg_printf("getHost(%012" PRIx64 ")\n", address);
    for (i = 0; i < MAX_HOSTS; i++) {


        if (hosts[i].valid == 0) {
            continue;
        }

        dbg_printf("  %012" PRIx64 " - %s\n", hosts[i].mac, ntoa(hosts[i].ip));

        if (hosts[i].mac == address) {
            dbg_printf("  -> Match found\n");
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
int purgeHostsTable(uint64_t mac, uint32_t ip, uint16_t port) {
    int i;
    int modified = 0;
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid = 0) continue;

        if ((hosts[i].ip == ip) && (hosts[i].port == port) && (hosts[i].mac != mac)) {
            dbg_printf("Purging %012" PRIx64" at %s:%d\n", hosts[i].mac, ntoa(hosts[i].ip), hosts[i].port);
            hosts[i].valid = 0;
            modified = 1;
        }
    }
    return modified;
}

// Set the IP address of a host (keyed by MAC address) and return 1 if the host map
// has been changed
int setHost(uint64_t mac, uint32_t ip, uint16_t port, uint8_t updateIp, uint8_t auth) {
    if (ip == 0) return 0;
    if (ip == -1) return 0;
    if ((ip & 0xFF) == 0x7f) return 0;

    int modified = 0;

    dbg_printf("setHost(%012" PRIx64 ", %s, %d, %d)\n",
        mac, ntoa(ip), port, updateIp
    );

    int i;

    if (auth) {
        modified = purgeHostsTable(mac, ip, port);
    }

    int found = 0;
    for (i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].valid == 0) continue;
        if ((hosts[i].ip == ip) && (hosts[i].mac == mac) && (hosts[i].port == port)) {
            found = 1;
            if (auth) {
                hosts[i].seen = time(NULL);
                modified = 1;
            }
        }
        if (hosts[i].mac == mac) {
            if (updateIp == 1) {
                hosts[i].seen = time(NULL);
                hosts[i].ip = ip;
                hosts[i].port = port;
                found = 1;
                modified = 1;
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
                modified = 1;
                break;
            }
        }
    }

    dumpMap();

    return modified;
}
