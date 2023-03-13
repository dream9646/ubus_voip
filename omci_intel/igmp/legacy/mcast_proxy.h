/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
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
 * File    : mcast_proxy.h
 *
 ******************************************************************/
#ifndef __IGMP_V3_LW_ROUTER_H__
#define __IGMP_V3_LW_ROUTER_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <string.h>
#include <asm/types.h>

#include "util.h"
#include "list.h"
#include "cpuport.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "mcast_proxy.h"
#include "igmp_mbship.h"

/*
 * IGMPv1/v2 query and host report format.
 */
struct igmp {
	u_char		igmp_type;	/* version & type of IGMP message  */
	u_char		igmp_code;	/* subtype for routing msgs        */
	u_short		igmp_cksum;	/* IP-style checksum               */
	struct in_addr	igmp_group;	/* group address being reported    */
};					/*  (zero for queries)             */

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

struct proxy_hpt_msg {
        int timer_id;
        uint32_t mcastaddr; 
};


/* V3 group record types [grec_type] */
#define IGMPV3_MODE_IS_INCLUDE		1
#define IGMPV3_MODE_IS_EXCLUDE		2
#define IGMPV3_CHANGE_TO_INCLUDE	3
#define IGMPV3_CHANGE_TO_EXCLUDE	4
#define IGMPV3_ALLOW_NEW_SOURCES	5
#define IGMPV3_BLOCK_OLD_SOURCES	6

#define IGMP_HOST_MEMBERSHIP_QUERY 0x11 /* From RFC1112 */
#define IGMP_HOST_MEMBERSHIP_REPORT 0x12 /* Ditto */
#define IGMPV2_HOST_MEMBERSHIP_REPORT 0x16 /* V2 version of 0x11 */
#define IGMP_HOST_LEAVE_MESSAGE 0x17
#define IGMPV3_HOST_MEMBERSHIP_REPORT 0x22 /* V3 version of 0x11 */

#define IGMP_MINLEN 8
#define IGMP_ALL_HOSTS htonl(0xE0000001L)
#define IGMP_ALL_ROUTER htonl(0xE0000002L)

void sendigmpv3generalquery();
int mcast_proxy_db_deinit();
void igmp_v3_lw_router_member_update(struct igmp_mbship_t *m);
void igmp_v3_lw_acceptigmp(struct cpuport_info_t *pktinfo);
void igmp_proxy_v3_compatibility_update(struct proxy_hpt_msg *msg);
void igmp_v3_lw_router_compatibility_filter(struct igmp_clientinfo_t **igmp_clientinfo);
#endif
