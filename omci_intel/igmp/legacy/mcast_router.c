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
#include <netinet/igmp.h>
#include <errno.h>
#include <sys/ioctl.h>
//#include <linux/mroute.h>
#include "mroute.h"

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "mcast_const.h"
#include "mcast.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "igmp_omci.h"
#include "mcast_router.h"
#include "mcast_timer.h"
#include "hwresource.h"

// Bit manipulation macros...
#define BIT_ZERO(X)      ((X) = 0)
#define BIT_SET(X,n)     ((X) |= 1 << (n))
#define BIT_CLR(X,n)     ((X) &= ~(1 << (n)))
#define BIT_TST(X,n)     ((X) & 1 << (n))
// Useful macros..          
#define VCMC( Vc )  (sizeof( Vc ) / sizeof( (Vc)[ 0 ] ))
#define VCEP( Vc )  (&(Vc)[ VCMC( Vc ) ])

struct ifreq lanif_info;
extern int timer_id_router_non_fast_leave_g;
extern struct igmp_config_t igmp_config_g;
extern struct igmp_mbship_db_t igmp_mbship_db_g;
static char out_buffer[8192];
char *send_buf=out_buffer; 
/*
 * Exported variables.
 */
char s1[19];        /* buffers to hold the string representations  */
char s2[19];        /* of IP addresses, to be passed to inet_fmt() */
char s3[19];        /* or inet_fmts().                             */
char s4[19];

typedef struct {
    uint32_t      group;
    uint32_t      vifaddr;
    short       started;
} groupvifdesc;

struct routetable {
    struct routetable   *nextroute;     // Pointer to the next group in line.
    struct routetable   *prevroute;     // Pointer to the previous group in line.
    uint32_t              group;          // The group to route
    uint32_t              originaddr;     // the origin adress (only set on activated routes)
    uint32_t              vifbits;        // bits representing recieving vifs.

    // Keeps the upstream membership state...
    short               upstrstate;     // upstream membership state.

    // These parameters contain aging details.
    uint32_t              agevifbits;     // bits representing aging vifs.
    int                 agevalue;       // Downcounter for death.          
    int                 ageactivity;    // Records any acitivity that notes there are still listeners.
};
static struct routetable   *routing_table;

// The upstream VIF index
int         upstreamvif;   

struct ifdesc ifdescvc[ MAX_IF ], *ifdescep = ifdescvc;

// Globals                  
uint32_t     allhosts_group;          /* All hosts addr in net order */
uint32_t     allrouters_group;          /* All hosts addr in net order */

mcast_proxy mproxy ={.igmp_socket = -1, .igmp_udp_socket = -1, .mld_socket = -1, .mld_udp_socket = -1};


/**
*   Finds the textual name of the supplied IGMP request.
*/
char *igmppacketkind(u_int type, u_int code)
{
	static char unknown[20];

	switch (type)
	{
	case IGMP_MEMBERSHIP_QUERY:     return  "Membership query  ";
	case IGMP_V1_MEMBERSHIP_REPORT:  return "V1 member report  ";
	case IGMP_V2_MEMBERSHIP_REPORT:  return "V2 member report  ";
	case IGMP_V2_LEAVE_GROUP:        return "Leave message     ";
    
	default:
        	sprintf(unknown, "unk: 0x%02x/0x%02x    ", type, code);
        	return unknown;
	}
}

/*
** Formats 'InAdr' into a dotted decimal string. 
**
** returns: - pointer to 'St'
**          
*/
char *fmtinadr( char *st, struct in_addr inadr )
{
	sprintf(st, "%u.%u.%u.%u", 
		((uint8_t *)&inadr.s_addr)[ 0 ],
		((uint8_t *)&inadr.s_addr)[ 1 ],
		((uint8_t *)&inadr.s_addr)[ 2 ],
		((uint8_t *)&inadr.s_addr)[ 3 ]);

	return st;
}

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

/*
 * Convert an IP subnet number in u_long (network) format into a printable
 * string including the netmask as a number of bits.
 */
