/*******************************************************************
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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65295.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo_hw.h"
#include "meinfo_hw_anig.h"
#include "util.h"

//classid 65295 ONT Optical Supervision Status

static int 
meinfo_hw_65295_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}
	
	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw) < 0)
		return -1;
	
	// 1: Power_Feed_Voltage_Measurement_Indicator
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		attr.data = 1;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	
	// 2: Power_Feed_Voltage
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		attr.data = (unsigned short)(diag_result.vcc*1000/20); // v->20mv
		meinfo_me_attr_set(me, 2, &attr, 0);
	}
	
	// 3: Receive_Optical_Power_Measurement_Indicator
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		attr.data = 1;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	
	// 4: Receive_Optical_Power
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		attr.data = (unsigned short)((uw_to_dbm(diag_result.rx_power)/0.002)); //uw->0.002dbm
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	
	// 5: Transmit_Optical_Power_Measurement_Indicator
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		attr.data = 1;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	
	// 6: Transmit_Optical_Power
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		attr.data = (unsigned short)((uw_to_dbm(diag_result.tx_power)/0.002));// uw ->0.002dbm
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	
	// 7: Laser_Bias_Current_Measurement_Indicator
	if (util_attr_mask_get_bit(attr_mask, 7)) {
		attr.data = 1;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	
	// 8: Laser_Bias_Current
	if (util_attr_mask_get_bit(attr_mask, 8)) {
		attr.data = (unsigned short)(diag_result.tx_bias_current*1000/2); // mA->2uA
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	
	// 9: Temperature_Measurement_Indicator
	if (util_attr_mask_get_bit(attr_mask, 9)) {
		attr.data = 1;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	
	// 10: Temperature
	if (util_attr_mask_get_bit(attr_mask, 10)) {
		attr.data = (unsigned short)(diag_result.temperature*256);	// c->1/256c
		meinfo_me_attr_set(me, 10, &attr, 0);
	}
	return 0;		
}

struct meinfo_hardware_t meinfo_hw_alu_65295 = {
	.get_hw		= meinfo_hw_65295_get,
};
