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
 * File    : meinfo_hw_263.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <math.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_anig.h"
#include "meinfo_hw_anig_fh.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_event.h"

extern struct extoam_link_status_t cpe_link_status_g;
static unsigned char extoam_event_sf, extoam_event_sd;
static unsigned char extoam_event_sf_previous, extoam_event_sd_previous;

// 9.2.1 ANI-G

static int 
meinfo_hw_263_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	int value;

	if (util_attr_mask_get_bit(attr_mask, 10) || 
	    util_attr_mask_get_bit(attr_mask, 14)) {  	
	    	if (omci_env_g.anig_support>=ENV_ANIG_SUPPORT_GET && 
	    	    omci_env_g.anig_type!=ENV_ANIG_TYPE_NONE) {
		    	struct anig_threshold_t diag_threshold;
		    	struct anig_result_t diag_result;
		    	struct anig_raw_t diag_raw;

			// get counters from hw
	    		if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
	    			return-1;
#if 0
			if ( omci_env_g.debug_level >= LOG_NOTICE) {
				meinfo_hw_anig_print_threshold_result(util_dbprintf_get_console_fd(), 
					omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw);
			}
#endif		
			if (util_attr_mask_get_bit(attr_mask, 10)) {	// optical signal level (rx)
				// ensure diag_result.rx_power is not 0 so uw_to_dbm() returns meaninful result
				if (diag_result.rx_power<0.0001)
					diag_result.rx_power=0.0001;	
				value=uw_to_dbm(diag_result.rx_power)/0.002;	// unit 0.002dbm
				// ensure value can be properly converted into short (as defined by omci)
				if (value < SHRT_MIN) value = SHRT_MIN;
				if (value > SHRT_MAX) value = SHRT_MAX;
				// update mib
				attr.ptr=NULL;
				attr.data=(short)value;
				meinfo_me_attr_set(me, 10, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 14)) {	// transmit optical level (tx)
				// ensure diag_result.rx_power is not 0 so uw_to_dbm() returns meaninful result
				if (diag_result.tx_power<0.0001)
					diag_result.tx_power=0.0001;	
				value=uw_to_dbm(diag_result.tx_power)/0.002;	// unit 0.002dbm
				// ensure value can be properly converted into short (as defined by omci)
				if (value < SHRT_MIN) value = SHRT_MIN;
				if (value > SHRT_MAX) value = SHRT_MAX;
				// update mib
				attr.ptr=NULL;
				attr.data=(short)value;	// unit 0.002dbm
				meinfo_me_attr_set(me, 14, &attr, 0);
			}
		}
	}
	
	return 0;
}

static int
meinfo_hw_263_alarm_check_sdsf(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me)
{
	int alarm_is_on=0;
	int sf_ind, sd_ind;
	int sd_state=GPON_ONU_SDSF_UNDER, sf_state=GPON_ONU_SDSF_UNDER;
	int ret;

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 2) &&
	    !util_alarm_mask_get_bit(miptr->alarm_mask, 3))
		return 0;
	
	sf_ind=(char)meinfo_util_me_attr_data(me, 6);
	sd_ind=(char)meinfo_util_me_attr_data(me, 7);
	
	if (gpon_hw_g.onu_sdsf_get)
		ret = gpon_hw_g.onu_sdsf_get(sd_ind, sf_ind, &sd_state, &sf_state);
	if (ret == 0) {		
		dbprintf(LOG_INFO, "sf=%d, sd=%d\n", sf_state, sd_state);
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 2)) {
			if (sf_state==GPON_ONU_SDSF_OVER) {
				dbprintf(LOG_ERR, "alarm: signal fail?\n");
				util_alarm_mask_set_bit(alarm_mask, 2);
				alarm_is_on=1;
				extoam_event_sf = 1;
				if(extoam_event_sf != extoam_event_sf_previous)
					extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_SF_ALARM );
				extoam_event_sf_previous = extoam_event_sf;
			} else if (sf_state==GPON_ONU_SDSF_UNDER_CLEAR) {
				util_alarm_mask_clear_bit(alarm_mask, 2);
				extoam_event_sf = 0;
				if(extoam_event_sf != extoam_event_sf_previous)
					extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_SF_CLEAR );
				extoam_event_sf_previous = extoam_event_sf;
			}
		}
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 3)) {
			if (sd_state==GPON_ONU_SDSF_OVER) {
				dbprintf(LOG_ERR, "alarm: signal degrade?\n");
				util_alarm_mask_set_bit(alarm_mask, 3);
				alarm_is_on=1;
				extoam_event_sd = 1;
				if(extoam_event_sd != extoam_event_sd_previous)
					extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_SD_ALARM );
				extoam_event_sd_previous = extoam_event_sd;
			} else if (sd_state==GPON_ONU_SDSF_UNDER_CLEAR) {
				util_alarm_mask_clear_bit(alarm_mask, 3);
				extoam_event_sd = 0;
				if(extoam_event_sd != extoam_event_sd_previous)
					extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_SD_CLEAR );
				extoam_event_sd_previous = extoam_event_sd;
			}
		}
	} else {
		dbprintf(LOG_ERR, "get SD SF status err(%d)\n", ret);
	}
	return alarm_is_on;
}

