#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>

#include "mesh.h"

int masterTerminate = 0;

struct configuration config;

void closedown();

void threadexit(int signal) {
	// Do nothing much
}

void cleanexit(int signal) {
#ifdef DEBUG
	printf("Terminating.\n");
#endif
    closedown();
//	pthread_kill(tapReader,SIGINT);
//	pthread_kill(netReader,SIGINT);
//	closeTapDevice();
	exit(0);
}

void reap(int signal) {
    int wstatus;
    waitpid(-1, &wstatus, WNOHANG);
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

    config.ifup[0] = 0;
    config.ifdown[0] = 0;
    config.psk[0] = 0;
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

            if (strcasecmp(key, "ifup") == 0) {
                strcpy(config.ifup, val);
                continue;
            }
            
            if (strcasecmp(key, "ifdown") == 0) {
                strcpy(config.ifdown, val);
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

            if (strcasecmp(key, "fork") == 0) {
                if (strcasecmp(val, "yes") == 0) {
                    config.fork = 1;
                }
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

void closedown() {
    if (config.ifdown[0]) {
        pid_t down = fork();
        if (!down) {
            char p[7];
            sprintf(p, "%d", config.port);
            execl(config.ifdown, "ifdown-meshnet", tapname, p, NULL);
            return;
        } else {
            int status = 0;
#ifdef DEBUG
            printf("Waiting for ifdown process\n");
#endif
            while (wait(&status) > 0);
#ifdef DEBUG
            printf("Done\n");
#endif
        }
    }

	closeTapDevice();
}

int main(int argc, char *argv[]) {
    int i;

	signal(SIGINT,  &cleanexit);
	signal(SIGKILL, &cleanexit);
	signal(SIGTERM, &cleanexit);
    signal(SIGCHLD, &reap);

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
		setHost(myMAC, config.address, config.port, 1, 1);
	}

    if (config.ifup[0]) {
        if (!fork()) {
            char p[7];
            sprintf(p, "%d", config.port);
            execl(config.ifup, "ifup-meshnet", tapname, p, NULL);
            return 0;
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
    pthread_join(netReader, NULL);
    pthread_join(periodic, NULL);

    closedown();

	return 0;
}
