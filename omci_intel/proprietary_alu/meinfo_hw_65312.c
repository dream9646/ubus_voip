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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65312.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo_hw.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_anig.h"
#include "util.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_event.h"

//classid 65312 ANI-G Supplemental 1

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
meinfo_hw_65312_alarm_check_temperature(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	short attr_temperature_alarm_high;
	short attr_temperature_alarm_low;
	short attr_temperature_warning_high;
	short attr_temperature_warning_low;
	double temperature_alarm_high;
	double temperature_alarm_low;
	double temperature_warning_high;
	double temperature_warning_low;

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 0) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 2) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 5) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 8)))
		return 0;

	attr_temperature_alarm_high = (short)meinfo_util_me_attr_data(me, 1);
	attr_temperature_alarm_low = (short)meinfo_util_me_attr_data(me, 4);
	attr_temperature_warning_high = (short)meinfo_util_me_attr_data(me, 7);
	attr_temperature_warning_low = (short)meinfo_util_me_attr_data(me, 10);

	// get temperature threshold
	temperature_alarm_high   = ((unsigned short) attr_temperature_alarm_high == 0x8000)?   diag_threshold->temperature_high : attr_temperature_alarm_high / 256.0;
	temperature_alarm_low    = ((unsigned short) attr_temperature_alarm_low == 0x8000)?    diag_threshold->temperature_low  : attr_temperature_alarm_low / 256.0;
	temperature_warning_high = ((unsigned short) attr_temperature_warning_high == 0x8000)? diag_threshold->temperature_high : attr_temperature_warning_high / 256.0;
	temperature_warning_low  = ((unsigned short) attr_temperature_warning_low == 0x8000)?  diag_threshold->temperature_low  : attr_temperature_warning_low / 256.0;

	dbprintf(LOG_INFO, "temperature: hw=%.3fc, alarm_max=%.3fc, alarm_min=%.3fc, warning_max=%.3fc, warning_min=%.3fc\n",
		diag_result->temperature, temperature_alarm_high, temperature_alarm_low, temperature_warning_high, temperature_warning_low);

	if (diag_result->temperature > temperature_alarm_high) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) > temperature_alarm_high(%.3fc)\n",
			diag_result->temperature, temperature_alarm_high);
		util_alarm_mask_set_bit(alarm_mask, 0);
		util_alarm_mask_clear_bit(alarm_mask, 5);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_HIGH_ALARM );
		}
	} else if (diag_result->temperature > temperature_warning_high) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) > temperature_warning_high(%.3fc)\n",
			diag_result->temperature, temperature_warning_high);
		util_alarm_mask_clear_bit(alarm_mask, 0);
		util_alarm_mask_set_bit(alarm_mask, 5);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_HIGH_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 0);
		util_alarm_mask_clear_bit(alarm_mask, 5);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_HI_WARN];

	if (diag_result->temperature < temperature_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) < temperature_alarm_low(%.3fc)\n",
			diag_result->temperature, temperature_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 2);
		util_alarm_mask_clear_bit(alarm_mask, 8);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_LOW_ALARM);
		}
	} else if (diag_result->temperature < temperature_warning_low) {
		dbprintf(LOG_WARNING, "alarm: temperature(%.3fc) < temperature_warning_low(%.3fc)\n",
			diag_result->temperature, temperature_warning_low);
		util_alarm_mask_clear_bit(alarm_mask, 2);
		util_alarm_mask_set_bit(alarm_mask, 8);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_LOW_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TEMPERATURE_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 2);
		util_alarm_mask_clear_bit(alarm_mask, 8);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_TEMPERATURE][EXTOAM_EVENT_THRESH_LO_WARN];

	return alarm_is_on;
}

