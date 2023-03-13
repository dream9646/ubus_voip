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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65281.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "meinfo_hw.h"
#include "gpon_sw.h"

static int 
meinfo_hw_65281_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct gpon_counter_global_t counter_global;
	int i;

	gpon_hw_g.counter_global_get(&counter_global); 		
	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: // Interval_End_Time
			//case 2: // Threshold_Data_1/2_Id
			case 3: // Lost_GEM_Fragment_Counter_Downstream
				meinfo_me_attr_update_pm(me, i, counter_global.rx_gem_los );
				break;
			//case 4: // Lost_GEM_Fragment_Counter_Upsteam
			case 5: // Received_GEM_Fragment_Counter
				meinfo_me_attr_update_pm(me, i, 0);
				break;
			case 6: // Received_GEM_Blocks
			case 11: // Received_GEM_Blocks_Counter_64
				meinfo_me_attr_update_pm(me, i, counter_global.rx_gem_non_idle);
				break;
			case 7: // Transmitted_GEM_Blocks
			case 12: // Transmitted_GEM_Blocks_Counter_64
				meinfo_me_attr_update_pm(me, i, counter_global.tx_gem_cnt);
				break;
			case 8: // Impaired_Blocks
				meinfo_me_attr_update_pm(me, i, counter_global.rx_gem_len_mis);
				break;
			//case 9: // Transmitted_GEM_Fragment_Counter
			case 10: // Bad_GEM_Headers_Received_Counter
				meinfo_me_attr_update_pm(me, i, 0);
				break;
			default:
				continue;//do nothing
			}
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65281 = {
	.get_hw		= meinfo_hw_65281_get
};