static char *inetfmts(uint32_t addr, uint32_t mask, char *s)
{
	register u_char *a, *m;
	int bits;

	if ((addr == 0) && (mask == 0))
	{
		sprintf(s, "default");
        	return (s);
	}
	a = (u_char *)&addr;
	m = (u_char *)&mask;
	bits = 33 - ffs(ntohl(mask));

	if (m[3] != 0) sprintf(s, "%u.%u.%u.%u/%d", a[0], a[1], a[2], a[3], bits);
	else if (m[2] != 0) sprintf(s, "%u.%u.%u/%d",    a[0], a[1], a[2], bits);
	else if (m[1] != 0) sprintf(s, "%u.%u/%d",       a[0], a[1], bits);
	else                sprintf(s, "%u/%d",          a[0], bits);

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
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'IfName' exists
**          
*/
struct ifdesc *getifbyname(const char *ifname)
{
	struct ifdesc *dp;

	for (dp = ifdescvc; dp < ifdescep; dp++)
	if (!strcmp( ifname, dp->name)) return dp;

	return NULL;
}

/*
** returns a pointer to the ifdesc of the interface 'ix'
**
** returns: - pointer to the ifdesc of the requested interface
**          - null if no interface 'ix' exists
**          
*/
struct ifdesc *getifbyix(int ix)
{
	struct ifdesc *dp;
	if (ix <0 && ix >= MAX_IF)
		return NULL;
	dp = &ifdescvc[ ix ];
	return dp < ifdescep ? dp : NULL;
}

/**
*   returns a pointer to the ifdesc whose subnet matches
*   the supplied ip adress. the ip must match a interfaces
*   subnet, or any configured allowed subnet on a interface.
*/
struct ifdesc *getifbyaddress(uint32_t ipaddr)
{

	struct ifdesc       *dp;
	struct subnetlist   *currsubnet;
	struct ifdesc       *res = NULL;
	uint32_t            last_subnet_mask = 0;
	
	for (dp = ifdescvc; dp < ifdescep; dp++)
	{
		// loop through all registered allowed nets of the vif...
		for (currsubnet = dp->allowednets; currsubnet != NULL; currsubnet = currsubnet->next)
		{
			// check if the ip falls in under the subnet....
			if(currsubnet->subnet_mask > last_subnet_mask && (ipaddr & currsubnet->subnet_mask) == currsubnet->subnet_addr)
			{
				res = dp;
				last_subnet_mask = currsubnet->subnet_mask;
			}
		}
	}
	return res;
}


/**
*   returns a pointer to the ifdesc whose subnet matches
*   the supplied ip adress. the ip must match a interfaces
*   subnet, or any configured allowed subnet on a interface.
*/
struct ifdesc *getifbyvifindex(unsigned vifindex)
{
	struct ifdesc *dp;
    
	if (vifindex > 0)
	{
		for ( dp = ifdescvc; dp < ifdescep; dp++ )
		{
			if (dp->index == vifindex)
			{
				return dp;
			}
		}
	}
	return NULL;
}


/**
*   function that checks if a given ipaddress is a valid
*   address for the supplied vif.
*/
int isadressvalidforif(struct ifdesc* intrface, uint32_t ipaddr)
{
    if(intrface == NULL)
    {
        return 0;
    }

    return 1;
}

static struct vifdesc {
    struct ifdesc *ifdp;
} vifdescvc[ MAXVIFS ];
/*
** adds the interface '*ifdp' as virtual interface to the mrouted api
** 
*/
static void addvif(int socket, struct ifdesc *ifdp)
{
	struct vifctl vifctl;
	struct vifdesc *vifdp;

	/* search free vifdesc
	*/
	for (vifdp = vifdescvc; vifdp < VCEP( vifdescvc ); vifdp++)
	{
		if (!vifdp->ifdp)
			break;
	}

	/*
	no more space
	*/
	if (vifdp >= VCEP( vifdescvc )) dbprintf_igmp(LOG_ERR, "addvif, out of vif space\n");

	vifdp->ifdp = ifdp;

	vifctl.vifc_vifi  = vifdp - vifdescvc; 
	vifctl.vifc_flags = 0;        /* no tunnel, no source routing, register ? */
	vifctl.vifc_threshold  = vifdp->ifdp->threshold;    // packet ttl must be at least 1 to pass them
	vifctl.vifc_rate_limit = vifdp->ifdp->ratelimit;    // ratelimit

	vifctl.vifc_lcl_addr.s_addr = vifdp->ifdp->inadr.s_addr;
	vifctl.vifc_rmt_addr.s_addr = INADDR_ANY;

	// set the index...
	vifdp->ifdp->index = vifctl.vifc_vifi;

	dbprintf_igmp(LOG_ERR, "adding vif, ix %d fl 0x%x ip 0x%08x %s, threshold: %d, ratelimit: %d\n", 
		vifctl.vifc_vifi, vifctl.vifc_flags,  vifctl.vifc_lcl_addr.s_addr, vifdp->ifdp->name,
		vifctl.vifc_threshold, vifctl.vifc_rate_limit);

	if (setsockopt(socket, IPPROTO_IP, MRT_ADD_VIF, (char *)&vifctl, sizeof(vifctl)))
		dbprintf_igmp(LOG_ERR,"fail to MRT_ADD_VIF errno:%d\n",errno );
}

/*
static void delvif(struct ifdesc *ifdp)
{
	struct vifctl vifctl;
	struct vifdesc *vifdp;
	
	for (vifdp = vifdescvc; vifdp < VCEP( vifdescvc ); vifdp++)
	{
		if (!vifdp->ifdp) break;
		if (vifdp->ifdp != ifdp) continue;
		
		vifctl.vifc_vifi  = vifdp - vifdescvc; 
		vifctl.vifc_flags = 0;
		vifctl.vifc_threshold  = vifdp->ifdp->threshold;    // packet ttl must be at least 1 to pass them
		vifctl.vifc_rate_limit = vifdp->ifdp->ratelimit;    // ratelimit
	
		vifctl.vifc_lcl_addr.s_addr = vifdp->ifdp->inadr.s_addr;
		vifctl.vifc_rmt_addr.s_addr = INADDR_ANY;
	
		// set the index...
		vifdp->ifdp->index = vifctl.vifc_vifi;
	
		dbprintf_igmp(LOG_ERR, "deleting vif, ix %d fl 0x%x ip 0x%08x %s, threshold: %d, ratelimit: %d\n", 
			vifctl.vifc_vifi, vifctl.vifc_flags,  vifctl.vifc_lcl_addr.s_addr, vifdp->ifdp->name,
			vifctl.vifc_threshold, vifctl.vifc_rate_limit);
	
		if (setsockopt(mproxy.igmp_socket, IPPROTO_IP, MRT_DEL_VIF, (char *)&vifctl, sizeof(vifctl)))
			dbprintf_igmp(LOG_ERR,"fail to MRT_DEL_VIF errno:%d\n",errno );
			
	}
}
*/

/*
** adds the multicast routed '*dp' to the kernel routes
**
** returns: - 0 if the function succeeds
**          - the errno value for non-fatal failure condition
*/
static int addmroute(struct mroutedesc *dp)
{
	struct mfcctl ctlreq;
	int rc;

	ctlreq.mfcc_origin    = dp->originadr;
	ctlreq.mfcc_mcastgrp  = dp->mcadr;
	ctlreq.mfcc_parent    = dp->invif;

	/*
	copy the ttl vector
	*/

	memcpy(ctlreq.mfcc_ttls, dp->ttlvc, sizeof(ctlreq.mfcc_ttls));

	{
		char fmtbuo[ 32 ], fmtbum[ 32 ];
		
		dbprintf_igmp(LOG_WARNING, "adding mfc: %s -> %s, inpvif: %d\n", 
			fmtinadr(fmtbuo, ctlreq.mfcc_origin), 
			fmtinadr(fmtbum, ctlreq.mfcc_mcastgrp),
			(int)ctlreq.mfcc_parent);
	}

	rc = setsockopt(mproxy.igmp_socket, IPPROTO_IP, MRT_ADD_MFC, (void *)&ctlreq, sizeof(ctlreq));
	if (rc) dbprintf_igmp(LOG_WARNING, "mrt_add_mfc fail. errno %d sizeof( ctlreq ) %d\n",errno,sizeof(ctlreq));

	return rc;
}

/*
** removes the multicast routed '*dp' from the kernel routes
**
** returns: - 0 if the function succeeds
**          - the errno value for non-fatal failure condition
*/
static int delmroute(struct mroutedesc *dp)
{
	struct mfcctl ctlreq;
	int rc;

	ctlreq.mfcc_origin    = dp->originadr;
	ctlreq.mfcc_mcastgrp  = dp->mcadr;
	ctlreq.mfcc_parent    = dp->invif;

	/*
	clear the ttl vector
	*/
	memset(ctlreq.mfcc_ttls, 0, sizeof(ctlreq.mfcc_ttls));

	{
		char fmtbuo[ 32 ], fmtbum[ 32 ];
		
		dbprintf_igmp(LOG_WARNING,  "removing mfc: %s -> %s, inpvif: %d\n",
			fmtinadr( fmtbuo, ctlreq.mfcc_origin ), fmtinadr(fmtbum, ctlreq.mfcc_mcastgrp), (int)ctlreq.mfcc_parent);
	}

	rc = setsockopt( mproxy.igmp_socket, IPPROTO_IP, MRT_DEL_MFC, (void *)&ctlreq, sizeof( ctlreq ) );
	if (rc) dbprintf_igmp( LOG_WARNING, "mrt_del_mfc\n" );

	return rc;
}

void k_set_if(uint32_t ifa)
{
	struct in_addr adr;

	adr.s_addr = ifa;
	if (setsockopt(mproxy.igmp_socket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&adr, sizeof(adr)) < 0)
		dbprintf_igmp(LOG_ERR,  "setsockopt IP_MULTICAST_IF %s\n", inetfmt(ifa, s1));
}

void k_set_loop(int l)
{
	u_char loop;

	loop = l;
	if (setsockopt(mproxy.igmp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop)) < 0)
		dbprintf_igmp(LOG_ERR, "setsockopt IP_MULTICAST_LOOP %u\n", loop);
}

int curttl = 0;

static void k_set_ttl(int socket, int t)
{
	u_char ttl;

	ttl = t;
	if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) < 0)
		dbprintf_igmp(LOG_ERR,  "setsockopt IP_MULTICAST_TTL %u\n", ttl);
	curttl = t;
}

static int joinleave(int cmd, int udpsock, struct ifdesc *ifdp, uint32_t mcastaddr) 
{
	struct ip_mreq ctlreq;
	const char *cmdst = cmd == 'j' ? "join" : "leave";
	struct igmp_group_t igmp_group;
    
	memset(&ctlreq, 0, sizeof(ctlreq));
	ctlreq.imr_multiaddr.s_addr = mcastaddr;
	ctlreq.imr_interface.s_addr = ifdp->inadr.s_addr;

	igmp_group.group_ip.ipv4.s_addr = mcastaddr;
	igmp_group.src_ip.ipv4.s_addr = 0;
	if (cmd != 'j'&& igmp_mbship_group_other_client_exist(&igmp_mbship_db_g, &igmp_group))
	{
		dbprintf_igmp( LOG_WARNING,  "other client join in %s. leave would not be sent to up stream\n",inetfmt( mcastaddr, s1 ) ); 
		return 0;
	}
	
	dbprintf_igmp( LOG_WARNING,  "%sMcGroup: %s on %s\n", cmdst, inetfmt( mcastaddr, s1 ), ifdp ? ifdp->name : "<any>" );
    
	if (setsockopt(mproxy.igmp_udp_socket, IPPROTO_IP, cmd == 'j' ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, (void *)&ctlreq, sizeof(ctlreq))) 
	{
		dbprintf_igmp(LOG_WARNING,  "MRT_%s_MEMBERSHIP failed\n", cmd == 'j' ? "ADD" : "DROP");
		return 1;
	}
	
	return 0;
}

