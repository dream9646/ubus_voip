/*******************************************************************
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
 * File    : meinfo_hw_65501.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_hw.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_anig.h"
#include "util.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_event.h"

//classid 65501 AniGExt

enum extoam_event_type_t {
	EXTOAM_EVENT_TYPE_TEMPERATURE,
	EXTOAM_EVENT_TYPE_VOLTAGE,
	EXTOAM_EVENT_TYPE_TXPOWER,
	EXTOAM_EVENT_TYPE_RXPOWER,
	EXTOAM_EVENT_TYPE_BIASCURRENT,
	EXTOAM_EVENT_TYPE_MAX
};

enum extoam_event_thresh_t {
	EXTOAM_EVENT_THRESH_LO_WARN,
	EXTOAM_EVENT_THRESH_HI_WARN,
	EXTOAM_EVENT_THRESH_LO_ALARM,
	EXTOAM_EVENT_THRESH_HI_ALARM,
	EXTOAM_EVENT_THRESH_MAX
};

extern struct extoam_link_status_t cpe_link_status_g;
static unsigned char extoam_event[EXTOAM_EVENT_TYPE_MAX][EXTOAM_EVENT_THRESH_MAX];
static unsigned char extoam_event_previous[EXTOAM_EVENT_TYPE_MAX][EXTOAM_EVENT_THRESH_MAX];

static int
meinfo_hw_65501_alarm_check_temperature(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	short attr_temperature_alarm_high;
	short attr_temperature_alarm_low;
	double temperature_alarm_high;
	double temperature_alarm_low;

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 2)))
		return 0;

	attr_temperature_alarm_high = (short)meinfo_util_me_attr_data(me, 7);
	attr_temperature_alarm_low = (short)meinfo_util_me_attr_data(me, 6);

	// get temperature threshold
	temperature_alarm_high   = ((unsigned short) attr_temperature_alarm_high == 0x8000)?   diag_threshold->temperature_high : attr_temperature_alarm_high / 256.0;
	temperature_alarm_low    = ((unsigned short) attr_temperature_alarm_low == 0x8000)?    diag_threshold->temperature_low  : attr_temperature_alarm_low / 256.0;

	dbprintf(LOG_INFO, "temperature: hw=%.3fc, alarm_max=%.3fc, alarm_min=%.3fc\n",
		diag_result->temperature, temperature_alarm_high, temperature_alarm_low);

	if (diag_result->temperature > temperature_alarm_high) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) > temperature_alarm_high(%.3fc)\n",
			diag_result->temperature, temperature_alarm_high);
		//util_alarm_mask_set_bit(alarm_mask, 5); // Spec does not define high threshold yet
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_HIGH_ALARM );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 5);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM];

	if (diag_result->temperature < temperature_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) < temperature_alarm_low(%.3fc)\n",
			diag_result->temperature, temperature_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 2);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_LOW_ALARM);
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 2);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM];

	return alarm_is_on;
}

static int
meinfo_hw_65501_alarm_check_voltage(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	double attr_vcc_alarm_high;
	double attr_vcc_alarm_low;
	double vcc_alarm_high;
	double vcc_alarm_low;

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 0)))
		return 0;

	attr_vcc_alarm_high = (unsigned short)meinfo_util_me_attr_data(me, 3);
	attr_vcc_alarm_low = (unsigned short)meinfo_util_me_attr_data(me, 2);

	// get voltage threshold
	vcc_alarm_high   = (attr_vcc_alarm_high==0xFFFF)?   diag_threshold->vcc_high : attr_vcc_alarm_high / 50;
	vcc_alarm_low    = (attr_vcc_alarm_low==0xFFFF)?    diag_threshold->vcc_low  : attr_vcc_alarm_low / 50;

	dbprintf(LOG_INFO, "voltage: hw=%.3fv, alarm_max=%.3fv, alarm_min=%.3fv\n",
		diag_result->vcc, vcc_alarm_high, vcc_alarm_low);

	if (diag_result->vcc > vcc_alarm_high) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) > voltage_alarm_high(%.3fv)\n",
			diag_result->vcc, vcc_alarm_high);
		//util_alarm_mask_set_bit(alarm_mask, 3); // Spec does not define high threshold yet
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_HIGH_ALARM );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 3);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM];

	if (diag_result->vcc < vcc_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) < voltage_alarm_low(%.3fv)\n",
			diag_result->vcc, vcc_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 0);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_LOW_ALARM );
		}
	 } else {
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 0);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM];

	return alarm_is_on;
}

static int
meinfo_hw_65501_alarm_check_txbias(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	double attr_tx_bias_current_alarm_high;
	double attr_tx_bias_current_alarm_low;
	double tx_bias_current_alarm_high;
	double tx_bias_current_alarm_low;

	attr_tx_bias_current_alarm_high = (unsigned short)meinfo_util_me_attr_data(me, 5);
	attr_tx_bias_current_alarm_low = (unsigned short)meinfo_util_me_attr_data(me, 4);

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 1)))
		return 0;

	// get voltage
	tx_bias_current_alarm_high   = (attr_tx_bias_current_alarm_high==0xFFFF)?   diag_threshold->tx_bias_current_high : attr_tx_bias_current_alarm_high / 500.0;
	tx_bias_current_alarm_low    = (attr_tx_bias_current_alarm_low==0xFFFF)?    diag_threshold->tx_bias_current_low  : attr_tx_bias_current_alarm_low / 500.0;

	dbprintf(LOG_INFO, "bias current: hw=%.3fv, alarm_max=%.3fv, alarm_min=%.3fv\n",
		diag_result->tx_bias_current, tx_bias_current_alarm_high, tx_bias_current_alarm_low);

	if (diag_result->tx_bias_current > tx_bias_current_alarm_high) {
		dbprintf(LOG_WARNING, "alarm: tx_bias_current(%.3fv) > tx_bias_current_alarm_high(%.3fv)\n",
			diag_result->tx_bias_current, tx_bias_current_alarm_high);
		//util_alarm_mask_set_bit(alarm_mask, 4); // Spec does not define high threshold yet
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_HIGH_ALARM );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 4);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM];

	if (diag_result->tx_bias_current < tx_bias_current_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: tx_bias_current(%.3fma) < tx_bias_current_alarm_low(%.3fma)\n",
			diag_result->tx_bias_current, tx_bias_current_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 1);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_LOW_ALARM );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 1);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM];

	return alarm_is_on;
}

static int
meinfo_hw_65501_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;

	if (omci_env_g.anig_support<ENV_ANIG_SUPPORT_GET_TEST_ALARM ||
	    omci_env_g.anig_type==ENV_ANIG_TYPE_NONE) {
	    	return 0;
	}
	if (meinfo_hw_util_get_ontstate()!= 5)	// no alarm check for anig if not in O5
		return 0;

	// get counters from hw
	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;

	// check voltage level
	meinfo_hw_65501_alarm_check_voltage(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check bias level
	meinfo_hw_65501_alarm_check_txbias(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check temperature level
	meinfo_hw_65501_alarm_check_temperature(miptr, alarm_mask, me, &diag_threshold, &diag_result);

	return 0;
}

static int 
meinfo_hw_65501_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65501 = {
	.alarm_hw		= meinfo_hw_65501_alarm,
	.get_hw			= meinfo_hw_65501_get,
};
