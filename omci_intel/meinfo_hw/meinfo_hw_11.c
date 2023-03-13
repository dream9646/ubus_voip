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
 * File    : meinfo_hw_11.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "util.h"
#include "meinfo.h"
#include "meinfo_hw.h"
#include "meinfo_hw_poe.h"
#include "switch.h"
#include "er_group_hw_util.h"
#include "omciutil_misc.h"
#include "iphost.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_event.h"

extern struct extoam_link_status_t cpe_link_status_g;
static unsigned char extoam_event[SWITCH_LOGICAL_PORT_TOTAL];
static unsigned char extoam_event_previous[SWITCH_LOGICAL_PORT_TOTAL];

static int
meinfo_hw_11_get_port_status(struct me_t *me, struct switch_port_mac_config_t *port_mac_status)
{
	struct switch_port_info_t port_info;

	if (switch_get_port_info_by_me(me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me->meid);	
		return -1;
	}
	if (switch_hw_g.port_status_get==NULL) {
		dbprintf(LOG_ERR,"switch_hw_g.port_config_get null?\n");	
		return -1;
	}
	switch_hw_g.port_status_get(port_info.logical_port_id, port_mac_status);		

	if (((1<<port_info.logical_port_id) & switch_get_all_logical_portmask_without_cpuext()) == 0) {
		char *ifname = meinfo_util_get_config_devname(me);
		int is_up;
		if (iphost_get_interface_updown(ifname, &is_up) == 0) {
			port_mac_status->link = is_up;
		} else {
			port_mac_status->link = 0;
		}
	}
	
	return 0;
}

static int
meinfo_hw_11_get_loop_status(struct me_t *me)
{
	int ret;
	unsigned int portmask = 0;
	struct switch_port_info_t port_info;

	if (switch_get_port_info_by_me(me, &port_info) < 0)
	{
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me->meid);	
		return -1;
	}

	if (omci_env_g.rldp_enable && switch_hw_g.rldp_loopedportmask_get != NULL)
	{
		if ((ret=switch_loop_detect_mask_get(&portmask)) < 0) {
			dbprintf(LOG_ERR,"switch_loop_detect_mask_get error\n");
			return -1;
		}

		if (portmask)
		{
			if ((portmask & (1 << (port_info.logical_port_id))) != 0)
			{
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}

	return 0;
}
					
//9.5.1 Physical path termination point Ethernet UNI
static int 
meinfo_hw_11_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	// do nothing if this pptp is an equivlance of veip
	if (omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;
	
	if (util_attr_mask_get_bit(attr_mask, 6) ||	// attr6: operation state
	    util_attr_mask_get_bit(attr_mask, 7)) {	// attr7: configuration indicator
	    	struct switch_port_mac_config_t port_mac_status;
	    
	    	if (meinfo_hw_11_get_port_status(me, &port_mac_status)<0)	
	    		return -1;
			    		
		// attr 6
		if (port_mac_status.link)	// link up
		        attr.data=0;	// opstate=0 means normal
		else
		        attr.data=1;	// opstate=1 means abnormal			
		attr.ptr=NULL;
		meinfo_me_attr_set(me, 6, &attr, 1);	// check_avc==1 means update avc_change_mask
		
		// attr 7			
		if (port_mac_status.link) {
			// return speed/duplex only if link is up
			if (port_mac_status.speed==2) {		// 1000m
				attr.data=3;
			} else if (port_mac_status.speed==1) {  // 100m
				attr.data=2;
			} else {   				// 10m		
				attr.data=1;
			}
			if (port_mac_status.duplex==0)	// half duplex
				attr.data|=0x10;
		} else {
			// return unknow if link down
			attr.data=0x0;		
		}
		meinfo_me_attr_set(me, 7, &attr, 1);	// check_avc==1 means update avc_change_mask
	}
	if (util_attr_mask_get_bit(attr_mask, 15)) {	// attr15: power control
		struct switch_port_info_t port_info;
		unsigned int phy_portid, ext_portid, enable = 0;
//		extern struct poe_result_t poe_result;
		
		if (switch_get_port_info_by_me(me, &port_info)<0) {
			dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me->meid);
			return -1;
		}
		// port logical->physical
		switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
		meinfo_hw_poe_get_admin(&enable, phy_portid);

		attr.ptr=NULL;
		attr.data=enable;
		meinfo_me_attr_set(me, 15, &attr, 0);
	}
	return 0;
}