static int joinmcgroup(int udpsock, struct ifdesc *ifdp, uint32_t mcastaddr)
{
	return joinleave('j', mproxy.igmp_udp_socket, ifdp, mcastaddr);
}

static int leavemcgroup(int udpsock, struct ifdesc *ifdp, uint32_t mcastaddr)
{
	return joinleave('l', mproxy.igmp_udp_socket, ifdp, mcastaddr);
}

void sendjoinleaveupstream(struct routetable* route, int join)
{
	struct ifdesc*      upstrif;
    
	// Get the upstream VIF...
	if (upstreamvif <0 || (upstrif = getifbyix( upstreamvif)) == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "FATAL: Unable to get Upstream IF.\n");
		return;
	}
	else
		dbprintf_igmp(LOG_WARNING, "upstream VIF index %d\n", upstreamvif);
    

	// Send join or leave request...
	if (join)
	{
		// Only join a group if there are listeners downstream...
		if (route->vifbits > 0)
		{
			dbprintf_igmp(LOG_WARNING,  "Joining group %s upstream on IF address %s\n",
				inetfmt(route->group, s1), inetfmt(upstrif->inadr.s_addr, s2));

			//k_join(route->group, upstrIf->InAdr.s_addr);
			joinmcgroup(mproxy.igmp_udp_socket, upstrif, route->group);
			route->upstrstate = ROUTESTATE_JOINED;
		}
		else
		{
			dbprintf_igmp(LOG_WARNING,  "No downstream listeners for group %s. No join sent.\n", inetfmt(route->group, s1));
		}

	}
	else
	{
		// Only leave if group is not left already...
		if (route->upstrstate != ROUTESTATE_NOTJOINED)
		{
			dbprintf_igmp(LOG_WARNING,  "Leaving group %s upstream on IF address %s\n", inetfmt(route->group, s1), inetfmt(upstrif->inadr.s_addr, s2));
            		//k_leave(route->group, upstrIf->InAdr.s_addr);
			leavemcgroup( mproxy.igmp_udp_socket, upstrif, route->group );
			route->upstrstate = ROUTESTATE_NOTJOINED;
		}
	}
}

static void logroutetable(char *header)
{
	struct routetable*  croute = routing_table;
	unsigned            rcount = 0;
    
	dbprintf_igmp(LOG_WARNING, "\n");
	dbprintf_igmp(LOG_WARNING, "current routing table (%s):\n", header);
	dbprintf_igmp(LOG_WARNING, "-----------------------------------------------------\n");
        
	if (croute==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "no routes in table...\n");
        }
	else
	{
		do
		{
			dbprintf_igmp(LOG_WARNING, "#%d: src: %s, dst: %s, age:%d, st: %s, outvifs: 0x%08x\n",
				rcount, inetfmt(croute->originaddr, s1), inetfmt(croute->group, s2),
				croute->agevalue,(croute->originaddr>0?"a":"i"), croute->vifbits);
                  
			croute = croute->nextroute;
			rcount++;
		} while (croute != NULL);
	}
	
	dbprintf_igmp(LOG_WARNING, "-----------------------------------------------------\n");
}

static int internupdatekernelroute(struct routetable *route, int activate) 
{
	struct   mroutedesc     mrdesc;
	struct   ifdesc         *dp;
	unsigned                ix;
    
	if (route->originaddr > 0)
	{

		// build route descriptor from table entry...
		// set the source address and group address...
		mrdesc.mcadr.s_addr     = route->group;
		mrdesc.originadr.s_addr = route->originaddr;
    
		// clear output interfaces 
		memset( mrdesc.ttlvc, 0, sizeof( mrdesc.ttlvc ) );
		dbprintf_igmp(LOG_WARNING, "vif bits : 0x%08x\n", route->vifbits);

		// set the ttl's for the route descriptor...
		for (ix = 0; (dp = getifbyix(ix)); ix++)
		{
			if (dp->state == IF_STATE_UPSTREAM)
			{
                		mrdesc.invif = dp->index;
            		}
			else if (BIT_TST(route->vifbits, dp->index))
			{
				dbprintf_igmp(LOG_WARNING, "setting ttl for vif %d to %d\n", dp->index, dp->threshold);
				mrdesc.ttlvc[ dp->index ] = dp->threshold;
			}
		}
    
		// do the actual kernel route update...
		if (activate)
		{
			// add route in kernel...
			addmroute(&mrdesc);
    		}
		else
		{
			// delete the route from kernel...
			delmroute(&mrdesc);
		}

	}
	else
	{
		dbprintf_igmp(LOG_NOTICE, "route is not active. no kernel updates done.\n");
	}

	return 1;
}

