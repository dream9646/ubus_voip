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
 * Module  : cli
 * File    : cli_pon.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
 
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cli.h"
#include "me_related.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "coretask.h"	// for coretask_cmd();

static char *flow_type_str[] = { "omci", "ether", "tdm", "?" };
static char gem2dsflowid[4096], gem2usflowid[4096][2];
static int
load_flowid_table(void)
{
	struct gpon_tm_dsflow_config_t dsflow_config;
	struct gpon_tm_usflow_config_t usflow_config;
	int i;

	for (i=0; i<4096; i++)
		gem2dsflowid[i] = gem2usflowid[i][0] = gem2usflowid[i][1] = -1;

	for (i=0; i<GPON_FLOW_ID_TOTAL; i++) {
		if (gpon_hw_g.tm_dsflow_get(i, &dsflow_config)==0 && dsflow_config.enable)
			gem2dsflowid[dsflow_config.gemport] = i;
		if (gpon_hw_g.tm_usflow_get(i, &usflow_config)==0 && usflow_config.enable) {
			if(gem2usflowid[usflow_config.gemport][0] == -1) //is used ?
				gem2usflowid[usflow_config.gemport][0] = i;
			else
				gem2usflowid[usflow_config.gemport][1] = i;
		}
	}
	return 0;
}

static int
tcont_map_print(int fd)
{
	int is_header_printed=0, i;
	int tcont_id;

	// allocid - hw tcont id
	for (i=0; i<4096; i++) {
		if (gpon_hw_g.tm_tcont_get_by_allocid(i, &tcont_id)!=0)
			continue;
		if (!is_header_printed) {
			util_fdprintf(fd, "%11s %7s\n", "AllocId", "TcontId");
			util_fdprintf(fd, "%11s %7s\n", "-----------", "-------");
			is_header_printed = 1;
		}	
		util_fdprintf(fd, "%4u(0x%03x) %7u\n", i, i, tcont_id);
	}
	if (!is_header_printed)
		util_fdprintf(fd, "no active tcont?\n");
	
	util_fdprintf(fd, "\n");
	return 0;
}	

static int
pq_map_print(int fd)
{
	int is_header_printed=0, i;
	struct gpon_tm_pq_config_t pq_config;

	for (i=0; i<GPON_PQ_ID_TOTAL; i++) {
		if (gpon_hw_g.tm_pq_config_get(i, &pq_config)!=0 || !pq_config.enable)
			continue;		
		if (!is_header_printed) {
			util_fdprintf(fd, "%3s %7s %4s %6s %10s %10s %11s %10s %11s\n", 
				"PQ", "TcontId", "Type", "Weight", "EgressDrop", "CIR(8kbps)", "CIR(Mbps)", "PIR(8kbps)", "PIR(Mbps)");
			util_fdprintf(fd, "%3s %7s %4s %6s %10s %10s %11s %10s %11s\n", 
				"---", "-------", "----", "------", "----------", "----------", "-----------", "----------", "-----------");
			is_header_printed = 1;
		}	

		util_fdprintf(fd, "%3u %7u %4s %6u %10s ",
			i, pq_config.tcont_id, (pq_config.policy==GPON_TM_POLICY_SP)?"SP":"WRR", 
			pq_config.wrr_weight, (pq_config.egress_drop)?"enabled":"disabled");
		// tm_pq_config_get() cir/pir unit is byte/s, hw cir/pir unit is 8kbps
		util_fdprintf(fd, "%10u %11s ", pq_config.cir/125/8, util_bps_to_ratestr(pq_config.cir*8ULL));
		util_fdprintf(fd, "%10u %11s\n", pq_config.pir/125/8, util_bps_to_ratestr(pq_config.pir*8ULL));
	}
	if (!is_header_printed)
		util_fdprintf(fd, "no active pq?\n");
	
	util_fdprintf(fd, "\n");
	return 0;
}	

