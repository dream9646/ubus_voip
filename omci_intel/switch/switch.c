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
 * Module  : switch
 * File    : switch.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>

#include "env.h"
#include "meinfo.h"
#include "hwresource.h"
#include "me_related.h"
#include "util.h"
#include "conv.h"
#include "switch.h"
#include "util_run.h"

#ifndef X86
#include "er_group_hw_util.h"
#endif

#define RATE_SIZE	8

extern struct switch_hw_t switch_null_g;
struct switch_hw_t switch_hw_g = {0};

void
switch_init_wan_logical_portid(void)
{
	struct me_t *meptr_422 = NULL; 
	struct meinfo_t *miptr = meinfo_util_miptr(422);
	struct attr_value_t attr_value = {0,NULL};

	attr_value.data = omci_env_g.wan_logical_portid;
	list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node)
	{
		if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_WAN ||
			meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_DS_BCAST)
		{
			meinfo_me_attr_set(meptr_422, 6, &attr_value, 0); //assign logical port id
		}
	}	
	return;
}

static unsigned int switch_linkup_logical_portmask = 0;
void
switch_update_linkup_logical_portmask(void)
{
	unsigned int all_portmask = switch_get_all_logical_portmask();
	unsigned int portmask = 0;
	int i;
	
	for (i=0; i< SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & all_portmask) {
			struct switch_port_mac_config_t port_mac_status;
			if ((1<<i) & switch_get_cpu_logical_portmask()) {
				portmask |= (1<<i);     // cpu related port is always link up
			} else {
				memset(&port_mac_status,0,sizeof(struct switch_port_mac_config_t));
				if (switch_hw_g.port_status_get(i, &port_mac_status) == 0) {
					if (port_mac_status.link) 
						portmask |= (1<<i);	// link up
				} else {
					portmask |= (1<<i);	// treat err as up			
				}
			}
		}
	}
	switch_linkup_logical_portmask = portmask;
}		

unsigned int 
switch_get_linkup_logical_portmask(void)
{
	return switch_linkup_logical_portmask;
}
		
unsigned int
switch_get_dmac_logical_portmask(char *macaddr)
{
	struct switch_mac_tab_entry_t mac_tab_entry;

	memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	memcpy(mac_tab_entry.mac_addr, macaddr, IFHWADDRLEN);

	// Check if target MAC is in MAC table, if it is, forward LTM, otherwise drop LTM
	if (switch_hw_g.mac_entry_get && switch_hw_g.mac_entry_get(&mac_tab_entry) == 0) {
	    	return mac_tab_entry.port_bitmap;
	}
	return 0;
}

unsigned int 
switch_get_wan_logical_portmask(void)
{
	return 1 << omci_env_g.wan_logical_portid;
}
unsigned int
switch_get_uni_logical_portmask(void)
{
	return omci_env_g.uni_logical_portmask;
}
unsigned int
switch_get_cpu_logical_portmask(void)
{
	return omci_env_g.cpu_logical_portmask;
}
unsigned int
switch_get_wifi_logical_portmask(void)
{
	return omci_env_g.wifi_logical_portmask;
}
unsigned int
switch_get_all_logical_portmask(void)
{
	return (1 << omci_env_g.wan_logical_portid) | omci_env_g.uni_logical_portmask | omci_env_g.cpu_logical_portmask | omci_env_g.wifi_logical_portmask;
}
unsigned int
switch_get_all_logical_portmask_without_cpuext(void)
{
	unsigned int cpuext_portmask = omci_env_g.cpu_logical_portmask & (~(1<<switch_get_cpu_logical_portid(0)));
	return switch_get_all_logical_portmask() & (~cpuext_portmask);
}
char
switch_get_wan_logical_portid(void)
{
	return omci_env_g.wan_logical_portid;
}
char
switch_get_uni_logical_portid(unsigned char uni_index)
{
	int i, n = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & omci_env_g.uni_logical_portmask) {
			if (uni_index == n)
				return i;
			n++;
		}
	}
	return -1;
}
char
switch_get_cpu_logical_portid(unsigned char cpu_index)
{
	int i, n = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & omci_env_g.cpu_logical_portmask) {
			if (cpu_index == n)
				return i;
			n++;
		}
	}
	return -1;
}
char
switch_get_wifi_logical_portid(unsigned char wifi_index)
{
	int i, n = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & omci_env_g.wifi_logical_portmask) {
			if (wifi_index == n)
				return i;
			n++;
		}
	}
	return -1;
}