/*
** Builds up a vector with the interface of the machine. Calls to the other functions of 
** the module will fail if they are called before the vector is build.
**          
*/
static int build_mcast_ifvc(void) 
{
	struct ifreq ifvc[CPUPORT_MAX_IF];
	struct ifreq *ifep;

	int sock;

	if ((sock = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
		dbprintf_igmp(LOG_ERR, "RAW socket open\n");

	/* get If vector
	*/
	{
		struct ifconf ioctlreq;
        	ioctlreq.ifc_buf = (void *)ifvc;
		ioctlreq.ifc_len = sizeof( ifvc );

        	if (ioctl( sock, SIOCGIFCONF, &ioctlreq ) < 0)
			dbprintf_igmp(LOG_ERR, "ioctl SIOCGIFCONF err:%d\n",errno);

		if (ioctlreq.ifc_len / sizeof(ifvc[0]) < 3)
		{
			dbprintf_igmp(LOG_ERR, "Interface not configured.\n");
			close(sock);
			return -1;
		}

        	ifep = (void *)((char *)ifvc + ioctlreq.ifc_len);
	}

	uint32_t lan_subnet=0, lan_mask=0;
    	/* loop over interfaces and copy lan and wan interface info to IfDescVc
	*/
	{
		struct ifreq  *ifpt, *ifnext;

        // Temp keepers of interface params...
		uint32_t addr, subnet, mask;

		for (ifpt = ifvc; ifpt < ifep; ifpt = ifnext)
		{
			struct ifreq ifreq;
			char fmtbu[ 32 ];

			ifnext = (struct ifreq *)((char *)&ifpt->ifr_addr + sizeof(struct sockaddr_in));
			if (ifnext < ifpt + 1)
				ifnext = ifpt + 1;
			if (!strcmp(ifpt->ifr_name,omci_env_g.router_wan_if) || !strcmp(ifpt->ifr_name,omci_env_g.router_lan_if))
			{
				if (((struct sockaddr_in *)&ifpt->ifr_addr)->sin_addr.s_addr == 0)
				{
					close(sock);
					return -1;
				}
				if (!strcmp(ifpt->ifr_name,omci_env_g.router_lan_if))
					memcpy(&lanif_info, ifpt, sizeof(struct ifreq));
				dbprintf_igmp(LOG_ERR, "VIF %s found!\n",ifpt->ifr_name);
			}
			else
				continue;
			
			strncpy(ifdescep->name, ifpt->ifr_name, sizeof(ifdescep->name));

			// Currently don't set any allowed nets...
			//IfDescEp->allowednets = NULL;

			// Set the index to -1 by default.
			ifdescep->index = -1;

			/* don't retrieve more info for non-IP interfaces
			*/
			if (ifpt->ifr_addr.sa_family != AF_INET)
			{
				dbprintf_igmp(LOG_ERR, "remove VIF %s non-IP interfaces!\n",ifpt->ifr_name);
				ifdescep->inadr.s_addr = 0;  /* mark as non-ip interface */
				ifdescep++;
				continue;
			}

			// get the interface adress...
			ifdescep->inadr = ((struct sockaddr_in *)&ifpt->ifr_addr)->sin_addr;
			addr = ifdescep->inadr.s_addr;

			memcpy( ifreq.ifr_name, ifdescep->name, sizeof( ifreq.ifr_name ) );
			ifreq.ifr_addr.sa_family = AF_INET;
			((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr = addr;

			// get the subnet mask...
			if (ioctl(sock, SIOCGIFNETMASK, &ifreq ) < 0) dbprintf_igmp(LOG_ERR, "ioctl siocgifnetmask for %s\n",ifreq.ifr_name);
			mask = ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
			subnet = addr & mask;

			/* get if flags
			**
			** typical flags:
			** lo    0x0049 -> Running, Loopback, Up
			** ethx  0x1043 -> Multicast, Running, Broadcast, Up
			** ipppx 0x0091 -> NoArp, PointToPoint, Up 
			** grex  0x00C1 -> NoArp, Running, Up
			** ipipx 0x00C1 -> NoArp, Running, Up
			*/
			if (ioctl(sock, SIOCGIFFLAGS, &ifreq) < 0) dbprintf_igmp( LOG_ERR,  "ioctl SIOCGIFFLAGS\n");

			ifdescep->flags = ifreq.ifr_flags;		    
			ifdescep->allowednets = (struct subnetlist *)malloc_safe(sizeof(struct subnetlist));
			// Create the network address for the IF..
			ifdescep->allowednets->subnet_mask = mask;
			ifdescep->allowednets->subnet_addr = subnet;

			// Set the default params for the IF...
			if (strcmp(ifdescep->name, omci_env_g.router_lan_if)) 
				ifdescep->state         = IF_STATE_UPSTREAM;
			else
			{
				ifdescep->state         = IF_STATE_DOWNSTREAM;
				lan_subnet = subnet; 
				lan_mask = mask;
			}
			ifdescep->robustness    = DEFAULT_ROBUSTNESS;
			ifdescep->threshold     = DEFAULT_THRESHOLD;   /* ttl limit */
			ifdescep->ratelimit     = DEFAULT_RATELIMIT; 
		    

			// Debug log the result...
			dbprintf_igmp(LOG_ERR, "buildIfVc: Interface %s Addr: %s, Flags: 0x%04x, Network: %s\n",
				 ifdescep->name, fmtinadr(fmtbu, ifdescep->inadr), ifdescep->flags, inetfmts(subnet,mask, s1));
			ifdescep++;
		} 
	}

	struct   ifdesc         *dp;
	if ((dp = getifbyname( omci_env_g.router_wan_if)))
	{
		dp->allowednets->subnet_mask = lan_mask;
		dp->allowednets->subnet_addr = lan_subnet;
	}

	close(sock);
	
	return 0;
}

static struct routetable *findroute(uint32_t group)
{
	struct routetable*  croute;

	for (croute = routing_table; croute; croute = croute->nextroute)
	{
		if(croute->group == group)
		{
			return croute;
		}
	}
	
	return NULL;
}

int insertroute(uint32_t group, int ifx)
{
    
	struct routetable*  croute;

	// Sanitycheck the group adress...
	if (!IN_MULTICAST(ntohl(group)))
	{
		dbprintf_igmp(LOG_WARNING, "The group address %s is not a valid Multicast group. Table insert failed.\n",inetfmt(group, s1));
		return 0;
	}

	// Santiycheck the VIF index...
	//if(ifx < 0 || ifx >= MAX_MC_VIFS) {}
	if (ifx >= MAX_MC_VIFS)
	{
		dbprintf_igmp(LOG_WARNING, "The VIF Ix %d is out of range (0-%d). Table insert failed.\n", ifx, MAX_MC_VIFS);
		return 0;
	}

	// Try to find an existing route for this group...
	croute = findroute(group);
	if (croute==NULL)
	{
		struct routetable*  newroute;
		
		dbprintf_igmp(LOG_WARNING, "No existing route for %s. Create new.\n",inetfmt(group, s1));
		// Create and initialize the new route table entry..
		newroute = (struct routetable*)malloc_safe(sizeof(struct routetable));
		// Insert the route desc and clear all pointers...
		newroute->group      = group;
		newroute->originaddr = 0;
		newroute->nextroute  = NULL;
		newroute->prevroute  = NULL;

		// The group is not joined initially.
		newroute->upstrstate = ROUTESTATE_NOTJOINED;
		
		// The route is not active yet, so the age is unimportant.
		newroute->agevalue    = igmp_config_g.robustness + 1;
		newroute->ageactivity = 0;
        
		BIT_ZERO(newroute->agevifbits);     // Initially we assume no listeners.

		// Set the listener flag...
       		BIT_ZERO(newroute->vifbits);    // Initially no listeners...
		if (ifx >= 0) BIT_SET(newroute->vifbits, ifx);

		// Check if there is a table already....
		if (routing_table == NULL)
		{
			// No location set, so insert in on the table top.
			routing_table = newroute;
			dbprintf_igmp(LOG_WARNING, "No routes in table. Insert at beginning.\n");
		}
		else
		{
			dbprintf_igmp(LOG_WARNING, "Found existing routes. Find insert location.\n");

			// Check if the route could be inserted at the beginning...
			if (routing_table->group > group)
			{
				dbprintf_igmp(LOG_WARNING, "Inserting at beginning, before route %s\n", inetfmt(routing_table->group,s1));
				
				// Insert at beginning...
				newroute->nextroute = routing_table;
				newroute->prevroute = NULL;
				routing_table = newroute;

				// If the route has a next node, the previous pointer must be updated.
				if (newroute->nextroute != NULL) newroute->nextroute->prevroute = newroute;
			}
			else
			{
				// Find the location which is closest to the route.
				for (croute = routing_table; croute->nextroute != NULL; croute = croute->nextroute)
				{
					// Find insert position.
					if (croute->nextroute->group > group)
					{
						break;
					}
                		}

                		dbprintf_igmp(LOG_WARNING, "Inserting after route %s\n",inetfmt(croute->group,s1));
                
                		// Insert after current...
                		newroute->nextroute = croute->nextroute;
                		newroute->prevroute = croute;
                		if (croute->nextroute != NULL)
				{
					croute->nextroute->prevroute = newroute; 
				}
				croute->nextroute = newroute;
			}
		}
		
		// Set the new route as the current...
		croute = newroute;
		
		// Log the cleanup in debugmode...
		dbprintf_igmp(LOG_WARNING, "Inserted route table entry for %s on VIF #%d\n", inetfmt(croute->group, s1), ifx);

	}
	else if (ifx >= 0)
	{
		// The route exists already, so just update it.
		BIT_SET(croute->vifbits, ifx);
        	
		// Register the VIF activity for the aging routine
		BIT_SET(croute->agevifbits, ifx);

		// Log the cleanup in debugmode...
		dbprintf_igmp(LOG_WARNING, "Updated route entry for %s on VIF #%d\n", inetfmt(croute->group, s1), ifx);
        
		// If the route is active, it must be reloaded into the Kernel..
		if (croute->originaddr != 0)
		{
			// Update route in kernel...
			if (!internupdatekernelroute(croute, 1))
			{
				dbprintf_igmp(LOG_WARNING, "The insertion into Kernel failed.\n");
				return 0;
			}
        	}
	}

	// Send join message upstream, if the route has no joined flag...
	if (croute->upstrstate != ROUTESTATE_JOINED)
	{
		// Send Join request upstream
		sendjoinleaveupstream(croute, 1);
	}
	
	logroutetable("insert route");
	
	return 1;
}


/**
*   Handles incoming membership reports, and
*   appends them to the routing table.
*/
static void acceptgroupreport(uint32_t src, uint32_t group, uint8_t type)
{
	struct ifdesc  *sourcevif;

	// Sanitycheck the group adress...
	if (!IN_MULTICAST(ntohl(group)))
	{
		dbprintf_igmp(LOG_WARNING, "The group address %s is not a valid Multicast group.\n",inetfmt(group, s1));
		return;
	}

	// Find the interface on which the report was recieved.
	sourcevif = getifbyaddress(src);
	if (sourcevif == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "No interfaces found for source %s\n", inetfmt(src,s1));
		return;
	}
	
	if(sourcevif->inadr.s_addr == src)
	{
		dbprintf_igmp(LOG_WARNING, "The IGMP message was from myself. Ignoring.\n");
		return;
	}

	// We have a IF so check that it's an downstream IF.
	if (sourcevif->state == IF_STATE_DOWNSTREAM)
	{
		dbprintf_igmp(LOG_WARNING, "Should insert group %s (from: %s) to route table. Vif Ix : %d\n", inetfmt(group,s1), inetfmt(src,s2), sourcevif->index);
		
		// The membership report was OK... Insert it into the route table..
		insertroute(group, sourcevif->index);
	}
	else
	{
        	// Log the state of the interface the report was recieved on.
        	dbprintf_igmp(LOG_WARNING, "Mebership report was recieved on %s. Ignoring.\n", sourcevif->state==IF_STATE_UPSTREAM?"the upstream interface":"a disabled interface");
	}
}




static int activateroute(uint32_t group, uint32_t originaddr)
{
	struct routetable *croute=NULL;
	int result = 0;

	// Find the requested route.
	croute = findroute(group);
	if (croute == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "No table entry for %s [From: %s]. Inserting route.\n", inetfmt(group, s1),inetfmt(originaddr, s2));

		// Insert route, but no interfaces have yet requested it downstream.
		insertroute(group, -1);

		// Retrieve the route from table...
		croute = findroute(group);
	}

	if (croute != NULL)
	{
        	// If the origin address is set, update the route data.
		if (originaddr > 0)
		{
			if (croute->originaddr > 0 && croute->originaddr!=originaddr)
				dbprintf_igmp(LOG_WARNING, "The origin for route %s changed from %s to %s\n", inetfmt(croute->group, s1), inetfmt(croute->originaddr, s2), inetfmt(originaddr, s3));
            		croute->originaddr = originaddr;
        	}
		
		// Only update kernel table if there are listeners !
		if (croute->vifbits > 0)
		{
			result = internupdatekernelroute(croute, 1);
		}
	}
	logroutetable("activate route");
	
	return result;
}

int igmp_router_mode_init(void)
{
	struct ip *ip;
	allhosts_group   = htonl(INADDR_ALLHOSTS_GROUP);
	allrouters_group = htonl(INADDR_ALLRTRS_GROUP);

	/*-----------------------------------------------------------------------------
	 *the below is router mode igmp initialization  
	 *-----------------------------------------------------------------------------*/
	if (build_mcast_ifvc())
	{
		dbprintf_igmp(LOG_WARNING, "Fail to build multicast IFVC\n");
		return -1;
	}

	if (mproxy.igmp_socket >= 0)
	{
		close(mproxy.igmp_socket);
		mproxy.igmp_socket = -1;
	}


	/*-----------------------------------------------------------------------------
	 *  initial socket for multicast
	 *-----------------------------------------------------------------------------*/
	int __igmp_socket;
	if ((__igmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
		dbprintf_igmp(LOG_ERR, "create mproxy.igmp_socket error\n");
		goto INIT_ERROR;
	}

	int va = 1;
	if (setsockopt(__igmp_socket, IPPROTO_IP, MRT_INIT, (void *)&va, sizeof(va))) {
		dbprintf_igmp(LOG_ERR, "MRT_INIT error\n");
		goto INIT_ERROR;
	}

	ip         = (struct ip *)send_buf;
	memset(ip, 0, sizeof(struct ip));
	/*
	* Fields zeroed that aren't filled in later:
	* - IP ID (let the kernel fill it in)
	* - Offset (we don't send fragments)
	* - Checksum (let the kernel fill it in)
	*/
	ip->ip_v   = IPVERSION;
	ip->ip_hl  = sizeof(struct ip) >> 2;
	ip->ip_tos = 0xc0;      /* Internet Control */
	ip->ip_ttl = MAXTTL;    /* applies to unicasts only */
	ip->ip_p   = IPPROTO_IGMP;

	/*-----------------------------------------------------------------------------
	 *  IP header include
	 *-----------------------------------------------------------------------------*/
	int hdrincl=1;
	if (setsockopt(__igmp_socket, IPPROTO_IP, IP_HDRINCL, (char *)&hdrincl, sizeof(hdrincl)) < 0)
		dbprintf_igmp(LOG_ERR, "setsockopt IP_HDRINCL %u\n", hdrincl);

	/*-----------------------------------------------------------------------------
	 *  setup multicast TTL
	 *-----------------------------------------------------------------------------*/
	k_set_ttl(__igmp_socket, 1);

	/*-----------------------------------------------------------------------------
	 * setup mproxy.igmp_socket recv buffer 
	 *-----------------------------------------------------------------------------*/
	{
		int bufsize = 256*1024;
		int minsize = 48*1024;
		int delta = bufsize / 2;
		int iter = 0;

		/*
		* Set the socket buffer.  If we can't set it as large as we
		* want, search around to try to find the highest acceptable
		* value.  The highest acceptable value being smaller than
		* minsize is a fatal error.
		*/
		if (setsockopt(__igmp_socket, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) < 0) {
			bufsize -= delta;
			while (1)
			{
				iter++;
				if (delta > 1) delta /= 2;

				if (setsockopt(__igmp_socket, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) < 0)
					bufsize -= delta;
				else
				{
					if (delta < 1024) break;
					bufsize += delta;
				}
			}
			
			if (bufsize < minsize)
			{
				dbprintf_igmp(LOG_ERR, "OS-allowed buffer size %u < app min %u\n",bufsize, minsize);
				/*NOTREACHED*/
			}
		}
		dbprintf_igmp(LOG_WARNING, "Got %d byte buffer size in %d iterations\n", bufsize, iter);
	}



	/*-----------------------------------------------------------------------------
	 *  disable multicast loop
	 *-----------------------------------------------------------------------------*/
	{
		u_char loop;

		loop = 0;
		if (setsockopt(__igmp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop)) < 0)
			dbprintf_igmp(LOG_ERR, "setsockopt IP_MULTICAST_LOOP %u\n", loop);
	}
	/* 
	reate VIFs for all IP, non-loop interfaces
	*/
	{
		unsigned ix;
		struct ifdesc *dp;
		int     vifcount = 0;
		upstreamvif = -1;

		for (ix = 0; (dp = getifbyix(ix)); ix++)
		{
			if (dp->inadr.s_addr && ! (dp->flags & IFF_LOOPBACK))
			{
				if (dp->state == IF_STATE_UPSTREAM)
				{
					if (upstreamvif == -1)
						upstreamvif = ix;
					else
						dbprintf_igmp(LOG_ERR, "Vif #%d was already upstream. Cannot set VIF #%d as upstream as well.\n\n", upstreamvif, ix);
				}
				addvif(__igmp_socket, dp);
				vifcount++;
			}
		}

		// if there is only one vif, or no defined upstream vif, we send an error.
		if (vifcount < 2 || upstreamvif < 0)
			dbprintf_igmp(LOG_ERR, "There must be at least 2 Vif's where one is upstream.\n");
		else
			dbprintf_igmp(LOG_ERR, "vif count: %d upstreamvif: %d\n", vifcount, upstreamvif);	
	}  

	/*-----------------------------------------------------------------------------
	 * initialize group socket for igmp 
	 *-----------------------------------------------------------------------------*/
	int __igmp_udp_socket;
	struct sockaddr_in sockadr;
	if ((__igmp_udp_socket = socket( AF_INET, SOCK_RAW, IPPROTO_IGMP )) < 0)
	{
		dbprintf_igmp(LOG_ERR, "mproxy.igmp_udp_socket error\n");
		goto INIT_ERROR;
	}

	memset(&sockadr, 0, sizeof(sockadr));
	sockadr.sin_family      = AF_INET;
	sockadr.sin_port        = htons(0);
	sockadr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(__igmp_udp_socket, (struct sockaddr *)&sockadr, sizeof(sockadr))) dbprintf_igmp(LOG_ERR, "sock_igmp socket bind error\n");


	/*-----------------------------------------------------------------------------
	 *  initialize routing table
	 *-----------------------------------------------------------------------------*/
	{
		unsigned ix;
		struct ifdesc *dp;

		// clear routing table...
		routing_table = NULL;

		// join the all routers group on downstream vifs...
		for (ix = 0; (dp = getifbyix(ix)); ix++)
		{
			// If this is a downstream vif, we should join the All routers group...
			if(dp->inadr.s_addr && ! (dp->flags & IFF_LOOPBACK) && dp->state == IF_STATE_DOWNSTREAM)
			{
				dbprintf_igmp(LOG_WARNING, "joining all-routers group %s on vif %s\n", inetfmt(allrouters_group,s1),inetfmt(dp->inadr.s_addr,s2));
				//k_join(allrouters_group, Dp->InAdr.s_addr);
				//joinmcgroup( mproxy.igmp_udp_socket, dp, allrouters_group );
			}
		}
	}
	
	if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_IGMP))
	{
		dbprintf_igmp(LOG_ERR, "start igmp intake\n");
		omci_env_g.task_pause_mask &= ~(1<<ENV_TASK_NO_IGMP);
	}

	mproxy.igmp_udp_socket = __igmp_udp_socket;
	mproxy.igmp_socket = __igmp_socket;
	
	return 0;

