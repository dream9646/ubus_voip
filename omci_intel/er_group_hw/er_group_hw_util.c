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
 * File    : er_group_hw_util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>

// @sdk/include


#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "me_related.h"

#include "omciutil_vlan.h"
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "switch_hw_prx126.h"
#endif
#include "switch.h"
#include "er_group_hw.h"
#include "gpon_sw.h"

char *
er_group_hw_util_action_str(int action)
{
	switch (action) {
		case ER_ATTR_GROUP_HW_OP_NONE:		return "NONE";
		case ER_ATTR_GROUP_HW_OP_ADD:		return "ADD";
		case ER_ATTR_GROUP_HW_OP_UPDATE:	return "UPDATE";
		case ER_ATTR_GROUP_HW_OP_DEL:		return "DEL";
		case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:	return "ADD_BY_UPDATE";
		case ER_ATTR_GROUP_HW_OP_ERROR:		return "ERROR";
	}
	return "UNKNOWN";
}

// this function converts omci semantic (mac_depth, learning_ind) into switch hw mac limiting semantic (limit_count, limit_exceed_action)
// 3 cases:
// learning_ind == 0: 		=> mac learn disabled 			=> limit_count = 0, limit_exceed_act = fwd (ukn_sa_act= fwd)
// learning_ind == 1, depth==0	=> mac learn enabled			=> limit_count = max, limit_exceed_act = fwd (ukn_sa_act= trap)
// learning_ind == 1, depth==n	=> mac learn enabled, mac limiting	=> limit_count = n, limit_exceed_act = drop (ukn_sa_act= trap)
int
er_group_hw_util_mac_learning_depth_set(unsigned int port, unsigned int depth, unsigned int learning_ind)
{
	int limit_count, limit_action_is_drop;
	int ret =0;
	
	// do nothing if no uni neither not whole ont
	if (port != 255 && ((1<<port) & switch_get_uni_logical_portmask()) == 0) {
		dbprintf(LOG_WARNING, "Only set mac learning depth for uni or ont, so skip port [%d]\n", port);
		return 0;
	}
	
	if (learning_ind == 0)  { // mac learn disabed
		limit_count = 0;
		limit_action_is_drop = 0;
	} else {
		if (depth == 0)	{// mac learn enabled. mac limit disabled
			limit_count = CHIP_L2_LEARN_LIMIT_CNT_MAX;
			limit_action_is_drop = 0;
		} else {
			limit_count = depth;
			limit_action_is_drop = omci_env_g.maclearn_drop_enable;
		}
	}

	if (switch_hw_g.mac_learning_limit_set)
		ret = switch_hw_g.mac_learning_limit_set(port, limit_count, limit_action_is_drop);

	if (ret == 0) {
		if (port == 255)
			dbprintf(LOG_WARNING, "ont depth=%d, learn=%d => limit_count=%d, limit_exceed_act:%s\n", 
				depth, learning_ind, limit_count, limit_action_is_drop?"drop":"fwd");
		else
			dbprintf(LOG_WARNING, "port%d depth=%d, learn=%d => limit_count=%d, limit_exceed_act:%s\n", 
				port, depth, learning_ind, limit_count, limit_action_is_drop?"drop":"fwd");
	} else {
		if (port == 255)
			dbprintf(LOG_ERR, "set error! ont depth=%d, learn=%d => limit_count=%d, limit_exceed_act:%s, ret=%d\n", 
				depth, learning_ind, limit_count, limit_action_is_drop?"drop":"fwd", ret);
		else
			dbprintf(LOG_ERR, "set error! port%d depth=%d, learn=%d => limit_count=%d, limit_exceed_act:%s, ret=%d\n", 
				port, depth, learning_ind, limit_count, limit_action_is_drop?"drop":"fwd", ret);
	}
	return ret;
}

