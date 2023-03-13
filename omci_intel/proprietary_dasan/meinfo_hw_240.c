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
 * Module  : proprietary_dasan
 * File    : meinfo_hw_240.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_hw.h"
#include "meinfo_hw_util.h"
#include "util.h"

#define BOSA_REPORT_ROGUE_CHECK	"/etc/init.d/bosa.sh rogue"

//classid 240 Private_ONT_System_Management

static int
meinfo_hw_240_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	char *output=NULL;
	
	if (meinfo_hw_util_get_ontstate() != 5)  // no alarm check for anig if not in O5
		return 0;
	
	if (util_readcmd(BOSA_REPORT_ROGUE_CHECK, &output) <0 || output==NULL) {
		if (output) free_safe(output);
		return -1;
	} else {
		int rogue = 0;
		sscanf(output, "%d", &rogue);
		free_safe(output);
		if(rogue) {
			util_alarm_mask_set_bit(alarm_mask, 0);
		} else {
			util_alarm_mask_clear_bit(alarm_mask, 0);
		}
		return 0;
	}
}

struct meinfo_hardware_t meinfo_hw_dasan_240 = {
	.alarm_hw	= meinfo_hw_240_alarm
};
