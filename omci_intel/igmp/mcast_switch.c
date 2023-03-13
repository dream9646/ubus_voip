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
 * File    : igmp_switch.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "list.h"
#include "util.h"
#include "switch.h"
#include "mcast.h"
#include "platform.h"


int 
igmp_switch_igmp_extract_set(int enable)
{
	unsigned int i = 0;

	if (switch_hw_g.igmp_extract_enable_set == NULL)
		return -1;

	for (i=0 ; i< SWITCH_LOGICAL_PORT_TOTAL ; i++)
	{
		if (((1<<i) & switch_get_all_logical_portmask_without_cpuext()) == 0)
			continue;
		if (switch_hw_g.igmp_extract_enable_set(i, enable) < 0)
			return -1;
	}
	return 0;
}

int 
igmp_switch_igmp_extract_get(unsigned int *enable)
{
	if (switch_hw_g.igmp_extract_enable_get == NULL)
		return -1;

	return switch_hw_g.igmp_extract_enable_get(0, enable);
}

int 
igmp_switch_mcast_entry_add(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	if (switch_hw_g.ipmcastaddr_add == NULL)
		return -1;

	if( switch_hw_g.ipmcastaddr_add(mac_tab_entry) < 0)
	{
		dbprintf_igmp(LOG_WARNING,"igmp multicast entry add err\n");
		return -1;
	}

	return 0;
}

int 
igmp_switch_mcast_entry_del(struct switch_mac_tab_entry_t *mac_tab_entry) 
{

	if (switch_hw_g.ipmcastaddr_del == NULL)
		return -1;

	if (switch_hw_g.ipmcastaddr_del(mac_tab_entry) < 0)
	{
		dbprintf_igmp(LOG_WARNING,"igmp multicast entry del err\n");
		return -1;
	}

	return 0;
}


int 
igmp_switch_mcast_entry_update(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	if (switch_hw_g.ipmcastaddr_add == NULL)
		return -1;

	if (switch_hw_g.ipmcastaddr_add (mac_tab_entry) < 0)
		return -1;

	return 0;
}

int 
igmp_switch_mcast_entry_get(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	return switch_hw_g.ipmcastaddr_get(mac_tab_entry);
}

int igmp_switch_ratelimit_set(int port, int upstream_rate)
{
	if (upstream_rate == 0 || upstream_rate > 250)
		upstream_rate = 250;		
	if (switch_hw_g.igmp_upstream_rate_set)
	{
		return switch_hw_g.igmp_upstream_rate_set(port, upstream_rate);
	}	
	return 0;	     
}

int 
igmp_switch_init(void)
{
	// igmp_proxy_snooping=1 is proxy only, no snooping, allow multicast floods to UNI
	if (THREAD_ENV.igmp_proxy_snooping == 0 || THREAD_ENV.igmp_proxy_snooping == 2)
	{
		if (switch_hw_g.unknown_mcast_drop_set == NULL)
			return -2;

		if (switch_hw_g.unknown_mcast_drop_set(SWITCH_ENABLED) < 0)
			return -1;
	}
	switch_hw_g.ipmcast_mode_set();

	if (igmp_switch_igmp_extract_set(SWITCH_ENABLED) < 0)
		dbprintf_igmp(LOG_ERR,"igmp extract error\n");

	return 0;
}

int 
igmp_switch_shutdown(void)
{
	igmp_switch_igmp_extract_set(SWITCH_DISABLED);

	if(switch_hw_g.unknown_mcast_drop_set(SWITCH_DISABLED) < 0)
		return -1;

	switch_hw_g.ipmcastaddr_flush();
	switch_hw_g.ipmcast_mode_clear();

	return 0;
}


