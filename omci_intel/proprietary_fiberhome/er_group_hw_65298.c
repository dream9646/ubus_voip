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
 * Module  : proprietary_fiberhome
 * File    : er_group_hw_65298.c
 *
 ******************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"


static int
er_group_hw_defence(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2], char *defense_attr)
{
//	unsigned short inst;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned short value;

	if (attr_inst == NULL)
		return -1;

//	inst=attr_inst->er_attr_instance[0].attr_value.data;
	value=attr_inst->er_attr_instance[0].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (util_write_file("/proc/defense", "%s %d\n", defense_attr, value) <0) {
			dbprintf(LOG_ERR,"/proc/defense not found?\n");
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
		value=current_attr_inst->er_attr_instance[0].attr_value.data;
		if (util_write_file("/proc/defense", "%s %d\n", defense_attr, value) <0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR,"/proc/defense not found?\n");
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

int
er_group_hw_mac_land_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "mac_land_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_ip_land_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "ip_land_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_tcp_blat_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_blat_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_udp_blat_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "udp_blat_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_tcp_null_scan_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_null_scan_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_xmas_scan_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_xmas_scan_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_tcp_syn_fin_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_syn_fin_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_tcp_syn_err_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_syn_err_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_tcp_short_hdr_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_short_hdr_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_tcp_frag_err_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "tcp_frag_err_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_icmpv4_frag_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "icmpv4_frag_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_icmpv6_frag_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "icmpv6_frag_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}

int
er_group_hw_icmpv4_longping_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "icmpv4_longping_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}


int
er_group_hw_icmpv6_longping_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char defense_attr[] = "icmpv6_longping_attack_alarm";
	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
}
