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
#ifdef DEBUG
            dumpMap();
            printf("Running periodic announcement\n");
#endif
            struct host *scan;
            int i;
            for (i = 0; i < MAX_HOSTS; i++) {
                if (hosts[i].valid == 0) continue;
                if (hosts[i].mac == myMAC) continue;
				announceMe(hosts[i].ip, hosts[i].port);
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