char *
switch_get_logical_portname(unsigned int logical_portid)
{
	static char buff[4][10] = {{0}, {0}, {0}, {0}};
	static int wheel = 0;
	char *portname;
	int i, index = 0;

	wheel = (wheel+1)%4;	// support multiple call from same caller
	portname = buff[wheel];

	if (logical_portid == omci_env_g.wan_logical_portid) {
		return "wan";

	} else if ((1<<logical_portid) & omci_env_g.wifi_logical_portmask) {
		// as wifi could be on cpu port or uni port(rgmii), so wifi should be check first
		for (i=0; i<logical_portid; i++) {
			if ((1<<i) & omci_env_g.wifi_logical_portmask)
				index++;
		}
		snprintf(portname, 9, "wifi%d", index);
		return portname;
	
	} else if ((1<<logical_portid) & omci_env_g.cpu_logical_portmask) {
		for (i=0; i<logical_portid; i++) {
			if ((1<<i) & omci_env_g.cpu_logical_portmask)
				index++;
		}
		snprintf(portname, 9, "cpu%d", index);
		return portname;

	} else if ((1<<logical_portid) & omci_env_g.uni_logical_portmask) {
		for (i=0; i<logical_portid; i++) {
			if ((1<<i) & omci_env_g.uni_logical_portmask)
				index++;
		}
		snprintf(portname, 9, "uni%d", index);
		return portname;
	}
	
	snprintf(portname, 9, "unknown%d", logical_portid);
	return portname;
}


// switch_get_port_info_by_...
// return 0: found, <0: error or not found

int
switch_get_port_info_by_me(struct me_t *me, struct switch_port_info_t *port_info)
{
	struct me_t *pptp_eth_uni_me, *private_pptp_eth_uni_me;
	struct me_t *bridge_port_me, *private_bridge_port_me;
	struct attr_value_t attr_value, attr_value_type, attr_value_meid;

	if (me == NULL || port_info == NULL)
	{
		dbprintf(LOG_ERR, "parameter NULL pointer\n");
		return -1;
	}

	//first, set internal sw id and bridge port me
	memset(port_info, 0x00, sizeof(struct switch_port_info_t));
	switch(me->classid)
	{
	case 11: //pptp ethernet uni
		if ((bridge_port_me = me_related_find_child_me(me, 47)) != NULL)
		{
			//bridge port me
			port_info->bridge_port_me = bridge_port_me;
		}
		if ((private_pptp_eth_uni_me=hwresource_public2private(me))==NULL)
		{
			dbprintf(LOG_WARNING, "public to private me error: classid=%u, meid=%u\n", 
				me->classid, me->meid);
			return -1;
		}
		break;
	case 47: //bridge port
		//bridge port me
		port_info->bridge_port_me = me;

		//check pointer to pptp eth uni
		meinfo_me_attr_get(me, 3, &attr_value_type); //type
		meinfo_me_attr_get(me, 4, &attr_value_meid); //pointer to meid
		if (attr_value_type.data != 1 || (pptp_eth_uni_me = meinfo_me_get_by_meid(11, attr_value_meid.data)) == NULL)
		{
			//not pointer to pptp eth uni, get info by private bridge port
			if ((private_bridge_port_me=hwresource_public2private(me))==NULL)
			{
				dbprintf(LOG_WARNING, "public to private me error: classid=%u, meid=%u\n", 
					me->classid, me->meid);
				return -1;
			}
			meinfo_me_attr_get(private_bridge_port_me, 4, &attr_value);
			port_info->port_type = attr_value.data;
			meinfo_me_attr_get(private_bridge_port_me, 5, &attr_value);
			port_info->port_type_index = attr_value.data;
			meinfo_me_attr_get(private_bridge_port_me, 6, &attr_value);
			port_info->logical_port_id = attr_value.data;

			dbprintf(LOG_DEBUG, "ani port: classid=%u, meid=%u, type=%u, index=%u, logical_port_id=%u\n", 
				me->classid, me->meid, port_info->port_type, port_info->port_type_index, port_info->logical_port_id);

			return 0;
		}

		//get private pptp ethernet uni
		if ((private_pptp_eth_uni_me=hwresource_public2private(pptp_eth_uni_me))==NULL)
		{
			dbprintf(LOG_WARNING, "public to private me error: classid=%u, meid=%u\n", 
				pptp_eth_uni_me->classid, pptp_eth_uni_me->meid);
			return -1;
		}
		break;
	case 413: //private pptp ethernet uni
		if ((pptp_eth_uni_me=hwresource_private2public(me)) != NULL)
		{
			if ((bridge_port_me = me_related_find_child_me(pptp_eth_uni_me, 47)) != NULL)
			{
				//bridge port me
				port_info->bridge_port_me = bridge_port_me;
			}
		}

		private_pptp_eth_uni_me = me;
		break;
	case 422: //private bridge port
		//get public 
		if ((bridge_port_me=hwresource_private2public(me)) != NULL)
		{
			//bridge port me
			port_info->bridge_port_me = bridge_port_me;
		} else {
			dbprintf(LOG_NOTICE, "private to public me error: classid=%u, meid=%u\n", 
				me->classid, me->meid);
			return -1;
		}

		//check pointer to pptp eth uni
		meinfo_me_attr_get(bridge_port_me, 3, &attr_value_type); //type
		meinfo_me_attr_get(bridge_port_me, 4, &attr_value_meid); //pointer to meid
		if (attr_value_type.data != 1 || (pptp_eth_uni_me = meinfo_me_get_by_meid(11, attr_value_meid.data)) == NULL)
		{
			//not pointer to pptp eth uni, get info by private bridge port
			meinfo_me_attr_get(me, 4, &attr_value);
			port_info->port_type = attr_value.data;
			meinfo_me_attr_get(me, 5, &attr_value);
			port_info->port_type_index = attr_value.data;
			meinfo_me_attr_get(me, 6, &attr_value);
			port_info->logical_port_id = attr_value.data;

			dbprintf(LOG_DEBUG, "ani port: classid=%u, meid=%u, type=%u, index=%u, logical_port_id=%u\n", 
				me->classid, me->meid, port_info->port_type, port_info->port_type_index, port_info->logical_port_id);

			return 0;
		}

		//get private pptp ethernet uni
		if ((private_pptp_eth_uni_me=hwresource_public2private(pptp_eth_uni_me))==NULL)
		{
			dbprintf(LOG_WARNING, "public to private me error: classid=%u, meid=%u\n", 
				pptp_eth_uni_me->classid, pptp_eth_uni_me->meid);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "disallowed class: classid=%u, meid=%u\n", 
			me->classid, me->meid);

		return -1;
	}
	
	port_info->port_type = ENV_BRIDGE_PORT_TYPE_UNI; //uni port
	port_info->port_type_index = private_pptp_eth_uni_me->instance_num; //index
	meinfo_me_attr_get(private_pptp_eth_uni_me, 4, &attr_value);
	port_info->logical_port_id = attr_value.data;

	dbprintf(LOG_DEBUG, "uni port: classid=%u, meid=%u, type=%u, index=%u, logical_port_id=%u\n", 
				me->classid, me->meid, port_info->port_type, port_info->port_type_index, port_info->logical_port_id);

	return 0;
}

