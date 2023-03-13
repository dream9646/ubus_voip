/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : igmp
 * File    : igmp_router.c
 *
 ******************************************************************/

#include <stdint.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/igmp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/mroute6.h>

#include "list.h"
#include "util.h"
#include "mcast_const.h"
#include "mcast.h"
#include "igmp_mbship.h"
#include "mcast_router.h"
#include "platform.h"

extern struct igmp_config_t igmp_config_g;
extern struct igmp_mbship_db_t igmp_mbship_db_g;
static char out_buffer[8192];
char *send_buf=out_buffer; 

// Globals                  
uint32_t     allhosts_group;          /* All hosts addr in net order */
uint32_t     allrouters_group;          /* All hosts addr in net order */


static LIST_HEAD(inf_list);
mcast_proxy mproxy;
static int vif2phyif[MAXVIFS];
static int mif2phyif[MAXMIFS];


/*
 * Convert an IP address in u_long (network) format into a printable string.
 */
char *inetfmt(uint32_t addr, char *s)
{
	register u_char *a;

	a = (u_char *)&addr;
	sprintf(s, "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
	return (s);
}

uint16_t inetchksum(uint16_t *addr, int len)
{
	register int nleft = len;
	register uint16_t *w = addr;
	uint16_t answer = 0;
	register int32_t sum = 0;

	/*
	*  Our algorithm is simple, using a 32 bit accumulator (sum),
	*  we add sequential 16 bit words to it, and at the end, fold
	*  back all the carry bits from the top 16 bits into the lower
     	*  16 bits.
	*/
	while (nleft > 1)
	{
		sum += *w++;
        	nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		*(uint8_t *) (&answer) = *(uint8_t *)w;
        	sum += answer;
    	}

	/*
	* add back carry outs from top 16 bits to low 16 bits
	*/
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16);         /* add carry */
	answer = ~sum;              /* truncate to 16 bits */
	return(answer);
}

/*
 * Construct an IGMP message in the output packet buffer.  The caller may
 * have already placed data in that buffer, of length 'datalen'.
 */

static void 
buildquery(short family, int ssm, int code, struct mcast_grec *query, int *datalen) 
{	
	if (family == AF_INET6)
	{
		struct mldv2_query_hdr *mld;
		
		mld = (struct mldv2_query_hdr *)(send_buf);
		mld->mldh.mld_type = MLD_LISTENER_QUERY;
		mld->mldh.mld_code = 0;
		mld->mldh.mld_cksum = 0;
		mld->mldh.mld_maxdelay = htons(10000); // 10 sec
		mld->mldh.mld_addr = query->grec_mca.ipv6;
		
		if (ssm)
		{
                	mld->qrv    = igmp_config_g.robustness;
			mld->qqic   = igmp_config_g.query_interval;
			mld->nsrcs  = query->grec_nsrcs;

                	int i;
			for (i=0;i < mld->nsrcs;i++)
			{
				mld->srcs[i] = query->grec_src[i].ipv6;
	    		}
	    		
	    		*datalen = sizeof(struct mldv2_query_hdr) + i * sizeof(mld->srcs[0]);
	    		dbprintf_igmp(LOG_INFO, "build MLDv2 %s%s %s query\n", (IN6_IS_ADDR_UNSPECIFIED(&mld->mldh.mld_addr))?"general":"group",(mld->nsrcs)?"-source":"", (IN6_IS_ADDR_UNSPECIFIED(&mld->mldh.mld_addr))?"":"specific");
		}
		else
		{
			*datalen = sizeof(struct mld_hdr);
			dbprintf_igmp(LOG_INFO, "build MLD %s query\n", (IN6_IS_ADDR_UNSPECIFIED(&mld->mldh.mld_addr))?"general":"group specific");
		}
		
		mld->mldh.mld_cksum = inetchksum((u_short *)mld, *datalen);		
	}
	else
	{
		struct igmpv3_query *igmp;
		
		igmp               = (struct igmpv3_query *)(send_buf);
		igmp->type         = IGMP_MEMBERSHIP_QUERY;
		igmp->code         = code;
		igmp->group        = query->grec_mca.ipv4.s_addr;
		igmp->csum         = 0;
		
		if (ssm)
		{
			igmp->qrv          = igmp_config_g.robustness;
			igmp->qqic         = igmp_config_g.query_interval;
			igmp->nsrcs        = query->grec_nsrcs;
	    
	    		int i;
			for (i=0;i < query->grec_nsrcs;i++)
			{
	        		igmp->srcs[i] = query->grec_src[i].ipv4.s_addr;
	    		}
	    		
	    		*datalen = sizeof(struct igmpv3_query) + i * sizeof(igmp->srcs[0]);
	    		dbprintf_igmp(LOG_INFO, "build IGMPv3 %s%s %s query\n", (igmp->group)?"group":"general",(igmp->nsrcs)?"-source":"", (igmp->group)?"specific":"");
	    	}
	    	else
	    	{
			*datalen = sizeof(struct igmphdr);
			dbprintf_igmp(LOG_INFO, "build IGMPv%s %s query\n", (igmp->code)?"2":"1", (igmp->group)?"group specific":"general");
		}
	    	         
		igmp->csum = inetchksum((u_short *)igmp, *datalen);
	}
}

