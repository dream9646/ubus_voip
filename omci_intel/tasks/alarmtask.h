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
 * Module  : tasks
 * File    : alarmtask.h
 *
 ******************************************************************/

#ifndef __ALARMTASK_H__
#define __ALARMTASK_H__

enum alarmtask_cmd_t {
	ALARMTASK_CMD_PROCESS,
	ALARMTASK_CMD_UPDATE,
	ALARMTASK_CMD_TERMINATE
};

struct alarmtask_msg_t {
	enum alarmtask_cmd_t cmd;
	unsigned short classid;			//carry user's data for alarmtask handler
	unsigned short instance_num;		//if classid=0, instance_num=0, check all me
	unsigned char alarm_mask[28]; //for update cmd to mask updated alarm
	unsigned char alarm_value[28]; //for update cmd to represent alarm vlaue
	struct list_head alarmtask_node;	//for alarmtask queue
};

int alarmtask_init();
int alarmtask_start();
int alarmtask_shutdown();
int alarmtask_stop();
int alarmtask_send_msg(unsigned short classid, unsigned short instance_num);
int alarmtask_send_update_msg(unsigned short classid, unsigned short instance_num, unsigned char *alarm_mask, unsigned char *alarm_value);
int alarmtask_boot_notify(unsigned char event,
                          unsigned short classid, 
			  unsigned short instance_num, unsigned short meid, void *data_ptr);

#endif
