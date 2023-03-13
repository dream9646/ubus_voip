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
 * Module  : meinfo_hw
 * File    : meinfo_hw_134.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "conv.h"
#include "iphost.h"
#include "wanif.h"

static int
update_me_with_hwconfig(struct me_t *me, unsigned char attr_mask[2], int is_avc)
{
	char *ifname = meinfo_util_get_config_devname(me);	// get iphost devname as ifname
	struct attr_value_t attr;
	
	if (ifname == NULL)
		return -1;

	if (util_attr_mask_get_bit(attr_mask, 1)) {		// ip option
		unsigned char ipopt = meinfo_util_me_attr_data(me, 1);
		int is_up;
		if (iphost_get_interface_updown(ifname, &is_up) == 0) {
			memset(&attr, 0x00, sizeof(attr));
			if (is_up) {
				attr.data = ipopt | 0x8;	// ip stack enable
			} else {
				attr.data = ipopt & ~(0x8);
			}
			meinfo_me_attr_set(me, 1, &attr, is_avc);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {		// mac addr
		unsigned char hwaddr[6];
		if (iphost_interface_is_exist(ifname) &&
		    iphost_get_hwaddr(ifname, hwaddr) == 0) {
			memset(&attr, 0x00, sizeof(attr));
			attr.ptr=hwaddr;	
			meinfo_me_attr_set(me, 2, &attr, is_avc);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 4) ||	// ip addr
	    util_attr_mask_get_bit(attr_mask, 5) ||	// netmask
	    util_attr_mask_get_bit(attr_mask, 6) ||	// gateway
	    util_attr_mask_get_bit(attr_mask, 7) ||	// primary dns
	    util_attr_mask_get_bit(attr_mask, 8) ||	// primary dns
	    util_attr_mask_get_bit(attr_mask, 9) ||	// current ip addr
	    util_attr_mask_get_bit(attr_mask, 10) ||	// current netmask
	    util_attr_mask_get_bit(attr_mask, 11) ||	// current gateway
	    util_attr_mask_get_bit(attr_mask, 12) ||	// current primary dns
	    util_attr_mask_get_bit(attr_mask, 13)) {	// current secondary dns
	    	
	    	int assignment, ip_addr, netmask_addr, gw_addr, dns1_addr, dns2_addr;
    		int curr_ip_addr, curr_netmask_addr, is_up;

		// get wanif info from pppoe/dhcp files or metafile.dat
	    	wanif_get_info(ifname, &assignment, &ip_addr, &netmask_addr, &gw_addr, &dns1_addr, &dns2_addr);
		{ // override the ip/netmask with values from iphost runtime
			char *ifname2 = (assignment == 2)? wanif_get_agent_pppname(ifname):ifname;
			if (iphost_get_ipv4_ip_netmask(ifname2, &curr_ip_addr, &curr_netmask_addr, &is_up) == 0) {
		    		ip_addr = curr_ip_addr;
		    		netmask_addr = curr_netmask_addr;
			}
		}

		memset(&attr, 0x00, sizeof(attr));

		attr.data=ntohl(ip_addr);
		if (util_attr_mask_get_bit(attr_mask, 4))
			meinfo_me_attr_set(me, 4, &attr, is_avc);
		if (util_attr_mask_get_bit(attr_mask, 9))
			meinfo_me_attr_set(me, 9, &attr, is_avc);

		attr.data=ntohl(netmask_addr);
		if (util_attr_mask_get_bit(attr_mask, 5)) 
			meinfo_me_attr_set(me, 5, &attr, is_avc);
		if (util_attr_mask_get_bit(attr_mask, 10)) 
			meinfo_me_attr_set(me, 10, &attr, is_avc);		

		attr.data=ntohl(gw_addr);
		if (util_attr_mask_get_bit(attr_mask, 6)) 
			meinfo_me_attr_set(me, 6, &attr, is_avc);
		if (util_attr_mask_get_bit(attr_mask, 11)) 
			meinfo_me_attr_set(me, 11, &attr, is_avc);

		attr.data=ntohl(dns1_addr);
		if (util_attr_mask_get_bit(attr_mask, 7))
			meinfo_me_attr_set(me, 7, &attr, is_avc);
		if (util_attr_mask_get_bit(attr_mask, 12))
			meinfo_me_attr_set(me, 12, &attr, is_avc);

		attr.data=ntohl(dns2_addr);
		if (util_attr_mask_get_bit(attr_mask, 8))
			meinfo_me_attr_set(me, 8, &attr, is_avc);
		if (util_attr_mask_get_bit(attr_mask, 13))
			meinfo_me_attr_set(me, 13, &attr, is_avc);
	}

	if (util_attr_mask_get_bit(attr_mask, 15)) {	//hostname
#ifdef OMCI_METAFILE_ENABLE
		struct metacfg_t kv_config;
		char buff[128];
		
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "system_modelname", "system_default_route_interface");

		memset(&attr, 0x00, sizeof(attr));
		attr.ptr=buff;
		// we return the system domain/host name instead of the one in dhcp lease info
		// since only few dhcp servers reutrn the two options in dhcp reply
		strncpy(buff, metacfg_adapter_config_get_value(&kv_config, "system_domainname", 1), 127);
		meinfo_me_attr_set(me, 14, &attr, is_avc);
		strncpy(buff, metacfg_adapter_config_get_value(&kv_config, "system_hostname", 1), 127);
		meinfo_me_attr_set(me, 15, &attr, is_avc);

		metacfg_adapter_config_release(&kv_config);
#endif
	}	
	return 0;
}


// 9.4.12 134 Ip_host_config_data
static int 
meinfo_hw_134_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char attr_mask_current[2] = {0xc0, 0xfa};

	// zerno attr mask means a special call from create_cb
	if (attr_mask[0]==0 && attr_mask[1] ==0) {		
		// init iphost static/current attrs with hw config, and avc is eliminated
		unsigned char attr_mask_static_current[2]={0xdf, 0xfa};	
		return update_me_with_hwconfig(me, attr_mask_static_current, 0);
	}

	// mask off iphost static (ip/gw/dns) attr, which should only be set by OLT after init
	attr_mask_current[0] &= attr_mask[0];
	attr_mask_current[1] &= attr_mask[1];
	// update iphost current mac/ip/gw/dns/hostname attr with avc enabled
	return update_me_with_hwconfig(me, attr_mask_current, 1);
}

struct meinfo_hardware_t meinfo_hw_134 = {
	.get_hw		= meinfo_hw_134_get,
};