IGMP_MODE igmp_v3_get_querier_version()
{
        struct mcast_proxy_grp_t *pos, *n;
        IGMP_MODE mode = IGMP_MODE_V3;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                if (pos->mode < mode) mode = pos->mode;                                 
	}
	
	return mode;
}

extern struct igmp_config_t igmp_config_g; // not good

static void sendmldquery(mcast_interface *p_if, struct mcast_grec *q)
{
	int outif = 0;
	int datalen = 0;
	struct sockaddr_in6 dst;
	union ipaddr ipdst;
	
	if (mproxy.mld_version == IM_DISABLE) return;
	
	if (p_if == NULL || q == NULL) return;
	
	int ssm = 0;
	if (igmp_config_g.igmp_version == MLD_V2_BIT) ssm = 1;
	buildquery(AF_INET6, ssm, 0, q, &datalen);
	
	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
	inet_pton(AF_INET6, "FF02::1", &dst.sin6_addr);
	ipdst.ipv6 = dst.sin6_addr;
			
	outif = p_if->if_index;
	
	if (setsockopt(mproxy.mld_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &outif, sizeof(outif)) < 0)
		dbprintf_igmp(LOG_ERR, "setsockopt IPV6_MULTICAST_IF failed\n");
	
	// set hop option
	if (sendto(mproxy.mld_socket, send_buf, datalen, 0, (struct sockaddr*)&dst, sizeof(struct sockaddr_in6)) < 0)
		dbprintf_igmp(LOG_ERR, "send mld query error: %s(0x%x)\n", strerror(errno), errno);
	else
		dbprintf_igmp(LOG_INFO, "send mld query to %s from %s\n", inet_ipaddr(ipdst, s1), inet_ipaddr(p_if->if_addr, s2));
}

static void sendigmpquery(mcast_interface *p_if, struct mcast_grec *q)
{
	struct sockaddr_in sdst;
	int datalen = 0;
	int dst = allhosts_group;
	
	if (p_if == NULL || q == NULL) return;		
	
	uint32_t src = p_if->if_addr.ipv4.s_addr;	    
	IGMP_MODE mode = igmp_v3_get_querier_version();
    
	unsigned int tmp = 0;
	switch(mode)
	{
	case IGMP_MODE_V1:
		// ignore group specific or group-source specific query in IGMPv1
		if (igmp_config_g.igmp_compatibity_mode && (q->grec_nsrcs || q->grec_mca.ipv4.s_addr)) return;
		buildquery(AF_INET, 0, 0, q, &datalen);	// v1 has only general query and code is 0
		break;
	case IGMP_MODE_V2:
        	// ignore group-source specific query in IGMPv2
        	if (igmp_config_g.igmp_compatibity_mode && q->grec_nsrcs)  return;
        	buildquery(AF_INET, 0, igmp_config_g.max_respon_time, q, &datalen);
        	break;
	case IGMP_MODE_V3:
		if (igmp_config_g.max_respon_time < 128)
			tmp = igmp_config_g.max_respon_time;
		else
		{
			int exp;
        		int mant;
        		tmp = igmp_config_g.max_respon_time;
        		int pos = 0;
			for (pos = 15 ; pos >= 7; pos--)
			{
				if (tmp & (1 << pos))
				{
					exp = pos - 5 + 1;
		                	mant = (tmp & (0xF << exp)) >> exp;
		                	exp -= 3;
		                	tmp = (1 << 7) | (exp << 4) | mant;
		                	break;
	            		}
	        	}
		}
        	buildquery(AF_INET, 1, tmp, q, &datalen);
        	break;
    	}
    	
	memset(&sdst, 0, sizeof(sdst));
	sdst.sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    	sdst.sin_len = sizeof(sdst);
#endif
    	sdst.sin_addr.s_addr = dst;
	
	// set outgoing if
	struct in_addr adr;
	adr.s_addr = src;
	
	if (setsockopt(mproxy.igmp_socket, IPPROTO_IP, IP_MULTICAST_IF, (void *)&adr, sizeof(adr)) < 0)
	{
		dbprintf_igmp(LOG_ERR,  "setsockopt IP_MULTICAST_IF %s failed\n", inetfmt(src, s1));
		return;
	}	
	    
	if (sendto(mproxy.igmp_socket, send_buf, datalen, 0, (struct sockaddr *)&sdst, sizeof(sdst)) < 0)
		dbprintf_igmp(LOG_ERR, "send igmp query error: %s(0x%x)\n", strerror(errno), errno);
	else
		dbprintf_igmp(LOG_INFO, "send igmp query to %s from %s\n", inetfmt(dst, s1), inetfmt(src, s2));

	// resetore outgoing if
        adr.s_addr = INADDR_ANY;
	if (setsockopt(mproxy.igmp_socket, IPPROTO_IP, IP_MULTICAST_IF, (void *)&adr, sizeof(adr)) < 0)
	{
		dbprintf_igmp(LOG_ERR,  "setsockopt IP_MULTICAST_IF %s failed\n", inetfmt(src, s1));
	}
}

