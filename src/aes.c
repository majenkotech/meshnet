#include <mcrypt.h>
#include <mhash.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mesh.h"

MCRYPT mcb;

int aes_init(char *psk)
{
	int maxkeylen;
	char *key;
	char *IV;
	int i;

	mcb = mcrypt_module_open((char *)"rijndael-128",NULL,(char *)"ecb",NULL);
	if(mcb == MCRYPT_FAILED)
	{
		printf("Error initializing encryption\n");
		return -1;
	}

	maxkeylen = mcrypt_enc_get_key_size(mcb);
	key = (char *)malloc(maxkeylen+1);
	mhash_keygen(KEYGEN_MCRYPT,MHASH_MD5,1,(uint8_t*)key,maxkeylen,NULL,0,(uint8_t*)psk,strlen(psk));

	IV = (char *)malloc(mcrypt_enc_get_iv_size(mcb));
	for (i=0; i< mcrypt_enc_get_iv_size(mcb); i++) {
		IV[i]=rand();
	}
	i=mcrypt_generic_init(mcb, key, maxkeylen, IV);
	if (i<0) {
		mcrypt_perror(i);
		return -1;
	}
    dbg_printf("Encryption system initialized\n");
	return 0;
}

int aes_sendto(int socket, uint8_t *buffer, int size, int flags, struct sockaddr *remoteServer, int remoteLen)
{
	uint8_t outbuffer[1600];
	uint8_t *paste;
	int rv,erv;
	int esize;
	int blocksize = mcrypt_enc_get_block_size(mcb);

	if(size % blocksize == 0)
	{
		esize = size;
	} else {
		esize = ((size / blocksize) + 1) * blocksize;
	}

	outbuffer[0] = (size & 0xFF00UL) >> 8;
	outbuffer[1] = size & 0xFFUL;


	paste = outbuffer+2;

	memcpy(paste,buffer,size);
	erv = mcrypt_generic(mcb,paste,esize);
	rv = sendto(socket,outbuffer,esize+2,flags,remoteServer,remoteLen);
	return rv;
}

int aes_recvfrom(int socket, uint8_t *buffer,int size,int flags,struct sockaddr *client, int *clilen)
{
	int rv,drv;
    uint8_t inbuffer[1600];
	uint16_t osize;

	rv = recvfrom(socket,inbuffer,1600,flags,client,(socklen_t *)clilen);
	if(!rv)
	{
		return -1;
	}

	osize = 0;
	osize |= inbuffer[0] << 8;
	osize |= inbuffer[1];
	if(osize > 1498)
	{
		// Someone is having us on...!
		return 0;
	}
	drv = mdecrypt_generic(mcb,inbuffer+2,rv-2);
    if (drv != 0) {
        return 0;
    }
	memcpy(buffer,inbuffer+2,osize);
	return osize;
}

