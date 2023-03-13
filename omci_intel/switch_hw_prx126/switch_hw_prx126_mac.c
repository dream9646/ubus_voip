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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_mac.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include


#include "util.h"
#include "util_inet.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "env.h"
#include "er_group_hw_util.h"

#include "intel_px126_util.h"
int
switch_hw_prx126_mac_learning_limit_get(unsigned int port, unsigned int *count, unsigned int *is_drop)
{
	return 0;
}

// 3 cases:
// count == 0, is_drop dont cared	=> mac learn disabled, no mac limit implied
// count == max, is drop == 0		=> mac learn enabled, mac limit exceed action = forward
// count == n, is_drop == 1		=> mac learn enabled, mac limit exceed action = drop
int
switch_hw_prx126_mac_learning_limit_set(unsigned int port, unsigned int count, unsigned int is_drop)
{
	return 0;
}


int
switch_hw_prx126_mac_entry_add(struct switch_mac_tab_entry_t *mac_tab_entry, int is_static)
{
	return 0;
}

int
switch_hw_prx126_mac_entry_del(struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}

int
switch_hw_prx126_mac_entry_update(struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}


int
switch_hw_prx126_mac_entry_get(struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}


int
switch_hw_prx126_mac_l2uc_get_next(int port, unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}

int
switch_hw_prx126_mac_l2mc_get_next(unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}

int
switch_hw_prx126_mac_ipmc_get_next(unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}

int
switch_hw_prx126_mac_table_flush(unsigned int portmap, int is_static)
{
	return 0;
}

// FIXME! STEVE - ipmc entry will be provided...
// type == RTK_LUT_L2UC(0): ucast only
// type == RTK_LUT_L2MC(1): mcast only
// type == RTK_LUT_L3MC(2): ipmc only
// type >= RTK_LUT_END(3):  all
int
switch_hw_prx126_mac_table_print_valid(int fd, int type) 
{
	return 0;
}

// -------------------------------------------------------------------
// MAC aging time set/get
// -------------------------------------------------------------------
int
switch_hw_prx126_mac_aging_set(unsigned int time)
{
	return 0;
}

int
switch_hw_prx126_mac_aging_get(unsigned int *time)
{
	return 0;
}

// -------------------------------------------------------------------
// MAC aging enable/disable by port 
// -------------------------------------------------------------------
int
switch_hw_prx126_mac_aging_enable_set(unsigned int port, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_mac_aging_enable_get(unsigned int port, unsigned int *enable)
{
	return 0;
}

// -------------------------------------------------------------------
// MAC flood enable/disable by port 
// -------------------------------------------------------------------
int
switch_hw_prx126_mac_flood_enable_set(unsigned int port, unsigned int enable, unsigned int learning_ind)
{
	return 0;
}

int
switch_hw_prx126_mac_flood_enable_get(unsigned int port, unsigned int *enable)
{
	return 0;
}
int 
switch_hw_prx126_mac_bridge_inf_create(unsigned int brd_meid)
{
	return  intel_omci_bridge_inf_create(brd_meid);
}
int 
switch_hw_prx126_mac_bridge_inf_del(unsigned int brd_meid)
{
	return intel_omci_bridge_inf_del(brd_meid);
}
int 
switch_hw_prx126_mac_bridge_port_cgf_update(unsigned short meid,struct switch_mac_bp_config_data_upd_data *upd_data,int is_del)
{
	return intel_omci_bridge_port_cfg_update(meid,(void*)upd_data,is_del);

}
int 
switch_hw_prx126_mac_bridge_port_connect(unsigned short br_meid,unsigned short br_port_meid,unsigned char tp_type,unsigned short tp_ptr)
{
	return intel_omci_bridge_port_connect(br_meid,br_port_meid,tp_type,tp_ptr);
}