static mcast_interface *mcast_interface_lookup(union ipaddr *src)
{
	unsigned short family;
	
	family = IS_IPV6_ADDR(*src)?AF_INET6:AF_INET;	
	
	mcast_interface *pos=NULL, *n=NULL;
	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family == family) return pos;
	}
	
	return NULL;
}

void mcast_sendquery(union ipaddr *src, struct mcast_grec *query)
{
	mcast_interface *p_if = mcast_interface_lookup(src);
	
	if (p_if->ss_family == AF_INET)
		sendigmpquery(p_if, query);
	else if (p_if->ss_family == AF_INET6)
		sendmldquery(p_if, query);
}

void mcast_sendgeneralquery()
{
	mcast_interface *pos=NULL, *n=NULL;
	
	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->type == INTERFACE_DOWNSTREAM)
		{
			// send the membership query...
		        struct mcast_grec gq;
		        memset(&gq, 0, sizeof(struct mcast_grec));
		        if (pos->ss_family == AF_INET)
		        	sendigmpquery(pos, &gq);
		        else if (pos->ss_family == AF_INET6)
		        	sendmldquery(pos, &gq);	
		}
	}
}

// kept for compatibility with cli_env
void sendgeneralmembershipquery()
{
	mcast_sendgeneralquery();
}

void sendigmpv3generalquery()
{
	mcast_sendgeneralquery();
}

// new
mcast_interface* mcast_interface_find(int family, if_type type)
{
	mcast_interface *pos=NULL, *n=NULL;
	
	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family == family && pos->type == type) return pos;
	}
	
	return NULL;
}

static void mcast_interface_cleanup_family(unsigned short family)
{
	mcast_interface *pos=NULL, *n=NULL;
	
	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family != family) continue;		
                list_del(&pos->node);
		free_safe(pos);
	}
}

#define NETUTILS_PATH_PROCNET_IFINET6    "/proc/net/if_inet6"
static unsigned long parse_hex(const char *p_str, unsigned char *p_inet)
{
	unsigned long len = 0;
	char *p = NULL;

	if (NULL == (p = (char*)p_str))
	return 0;
	while (*p)
	{
		int tmp;
		if (p[1] == 0) return 0;
		
		if (sscanf(p, "%02x", &tmp) != 1) return 0;
		p_inet[len] = tmp;
		len++;
		p += 2;
	}
	return len;
}

static STATUS mcast_get_ip6_addr(char *p_ifname, struct in6_addr *p_addr)
{
	FILE *fp = NULL;
	char ipv6Addr[INET6_ADDRSTRLEN];
	unsigned int ifIndex;
	unsigned int prefixLen;
	unsigned int addrScope;
	unsigned int ifFlags;
	char ifName[IF_NAMESIZE];

	if ((p_ifname == NULL)  || (p_addr == NULL))
	{
		return STATUS_NOK;
	}

/* # cat /proc/net/if_inet6  (see net/ipv6/addrconf.c for more info)
 *  00000000000000000000000000000001 01 80 10 80 lo
 *  +------------------------------+ ++ ++ ++ ++ ++
 *  |                                |  |  |  |  |
 *  1                                2  3  4  5  6
 *
 *  1. IPv6 address displayed in 32 hexadecimal chars without colons as separator
 *  2. Netlink device number (interface index) in hexadecimal (see ip addr , too)
 *  3. Prefix length in hexadecimal
 *  4. Scope value (see kernel source include/net/ipv6.h and net/ipv6/addrconf.c for more info)
 *  5. Interface flags (see include/linux/rtnetlink.h and net/ipv6/addrconf.c for more info)
 *  6. Device name
*/
	if ((fp = fopen(NETUTILS_PATH_PROCNET_IFINET6, "r")) != NULL)
	{
		while (fscanf(fp, "%s %02x %02x %02x %02x %s\n", ipv6Addr, &ifIndex, &prefixLen, &addrScope, &ifFlags, ifName) != EOF)
		{

			if (strcmp(p_ifname, ifName) != 0)
			{
				/* Interface name does not match, try next */
				continue;
			}

			if (strncmp(ipv6Addr, "fe80", strlen("fe80")) != 0)
			{
				/*we need find link-local address*/
				continue;
			}
			
			/* Convert IP address from text */
			parse_hex(ipv6Addr, p_addr->s6_addr);
			fclose(fp);
			return STATUS_OK;
		}
		fclose(fp);
	}
	
	return STATUS_NOK;
}

