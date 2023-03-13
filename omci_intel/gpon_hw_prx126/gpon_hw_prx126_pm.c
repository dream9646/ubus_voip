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
 * File    : gpon_hw_prx126_pm.c
 *
 ******************************************************************/

#include "util.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"
#include "intel_px126_pm.h"

struct gpon_mib_t {
	struct gpon_counter_global_t global;
	struct gpon_counter_tcont_t tcont[GPON_TCONT_ID_TOTAL];
	struct gpon_counter_dsflow_t dsflow[GPON_FLOW_ID_TOTAL];
	struct gpon_counter_usflow_t usflow[GPON_FLOW_ID_TOTAL];
};

static struct gpon_mib_t pm_mib_current;
static struct gpon_mib_t pm_mib_hwprev;  // 220923 LEO preserve for counter reset feature

/// counter api, get counter from underlying hw////////////////////////////////////////////////////
int
gpon_hw_prx126_counter_global_get(struct gpon_counter_global_t *counter_global)
{
	struct intel_px126_pon_ploam_ds_counters ploam_ds_counter;
	struct intel_px126_pon_ploam_us_counters ploam_us_counter;
	struct intel_px126_pon_gtc_counters gtc_counter;
	struct intel_px126_pon_xgtc_counters xgtc_counter;
	struct intel_px126_pon_fec_counters fec_counter;
	struct intel_px126_pon_gem_port_counters gem_counter;

	if (counter_global == NULL)
		return -1;

	intel_px126_pm_ploam_ds_counters_get(&ploam_ds_counter);
	intel_px126_pm_ploam_us_counters_get(&ploam_us_counter);
	intel_px126_pm_gtc_counters_get(&gtc_counter);
	intel_px126_pm_xgtc_counters_get(&xgtc_counter);
	intel_px126_pm_fec_counters_get(&fec_counter);
	intel_px126_pm_gem_all_counters_get(&gem_counter);

	counter_global->rx_sn_req = 0;
	counter_global->rx_ranging_req = ploam_ds_counter.ranging_time;

	counter_global->rx_bip_err_bit = gtc_counter.bip_errors;
	counter_global->rx_bip_err_block = 0;
	counter_global->rx_fec_correct_bit = gtc_counter.bytes_corr*8;
	counter_global->rx_fec_correct_byte = gtc_counter.bytes_corr;
	counter_global->rx_fec_correct_cw = gtc_counter.fec_codewords_corr;
	counter_global->rx_fec_uncor_cw = gtc_counter.fec_codewords_uncorr;
	counter_global->rx_lom = gtc_counter.lods_events;
	counter_global->rx_plen_err = 0;

	counter_global->rx_ploam_cnt = ploam_ds_counter.all;
	counter_global->rx_ploam_err = xgtc_counter.ploam_mic_err;
	counter_global->rx_ploam_correctted = 0;
	counter_global->rx_ploam_proc = 0;
	counter_global->rx_ploam_overflow = 0;
	counter_global->rx_ploam_unknown = ploam_ds_counter.unknown;

	counter_global->rx_bwmap_cnt = 0;
	counter_global->rx_bwmap_crc_err = gtc_counter.bwmap_hec_errors_corr;
	counter_global->rx_bwmap_overflow = 0;
	counter_global->rx_bwmap_inv0 = 0;
	counter_global->rx_bwmap_inv1 = 0;

	counter_global->rx_gem_los = gtc_counter.lods_events;
	counter_global->rx_gem_idle = gtc_counter.gem_idle;
	counter_global->rx_gem_non_idle = 0;
	counter_global->rx_hec_correct = gtc_counter.gem_hec_errors_corr;
	counter_global->rx_over_interleaving = 0;
	counter_global->rx_gem_len_mis = 0;
	counter_global->rx_match_multi_flow = 0;

	counter_global->rx_eth_unicast = 0;
	counter_global->rx_eth_multicast = 0;
	counter_global->rx_eth_multicast_fwd = 0;
	counter_global->rx_eth_multicast_leak = 0;
	counter_global->rx_eth_fcs_err = 0;

	counter_global->rx_omci = 0;
	counter_global->rx_omci_crc_err = 0;
	counter_global->rx_omci_proc = 0;
	counter_global->rx_omci_drop = 0;

	counter_global->tx_boh_cnt = 0;

	counter_global->tx_dbru_cnt = 0;

	counter_global->tx_ploam_cnt = ploam_us_counter.all;
	counter_global->tx_ploam_proc = 0;
	counter_global->tx_ploam_urg = 0;
	counter_global->tx_ploam_urg_proc = 0;
	counter_global->tx_ploam_nor = 0;
	counter_global->tx_ploam_nor_proc = 0;
	counter_global->tx_ploam_sn = ploam_us_counter.ser_no;
	counter_global->tx_ploam_nomsg = ploam_us_counter.no_message;

	counter_global->tx_gem_cnt = gem_counter.tx_frames;
	counter_global->tx_gem_byte = gem_counter.tx_bytes;

	counter_global->tx_eth_abort_ebb = 0;

	counter_global->tx_omci_proc = 0;

	return 0;
}

