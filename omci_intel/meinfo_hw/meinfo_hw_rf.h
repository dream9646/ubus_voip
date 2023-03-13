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
 * File    : meinfo_hw_rf.h
 *
 ******************************************************************/

#ifndef __MEINFO_HW_UTIL_RF_H__
#define __MEINFO_HW_UTIL_RF_H__

struct rf_result_t {
	double rf_tx;		// dBmV
	double opt_rx;		// dBm
	double agc;		// dB
	unsigned int rf_en;
	unsigned int opt_en;
	unsigned int agc_en;
};

struct rf_opt_history_t {
	double opt_rx;
	time_t tv_sec;
};

/* meinfo_hw_rf.c */
int meinfo_hw_rf_get_threshold_result(struct rf_result_t *r);
int meinfo_hw_rf_print_threshold_result(int fd, struct rf_result_t *r);
int meinfo_hw_rf_set_admin_state(int enable);
int meinfo_hw_rf_set_opt_state(int enable);
int meinfo_hw_rf_set_agc_state(int enable);
int meinfo_hw_rf_set_agc_offset(char);

/* meinfo_hw_90.c */
extern struct rf_opt_history_t rf_opt_history[60];
#endif