static int
meinfo_hw_65312_alarm_check_voltage(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	double attr_vcc_alarm_high;
	double attr_vcc_alarm_low;
	double attr_vcc_warning_high;
	double attr_vcc_warning_low;
	double vcc_alarm_high;
	double vcc_alarm_low;
	double vcc_warning_high;
	double vcc_warning_low;

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 1) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 3) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 6) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 9)))
		return 0;

	attr_vcc_alarm_high = (unsigned short)meinfo_util_me_attr_data(me, 2);
	attr_vcc_alarm_low = (unsigned short)meinfo_util_me_attr_data(me, 5);
	attr_vcc_warning_high = (unsigned short)meinfo_util_me_attr_data(me, 8);
	attr_vcc_warning_low = (unsigned short)meinfo_util_me_attr_data(me, 11);

	// get voltage threshold
	vcc_alarm_high   = (attr_vcc_alarm_high==0xFFFF)?   diag_threshold->vcc_high : attr_vcc_alarm_high / 10000.0;
	vcc_alarm_low    = (attr_vcc_alarm_low==0xFFFF)?    diag_threshold->vcc_low  : attr_vcc_alarm_low / 10000.0;
	vcc_warning_high = (attr_vcc_warning_high==0xFFFF)? diag_threshold->vcc_high : attr_vcc_warning_high / 10000.0;
	vcc_warning_low  = (attr_vcc_warning_low==0xFFFF)?  diag_threshold->vcc_low  : attr_vcc_warning_low / 10000.0;

	dbprintf(LOG_INFO, "voltage: hw=%.3fv, alarm_max=%.3fv, alarm_min=%.3fv, warning_max=%.3fv, warning_min=%.3fv\n",
		diag_result->vcc, vcc_alarm_high, vcc_alarm_low, vcc_warning_high, vcc_warning_low);

	if (diag_result->vcc > vcc_alarm_high) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) > voltage_alarm_high(%.3fv)\n",
			diag_result->vcc, vcc_alarm_high);
		util_alarm_mask_set_bit(alarm_mask, 1);
		util_alarm_mask_clear_bit(alarm_mask, 6);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_HIGH_ALARM );
		}
	} else if (diag_result->vcc > vcc_warning_high) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) > voltage_warning_high(%.3fv)\n",
			diag_result->vcc, vcc_warning_high);
		util_alarm_mask_clear_bit(alarm_mask, 1);
		util_alarm_mask_set_bit(alarm_mask, 6);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_HIGH_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 1);
		util_alarm_mask_clear_bit(alarm_mask, 6);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_HI_WARN];

	if (diag_result->vcc < vcc_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) < voltage_alarm_low(%.3fv)\n",
			diag_result->vcc, vcc_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 3);
		util_alarm_mask_clear_bit(alarm_mask, 9);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_LOW_ALARM );
		}
	} else if (diag_result->vcc < vcc_warning_low) {
		dbprintf(LOG_WARNING, "alarm: voltage(%.3fv) < voltage_warning_low(%.3fv)\n",
			diag_result->vcc, vcc_warning_low);
		util_alarm_mask_clear_bit(alarm_mask, 3);
		util_alarm_mask_set_bit(alarm_mask, 9);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_LOW_WARN );
		}
	 } else {
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_VOLTAGE_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 3);
		util_alarm_mask_clear_bit(alarm_mask, 9);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_VOLTAGE][EXTOAM_EVENT_THRESH_LO_WARN];

	return alarm_is_on;
}

