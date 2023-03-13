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
 * File    : er_group_hw_53_65283.c
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
#include "er_group_hw.h"
#include "voip_hw.h"
#include "cli.h"
#include "switch.h"

#define STRING_LEN	256

//53@inst,65283@9,65283@10
//me 65283
/*
 1: LocalUdpPort (2byte,unsigned integer,R)
 2: RemIpAdd (4byte,unsigned integer,R)
 3: RemUdpPort (2byte,unsigned integer,R)
 4: SecRemIpAddr (4byte,unsigned integer-ipv4,R)
 5: SecRemUdpPort (2byte,unsigned integer,R)
 6: Qos (4byte,unsigned integer,R)
 7: IpLineStatus (2byte,unsigned integer,R)
 8: Active911Call (1byte,enumeration,R)
 9: ClrErrorCntrs (1byte,unsigned integer,W)
10: ClrCallCntrs (1byte,unsigned integer,W)
*/

int
er_group_hw_voip_call_statistics_extension4(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char port_id, clrerrorcntrs, clrcallcntrs;
	char cmdstr[512];
	//struct voip_hw_voip_call_statistics_extension4_t vcse4;
	//int ret=-1;

	if (attr_inst == NULL)
		return -1;

	port_id= (unsigned char)attr_inst->er_attr_instance[0].attr_value.data;
	//reset the error counters in Calix Voip Call Statistics Extension 3
	clrerrorcntrs= (unsigned char)attr_inst->er_attr_instance[1].attr_value.data;

	//reset the call counters in Calix Voip Statistics and Calix Voip statistics extension 2
	clrcallcntrs= (unsigned char)attr_inst->er_attr_instance[2].attr_value.data;

	dbprintf(LOG_NOTICE, "action=%d, port_id=%d, clrerrorcntrs=%d, clrcallcntrs=%d\n", action, port_id, clrerrorcntrs, clrcallcntrs);
	dbprintf(LOG_NOTICE, "attr_order9=%d, attr_order10=%d\n", util_attr_mask_get_bit(attr_mask,9), util_attr_mask_get_bit(attr_mask,10));

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		
	//The customer requested that the corresponding action be performed even if this value is set to 0

                if (util_attr_mask_get_bit(attr_mask,9)) {
			//Only need event, don't need really to set attribute value to 1
			//fvcli don't have exact corresponding command
			snprintf(cmdstr, 512, "fvcli -p set callstatsclear %d 0x003f", port_id);
			util_run_by_system(cmdstr);

			snprintf(cmdstr, 512, "fvcli -p set rtpstatsclear %d 0x01ff", port_id);
			util_run_by_system(cmdstr);
                }

                if (util_attr_mask_get_bit(attr_mask,10)) {
			//Only need event, don't need really to set attribute value to 1
			//fvcli don't have exact corresponding command
			snprintf(cmdstr, 512, "fvcli -p set callstatsclear %d 0x003f", port_id);
			util_run_by_system(cmdstr);

			snprintf(cmdstr, 512, "fvcli -p set rtpstatsclear %d 0x01ff", port_id);
			util_run_by_system(cmdstr);
                }
#if 0
		//set and clear, after execute set to 0
		memset(&vcse4, 0x00, sizeof(vcse4));
		if ((ret = er_group_hw_voip_wrapper_voip_call_statistics_extension4(VOIP_HW_ACTION_SET, port_id, attr_mask, &vcse4)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
#endif
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_voip_call_statistics_extension4(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