int
gpon_hw_prx126_counter_tcont_get(int tcont_id, struct gpon_counter_tcont_t *counter_tcont)
{
	struct intel_px126_pon_alloc_counters counter;

	if (counter_tcont==NULL)
		return -1;

	intel_px126_pm_alloc_counters_get(tcont_id,&counter);

	//intel_px126_pm_gem_all_counters_get();

	counter_tcont->tx_gem_pkt = 0;
	counter_tcont->tx_eth_pkt = 0;
	counter_tcont->tx_idle_byte = 0; // "counter.idle" GEM idle frames

	return 0;
}

int
gpon_hw_prx126_counter_dsflow_get(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow)
{
	struct intel_px126_pon_gem_port_counters gem_port_counter;
	struct intel_px126_pon_eth_counters gem_eth_counter;

	if (counter_dsflow==NULL)
		return -1;

	intel_px126_pm_gem_port_counters_get((unsigned int)dsflow_id, &gem_port_counter);
	counter_dsflow->rx_gem_block = gem_port_counter.rx_frames;
	counter_dsflow->rx_gem_byte = gem_port_counter.rx_bytes;

	intel_px126_pm_eth_rx_counters_get((unsigned int)dsflow_id, &gem_eth_counter);
	counter_dsflow->rx_eth_pkt_fwd = (gem_eth_counter.frames_lt_64 +
					  gem_eth_counter.frames_64 +
					  gem_eth_counter.frames_65_127 +
					  gem_eth_counter.frames_128_255 +
					  gem_eth_counter.frames_256_511 +
					  gem_eth_counter.frames_512_1023 +
					  gem_eth_counter.frames_1024_1518 +
					  gem_eth_counter.frames_gt_1518);

	counter_dsflow->rx_eth_pkt = (counter_dsflow->rx_eth_pkt_fwd +
				      gem_eth_counter.frames_fcs_err +
				      gem_eth_counter.frames_too_long);

	return 0;
}

int
gpon_hw_prx126_counter_usflow_get(int usflow_id, struct gpon_counter_usflow_t *counter_usflow)
{
	struct intel_px126_pon_gem_port_counters gem_port_counter;
	struct intel_px126_pon_eth_counters gem_eth_counter;

	if (counter_usflow==NULL)
		return -1;

	intel_px126_pm_gem_port_counters_get((unsigned int)usflow_id, &gem_port_counter);
#if 0
	// workaround the issue that usgem byte count wont clear-after-read when counter is overflow 32bit
	{	
		static unsigned long long real_usgem_byte_prev[GPON_FLOW_ID_TOTAL];
		unsigned long long real_usgem_byte = gem_port_counter.tx_bytes + gem_port_counter.tx_frames * 5;

		// we detect if counter dont clear-after-read by checking the avg size
		if (gem_port_counter.tx_frames > 0) {
			// workaround, try to calc the different
			if (gem_port_counter.tx_bytes/gem_port_counter.tx_frames > 2031) {
				unsigned long long corrected_usgem_byte = real_usgem_byte -real_usgem_byte_prev[usflow_id] - gem_port_counter.tx_frames * 5;
				dbprintf(LOG_NOTICE, "\nus %d, blk=%lu, byte=%llu(%x:%08x), avgpkt=%lu => byte=%llu, avgpkt=%lu\n", 
					usflow_id, gem_port_counter.tx_frames, gem_port_counter.tx_bytes,
					(unsigned int)((gem_port_counter.tx_bytes>>32) &0xffffffff), 
					(unsigned int)(gem_port_counter.tx_bytes & 0xffffffff),
					(unsigned int)(gem_port_counter.tx_bytes/gem_port_counter.tx_frames),
					corrected_usgem_byte,
					(unsigned int)(corrected_usgem_byte/gem_port_counter.tx_frames));
				gem_port_counter.tx_bytes = corrected_usgem_byte;
			}				
		} else if (gem_port_counter.tx_frames == 0) {
			if (gem_port_counter.tx_bytes >0) {
				dbprintf(LOG_NOTICE, "\nus %d, blk=%lu, byte=%llu => byte=0\n", 
					usflow_id, gem_port_counter.tx_frames, gem_port_counter.tx_bytes);
				gem_port_counter.tx_bytes = 0;
			}
		}

		real_usgem_byte_prev[usflow_id] = real_usgem_byte;
	} 
#endif

	counter_usflow->tx_gem_block = gem_port_counter.tx_frames;
	counter_usflow->tx_gem_byte = gem_port_counter.tx_bytes;

	intel_px126_pm_eth_tx_counters_get((unsigned int)usflow_id, &gem_eth_counter);
	counter_usflow->tx_eth_cnt = (gem_eth_counter.frames_lt_64 +
					  gem_eth_counter.frames_64 +
					  gem_eth_counter.frames_65_127 +
					  gem_eth_counter.frames_128_255 +
					  gem_eth_counter.frames_256_511 +
					  gem_eth_counter.frames_512_1023 +
					  gem_eth_counter.frames_1024_1518 +
					  gem_eth_counter.frames_gt_1518);

	return 0;
}

