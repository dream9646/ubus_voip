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
 * File    : mcast_snoop.h
 *
 ******************************************************************/

#ifndef __MCAST_SNOOP_H__
#define __MCAST_SNOOP_H__

#ifndef ipaddr_t
#define ipaddr_t unsigned int /* ipv4 address type */
#endif

struct mcast_snoop_src_t{
        union ipaddr src_ip;
        struct list_head host_ssm_list;
        struct list_head entry_node;    
};

struct mcast_snoop_grp_t{
	int flag;
        short vid;
        union ipaddr group_ip;
        struct list_head host_asm_list;
        struct list_head src_list;
        struct list_head entry_node;    
};

extern struct igmp_daemon_t igmpv3_lw;
#endif