static int
gem_map_print(int fd)
{
	struct gpon_tm_usflow_config_t usflow_config;
	struct gpon_tm_dsflow_config_t dsflow_config;
	int is_header_printed=0, i;

	load_flowid_table();

	for (i=0; i<4096; i++) {
		if (gem2usflowid[i][0]<0 && gem2dsflowid[i]<0)
			continue;
		if (!is_header_printed) {
			util_fdprintf(fd, "%11s %5s %3s %5s  %5s %5s %5s %3s\n", 
				"Gem", "UsFlw", "PQ", "Type", "DsFlw", "Type", "Mcast", "Aes");
			util_fdprintf(fd, "%11s %5s %3s %5s  %5s %5s %5s %3s\n", 
				"-----------", "-----", "---", "-----",  "-----", "-----", "-----", "---");
			is_header_printed = 1;
		}	

		util_fdprintf(fd, "%4u(0x%03x) ", i, i);
		if (gem2usflowid[i][0]>=0) {
			int pq_id = -1;
			gpon_hw_g.tm_usflow2pq_get(gem2usflowid[i][0], &pq_id);
			gpon_hw_g.tm_usflow_get(gem2usflowid[i][0], &usflow_config);
			util_fdprintf(fd, "%5u %3u %5s  ", 
				gem2usflowid[i][0], pq_id, flow_type_str[usflow_config.flow_type%4]);
		} else {
			util_fdprintf(fd, "%5s %3s %5s  ", "-", "-", "-");
		}
				
		if (gem2dsflowid[i]>=0) {
			gpon_hw_g.tm_dsflow_get(gem2dsflowid[i], &dsflow_config);
			util_fdprintf(fd, "%5u %5s %5u %3u\n", 
				gem2dsflowid[i], flow_type_str[dsflow_config.flow_type%4],  
				dsflow_config.mcast_enable, dsflow_config.aes_enable);
		} else {			
			util_fdprintf(fd, "%5s %5s %5s %3s\n", "-", "-", "-", "-");
		}

		if(omci_env_g.tm_pq_gem_qos_method==1) {
			//yellow
			if (gem2usflowid[i][1]>=0) {
				int pq_id = -1;
				gpon_hw_g.tm_usflow2pq_get(gem2usflowid[i][1], &pq_id);
				gpon_hw_g.tm_usflow_get(gem2usflowid[i][1], &usflow_config);
				util_fdprintf(fd, "%4u(0x%03x) ", i, i);
				util_fdprintf(fd, "%5u %3u %5s  ", 
					gem2usflowid[i][1], pq_id, flow_type_str[usflow_config.flow_type%4]);
				util_fdprintf(fd, "%5s %5s %5s %3s\n", "-", "-", "-", "-");
			}
		}
	}			
	if (!is_header_printed)
		util_fdprintf(fd, "no active gemport?\n");
	
	util_fdprintf(fd, "\n");
	return 0;
}	

static int
cli_gpon_summary(int fd)
{
	unsigned long long superframe_seq=27888118;

	// superframe
	gpon_hw_g.onu_superframe_seq_get(&superframe_seq);
	util_fdprintf(fd, "superframe seq=%llu\n\n", superframe_seq);

	tcont_map_print(fd);
	pq_map_print(fd);
	gem_map_print(fd);
	
	return CLI_OK;
}

