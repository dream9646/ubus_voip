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
 * File    : meinfo_hw_256.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "meinfo_hw.h"
#include "meinfo_hw_poe.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "switch.h"

// 9.1.1 ONT_G

static int 
meinfo_hw_256_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// attr1: vendor id
		if (meinfo_config_get_str_from_custom_mib(me->classid, me->instance_num, 1) == NULL) {
			char vendor_id[4];
			unsigned int serial_number;
			struct attr_value_t attr;

			if (omci_env_g.ont_force_vendor_id[0] >= 0x30)
				strncpy(vendor_id, omci_env_g.ont_force_vendor_id, sizeof(vendor_id));
			else if (gpon_hw_g.onu_serial_number_get)
				gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);
			
			attr.data=*((unsigned int *)vendor_id);
			meinfo_me_attr_set(me, 1, &attr, 0);	// check_avc==1 means update avc_change_mask
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 3)) {	// attr3: serial number
		if (meinfo_config_get_str_from_custom_mib(me->classid, me->instance_num, 3) == NULL) {
			char vendor_id[4];
			unsigned int serial_number;
			char buff[8];
			struct attr_value_t attr;

			if (gpon_hw_g.onu_serial_number_get)
				gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);
			
			// FIXME, KEVIN 
			// vendor id might need to use memcpy() instead of util_bitmap_set_value() for endian?
			util_bitmap_set_value(buff, 8*8, 0, 32, *((unsigned int *)vendor_id)); //id
			util_bitmap_set_value(buff, 8*8, 32, 32, serial_number); //id
			attr.ptr=buff;

			meinfo_me_attr_set(me, 3, &attr, 0);	// check_avc==1 means update avc_change_mask
		}
	}

	return 0;
}

static int 
meinfo_hw_256_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	// get faked answer from software callback
	return miptr->callback.test_is_supported_cb(me, req);
}

static int 
meinfo_hw_256_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	// get faked answer from software callback
	return miptr->callback.test_cb(me, req, result);
}

static int 
meinfo_hw_256_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	
	if (util_alarm_mask_get_bit(miptr->alarm_mask, 15)) {
		unsigned int phy_portid, ext_portid, port_id, over_current = 0;
		extern struct poe_result_t poe_result;
		
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ((1<<port_id) & switch_get_uni_logical_portmask()) {
				// port logical->physical
				switch_hw_g.portid_logical_to_private(port_id, &phy_portid, &ext_portid);
				// only when priority != 0, the result is meaningful
				// otherwise it maybe false alarm (read wrong value from i2c)
				// and skip access this time (return 0 directly)
				if(poe_result.poe_report[phy_portid].priority) {
					if (poe_result.poe_report[phy_portid].over_current==1) {
						over_current = 1;
						break;
					}
				} else {
					// wrong access from i2c
					return 0;
				}
			}
		}
		if(over_current==1)
			util_alarm_mask_set_bit(alarm_mask, 15);
		else
			util_alarm_mask_clear_bit(alarm_mask, 15);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_256 = {
	.get_hw			= meinfo_hw_256_get,
	.test_is_supported_hw	= meinfo_hw_256_test_is_supported,
	.test_hw		= meinfo_hw_256_test,
	.alarm_hw	= meinfo_hw_256_alarm,
};