INIT_ERROR:
	if (__igmp_udp_socket > 0)
		close(__igmp_udp_socket);
	if (__igmp_socket > 0)
		close(__igmp_socket);
	return -1;
}

static int removeroute(struct routetable*  croute)
{
	int result = 1;
    
	// If croute is null, no routes was found.
	if (croute==NULL) return 0;
    
	// Log the cleanup in debugmode...
	dbprintf_igmp(LOG_WARNING, "Removed route entry for %s from table\n", inetfmt(croute->group, s1));

	//BIT_ZERO(croute->vifBits);

	// Uninstall current route from kernel
	if (!internupdatekernelroute(croute, 0))
	{
		dbprintf_igmp(LOG_WARNING, "The removal from Kernel failed\n");
		result = 0;
	}

	// Send Leave request upstream if group is joined
	if (croute->upstrstate == ROUTESTATE_JOINED || (croute->upstrstate == ROUTESTATE_CHECK_LAST_MEMBER && !igmp_config_g.immediate_leave)) 
	{
        	sendjoinleaveupstream(croute, 0);
	}

	// Update pointers...
	if (croute->prevroute == NULL)
	{	// Topmost node...
		if (croute->nextroute != NULL) croute->nextroute->prevroute = NULL; 
        	routing_table = croute->nextroute;
	}
	else
	{
        	croute->prevroute->nextroute = croute->nextroute;
        	if (croute->nextroute != NULL) croute->nextroute->prevroute = croute->prevroute;
    	}
    	// Free the memory, and set the route to NULL...
    	free_safe(croute);
    	croute = NULL;

    	logroutetable("Remove route");

	return result;
}



