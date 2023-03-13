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
 * Module  : er_group_hw
 * File    : er_group_hw_334.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "meinfo_hw_util.h"
#include "er_group.h"
#include "cli.h"
#include "hwresource.h"
#include "switch.h"

//334@2.4
int
er_group_hw_334_accum_disable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
        unsigned short *pCB;
        unsigned short accum_clear;
        int i;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
                pCB = (unsigned short *)attr_inst->er_attr_instance[0].attr_value.ptr;
                if (pCB == NULL) return 0;
                accum_clear = (*(pCB +3) & 0x8000)?1:0;
                dbprintf(LOG_DEBUG,"ME 334 accum disable(0x%x) global clear %d\n", *(pCB +3), accum_clear);
                if (accum_clear)
                {
                        switch_hw_g.pm_refresh(accum_clear);
                        
                        for (i = 3;i <= meinfo_util_get_attr_total(334); i++)
                        {
                                meinfo_me_attr_clear_pm(me, i);       
                        }
                }
                ((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[3] = ntohs( *(pCB +3) & 0x7FFF);
		
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_334_accum_disable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}
        
        return 0;
}