static int
global_counter_print(int fd, int is_reset)
{
	static struct timeval global_tv;
	struct timeval now;
	unsigned long msec;
	static unsigned long long last_tx_gem_byte, last_rx_gem_idle;
	struct gpon_counter_global_t cnt_global;

	if (!is_reset)	// the reset of pm is done in the caller of this printer function
		gpon_hw_g.pm_refresh(0);//0:refresh 1: reset

	util_get_uptime(&now);
	msec = util_timeval_diff_msec(&now, &global_tv);
	if (msec ==0) msec = 1;
	global_tv = now;

	if(is_reset) {
		last_tx_gem_byte = 0;
		return 0;
	}

	gpon_hw_g.pm_global_get(&cnt_global);

	util_fdprintf(fd, "Downstream global counter:\n");

	util_fdprintf(fd, "%7s", "Phy:");
	util_fdprintf(fd, "\t%12s=%-10llu", "biperr_bit", cnt_global.rx_bip_err_bit);
	util_fdprintf(fd, "\t%12s=%-10llu", "biperr_blk", cnt_global.rx_bip_err_block);
	util_fdprintf(fd, "\t%12s=%-10llu", "supfm_loss", cnt_global.rx_lom);
	util_fdprintf(fd, "\t%12s=%-10llu", "plen_err", cnt_global.rx_plen_err);
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "%7s", "");
	util_fdprintf(fd, "\t%12s=%-10llu", "fec_cor_bit", cnt_global.rx_fec_correct_bit);
	util_fdprintf(fd, "\t%12s=%-10llu", "fec_cor_byte", cnt_global.rx_fec_correct_byte);
	util_fdprintf(fd, "\t%12s=%-10llu", "fec_cor_cw", cnt_global.rx_fec_correct_cw);
	util_fdprintf(fd, "\t%12s=%-10llu", "fec_uncor_cw", cnt_global.rx_fec_uncor_cw);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Eth:");
	util_fdprintf(fd, "\t%12s=%-10llu", "unicast", cnt_global.rx_eth_unicast);
	util_fdprintf(fd, "\t%12s=%-10llu", "mcast", cnt_global.rx_eth_multicast);
	util_fdprintf(fd, "\t%12s=%-10llu", "mcast_fwd", cnt_global.rx_eth_multicast_fwd);
	util_fdprintf(fd, "\t%12s=%-10llu", "mcast_leak", cnt_global.rx_eth_multicast_leak);
	util_fdprintf(fd, "\t%12s=%-10llu", "fcs_err", cnt_global.rx_eth_fcs_err);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Gem:");
	util_fdprintf(fd, "\t%12s=%-10llu", "non_idle_pkt", cnt_global.rx_gem_non_idle);
	util_fdprintf(fd, "\t%12s=%-10llu (link idle BW=%s)", "idle_pkt", cnt_global.rx_gem_idle,
		util_bps_to_ratestr(util_ull_value_diff(cnt_global.rx_gem_idle, last_rx_gem_idle)*5*8ULL *1000/msec));
	util_fdprintf(fd, "\t%12s=%-10llu", "multiflow", cnt_global.rx_match_multi_flow);
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "%7s", "");
	util_fdprintf(fd, "\t%12s=%-10llu", "los", cnt_global.rx_gem_los);
	util_fdprintf(fd, "\t%12s=%-10llu", "hec_corret", cnt_global.rx_hec_correct);
	util_fdprintf(fd, "\t%12s=%-10llu", "ov_interlv", cnt_global.rx_over_interleaving);
	util_fdprintf(fd, "\t%12s=%-10llu", "len_mis", cnt_global.rx_gem_len_mis);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Omci:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.rx_omci);
	util_fdprintf(fd, "\t%12s=%-10llu", "crc_err", cnt_global.rx_omci_crc_err);
	util_fdprintf(fd, "\t%12s=%-10llu", "proc", cnt_global.rx_omci_proc);
	util_fdprintf(fd, "\t%12s=%-10llu", "drop", cnt_global.rx_omci_drop);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Ploam:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.rx_ploam_cnt);
	util_fdprintf(fd, "\t%12s=%-10llu", "err", cnt_global.rx_ploam_err);
	util_fdprintf(fd, "\t%12s=%-10llu", "corrected", cnt_global.rx_ploam_correctted);
 	util_fdprintf(fd, "\t%12s=%-10llu", "proc", cnt_global.rx_ploam_proc);
	util_fdprintf(fd, "\t%12s=%-10llu", "overflow", cnt_global.rx_ploam_overflow);
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "%7s", "");
	util_fdprintf(fd, "\t%12s=%-10llu", "unknown", cnt_global.rx_ploam_unknown);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Bwmap:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.rx_bwmap_cnt);
	util_fdprintf(fd, "\t%12s=%-10llu", "crc_err", cnt_global.rx_bwmap_crc_err);
	util_fdprintf(fd, "\t%12s=%-10llu", "overflow", cnt_global.rx_bwmap_overflow);
	util_fdprintf(fd, "\t%12s=%-10llu", "inv0", cnt_global.rx_bwmap_inv0);
	util_fdprintf(fd, "\t%12s=%-10llu", "inv1", cnt_global.rx_bwmap_inv1);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Act:");
	util_fdprintf(fd, "\t%12s=%-10llu", "sn_req", cnt_global.rx_sn_req);
	util_fdprintf(fd, "\t%12s=%-10llu", "ranging_rq", cnt_global.rx_ranging_req);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "Upstream global counter:\n");
	util_fdprintf(fd, "%7s", "Phy:");
	util_fdprintf(fd, "\t%12s=%-10llu", "boh_cnt", cnt_global.tx_boh_cnt);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Eth:");
	util_fdprintf(fd, "\t%12s=%-10llu", "abort_ebb", cnt_global.tx_eth_abort_ebb);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Gem:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.tx_gem_cnt);
	util_fdprintf(fd, "\t%12s=%-10llu (onu got BW=%s)", "byte", cnt_global.tx_gem_byte,
		util_bps_to_ratestr(util_ull_value_diff(cnt_global.tx_gem_byte, last_tx_gem_byte)*8ULL *1000/msec));
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Omci:");
	util_fdprintf(fd, "\t%12s=%-10llu", "proc", cnt_global.tx_omci_proc);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Ploam:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.tx_ploam_cnt);
	util_fdprintf(fd, "\t%12s=%-10llu", "nomsg", cnt_global.tx_ploam_nomsg);
	util_fdprintf(fd, "\t%12s=%-10llu", "proc", cnt_global.tx_ploam_proc);
	util_fdprintf(fd, "\t%12s=%-10llu", "sn", cnt_global.tx_ploam_sn);
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "%7s", "");
	util_fdprintf(fd, "\t%12s=%-10llu", "urg", cnt_global.tx_ploam_urg);
	util_fdprintf(fd, "\t%12s=%-10llu", "urg_proc", cnt_global.tx_ploam_urg_proc);
	util_fdprintf(fd, "\t%12s=%-10llu", "nor", cnt_global.tx_ploam_nor);
	util_fdprintf(fd, "\t%12s=%-10llu", "nor_proc", cnt_global.tx_ploam_nor_proc);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "%7s", "Dbru:");
	util_fdprintf(fd, "\t%12s=%-10llu", "total", cnt_global.tx_dbru_cnt);
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "\n");

	last_tx_gem_byte = cnt_global.tx_gem_byte;
	last_rx_gem_idle = cnt_global.rx_gem_idle;
	return 0;
}

