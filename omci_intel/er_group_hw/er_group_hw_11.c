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
 * Module  : er_group_hw
 * File    : er_group_hw_11.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "meinfo_hw_poe.h"
#include "omciutil_misc.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "switch_hw_prx126.h"
#endif

// 11 Physical_path_termination_point_Ethernet_UNI

//11@inst,11@3
int
er_group_hw_ethernet_autoneg_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	//unsigned short inst;
	unsigned char codepoint, codepoint2;
	//struct switch_port_mac_config_t port_mac_config;
	int ret = 0;
	struct er_attr_group_instance_t *current_attr_inst;
	struct switch_port_info_t port_info;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;	
	//inst=attr_inst->er_attr_instance[0].attr_value.data;
	codepoint=attr_inst->er_attr_instance[1].attr_value.data;
	
	if (switch_get_port_info_by_me(me, &port_info) < 0)
	{
		if( me != NULL )
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", me->classid, me->meid);
		return -1;
	}

	/*if (action == ER_ATTR_GROUP_HW_OP_DEL) {	// rate auto, duplex auto as pptp default
		port_mac_config.speed=1000;
		port_mac_config.duplex=1;
		port_mac_config.nway=1;		
	}*/
		
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (switch_hw_g.port_autoneg_set) {
			ret = switch_hw_g.port_autoneg_set(port_info.logical_port_id, codepoint);
			if (ret <0)
				dbprintf(LOG_ERR, "error, ret=%d\n", ret);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}		
		codepoint2=current_attr_inst->er_attr_instance[1].attr_value.data;
		if (codepoint!=codepoint2) {	// to avoid unnecessary link down, we do autoneg only if codepoint is changed
			//add
			if (er_group_hw_ethernet_autoneg_mode(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
			{
				er_attr_group_util_release_attrinst(current_attr_inst);
				dbprintf(LOG_ERR, "fail to update for adding\n");
				return -1;
			}
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

//11@inst,11@4
int
er_group_hw_phy_loopback(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char loopback_configuration;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = 0;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	loopback_configuration=attr_inst->er_attr_instance[1].attr_value.data;
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		if( pptp_uni_me != NULL )
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.port_phylb_set &&
			(ret = switch_hw_g.port_phylb_set(port_info.logical_port_id, (loopback_configuration==3)?2:0) < 0))
		{
			dbprintf(LOG_ERR, "error, ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_phy_loopback(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@5
int
er_group_hw_ethernet_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char enable;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int phy_portid, ext_portid;
	int ret;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	enable= (!attr_inst->er_attr_instance[1].attr_value.data);
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		if( pptp_uni_me != NULL )
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}
	// port logical->physical
	switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
	

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.port_enable_set &&
			(ret = switch_hw_g.port_enable_set(port_info.logical_port_id, enable) < 0))
		{
			dbprintf(LOG_ERR, "error, ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_ethernet_enable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@8
int
er_group_hw_ethernet_mtu(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short mtu;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = 0;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	mtu=attr_inst->er_attr_instance[1].attr_value.data;
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.port_eth_mtu_set &&
			(ret = switch_hw_g.port_eth_mtu_set(port_info.logical_port_id, mtu+8) < 0)) // Add 8 bytes to exclude the length of VLAN single/double tag
		{
			dbprintf(LOG_ERR, "error, ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_ethernet_mtu(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@10
int
er_group_hw_ethernet_setpause(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short quanta;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = 0;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	quanta=attr_inst->er_attr_instance[1].attr_value.data;
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.pause_quanta_set &&
			(ret = switch_hw_g.pause_quanta_set(quanta) < 0))
		{
			dbprintf(LOG_ERR, "error, ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_ethernet_setpause(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@14
int
er_group_hw_l2s_bridging_pppoe_filt(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char allow_pppoe_only;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = 0;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	allow_pppoe_only=attr_inst->er_attr_instance[1].attr_value.data;
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.eth_pppoe_filter_enable_set == NULL || 
			(ret = switch_hw_g.eth_pppoe_filter_enable_set(port_info.logical_port_id, allow_pppoe_only) < 0))
		{
			dbprintf(LOG_ERR, "error, ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_l2s_bridging_pppoe_filt(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@1
int
er_group_hw_loopdetect_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char expected_type;
	struct me_t *pptp_uni_me;
	struct switch_port_info_t port_info;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int loop_detection_mask;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	expected_type=attr_inst->er_attr_instance[1].attr_value.data;
	if (switch_get_port_info_by_me(pptp_uni_me, &port_info) < 0) {
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", pptp_uni_me->classid, pptp_uni_me->meid);
		return -1;
	}

	loop_detection_mask = switch_get_uni_logical_portmask();

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE)==0)
		{
			// in zte spec. ZTE_Proprietary_OMCI_20111011.doc; section 4.5, p89, Expected type
			// extend standard class Physical path termination point Ethernet UNI
			
			switch(expected_type) {
			case 241:
				//disable loop detect
				if(switch_loop_detect_set(0, loop_detection_mask) < 0) {
					dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
					return -1;
				}
			case 242:
				//enable loop detect
				if(switch_loop_detect_set(1, loop_detection_mask) < 0) {
					dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
					return -1;
				}

				//runtime change rldp action to only loop detect
				omci_env_g.rldp_enable=2;
				break;
			case 243:
				//enable loop detect, if detect loop happen, close looping port, 
				//accorting to ZTE_Proprietary_OMCI_20111011.doc, need referance ONT3-G(class 247)
				if(switch_loop_detect_set(1, loop_detection_mask) < 0) {
					dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
					return -1;
				}

				//runtime change rldp action to port admin down
				omci_env_g.rldp_enable=3;
				break;
			default:
				//disable loop detect
				if(switch_loop_detect_set(0, loop_detection_mask) < 0) {
					dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
					return -1;
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_loopdetect_enable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
#if 0
static int
dhcp_config(int enable, int port_id, unsigned int mode)
{
	FILE *fp = NULL;
	
	util_run_by_system("cat /proc/ifsw.srcmask | grep dhcp_untag_rt_allow_src_mask > /tmp/dhcp_allow_src_mask");
	fp = fopen("/tmp/dhcp_allow_src_mask", "r");
	
	if(fp) {
		unsigned char cmd[64];
		unsigned char buf1[32], buf2[32], buf3[32], buf4[32];
		unsigned int rt_mask=0, br_mask=0, uni_mask=0;
		 
		fscanf(fp, "%s %s %s %s", buf1, buf2, buf3, buf4);
		fclose(fp);
		
		rt_mask = strtol(buf2, NULL, 16);
		br_mask = strtol(buf4, NULL, 16);
		uni_mask = (1 << port_id);
		
		if(enable) {
			switch(mode) {
				case 0: // br_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
				case 1: // rt_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
				case 2: // no_ctrl
				default:
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
			}
		} else {
			switch(mode) {
				case 0: // br_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask | uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
				case 1: // rt_path
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask | uni_mask);
					break;
				case 2: // no_ctrl
				default:
					util_write_file("/proc/ifsw", "dhcp_untag_rt_allow_src_mask 0x%x\n", rt_mask & ~uni_mask);
					util_write_file("/proc/ifsw", "dhcp_untag_br_allow_src_mask 0x%x\n", br_mask & ~uni_mask);
					break;
			}
		}
	}
	
	util_run_by_system("rm -f /tmp/dhcp_allow_src_mask");
	
	return 0;
}
#endif
//11@inst,11@11
int
er_group_hw_bridge_router_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *pptp_uni_me, *iethuni_me;
	unsigned short MAYBE_UNUSED port_id;
	unsigned int MAYBE_UNUSED mode = 2;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;

	if (attr_inst == NULL)
		return -1;
	
	pptp_uni_me=er_attr_group_util_attrinst2me(attr_inst, 0);
	if (pptp_uni_me==NULL)
		return -2;

	iethuni_me = hwresource_public2private(pptp_uni_me);
	port_id = meinfo_util_me_attr_data(iethuni_me, 4);
	mode = attr_inst->er_attr_instance[1].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		/*if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			dhcp_config(1, port_id, mode);
			batchtab_omci_update("isolation_set");
		}*/
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		/*if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			dhcp_config(0, port_id, mode);
			batchtab_omci_update("isolation_set");
		}*/
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_bridge_router_mode(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_bridge_router_mode(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@inst,11@5,11@15
int
er_group_hw_poe_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char admin, enable;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int phy_portid, ext_portid;
	struct switch_port_info_t port_info;
	struct me_t *pptp_uni_me= meinfo_me_get_by_meid(11, me->meid);

    	if (pptp_uni_me && switch_get_port_info_by_me(pptp_uni_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", pptp_uni_me->meid);	
		return -1;
	}

	// POE is meaningful on ethernet pptpuni only, this check will skip wifi pptpuni
	if ((switch_get_uni_logical_portmask() & (1<<port_info.logical_port_id)) == 0) {
		return 0;
	}
	
	// port logical->physical
	switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
	
	admin = attr_inst->er_attr_instance[1].attr_value.data;
	enable = (admin==1) ? 0 : attr_inst->er_attr_instance[2].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			//get PsE_Control to check management capability
			extern int poe_mgt_cap;
			if (poe_mgt_cap == -1)
				meinfo_hw_poe_get_management_capability(&poe_mgt_cap);
			if (poe_mgt_cap == 0)
				meinfo_hw_poe_set_admin_state(phy_portid, enable);
			if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
				// write poe state to default config because calix don't want to power off poe client during mib reset
				meinfo_config_set_attr_by_str(me->classid, me->instance_num, 15, (enable) ? "1" : "0");
			}
		} else
			meinfo_hw_poe_set_admin_state(phy_portid, enable);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_poe_control(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
