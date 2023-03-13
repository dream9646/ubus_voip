#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <net/ethernet.h>

#include "util.h"
#include "conv.h"
#include "regexp.h"
#include "util_inet.h"
#include "iphost.h"

// ip addr in .s_addr is in network order,
// ip used in omci attr_set() is local order,
// ip in omci mib/msg is network order
char *
iphost_inet_string(int ip)
{
	static char str[8][20];	// trick to allow re-entry
	static int i;
	struct in_addr addr = { .s_addr = ip };
	
	i = (i+1)%8;	
	strncpy(str[i], inet_ntoa(addr), 19);
	return str[i];
}

int
iphost_set_hostname(char *hostname)
{
	if (sethostname(hostname, strlen(hostname)) <0) {
		dbprintf(LOG_ERR, "sethostname %s err: %d(%s)\n", hostname, errno, strerror(errno));
		return -1;
	}
	return 0;
}
int
iphost_get_hostname(char *hostname, int len)
{
	if (gethostname(hostname, len) <0) {
		dbprintf(LOG_ERR, "gethostname err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	return 0;
}

// interface related /////////////////////////////////////////////////////////////////////////

// per ifname_list shoyld be an array of [17], eg: char iflist[list_size][17]
int 
iphost_get_interface_list(char *ifname_list[], int ifname_list_size)
{
	int sockfd, total_found, total_copied, i, j;
	struct ifconf ifc;
	struct ifreq ifr[32];

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	/* Get the all interfaces information */
	ifc.ifc_req = ifr;
	ifc.ifc_len = (32 * sizeof(struct ifreq));
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
		dbprintf(LOG_ERR, "get iflist err: %d(%s)\n", errno, strerror(errno));
		close(sockfd);
		return -1;
	}
	close(sockfd);
	
	//get ifname list
	total_found = ifc.ifc_len / sizeof(struct ifreq);
	total_copied = 0;
	for (i = 0; i < total_found; i++) {
		int duplicated = 0;
		for (j=0; j< total_copied; j++) {
			if (strcmp(ifc.ifc_req[i].ifr_name, ifname_list[j]) == 0) {
				duplicated = 1;
				break;
			}
		}
		if (!duplicated) {
			strcpy(ifname_list[total_copied], ifc.ifc_req[i].ifr_name);
			total_copied++;
			if (total_copied >= ifname_list_size)
				break;
		}
	}
	return total_copied;
}

// ret: -1: err, 0: not exist, 1:exist
int
iphost_interface_is_exist(char *ifname)
{
	struct ifreq ifr;
	int sockfd, ret;

	if (!ifname)
		return 0;
	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		return -1;
	}
	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;
	// get interface status
	ret = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	close(sockfd);

	if (ret<0)
		return 0;
	return 1;
}

int
iphost_set_interface_updown(char *ifname, int is_up)
{
	struct ifreq ifr;
	int sockfd;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// get inetrface status, then config up/down
	ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	if (is_up) {
		ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	} else {
		ifr.ifr_flags &= (~(IFF_UP | IFF_RUNNING));
	}
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
		dbprintf(LOG_ERR, "%s %s err: %d(%s)\n", ifname, is_up?"ifup":"ifdown", errno, strerror(errno));
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}
int
iphost_get_interface_updown(char *ifname, int *is_up)
{
	struct ifreq ifr;
	int sockfd;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// get interface status
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
		dbprintf(LOG_INFO, "%s getflag err: %d(%s)\n", ifname, errno, strerror(errno));
		close(sockfd);
		*is_up = 0;
		return -1;
	}

	close(sockfd);
	*is_up = (ifr.ifr_flags & (IFF_UP|IFF_RUNNING))? 1: 0;
	return 0;
}

