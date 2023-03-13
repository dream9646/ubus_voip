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

#include "mroute.h"
#include <asm/types.h>
#include <netinet/icmp6.h>
#include "util_inet.h"


/*
 * Exported variables.
 */
char s1[INET6_ADDRSTRLEN];        /* buffers to hold the string representations  */
char s2[INET6_ADDRSTRLEN];        /* of IP addresses, to be passed to inet_fmt() */
char s3[INET6_ADDRSTRLEN];        /* or inet_fmts().                             */
char s4[INET6_ADDRSTRLEN];

#define MAX_MC_VIFS    32     // !!! check this const in the specific includes
#define MAX_IF         40     // max. number of interfaces recognized 

#define inet_ipaddr(ipaddr, s)  util_inet_ntop(IS_IPV6_ADDR(ipaddr)?AF_INET6:AF_INET, &ipaddr, (char *)s, sizeof(s))

/*
 * Limit on length of route data
 */
#define MIN_IP_HEADER_LEN	20


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

#include <net/if.h>

// IGMP socket as interface for the mrouted API
// - receives the IGMP messages

int igmp_router_mode_init(void);
void igmp_router_mode_deinit(void);
void sendgroupspecificmemberquery(void *argument);

char *inetfmt(uint32_t addr, char *s);
uint16_t inetchksum(uint16_t *addr, int len);

// new
typedef enum {
    STATUS_NOK  = -1,
    STATUS_OK   = 0,
} STATUS;

typedef enum {
    INTERFACE_DOWNSTREAM,
    INTERFACE_UPSTREAM
} if_type;

typedef struct mcast_interface {
    struct list_head 		node;
    char                        if_name[IF_NAMESIZE];
    unsigned short		ss_family;
    union ipaddr		if_addr;
    if_type			type;
    int                         if_index;
    int                         if_mtu;

}mcast_interface;

typedef struct {
    int     igmp_version;
    int     mld_version;
    int     igmp_socket;	// downstream
    int     mld_socket;
    int     igmp_udp_socket;	// upstream
    int     mld_udp_socket;
    int     upif_index;
    unsigned char quick_leave;	// not used here
}mcast_proxy;

typedef enum {
    IM_DISABLE = 0,
    IM_IGMPv1  = 1,
    IM_IGMPv2_MLDv1  = 2,
    IM_IGMPv3_MLDv2  = 3,
} im_version;

struct igmphdr
{
	__u8 type;
	__u8 code; /* For newer IGMP */
	__u16 csum;
	__u32 group;
};

struct igmpv3_grec {
	__u8 grec_type;
	__u8 grec_auxwords;
	__u16 grec_nsrcs;
	__u32 grec_mca;
	__u32 grec_src[0];
};

struct igmpv3_report {
	__u8 type;
	__u8 resv1;
	__u16 csum;
	__u16 resv2;
	__u16 ngrec;
	struct igmpv3_grec grec[0];
};

struct mldv2_query_hdr
{
    struct mld_hdr  mldh;
#if BYTE_ORDER == LITTLE_ENDIAN 
    uint8_t qrv:3,
            suppress:1,
            resv:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    uint8_t resv:4,
            suppress:1,
            qrv:3;
#endif
    uint8_t qqic;
    uint16_t nsrcs;
    struct in6_addr srcs[0];
};

struct igmpv3_query {
	__u8 type;
	__u8 code;
	__u16 csum;
	__u32 group;
#if BYTE_ORDER == LITTLE_ENDIAN 
	__u8 qrv:3,
	     suppress:1,
	     resv:4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
	__u8 resv:4,
	     suppress:1,
	     qrv:3;
#endif
	__u8 qqic;
	__u16 nsrcs;
	__u32 srcs[0];
};

struct mld2_grec {
	__u8		grec_type;
	__u8		grec_auxwords;
	__u16		grec_nsrcs;
	struct in6_addr	grec_mca;
	struct in6_addr	grec_src[0];
};

struct mld2_report {
	struct icmp6_hdr	mld2r_hdr;
	struct mld2_grec	mld2r_grec[0];
};

#define mld2r_type		mld2r_hdr.icmp6_type
#define mld2r_resv1		mld2r_hdr.icmp6_code
#define mld2r_cksum		mld2r_hdr.icmp6_cksum
#define mld2r_resv2		mld2r_hdr.icmp6_dataun.icmp6_un_data16[0]
#define mld2r_ngrec		mld2r_hdr.icmp6_dataun.icmp6_un_data16[1]

// combine igmpv3_grec & mld2_grec
struct mcast_grec {
	__u8 grec_type;
	__u8 grec_auxwords;
	__u16 grec_nsrcs;
	union ipaddr grec_mca;
	union ipaddr grec_src[0];
};

typedef struct mcast_report {
	__u8 type;
	__u16 ngrec;
	struct mcast_grec grec[0];
} mcast_report;

typedef enum
{
	IGMP_MODE_V3 = 4,
        IGMP_MODE_V2 = 2,
        IGMP_MODE_V1 = 1
}IGMP_MODE;

struct mcast_proxy_grp_t
{
        union ipaddr group_ip;
        IGMP_MODE mode;         // compatibility mode
        int hpt_timer_v1;       // host present timer for IGMPv1
        int hpt_timer_v2;       // host present timer for IGMPv2
        struct list_head host_asm_list;
        struct list_head src_list;
        struct list_head entry_node;    
};

IGMP_MODE igmp_v3_get_querier_version();
void sendgeneralmembershipquery();
void sendigmpv3generalquery();
void mcast_sendquery(union ipaddr *src, struct mcast_grec *query);
mcast_interface* mcast_interface_find(int family, if_type type);
#endif
