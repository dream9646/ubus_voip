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
 * Module  : proprietary_calix
 * File    : meinfo_hw_65502.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

//classid 65502 Serial_Number

static int 
meinfo_hw_65502_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}
	
	// 1: MFGER_Serial_Number
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		unsigned char *sn, mac[6];
		attr.data=0;
		attr.ptr= malloc_safe(12);
		memset(mac, 0, sizeof(mac));
		if ((sn=util_get_bootenv("calix_serial_number")) != NULL) {
			sprintf(attr.ptr, "%s", sn);
		} else {
			if (switch_hw_g.eth_address_get)
				switch_hw_g.eth_address_get(0, mac);
			sprintf(attr.ptr, "33%02X%02X%02X%02X%02X",mac[1],mac[2],mac[3],mac[4],mac[5]);
		}
		meinfo_me_attr_set(me, 1, &attr, 0);
		if (attr.ptr)
			free_safe(attr.ptr);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65502 = {
	.get_hw		= meinfo_hw_65502_get,
};
