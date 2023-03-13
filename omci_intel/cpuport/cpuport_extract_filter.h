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
 * Module  : cpuport
 * File    : cpuport_extract_filter.h
 *
 ******************************************************************/

#ifndef __CPUPORT_EXTRACT_FILTER_H__
#define __CPUPORT_EXTRACT_FILTER_H__

enum cpuport_extract_type {
	CPUPORT_EXTRACT_IGMP_JOIN_LEAVE = 1,
	CPUPORT_EXTRACT_IGMP_QUERY,
	CPUPORT_EXTRACT_LOOPDETECT,
	CPUPORT_EXTRACT_PPPOE,
	CPUPORT_EXTRACT_DHCP,
	CPUPORT_EXTRACT_BPDU,
	CPUPORT_EXTRACT_8021X,
	CPUPORT_EXTRACT_8021AG,
	CPUPORT_EXTRACT_ARP,
	CPUPORT_EXTRACT_MLD_JOIN_LEAVE,
	CPUPORT_EXTRACT_MLD_QUERY,
	CPUPORT_EXTRACT_EXT_OAM,
	CPUPORT_EXTRACT_LLDP
};

struct cpuport_extract_filter_t {
	unsigned char pkt_type;
	char pkt_type_desc[32];
	int (*filter_cb)( struct cpuport_info_t *pktinfo);
	int pkt_qid;
	struct list_head node;
};

extern struct list_head cpuport_extract_filter_g;

/* cpuport_extract_filter.c */
int cpuport_extract_filter_init(void);
int cpuport_extract_filter_shutdown(void);
int cpuport_extract_filter_register(unsigned char pkt_type, char *pkt_type_desc, int (*cpuport_extract_filter_cb)(struct cpuport_info_t *pktinfo), int msg_qid);
int cpuport_extract_filter_unregister(unsigned char pkt_type);
int cpuport_extract_filter_dump(int fd);
#endif