static int
meinfo_hw_65312_alarm_check_txbias(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	double attr_tx_bias_current_alarm_high;
	double attr_tx_bias_current_alarm_low;
	double attr_tx_bias_current_warning_high;
	double attr_tx_bias_current_warning_low;
	double tx_bias_current_alarm_high;
	double tx_bias_current_alarm_low;
	double tx_bias_current_warning_high;
	double tx_bias_current_warning_low;

	attr_tx_bias_current_alarm_high = (unsigned short)meinfo_util_me_attr_data(me, 3);	// no alarm mask defined in this me (defined in me263)
	attr_tx_bias_current_alarm_low = (unsigned short)meinfo_util_me_attr_data(me, 6);
	attr_tx_bias_current_warning_high = (unsigned short)meinfo_util_me_attr_data(me, 9);
	attr_tx_bias_current_warning_low = (unsigned short)meinfo_util_me_attr_data(me, 12);

	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 4) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 7) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 10)))
		return 0;

	// get voltage
	tx_bias_current_alarm_high   = (attr_tx_bias_current_alarm_high==0xFFFF)?   diag_threshold->tx_bias_current_high : attr_tx_bias_current_alarm_high / 500.0;
	tx_bias_current_alarm_low    = (attr_tx_bias_current_alarm_low==0xFFFF)?    diag_threshold->tx_bias_current_low  : attr_tx_bias_current_alarm_low / 500.0;
	tx_bias_current_warning_high = (attr_tx_bias_current_warning_high==0xFFFF)? diag_threshold->tx_bias_current_high : attr_tx_bias_current_warning_high / 500.0;
	tx_bias_current_warning_low  = (attr_tx_bias_current_warning_low==0xFFFF)?  diag_threshold->tx_bias_current_low  : attr_tx_bias_current_warning_low / 500.0;

	dbprintf(LOG_INFO, "bias current: hw=%.3fv, alarm_max=%.3fv, alarm_min=%.3fv, warning_max=%.3fv, warning_min=%.3fv\n",
		diag_result->tx_bias_current, tx_bias_current_alarm_high, tx_bias_current_alarm_low, tx_bias_current_warning_high, tx_bias_current_warning_low);

	if (diag_result->tx_bias_current > tx_bias_current_alarm_high) {
		// we clear warning tca as alarm tca would be set in me263#6
		util_alarm_mask_clear_bit(alarm_mask, 7);
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_HIGH_ALARM );
		}
	} else if (diag_result->tx_bias_current > tx_bias_current_warning_high) {
		dbprintf(LOG_WARNING, "alarm: tx_bias_current(%.3fma) > tx_bias_current_warning_high(%.3fma)\n",
			diag_result->tx_bias_current, tx_bias_current_warning_high);
		util_alarm_mask_set_bit(alarm_mask, 7);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_HIGH_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 7);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_HI_WARN];

	if (diag_result->tx_bias_current < tx_bias_current_alarm_low) {
		dbprintf(LOG_WARNING, "alarm: tx_bias_current(%.3fma) < tx_bias_current_alarm_low(%.3fma)\n",
			diag_result->tx_bias_current, tx_bias_current_alarm_low);
		util_alarm_mask_set_bit(alarm_mask, 4);
		util_alarm_mask_clear_bit(alarm_mask, 10);
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_LOW_ALARM );
		}
		alarm_is_on=1;
	} else if (diag_result->tx_bias_current < tx_bias_current_warning_low) {
		dbprintf(LOG_WARNING, "alarm: tx_bias_current(%.3fma) < tx_bias_current_warning_low(%.3fma)\n",
			diag_result->tx_bias_current, tx_bias_current_warning_low);
		util_alarm_mask_clear_bit(alarm_mask, 4);
		util_alarm_mask_set_bit(alarm_mask, 10);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_LOW_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_BIASCURRENT_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 4);
		util_alarm_mask_clear_bit(alarm_mask, 10);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_BIASCURRENT][EXTOAM_EVENT_THRESH_LO_WARN];

	return alarm_is_on;
}

static int
meinfo_hw_65312_alarm_check_rxpower(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	unsigned char attr_rx_max, attr_rx_min;
	float rx_warning_max, rx_warning_min, rx_alarm_max, rx_alarm_min, rx_val;

	struct me_t *me_263 = meinfo_me_get_by_instance_num(263, 0);
	if(me_263) {
		attr_rx_min=(unsigned char)meinfo_util_me_attr_data(me_263, 11);
		rx_alarm_min = (attr_rx_min==0xff)? uw_to_dbm(diag_threshold->rx_power_low) : attr_rx_min*(-0.5);
		attr_rx_max=(unsigned char)meinfo_util_me_attr_data(me_263, 12);
		rx_alarm_max = (attr_rx_max==0xff)? uw_to_dbm(diag_threshold->rx_power_high) : attr_rx_max*(-0.5);
	}
	
	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 11) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 12)))
		return 0;

	attr_rx_max = (unsigned short)meinfo_util_me_attr_data(me, 13);
	attr_rx_min = (unsigned short)meinfo_util_me_attr_data(me, 14);

	// get rx optical level
	rx_warning_max = (attr_rx_max==0xff)? uw_to_dbm(diag_threshold->rx_power_high) : attr_rx_max*(-0.5);
	rx_warning_min = (attr_rx_min==0xff)? uw_to_dbm(diag_threshold->rx_power_low)  : attr_rx_min*(-0.5);
	rx_val = uw_to_dbm(diag_result->rx_power);

	dbprintf(LOG_INFO, "rx: hw=%.3fdbmw, min=%u(%.3fdbmw), max=%u(%.3fdbmw)\n",
		rx_val, attr_rx_min, rx_warning_min, attr_rx_max, rx_warning_max);

	if (rx_val > rx_alarm_max) {
		// we clear warning tca as alarm tca would be set in me263
		util_alarm_mask_clear_bit(alarm_mask, 11);
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_HIGH_ALARM );
		}
	} else if (rx_val > rx_warning_max) {
		dbprintf(LOG_WARNING, "alarm: rx(%.3fdbmw) > rx_max(%.3fdbmw)\n", rx_val, rx_warning_max);
		util_alarm_mask_set_bit(alarm_mask, 11);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_HIGH_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 11);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_HI_WARN];

	if (rx_val < rx_alarm_min) {
		// we clear warning tca as alarm tca would be set in me263
		util_alarm_mask_clear_bit(alarm_mask, 12);
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_LOW_ALARM );
		}
	} else if (rx_val < rx_warning_min) {
		dbprintf(LOG_WARNING, "alarm: rx(%.3fdbmw) < rx_min(%.3fdbmw)\n", rx_val, rx_warning_min);
		util_alarm_mask_set_bit(alarm_mask, 12);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_LOW_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_RXPOWER_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 12);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_RXPOWER][EXTOAM_EVENT_THRESH_LO_WARN];

	return alarm_is_on;
}

