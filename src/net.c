#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <ifaddrs.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "mesh.h"

pthread_t netReader;

int readsocket;
int writesocket;

static int one = 1;

void printMac(uint64_t mac) {
	printf("%012" PRIx64 " = %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		mac,
		(uint8_t)((mac >> 40) & 0xFF),
		(uint8_t)((mac >> 32) & 0xFF),
		(uint8_t)((mac >> 24) & 0xFF),
		(uint8_t)((mac >> 16) & 0xFF),
		(uint8_t)((mac >> 8) & 0xFF),
		(uint8_t)((mac) & 0xFF));
}

void broadcastPacket(uint8_t *packet, unsigned long size)
{
	netSend((uint64_t)0xFFFFFFFFFFFFULL,packet,size);
}

void broadcastHosts()
{
	uint8_t packet[1600];
    uint8_t *ptr;
	int offset;
	uint8_t count;

	memset(packet,0,1600);

	packet[0] = DT_COMMAND;
	packet[1] = CMD_HOSTS;
	packet[2] = 0x00; // Number of hosts in this packet - to be filled in at send time;
	offset = 3;
	count = 0;

    struct host *scan;

    for (scan = hosts; scan; scan = scan->next) {
        if (scan->mac == myMAC) continue;
		count++;
		ptr = packet + offset;
		ptr[0] = (scan->mac & 0xFF0000000000ULL) >> 40;
		ptr[1] = (scan->mac & 0xFF00000000ULL) >> 32;
		ptr[2] = (scan->mac & 0xFF000000ULL) >> 24;
		ptr[3] = (scan->mac & 0xFF0000ULL) >> 16;
		ptr[4] = (scan->mac & 0xFF00ULL) >> 8;
		ptr[5] = (scan->mac & 0xFFULL);
		ptr[6] = (scan->ip & 0xFF000000) >> 24;
		ptr[7] = (scan->ip & 0xFF0000) >> 16;
		ptr[8] = (scan->ip & 0xFF00) >> 8;
		ptr[9] = (scan->ip & 0xFF);
		if(count==148) // Maximum number of hosts per packet
		{
			unsigned long size = count * 10 + 3;
			packet[2] = count;
			broadcastPacket(packet,size);
			memset(packet, 0, 1600);

			packet[0] = DT_COMMAND;
			packet[1] = CMD_HOSTS;
			packet[2] = 0x00;
			offset = 3;
			count = 0;
		}
		offset+=10;
	}
	if(count>0)
	{
		unsigned long size = count * 10 + 3;
		packet[2] = count;
		broadcastPacket(packet,size);
	}
}

