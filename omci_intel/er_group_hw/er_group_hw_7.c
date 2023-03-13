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
 * File    : er_group_hw_7.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "omcimain.h"
#include "omcimsg.h"
#include "switch.h"

// 7 Software_image

//7@2,7@3,7@4

#define	SW_TRYACTIVE	100	
#define	SW_COMMIT	101	
#define	SW_VALID0	102	
#define	SW_VALID1	103	
#define SW_ACTIVE0_OK	104
#define SW_ACTIVE1_OK	105
#define STRING_LEN      	256

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#define SW_REBOOT_DELAY (1 * 1000)
#endif

/*
u-boot-env parameter: 

sw_commit	image0	image1
--------------------------
0		1	0
1		0	1

sw_tryactive 	image0	image1
--------------------------
0		1	0	boot image0
1		0	1	boot image1
2		according commit flag

sw_valid0	image0	image1
--------------------------
0		0	X
1		1	X

sw_valid1	image0	image1
--------------------------
0		X	0
1		X	1

*/

int 
er_group_hw_set_bootenv(unsigned char parameter, unsigned char value)
{
	int ret;
	char *key=NULL;
	char strbuf[STRING_LEN];

	unlink("/var/run/swimage/dump_env.log");

	switch(parameter) {
	case SW_TRYACTIVE:	key="sw_tryactive"; break;
	case SW_COMMIT:		key="sw_commit"; break;
	case SW_VALID0:		key="sw_valid0"; break;
	case SW_VALID1:		key="sw_valid1"; break;
	case SW_ACTIVE0_OK:	key="sw_active0_ok"; break;
	case SW_ACTIVE1_OK:	key="sw_active1_ok"; break;
	default:
		dbprintf(LOG_ERR, "Error parameter to set bootenv\n");
		return -1;
	}

	snprintf(strbuf, STRING_LEN, "fw_setenv %s %d", key, value);
	ret=util_run_by_system(strbuf);

	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "fw_setenv error ret=%d, parameter=%d, value=%d\n", WEXITSTATUS(ret), parameter, value);
		return -1;
	}
	return 0;
}

int
er_group_hw_update_commit_env(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	unsigned char value, commit;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (omci_is_ready() == 0 || omcimsg_mib_reset_is_in_progress() == 1)
			return 0;
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		value=current_attr_inst->er_attr_instance[0].attr_value.data;
		if( me->instance_num == 0) {
			if (value ==1)
				commit = 0;
			else
				commit = 1;
		} else if( me->instance_num == 1) {
			if (value ==1)
				commit = 1;
			else
				commit = 0;
		} else {
			return 0;
		}

		dbprintf(LOG_ERR, "SW_COMMIT, commit=%d\n", commit);
		if ((ret = er_group_hw_set_bootenv(SW_COMMIT, commit)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			er_attr_group_util_release_attrinst(current_attr_inst);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		dbprintf(LOG_NOTICE,"Not require action\n");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

int
er_group_hw_update_active_env(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	unsigned char value, active;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (omci_is_ready() == 0 || omcimsg_mib_reset_is_in_progress() == 1)
			return 0;
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		value=current_attr_inst->er_attr_instance[0].attr_value.data;
		if( me->instance_num == 0) {
			if (value ==1)
				active= 0;
			else
				active= 1;
		} else if( me->instance_num == 1) {
			if (value ==1)
				active = 1;
			else
				active = 0;
		} else {
			return 0;
		}

		dbprintf(LOG_ERR, "SW_TRYACTIVE, active=%d\n", active);
		if ((ret = er_group_hw_set_bootenv(SW_TRYACTIVE, active)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			er_attr_group_util_release_attrinst(current_attr_inst);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		dbprintf(LOG_NOTICE,"Not require action\n");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

int
er_group_hw_update_valid_env(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	unsigned char value;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (omci_is_ready() == 0 || omcimsg_mib_reset_is_in_progress() == 1)
			return 0;
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		value=current_attr_inst->er_attr_instance[0].attr_value.data;
		if( me->instance_num == 0){
			dbprintf(LOG_ERR, "SW_VALID0, value=%d\n", value);
			if ((ret = er_group_hw_set_bootenv(SW_VALID0, value)) < 0) {
				dbprintf(LOG_ERR, "ret=%d\n", ret);
				er_attr_group_util_release_attrinst(current_attr_inst);
				return -1;
			}

			if( value == 0 ) {	//active0_ok should reset to 0
				dbprintf(LOG_ERR, "SW_ACTIVE0_OK, value=%d\n", value);
				if ((ret = er_group_hw_set_bootenv(SW_ACTIVE0_OK, value)) < 0) {
					dbprintf(LOG_ERR, "ret=%d\n", ret);
					er_attr_group_util_release_attrinst(current_attr_inst);
					return -1;
				}
			}

		} else if( me->instance_num == 1){
			dbprintf(LOG_ERR, "SW_VALID1, value=%d\n", value);
			if ((ret = er_group_hw_set_bootenv(SW_VALID1, value)) < 0) {
				dbprintf(LOG_ERR, "ret=%d\n", ret);
				er_attr_group_util_release_attrinst(current_attr_inst);
				return -1;
			}

			if( value == 0 ) {	//active1_ok should reset to 0
				dbprintf(LOG_ERR, "SW_ACTIVE1_OK, value=%d\n", value);
				if ((ret = er_group_hw_set_bootenv(SW_ACTIVE1_OK, value)) < 0) {
					dbprintf(LOG_ERR, "ret=%d\n", ret);
					er_attr_group_util_release_attrinst(current_attr_inst);
					return -1;
				}
			}
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		dbprintf(LOG_NOTICE,"Not require action\n");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
