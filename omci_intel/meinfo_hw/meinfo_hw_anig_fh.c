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
 * File    : meinfo_hw_anig_fh.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <math.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"
#include "meinfo_hw_anig.h"
#include "meinfo_hw_anig_fh.h"

static int
meinfo_hw_anig_fh_alarm_get_threshold(unsigned short alarm_code, double *alarm_threshold, double *clear_threshold, unsigned short *alarm_enable)
{
	struct me_t *me = meinfo_me_get_by_meid(65297, alarm_code);
	unsigned char *a, *c;
	unsigned char extend_type, precision;
	int value;
	
	if (me==NULL)
		return -1;
	
	a=meinfo_util_me_attr_ptr(me, 2);
	c=meinfo_util_me_attr_ptr(me, 3);

	extend_type = (unsigned char)util_bitmap_get_value(a, 48, 0, 8);
	precision = (unsigned char)util_bitmap_get_value(a, 48, 8, 8);
	value = (unsigned char)util_bitmap_get_value(a, 48, 16, 32);
	if (extend_type == 0)
		*alarm_threshold = value / pow(10, precision);
	else
		return -2;
	
	extend_type = (unsigned char)util_bitmap_get_value(c, 48, 0, 8);
	precision = (unsigned char)util_bitmap_get_value(c, 48, 8, 8);
	value = (unsigned char)util_bitmap_get_value(c, 48, 16, 32);
	if (extend_type == 0)
		*clear_threshold = value / pow(10, precision);
	else
		return -3;
		
	*alarm_enable = (unsigned short)meinfo_util_me_attr_data(me, 4);	
	return 0;
}

static int
meinfo_hw_anig_fh_alarm_is_enable(unsigned short alarm_code)
{
	struct me_t *me = meinfo_me_get_by_meid(65297, alarm_code);
	if (me)
		return meinfo_util_me_attr_data(me, 4)?1:0;
	return 0;
}

static int
meinfo_hw_anig_fh_alarm_check_upperbound(struct meinfo_t *miptr, unsigned char alarm_mask[28], unsigned short alarm_number, 
					double value, unsigned short alarm_code, unsigned warning_code, char *prompt_str)
{
	int alarm_is_on=-1;	// 0 alarm is off, 1 alarm is on, -1 alarm remains unchanged
	
	unsigned short alarm_enable=0;
	double alarm_threshold, clear_threshold;	// for alarm
	unsigned short alarm_enable2=0;
	double alarm_threshold2, clear_threshold2;	// for warning

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, alarm_number))
		return alarm_is_on;

	meinfo_hw_anig_fh_alarm_get_threshold(alarm_code, &alarm_threshold, &clear_threshold, &alarm_enable);
	meinfo_hw_anig_fh_alarm_get_threshold(warning_code, &alarm_threshold2, &clear_threshold2, &alarm_enable2);

	if (value > alarm_threshold) {
		if (!util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
			dbprintf(LOG_ERR, "%s: value(%.3f) > max(%.3f), alarm %d is set\n", prompt_str, value, alarm_threshold, alarm_number);
			util_alarm_mask_set_bit(alarm_mask, alarm_number);
		}
		alarm_is_on=1;

	} else if (clear_threshold < alarm_threshold && clear_threshold >= alarm_threshold-3) {	
		// utilize the clear_threshold only if alarm_threshold -3 <= clear_threshold < alarm_threshold
		if (value < clear_threshold) {
	 		if (util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
				dbprintf(LOG_ERR, "%s: value(%.3f) <= clear_max(%.3f), alarm %d is cleared\n", prompt_str, value, clear_threshold, alarm_number);
				util_alarm_mask_clear_bit(alarm_mask, alarm_number);
				alarm_is_on=0;
			}
		}

	} else if (value <= alarm_threshold) {
 		if (util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
			dbprintf(LOG_ERR, "%s: value(%.3f) <= max(%.3f), alarm %d is cleared\n", prompt_str, value, alarm_threshold, alarm_number);
			util_alarm_mask_clear_bit(alarm_mask, alarm_number);
		}
		alarm_is_on=0;
	}
	return alarm_is_on;
}