static STATUS init_interface(mcast_proxy *p_mproxy)
{
	mcast_interface *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos, n, &inf_list, node) {
		struct ifreq    ifr;
		int fd = 0;
		
		dbprintf_igmp(LOG_ERR, "init '%s' %s interface\n", pos->if_name, (pos->ss_family == AF_INET)?"IPv4":"IPv6");
		
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			dbprintf_igmp(LOG_ERR, "RAW socket open failed\n");
			goto error;
		}
		
                memset(&ifr, 0, sizeof(struct ifreq));
        	strcpy(ifr.ifr_name, pos->if_name);

                if (pos->ss_family == AF_INET)	// ipv4
                {
			if (ioctl(fd, SIOCGIFADDR, (caddr_t)&ifr, sizeof(struct ifreq)) < 0)
			{
                		dbprintf_igmp(LOG_ERR, "SIOCGIFADDR failed %s\n", ifr.ifr_name);
                		goto error;
            		}
            		struct sockaddr *sa = (struct sockaddr *)&ifr.ifr_addr;
			
			memcpy(&pos->if_addr, &(((struct sockaddr_in *)sa)->sin_addr),sizeof(struct in_addr));	
		}
		else	// ipv6
		{
			struct in6_addr in6;
			if (mcast_get_ip6_addr(pos->if_name, &in6) == STATUS_NOK) {
				dbprintf_igmp(LOG_ERR, "interface %s doesn't have ipv6 address\n", pos->if_name);
				goto error;
			}
			memcpy(&pos->if_addr, (struct in6_addr*)&in6, sizeof(struct in6_addr));
		}
		char  if_ip[INET6_ADDRSTRLEN];
		util_inet_ntop(pos->ss_family, &pos->if_addr, (char *)if_ip, sizeof(if_ip));
		dbprintf_igmp(LOG_ERR, "interface %s address: %s\n", pos->if_name, if_ip);
			
        	if (ioctl(fd, SIOCGIFINDEX, (caddr_t)&ifr, sizeof(struct ifreq)) < 0)
		{
            		    dbprintf_igmp(LOG_ERR, "SIOCGIFINDEX error\n");
			    goto error;
        	}
		pos->if_index = ifr.ifr_ifindex;

                if (pos->type == INTERFACE_UPSTREAM) p_mproxy->upif_index = pos->if_index;
		           
         	if (ioctl(fd, SIOCGIFMTU, (caddr_t)&ifr, sizeof(struct ifreq)) < 0)
		{
			dbprintf_igmp(LOG_ERR, "SIOCGIFMTU error\n");
            		goto error;
        	}

        	pos->if_mtu = ifr.ifr_mtu;
        	
                if (ioctl(fd, SIOCGIFFLAGS, (caddr_t)&ifr, sizeof(struct ifreq)) < 0)
		{
			dbprintf_igmp(LOG_ERR, "SIOCGIFFLAGS error\n");
            		goto error;
        	}
        
        	dbprintf_igmp(LOG_ERR, "if_index - %d, if_flags = %d\n", pos->if_index, ifr.ifr_flags);
		       	
        	close(fd);
        	continue;
				
	error:
		close(fd);
		list_del(&pos->node);	// remove interface failed to init
		free_safe(pos);
	}
	
	if (mcast_interface_find(AF_INET, INTERFACE_UPSTREAM) == NULL ||
        	mcast_interface_find(AF_INET, INTERFACE_DOWNSTREAM) == NULL) 
	{
		mcast_interface_cleanup_family(AF_INET);
		p_mproxy->igmp_version = IM_DISABLE;
		dbprintf_igmp(LOG_ERR, "Disable IGMP v1/v2/v3\n");
    	}
    	
	if (mcast_interface_find(AF_INET6, INTERFACE_UPSTREAM) == NULL ||
        	mcast_interface_find(AF_INET6, INTERFACE_DOWNSTREAM) == NULL) 
	{
		mcast_interface_cleanup_family(AF_INET6);
		p_mproxy->mld_version = IM_DISABLE;
		dbprintf_igmp(LOG_ERR, "Disable MLD v1/v2\n");
    	}	

	if (p_mproxy->igmp_version == IM_DISABLE && p_mproxy->mld_version == IM_DISABLE)
	{
        	dbprintf_igmp(LOG_ERR, "Neither IGMP nor MLD proxy can be formed\n");
		return STATUS_NOK;
	}
    	
	return STATUS_OK;	
}