int
iphost_set_hwaddr(char *ifname, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int sockfd, i;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// config macaddr
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		ifr.ifr_hwaddr.sa_data[i] = hwaddr[i];
	if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0) {
		dbprintf(LOG_ERR, "%s setmac %s err: %d(%s)\n", 
			ifname, util_str_macaddr(hwaddr), errno, strerror(errno));
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}
int
iphost_get_hwaddr(char *ifname, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int sockfd, i;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// get hwaddr
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) <0) {
		dbprintf(LOG_ERR, "%s getmac err: %d(%s)\n", ifname, errno, strerror(errno));
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


// ipv4 related /////////////////////////////////////////////////////////////////////////

int
iphost_set_ipv4_ip_netmask(char *ifname, int ip_addr, int netmask_addr)
{
	struct ifreq ifr;
	struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
	int sockfd;

	if (!ifname)
		return -1;

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// cofig ipaddr
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = ip_addr;
	if (ioctl(sockfd, SIOCSIFADDR, &ifr) <0) {
		dbprintf(LOG_ERR, "%s setip %s err: %d(%s)\n", ifname, iphost_inet_string(ip_addr), errno, strerror(errno));
		close(sockfd);
		return -1;
	}
	// cofig netmask
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = netmask_addr;
	if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) <0) {
		// set netmask will return err if ipaddr is 0.0.0.0, however, the set stillworks
		// so we only return error if ipaddr is not 0.0.0.0
		if (ip_addr) {
			dbprintf(LOG_ERR, "%s setmask %s err: %d(%s)\n", ifname, iphost_inet_string(netmask_addr), errno, strerror(errno));
			close(sockfd);
			return -1;
		}
	}
#if 0
	// bring up interface, not sure if this is required to trigger gratuitous arp 
	ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
		dbprintf(LOG_ERR, "%s ifup err: %d(%s)\n", ifname, errno, strerror(errno));
		close(sockfd);
		return -1;
	}