void setroutelastmembermode(uint32_t group)
{
	struct routetable   *croute;

	croute = findroute(group);
	if (croute != NULL)
	{
        // Check for fast leave mode...
//        if(croute->upstrstate == ROUTESTATE_JOINED && igmp_config_g.immediate_leave) {
            // Send a leave message right away..
		sendjoinleaveupstream(croute, 0);
//        }
        // Set the routingstate to Last member check...
		croute->upstrstate = ROUTESTATE_CHECK_LAST_MEMBER;
        // Set the count value for expiring... (-1 since first aging)
		croute->agevalue = igmp_config_g.lastmemberquerycount;
	}
}

/**
*   Ages a specific route
*/
static int internageroute(struct routetable*  croute)
{
	int result = 0;

	// Drop age by 1.
	croute->agevalue--;

	// Check if there has been any activity...
	if (croute->agevifbits > 0 && croute->ageactivity == 0)
	{
		// There was some activity, check if all registered vifs responded.
		if (croute->vifbits == croute->agevifbits)
		{
			// Everything is in perfect order, so we just update the route age.
			croute->agevalue = igmp_config_g.robustness + 1;
			//croute->ageActivity = 0;
		}
		else
		{
			// One or more VIF has not gotten any response.
			croute->ageactivity++;

			// Update the actual bits for the route...
			croute->vifbits = croute->agevifbits;
		}
	}
	// Check if there have been activity in aging process...
	else if(croute->ageactivity > 0)
	{

		// If the bits are different in this round, we must
		if (croute->vifbits != croute->agevifbits)
		{
			// Or the bits together to insure we don't lose any listeners.
			croute->vifbits |= croute->agevifbits;
			// Register changes in this round as well..
			croute->ageactivity++;
		}
	}

	// If the aging counter has reached zero, its time for updating...
	if (croute->agevalue == 0)
	{
        	// Check for activity in the aging process,
        	if(croute->ageactivity > 0)
		{           
        		dbprintf_igmp(LOG_WARNING, "Updating route after aging : %s\n",inetfmt(croute->group,s1)); 
            
			// Just update the routing settings in kernel...
			internupdatekernelroute(croute, 1);
    
			// We append the activity counter to the age, and continue...
			croute->agevalue = croute->ageactivity;
			croute->ageactivity = 0;
		}
		else
		{
			dbprintf_igmp(LOG_WARNING, "Removing group %s. Died of old age\n",inetfmt(croute->group,s1));
			// No activity was registered within the timelimit, so remove the route.
            		removeroute(croute);
		}
        	// Tell that the route was updated...
        	result = 1;
	}

	// The aging vif bits must be reset for each round...
	BIT_ZERO(croute->agevifbits);
	
	return result;
}

/**
*   Ages groups in the last member check state. If the
*   route is not found, or not in this state, 0 is returned.
*/
static int lastmembergroupage(uint32_t group)
{
	struct routetable   *croute;

	croute = findroute(group);
	if (croute!=NULL)
	{
		if (croute->upstrstate == ROUTESTATE_CHECK_LAST_MEMBER) 
			return !internageroute(croute);
		else 
			return 0;
	}
	return 0;
}

/*
 * Construct an IGMP message in the output packet buffer.  The caller may
 * have already placed data in that buffer, of length 'datalen'.
 */
unsigned int option = 0x94040000;
 
static void buildigmp(uint32_t src, uint32_t dst, int type, int code, uint32_t group, int datalen) 
{
	struct ip *ip;
	struct igmp *igmp;
	extern int curttl;

	ip                      = (struct ip *)send_buf;
	ip->ip_v = 0x4;
	ip->ip_hl = (MIN_IP_HEADER_LEN + sizeof(option)) >> 2;
	ip->ip_src.s_addr       = src;
	ip->ip_dst.s_addr       = dst;
	ip->ip_len = htons(MIN_IP_HEADER_LEN + IGMP_MINLEN + sizeof(option) + datalen);

	if (IN_MULTICAST(ntohl(dst)))
		ip->ip_ttl = curttl;
	else
		ip->ip_ttl = MAXTTL;
    
	// fill router alert option
	unsigned char *p =  (unsigned char *)(send_buf + MIN_IP_HEADER_LEN);
	*((unsigned int *)p) = htonl(option); 
        
	igmp                    = (struct igmp *)(send_buf + MIN_IP_HEADER_LEN + sizeof(option));
	igmp->igmp_type         = type;
	igmp->igmp_code         = (igmp_config_g.igmp_version & IGMP_V1_BIT)?0:code;
	igmp->igmp_group.s_addr = group;
	igmp->igmp_cksum        = 0;
	igmp->igmp_cksum        = inetchksum((u_short *)igmp, IGMP_MINLEN + datalen);
}

