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
 * File    : mcast_router.h
 *
 ******************************************************************/

#ifndef __IGMP_ROUTER_H__
#define __IGMP_ROUTER_H__

#define MAX_MC_VIFS    32     // !!! check this const in the specific includes
#define MAX_IF         40     // max. number of interfaces recognized 


/*
 * Limit on length of route data
 */
#define MAX_IP_PACKET_LEN	576
#define MIN_IP_HEADER_LEN	20
#define MAX_IP_HEADER_LEN	60


/*-----------------------------------------------------------------------------
 *  igmp router related definition
 *-----------------------------------------------------------------------------*/

#define IF_STATE_DISABLED      0   // Interface should be ignored.
#define IF_STATE_UPSTREAM      1   // Interface is the upstream interface
#define IF_STATE_DOWNSTREAM    2   // Interface is a downstream interface

// Multicast default values...
#define DEFAULT_ROBUSTNESS     2
#define DEFAULT_THRESHOLD      1
#define DEFAULT_RATELIMIT      0

// Define timer constants (in seconds...)
#define INTERVAL_QUERY          125
#define INTERVAL_QUERY_RESPONSE  10
//#define INTERVAL_QUERY_RESPONSE  10

#define ROUTESTATE_NOTJOINED            0   // The group corresponding to route is not joined
#define ROUTESTATE_JOINED               1   // The group corresponding to route is joined
#define ROUTESTATE_CHECK_LAST_MEMBER    2   // The router is checking for hosts

typedef struct {
    int     igmp_socket;	// downstream
    int     igmp_udp_socket;	// upstream
    int     mld_socket;	// downstream
    int     mld_udp_socket;	// upstream
}mcast_proxy;

struct subnetlist {
    uint32_t              subnet_addr;
    uint32_t              subnet_mask;
    struct subnetlist*  next;
};

struct vif_config_t {
	char		*vif_name;
	unsigned char	ratelimit;
	unsigned char	threshold;
	unsigned char	state;
	struct subnetlist *allowednet;
};

#include <net/if.h>
struct ifdesc {
	char		name[IF_NAMESIZE];
	struct in_addr	inadr;          /* == 0 for non ip interfaces */            
	short          	flags;
	short           state;
	struct subnetlist*  allowednets;
	unsigned int    robustness;
	unsigned char   threshold;   /* ttl limit */
	unsigned int    ratelimit; 
	unsigned int    index;
};

struct mroutedesc {
	struct in_addr	originadr, mcadr;
	short		invif;
	uint8_t		ttlvc[ MAX_MC_VIFS ];
};

// IGMP socket as interface for the mrouted API
// - receives the IGMP messages
extern int mrouterfd;

int enablemrouter( void );
void disablemrouter( void );
//int delmroute( struct mroutedesc * dp );
int getvifix( struct ifdesc *ifdp );
int igmp_router_mode_init(void);
void igmp_router_mode_deinit(void);
void acceptigmp(struct cpuport_info_t *pktinfo ); 
void ageactiveroutes(void); 
void sendgroupspecificmemberquery(void *argument);
struct me_t * igmp_router_bridge_port_me_of_lan_get(unsigned char logical_port_id);
void sendgeneralmembershipquery();

char *fmtinadr( char *st, struct in_addr inadr );
char *inetfmt(uint32_t addr, char *s);
uint16_t inetchksum(uint16_t *addr, int len);
struct ifdesc *getifbyname( const char *ifname );
struct ifdesc *getifbyix(int ix);
struct ifdesc *getifbyaddress( uint32_t ipaddr );
struct ifdesc *getifbyvifindex( unsigned vifindex );
void k_set_if(uint32_t ifa);
void k_set_loop(int l);
#endif
