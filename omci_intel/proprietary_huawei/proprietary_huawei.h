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
 * Module  : proprietary_huawei
 * File    : proprietary_huawei.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_HUAWEI_H__
#define __PROPRIETARY_HUAWEI_H__

/* er_group_hw_171.c */
int er_group_hw_ignore_vlan_drop_rule(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_367.c */
int er_group_hw_l2s_tags_create_add01_multicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* proprietary_huawei.c */
int proprietary_huawei(void);
int proprietary_huawei_get_voip_wan(struct me_t *);

#endif
