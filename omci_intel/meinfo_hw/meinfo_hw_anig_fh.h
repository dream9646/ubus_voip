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
 * File    : meinfo_hw_anig_fh.h
 *
 ******************************************************************/
#ifndef __MEINFO_HW_UTIL_ANIG_FH_H__
#define __MEINFO_HW_UTIL_ANIG_FH_H__

/* meinfo_hw_anig_fh.c */
int meinfo_hw_anig_fh_alarm_rxpower_enabled(void);
int meinfo_hw_anig_fh_alarm_txpower_enabled(void);
int meinfo_hw_anig_fh_alarm_txbias_enabled(void);
int meinfo_hw_anig_fh_alarm_check_rxpower(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result);
int meinfo_hw_anig_fh_alarm_check_txpower(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result);
int meinfo_hw_anig_fh_alarm_check_txbias(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result);
int meinfo_hw_anig_fh_alarm_check_voltage(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result);
int meinfo_hw_anig_fh_alarm_check_temperature(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result);

#endif
