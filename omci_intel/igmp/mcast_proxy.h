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
#ifndef __MCAST_PROXY_H__
#define __MCAST_PROXY_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <errno.h>
#include <string.h>
#include <asm/types.h>

#include "util.h"
#include "list.h"
#include "cpuport.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "mcast_router.h"
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


struct proxy_hpt_msg {
        int timer_id;
        uint32_t mcastaddr; 
};

#define IGMP_HOST_MEMBERSHIP_QUERY 0x11 /* From RFC1112 */
#define IGMP_HOST_MEMBERSHIP_REPORT 0x12 /* Ditto */
#define IGMPV2_HOST_MEMBERSHIP_REPORT 0x16 /* V2 version of 0x11 */
#define IGMP_HOST_LEAVE_MESSAGE 0x17
#define IGMPV3_HOST_MEMBERSHIP_REPORT 0x22 /* V3 version of 0x11 */

#define IGMP_MINLEN 8
#define IGMP_ALL_HOSTS htonl(0xE0000001L)
#define IGMP_ALL_ROUTER htonl(0xE0000002L)

int mcast_proxy_db_deinit();
void mcast_proxy_member_update(struct mcast_mbship_t *m);
void mcast_proxy_acceptigmp(struct cpuport_info_t *pktinfo);
void igmp_proxy_v3_compatibility_update(struct proxy_hpt_msg *msg);
void igmp_v3_lw_router_compatibility_filter(struct igmp_clientinfo_t **igmp_clientinfo);
#endif
