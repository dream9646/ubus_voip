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
 * File    : proprietary_dasan.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_DASAN_H__
#define __PROPRIETARY_DASAN_H__

/* er_group_hw_240.c */
int er_group_hw_self_rogue_detect(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_244.c */
int er_group_hw_self_loop_detect(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* proprietary_dasan.c */
int proprietary_dasan(void);

#endif
