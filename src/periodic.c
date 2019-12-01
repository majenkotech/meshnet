#include <pthread.h>

#include "mesh.h"

pthread_t periodic;

// Things that need to be done periodically
void *periodicThread(void *arg)
{
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
            for (scan = hosts; scan; scan = scan->next) {
                if (scan->mac == myMAC) continue;
				announceMe(scan->ip, scan->port);
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