static int get_vifi_ipv4(int ifindex)
{
	int i;
	for (i = 0; i < MAXVIFS; i++)
	{
		/* found already allocated one */
		if (vif2phyif[i] == ifindex) return i;
		
		/* you have seeked all the registerd mifs */
		if (vif2phyif[i] == 0)
		{
			vif2phyif[i] = ifindex;
			return i;
		}
	}
	return 0;
}

static int get_mifi_ipv6(int ifindex)
{
	int i;
	for (i = 0; i < MAXMIFS; i++)
	{
        	/* found already allocated one */
		if (mif2phyif[i] == ifindex) return i;
		

		/* you have seeked all the registerd mifs */
		if (mif2phyif[i] == 0)
		{
			mif2phyif[i] = ifindex;
			return i;
		}
	}
	return 0;
}

static int k_get_vmif(int ifindex, int family)
{
    if (family == AF_INET)
        return get_vifi_ipv4(ifindex);
    else if (family == AF_INET6)
        return get_mifi_ipv6(ifindex);

    return 0;
}

static void k_add_ip4_vif(int socket, struct in_addr* sin, int ifindex)
{
	struct vifctl vif;
	memset(&vif, 0, sizeof(vif));
	memcpy(&vif.vifc_lcl_addr, sin, sizeof(struct in_addr));

	vif.vifc_vifi = k_get_vmif(ifindex, AF_INET);
	vif.vifc_flags = 0;        /* no tunnel, no source routing, register ? */
	vif.vifc_threshold = 1;
	dbprintf_igmp(LOG_INFO, "socket = %d vif.vifc_vifi = %d %s\n", socket, vif.vifc_vifi, inet_ntoa(vif.vifc_lcl_addr));
	if (setsockopt(socket, IPPROTO_IP, MRT_ADD_VIF, &vif, sizeof(struct vifctl)) < 0)
	{
        	dbprintf_igmp(LOG_ERR, "MRT_ADD_VIF failed - %s\n", strerror(errno));
        }

}

void k_add_ip6_mif(int socket, int ifindex)
{
	struct mif6ctl mif;
	mifi_t mifi;
	
	mifi = k_get_vmif(ifindex, AF_INET6);
	memset(&mif, 0, sizeof(mif));
	mif.mif6c_mifi= mifi;
	mif.mif6c_pifi= ifindex;
	mif.mif6c_flags = 0;
	
	dbprintf_igmp(LOG_INFO, "socket = %d vif.mif6c_mifi = %d mif6c_pifi = %d\n", socket, mif.mif6c_mifi, mif.mif6c_pifi);
	
	if (setsockopt(socket, IPPROTO_IPV6, MRT6_ADD_MIF, &mif, sizeof(struct mif6ctl)) < 0)
	{
        	dbprintf_igmp(LOG_ERR, "MRT6_ADD_MIF failed - %s\n", strerror(errno));
        }
}

static void add_all_vif()
{
	mcast_interface *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family == AF_INET)
			k_add_ip4_vif(mproxy.igmp_socket, &pos->if_addr.ipv4, pos->if_index);
		
		if (pos->ss_family == AF_INET6)
			k_add_ip6_mif(mproxy.mld_socket, pos->if_index);
	}
}

static void k_del_ip4_vif(int socket, int ifindex)
{
	struct vifctl vif;
	memset(&vif, 0, sizeof(vif));
	
	vif.vifc_vifi = k_get_vmif(ifindex, AF_INET);
	
	if (setsockopt(socket, IPPROTO_IP, MRT_DEL_VIF, &vif, sizeof(struct vifctl)) < 0)
	{
        	dbprintf_igmp(LOG_ERR, "MRT_DEL_VIF failed\n");
	}
}

void k_del_ip6_mif (int socket, int ifindex)
{
	mifi_t mifi;
	
	mifi = k_get_vmif(ifindex, AF_INET6);
	
	if (setsockopt(socket, IPPROTO_IPV6, MRT6_DEL_MIF, &mifi, sizeof(mifi)) < 0)
	{
        	dbprintf_igmp(LOG_ERR, "MRT6_DEL_MIF failed\n");
	}
}