static int 
meinfo_hw_11_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct switch_port_mac_config_t port_mac_status;

	// do nothing if this pptp is an equivlance of veip
	if (omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (meinfo_hw_11_get_port_status(me, &port_mac_status)<0)
    		return -1;
    	
	if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
		if (port_mac_status.link) {	// link up
			struct switch_port_info_t port_info;
			if (switch_get_port_info_by_me(me, &port_info)<0)
				return -1;
			switch(port_info.logical_port_id) {
				case 0:
					extoam_event[0] = 1;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[0] != extoam_event_previous[0])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_0_LINK_UP );
					}
					extoam_event_previous[0] = extoam_event[0];
					break;
				case 1:
					extoam_event[1] = 1;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[1] != extoam_event_previous[1])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_1_LINK_UP );
					}
					extoam_event_previous[1] = extoam_event[1];
					break;
				case 2:
					extoam_event[2] = 1;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[2] != extoam_event_previous[2])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_2_LINK_UP );
					}
					extoam_event_previous[2] = extoam_event[2];
					break;
				case 3:
					extoam_event[3] = 1;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[3] != extoam_event_previous[3])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_3_LINK_UP );
					}
					extoam_event_previous[3] = extoam_event[3];
					break;
				case 4:
					extoam_event[4] = 1;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[4] != extoam_event_previous[4])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_4_LINK_UP );
					}
					extoam_event_previous[4] = extoam_event[4];
					break;
			}
			util_alarm_mask_clear_bit(alarm_mask, 0);
		} else {
			struct switch_port_info_t port_info;
			if (switch_get_port_info_by_me(me, &port_info)<0)
				return -1;
			switch(port_info.logical_port_id) {
				case 0:
					extoam_event[0] = 0;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[0] != extoam_event_previous[0])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_0_LINK_DOWN );
					}
					if(extoam_event[0] != extoam_event_previous[0] && omci_env_g.uni_linkdown_lock) {
						if (switch_hw_g.port_enable_set)
							switch_hw_g.port_enable_set(port_info.logical_port_id, 0); // port down
					}
					extoam_event_previous[0] = extoam_event[0];
					break;
				case 1:
					extoam_event[1] = 0;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[1] != extoam_event_previous[1])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_1_LINK_DOWN );
					}
					if(extoam_event[1] != extoam_event_previous[1] && omci_env_g.uni_linkdown_lock) {
						if (switch_hw_g.port_enable_set)
							switch_hw_g.port_enable_set(port_info.logical_port_id, 0); // port down
					}
					extoam_event_previous[1] = extoam_event[1];
					break;
				case 2:
					extoam_event[2] = 0;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[2] != extoam_event_previous[2])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_2_LINK_DOWN );
					}
					if(extoam_event[2] != extoam_event_previous[2] && omci_env_g.uni_linkdown_lock) {
						if (switch_hw_g.port_enable_set)
							switch_hw_g.port_enable_set(port_info.logical_port_id, 0); // port down
					}
					extoam_event_previous[2] = extoam_event[2];
					break;
				case 3:
					extoam_event[3] = 0;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[3] != extoam_event_previous[3])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_3_LINK_DOWN );
					}
					if(extoam_event[3] != extoam_event_previous[3] && omci_env_g.uni_linkdown_lock) {
						if (switch_hw_g.port_enable_set)
							switch_hw_g.port_enable_set(port_info.logical_port_id, 0); // port down
					}
					extoam_event_previous[3] = extoam_event[3];
					break;
				case 4:
					extoam_event[4] = 0;
					if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
						if(extoam_event[4] != extoam_event_previous[4])
							extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_PORT_4_LINK_DOWN );
					}
					if(extoam_event[4] != extoam_event_previous[4] && omci_env_g.uni_linkdown_lock) {
						if (switch_hw_g.port_enable_set)
							switch_hw_g.port_enable_set(port_info.logical_port_id, 0); // port down
					}
					extoam_event_previous[4] = extoam_event[4];
					break;
			}
			util_alarm_mask_set_bit(alarm_mask, 0);
		}
	}

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0 ||
	    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE)==0) {
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 208)) {
			int loop_status=meinfo_hw_11_get_loop_status(me);

			if (loop_status<0)
				return -1;

			if (loop_status==1 && port_mac_status.link)	//loop detected, check link up additionally
				util_alarm_mask_set_bit(alarm_mask, 208);
			else
				util_alarm_mask_clear_bit(alarm_mask, 208);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_11 = {
	.get_hw		= meinfo_hw_11_get,
	.alarm_hw	= meinfo_hw_11_alarm,
};

