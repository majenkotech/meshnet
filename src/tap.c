#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <ifaddrs.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "mesh.h"

unsigned long tapdev;
char tapname[16];
uint64_t myMAC;

pthread_t tapReader;

void getMyMacAddress()
{
    struct ifaddrs *addrlist, *addr;

    getifaddrs(&addrlist);

    for(addr = addrlist; addr!=NULL; addr=addr->ifa_next)
    {
		if(!strcmp(addr->ifa_name,tapname))
		{
			if(addr->ifa_addr->sa_family == AF_PACKET)
			{
				struct sockaddr_ll *ll = (struct sockaddr_ll *) addr->ifa_addr;
				myMAC = 0x0ULL;

                myMAC |= ((uint64_t)ll->sll_addr[0] << 40);
                myMAC |= ((uint64_t)ll->sll_addr[1] << 32);
                myMAC |= ((uint64_t)ll->sll_addr[2] << 24);
                myMAC |= ((uint64_t)ll->sll_addr[3] << 16);
                myMAC |= ((uint64_t)ll->sll_addr[4] << 8);
                myMAC |= ((uint64_t)ll->sll_addr[5] << 0);
			}
		}
    }
}

void *tapReaderThread(void *arg)
{
	uint8_t buffer[1600];
	uint8_t packet[1600];
	memset(buffer,0,1600);
	int n=read(tapdev,buffer,1550);
	while(n != -1) {
		uint64_t mac;
		memset(packet,0,1600);
		packet[0] = DT_DATA;
		memcpy(packet+1,buffer,n);

#ifdef DEBUG
                    printf("== Received frame from tap type %02x%02x==\n", buffer[12], buffer[13]);
                    if ((buffer[12] == 0x86) && (buffer[13] == 0xdd)) {
                        printf("Source:      %02x:%02x:%02x:%02x:%02x:%02x\n",
                            buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11]);
                        printf("Destination: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
                    }
#endif


		// Extract the destination MAC address from the packet header
		mac = 0x0ULL;
        
        mac |= (uint64_t)buffer[0] << 40;
        mac |= (uint64_t)buffer[1] << 32;
        mac |= (uint64_t)buffer[2] << 24;
        mac |= (uint64_t)buffer[3] << 16;
        mac |= (uint64_t)buffer[4] << 8;
        mac |= (uint64_t)buffer[5] << 0;

		netSend(mac,packet,n+1);

		memset(buffer,0,1600);
		n=tap_read(tapdev,buffer,1550);
	}
	return 0;
}

void startTapReader()
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tapReader, &attr, tapReaderThread, NULL);
}

void openTapDevice()
{
    tapname[0] = 0;
	tapdev = tap_open(tapname);
    if (tapdev >= 0) {
#ifdef DEBUG
        printf("Opened TAP device %s\n",tapname);
#endif
        getMyMacAddress();
#ifdef DEBUG
        printf("My MAC address: %012" PRIx64 "\n", myMAC);
#endif
    } else {
        printf("Unable to open TAP device: %s\n", strerror(errno));
    }
}

void closeTapDevice()
{
	tap_close(tapdev, tapname);
}