static void del_all_vif()
{
	mcast_interface *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family == AF_INET)
			k_del_ip4_vif(mproxy.igmp_socket, pos->if_index);
		
		if (pos->ss_family == AF_INET6)
			k_del_ip6_mif(mproxy.mld_socket, pos->if_index);
	}
}

static void mcast_interface_create(char *p_ifname, int family, if_type type)
{
	mcast_interface* p_if = NULL;
	if ((p_if = (mcast_interface *)malloc_safe(sizeof(mcast_interface))) == NULL)
	{
		dbprintf_igmp(LOG_ERR, "if create error\n");
		return;
	}
	memset(p_if, 0, sizeof(mcast_interface));
	strcpy(p_if->if_name, p_ifname);

	p_if->ss_family = family;
	p_if->type = type;
	
	INIT_LIST_NODE(&p_if->node);
	list_add(&p_if->node, &inf_list);
}

static STATUS k_start4_mproxy(int socket)
{
	int d = 1;
	if (setsockopt(socket, IPPROTO_IP, MRT_INIT, &d, sizeof(int)) < 0)
	{
    		dbprintf_igmp(LOG_ERR, "MRT_INIT failed\n");
        	return STATUS_NOK;
    	}

    	memset(&vif2phyif, 0, sizeof(vif2phyif));
    	vif2phyif[0] = -1;
	    
	return STATUS_OK;
}

static void k_stop4_mproxy(int socket)
{
	int d = 1;
	if (setsockopt(socket, IPPROTO_IP, MRT_DONE, &d, sizeof(int)) < 0)
		dbprintf_igmp(LOG_ERR, "MRT_DONE failed\n");
}

static STATUS init_mproxy4(mcast_proxy *p_mp)
{
	p_mp->igmp_socket = -1;
	int i;
	unsigned char ui;
	
	if (p_mp->igmp_version == IM_DISABLE) return STATUS_OK;
	
	p_mp->igmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
        if (p_mp->igmp_socket == -1)
        {
            dbprintf_igmp(LOG_ERR, "can't create igmp socket\n");
            return STATUS_NOK;
        }

        // Set router alert
	char ra[4];
	ra[0] = 148;
        ra[1] = 4;
        ra[2] = 0;
        ra[3] = 0;
        if (setsockopt(p_mp->igmp_socket, IPPROTO_IP, IP_OPTIONS, ra, 4) < 0)
        	dbprintf_igmp(LOG_ERR, "IP_OPTIONS: %s\n", strerror(errno));
		
        // Set reuseaddr, ttl, loopback and set outgoing interfaces
        i = 1;
        if (setsockopt(p_mp->igmp_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(i)) < 0)
        	dbprintf_igmp(LOG_ERR, "SO_REUSEADDR: %s\n", strerror(errno));

        i = 256*1024;
        if (setsockopt(p_mp->igmp_socket, SOL_SOCKET, SO_RCVBUF, (void*)&i, sizeof(i)) < 0)
        	dbprintf_igmp(LOG_ERR, "SO_RCVBUF: %s\n", strerror(errno));

        ui = 1;
        if (setsockopt(p_mp->igmp_socket, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ui, sizeof(ui)))
        	dbprintf_igmp(LOG_ERR, "IP_MULTICAST_TTL: %s\n", strerror(errno));
        
        ui = 0;
        if (setsockopt(p_mp->igmp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (void*)&ui, sizeof(ui)) < 0)
            dbprintf_igmp(LOG_ERR, "IP_MULTICAST_LOOP: %s\n", strerror(errno));
        
        /*
	ui = 1;
        if (setsockopt(p_mp->igmp_socket, IPPROTO_IP, IP_PKTINFO, &ui, sizeof(ui)))
            dbprintf_igmp(LOG_ERR, "IP_PKTINFO: %s\n", strerror(errno));
        */
	    	        
        if (k_start4_mproxy(p_mp->igmp_socket) < 0) return STATUS_NOK;
        dbprintf_igmp(LOG_ERR, "p_mp->igmp_socket = %d\n", p_mp->igmp_socket);

        p_mp->igmp_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (p_mp->igmp_udp_socket == -1)
        {
            dbprintf_igmp(LOG_ERR, "can't create udp socket\n");
            return STATUS_NOK;
        }   
	mcast_interface *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family != AF_INET || pos->type != INTERFACE_DOWNSTREAM) continue;
		
		// join all routers
	}        
	    	
	return STATUS_OK;
}