static int
tcont_counter_print(int fd, int is_reset)
{
	struct gpon_counter_tcont_t cnt;
	int is_header_printed=0, i, j;

	static struct timeval tcont_tv;
	struct timeval now;
	unsigned long msec;
	static unsigned long long last_tx_idle_byte[GPON_TCONT_ID_TOTAL];

	if (!is_reset)	// the reset of pm is done in the caller of this printer function
		gpon_hw_g.pm_refresh(0);//0:refresh 1: reset

	util_get_uptime(&now);
	msec = now.tv_sec*1000ULL+now.tv_usec/1000 - 
		(tcont_tv.tv_sec*1000ULL+tcont_tv.tv_usec/1000);
	if (msec ==0) msec = 1;
	tcont_tv = now;

	if(is_reset) {
		memset(last_tx_idle_byte, 0, sizeof(last_tx_idle_byte));
		return 0;
	}

	for (i=0; i<4096; i++) {
		int tcont_id;

		if (gpon_hw_g.tm_tcont_get_by_allocid(i, &tcont_id)!=0)
			continue;
		if (!is_header_printed) {
			util_fdprintf(fd, "%11s %7s %12s\n", "AllocId" , "TcontId", "Idle BW");
			util_fdprintf(fd, "%11s %7s %12s\n", "-----------" , "-------", "------------");
			is_header_printed = 1;
		}
		if (gpon_hw_g.pm_tcont_get(tcont_id, &cnt) != 0) {
			util_fdprintf(fd, "%4u(0x%03x) %7d %12s\n", i, i, tcont_id, "n/a");
		} else {
			util_fdprintf(fd, "%4u(0x%03x) %7d %12s\n", i, i, tcont_id, 
				util_bps_to_ratestr(util_ull_value_diff(cnt.tx_idle_byte, last_tx_idle_byte[tcont_id])*8ULL *1000/msec));
		}
	}

	//update current value for next time referance;
	for(j=0; j < GPON_TCONT_ID_TOTAL; j++) {
		gpon_hw_g.pm_tcont_get(j, &cnt);
		last_tx_idle_byte[j]=cnt.tx_idle_byte;
	}

	if (!is_header_printed)
		util_fdprintf(fd, "no active tcont?\n");
	
	util_fdprintf(fd, "\n");
	return 0;
}