static int
meinfo_hw_65312_alarm_check_txpower(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me,
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	unsigned char attr_tx_max, attr_tx_min;
	float tx_warning_max, tx_warning_min, tx_alarm_max, tx_alarm_min, tx_val;

	struct me_t *me_263 = meinfo_me_get_by_instance_num(263, 0);
	if(me_263) {
		attr_tx_min=(unsigned char)meinfo_util_me_attr_data(me_263, 15);
		tx_alarm_min = (attr_tx_min==0x81)? uw_to_dbm(diag_threshold->tx_power_low) : attr_tx_min*0.5;
		attr_tx_max=(unsigned char)meinfo_util_me_attr_data(me_263, 16);
		tx_alarm_max = (attr_tx_max==0x81)? uw_to_dbm(diag_threshold->tx_power_high) : attr_tx_max*0.5;
	}
	
	if (!(util_alarm_mask_get_bit(miptr->alarm_mask, 13) ||
	       util_alarm_mask_get_bit(miptr->alarm_mask, 14)))
		return 0;

	attr_tx_max = (unsigned short)meinfo_util_me_attr_data(me, 15);
	attr_tx_min = (unsigned short)meinfo_util_me_attr_data(me, 16);

	// get tx optical level

	tx_warning_max = (attr_tx_max==0x81)? uw_to_dbm(diag_threshold->tx_power_high) : attr_tx_max*0.5;
	tx_warning_min = (attr_tx_min==0x81)? uw_to_dbm(diag_threshold->tx_power_low)  : attr_tx_min*0.5;
	tx_val=uw_to_dbm(diag_result->tx_power);

	dbprintf(LOG_INFO, "tx: hw=%.3fdbmw, min=%u(%.3fdbmw), max=%u(%.3fdbmw)\n",
		tx_val, attr_tx_min, tx_warning_min, attr_tx_max, tx_warning_max);

	if (tx_val > tx_alarm_max) {
		// we clear warning tca as alarm tca would be set in me263
		util_alarm_mask_clear_bit(alarm_mask, 13);
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_HIGH_ALARM );
		}
	} else if (tx_val > tx_warning_max) {
		dbprintf(LOG_WARNING, "alarm: tx(%.3fdbmw) > tx_max(%.3fdbmw)\n", tx_val, tx_warning_max);
		util_alarm_mask_set_bit(alarm_mask, 13);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_HIGH_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_HIGH_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 13);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_HI_WARN];

	if (tx_val < tx_alarm_min) {
		// we clear warning tca as alarm tca would be set in me263
		util_alarm_mask_clear_bit(alarm_mask, 14);
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 1;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_LOW_ALARM );
		}
	} else if (tx_val < tx_warning_min) {
		dbprintf(LOG_WARNING, "alarm: tx(%.3fdbmw) < tx_min(%.3fdbmw)\n", tx_val, tx_warning_min);
		util_alarm_mask_set_bit(alarm_mask, 14);
		alarm_is_on=1;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 1;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_LOW_WARN );
		}
	} else {
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 0;
		extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 0;
		if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK && cpe_link_status_g.trap_state == 1 ) {
			if(extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] ||
			extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] !=
			extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN])
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_TXPOWER_LOW_CLEAR );
		}
		util_alarm_mask_clear_bit(alarm_mask, 14);
	}
	extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM] = 
	extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_ALARM];
	extoam_event_previous[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN] = 
	extoam_event[EXTOAM_EVENT_TYPE_TXPOWER][EXTOAM_EVENT_THRESH_LO_WARN];

	return alarm_is_on;
}

static int
meinfo_hw_65312_alarm(struct me_t *me, unsigned char alarm_mask[28])
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

	// check temperature level
	meinfo_hw_65312_alarm_check_temperature(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check voltage level
	meinfo_hw_65312_alarm_check_voltage(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check bias level
	meinfo_hw_65312_alarm_check_txbias(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check rx power level
	meinfo_hw_65312_alarm_check_rxpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);
	// check tx power level
	meinfo_hw_65312_alarm_check_txpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);

	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65312 = {
	.alarm_hw		= meinfo_hw_65312_alarm,
};