/* 
 * Call build_igmp() to build an IGMP message in the output packet buffer.
 * Then send the message from the interface with IP address 'src' to
 * destination 'dst'.
 */
static void sendigmp(uint32_t src, uint32_t dst, int type, int code, uint32_t group, int datalen )
{
	struct sockaddr_in sdst;
	int setloop = 0, setigmpsource = 0;
	
	buildigmp(src, dst, type, code, group, datalen);

	if (IN_MULTICAST(ntohl(dst)))
	{
        	k_set_if(src);
        	setigmpsource = 1;
        	if (dst == allhosts_group)
		{
			setloop = 1;
			k_set_loop(1);
        	}
	}

	memset(&sdst, 0, sizeof(sdst));
	sdst.sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
	sdst.sin_len = sizeof(sdst);
#endif
	sdst.sin_addr.s_addr = dst;
	if (sendto(mproxy.igmp_socket, send_buf,MIN_IP_HEADER_LEN + IGMP_MINLEN + sizeof(option) + datalen, 0, (struct sockaddr *)&sdst, sizeof(sdst)) < 0)
	{
		if (errno == ENETDOWN)
			dbprintf_igmp(LOG_ERR, "Sender VIF was down.\n");
		else
			dbprintf_igmp(LOG_WARNING, "sendto to %s on %s\n", inetfmt(dst, s1), inetfmt(src, s2));
	}

	if (setigmpsource)
	{
        	if (setloop) k_set_loop(0);
        	// Restore original...
        	k_set_if(INADDR_ANY);
	}

	dbprintf_igmp(LOG_WARNING, "SENT %s from %-15s to %s\n", igmppacketkind(type, code), src==INADDR_ANY ? "INADDR_ANY":inetfmt(src, s1), inetfmt(dst, s2));
}

/**
*   Sends a group specific member report query until the 
*   group times out...
*/
void sendgroupspecificmemberquery(void *argument) 
{
	// Cast argument to correct type...
	groupvifdesc   *gvdesc = (groupvifdesc*) argument;

	if(gvdesc->started)
	{
		// If aging returns false, we don't do any further action...
		if(!lastmembergroupage(gvdesc->group)) return;
	}
	else
	{
		gvdesc->started = 1;
	}

	// Send a group specific membership query...
	sendigmp(gvdesc->vifaddr, gvdesc->group, 
		IGMP_MEMBERSHIP_QUERY,
		igmp_config_g.last_member_query_interval, gvdesc->group, 0);

	dbprintf_igmp(LOG_WARNING, "Sent membership query from %s to %s. Delay: %d\n", inetfmt(gvdesc->vifaddr,s1), inetfmt(gvdesc->group,s2), igmp_config_g.last_member_query_interval);

	// Set timeout for next round...
	timer_id_router_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_ROUTER_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100,(void *)gvdesc);
}

void sendgeneralmembershipquery()
{
	struct  ifdesc  *dp;
	int             ix;

	// loop through all downstream vifs...
	for ( ix = 0; (dp = getifbyix(ix)); ix++ )
	{
        	if ( dp->inadr.s_addr && ! (dp->flags & IFF_LOOPBACK) )
		{
			if(dp->state == IF_STATE_DOWNSTREAM)
			{
				// send the membership query...
				sendigmp(dp->inadr.s_addr, allhosts_group, 
                         		IGMP_MEMBERSHIP_QUERY,
                        		igmp_config_g.max_respon_time , 0, 0);
                
                		dbprintf_igmp(LOG_WARNING, "sent membership query from %s to %s. delay: %d\n",
					inetfmt(dp->inadr.s_addr,s1), inetfmt(allhosts_group,s2), igmp_config_g.max_respon_time);
            		}
        	}
	}
}

/**
*   Recieves and handles a group leave message.
*/
void acceptleavemessage(uint32_t src, uint32_t group) 
{
	struct ifdesc   *sourcevif;
    
	dbprintf_igmp(LOG_WARNING, "Got leave message from %s to %s. Starting last member detection.\n",inetfmt(src, s1), inetfmt(group, s2));

	// Sanitycheck the group adress...
	if (!IN_MULTICAST(ntohl(group)))
	{
		dbprintf_igmp(LOG_WARNING, "The group address %s is not a valid Multicast group.\n", inetfmt(group, s1));
		return;
	}

	// Find the interface on which the report was recieved.
	sourcevif = getifbyaddress(src);
	if (sourcevif == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "No interfaces found for source %s\n", inetfmt(src,s1));
		return;
	}

	// We have a IF so check that it's an downstream IF.
	if (sourcevif->state == IF_STATE_DOWNSTREAM)
	{

        	groupvifdesc   *gvdesc;
        	gvdesc = (groupvifdesc*) malloc_safe(sizeof(groupvifdesc));

        	// Tell the route table that we are checking for remaining members...
        	setroutelastmembermode(group);

        	// Call the group spesific membership querier...
        	gvdesc->group = group;
        	gvdesc->vifaddr = sourcevif->inadr.s_addr;
        	gvdesc->started = 0;

        	sendgroupspecificmemberquery(gvdesc);
	}
	else
	{
        	// just ignore the leave request...
        	dbprintf_igmp(LOG_WARNING, "The found if for %s was not downstream. Ignoring leave request.\n", inetfmt(src, s1));
	}
}

/**
*   Handles incoming IGMP V3 membership reports, and
*   appends them to the routing table.
*   only suport 1 record without source address
*/
static void igmpv3_acceptgroupreport(uint32_t src, unsigned char *igmp)
{

	dbprintf_igmp(LOG_WARNING, "IGMP V3 accept\n");
    	if (ntohs( *(unsigned short *)(igmp+ 6)) != 1) //only support 1 record
		return;

	dbprintf_igmp(LOG_WARNING, "\n");

	if ( (*(igmp + 8)  == 1 || *(igmp + 8)  == 3) && ntohs( *(unsigned short *)(igmp+ 10) ) == 0 )
	{ 
		//INCLUDE_NULL == IGMP v2 leave
		acceptleavemessage(src, *(unsigned int *)(igmp+ 12) ); 
		
	}
	else if( (*(igmp + 8)  == 2 || *(igmp + 8)  == 4) && ntohs( *(unsigned short *)(igmp+ 10) ) == 0 )
	{
		//EXCLUDE_NULL == IGMP v2 join
		acceptgroupreport(src, *(unsigned int *)(igmp+ 12), 0x22 );
		activateroute(*(unsigned int *)(igmp+ 12), src);
	}
}



void ageactiveroutes(void) 
{
    struct routetable   *croute, *nroute;
    
    dbprintf_igmp(LOG_WARNING, "Aging routes in table.\n");

    // Scan all routes...
    for (croute = routing_table; croute != NULL; croute = nroute)
    {
        
        // Keep the next route (since current route may be removed)...
        nroute = croute->nextroute;

        // Run the aging round algorithm.
        if(croute->upstrstate != ROUTESTATE_CHECK_LAST_MEMBER)
	{
            // Only age routes if Last member probe is not active...
            internageroute(croute);
        }
    }
    logroutetable("Age active routes\n");
}


/**
 * Process a newly received IGMP packet that is sitting in the input
 * packet buffer.
 */