static int
gem_counter_print(int fd, int is_reset) 
{
	static unsigned long long last_tx_gem_byte[GPON_FLOW_ID_TOTAL];
	static unsigned long long last_rx_gem_byte[GPON_FLOW_ID_TOTAL];
	struct gpon_counter_dsflow_t cnt_ds[GPON_FLOW_ID_TOTAL];
	struct gpon_counter_usflow_t cnt_us[GPON_FLOW_ID_TOTAL];
	static struct timeval gem_tv;
	struct timeval now;
	unsigned long msec;
	int is_header_printed=0, i, j;
	
	load_flowid_table();

	if (!is_reset)	// the reset of pm is done in the caller of this printer function
		gpon_hw_g.pm_refresh(0);//0:refresh 1: reset

	util_get_uptime(&now);
	msec = now.tv_sec*1000ULL+now.tv_usec/1000 - 
			(gem_tv.tv_sec*1000ULL+gem_tv.tv_usec/1000);
	if (msec ==0) msec = 1;
	gem_tv = now;

	if(is_reset) {
		memset(last_tx_gem_byte, 0, sizeof(last_tx_gem_byte));
		memset(last_rx_gem_byte, 0, sizeof(last_rx_gem_byte));
		return 0;
	}

	// get all flow counters in one shot
	memset(&cnt_ds, 0, sizeof(cnt_ds));
	memset(&cnt_us, 0, sizeof(cnt_us));
	for(j=0; j < GPON_FLOW_ID_TOTAL; j++) {
		gpon_hw_g.pm_usflow_get(j, &cnt_us[j]);
		gpon_hw_g.pm_dsflow_get(j, &cnt_ds[j]);
	}

	for (i=0; i<4096; i++) {	// iterate gemport id 0..4095
		if (gem2dsflowid[i]==-1 && gem2usflowid[i][0]==-1)
			continue;
		if (!is_header_printed) {
			util_fdprintf(fd, "%11s %3s %3s %12s %10s %10s %11s  %12s %10s %10s %10s %11s\n", 
				"Gem", "Flw", "PQ", 
				"UsGemByte", "UsGemPkt", "UsEthPkt", "UsRate", 
				"DsGemByte", "DsGemPkt", "DsEthPkt", "FwdEthPkt", "DsRate");
			util_fdprintf(fd, "%11s %3s %3s %12s %10s %10s %11s  %12s %10s %10s %10s %11s\n", 
				"-----------", "---", "---", 
				"------------", "----------", "---------", "-----------",
				"------------", "----------", "---------", "---------", "-----------");
			is_header_printed = 1;
		}

		util_fdprintf(fd, "%4u(0x%03x) ", i, i);			
		if (gem2usflowid[i][0] >= 0) {
			int flowid = gem2usflowid[i][0];
			int pq_id = -1;
			gpon_hw_g.tm_usflow2pq_get(flowid, &pq_id); 
			// 220715 LEO START: correct pq_id fdprintf from unsigned to signed
			#if 0
			util_fdprintf(fd, "%3u %3u %12llu %10llu %10llu %11s  ", flowid, pq_id,
			#else
			util_fdprintf(fd, "%3u %3d %12llu %10llu %10llu %11s  ", flowid, pq_id, 
			#endif
			// 220715 LEO END
				cnt_us[flowid].tx_gem_byte, cnt_us[flowid].tx_gem_block, cnt_us[flowid].tx_eth_cnt,
				util_bps_to_ratestr(util_ull_value_diff(cnt_us[flowid].tx_gem_byte, last_tx_gem_byte[flowid])*8ULL *1000/msec));
		} else {
			util_fdprintf(fd, "%3s %3s %12s %10s %10s %11s  ", "-", "-", "-", "-", "-", "-");
		}
		if (gem2dsflowid[i] >= 0) {
			int flowid = gem2dsflowid[i];
			util_fdprintf(fd, "%12llu %10llu %10llu %10llu %11s\n",
				cnt_ds[flowid].rx_gem_byte, cnt_ds[flowid].rx_gem_block, cnt_ds[flowid].rx_eth_pkt, cnt_ds[flowid].rx_eth_pkt_fwd,
				util_bps_to_ratestr(util_ull_value_diff(cnt_ds[flowid].rx_gem_byte, last_rx_gem_byte[flowid])*8ULL *1000/msec));
		} else {
			util_fdprintf(fd, "%12s %10s %10s %10s %11s\n", "-", "-", "-", "-", "-");
		}

		//yellow
		if (gem2usflowid[i][1] >= 0) {
			int flowid = gem2usflowid[i][1];
			int pq_id = -1;
			gpon_hw_g.tm_usflow2pq_get(flowid, &pq_id); 
			util_fdprintf(fd, "%4u(0x%03x) ", i, i);			
			// 220720 LEO START: correct pq_id fdprintf from unsigned to signed
			#if 0
			util_fdprintf(fd, "%3u %3u %12llu %10llu %10llu %11s  ", flowid, pq_id, 
			#else
			util_fdprintf(fd, "%3u %3d %12llu %10llu %10llu %11s  ", flowid, pq_id, 
			#endif
			// 220715 LEO END
				cnt_us[flowid].tx_gem_byte, cnt_us[flowid].tx_gem_block, cnt_us[flowid].tx_eth_cnt,
				util_bps_to_ratestr(util_ull_value_diff(cnt_us[flowid].tx_gem_byte, last_tx_gem_byte[flowid])*8ULL *1000/msec));
			util_fdprintf(fd, "%12s %10s %10s %10s %11s\n", "-", "-", "-", "-", "-");
		}
	}

	// keep flow byte counters for next time referance
	for(j=0; j < GPON_FLOW_ID_TOTAL; j++) {
		last_tx_gem_byte[j]=cnt_us[j].tx_gem_byte;
		last_rx_gem_byte[j]=cnt_ds[j].rx_gem_byte;
	}

	if (!is_header_printed)
		util_fdprintf(fd, "no active gemport?\n");
		
	util_fdprintf(fd, "\n");
	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_gpon_help(int fd)
{
	util_fdprintf(fd, 
		"gpon [tcont|pq|gem|clearpq|recovery|serdes_reset]\n"
		"     cnt [global|tcont|gem|*|reset]\n"
		"     history|h [0..12]|reset\n"
		);
}

int
cli_gpon_cmdline(int fd, int argc, char *argv[])
{

	if (strcmp(argv[0], "gpon")==0) {
		if (argc==1) {
			return cli_gpon_summary(fd);

		} else if (argc==2 && strcmp(argv[1], "help")==0) {
			cli_gpon_help(fd);
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "clearpq")==0) {
			if (gpon_hw_g.util_clearpq) {
				unsigned int state;			
				gpon_hw_g.util_protect_start(&state);
				gpon_hw_g.util_clearpq(0);
				gpon_hw_g.util_protect_end(state);
			}
			return CLI_OK;

		} else if (argc>=2 && strcmp(argv[1], "reset_dbru")==0) {
			if (gpon_hw_g.util_reset_dbru) {
				unsigned int state;			
				gpon_hw_g.util_protect_start(&state);
				gpon_hw_g.util_clearpq(0);
				gpon_hw_g.util_reset_dbru();
				gpon_hw_g.util_protect_end(state);
			}
			return CLI_OK;
		
		} else if (argc>=2 && strcmp(argv[1], "reset_serdes")==0) {
			coretask_cmd(CORETASK_CMD_MIB_RESET);
			if (gpon_hw_g.util_reset_serdes) {
				if (argc == 2) {
					gpon_hw_g.util_reset_serdes(10);
				} else if (argc == 3) {
					int second = util_atoi(argv[2]);
					gpon_hw_g.util_reset_serdes(1000*second);
				}
			}
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "recovery")==0) {
			if (gpon_hw_g.util_recovery)
				gpon_hw_g.util_recovery();
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "tcont")==0) {
			tcont_map_print(fd);
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "pq")==0) {
			pq_map_print(fd);
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "gem")==0) {
			gem_map_print(fd);
			return CLI_OK;

		} else if (argc>=2 && strcmp(argv[1], "cnt")==0) {
			if (argc==2) {
				global_counter_print(fd, 0);
				tcont_counter_print(fd, 0);
				pq_map_print(fd);
				gem_counter_print(fd, 0);
				return CLI_OK;		
			} else {
				if (strcmp(argv[2], "global")==0) {
					global_counter_print(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "tcont")==0) {
					tcont_counter_print(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "gem")==0) {
					gem_counter_print(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					global_counter_print(fd, 0);
					tcont_counter_print(fd, 0);
					gem_counter_print(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "reset")==0) {
					gpon_hw_g.pm_refresh(1);//0:refresh 1: reset
					global_counter_print(fd, 1);
					gem_counter_print(fd, 1);
					tcont_counter_print(fd, 1);
					return CLI_OK;
				} 
			}
		} else if (argc>=2 && (strcmp(argv[1], "history")==0 || strcmp(argv[1], "h")==0)) {
			if (argc==2) {
				gpon_hw_g.history_print(fd, -1);	// show type help
				util_fdprintf(fd, "  reset - clear gpon history\n");
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "reset") == 0) {
					gpon_hw_g.history_init();
					coretask_cmd(CORETASK_CMD_SWITCH_HISTORY_INTERVAL_CHANGE);
					return CLI_OK;					
				} else {
					int type = util_atoi(argv[2]);
					gpon_hw_g.history_print(fd, type);
					return CLI_OK;
				}
			}

		} else {
			return CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'gpon'		
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}
