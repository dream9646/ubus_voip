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
 * File    : er_group_hw_45_280_298.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

// @sdk/include

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

// 46 MAC_bridge_configuration_data or 130 802.1p_mapper_service_profile
// in T-REC-G.988-201006-P change to 
// 45 MAC bridge service profile or 130 802.1p_mapper_service_profile
// 280 Traffic_descriptor
// 298 Dot1_rate_limiter
// port_storm_control
//		0	1	2	3    4      5      6
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@3
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@4
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@5

int
er_group_hw_port_storm_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2], int pkt_type)
{
	unsigned char tp_type;
	struct me_t *meptr280;
	unsigned short meid280;
	unsigned int MAYBE_UNUSED cir=0;
	unsigned int pir=0, rate=0;
	unsigned int bucket_size=0;
	int port;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	//meid == 298's attr1: Parent Me pointer's value
	if ( attr_inst->er_attr_instance[0].attr_value.data != attr_inst->er_attr_instance[4].attr_value.data ) {
	       	dbprintf(LOG_ERR,"Invalid command\n");
		return -1;
	}
		
	//280's meid should == one of 298's attr 3, 4, or 5
	meid280=attr_inst->er_attr_instance[1].attr_value.data;

	if( meid280 != attr_inst->er_attr_instance[6].attr_value.data ) {
	       	dbprintf(LOG_WARNING, "not expect me280 for this pkt_type\n");
		return -1;
	}

	tp_type=attr_inst->er_attr_instance[5].attr_value.data;

	//todo: maybe different
	if( tp_type == 1 ) {
	       	dbprintf(LOG_WARNING,"Use tp_type 1: MAC bridge service profile\n");
	} else if (tp_type == 2) {
	       	dbprintf(LOG_WARNING,"Use tp_type 2: 802.1p mapper service profile\n");
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:

		if( attr_inst->er_attr_instance[6].attr_value.data == meid280 ) {

			//9.3.18 Dot1 Rate Limiter 3: Upstream_unicast_flood_rate_pointer
			if ((meptr280=er_attr_group_util_attrinst2me(attr_inst, 6))==NULL)
			{
				dbprintf(LOG_ERR, "get meptr error:classid=%u, meid=%u\n", 
					attr_inst->er_attr_group_definition_ptr->er_attr[6].classid, attr_inst->er_attr_instance[6].meid);
				return -1;
			}
			cir=attr_inst->er_attr_instance[2].attr_value.data;//CIR
			pir=attr_inst->er_attr_instance[3].attr_value.data;//PIR

			//for rate limit use pir only
			rate=pir;
			
			bucket_size=rate;			
			if(bucket_size > 0xf000)
				bucket_size = 0xf000;
			else if(bucket_size < 1000)
				bucket_size = 1000;

			dbprintf(LOG_WARNING, "enable storm rate limit %d for pkt_type=%d\n", rate, pkt_type);
			switch(pkt_type)
			{
				case STORM_UNKNOWN_UNICAST:
					switch_hw_g.rate_meter_bucket_set(UNKNOWN_UNICAST_METER, bucket_size);
					util_write_file("/proc/re8686_ext", "storm_us_ucast %d\n", (rate)?rate:0);
				break;
				case STORM_BROADCAST:
					switch_hw_g.rate_meter_bucket_set(BROADCAST_METER, bucket_size);
					util_write_file("/proc/re8686_ext", "storm_us_bcast %d\n", (rate)?rate:0);
				break;
				case STORM_UNKNOWN_MULTICAST:
					switch_hw_g.rate_meter_bucket_set(UNKNOWN_MULTICAST_METER, bucket_size);
					util_write_file("/proc/re8686_ext", "storm_us_mcast %d\n", (rate)?rate:0);
				break;
				default:
					dbprintf(LOG_ERR, "WARNING! unexpected storm type\n"); 
					return -1;
			}

			//In most cases we need to protect CPU, so we will not deliberately disable storm limit.
			if(rate) {
				switch(pkt_type)
				{
					case STORM_UNKNOWN_UNICAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_UNICAST, 1);
					break;
					case STORM_BROADCAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_BROADCAST, 1);
					break;
					case STORM_UNKNOWN_MULTICAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_MULTICAST, 1);
					break;
				}
			}

			for(port=0; port < SWITCH_LOGICAL_PORT_TOTAL; port++) {
				if ((1<<port) & switch_get_uni_logical_portmask()) {
					switch(pkt_type)
					{
						case STORM_UNKNOWN_UNICAST:
							switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_UNKNOWN_UNICAST, (rate)?1:0);
							switch_hw_g.rate_storm_set(port, STORM_UNKNOWN_UNICAST, rate, 1);
						break;
						case STORM_BROADCAST:
							switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_BROADCAST, (rate)?1:0);
							switch_hw_g.rate_storm_set(port, STORM_BROADCAST, rate, 1);
						break;
						case STORM_UNKNOWN_MULTICAST:
							switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_UNKNOWN_MULTICAST, (rate)?1:0);
							switch_hw_g.rate_storm_set(port, STORM_UNKNOWN_MULTICAST, rate, 1);
							//know multicast should not be filter
							//switch_hw_g.rate_storm_set(port, STORM_MULTICAST, rate, 1);
						break;
					}
					if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) &&
					(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX)) {
						if(pkt_type == STORM_UNKNOWN_UNICAST) {
							// calix don't want unknown-unicast to be limited
							dbprintf(LOG_WARNING, "disable storm rate limit for pkt_type=%d port=%d\n", pkt_type, port);
							switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_UNKNOWN_UNICAST, 0);
						}
					}
				}
			}

			//If rate is zero, disable storm limit.
			if(!rate) {
				switch(pkt_type)
				{
					case STORM_UNKNOWN_UNICAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_UNICAST, 0);
					break;
					case STORM_BROADCAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_BROADCAST, 0);
					break;
					case STORM_UNKNOWN_MULTICAST:
						switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_MULTICAST, 0);
					break;
				}
			}
			if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX)) {
				if(pkt_type == STORM_UNKNOWN_UNICAST) {
					// calix don't want unknown-unicast to be limited
					dbprintf(LOG_WARNING, "disable storm rate limit for pkt_type=%d\n", pkt_type);
					switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_UNICAST, 0);
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
		if (er_group_hw_port_storm_control(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask, pkt_type) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		dbprintf(LOG_WARNING, "disable storm rate limit for pkt_type=%d\n", pkt_type);
		for(port=0; port < SWITCH_LOGICAL_PORT_TOTAL; port++) {
			if ((1<<port) & switch_get_uni_logical_portmask()) {
				switch(pkt_type)
				{
					case STORM_UNKNOWN_UNICAST:
						switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_UNKNOWN_UNICAST, 0);
					break;
					case STORM_BROADCAST:
						switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_BROADCAST, 0);
					break;
					case STORM_UNKNOWN_MULTICAST:
						switch_hw_g.rate_storm_port_enable_set(port, STORM_GROUP_UNKNOWN_MULTICAST, 0);
					break;
				}
			}
		}
		switch(pkt_type)
		{
			case STORM_UNKNOWN_UNICAST:
				switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_UNICAST, 0);
				util_write_file("/proc/re8686_ext", "storm_us_ucast %d\n", 0);
			break;
			case STORM_BROADCAST:
				switch_hw_g.rate_storm_enable_set(STORM_GROUP_BROADCAST, 0);
				util_write_file("/proc/re8686_ext", "storm_us_bcast %d\n", 0);
			break;
			case STORM_UNKNOWN_MULTICAST:
				switch_hw_g.rate_storm_enable_set(STORM_GROUP_UNKNOWN_MULTICAST, 0);
				util_write_file("/proc/re8686_ext", "storm_us_mcast %d\n", 0);
			break;
		}

		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

int
er_group_hw_port_storm_control_unknow_unicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_UNKNOWN_UNICAST);
}

int
er_group_hw_port_storm_control_broadcast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_BROADCAST);
}

int
er_group_hw_port_storm_control_unknow_multicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_UNKNOWN_MULTICAST);
}

