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
 * File    : gpon_hw_prx126_history.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"
#include "util_run.h"
#include "list.h"
#include "meinfo.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"
#include "switch.h"
#include "env.h"

// this implement gpon counter history with functions in gpon_hw_prx126_pm.c

struct gpon_history_t {
	int valid;
	long second;	// uptime in seconds
	struct gpon_counter_global_t global;
};

#define GPON_HISTORY_MODE_DIFF	0
#define GPON_HISTORY_MODE_TOTAL	1
// while we want to display last 96 history record, we maintain 96+1 record actually
// as we need one more record to calculate 96 diffs
#define GPON_HISTORY_SIZE		(96+1)	// 24 hour * 4

static struct gpon_history_t gpon_history[GPON_HISTORY_SIZE];	// to keep per port difference
static int gpon_history_next_i = 0;

static char *
uptime_to_localtime_str(long uptime_in_seconds)
{
	static char timestr[32];
	long uptime_now = util_get_uptime_sec();
	long seconds=time(0) - (uptime_now - uptime_in_seconds);
	struct tm *t=(struct tm *)localtime(&seconds);

	snprintf(timestr, 32, "%04d/%02d/%02d %02d:%02d:%02d",
		t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	return timestr;
}

int
gpon_hw_prx126_history_init(void)
{
	memset(gpon_history, 0, sizeof(gpon_history));
	// point to end of ring, rotate for 1 step
	gpon_history_next_i = GPON_HISTORY_SIZE-1;
	gpon_hw_prx126_history_add();
	gpon_history[ GPON_HISTORY_SIZE-1].valid = 0;

	return 0;
}

// update gpon_history[next_i]
static int
gpon_hw_prx126_history_update(void)
{
	// fetch counters from hw into pm internal data stru
	gpon_hw_prx126_pm_refresh(0);

	gpon_history[gpon_history_next_i].second = util_get_uptime_sec();
	gpon_hw_prx126_pm_global_get(&gpon_history[gpon_history_next_i].global);
	gpon_history[gpon_history_next_i].valid = 1;
	return 0;
}

// update gpon_history[next_i] & inc gpon_history_next_i
int
gpon_hw_prx126_history_add()
{
	gpon_hw_prx126_history_update();
	gpon_history_next_i = (gpon_history_next_i+1) % GPON_HISTORY_SIZE;
	dbprintf(LOG_INFO, "gpon_history_next_i is changed to %d\n", gpon_history_next_i);
	return 0;
}

// 0:rxpkt, 1:txpkt, 2:fwdrop, 3:rxerr, 4:txerr, 5:rxrate, 6:txrate, 7:rxbyte, 8:txbyte
// 9:rxpktucast, 10:txpktucast, 11:rxpktmcast, 12:txpktmcast, 13:rxpktbcast, 14:txpktbcast
int
gpon_hw_prx126_history_print(int fd, int type)
{
	static char *type_str[] = {
		"DsPHY", "UsPHY",
		"DsETH", "UsETH",
		"DsGEM", "UsGEM",
		"DsOMCI", "UsOMCI",
		"DsPLOAM", "UsPLOAM",
		"BWMAP", "ACT", "DBRU",
		NULL };

	int i, line_printed = 0;
	long start_time = 0, end_time = 0;

	if (type < 0 || type > 12) {
		util_fdprintf(fd, "supported history type:\n");
		for (i=0; type_str[i] != NULL; i++) {
			util_fdprintf(fd, "  %d - %s\n", i, type_str[i]);
		}
		return -1;
	}

	gpon_hw_prx126_history_update();	// update the next_i record but not inc the next_i value

	for (i = 0; i < GPON_HISTORY_SIZE-1; i++) {	// show next_i+2... next_i+2 + GPON_HISTORY_SIZE-2, total show = GPON_HISTORY_SIZE -1
		unsigned int prev = (gpon_history_next_i + i + 1) % GPON_HISTORY_SIZE;
		unsigned int curr = (gpon_history_next_i + i + 2) % GPON_HISTORY_SIZE;
		struct gpon_history_t *h0 = &gpon_history[prev];
		struct gpon_history_t *h = &gpon_history[curr];
		struct gpon_counter_global_t *g0 = &gpon_history[prev].global;
		struct gpon_counter_global_t *g = &gpon_history[curr].global;
		unsigned int second_diff = h->second - h0->second;

		if (!h->valid)
			continue;
		if (start_time == 0)
			start_time = h0->second;

		if (line_printed %24 == 0) {
			util_fdprintf(fd, "%-9s",  type_str[type]);
			switch (type) {
			case 0: util_fdprintf(fd, "%10s %10s %10s %10s %10s %10s %10s %10s\n",
				"biperr_bit", "biperr_blk", "supfm_loss", "plen_err",
				"fec_cr_bit", "fec_cr_byt", "fec_cr_cw", "fec_uncr_cw"); // ds_phy
				break;
			case 1: util_fdprintf(fd, "%12s\n",
				"boh_cnt"); // us_phy
				break;
			case 2: util_fdprintf(fd, "%10s %10s %10s %10s %10s\n",
				"unicast", "mcast", "mcast_fwd", "mcast_leak", "fcs_err"); // ds_eth
				break;
			case 3: util_fdprintf(fd, "%10s\n",
				"abort_ebb"); // us_eth
				break;
			case 4: util_fdprintf(fd, "%12s %20s %11s %10s %10s %10s %10s %10s\n",
				"non_idle", "idle_pkt", "lnk_idle_BW", "multiflow", "los", "hec_corret", "ov_interlv", "len_mis"); // ds_gem
				break;
			case 5: util_fdprintf(fd, "%12s %20s %11s\n",
				"total", "byte", "onu_got_BW"); // us_gem
				break;
			case 6: util_fdprintf(fd, "%12s %10s %10s %10s\n",
				"total", "crc_err", "proc", "drop"); // ds_omci
				break;
			case 7: util_fdprintf(fd, "%10s\n",
				"proc"); // us_omci
				break;
			case 8: util_fdprintf(fd, "%10s %10s %10s %10s %10s %10s\n",
				"total", "err", "corrected", "proc", "overflow", "unknown"); // ds_ploam
				break;
			case 9: util_fdprintf(fd, "%10s %10s %10s %10s %10s %10s %10s %10s \n",
				"total", "nomsg", "proc", "urg", "urg_proc", "nor", "nor_proc", "sn"); // us_ploam
				break;
			case 10: util_fdprintf(fd, "%12s %10s %10s %10s %10s\n",
				"total", "crc_err", "overflow", "inv0", "inv1"); // ds_bwmap
				break;
			case 11: util_fdprintf(fd, "%10s %10s\n",
				"sn_req", "ranging_rq"); // ds_act
				break;
			case 12: util_fdprintf(fd, "%10s\n",
				"total"); // dbru
				break;
			}
		}

		if (second_diff == 0)
			second_diff = 1;

		util_fdprintf(fd, "%03d%6s", line_printed, "");

		if (omci_env_g.switch_history_mode == GPON_HISTORY_MODE_DIFF) {	// show history in diff
			switch (type) {
			case 0: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_bip_err_bit-g0->rx_bip_err_bit, g->rx_bip_err_block-g0->rx_bip_err_block, g->rx_lom-g0->rx_lom, g->rx_plen_err-g0->rx_plen_err,
				g->rx_fec_correct_bit-g0->rx_fec_correct_bit, g->rx_fec_correct_byte-g0->rx_fec_correct_byte, g->rx_fec_correct_cw-g0->rx_fec_correct_cw, g->rx_fec_uncor_cw-g0->rx_fec_uncor_cw);
				break;
			case 1: util_fdprintf(fd, "%12llu\n",
				g->tx_boh_cnt-g0->tx_boh_cnt);
				break;
			case 2: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu\n",
				g->rx_eth_unicast-g0->rx_eth_unicast, g->rx_eth_multicast-g0->rx_eth_multicast, g->rx_eth_multicast_fwd-g0->rx_eth_multicast_fwd, g->rx_eth_multicast_leak-g0->rx_eth_multicast_leak, g->rx_eth_fcs_err-g0->rx_eth_fcs_err);
				break;
			case 3: util_fdprintf(fd, "%10llu\n",
				g->tx_eth_abort_ebb-g0->tx_eth_abort_ebb);
				break;
			case 4: util_fdprintf(fd, "%12llu %20llu %11s %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_gem_non_idle-g0->rx_gem_non_idle, g->rx_gem_idle-g0->rx_gem_idle,
				util_bps_to_ratestr((g->rx_gem_idle-g0->rx_gem_idle)*5*8ULL/second_diff),
				g->rx_match_multi_flow-g0->rx_match_multi_flow,
				g->rx_gem_los-g0->rx_gem_los, g->rx_hec_correct-g0->rx_hec_correct, g->rx_over_interleaving-g0->rx_over_interleaving, g->rx_gem_len_mis-g0->rx_gem_len_mis);
				break;
			case 5: util_fdprintf(fd, "%12llu %20llu %11s\n",
				g->tx_gem_cnt-g0->tx_gem_cnt, g->tx_gem_byte-g0->tx_gem_byte,
				util_bps_to_ratestr((g->tx_gem_byte-g0->tx_gem_byte)*8ULL/second_diff));
				break;
			case 6: util_fdprintf(fd, "%12llu %10llu %10llu %10llu\n",
				g->rx_omci-g0->rx_omci, g->rx_omci_crc_err-g0->rx_omci_crc_err, g->rx_omci_proc-g0->rx_omci_proc, g->rx_omci_drop-g0->rx_omci_drop);
				break;
			case 7: util_fdprintf(fd, "%10llu\n",
				g->tx_omci_proc-g0->tx_omci_proc);
				break;
			case 8: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_ploam_cnt-g0->rx_ploam_cnt, g->rx_ploam_err-g0->rx_ploam_err, g->rx_ploam_correctted-g0->rx_ploam_correctted, g->rx_ploam_proc-g0->rx_ploam_proc, g->rx_ploam_overflow-g0->rx_ploam_overflow, g->rx_ploam_unknown-g0->rx_ploam_unknown);
				break;
			case 9: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu %10llu %10llu \n",
				g->tx_ploam_cnt-g0->tx_ploam_cnt, g->tx_ploam_nomsg-g0->tx_ploam_nomsg, g->tx_ploam_proc-g0->tx_ploam_proc, g->tx_ploam_urg-g0->tx_ploam_urg,
				g->tx_ploam_urg_proc-g0->tx_ploam_urg_proc, g->tx_ploam_nor-g0->tx_ploam_nor, g->tx_ploam_nor_proc-g0->tx_ploam_nor_proc, g->tx_ploam_sn-g0->tx_ploam_sn);
				break;
			case 10: util_fdprintf(fd, "%12llu %10llu %10llu %10llu %10llu\n",
				g->rx_bwmap_cnt-g0->rx_bwmap_cnt, g->rx_bwmap_crc_err-g0->rx_bwmap_crc_err, g->rx_bwmap_overflow-g0->rx_bwmap_overflow, g->rx_bwmap_inv0-g0->rx_bwmap_inv0, g->rx_bwmap_inv1-g0->rx_bwmap_inv1);
				break;
			case 11: util_fdprintf(fd, "%10llu %10llu\n",
				g->rx_sn_req-g0->rx_sn_req, g->rx_ranging_req-g0->rx_ranging_req);
				break;
			case 12: util_fdprintf(fd, "%10llu\n",
				g->tx_dbru_cnt-g0->tx_dbru_cnt);
				break;
			}
		} else {
			switch (type) {
			case 0: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_bip_err_bit, g->rx_bip_err_block, g->rx_lom, g->rx_plen_err,
				g->rx_fec_correct_bit, g->rx_fec_correct_byte, g->rx_fec_correct_cw, g->rx_fec_uncor_cw);
				break;
			case 1: util_fdprintf(fd, "%12llu\n",
				g->tx_boh_cnt);
				break;
			case 2: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu\n",
				g->rx_eth_unicast, g->rx_eth_multicast, g->rx_eth_multicast_fwd, g->rx_eth_multicast_leak, g->rx_eth_fcs_err);
				break;
			case 3: util_fdprintf(fd, "%10llu\n",
				g->tx_eth_abort_ebb);
				break;
			case 4: util_fdprintf(fd, "%12llu %20llu %11s %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_gem_non_idle, g->rx_gem_idle,
				util_bps_to_ratestr((g->rx_gem_idle-g0->rx_gem_idle)*5*8ULL/second_diff),
				g->rx_match_multi_flow,
				g->rx_gem_los, g->rx_hec_correct, g->rx_over_interleaving, g->rx_gem_len_mis);
				break;
			case 5: util_fdprintf(fd, "%12llu %20llu %11s\n",
				g->tx_gem_cnt, g->tx_gem_byte,
				util_bps_to_ratestr((g->tx_gem_byte-g0->tx_gem_byte)*8ULL/second_diff));
				break;
			case 6: util_fdprintf(fd, "%12llu %10llu %10llu %10llu\n",
				g->rx_omci, g->rx_omci_crc_err, g->rx_omci_proc, g->rx_omci_drop);
				break;
			case 7: util_fdprintf(fd, "%10llu\n",
				g->tx_omci_proc);
				break;
			case 8: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu\n",
				g->rx_ploam_cnt, g->rx_ploam_err, g->rx_ploam_correctted, g->rx_ploam_proc, g->rx_ploam_overflow, g->rx_ploam_unknown);
				break;
			case 9: util_fdprintf(fd, "%10llu %10llu %10llu %10llu %10llu %10llu %10llu %10llu \n",
				g->tx_ploam_cnt, g->tx_ploam_nomsg, g->tx_ploam_proc, g->tx_ploam_urg,
				g->tx_ploam_urg_proc, g->tx_ploam_nor, g->tx_ploam_nor_proc, g->tx_ploam_sn);
				break;
			case 10: util_fdprintf(fd, "%12llu %10llu %10llu %10llu %10llu\n",
				g->rx_bwmap_cnt, g->rx_bwmap_crc_err, g->rx_bwmap_overflow, g->rx_bwmap_inv0, g->rx_bwmap_inv1);
				break;
			case 11: util_fdprintf(fd, "%10llu %10llu\n",
				g->rx_sn_req, g->rx_ranging_req);
				break;
			case 12: util_fdprintf(fd, "%10llu\n",
				g->tx_dbru_cnt);
				break;
			}
		}
		end_time = h->second;
		line_printed++;
	}

	{
		char start_str[32], end_str[32];
		strncpy(start_str, uptime_to_localtime_str(start_time), 32);
		strncpy(end_str, uptime_to_localtime_str(end_time), 32);
		util_fdprintf(fd, "%s..%s (%s mode, %d min per line)\n",
			start_str, end_str,
			omci_env_g.switch_history_mode?"total":"diff", omci_env_g.switch_history_interval);
	}
	return 0;
}
