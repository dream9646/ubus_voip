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
 * File    : meinfo_hw_349.c
 *
 ******************************************************************/

#include "util.h"
#include "switch.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_poe.h"

// 9.5.6 PoE_Control

struct poe_result_t poe_result;

static int 
meinfo_hw_349_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *pptp_uni_me= meinfo_me_get_by_meid(11, me->meid);
	struct switch_port_info_t port_info;
	unsigned int phy_portid, ext_portid;
	
    	if (pptp_uni_me && switch_get_port_info_by_me(pptp_uni_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", pptp_uni_me->meid);	
		return -1;
	}
	
	// port logical->physical
	switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
	
	// get counters from hw
	if (meinfo_hw_poe_get_threshold_result(&poe_result)<0)
   		return -1;
   	
	// only when priority != 0, the result is meaningful
	// otherwise it maybe false alarm (read wrong value from i2c)
	if(poe_result.poe_report[phy_portid].priority) {
		// Operational_State
		if (util_attr_mask_get_bit(attr_mask, 3)) {
			struct attr_value_t attr;
		    	int enable = poe_result.poe_report[phy_portid].enable;
			attr.ptr=NULL;
			attr.data= !enable;
			meinfo_me_attr_set(me, 3, &attr, 1);
		}
		
		// Power_Detection_Status
		if (util_attr_mask_get_bit(attr_mask, 4)) {
			struct attr_value_t attr;
		    	int enable = poe_result.poe_report[phy_portid].enable;
		    	int status = poe_result.poe_report[phy_portid].status;
			attr.ptr=NULL;
			attr.data=(enable) ? status : 0;
			meinfo_me_attr_set(me, 4, &attr, 0);
		}
		
		// Power_Classification_Status
		if (util_attr_mask_get_bit(attr_mask, 5)) {
			struct attr_value_t attr;
		    	int classify = poe_result.poe_report[phy_portid].classify;
			attr.ptr=NULL;
			attr.data=classify;
			meinfo_me_attr_set(me, 5, &attr, 0);
		}
		
		// Power_Priority
		if (util_attr_mask_get_bit(attr_mask, 6)) {
			struct attr_value_t attr;
		    	int priority = poe_result.poe_report[phy_portid].priority;
			attr.ptr=NULL;
			attr.data=priority;
			meinfo_me_attr_set(me, 6, &attr, 0);
		}
		
		// Invalid_Signature_Counter
		if (util_attr_mask_get_bit(attr_mask, 7)) {
			struct attr_value_t attr;
			attr.ptr=NULL;
			attr.data=poe_result.poe_report[phy_portid].invd_cnt;
			meinfo_me_attr_set(me, 7, &attr, 0);
		}
		
		// Power_Denied_Counter
		if (util_attr_mask_get_bit(attr_mask, 8)) {
			struct attr_value_t attr;
			attr.ptr=NULL;
			attr.data=poe_result.poe_report[phy_portid].pwrd_cnt;
			meinfo_me_attr_set(me, 8, &attr, 0);
		}
		
		// Overload_Counter
		if (util_attr_mask_get_bit(attr_mask, 9)) {
			struct attr_value_t attr;
			attr.ptr=NULL;
			attr.data=poe_result.poe_report[phy_portid].ovl_cnt;
			meinfo_me_attr_set(me, 9, &attr, 0);
		}
		
		// Short_Counter
		if (util_attr_mask_get_bit(attr_mask, 10)) {
			struct attr_value_t attr;
			attr.ptr=NULL;
			attr.data=poe_result.poe_report[phy_portid].sc_cnt;
			meinfo_me_attr_set(me, 10, &attr, 0);
		}
		
		// MPS_Absent_Counter
		if (util_attr_mask_get_bit(attr_mask, 11)) {
			struct attr_value_t attr;
			attr.ptr=NULL;
			attr.data=poe_result.poe_report[phy_portid].mps_abs_cnt;
			meinfo_me_attr_set(me, 11, &attr, 0);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_349 = {
	.get_hw			= meinfo_hw_349_get,
};