STATUS k_start6_mproxy(int socket)
{
	int d = 1;
	
	if (setsockopt(socket, IPPROTO_IPV6, MRT6_INIT, &d, sizeof(int)) < 0)
	{
		dbprintf_igmp(LOG_ERR, "MRT6_INIT failed - %s\n", strerror(errno));
		return STATUS_NOK;
	}
	
	/* start vif/mif management */
    	memset(&mif2phyif, 0, sizeof(mif2phyif));
    	mif2phyif[0] = -1;
	    
	return STATUS_OK;
}

static void k_stop6_mproxy(int socket)
{
	int d = 1;
	// mld use MRT6_INIT for stopping
	if (setsockopt(socket, IPPROTO_IPV6, MRT6_DONE, &d, sizeof(int)) < 0)
		dbprintf_igmp(LOG_ERR, "MRT6_INIT failed  - %s\n", strerror(errno));
}


static int get_up_if_index(void)
{
    return mproxy.upif_index;
}

static int get_udp_socket(int family)
{
	if(family == AF_INET)
		return mproxy.igmp_udp_socket;
	else if(family == AF_INET6)
		return mproxy.mld_udp_socket;
	else
		dbprintf_igmp(LOG_ERR, "unsoupprt family %d\n", family);
	return 0;
}

/*protocol independent code*/
static STATUS k_mcast_join(union ipaddr *p_addr, char* ifname)
{

	struct group_req req;
	int socket = 0;

	if (ifname != NULL)
	{
		if ((req.gr_interface = if_nametoindex(ifname)) == 0) 
		{
            		dbprintf_igmp(LOG_ERR, "if_nametoindex failed\n");
            		return STATUS_NOK;
        	}
	}
	else
		req.gr_interface = get_up_if_index();/*if ifindex == 0, kernel will select interface*/
	
	((struct sockaddr_in6*)&req.gr_group)->sin6_addr = p_addr->ipv6; 
	req.gr_group.ss_family = AF_INET6;
	socket = get_udp_socket(AF_INET6);

	dbprintf_igmp(LOG_ERR, "socket = %d ifname = %s req.gr_interface = %d grp = %s\n", socket, ifname, req.gr_interface, inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&req.gr_group)->sin6_addr), s1, sizeof(s1)));
		
	return setsockopt(socket, SOL_IPV6, MCAST_JOIN_GROUP, &req, sizeof(struct group_req));
}

static STATUS init_mproxy6(mcast_proxy *p_mp)
{
	struct icmp6_filter filt;
	int             on;
	
	if (p_mp->mld_version == IM_DISABLE) return STATUS_OK;
	        
        p_mp->mld_socket = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

        if (p_mp->mld_socket == -1)
        {
            dbprintf_igmp(LOG_ERR, "can't create mld socket\n");
            return STATUS_NOK;
        }
        
        // filter all non-MLD ICMP messages
        ICMP6_FILTER_SETBLOCKALL(&filt);
        ICMP6_FILTER_SETPASS(MLD_LISTENER_REPORT, &filt);
        ICMP6_FILTER_SETPASS(MLD_LISTENER_REDUCTION, &filt);
        ICMP6_FILTER_SETPASS(MLD_V2_LISTENER_REPORT, &filt);

        if (setsockopt(p_mp->mld_socket, IPPROTO_ICMPV6, ICMP6_FILTER, &filt, sizeof(filt)) < 0)
        	dbprintf_igmp(LOG_ERR, "ICMP6_FILTER: %s\n", strerror(errno));

        // specify to tell receiving interface
        unsigned char rabuf[] = {0x05, 0x02, 0x00, 0x00, 0x01, 0x00};
        struct { struct ip6_hbh p_iph;char buf[sizeof(rabuf)];} hyhoption;
	hyhoption.p_iph.ip6h_len = 0;
        hyhoption.p_iph.ip6h_nxt =  IPPROTO_ICMPV6;
        memcpy(hyhoption.buf, rabuf, sizeof(rabuf));

        if (setsockopt(p_mp->mld_socket, IPPROTO_IPV6, IPV6_HOPOPTS, &hyhoption, sizeof(hyhoption)) < 0)
                dbprintf_igmp(LOG_ERR, "IPV6_HOPOPTS: %s\n", strerror(errno));
                
        on = 1;
        if (setsockopt(p_mp->mld_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0)
        	dbprintf_igmp(LOG_ERR, "IPV6_RECVPKTINFO: %s\n", strerror(errno));

        on = 1;
        if (setsockopt(p_mp->mld_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &on, sizeof(on)) < 0)
        	dbprintf_igmp(LOG_ERR, "IPV6_MULTICAST_HOPS: %s\n", strerror(errno));

        on = 0;
        if (setsockopt(p_mp->mld_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on, sizeof(on)) < 0)
        	dbprintf_igmp(LOG_ERR, "IPV6_MULTICAST_LOOP: %s\n", strerror(errno));

        on = 1;
        if (setsockopt(p_mp->mld_socket, IPPROTO_IPV6, IPV6_RECVHOPOPTS, &on, sizeof(on)) < 0)
        	dbprintf_igmp(LOG_ERR, "IPV6_RECVHOPOPTS: %s\n", strerror(errno));
        
        if (k_start6_mproxy(p_mp->mld_socket) < 0)
            return STATUS_NOK;

        dbprintf_igmp(LOG_ERR, "p_mp->mld_socket = %d\n", p_mp->mld_socket);
        p_mp->mld_udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
        if (p_mp->mld_udp_socket == -1)
        {
            dbprintf_igmp(LOG_ERR, "can't create mld udp socket\n");
            return STATUS_NOK;
        }
        mcast_interface *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos, n, &inf_list, node) {
		if (pos->ss_family != AF_INET6 || pos->type != INTERFACE_DOWNSTREAM) continue;

		struct in6_addr all_ipv6_router;
		union ipaddr allrouter;
		
		dbprintf_igmp(LOG_ERR, "join all routers on %s\n", pos->if_name);
		// join all routers
		// if not ip6_input.c of ip6_mc_input could drop join/leave report
		inet_pton(AF_INET6, "FF02::2", &all_ipv6_router);
		allrouter.ipv6 = all_ipv6_router;
		if (k_mcast_join(&allrouter, pos->if_name) < 0)
		{
			k_mcast_join(&allrouter, pos->if_name);
			dbprintf_igmp(LOG_ERR, "k_mcast_join FF02::2 error %s\n", strerror(errno));
		}
		
		inet_pton(AF_INET6, "FF02::16", &all_ipv6_router);
		allrouter.ipv6 = all_ipv6_router;
		if (k_mcast_join(&allrouter, pos->if_name) < 0)
		{
			k_mcast_join(&allrouter, pos->if_name);
			dbprintf_igmp(LOG_ERR, "k_mcast_join FF02::16 error %s\n", strerror(errno));
		}		
	}
    
	return STATUS_OK;
}

