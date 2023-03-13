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
 * Module  : proprietary_zte
 * File    : er_group_hw_253.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"

static int
uni_broadcast_rate_limit(int enable, int port_id, unsigned int pkt)
{
	if(switch_hw_g.rate_storm_set) {
		// pps->bytes/s, assume bcast frame=1518B
		// default value: 1Mbps = 131072 bytes/s
		switch_hw_g.rate_storm_set(port_id, STORM_BROADCAST, (enable) ? pkt*1518 : 131072, 0);
	}

	return 0;
}

//253@meid,253@1
int
er_group_hw_broadcast_rate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL, *iethuni_me = NULL;
	unsigned short meid, port_id;
	unsigned int rate = 0;
	
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
		dbprintf(LOG_WARNING, "could not get bridge port me, classid=11, meid=%u\n", meid);
		return -1;
	}
	iethuni_me = hwresource_public2private(meptr);
	port_id = meinfo_util_me_attr_data(iethuni_me, 4);
	rate = attr_inst->er_attr_instance[1].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		uni_broadcast_rate_limit(1, port_id, rate);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		uni_broadcast_rate_limit(0, port_id, rate);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_broadcast_rate(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_broadcast_rate(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}

static int
dhcp_config(int enable, int port_id, unsigned int mode)
{
	FILE *fp = NULL;
	
	util_run_by_system("cat /proc/ifsw.srcmask | grep dhcp_untag_rt_allow_src_mask > /tmp/dhcp_allow_src_mask");
	fp = fopen("/tmp/dhcp_allow_src_mask", "r");
	
	if(fp) {
		unsigned char buf1[32], buf2[32], buf3[32], buf4[32];
		unsigned int rt_mask=0, br_mask=0, uni_mask=0;
		 
		fscanf(fp, "%s %s %s %s", buf1, buf2, buf3, buf4);
		fclose(fp);
		
		rt_mask = strtol(buf2, NULL, 16);
		br_mask = strtol(buf4, NULL, 16);
		uni_mask = (1 << port_id);
		
		if(enable) {
			switch(mode) {
				case 0: // no_ctrl
				default:
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
				case 1: // rt_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
				case 2: // br_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
				case 3: // disable
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
			}
		} else {
			switch(mode) {
				case 0: // no_ctrl
				default:
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
				case 1: // rt_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
				case 2: // br_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
				case 3: // disable
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
			}
		}
	}
	
	util_run_by_system("rm -f /tmp/dhcp_allow_src_mask");
	
	return 0;
}

//253@meid,253@2
int
er_group_hw_port_dhcp(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL, *iethuni_me = NULL;
	unsigned short meid, port_id;
	unsigned int mode = 0;
	
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
		dbprintf(LOG_WARNING, "could not get bridge port me, classid=11, meid=%u\n", meid);
		return -1;
	}
	iethuni_me = hwresource_public2private(meptr);
	port_id = meinfo_util_me_attr_data(iethuni_me, 4);
	mode = attr_inst->er_attr_instance[1].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		dhcp_config(1, port_id, mode);
		batchtab_omci_update("isolation_set");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		dhcp_config(0, port_id, mode);
		batchtab_omci_update("isolation_set");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_port_dhcp(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_port_dhcp(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}