int
switch_get_port_info_by_logical_port_id(unsigned int logical_port_id, struct switch_port_info_t *port_info)
{
	struct meinfo_t *miptr_47;
	struct me_t *bridge_port_me;
	struct switch_port_info_t port_info2;		

	if (port_info == NULL) {
		dbprintf(LOG_ERR, "parameter NULL pointer\n");
		return -1;
	}

	//first, set internal sw id and bridge port me
	memset(port_info, 0x00, sizeof(struct switch_port_info_t));
	
	//traverse all bport me
	if ((miptr_47 = meinfo_util_miptr(47)) == NULL) // 47 bridge port me
	{
		dbprintf(LOG_ERR, "get 47 miptr error\n");
		return -1;
	}

	list_for_each_entry(bridge_port_me, &miptr_47->me_instance_list, instance_node) {
		if (switch_get_port_info_by_me(bridge_port_me, &port_info2)==0) {
			if (port_info2.logical_port_id == logical_port_id) {
				*port_info = port_info2;
				return 0;
			}
		}
	}

	port_info2.bridge_port_me = NULL;
	port_info2.logical_port_id = logical_port_id;
	if ((1<<logical_port_id) & (switch_get_uni_logical_portmask() | switch_get_wifi_logical_portmask())) {
		unsigned int uni_portmask = (switch_get_uni_logical_portmask() | switch_get_wifi_logical_portmask());
		int i, uni_index = -1;
		port_info2.port_type = 1; // 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
		for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
			if ((1<<i) & uni_portmask) {
				uni_index++;
				if (i == logical_port_id)
					port_info2.port_type_index = uni_index;
			}
		}
	}
	else if (logical_port_id == switch_get_wan_logical_portid()) {
		port_info2.port_type = 2; // 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
		port_info2.port_type_index = 0; // FIXME
	}
	else if (logical_port_id == switch_get_cpu_logical_portid(0)) {
		port_info2.port_type = 0; // 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
		port_info2.port_type_index = 0;  //?�FIXME
	}
	else {
		dbprintf(LOG_WARNING, "unknown logical_port_id = %d\n", logical_port_id);
		return -1;
	}
	dbprintf(LOG_WARNING, "warning: no me related to logical_port_id=%u\n", logical_port_id);
	return 0;	// while me not found, but this logical port found & related port_info is available, so return 0
}

