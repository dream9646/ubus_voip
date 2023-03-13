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
 * File    : meinfo_hw_65319.c
 *
 ******************************************************************/

#include "meinfo_hw.h"
#include "util.h"
#include "metacfg_adapter.h"
#include "batchtab.h"
#include "switch.h"
#include "hwresource.h"

// 9.99.24 LLDP_MED_Control
#ifdef OMCI_METAFILE_ENABLE
static char* meta_state_str[SWITCH_LOGICAL_PORT_TOTAL];
#endif

static int
meinfo_hw_65319_get(struct me_t *me, unsigned char attr_mask[2])
{
#ifdef OMCI_METAFILE_ENABLE
	static int is_state_str_inited = 0;
	struct metacfg_t kv_config;
	int value, op_state = 1;
	unsigned int portid;

	if (!is_state_str_inited) {
		int i;		
		for (i=0; i< SWITCH_LOGICAL_PORT_TOTAL; i++) {
			switch(i) {
				case 0: meta_state_str[i] = "lldp_port0_admin_state"; break;
				case 1: meta_state_str[i] = "lldp_port1_admin_state"; break;
				case 2: meta_state_str[i] = "lldp_port2_admin_state"; break;
				case 3: meta_state_str[i] = "lldp_port3_admin_state"; break;
				default: meta_state_str[i] = ""; break;
			}
		}
		is_state_str_inited = 1;
	}

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "lldp_management_capability", "lldp_port3_admin_state");

	value = util_atoi(metacfg_adapter_config_get_value(&kv_config, "lldp_management_capability", 1));

	// Management_Capabilities
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=value;
		meinfo_me_attr_set(me, 1, &attr, 1);
	}

	if (value == 0) {
		op_state = meinfo_util_me_attr_data(me, 2);
	} else {
		struct me_t *pptp_me = meinfo_me_get_by_instance_num(11, me->instance_num);
		struct me_t *ipptp_me = hwresource_public2private(pptp_me);
		portid = meinfo_util_me_attr_data(ipptp_me, 4);
		if((1<<portid) & switch_get_uni_logical_portmask())
			op_state = util_atoi(metacfg_adapter_config_get_value(&kv_config, meta_state_str[portid], 1));
	}

	// Operational_State
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=op_state;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}

	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65319 = {
	.get_hw			= meinfo_hw_65319_get,
};

