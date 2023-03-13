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
 * File    : meinfo_hw_poe.h
 *
 ******************************************************************/

#ifndef __MEINFO_HW_UTIL_POE_H__
#define __MEINFO_HW_UTIL_POE_H__

#include "switch.h"

struct poe_report_t {
	unsigned int power;
	unsigned int enable;
	unsigned int classify;
	unsigned int priority;
	unsigned int status;
	unsigned int over_current;
	unsigned int invd_cnt;
	unsigned int pwrd_cnt;
	unsigned int ovl_cnt;
	unsigned int udl_cnt;
	unsigned int sc_cnt;
	unsigned int mps_abs_cnt;
};

struct poe_result_t {
	struct poe_report_t poe_report[SWITCH_LOGICAL_PORT_TOTAL];
};

/* meinfo_hw_poe.c */
int meinfo_hw_poe_get_threshold_result(struct poe_result_t *r);
int meinfo_hw_poe_print_threshold_result(int fd, struct poe_result_t *r);
int meinfo_hw_poe_get_management_capability(int *poe_mgt_cap);
int meinfo_hw_poe_get_power_budget(int *power_budget);
int meinfo_hw_poe_get_admin(int *poe_admin, int port_id);
int meinfo_hw_poe_set_power_budget(int power_budget);
int meinfo_hw_poe_set_admin_state(int port, int enable);
int meinfo_hw_poe_set_class(int port, int value);
int meinfo_hw_poe_set_priority(int port, int value);

#endif