#endif
	close(sockfd);
	return 0;
}
int
iphost_get_ipv4_ip_netmask(char *ifname, int *ip_addr, int *netmask_addr, int *is_up)
{
	struct ifreq ifr;
	struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
	int sockfd;

	if (!ifname)
		return -1;

	*ip_addr = *netmask_addr = *is_up = 0;	// default 0 for all return value

	/* create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		dbprintf(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	// set ifname & family
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	// get interface status
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0) {
		*is_up = (ifr.ifr_flags & (IFF_UP|IFF_RUNNING))? 1: 0;
	} else {
		dbprintf(LOG_INFO, "%s getflag err: %d(%s)\n", ifname, errno, strerror(errno));
		close(sockfd);
		return -1;
	}

	// if get interface status is okay, which means the interface does exist
	// then we ignore EADDRNOTAVAIL error in getip/getnetmask, as which means addr=0.0.0.0

	// get netmask
	addr->sin_family = AF_INET;
	if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) == 0) {
		*netmask_addr = addr->sin_addr.s_addr;
	} else if (errno != EADDRNOTAVAIL) {
		dbprintf(LOG_ERR, "%s getmask err: %s(%d)\n", ifname, strerror(errno));
		close(sockfd);
		return -1;
	}

	// get ipaddr
	addr->sin_family = AF_INET;
	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
		*ip_addr = addr->sin_addr.s_addr;
	} else if (errno != EADDRNOTAVAIL) {
		dbprintf(LOG_ERR, "%s getip err: %d(%s)\n", ifname, errno, strerror(errno));
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}

int
iphost_add_ipv4_route(int dstnet_addr, int genmask_addr, int gateway_addr)	// addr in network order
{
	struct rtentry route;
	struct sockaddr_in *addr;
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <0) {
		dbprintf(LOG_ERR, "create socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	memset(&route, 0, sizeof(route));

	addr = (struct sockaddr_in *)&route.rt_gateway;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = gateway_addr;

	addr = (struct sockaddr_in *)&route.rt_dst;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = dstnet_addr;

	addr = (struct sockaddr_in *)&route.rt_genmask;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = genmask_addr;

	route.rt_flags = RTF_UP;
	if (dstnet_addr == 0)	// default route
		route.rt_flags |= RTF_GATEWAY;
	route.rt_metric = 0;

	if (ioctl(sockfd, SIOCADDRT, &route) < 0) {
		dbprintf(LOG_ERR, "ioctl addrt %s/%s-%s err: %d(%s)\n", 
			iphost_inet_string(dstnet_addr),
			iphost_inet_string(genmask_addr),
			iphost_inet_string(gateway_addr),
			errno, strerror(errno));
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}
int
iphost_del_ipv4_route(int dstnet_addr, int genmask_addr)
{
	struct rtentry route;
	struct sockaddr_in *addr;
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <0) {
		dbprintf(LOG_ERR, "create socket err: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	memset(&route, 0, sizeof(route));

	addr = (struct sockaddr_in *)&route.rt_dst;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = dstnet_addr;

	addr = (struct sockaddr_in *)&route.rt_genmask;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = genmask_addr;

	if (ioctl(sockfd, SIOCDELRT, &route) < 0) {
		close(sockfd);
		if (dstnet_addr != 0) {  // dont show err if del default route
			dbprintf(LOG_ERR, "ioctl delrt %s/%s err: %d(%s)\n", 
				iphost_inet_string(dstnet_addr),
				iphost_inet_string(genmask_addr),
				errno, strerror(errno));
		}
		return -1;
	}

	close(sockfd);
	return 0;
}
/*
Iface	Destination	Gateway 	Flags	RefCnt	Use	Metric	Mask		MTU	Window	IRTT
ppp0	C9C8140A	00000000	0005	0	0	0	FFFFFFFF	0	0	0
eth0	4968A376	FE01A8C0	0007	0	0	0	FFFFFFFF	0	0	0
eth0	0001A8C0	00000000	0001	0	0	0	00FFFFFF	0	0	0
eth1	0038A8C0	00000000	0001	0	0	0	00FFFFFF	0	0	0
ppp0	0000140A	00000000	0001	0	0	0	0000FFFF	0	0	0
eth0	0000FEA9	00000000	0001	0	0	1002	0000FFFF	0	0	0
eth1	0000FEA9	00000000	0001	0	0	1003	0000FFFF	0	0	0
eth0	00000000	FE01A8C0	0003	0	0	0	00000000	0	0	0
*/
int
iphost_get_ipv4_route(int dstnet_addr, int genmask_addr, int *gateway_addr)
{
	char line[256];
	char *proc_route="/proc/net/route";
	FILE *fp = fopen(proc_route, "r");

	if (fp == NULL) {
		dbprintf(LOG_ERR, "%s read err: %d(%s)\n", proc_route, errno, strerror(errno));
		return -1;
	}

	fgets(line, 256, fp);	// skip header line
	while (fgets(line, 256, fp)!=NULL) {
		char iface[20];
		unsigned long dst, gate, mask;
		int flags, refcnt, use, metric, mtu, window, rtt;
		int n;

		n = sscanf(line, "%s %lX %lX %x %d %d %d %lX %d %d %d",
			   iface, &dst, &gate, &flags, &refcnt, &use, &metric, &mask, &mtu, &window, &rtt);
		if (n != 11) {
			dbprintf(LOG_ERR, "route parsing err: %s\n", line);
			continue;
		}
		if (!(flags & RTF_UP))
			continue;
		if (!(flags & RTF_GATEWAY))
			continue;
		if (dst == dstnet_addr && mask == genmask_addr) {
			fclose(fp);
			*gateway_addr = gate;
			return 1;
		}
	}
	
	fclose(fp);
	*gateway_addr = 0;
	return 0;
}

int
iphost_set_ipv4_defgw(int gateway_addr)
{
	int i;
	for (i=0; i<10; i++) {
		if (iphost_del_ipv4_route(0, 0) <0)
			break;
	}
	return iphost_add_ipv4_route(0, 0, gateway_addr);
}
int
iphost_get_ipv4_defgw(int *gateway_addr)
{
	if (iphost_get_ipv4_route(0, 0, gateway_addr)==0)	// not found
		return -1;
	return 0;
}

int
iphost_set_ipv4_dns(char *resolv_conf, char *search_domain, int dns1_addr, int dns2_addr)
{
	FILE *fp = fopen(resolv_conf, "w");

	if (fp == NULL) {
		dbprintf(LOG_ERR, "%s open err: %d(%s)\n", resolv_conf, errno, strerror(errno));
		return -1;
	}
	if (search_domain)
		fprintf(fp, "search %s\n", search_domain);
	if (dns1_addr)
		fprintf(fp, "nameserver %s\n", iphost_inet_string(dns1_addr));
	if (dns2_addr)
		fprintf(fp, "nameserver %s\n", iphost_inet_string(dns2_addr));
	fclose(fp);

	return 0;
}
int
iphost_get_ipv4_dns(char *resolv_conf, int *dns1_addr, int *dns2_addr)
{
	struct regexp_context_t re_dns;
	char *output=NULL;
	int addr;

	*dns1_addr = *dns2_addr = 0;
	
	if (util_readfile(resolv_conf, &output) <=0)
		return-1;

	if (!regexp_is_compiled(&re_dns))
		regexp_prepare(&re_dns, "nameserver[ \t]+([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");

	if (regexp_match(&re_dns, output) == 0) {
		regexp_release(&re_dns);
		free_safe(output);
		return -1;
	}

	addr = inet_addr(regexp_match_str(&re_dns, 1));
	if (addr != inet_addr("127.0.0.1")) {
		*dns1_addr = addr;
	}
	while (regexp_nextmatch(&re_dns)) {
		addr = inet_addr(regexp_match_str(&re_dns, 1));
		if (addr != inet_addr("127.0.0.1")) {
			if (*dns1_addr == 0) {
				*dns1_addr = addr;
			} else if (*dns2_addr == 0) {
				*dns2_addr = addr;
				break;
			}
		}
	}
		
	regexp_release(&re_dns);
	free_safe(output);
	return 0;
}

// ipv4/6 related /////////////////////////////////////////////////////////////////////////

// struct ifaddrs {
//     struct ifaddrs  *ifa_next;    /* Next item in list */
//     char            *ifa_name;    /* Name of interface */
//     unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
//     struct sockaddr *ifa_addr;    /* Address of interface */
//     struct sockaddr *ifa_netmask; /* Netmask of interface */
//     union {
//         struct sockaddr *ifu_broadaddr;
//                          /* Broadcast address of interface */
//         struct sockaddr *ifu_dstaddr;
//                          /* Point-to-point destination address */
//     } ifa_ifu;
// #define              ifa_broadaddr ifa_ifu.ifu_broadaddr
// #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
//     void            *ifa_data;    /* Address-specific data */
// };
int
iphost_get_ifaddrs(char *ifname, struct iphost_ifaddrs_t *iph)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_in *in;
	struct sockaddr_in6 *in6;

	memset(iph, 0, sizeof(struct iphost_ifaddrs_t));

	if (getifaddrs(&ifaddr) == -1)
		return -1;

	// walk through linked list, maintaining head pointer so we can free list later
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		if (strcmp(ifname, ifa->ifa_name) !=0)
			continue;

		// interface flags is a dev->flags in kernel, 
		// a device might have no ip4/ip6 addr, but still exist, and if_flags would be available
		iph->if_flags = ifa->ifa_flags;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			in = (struct sockaddr_in*) ifa->ifa_addr;
			iph->in_addr[iph->ipv4_total] = in->sin_addr;
			in = (struct sockaddr_in*) ifa->ifa_netmask;
			iph->in_netmask[iph->ipv4_total] = in->sin_addr;
			iph->ipv4_total++;
		
		} else if (ifa->ifa_addr->sa_family == AF_INET6) {
			in6 = (struct sockaddr_in6*) ifa->ifa_addr;		
			iph->in6_addr[iph->ipv6_total] = in6->sin6_addr;
			in6 = (struct sockaddr_in6*) ifa->ifa_netmask;
			iph->in6_netmask[iph->ipv6_total] = in6->sin6_addr;
			iph->ipv6_total++;
		}
	}

	freeifaddrs(ifaddr);
	return (iph->ipv4_total + iph->ipv6_total);
}