int igmp_router_mode_init(void)
{
	memset(&mproxy, 0, sizeof(mcast_proxy));
	mproxy.igmp_version = IM_IGMPv3_MLDv2;
    	mproxy.mld_version = IM_IGMPv3_MLDv2;

        mproxy.igmp_socket = -1;
	mproxy.mld_socket = -1;
	mproxy.igmp_udp_socket = -1;
	mproxy.mld_udp_socket = -1;
	
	allhosts_group   = htonl(INADDR_ALLHOSTS_GROUP);
		
	// add upstream and downstream IPv4 interfaces
	mcast_interface_create(THREAD_ENV.router_wan_if, AF_INET, INTERFACE_UPSTREAM);
	mcast_interface_create(THREAD_ENV.router_lan_if, AF_INET, INTERFACE_DOWNSTREAM);
        // add upstream and downstream IPv6 interfaces
	mcast_interface_create(THREAD_ENV.router_wan_if, AF_INET6, INTERFACE_UPSTREAM);
	mcast_interface_create(THREAD_ENV.router_lan_if, AF_INET6, INTERFACE_DOWNSTREAM);
		
	
	STATUS proxy4status = init_mproxy4(&mproxy);
	STATUS proxy6status = init_mproxy6(&mproxy);	
	if (init_interface(&mproxy) == STATUS_NOK || 
	(proxy4status == STATUS_NOK && proxy6status == STATUS_NOK))
	{
		dbprintf_igmp(LOG_ERR, "route mode init failed\n");
		return -1;
	}
	
	add_all_vif();

	if (THREAD_ENV.task_pause_mask & (1<<ENV_TASK_NO_IGMP))
	{
		dbprintf_igmp(LOG_ERR, "start igmp intake\n");
		THREAD_ENV.task_pause_mask &= ~(1<<ENV_TASK_NO_IGMP);
	}
		
	return 0;
}

void igmp_router_mode_deinit(void) 
{
	del_all_vif();	
	k_stop4_mproxy(mproxy.igmp_socket);
	k_stop6_mproxy(mproxy.mld_socket);
	close(mproxy.igmp_socket);
	close(mproxy.mld_socket);
	close(mproxy.igmp_udp_socket);
	close(mproxy.mld_udp_socket);
	mproxy.igmp_socket = -1;
	mproxy.mld_socket = -1;
	mproxy.igmp_udp_socket = -1;
	mproxy.mld_udp_socket = -1;		
	mcast_interface_cleanup_family(AF_INET);
	mcast_interface_cleanup_family(AF_INET6);
}