void *netReaderThread(void *arg)
{
	struct sockaddr_in client;
	int clilen, readlen;
	uint8_t buffer[1600];
	uint8_t *payload;

	uint8_t part1[1600];
	uint8_t part2[1600];
	uint8_t all[3200];
	int part1len;
	int part2len;

	memset(buffer, 0, 1600);
	clilen = sizeof(client);

	readlen = aes_recvfrom(readsocket,buffer,1599,0,(struct sockaddr *) &client, &clilen);
	while(readlen>=0) {
        if (readlen > 0) {
            // Now to decide what to do with it.
            switch(*buffer) {
                case DT_PART1: {
                    // This is the first part of a split packet.
                    memset(part1, 0, 1600);
                    memcpy(part1,buffer+1,readlen-1);
                    part1len = readlen-1;
                } break;
                case DT_PART2: {
                    // This is the second part of a split packet.
                    memset(part2, 0, 1600);
                    memcpy(part2,buffer+1,readlen-1);
                    part2len = readlen-1;


                    memset(all, 0, 3200);
                    memcpy(all,part1,part1len);
                    memcpy(all+part1len,part2,part2len);
                    if (write(tapdev,all,part1len+part2len) <= 0) {
                        printf("Write error sending packet\n");
                    }
                } break;
                case DT_DATA: {
                    // This is a data packet.  Forward it to the TAP device.
                    payload = buffer+1;
                    if (write(tapdev,payload,readlen-1) <= 0) {
                        printf("Write error sending packet\n");
                    }
                } break;
                case DT_COMMAND: {
                    // This is a command packet.  Do different things depending on the command type.
                    switch(*(buffer+1)) {
                        case CMD_ANNOUNCE: {
                            uint64_t mac;
                            mac = 0x0ULL;
                            // This is a peer announcing itself.
                            mac |= (uint64_t)buffer[2] << 40;
                            mac |= (uint64_t)buffer[3] << 32;
                            mac |= (uint64_t)buffer[4] << 24;
                            mac |= (uint64_t)buffer[5] << 16;
                            mac |= (uint64_t)buffer[6] << 8;
                            mac |= (uint64_t)buffer[7] << 0;
                            if (mac == myMAC) break;
                            if(setHost(mac,client.sin_addr.s_addr))
                            {
                                // Broadcast Hosts Table
                                broadcastHosts();
                            }
                        } break;
                        case CMD_HOSTS: {
                            int modified = 0;
                            uint8_t count = *(buffer+2);
                            uint8_t *ptr;
                            int i;
                            for(i=0; i<count; i++)
                            {
                                uint64_t mac;
                                uint32_t ip;
                                ptr = buffer + 3 + (i*10);
                                mac = 0x0ULL;
                                mac |= (uint64_t)ptr[0] << 40;
                                mac |= (uint64_t)ptr[1] << 32;
                                mac |= (uint64_t)ptr[2] << 24;
                                mac |= (uint64_t)ptr[3] << 16;
                                mac |= (uint64_t)ptr[4] << 8;
                                mac |= (uint64_t)ptr[5] << 0;
                                ip = 0x0;
                                ip |= (uint32_t)ptr[6] << 24;
                                ip |= (uint32_t)ptr[7] << 16;
                                ip |= (uint32_t)ptr[8] << 8;
                                ip |= (uint32_t)ptr[9] << 0;

                                if(setHost(mac,ip))
                                {
                                    modified = 1;
                                }
                            }
                            if(modified == 1)
                            {
                                broadcastHosts();
                            }
                        } break;
                    }
                } break;
            }
            readlen = aes_recvfrom(readsocket,buffer,1599,0,(struct sockaddr *) &client, &clilen);
        }
    }
	return 0;
}

void startNetReader()
{
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&netReader, &attr, netReaderThread, NULL);
}

int ipSend(uint32_t ip, uint8_t *packet, unsigned long length)
{
	int rc;
	struct sockaddr_in remoteServer;
	remoteServer.sin_family = AF_INET;
	remoteServer.sin_port = htons(config.port);
	remoteServer.sin_addr.s_addr = ip;

	if(*packet==DT_DATA && length>1400)
	{
		int rc1,rc2;
		// Split the packet into 2 parts

		uint8_t part1[1600];
		memset(part1, 0, 1600);
		part1[0] = DT_PART1;
		memcpy(&part1[1],packet+1,1400);
		rc1 = aes_sendto(writesocket, part1, 1401, 0, (struct sockaddr *) &remoteServer, sizeof(remoteServer));

		uint8_t part2[1600];
		memset(part2, 0, 1600);
		part2[0] = DT_PART2;
		memcpy(&part2[1],packet+1401,length-1401);
		rc2 = aes_sendto(writesocket, part2, length-1400, 0, (struct sockaddr *) &remoteServer, sizeof(remoteServer));

		rc = rc1+rc2;
		return rc;
	}
	rc = aes_sendto(writesocket, packet, length, 0, (struct sockaddr *) &remoteServer, sizeof(remoteServer));
	return rc;
}

int netSend(uint64_t mac, uint8_t *packet, unsigned long length)
{
	unsigned long destination;
	int count;
    struct host *scan;

	// Broadcast FF:FF:FF:FF:FF:FF
	if(mac == 0xFFFFFFFFFFFFULL) {

        for (scan = hosts; scan; scan = scan->next) {
            if (scan->mac == myMAC) continue;
			if(ipSend(scan->ip,packet,length)>0) {
				count++;
			}
		}
		return count;
	} else {
        if (mac != myMAC) {
            uint32_t ip = getHost(mac);
            if (ip != -1) {
                return ipSend(ip,packet,length);
            } else {
                return -1;
            }
        } else {
            return -1;
        }
	}
}