void acceptigmp(struct cpuport_info_t *pktinfo ) 
{
	register uint32_t src, dst, group;
	struct ip *ip;
	struct igmp *igmp;
	int ipdatalen, iphdrlen, igmpdatalen;

	if (pktinfo->frame_len < sizeof(struct ip))
	{
		dbprintf_igmp(LOG_WARNING, "received packet too short (%u bytes) for IP header\n", pktinfo->frame_len);
		return;
	}

	ip        = (struct ip *)pktinfo->frame_ptr;
	src       = ip->ip_src.s_addr;
	dst       = ip->ip_dst.s_addr;

	/* 
	* this is most likely a message from the kernel indicating that
	* a new src grp pair message has arrived and so, it would be 
	* necessary to install a route into the kernel for this.
	*/
	if (ip->ip_p == 0)
	{
		if (src == 0 || dst == 0)
		{
			dbprintf_igmp(LOG_WARNING,  "kernel request not accurate\n");
		}
		else
		{
			struct ifdesc *checkvif;
			
			// Check if the source address matches a valid address on upstream vif.
			if (upstreamvif <0 || (checkvif = getifbyix(upstreamvif)) == NULL)
			{
				dbprintf_igmp(LOG_ERR,  "Upstream VIF was null.\n");
				return;
			}
			else if (src == checkvif->inadr.s_addr)
			{
				dbprintf_igmp(LOG_ERR,"Route activation request from %s for %s is from myself. Ignoring.\n", inetfmt(src, s1), inetfmt(dst, s2));
				return;
			}
			else if (!isadressvalidforif(checkvif, src))
			{
				dbprintf_igmp(LOG_WARNING, "The source address %s for group %s, is not in any valid net for upstream VIF.\n", inetfmt(src, s1), inetfmt(dst, s2));
				return;
			}
			
			// Activate the route.
			dbprintf_igmp(LOG_WARNING, "Route activate request from %s to %s\n", inetfmt(src,s1), inetfmt(dst,s2));
			activateroute(dst, src);
		}
		return;
	}

	iphdrlen  = ip->ip_hl << 2;
	ipdatalen = ntohs(ip->ip_len) - (ip->ip_hl << 2);

	/*
	if (iphdrlen + ipdatalen != pktinfo->frame_len ) {
		dbprintf_igmp(LOG_WARNING, "received packet from %s shorter (%u bytes) than hdr+data length (%u+%u)\n",
		inetfmt(src, s1), pktinfo->frame_len, iphdrlen, ipdatalen);
		return;
	}
	*/


	igmp        = (struct igmp *)(pktinfo->frame_ptr + iphdrlen);
	group       = igmp->igmp_group.s_addr;
	igmpdatalen = ipdatalen - IGMP_MINLEN;
	if (igmpdatalen < 0)
	{
		dbprintf_igmp(LOG_WARNING, "received IP data field too short (%u bytes) for IGMP, from %s\n", ipdatalen, inetfmt(src, s1));
		return;
	}

	dbprintf_igmp(LOG_WARNING, "RECV %s from %-15s to %s\n", igmppacketkind(igmp->igmp_type, igmp->igmp_code), inetfmt(src, s1), inetfmt(dst, s2));

	switch (igmp->igmp_type)
	{
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
		acceptgroupreport(src, group, igmp->igmp_type);
		return;
	case IGMP_V3_MEMBERSHIP_REPORT:	
		igmpv3_acceptgroupreport(src, (unsigned char *)igmp);
		return;

	case IGMP_V2_LEAVE_GROUP:
		acceptleavemessage(src, group);
		return;

	case IGMP_MEMBERSHIP_QUERY:
		//forward_query(pktinfo);
		return;

	default:
		dbprintf_igmp(LOG_WARNING, "ignoring unknown IGMP message type %x from %s to %s\n", igmp->igmp_type, inetfmt(src, s1), inetfmt(dst, s2));
		return;
	}
}

struct me_t *igmp_router_bridge_port_me_of_lan_get(unsigned char logical_port_id)
{
	if (logical_port_id < 12)
	{
		struct me_t *meptr_422 = NULL; 
		struct meinfo_t *miptr = meinfo_util_miptr(422);
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		        if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
                        if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_CPU)
			{ // VEIP
                                return hwresource_private2public(meptr_422);
                        }
		}
		
		// cant find VEIP, find corresponding UNI instead
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
                        if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
			if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_UNI)
			{ // UNI
			        if (meinfo_util_me_attr_data(meptr_422, 6) != logical_port_id) continue;
			        dbprintf_igmp(LOG_WARNING, "UNI for logical_port_id %d found\n", logical_port_id);
				return hwresource_private2public(meptr_422);
			}
		}
		
		// cant find own UNI, use UNI0 instead (p2p_enable)
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
	               if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
                       if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_UNI)
		       { // UNI
			        if (meinfo_util_me_attr_data(meptr_422, 6) == 0 && omci_env_g.port2port_enable)
			        {
				    dbprintf_igmp(LOG_WARNING, "VEIP/UNI not found. UNI0 used intead\n");
                                    return hwresource_private2public(meptr_422);
				}
			}
		}                		
	}
	return NULL;
}

/**
*   Clear all routes from routing table, and alerts Leaves upstream.
*/
static void clearallroutes() 
{
	struct routetable   *croute, *remainroute;

	// Loop through all routes...
	for(croute = routing_table; croute; croute = remainroute)
	{
		remainroute = croute->nextroute;

		// Log the cleanup in debugmode...
		dbprintf_igmp(LOG_WARNING, "Removing route entry for %s\n", inetfmt(croute->group, s1));

		// Uninstall current route
		if (!internupdatekernelroute(croute, 0)) dbprintf_igmp(LOG_WARNING, "The removal from Kernel failed.\n");

		// Send Leave message upstream.
		sendjoinleaveupstream(croute, 0);

		// Clear memory, and set pointer to next route...
		free_safe(croute);
	}
	routing_table = NULL;

	// Send a notice that the routing table is empty...
	dbprintf_igmp(LOG_WARNING, "All routes removed. Routing table is empty.\n");
}

/*
** Diables the mrouted API and relases by this the lock.
**          
*/
void disablemrouter()
{
	int ret;

	if (mproxy.igmp_socket > 0) {
		int socket = mproxy.igmp_socket;
		mproxy.igmp_socket = -1;
		if ((ret = setsockopt(socket, IPPROTO_IP, MRT_DONE, NULL, 0)) < 0) {
			dbprintf_igmp(LOG_ERR, "failed to set sockopt MRT_DONE."
						"%d, %s\n",
						ret, strerror(ret));
		}
		if ((ret = close(socket)) < 0) {
			dbprintf_igmp(LOG_ERR, "failed to close igmp socket",
						"%d, %s\n",
						ret, strerror(ret));
		}
	}

	if (mproxy.igmp_udp_socket > 0) {
		int socket = mproxy.igmp_udp_socket;
		mproxy.igmp_udp_socket = -1;
		if ((ret = close(socket)) < 0) {
			dbprintf_igmp(LOG_ERR, "failed to close igmp udp socket",
						"%d, %s\n",
						ret, strerror(ret));
		}
	}
}

void igmp_router_mode_deinit(void) 
{
        unsigned ix;
        struct ifdesc *dp=NULL;

	// remove registered vif
/*	{
		for (ix = 0; (dp = getifbyix(ix)); ix++)
		{
			if (dp->inadr.s_addr && ! (dp->flags & IFF_LOOPBACK))
			{
				delvif(dp);
			}
		}
	}
*/
	clearallroutes();       // remove all routes.		
	disablemrouter();       // disable the multirout api

        for (ix = 0; (dp = getifbyix(ix)); ix++)
	{
		if (dp->allowednets) free_safe(dp->allowednets);
	}
	ifdescep = ifdescvc;

	memset (ifdescvc ,0 ,sizeof(struct ifdesc)*MAX_IF);

	struct vifdesc *vifdp;

	/* search free vifdesc
	*/
	for (vifdp = vifdescvc; vifdp < VCEP( vifdescvc ); vifdp++)
	{
		if (vifdp->ifdp) vifdp->ifdp = NULL;
	}
}