int
switch_get_port_info_by_port_type_index(int port_type, int port_type_index, struct switch_port_info_t *port_info)
{
	struct meinfo_t *miptr_422;
	struct me_t *private_bport_me;
	struct switch_port_info_t port_info2;
	struct attr_value_t attr_value;

	if (port_info == NULL)
		return -1;

	//first, set internal sw id and bridge port me
	memset(port_info, 0x00, sizeof(struct switch_port_info_t));
	
	//traverse all bport me
	if ((miptr_422 = meinfo_util_miptr(422)) == NULL) // 422 pptp uni
	{
		dbprintf(LOG_ERR, "get 422 miptr error\n");
		return -1;
	}

	list_for_each_entry(private_bport_me, &miptr_422->me_instance_list, instance_node) {
		meinfo_me_attr_get(private_bport_me, 4, &attr_value);	// port_type
		if (port_type != attr_value.data)
			continue;
		meinfo_me_attr_get(private_bport_me, 5, &attr_value);	// port_type_index
		if (port_type_index != attr_value.data)
			continue;
		if (switch_get_port_info_by_me(private_bport_me, &port_info2)==0) {
			*port_info = port_info2;
			return 0;
		}
	}	

	dbprintf(LOG_ERR, "fatal error: port_info not found for port_type %d, port_type_index %d\n", port_type, port_type_index);
	return -1;	// not found
}

int
switch_get_port_info_by_uni_num(unsigned char uni_num, struct switch_port_info_t *port_info)
{
	struct meinfo_t *miptr_11;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info2;		

	if (port_info == NULL)
		return -1;

	//first, set internal sw id and bridge port me
	memset(port_info, 0x00, sizeof(struct switch_port_info_t));
	
	//traverse all bport me
	if ((miptr_11 = meinfo_util_miptr(11)) == NULL) // 11 pptp uni
	{
		dbprintf(LOG_ERR, "get 11 miptr error\n");
		return -1;
	}

	list_for_each_entry(pptp_uni_me, &miptr_11->me_instance_list, instance_node) {
		if (pptp_uni_me->instance_num == uni_num) {
			if (switch_get_port_info_by_me(pptp_uni_me, &port_info2)==0) {
				*port_info = port_info2;
				return 0;
			}
		}
	}	

	dbprintf(LOG_ERR, "fatal error: have no this uni num=%u\n", uni_num);
	return -1;	// not found
}

static int
switch_hw_empty_func(void)
{
	return 0;
}

int
switch_hw_register(struct switch_hw_t *switch_hw)
{
	void **base = (void**)&switch_hw_g;
	int i, total = sizeof(struct switch_hw_t)/sizeof(void*);	

	if (switch_hw) {
		switch_hw_g = *switch_hw;
		for (i=0; i<total; i++) {
			if (base[i] == NULL)
				base[i] = (void *)switch_hw_empty_func;
		}
	} else {
		switch_hw_g = switch_null_g;
		for (i=0; i<total; i++) {
			if (base[i] == NULL)
				base[i] = (void *)switch_hw_empty_func;
		}
	}
	return 0;
}

int
switch_hw_unregister()
{
	return switch_hw_register(NULL);
}

#ifndef X86
static int
switch_rldp_uni_port_disable(unsigned int portmask, unsigned int *block_mask)
{
	int portid, ret=0;

	// only care uni portmask
	portmask &= switch_get_uni_logical_portmask();

	for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if ((1<<portid) & portmask) {
			*block_mask |= ( 1<<portid);
			// block this port
			if (switch_hw_g.port_enable_set && (ret = switch_hw_g.port_enable_set(portid, 0) < 0))
				dbprintf(LOG_ERR, "error, ret=%d\n", ret);
		}
	}
	return ret;
}

static int
switch_rldp_uni_port_enable(unsigned int block_mask)
{
	int portid, ret=0;
	
	// only care uni portmask
	block_mask &= switch_get_uni_logical_portmask();

	for( portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if ((1<<portid) & block_mask) {
			// enable block port
			if (switch_hw_g.port_enable_set && (ret = switch_hw_g.port_enable_set(portid, 1) < 0))
				dbprintf(LOG_ERR, "error, ret=%d\n", ret);
		}
	}
	return ret;
}
#endif