int initNetwork() {
	struct sockaddr_in reader, writer;

    hosts = NULL;

	// First open the reading socket

	readsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(!readsocket) {
        printf("Error opening network read socket: %s\n", strerror(errno));
		return -1;
	}
	
	reader.sin_family = AF_INET;
	reader.sin_addr.s_addr = htonl(INADDR_ANY);
	reader.sin_port = htons(config.port);
	if(bind(readsocket, (struct sockaddr *) &reader, sizeof(reader))<0) {
        printf("Error binding network read socket: %s\n", strerror(errno));
		return -1;
	}

    setsockopt(readsocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

	// Read socket should now be ready for action;

	writesocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(!writesocket) {
        printf("Error opening network write socket: %s\n", strerror(errno));
		return -1;
	}
	
	writer.sin_family = AF_INET;
	writer.sin_addr.s_addr = htonl(INADDR_ANY);
	writer.sin_port = htons(0);

	if(bind(writesocket, (struct sockaddr *) &writer, sizeof(writer)) < 0) {
        printf("Error binding network write socket: %s\n", strerror(errno));
		return -1;
	}

    setsockopt(writesocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
    return 0;
}

uint32_t aton(const char *addr)
{
	struct sockaddr_in host;
	if(inet_aton(addr,&host.sin_addr) == 0)
	{
		return -1;
	}
	return host.sin_addr.s_addr;
}

char *ntoa(uint32_t ip)
{
	struct sockaddr_in host;
	host.sin_addr.s_addr=ip;
	return inet_ntoa(host.sin_addr);
}

void announceMe(uint32_t ip)
{
	uint8_t *packet;

	// Packet format: 0x01 0x00 M0 M1 M2 M3 M4 M5 <PSK>
	//                          \- MAC Address -/
	
	packet = (uint8_t *)malloc(strlen(config.psk)+9);
	memset(packet, 0, strlen(config.psk)+9);

	packet[0] = DT_COMMAND; // Command data type
	packet[1] = CMD_ANNOUNCE; // I am announcing myself to you
	packet[2] = (myMAC & 0xFF0000000000ULL) >> 40;
	packet[3] = (myMAC & 0xFF00000000ULL) >> 32;
	packet[4] = (myMAC & 0xFF000000ULL) >> 24;
	packet[5] = (myMAC & 0xFF0000ULL) >> 16;
	packet[6] = (myMAC & 0xFF00ULL) >> 8;
	packet[7] = (myMAC & 0xFFULL);
    memcpy(packet + 8, config.psk, strlen(config.psk));

	// We're not interested in sending the trailing 0x00
	if(ipSend(ip,packet,strlen(config.psk)+8)<=0) {
        printf("Error sending announcement\n");
	}

	// Don't forget to free the packet memory now we're done with it.
	free(packet);
}

uint64_t str2mac(char *address) {
	unsigned int m1,m2,m3,m4,m5,m6;
	uint64_t mac = 0x0ULL;
	if(sscanf(address,"%02X:%02X:%02X:%02X:%02X:%02X",
		&m1,&m2,&m3,&m4,&m5,&m6) == 6)
	{
		mac |= (uint64_t)m1 << 40;
		mac |= (uint64_t)m2 << 32;
		mac |= (uint64_t)m3 << 24;
		mac |= (uint64_t)m4 << 16;
		mac |= (uint64_t)m5 << 8;
		mac |= (uint64_t)m6 << 0;
	}
	return mac;
}

char *mac2str(uint64_t mac)
{
	static char address[19];
	sprintf(address,"%02X:%02X:%02X:%02X:%02X:%02X\n",
		(uint8_t)((mac >> 40) & 0xFF),
		(uint8_t)((mac >> 32) & 0xFF),
		(uint8_t)((mac >> 24) & 0xFF),
		(uint8_t)((mac >> 16) & 0xFF),
		(uint8_t)((mac >> 8) & 0xFF),
		(uint8_t)((mac) & 0xFF));
	return address;
}
