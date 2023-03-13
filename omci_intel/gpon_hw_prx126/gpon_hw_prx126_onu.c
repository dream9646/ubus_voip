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
 * Module  : gpon_hw_prx126
 * File    : gpon_hw_prx126_onu.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "util_run.h"
#include "list.h"
#include "meinfo.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"

#include "intel_px126_tm.h"
#include "intel_px126_pon.h"

int
gpon_hw_prx126_onu_mgmt_mode_get(int *mgmt_mode)
{
	return 0;
}

int
gpon_hw_prx126_onu_mgmt_mode_set(int mgmt_mode)
{
	return 0;
}

int
gpon_hw_prx126_onu_serial_number_get(char *vendor_id, unsigned int *serial_number)
{
	return intel_prx126_pon_serial_number_get(vendor_id,serial_number);
}

int
gpon_hw_prx126_onu_serial_number_set(char *vendor_id, unsigned int *serial_number)
{

	return 0;
}

int
gpon_hw_prx126_onu_password_get(char *password)
{

	return intel_prx126_pon_password_get(password);
}

int
gpon_hw_prx126_onu_password_set(char *password)
{
	return 0;
}

int
gpon_hw_prx126_onu_activate(unsigned int state)
{
	return intel_prx126_pon_active();
}

int
gpon_hw_prx126_onu_deactivate(void)
{
	return intel_prx126_pon_deactive();
}

int
gpon_hw_prx126_onu_state_get(unsigned int *state)
{
	return intel_px126_pon_get_state(state);
}

int
gpon_hw_prx126_onu_id_get(unsigned short *id)
{
	int ret = 0;
	/*intel omci alloc index is always*/
	ret = intel_px126_tm_get_allocid_by_tmidex(CHIP_GPON_OMCI_TCONT_ID,id);
	
	return ret;
}

int
gpon_hw_prx126_onu_alarm_event_get(struct gpon_onu_alarm_event_t *alarm_event)
{
	unsigned int status;

	memset(alarm_event, 0, sizeof(struct gpon_onu_alarm_event_t));

	if (!intel_prx126_gpon_alarm_status_get(PON_ALARM_LOS, &status))
		alarm_event->los = (status)?1:0;

	if (!intel_prx126_gpon_alarm_status_get(PON_ALARM_LOF, &status))
		alarm_event->lof = (status)?1:0;

	if (!intel_prx126_gpon_alarm_status_get(PON_ALARM_LOL, &status))
		alarm_event->lol = (status)?1:0;

	return 0;
}

int
gpon_hw_prx126_onu_gem_blocksize_get(int *gem_blocksize)
{
	// FIXME, 2510 doesn't allow to get gem block size
	*gem_blocksize = 48;
	return 0;
}

int
gpon_hw_prx126_onu_gem_blocksize_set(int gem_blocksize)
{
	// FIXME, 2510 doesn't allow to set gem block size
	return 0;
}

// map indicator to bit error per second
static unsigned long long
ber_map(int x)
{
	switch(x) {
		case 0: return 2488320000LL;
		case 1: return 248832000LL;
		case 2: return 24883200LL;
		case 3: return 2488320LL;
		case 4: return 248832LL;
		case 5: return 24883LL;
		case 6: return 2488LL;
		case 7: return 248LL;
		case 8: return 24LL;
		case 9: return 2LL;
		case 10: return 1LL;
	}
	return 24883LL;	// as if x==5
}

// 1: over, 0: not over, -1: under & clear
int
gpon_hw_prx126_onu_sdsf_get(int sd_threshold_ind, int sf_threshold_ind, int *sd_state, int *sf_state)
{
	unsigned long biterr_per_sec;
	unsigned long long sd_threshold, sf_threshold;

	{	// calc biterr_per_sec
		static unsigned long rx_bip_err_bit_prev = 0;
		static struct timeval timeval_prev = {0,0};
		struct gpon_counter_global_t cnt_global;
		struct timeval timeval_now;
		unsigned long rx_bip_err_bit_diff;
		unsigned long us_diff;
		
		util_get_uptime(&timeval_now);
		us_diff = util_timeval_diff_usec(&timeval_now, &timeval_prev);

		// since pm task reads chip hw periodically, (hw counter is clear-after-read)
		// others should read counter through pm instead of chip hw to ensure hw counter being fully collected into pm
		
		gpon_hw_g.pm_refresh(0);	//0:refresh 1: reset
		gpon_hw_g.pm_global_get(&cnt_global);
		if (cnt_global.rx_bip_err_bit >= rx_bip_err_bit_prev) {
			rx_bip_err_bit_diff = cnt_global.rx_bip_err_bit - rx_bip_err_bit_prev;		
		} else {
			// rx_bip_err_bit is 64bit in pm, so it wont overflow, 
			// if rx_bip_err_bit < prev, it must be soneone has issued 'gpon cnt reset'
			rx_bip_err_bit_diff = cnt_global.rx_bip_err_bit;
		}
		biterr_per_sec = rx_bip_err_bit_diff*1000/(us_diff/1000);		

// for test, using env omci_history_size as rx_bip_err_bit_diff
//		biterr_per_sec = omci_env_g.omci_history_size*1000/(us_diff/1000);		

		rx_bip_err_bit_prev = cnt_global.rx_bip_err_bit;
		timeval_prev = timeval_now;
	}

	sd_threshold = ber_map(sd_threshold_ind);
	sf_threshold = ber_map(sf_threshold_ind);

	dbprintf(LOG_NOTICE, "biterr_per_sec=%d, SD:ind=%d,threshold=%llu, SF:ind=%d,threshold=%llu\n", 
		biterr_per_sec, sd_threshold_ind, sd_threshold, sf_threshold_ind, sf_threshold);
	
	if (biterr_per_sec > sd_threshold)
		*sd_state = GPON_ONU_SDSF_OVER;
	else if (biterr_per_sec <= sd_threshold/10)	// as sd_threshold/10 might be 0, the = is required
		*sd_state = GPON_ONU_SDSF_UNDER_CLEAR;
	else
		*sd_state = GPON_ONU_SDSF_UNDER;

	if (biterr_per_sec > sf_threshold)
		*sf_state = GPON_ONU_SDSF_OVER;
	else if (biterr_per_sec <= sf_threshold/10)
		*sf_state = GPON_ONU_SDSF_UNDER_CLEAR;
	else
		*sf_state = GPON_ONU_SDSF_UNDER;

	return 0;
}

int
gpon_hw_prx126_onu_superframe_seq_get(unsigned long long *superframe_sequence)
{
	unsigned int mf_cnt;
	
	if (intel_prx126_gpon_tod_sync_mf_get(&mf_cnt))
		return -1;

	*superframe_sequence = mf_cnt;

	return 0;
}

