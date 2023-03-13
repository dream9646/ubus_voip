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
 * File    : meinfo_hw_48.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "bitmap.h"
#include "uid_stp.h"
#include "stp_in.h"
//#include "stp_to.h"
#include "uid.h"
#include "bridge.h"
#include "util.h"
#include "rstp.h"

/*
class id: 48 
name: MAC_bridge_port_designation_data
section: 9.3.5
*/

static int
meinfo_hw_48_get_stp_port_state(struct me_t *me48, void *designated_bridge_root_cost_port )
{
	struct me_t *me_47;
	struct switch_port_info_t port_info;
	BITMAP_T	ports_bitmap;
	int rc;

	int i = 0;

	UID_STP_PORT_STATE_T uid_port;

	if( (me_47=meinfo_me_get_by_meid(47, me48->meid)) == NULL) {
		return -1;
	}

	if (switch_get_port_info_by_me(me_47, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me_47->meid);	
		return -1;
	}

	for ( i = 0 ; i < MAX_STP_PORTS ; i++ ) {
		if (port_info.logical_port_id == stp_port[i].logical_port_id)
			break;
	}

	if (i == MAX_STP_PORTS )
		return -1;
	
	BitmapSetBit(&ports_bitmap, i);
	uid_port.port_no = i+1 ;
	rc = STP_IN_port_get_state (0, &uid_port);
	if (rc) {
		dprintf(LOG_ERR, "can't get rstp port state: %s\n", STP_IN_get_error_explanation (rc));
		return -1;
	}

	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8,   0, 2*8, uid_port.designated_bridge.prio);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 2*8, 1*8, uid_port.designated_bridge.addr[0]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 3*8, 1*8, uid_port.designated_bridge.addr[1]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 4*8, 1*8, uid_port.designated_bridge.addr[2]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 5*8, 1*8, uid_port.designated_bridge.addr[3]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 6*8, 1*8, uid_port.designated_bridge.addr[4]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 7*8, 1*8, uid_port.designated_bridge.addr[5]);

	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 8*8, 2*8, uid_port.designated_root.prio);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 10*8, 1*8, uid_port.designated_root.addr[0]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 11*8, 1*8, uid_port.designated_root.addr[1]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 12*8, 1*8, uid_port.designated_root.addr[2]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 13*8, 1*8, uid_port.designated_root.addr[3]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 14*8, 1*8, uid_port.designated_root.addr[4]);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 15*8, 1*8, uid_port.designated_root.addr[5]);

	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 16*8, 4*8, uid_port.designated_port);
	util_bitmap_set_value( designated_bridge_root_cost_port, 24*8, 20*8, 4*8, uid_port.designated_cost);

	return 0;
}

static int
meinfo_hw_48_get_port_stp_state(struct me_t *me48, unsigned int *port_stp_state)
{
	struct switch_port_info_t port_info;
	struct switch_port_mac_config_t port_mac_status;
	struct me_t *me_47;
	struct attr_value_t attr_value;

	if( (me_47=meinfo_me_get_by_meid(47, me48->meid)) == NULL) {
		return -1;
	}

	meinfo_me_attr_get(me_47, 3, &attr_value);	//3:TP_type  != PPTP UNI
	if( attr_value.data != 1 ) {
		return -1;
	}

	//dbprintf(LOG_ERR,"Update PPTP UNI\n");
	if (switch_get_port_info_by_me(me_47, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me_47->meid);	
		return -1;
	}

	//dbprintf(LOG_ERR,"core port_id=%d\n", port_info.logical_port_id);	
	if (switch_hw_g.stp_state_port_get==NULL || 
		switch_hw_g.stp_state_port_get(port_info.logical_port_id, port_stp_state)) {
		dbprintf(LOG_ERR,"switch_hw_g.port_config_get null or error\n");	
		return -1;
	}
	/* 9.3.5 MAC_bridge_port_designation_data
	0 Disabled.
	1 Listening.
	2 Learning.
	3 Forwarding.
	4 Blocking.
	5 Linkdown.
	6 Stp_off.
	*/
	//stp_off overwrite normal stp state, linkdown overwrite all state
	//dbprintf(LOG_ERR,"port_stp_state=%d\n", *port_stp_state);	
	meinfo_me_attr_get(me_47, 7, &attr_value);	//7: Port_spanning_tree_ind
	if( attr_value.data == 0 )	//disable STP
		*port_stp_state = 6;

	if (switch_hw_g.port_status_get==NULL || 
	    switch_hw_g.port_status_get(port_info.logical_port_id,&port_mac_status)){
		dbprintf(LOG_ERR,"switch_hw_g.port_status_get null or error, %d\n", port_info.logical_port_id);
		return -1;
	}

	if (port_mac_status.link== 0) {	//if linkdown
		*port_stp_state = 5;
	}

	//dbprintf(LOG_ERR,"end port_stp_state=%d\n", *port_stp_state);	
	return 0;
}
					
/*
class id: 48 
name: MAC_bridge_port_designation_data
section: 9.3.5
*/

static int 
meinfo_hw_48_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	unsigned int port_stp_state=6;
//	void *designated_bridge_root_cost_port = NULL;

	if (util_attr_mask_get_bit(attr_mask, 1)) {	// attr1: Designated_bridge_root_cost_port

		attr.data=0;
		attr.ptr= malloc_safe(24);
		meinfo_hw_48_get_stp_port_state(me, attr.ptr);
		meinfo_me_attr_set(me, 1, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}


	if (util_attr_mask_get_bit(attr_mask, 2)) {	// attr2: port state
		if ( meinfo_hw_48_get_port_stp_state(me, &port_stp_state) == 0 ) {
			attr.ptr=NULL;
			attr.data = port_stp_state;
			meinfo_me_attr_set(me, 2, &attr, 0);	// check_avc==0 means none update avc_change_mask
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_48 = {
	.get_hw		= meinfo_hw_48_get,
};