// pm api, get 64bit counter from gpon mib /////////////////////////////////////////////////////

int
gpon_hw_prx126_pm_refresh(int is_reset)
{
	// 220923 LEO START rewrite for counter reset feature
	static int refresh_lock = 0;
	int total=sizeof (struct gpon_mib_t)/sizeof(unsigned long long), i;
	struct gpon_mib_t mib_hw;
	unsigned long long *value_current = (void *)&pm_mib_current;
	unsigned long long *value_hw = (void *)&mib_hw;
	unsigned long long *value_hwprev = (void *)&pm_mib_hwprev;

	if(refresh_lock)  return -1;  //avoid re-entry from differnet thread
	refresh_lock = 1;

	gpon_hw_g.counter_global_get(&mib_hw.global);
	for (i=0; i < GPON_TCONT_ID_TOTAL; i++) {
		gpon_hw_g.counter_tcont_get(i, &mib_hw.tcont[i]);
	}

	for (i=0; i < GPON_FLOW_ID_TOTAL; i++) {
		gpon_hw_g.counter_dsflow_get(i, &mib_hw.dsflow[i]);
		gpon_hw_g.counter_usflow_get(i, &mib_hw.usflow[i]);
	}

	if (is_reset) {
		memset(&pm_mib_current, 0, sizeof(pm_mib_current));
	} else {
		for (i=0; i<total; i++)
		{
			value_current[i] = value_current[i] + util_ull_value_diff(value_hw[i], value_hwprev[i]);
		}
	}

	pm_mib_hwprev = mib_hw;
	refresh_lock = 0;
	// 220923 LEO END

	return 0;
}

int
gpon_hw_prx126_pm_global_get(struct gpon_counter_global_t *counter_global)
{
	*counter_global = pm_mib_current.global;

	return 0;
}

int
gpon_hw_prx126_pm_tcont_get(int tcont_id, struct gpon_counter_tcont_t *counter_tcont)
{
	if(tcont_id >= GPON_TCONT_ID_TOTAL) {
		dbprintf(LOG_ERR, "Warning tcont_id should less than %d\n", GPON_TCONT_ID_TOTAL);
		return -1;
	}

	*counter_tcont = pm_mib_current.tcont[tcont_id];

	return 0;
}

int
gpon_hw_prx126_pm_dsflow_get(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow)
{
	if(dsflow_id >= GPON_FLOW_ID_TOTAL) {
		dbprintf(LOG_ERR, "Warning dsflow_id should less than %d\n", GPON_FLOW_ID_TOTAL);
		return -1;
	}

	*counter_dsflow = pm_mib_current.dsflow[dsflow_id];

	return 0;
}

int
gpon_hw_prx126_pm_usflow_get(int usflow_id, struct gpon_counter_usflow_t *counter_usflow)
{
	if(usflow_id >= GPON_FLOW_ID_TOTAL) {
		dbprintf(LOG_ERR, "Warning usflow_id should less than %d\n", GPON_FLOW_ID_TOTAL);
		return -1;
	}

	*counter_usflow = pm_mib_current.usflow[usflow_id];

	return 0;
}