static int
meinfo_hw_263_alarm_check_rxpower(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me, 
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	unsigned char rx_min, rx_max;
	float rx_val, rx_min_val, rx_max_val;

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 0) &&
	    !util_alarm_mask_get_bit(miptr->alarm_mask, 1))
		return 0;

	// we sync the unit to dbm for all counters
	// alarm will be triggered only if (value>max || value<min) && max>=min
	// some hw may has wrong max/min, we don't trigger alarm for these device
	
	// get rx optical level
	rx_val=uw_to_dbm(diag_result->rx_power);

	rx_min=(unsigned char)meinfo_util_me_attr_data(me, 11);
	if (rx_min==0xff) {	// use vendor default
		rx_min_val=uw_to_dbm(diag_threshold->rx_power_low);	
	} else {
		rx_min_val=rx_min*(-0.5);
	}
	rx_max=(unsigned char)meinfo_util_me_attr_data(me, 12);	
	if (rx_max==0xff) {	// use vendor default
		rx_max_val=uw_to_dbm(diag_threshold->rx_power_high);
	} else {
		rx_max_val=rx_max*(-0.5);
	}
	dbprintf(LOG_INFO, "rx: hw=%.3fdbmw, min=%u(%.3fdbmw), max=%u(%.3fdbmw)\n", 
		rx_val, rx_min, rx_min_val, rx_max, rx_max_val);

	if ((rx_min!=0xff && rx_max!=0xff) && (rx_max_val < rx_min_val)) {
		dbprintf(LOG_NOTICE, "alarm skipped! rx_max(%.3fdbmw) < rx_min(%.3fdbmw)?\n", rx_max_val, rx_min_val);
		util_alarm_mask_clear_bit(alarm_mask, 0);
		util_alarm_mask_clear_bit(alarm_mask, 1);
	} else {
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
			if (rx_val < rx_min_val) {
				dbprintf(LOG_NOTICE, "alarm: rx(%.3fdbmw) < rx_min(%.3fdbmw)\n", rx_val, rx_min_val);
				util_alarm_mask_set_bit(alarm_mask, 0);
				alarm_is_on=1;
			} else {
				util_alarm_mask_clear_bit(alarm_mask, 0);
			}	
		}	
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 1)) {
			if (rx_val > rx_max_val) {
				dbprintf(LOG_NOTICE, "alarm: rx(%.3fdbmw) > rx_max(%.3fdbmw)\n", rx_val, rx_max_val);
				util_alarm_mask_set_bit(alarm_mask, 1);
				alarm_is_on=1;
			} else {
				util_alarm_mask_clear_bit(alarm_mask, 1);
			}
		}
	}
	return alarm_is_on;
}