// this is a wrapper function of er_group_hw_util_mac_learning_depth_set, 
// this function considers both the per port and per ont mac limit depth all at once
int
er_group_hw_util_mac_learning_depth_set2(unsigned int port, unsigned int port_depth, unsigned global_depth, unsigned int learning_ind)
{
#if 0
	int err = 0;
	// for platform which des support per port and per ont mac limiting
	// set poer port mac depth
	if (er_group_hw_util_mac_learning_depth_set(port, port_depth, learning_ind) != 0)
		err |= 1;
	// set per ont mac depth (port==255)
	if (er_group_hw_util_mac_learning_depth_set(255, global_depth, learning_ind) != 0)
		err |= 2;
	if (err)
		return (-1*err);
#else
	// 2511b: per port maclimit only
	// 2510_swnat: per port and per ont maclimit
	// 2510_hwnat: per port maclimit only

	// since 2511b & 2510_hwnat are the most used cases, which doesn't have per ont mac limiting
	// so we dont call per ont mac limit function, we just use the per ont mac limit depth for per port if per port maclimit depth == 0
	if (port_depth == 0) 
		port_depth = global_depth;	
	if (er_group_hw_util_mac_learning_depth_set(port, port_depth, learning_ind) != 0)
		return -1;
#endif
	return 0;
}


// below code is based on rtk only, not rtk_rg
// and its function is replaced the the similar function in classf function "er_group_hw_util_classf_update_dscp2pbit"
# if 0
// FV2510 support only one dscp-to-pbit mapping table
int
er_group_hw_util_config_dscp2pbit_table(unsigned int portmask, char *omci_dscp2pbit_bitmap)
{
	int uni_port_total = 5;
	int pon_portid = 5;
	int i;

	if (switch_hw_g.uni_port_total)
		uni_port_total = switch_hw_g.uni_port_total();
	if (switch_hw_g.pon_portid_get)
		pon_portid = switch_hw_g.pon_portid_get();
	
	// dscp table configuration:
	// map 64 dscp to pbit as specified in dscp2pbit_bitmap
	if (switch_hw_g.qos_dscp2pri_set_by_pri) {
		for(i=0; i < 64; i++) {
			if (omci_dscp2pbit_bitmap) {
				unsigned char dscp2pbit=util_bitmap_get_value(omci_dscp2pbit_bitmap, 8*24, i*3, 3);
				switch_hw_g.qos_dscp2pri_set_by_pri(i, dscp2pbit);
			} else {
				switch_hw_g.qos_dscp2pri_set_by_pri(i, 0);	// if dscp2pbit bitmap null, map all dscp to pbit0
			}
		}
	}

	// 1. we config internal pbit source from dscp on UNI only,
	//    for pon port, we will config pkt pbit as internal pbit source for pkt pbit to uni egress q mapping
	// 2. we map internal to 8 egressq evenly on pon port only
	//    for uni port, we will map internal pbit to different egress Q with sp/wrr depending on omci uni port config

	// per uni port configuration:
	// config dscp with the first precedence(weight=15) in the sources of internal pbit
	// ps: while we can only config the pri selector of related uni ports, it is not necessary, 
	//     as whether the internal priority derived from dscp would be used or not is fully determined by classf
	if (switch_hw_g.qos_prisel_weight_set) {
		for (i =0; i < uni_port_total; i++) {
			if (portmask & (1<<i))
				switch_hw_g.qos_prisel_weight_set(i, QOS_PRISEL_DSCP, 15);
		}
	}

	// FIXME, KEVIN
	// pon port configuration:
	// As internal pbit also can be used to determine which egress Q is used, 
	// we have to map all internal pbits to different egress q in pon port, and each q is wrr with same weight
	// thus all traffic has equal chance in pon port egress and avoid head-of-line congestion.
	// finally, further differencial service then be determined by gpon tcont/pq configuration

	// config pon port 8 egress Q as wrr with weight=64
	if (switch_hw_g.qos_port_scheduled_weight_set_by_qid) {
		for (i=0; i<8; i++)
			switch_hw_g.qos_port_scheduled_weight_set_by_qid(pon_portid, i, 64);
	}
	// config p2q table(internal pbit to qid) in group 0 
	if (switch_hw_g.qos_p2q_set_by_pri) {
		for (i=0; i<8; i++)
			switch_hw_g.qos_p2q_set_by_pri(i, i);	// grp, pri, qid
	}
	// config pon port to p2q table in group 0
	if (switch_hw_g.qos_p2q_group_set_by_port)
		switch_hw_g.qos_p2q_group_set_by_port(pon_portid, 0);
	return 0;
}

#endif