int
iphost_get_masklen_ipv4(struct in_addr *ipv4_mask)
{
	unsigned char *s = (void *)&ipv4_mask->s_addr;
	int i, j;
	// locate the first bit that is 0
	for (i=0; i<4; i++) {
		if (s[i] == 0xff)
			continue;
		for (j=0; j<7; j++) {
			if ((s[i] & (1<<(7-j))) == 0)
				return i*8+j;
		}
	}
	return 32;
}

int
iphost_get_masklen_ipv6(struct in6_addr *ipv6_mask)
{
	unsigned char *s = (void *)&ipv6_mask->s6_addr;
	int i, j;
	// locate the first bit that is 0
	for (i=0; i<16; i++) {
		if (s[i] == 0xff)
			continue;
		for (j=0; j<7; j++) {
			if ((s[i] & (1<<(7-j))) == 0)
				return i*8+j;
		}
	}
	return 128;
}

int
iphost_get_ipv6_dns(char *resolv_conf, struct in6_addr *dns1_addr, struct in6_addr *dns2_addr)
{
	struct regexp_context_t re_dns;
	char *output=NULL;
	int ret1 = -1, ret2 = -1;

	memset(dns1_addr, 0, sizeof(struct in6_addr));
	memset(dns2_addr, 0, sizeof(struct in6_addr));

	if (util_readfile(resolv_conf, &output) <=0)
		return-1;

	if (!regexp_is_compiled(&re_dns))
		regexp_prepare(&re_dns, "nameserver[ \t]+([0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f:]*)");

	if (regexp_match(&re_dns, output) == 0) {
		regexp_release(&re_dns);
		free_safe(output);
		return -1;
	}

	ret1 = util_inet_pton(AF_INET6, (void*)regexp_match_str(&re_dns, 1), dns1_addr);
	while (regexp_nextmatch(&re_dns)) {
		if (ret1 <0)
			ret1 = util_inet_pton(AF_INET6, (void*)regexp_match_str(&re_dns, 1), dns1_addr);
		else
			ret2 = util_inet_pton(AF_INET6, (void*)regexp_match_str(&re_dns, 1), dns2_addr);
		if (ret2 == 0)
			break;
	}
	regexp_release(&re_dns);
	free_safe(output);
	return 0;
}


