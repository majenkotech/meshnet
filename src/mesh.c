#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "mesh.h"

struct configuration config;

void threadexit(int signal) {
	// Do nothing much
}

void cleanexit(int signal) {
#ifdef DEBUG
	printf("Terminating.\n");
#endif
//	pthread_kill(tapReader,SIGINT);
//	pthread_kill(netReader,SIGINT);
//	closeTapDevice();
	exit(0);
}

void usage() {
	printf("Usage: mesh <config file>\n");
}

char *trimWhiteSpace(char *str) {
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int loadConfig(const char *file) {
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("Unable to open %s: %s\n", file, strerror(errno));
        return -1;
    }

    config.psk[0] = 0;
    config.ip = -1;
    config.netmask = 0;
    config.network6[0] = 0;
    config.peerips[0] = -1;
    config.address = -1;
    config.announce = 60;
    config.port = 3647;
    config.fork = 0;

    char temp[1024];
    int line = 0;
    while (fgets(temp, 1024, f)) {
        line++;

        char *key = strtok(temp, " \t\n");
        char *val = strtok(NULL, " \t\n");

        if (key == NULL) continue;

        if (val) {

            if (strcasecmp(key, "psk") == 0) {

                strncpy(config.psk, val, 1024);
                continue;
            }

            if (strcasecmp(key, "network") == 0) {
                char *ip1 = strtok(val, ".");
                char *ip2 = strtok(NULL, ".");
                char *ip3 = strtok(NULL, ".");
                char *ip4 = strtok(NULL, "/");
                char *mask = strtok(NULL, "\n");

                if (mask) {
                    uint8_t ip1n = strtoul(ip1, NULL, 10);
                    uint8_t ip2n = strtoul(ip2, NULL, 10);
                    uint8_t ip3n = strtoul(ip3, NULL, 10);
                    uint8_t ip4n = strtoul(ip4, NULL, 10);
                    uint8_t maskn = strtoul(mask, NULL, 10);
                    config.ip = 0;
                    config.ip |= (uint32_t)ip4n << 24;
                    config.ip |= (uint32_t)ip3n << 16;
                    config.ip |= (uint32_t)ip2n << 8;
                    config.ip |= (uint32_t)ip1n << 0;
                    config.netmask = maskn;
                } else {
                    printf("Syntax error at line %d\n", line);
                    fclose(f);
                    return -1;
                }
                continue;
            }

            if (strcasecmp(key, "network6") == 0) {
                strcpy(config.network6, val);
                continue;
            }

            if (strcasecmp(key, "peer") == 0) {

                char *peer = strtok(val, ",");
                int peerno = 0;
                while (peer) {
                    uint16_t port = 3647;
                    char *colon = strchr(peer, ':');
                    if (colon != NULL) {
                        *colon = 0;
                        colon++;
                        port = strtoul(colon, NULL, 10);
                    }
                    config.peerips[peerno] = inet_addr(peer);
                    config.peerports[peerno] = port;

#if DEBUG
                    printf("Adding peer %s:%d\n", ntoa(config.peerips[peerno]), config.peerports[peerno]);
#endif
                    peerno++;
                    config.peerips[peerno] = -1;
                    peer = strtok(NULL, ",");
                }
                continue;
            }

            if (strcasecmp(key, "ip") == 0) {
                config.address = inet_addr(val);
                continue;
            }

            if (strcasecmp(key, "announce") == 0) {
                config.announce = strtoul(val, NULL, 10);
                continue;
            }

            if (strcasecmp(key, "port") == 0) {
                config.port = strtoul(val, NULL, 10);
                continue;
            }

            if ((strcasecmp(key, "fork") == 0) && (strcasecmp(val, "yes") == 0)) {
                config.fork = 1;
                continue;
            }

            printf("Syntax error at line %d\n", line);
        } else {
            printf("Syntax error at line %d\n", line);
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    int i;

	signal(SIGINT,&cleanexit);
	signal(SIGKILL,&cleanexit);
	signal(SIGTERM,&cleanexit);

    if (argc != 2) {
        if (loadConfig(SYSCONF "/meshnet/default.mesh") == -1) {
            return -1;
        }
    } else {
        if (loadConfig(argv[1]) == -1) {
            return -1;
        }
    }

    if (config.psk[0] == 0) {
        printf("Error: no PSK provided.\n");
        return -1;
    }

	aes_init(config.psk);

    if (config.fork == 1) {
		if(fork())
		{
			exit(0);
		}
    }

	openTapDevice();
	if (initNetwork() != 0) {
        printf("Unable to configure network - exiting.");
        return -1;
    }
	startTapReader();
	startNetReader();
	startPeriodic();
	sleep(1);

    if (config.address != -1) {
		setHost(myMAC, config.address, config.port, 1);
	}

    if (config.ip != -1) {
        char temp[2048];

        sprintf(temp, "/sbin/ifconfig %s up", tapname);
        if (system(temp) != 0) {
            printf("Error bringing up interface.\n");
            return -1;
        }

        sprintf(temp, "/sbin/ifconfig %s inet %d.%d.%d.%d/%d", 
            tapname,
            (config.ip >> 0) & 0xFF,
            (config.ip >> 8) & 0xFF,
            (config.ip >> 16) & 0xFF,
            (config.ip >> 24) & 0xFF,
            config.netmask
        );

        if (system(temp) != 0) {
            printf("Error setting IP address\n");
            return -1;
        }

        if (config.network6[0]) {
            sprintf(temp, "/sbin/ifconfig %s add %s", tapname, config.network6);
            if (system(temp) != 0) {
                printf("Error setting IPv6 address\n");
                return -1;
            }
        }

    } else { // We want DHCP?
#ifdef DEBUG
        printf("Starting dhclient...\n");
#endif
        if (!fork()) {
            execl("/sbin/dhclient", "dhclient", tapname, NULL);
            exit(0);
        }
    }
        
    for (i = 0; (i < MAX_PEERS) && (config.peerips[i] != -1); i++) {
#ifdef DEBUG
        printf("Announcing to %d.%d.%d.%d\n",
            (config.peerips[i] >> 0) & 0xFF,
            (config.peerips[i] >> 8) & 0xFF,
            (config.peerips[i] >> 16) & 0xFF,
            (config.peerips[i] >> 24) & 0xFF
        );
#endif
        announceMe(config.peerips[i], config.peerports[i]);
    }
            

	pthread_join(tapReader,NULL);

	closeTapDevice();

	return 0;
}
