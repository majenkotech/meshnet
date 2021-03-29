#include <pthread.h>

#include "mesh.h"

pthread_t periodic;

// Things that need to be done periodically
void *periodicThread(void *arg)
{
    int i;
	while(1)
	{
		sleep(1);
		// Once a minute - announce me to everyone I know of
		if((time(NULL) % config.announce) == 0)
		{
            dumpMap();
            dbg_printf("Running periodic announcement\n");
            int i,j;
            for (i = 0; i < MAX_HOSTS; i++) {
                if (hosts[i].valid == 0) continue;
                if (hosts[i].mac == myMAC) continue;
                int ispeer = 0;
                for (j = 0; (j < MAX_PEERS) && (config.peerips[j] != -1); j++) {
                    if ((config.peerips[j] == hosts[i].ip) && (config.peerports[j] == hosts[i].port)) {
                        ispeer = 1;
                        break;
                    }
                }
                if (!ispeer) {
                    announceMe(hosts[i].ip, hosts[i].port);
                }
			}

            for (i = 0; (i < MAX_PEERS) && (config.peerips[i] != -1); i++) {
                announceMe(config.peerips[i], config.peerports[i]);
            }

		}
	}
}

void startPeriodic()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&periodic, &attr, periodicThread, NULL);
}

