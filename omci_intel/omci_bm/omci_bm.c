#include <stdio.h>
#include <stdlib.h>
#include "bm_if.c"

static char omci_bm_mac[6] = {0};
static char omci_bm_info[8] = {0};

// if omci_bm_init is never called			omci_bm_mac will be 00:00:00:00:00:00
// if omci_bm_init is called, but signature error	omci_bm_mac will be 88:62:27:88:81:18
// if omci_bm_init is called, and signature ok		omci_bm_mac will be signatured_macaddr
int
omci_bm_init(void)
{
	char *buf_sign = NULL;
	int buf_size = 0;
	
	omci_bm_mac[0]=0x88;
	omci_bm_mac[1]=0x62;
	omci_bm_mac[2]=0x27;
	omci_bm_mac[3]=0x88;
	omci_bm_mac[4]=0x81;
	omci_bm_mac[5]=0x18;	
	
	omci_bm_info[0]='5';   
	omci_bm_info[1]='v';   
	omci_bm_info[2]='t';   
	omci_bm_info[3]='e';   
	omci_bm_info[4]='c';   
	omci_bm_info[5]='h';   
	
	return bm_init(buf_sign, buf_size);
}

int
omci_bm_getmac(unsigned char *mac)
{
	int ret=bm_getmac(omci_bm_mac);		// overwritten only if bm_init successed
	memcpy(mac, omci_bm_mac, 6);
	return ret;
}

int
omci_bm_getinfo(unsigned char *info)
{
	int ret=bm_getinfo(omci_bm_info);	// overwritten only if bm_init successed
	strcpy(info, omci_bm_info);
	return ret;
}