// test main //////////////////////////////////////////////////////////////////////////////////////

#if 0
int
main(int argc, char **argv)
{
#if 0
	{
		int ip_addr, mask_addr, defgw_addr, is_up; 	
		iphost_set_ipv4_ip_netmask("pon0", inet_addr("192.168.1.100"), inet_addr("255.255.255.0"));		
		iphost_set_ipv4_defgw(inet_addr("192.168.1.251"));
		if (iphost_get_ipv4_ip_netmask("pon0", &ip_addr, &mask_addr, &is_up) ==0)
			printf("ip=%s, mask=%s, up=%d\n", iphost_inet_string(ip_addr), iphost_inet_string(mask_addr), is_up);
		if (iphost_get_ipv4_defgw(&defgw_addr) ==0)
			printf("defgw=%s\n", iphost_inet_string(defgw_addr));	
		iphost_set_ipv4_ip_netmask("pon0", inet_addr("192.168.2.200"), inet_addr("255.255.240.0"));
		iphost_set_ipv4_defgw(inet_addr("192.168.2.252"));
		if (iphost_get_ipv4_ip_netmask("pon0", &ip_addr, &mask_addr, &is_up) ==0)
			printf("ip=%s, mask=%s, up=%d\n", iphost_inet_string(ip_addr), iphost_inet_string(mask_addr), is_up);
		if (iphost_get_ipv4_defgw(&defgw_addr) ==0)
			printf("defgw=%s\n", iphost_inet_string(defgw_addr));
	}

	{
		unsigned char hwaddr[6];	
		if (conv_str_mac_to_mem((char*)hwaddr, "00:02:27:83:11:44") == 0)
			iphost_set_hwaddr("pon0", hwaddr);
		if (iphost_get_hwaddr("pon0", hwaddr) ==0)
			printf("hwaddr=%02x:%02x:%02x:%02x:%02x:%02x\n",
				hwaddr[0], hwaddr[1], hwaddr[2],
				hwaddr[3], hwaddr[4], hwaddr[5]);		
		if (conv_str_mac_to_mem((char*)hwaddr, "00:02:27:83:22:33") == 0)
			iphost_set_hwaddr("pon0", hwaddr);
		if (iphost_get_hwaddr("pon0", hwaddr) ==0)
			printf("hwaddr=%02x:%02x:%02x:%02x:%02x:%02x\n",
				hwaddr[0], hwaddr[1], hwaddr[2],
				hwaddr[3], hwaddr[4], hwaddr[5]);
	}

	{
		char hostname[128];
		iphost_set_hostname("test1.com");
		if (iphost_get_hostname(hostname, 128)==0)
			printf("hostname=%s\n", hostname);
		iphost_set_hostname("test2.com");
		if (iphost_get_hostname(hostname, 128)==0)
			printf("hostname=%s\n", hostname);
	}

	{
		int dns1_addr, dns2_addr;
		iphost_set_ipv4_dns("/tmp/resolv.conf", "test.com", inet_addr("8.8.8.1") , inet_addr("8.8.8.8"));
		if (iphost_get_ipv4_dns("/tmp/resolv.conf", &dns1_addr, &dns2_addr) ==0)
			printf("dns1_addr=0x%x, dns2_addr=0x%x\n", dns1_addr, dns2_addr);
		
		iphost_set_ipv4_dns("/tmp/resolv.conf", "test2.com", inet_addr("8.8.8.2") , inet_addr("8.8.8.9"));
		if (iphost_get_ipv4_dns("/tmp/resolv.conf", dns1_addr, dns2_addr) ==0)
			printf("dns1_str=%s, dns2_str=%s\n", iphost_inet_string(dns1_addr), iphost_inet_string(dns2_addr));
	}
#endif
	return 0;
}
#endif


