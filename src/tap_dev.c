#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "mesh.h"

/* 
 * Allocate Ether TAP device, returns opened fd. 
 * Stores dev name in the first arg(must be large enough).
 */ 
int tap_open(char *dev)
{
    char tapname[14];
    int i, fd;
    struct ifreq ifr;

    if( dev ) {
        fd = open("/dev/net/tun", O_RDWR);
        if(fd)
        {
            memset(&ifr, 0, sizeof(ifr));
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

            if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
                close(fd);
                return -1;
            }

            strcpy(dev, ifr.ifr_name);
            return fd;
        }
    }
    return -1;
}

int tap_close(int fd, char *dev)
{
    return close(fd);
}

/* Write frames to TAP device */
int tap_write(int fd, uint8_t *buf, int len)
{
    return write(fd, buf, len);
}

/* Read frames from TAP device */
int tap_read(int fd, uint8_t *buf, int len)
{
    return read(fd, buf, len);
}