static int
meinfo_hw_anig_fh_alarm_check_lowerbound(struct meinfo_t *miptr, unsigned char alarm_mask[28], unsigned short alarm_number, 
					double value, unsigned short alarm_code, unsigned warning_code, char *prompt_str)
{
	int alarm_is_on=-1;	// 0 alarm is off, 1 alarm is on, -1 alarm remains unchanged
	
	unsigned short alarm_enable=0;
	double alarm_threshold, clear_threshold;	// for alarm
	unsigned short alarm_enable2=0;
	double alarm_threshold2, clear_threshold2;	// for warning

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, alarm_number))
		return alarm_is_on;

	meinfo_hw_anig_fh_alarm_get_threshold(alarm_code, &alarm_threshold, &clear_threshold, &alarm_enable);
	meinfo_hw_anig_fh_alarm_get_threshold(warning_code, &alarm_threshold2, &clear_threshold2, &alarm_enable2);
	
	if (value < alarm_threshold) {
		if (!util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
			dbprintf(LOG_ERR, "%s: value(%.3f) < min(%.3f), alarm %d is set\n", prompt_str, value, alarm_threshold, alarm_number);
			util_alarm_mask_set_bit(alarm_mask, alarm_number);
			alarm_is_on=1;
		}

	} else if (clear_threshold > alarm_threshold && clear_threshold <= alarm_threshold +3) {
		// utilize the clear_threshold only if alarm_threshold < clear_threshold <= alarm_threshold +3
		if (value > clear_threshold) {
	 		if (util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
				dbprintf(LOG_ERR, "%s: value(%.3f) >= clear_min(%.3f), alarm %d is cleared\n", prompt_str, value, clear_threshold, alarm_number);
				util_alarm_mask_clear_bit(alarm_mask, alarm_number);
				alarm_is_on=0;
			}
		}

	} else if (value >= alarm_threshold) {
 		if (util_alarm_mask_get_bit(alarm_mask, alarm_number)) {
			dbprintf(LOG_ERR, "%s: value(%.3f) >= min(%.3f), alarm %d is cleared\n", prompt_str, value, alarm_threshold, alarm_number);
			util_alarm_mask_clear_bit(alarm_mask, alarm_number);
			alarm_is_on=0;
		}
	}
	return alarm_is_on;
}

//////////////////////////////////////////////////////////////////

int
meinfo_hw_anig_fh_alarm_rxpower_enabled(void)
{
	return (meinfo_hw_anig_fh_alarm_is_enable(997) || meinfo_hw_anig_fh_alarm_is_enable(998));
}
int
meinfo_hw_anig_fh_alarm_txpower_enabled(void)
{
	return (meinfo_hw_anig_fh_alarm_is_enable(999) || meinfo_hw_anig_fh_alarm_is_enable(1000));
}
int
meinfo_hw_anig_fh_alarm_txbias_enabled(void)
{
	return (meinfo_hw_anig_fh_alarm_is_enable(1001) || meinfo_hw_anig_fh_alarm_is_enable(1002));
}
int
meinfo_hw_anig_fh_alarm_check_rxpower(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	if (meinfo_hw_anig_fh_alarm_is_enable(997))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_upperbound(miptr, alarm_mask, 1, uw_to_dbm(diag_result->rx_power), 997, 1007, "rx(dbm)");
	if (meinfo_hw_anig_fh_alarm_is_enable(998))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_lowerbound(miptr, alarm_mask, 0, uw_to_dbm(diag_result->rx_power), 998, 1008, "rx(dbm)");
	return alarm_is_on;
}
int
meinfo_hw_anig_fh_alarm_check_txpower(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	if (meinfo_hw_anig_fh_alarm_is_enable(999))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_upperbound(miptr, alarm_mask, 5, uw_to_dbm(diag_result->tx_power), 999, 1009, "tx(dbm)");
	if (meinfo_hw_anig_fh_alarm_is_enable(1000))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_lowerbound(miptr, alarm_mask, 4, uw_to_dbm(diag_result->tx_power), 1000, 1010, "tx(dbm)");
	return alarm_is_on;
}
int
meinfo_hw_anig_fh_alarm_check_txbias(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	if (meinfo_hw_anig_fh_alarm_is_enable(1001))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_upperbound(miptr, alarm_mask, 213, diag_result->tx_bias_current, 1001, 1011, "bias current(mA)");
	if (meinfo_hw_anig_fh_alarm_is_enable(1002))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_lowerbound(miptr, alarm_mask, 212, diag_result->tx_bias_current, 1002, 1012, "bias current(mA)");
	return alarm_is_on;
}
int
meinfo_hw_anig_fh_alarm_check_voltage(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	if (meinfo_hw_anig_fh_alarm_is_enable(1003))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_upperbound(miptr, alarm_mask, 211, diag_result->vcc, 1003, 1013, "Vcc(V)");
	if (meinfo_hw_anig_fh_alarm_is_enable(1004))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_lowerbound(miptr, alarm_mask, 210, diag_result->vcc, 1004, 1014, "Vcc(V)");	
	return alarm_is_on;
}
int
meinfo_hw_anig_fh_alarm_check_temperature(struct meinfo_t *miptr, unsigned char alarm_mask[28], struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	if (meinfo_hw_anig_fh_alarm_is_enable(1023))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_upperbound(miptr, alarm_mask, 209, diag_result->temperature, 1023, 1015, "C degree");
    	if (meinfo_hw_anig_fh_alarm_is_enable(1024))
		alarm_is_on |= meinfo_hw_anig_fh_alarm_check_lowerbound(miptr, alarm_mask, 208, diag_result->temperature, 1024, 1016, "C degree");	
	return alarm_is_on;
}