int
switch_loop_detect_mask_get(unsigned int *portmask)
{
	if (switch_hw_g.rldp_loopedportmask_get == NULL) 
		return -1;

	return switch_hw_g.rldp_loopedportmask_get(portmask);
}

int
switch_loop_block_time_get(unsigned int *loop_block_time)
{
	//todo need to discuss
	//if (switch_hw_g.rldp_loop_block_time_get == NULL) 
	//	return -1;
	//return switch_hw_g.rldp_loop_block_time_get(loop_block_time);
	*loop_block_time=60;     //default 60 seconds

	return 0;
}

int
switch_loop_block_time_set(unsigned int loop_block_time)
{
	return 0;
}

#ifndef X86
//loop detection, action is controled by omci_env_g.rldp_enable
long block_start_time;
void 
switch_loop_detection()
{
	static unsigned int block_mask=0;
	unsigned int portmask = 0;

	if (block_mask == 0 && omci_env_g.rldp_enable >=2) {	// show msg if >=2
		int ret;
		if ((ret=switch_loop_detect_mask_get(&portmask)) == 0) {
			// only care linkup ports
			portmask &= switch_get_linkup_logical_portmask();
			if (portmask) {	//portmask in driver(software) will keep rg_db.systemGlobal.rldp_loop_block_time sec
				dbprintf(LOG_ERR, "loop detected, port mask=0x%x\n", portmask);
				if (omci_env_g.rldp_enable >= 3) {	// block if >=3
					switch_rldp_uni_port_disable(portmask, &block_mask);
					dbprintf(LOG_ERR, "port block [block_mask=0x%x]\n", block_mask);
					block_start_time= util_get_uptime_sec();
				}
			}
		} else {
			dbprintf(LOG_WARNING, "loop detection error %d\n", ret);
		}
	}
	
	if (block_mask) {
		int loop_block_time;
		switch_loop_block_time_get(&loop_block_time);
		// block rg_db.systemGlobal.rldp_loop_block_time secs
		if (util_get_uptime_sec() - block_start_time > loop_block_time+3 ) {
			switch_rldp_uni_port_enable(block_mask);
			dbprintf(LOG_ERR, "port unblock [block_mask=0x%x]\n", block_mask);
			block_mask = 0;
		}
	}
}
#endif

int
switch_loop_detect_set(unsigned int enable, unsigned int portmask)
{
        int option=1;

	if(omci_env_g.rldp_enable==0) {
		dbprintf(LOG_ERR, "exit process because omci_env_g.rldp_enable=0\n");
		return 0;
	}

	if (switch_hw_g.rldp_enable_set == NULL || switch_hw_g.rldp_portmask_set == NULL) 
		return -1;

	// for RTL9600_SERIES, phy port 5 needs to be enabled if any port rldp is enabled, 
	// refer to end of switch_hw_fvt2510_rldp_portmask_set()
	switch_hw_g.rldp_portmask_set(portmask);
	if (portmask && enable) {
		switch_hw_g.rldp_enable_set(option);
	} else {
		switch_hw_g.rldp_enable_set(0);
	}

	return 0;
}

int 
switch_loop_detect_interval_set(unsigned int interval_check, unsigned int interval_loop)
{
	int ret = 0;
	if (switch_hw_g.rldp_time_period_set == NULL) 
		return -1;

	if((ret=switch_hw_g.rldp_time_period_set(interval_check, interval_loop)) !=0) {
		dbprintf(LOG_ERR, "switch_hw_g.rldp_time_period_set error=%d\n", ret);
		return -1;
	}
	return ret;
}

int
switch_get_port_config(int logical_port_id, struct switch_port_mac_config_t* port_mac_config)
{
	int ret;

	memset( port_mac_config, 0, sizeof(struct switch_port_mac_config_t));
	if ((ret = switch_hw_g.port_status_get(logical_port_id, port_mac_config)) !=0 ) {
		dbprintf_lldp(LOG_ERR, "ret = %d\n", ret);
		return -1;
	}

	return 0;
}

#define METAFILE_PATH	"/etc/wwwctrl/metafile.dat"

int
switch_get_system_hw_intf_lan_vid(void)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char *value_ptr;
		
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_safe(&kv_config, METAFILE_PATH);
	if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, "system_hw_intf_lan_vid", 1)) > 0)
	{
		//overwrite to omci env
		omci_env_g.classf_hw_intf_lan_vid = util_atoi(value_ptr);
		metacfg_adapter_config_release(&kv_config);
		return 0;
	} else {
		//do nothing
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}
#else
	return 0;
#endif
}