static int
meinfo_hw_263_alarm_check_txpower(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me, 
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	unsigned char tx_min, tx_max;
	float tx_val, tx_min_val, tx_max_val;

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 4) &&
	    !util_alarm_mask_get_bit(miptr->alarm_mask, 5))
	    	return 0;
	    	
	// we sync the unit to dbm for all counters
	// alarm will be triggered only if (value>max || value<min) && max>=min
	// some hw may has wrong max/min, we don't trigger alarm for these device
	
	// get tx optical level
	tx_val=uw_to_dbm(diag_result->tx_power);

	tx_min=(char)meinfo_util_me_attr_data(me, 15);
	if ((unsigned char)tx_min==0x81) {	// use vendor default
		tx_min_val=uw_to_dbm(diag_threshold->tx_power_low);	
	} else {
		tx_min_val=tx_min*0.5;
	}
	tx_max=(char)meinfo_util_me_attr_data(me, 16);	
	if ((unsigned char)tx_max==0x81) {	// use vendor default
		tx_max_val=uw_to_dbm(diag_threshold->tx_power_high);
	} else {
		tx_max_val=tx_max*0.5;
	}
	dbprintf(LOG_INFO, "tx: %.3fdbmw, min=%d(%.3fdbmw), max=%d(%.3fdbmw)\n", 
		tx_val, tx_min, tx_min_val, tx_max, tx_max_val);

	if ((tx_min!=0x81 && tx_max!=0x81) && 
		(tx_max_val < tx_min_val)) {
		dbprintf(LOG_NOTICE, "alarm skipped! tx_max(%.3fdbmw) < tx_min(%.3fdbmw)?\n", tx_max_val, tx_min_val);
		util_alarm_mask_clear_bit(alarm_mask, 4);
		util_alarm_mask_clear_bit(alarm_mask, 5);
	} else {
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 4)) {
			if (tx_val < tx_min_val) {
				dbprintf(LOG_NOTICE, "alarm: tx(%.3fdbmw) < tx_min(%.3fdbmw)\n", tx_val, tx_min_val);
				util_alarm_mask_set_bit(alarm_mask, 4);
				alarm_is_on=1;
			} else {
				util_alarm_mask_clear_bit(alarm_mask, 4);
			}	
		}	
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 5)) {
			if (tx_val > tx_max_val) {
				dbprintf(LOG_NOTICE, "alarm: tx(%.3fdbmw) > tx_max(%.3fdbmw)\n", tx_val, tx_max_val);
				util_alarm_mask_set_bit(alarm_mask, 5);
				alarm_is_on=1;
			} else {
				util_alarm_mask_clear_bit(alarm_mask, 5);
			}
		}						
	}
	return alarm_is_on;
}

static int
meinfo_hw_263_alarm_check_txbias(struct meinfo_t *miptr, unsigned char alarm_mask[], 
				struct anig_threshold_t *diag_threshold, struct anig_result_t *diag_result)
{
	int alarm_is_on=0;
	double tx_bias_current_high = diag_threshold->tx_bias_current_high;

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 6) || 
	    diag_result->tx_bias_current==0xffff)
		return 0;
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) {
		struct me_t *me_65312 = meinfo_me_get_by_instance_num(65312, 0);
		if(me_65312) {
			tx_bias_current_high = (unsigned short)meinfo_util_me_attr_data(me_65312, 3);
			if(tx_bias_current_high == 0xFFFF) 
				tx_bias_current_high = diag_threshold->tx_bias_current_high;
			else
				tx_bias_current_high = tx_bias_current_high / 500.0;
		}
	}
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
		struct me_t *me_65501 = meinfo_me_get_by_instance_num(65501, 0);
		if(me_65501) {
			tx_bias_current_high = (unsigned short)meinfo_util_me_attr_data(me_65501, 5);
			if(tx_bias_current_high == 0xFFFF) 
				tx_bias_current_high = diag_threshold->tx_bias_current_high;
			else
				tx_bias_current_high = tx_bias_current_high / 500.0;
		}
	}
	
	dbprintf(LOG_INFO, "tx_bias_current: hw=%.3fma, min=%.3fma, max=%.3fma\n", 
		diag_result->tx_bias_current, diag_threshold->tx_bias_current_low, tx_bias_current_high);

	if (diag_result->tx_bias_current < diag_threshold->tx_bias_current_low) {
		dbprintf(LOG_NOTICE, "alarm: tx_bias_current(%.3fma) < tx_bias_current_low(%.3fma)\n", 
			diag_result->tx_bias_current, diag_threshold->tx_bias_current_low);
		alarm_is_on=1;
	}
	if (diag_result->tx_bias_current > tx_bias_current_high) {
		dbprintf(LOG_NOTICE, "alarm: tx_bias_current(%.3fma) > tx_bias_current_high(%.3fma)\n", 
			diag_result->tx_bias_current, tx_bias_current_high);
		alarm_is_on=1;
	}
	if (alarm_is_on)
		util_alarm_mask_set_bit(alarm_mask, 6);
	else
		util_alarm_mask_clear_bit(alarm_mask, 6);
	
	return alarm_is_on;
}

