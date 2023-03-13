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
 * File    : meinfo_hw_257.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"

//ONT2_G section: 9.1.2
static int 
meinfo_hw_257_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 10)) {	// SysUpTime
		FILE *fp = fopen("/proc/uptime", "r");
		if(fp) {
			struct attr_value_t attr;
			char *p1, *p2, strbuf[128];
			fgets(strbuf, sizeof(strbuf), fp);
			p1 = strtok(strbuf,".");
			p2 = strtok(NULL," ");
			attr.data = atoi(p1) * 100 + atoi(p2); // unit: 10ms
			meinfo_me_attr_set(me, 10, &attr, 0); 
			fclose(fp);
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_257 = {
	.get_hw			= meinfo_hw_257_get,
};

