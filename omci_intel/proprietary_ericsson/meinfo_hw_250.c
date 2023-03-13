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
 * Module  : proprietary_tellion
 * File    : meinfo_hw_250.c
 *
 ******************************************************************/

#include <string.h>
#include <limits.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "meinfo_hw_anig.h"
#include "meinfo_hw_rf.h"
#include "util.h"
#include "gpon_sw.h"

//classid 250 Optical_Module_Monitor

static int
meinfo_hw_250_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	int ret_opt = -1, ret_rf = -1;
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	struct rf_result_t rf_result;

	// get counters from hw
	ret_opt = meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw);
	ret_rf = meinfo_hw_rf_get_threshold_result(&rf_result);

	if (util_attr_mask_get_bit(attr_mask, 1)) {	// Identifier
		char vendor_id[4];
		unsigned int serial_number;
		if (omci_env_g.ont_force_vendor_id[0] >= 0x30)
			strncpy(vendor_id, omci_env_g.ont_force_vendor_id, sizeof(vendor_id));
		else if (gpon_hw_g.onu_serial_number_get)
			gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);
		attr.ptr = malloc_safe(16);
		if (attr.ptr) {
			memcpy(attr.ptr, vendor_id, sizeof(vendor_id));
			meinfo_me_attr_set(me, 1, &attr, 0);
			free_safe(attr.ptr);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 2)) {	// Receive_Power
		if (ret_opt >= 0) {
			unsigned short value;
			if (diag_result.rx_power<0.0001)
				diag_result.rx_power=0.0001;
			value=(unsigned short)(uw_to_dbm(diag_result.rx_power)/0.002);    // 0.002dbm
			attr.ptr=NULL;
			attr.data=(unsigned short)value;
			meinfo_me_attr_set(me, 2, &attr, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 3)) {	// Transmit_Power
		if (ret_opt >= 0) {
			unsigned short value;
			if (diag_result.tx_power<0.0001)
				diag_result.tx_power=0.0001;
			value=(unsigned short)(uw_to_dbm(diag_result.tx_power)/0.002);    // 0.002dbm
			attr.ptr=NULL;
			attr.data=(unsigned short)value;
			meinfo_me_attr_set(me, 3, &attr, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 4)) {	// Temperature
		if (ret_opt >= 0) {
			char value;
			if (diag_result.temperature<0.0001)
				diag_result.temperature=0.0001;
			value=(char) (diag_result.temperature);    // c
			if (value < CHAR_MIN) value = CHAR_MIN;
			if (value > CHAR_MAX) value = CHAR_MAX;
			attr.ptr=NULL;
			attr.data=(char)value;
			meinfo_me_attr_set(me, 4, &attr, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 5)) {	// Bias_Current
		if (ret_opt >= 0) {
			unsigned short value;
			if (diag_result.tx_bias_current<0.0001)
				diag_result.tx_bias_current=0.0001;
			value=(unsigned short)(diag_result.tx_bias_current*1000);    // mA->uA
			attr.ptr=NULL;
			attr.data=(unsigned short)value;
			meinfo_me_attr_set(me, 5, &attr, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 6)) {	// Supply_Voltage
		if (ret_opt >= 0) {
			unsigned short value;
			if (diag_result.vcc<0.0001)
				diag_result.vcc=0.0001;
			value=(unsigned short)(diag_result.vcc*1000);    // v->mv
			attr.ptr=NULL;
			attr.data=(unsigned short)value;
			meinfo_me_attr_set(me, 6, &attr, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 7)) {	// RF_Receive_Power
		if (ret_rf >= 0) {
			unsigned short value;
			if(rf_result.opt_rx != 0xffff)
				value=(unsigned short)(uw_to_dbm(rf_result.opt_rx)/0.002);    // 0.002dbm
			attr.ptr=NULL;
			attr.data=(unsigned short)value;
			meinfo_me_attr_set(me, 7, &attr, 0);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_ericsson_250 = {
	.get_hw		= meinfo_hw_250_get,
};