static int 
meinfo_hw_263_alarm(struct me_t *me, unsigned char alarm_mask[28])
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
   		return-1;	    	

	// check SD(signal degrade), SF(signal fail)
	meinfo_hw_263_alarm_check_sdsf(miptr, alarm_mask, me);

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0) {
		if (meinfo_hw_anig_fh_alarm_rxpower_enabled())
			meinfo_hw_anig_fh_alarm_check_rxpower(miptr, alarm_mask, &diag_result);
		else
			meinfo_hw_263_alarm_check_rxpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);
			
		if (meinfo_hw_anig_fh_alarm_txpower_enabled())
			meinfo_hw_anig_fh_alarm_check_txpower(miptr, alarm_mask, &diag_result);
		else
			meinfo_hw_263_alarm_check_txpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);
			
		if (meinfo_hw_anig_fh_alarm_txbias_enabled())
			meinfo_hw_anig_fh_alarm_check_txbias(miptr, alarm_mask, &diag_result);
		else
			meinfo_hw_263_alarm_check_txbias(miptr, alarm_mask, &diag_threshold, &diag_result);
		meinfo_hw_anig_fh_alarm_check_voltage(miptr, alarm_mask, &diag_result);
		meinfo_hw_anig_fh_alarm_check_temperature(miptr, alarm_mask, &diag_result);
			
	} else {
		// check rx optical level
		meinfo_hw_263_alarm_check_rxpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);
		// check tx optical level
		meinfo_hw_263_alarm_check_txpower(miptr, alarm_mask, me, &diag_threshold, &diag_result);
		// check tx bias current
		meinfo_hw_263_alarm_check_txbias(miptr, alarm_mask, &diag_threshold, &diag_result);
	}
	return 0;
}

/*
struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned char padding[31];
} __attribute__ ((packed));
*/
static int 
meinfo_hw_263_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *) req;

	if (omci_env_g.anig_support>=ENV_ANIG_SUPPORT_GET_TEST && 
	    omci_env_g.anig_type!=ENV_ANIG_TYPE_NONE) {
		if (test_req->select_test==7)					// self test
			return 1;
		if (test_req->select_test>=8 && test_req->select_test<=15)	// vendor test
			return 0;
	}	
	return 0;
}

/*
struct omcimsg_test_result_ani_linetest_t {
	unsigned char type1;
	unsigned short power_feed_voltage;
	unsigned char type3;
	unsigned short received_optical_power;
	unsigned char type5;
	unsigned short transmitted_optical_power;
	unsigned char type9;
	unsigned short laser_bias_current;
	unsigned char type12;
	unsigned short temperature_degrees;
	unsigned char padding[17];
} __attribute__ ((packed));
*/
static int 
meinfo_hw_263_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *)req;
	struct omcimsg_test_result_ani_linetest_t *test_result =(struct omcimsg_test_result_ani_linetest_t *)result;

	if (test_req->select_test==7) {	// always return pass for selft test
		struct anig_threshold_t diag_threshold;
		struct anig_result_t diag_result;
		struct anig_raw_t diag_raw;

	    	if (omci_env_g.anig_support>=ENV_ANIG_SUPPORT_GET_TEST &&
	    	    omci_env_g.anig_type!=ENV_ANIG_TYPE_NONE) {
			if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0) {
				// fill all fields with 0, which means all test type are not supported
				memset(test_result, 0, sizeof(struct omcimsg_test_result_ani_linetest_t));
				return -1;
			}
	
			if ( omci_env_g.debug_level >= LOG_NOTICE) {
				meinfo_hw_anig_print_threshold_result(util_dbprintf_get_console_fd(), 
					omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw);
			}
			// unit 20mv
			test_result->type1=1;
			test_result->power_feed_voltage=htons(diag_result.vcc*1000/20); // v->20mv		
			// unit 0.002dbuw
			test_result->type3=3;
			test_result->received_optical_power=htons((short)(uw_to_dbuw(diag_result.rx_power)/0.002)); //uw->0.002dbuw
			// unit 0.002dbuw
			test_result->type5=5;
			test_result->transmitted_optical_power=htons((short)(uw_to_dbuw(diag_result.tx_power)/0.002));// uw ->0.002dbuw
			// unit 2ua
			test_result->type9=9;
			test_result->laser_bias_current=htons(diag_result.tx_bias_current*1000/2); // mA->2uA
			// unit 1/256 c degree
			test_result->type12=12;
			test_result->temperature_degrees=htons(diag_result.temperature*256);	// c->1/256c
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_263 = {
	.get_hw			= meinfo_hw_263_get,
	.alarm_hw		= meinfo_hw_263_alarm,
	.test_is_supported_hw	= meinfo_hw_263_test_is_supported,
	.test_hw		= meinfo_hw_263_test,
};

