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
 * File    : meinfo_hw_131.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "gpon_sw.h"

static int 
meinfo_hw_131_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 4)) {	// Time of day information
	
		struct attr_value_t attr;
		struct timeval tv;
		unsigned char tod_bitmap[14];
		unsigned long long superframer_seq = 0;
		
		// get current time
		gettimeofday(&tv, NULL);
		// get superframe count
		if (gpon_hw_g.onu_superframe_seq_get)
			gpon_hw_g.onu_superframe_seq_get(&superframer_seq);

		// set to meinfo attr
		memset(tod_bitmap, 0x00, sizeof(tod_bitmap));
		// attr4 field1: supreframe sequence number (4 bytes)
		util_bitmap_set_value(tod_bitmap, sizeof(tod_bitmap)*8, 0, 32, superframer_seq);
		// attr4 field2: TstampN seconds_field (6 bytes)
		util_bitmap_set_value(tod_bitmap, sizeof(tod_bitmap)*8, 32, 48, tv.tv_sec);
		// attr4 field2: TstampN nanoseconds_field (4 bytes)
		util_bitmap_set_value(tod_bitmap, sizeof(tod_bitmap)*8, 80, 32, tv.tv_usec*1000);
		
		//dbprintf(LOG_ERR, "Set TstampN: tv_sec(%dB)=0x%012x, tv_usec(%dB)=0x%08x, nsec=0x%08x\n",sizeof(tv.tv_sec), tv.tv_sec, sizeof(tv.tv_usec), tv.tv_usec, tv.tv_usec*1000);
		
		attr.ptr = tod_bitmap;
		meinfo_me_attr_set(me, 4, &attr, 0); 
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_131 = {
	.get_hw		= meinfo_hw_131_get
};


