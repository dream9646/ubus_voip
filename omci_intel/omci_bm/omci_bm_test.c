#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
//#include <net/route.h>
#include <net/ethernet.h>


#include "iphost.h"

/* omci_bm.c */
int omci_bm_init(void);
int omci_bm_getmac(unsigned char *mac);
int omci_bm_getinfo(unsigned char *info);

int
iphost_get_hwaddr(char *ifname, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int sockfd, i;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		fprintf(stderr, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// get hwaddr
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) <0) {
		fprintf(stderr, "%s getmac err: %d(%s)\n", ifname, errno, strerror(errno));
		close(sockfd);
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			hwaddr[i] = 0;		
		return -1;
	}
	close(sockfd);

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		hwaddr[i] = ifr.ifr_hwaddr.sa_data[i];
	return 0;
}

int
main(int argc, char **argv)
{
	unsigned char sig_mac[6] = {0};
	char sig_info[8] = {0};

	int ret;

	ret = omci_bm_init();
	printf("omci_bm_init() ret=%d\n", ret);	

	ret = omci_bm_getmac(sig_mac);
	printf("omci_bm_getmac() ret=%d\n", ret);
	if (ret == 0) {
		printf("sig_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			sig_mac[0],sig_mac[1],sig_mac[2],sig_mac[3],sig_mac[4],sig_mac[5]);
	}

	ret = omci_bm_getinfo(sig_info);
	printf("omci_bm_getinfo() ret=%d\n", ret);
	if (ret == 0) {
		printf("sig_info=%s\n", sig_info);
	}	

	{
		char ifname[8] = {0};
		unsigned char ifmac[6];
		int i;

		ifname[0] = 'p'; ifname[1] = 'o'; ifname[2] = 'n'; ifname[3] = '0';
		for (i=5; i>0; i--) {	// replace - with 0 so sig_info could be used as ifname
			if (sig_info[i] == '-') sig_info[i] = 0;
		}
		if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
			printf("sig_mac interface %s found\n", ifname);
		} else if (iphost_get_hwaddr(sig_info, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
			printf("sig_mac interface %s found\n", sig_info);
		} else {
			printf("sig_mac interface not found!\n");
		}
	}

	return 0;
}
