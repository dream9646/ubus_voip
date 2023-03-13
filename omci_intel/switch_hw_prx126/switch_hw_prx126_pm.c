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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_pm.c
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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include
#include "util.h"
#include "util_run.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "switch.h"
#include "env.h"
#include "iphost.h"
#include "omciutil_misc.h"
#include "intel_px126_pm.h"

/// counter api, get counter from underlying hw////////////////////////////////////////////////////

int
switch_hw_prx126_counter_global_get(struct switch_counter_global_t *counter_global)
{
	if (counter_global == NULL)
		return -1;
	
	counter_global->dot1dTpLearnedEntryDiscards = 0;

	return 0;
}

int
switch_hw_prx126_counter_port_get(int port_id, struct switch_counter_port_t *counter_port)
{
	int ret = 0;
	//unsigned int phy_portid=0, ext_portid=0;
	struct intel_px126_switch_eth_port_counters counter={0};
	unsigned int me_id;

	if (counter_port == NULL)
		return -1;

	memset(counter_port, 0, sizeof(struct switch_counter_port_t));

	if (port_id == 0) {			// uni0:0
		me_id = 0x100+1;
	// 220809 LEO START add pon & cpu interface
	} else if (port_id == 5) {  // pon:5
		struct meinfo_t *miptr = meinfo_util_miptr(130);
		struct me_t *me;
		list_for_each_entry(me, &miptr->me_instance_list, instance_node)
		{	// this for loop run only once ideally
			me_id = me->meid;
		}
	} else if (port_id == 6) {  // cpu:6
		me_id = 0;
	// 220809 LEO END
	} else {
		// 220809 LEO START suppress console outputs
		//dbprintf(LOG_ERR, "unknow portnum=%d\n", port_id);
		// 220809 LEO END
		return 0;
	}

	// 220811 LEO remove an "#if 0" block

	// 220810 LEO START add cpu counter statistics
	if (port_id == 6 && me_id == 0)
	{
		#define STATPATH "/sys/class/net/eth0_0_1_lct/statistics/"
		FILE *sys_fp;
		sys_fp=popen("/bin/cat "STATPATH"rx_bytes "STATPATH"rx_packets "STATPATH"multicast \
			"STATPATH"rx_dropped "STATPATH"tx_bytes "STATPATH"tx_dropped "STATPATH"tx_packets \
			"STATPATH"rx_nohandler "STATPATH"collisions "STATPATH"rx_errors "STATPATH"tx_errors " , "r");
		if(!sys_fp)
		{
			dbprintf(LOG_ERR, "popen error.\n");
			return -1;
		}
		fscanf(sys_fp, "%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n", \
			&counter_port->ifInOctets, &counter_port->ifInUcastPkts, &counter_port->ifInMulticastPkts, \
			&counter_port->ifInDiscards, &counter_port->ifOutOctets, &counter_port->ifOutDiscards, \
			&counter_port->ifOutUcastPkts, &counter_port->dot3ControlInUnknownOpcodes, \
			&counter_port->etherStatsCollisions, &counter_port->etherStatsCRCAlignErrors, \
			&counter_port->etherStatsTxCRCAlignErrors);
		pclose(sys_fp);
		#undef STATPATH

		return 0;
	}
	// 220810 LEO END

	if ((ret = intel_px126_pm_eth_port_counter_get(me_id, &counter)) != 0) {
		// 220914 LEO START suppress error message when no ifname info
		dbprintf(LOG_NOTICE, "rtk_stat_port_getAll error %x for port %d\n", ret, port_id);
		// 220914 LEO END
		return ret;
	}

	counter_port->ifInOctets = counter.RxGoodBytes;
	counter_port->ifInUcastPkts = counter.RxUnicastPkts;
	counter_port->ifInMulticastPkts = counter.RxMulticastPkts;
	counter_port->ifInBroadcastPkts = counter.RxBroadcastPkts;
	counter_port->ifInDiscards = counter.RxDroppedPkts;

	counter_port->ifOutOctets = counter.TxGoodBytes;
	counter_port->ifOutDiscards = counter.TxDroppedPkts;
	counter_port->ifOutUcastPkts = counter.TxUnicastPkts;
	counter_port->ifOutMulticastPkts = counter.TxMulticastPkts;
	counter_port->ifOutBroadcastPkts = counter.TxBroadcastPkts;	// sdk mib name differs than RMON 

	counter_port->dot1dBasePortDelayExceededDiscards = 0;//counter.dot1dBasePortDelayExceededDiscards;
	counter_port->dot1dTpPortInDiscards = 0;//counter.dot1dTpPortInDiscards;
	counter_port->dot1dTpHcPortInDiscards = 0;//counter.dot1dTpHcPortInDiscards;

	counter_port->dot3InPauseFrames = counter.RxGoodPausePkts;
	counter_port->dot3OutPauseFrames = counter.TxPauseCount;
	counter_port->dot3OutPauseOnFrames = 0;//counter.dot3OutPauseOnFrames;
	counter_port->dot3StatsAligmentErrors = counter.RxAlignErrorPkts;
	counter_port->dot3StatsFCSErrors = counter.RxFCSErrorPkts;
	counter_port->dot3StatsSingleCollisionFrames = counter.TxSingleCollCount;
	counter_port->dot3StatsMultipleCollisionFrames = counter.TxMultCollCount;
	counter_port->dot3StatsDeferredTransmissions = 0;//counter.dot3StatsDeferredTransmissions;
	counter_port->dot3StatsLateCollisions = counter.TxLateCollCount;
	counter_port->dot3StatsExcessiveCollisions = counter.TxExcessCollCount;
	counter_port->dot3StatsFrameTooLongs = 0;//counter.dot3StatsFrameTooLongs;
	counter_port->dot3StatsSymbolErrors = 0;//counter.dot3StatsSymbolErrors;
	counter_port->dot3ControlInUnknownOpcodes = 0;//counter.dot3ControlInUnknownOpcodes;

	counter_port->etherStatsDropEvents = 0;//counter.etherStatsDropEvents;
	counter_port->etherStatsOctets = 0;//counter.etherStatsOctets;
	counter_port->etherStatsBcastPkts = 0;//counter.etherStatsBcastPkts;
	counter_port->etherStatsMcastPkts = 0;//counter.etherStatsMcastPkts;
	counter_port->etherStatsUndersizePkts = 0;//counter.etherStatsUndersizePkts;
	counter_port->etherStatsOversizePkts = 0;//counter.etherStatsOversizePkts;
	counter_port->etherStatsFragments = 0;//counter.etherStatsFragments;
	counter_port->etherStatsJabbers = 0;//counter.etherStatsJabbers;
	counter_port->etherStatsCollisions = counter.TxCollCount;
	counter_port->etherStatsCRCAlignErrors = 0;//counter.etherStatsCRCAlignErrors;

	counter_port->etherStatsPkts64Octets = 0;//counter.etherStatsPkts64Octets;
	counter_port->etherStatsPkts65to127Octets = 0;//counter.etherStatsPkts65to127Octets;
	counter_port->etherStatsPkts128to255Octets = 0;//counter.etherStatsPkts128to255Octets;
	counter_port->etherStatsPkts256to511Octets = 0;//counter.etherStatsPkts256to511Octets;
	counter_port->etherStatsPkts512to1023Octets = 0;//counter.etherStatsPkts512to1023Octets;
	counter_port->etherStatsPkts1024to1518Octets = 0;//counter.etherStatsPkts1024to1518Octets;

	counter_port->etherStatsTxOctets = 0;//counter.etherStatsTxOctets;
	counter_port->etherStatsTxUndersizePkts = counter.TxUnderSizeGoodPkts;
	counter_port->etherStatsTxOversizePkts = counter.TxOversizeGoodPkts;
	counter_port->etherStatsTxPkts64Octets = counter.Tx64BytePkts;
	counter_port->etherStatsTxPkts65to127Octets = counter.Tx127BytePkts;
	counter_port->etherStatsTxPkts128to255Octets = counter.Tx255BytePkts;
	counter_port->etherStatsTxPkts256to511Octets = counter.Tx511BytePkts;
	counter_port->etherStatsTxPkts512to1023Octets = counter.Tx1023BytePkts;
	// 220914 LEO START move ?xMaxBytePkts assignment from etherStats?xPkts1519toMaxOctets to etherStats?xPkts1024to1518Octets
	counter_port->etherStatsTxPkts1024to1518Octets = counter.TxMaxBytePkts;
	counter_port->etherStatsTxPkts1519toMaxOctets = 0;
	// 220914 LEO END
	counter_port->etherStatsTxBroadcastPkts = 0;//counter.etherStatsTxBcastPkts;	// sdk mib name differs than RMON 
	counter_port->etherStatsTxMulticastPkts = 0;//counter.etherStatsTxMcastPkts;	// sdk mib name differs than RMON 
	counter_port->etherStatsTxFragments = 0;//counter.etherStatsTxFragments;
	counter_port->etherStatsTxJabbers = 0;//counter.etherStatsTxJabbers;
	counter_port->etherStatsTxCRCAlignErrors = 0;//counter.etherStatsTxCRCAlignErrors;

	counter_port->etherStatsRxUndersizePkts = counter.RxUnderSizeGoodPkts;
	counter_port->etherStatsRxUndersizeDropPkts = counter.RxUnderSizeErrorPkts;
	counter_port->etherStatsRxOversizePkts = counter.RxOversizeGoodPkts;
	counter_port->etherStatsRxPkts64Octets = counter.Rx64BytePkts;
	counter_port->etherStatsRxPkts65to127Octets = counter.Rx127BytePkts;
	counter_port->etherStatsRxPkts128to255Octets = counter.Rx255BytePkts;
	counter_port->etherStatsRxPkts256to511Octets = counter.Rx511BytePkts;
	counter_port->etherStatsRxPkts512to1023Octets = counter.Rx1023BytePkts;
	// 220914 LEO START move ?xMaxBytePkts assignment from etherStats?xPkts1519toMaxOctets to etherStats?xPkts1024to1518Octets
	counter_port->etherStatsRxPkts1024to1518Octets = counter.RxMaxBytePkts;
	counter_port->etherStatsRxPkts1519toMaxOctets = 0;
	// 220914 LEO END

	counter_port->inOampduPkts = 0;//counter.inOampduPkts;
	counter_port->outOampduPkts = 0;//counter.outOampduPkts;
	
	return 0;
}

int
switch_hw_prx126_counter_wifi_get(int port_id, struct switch_counter_port_t *counter_port)
{
	if (counter_port == NULL)
		return -1;

	memset(counter_port, 0, sizeof(struct switch_counter_port_t));

	return 0;
}

// pm api, get 64bit counter from switch mib /////////////////////////////////////////////////////

static struct switch_mib_t switch_mib_current; 
static struct switch_mib_t switch_mib_hwprev; 

int
switch_hw_prx126_pm_reset_port(unsigned int logical_portid) 
{
	unsigned int phy_portid=0, ext_portid=0;
	unsigned int me_id;

	switch_hw_prx126_conv_portid_logical_to_private(logical_portid, &phy_portid, &ext_portid);

	if (phy_portid == 0) {
		me_id = 0x100+1;
		intel_px126_pm_eth_port_refresh(me_id);
	memset(&switch_mib_current.port[logical_portid], 0, sizeof(struct switch_counter_port_t));
	}

	return 0;
}

int
switch_hw_prx126_pm_refresh(int is_reset) 
{
	static int refresh_lock = 0;
	int total=sizeof (struct switch_mib_t)/sizeof(unsigned long long);
	struct switch_mib_t switch_mib_hw;
	unsigned long long *value_current = (void*)&switch_mib_current;
	unsigned long long *value_hw = (void*)&switch_mib_hw;
	unsigned long long *value_hwprev = (void*)&switch_mib_hwprev;
	int i;
	unsigned int me_id;

	if (refresh_lock)	// avoid re-entry from different thread
		return -1;
	refresh_lock = 1;

	if (is_reset) {
		//int is_wlan_up = 0;
		for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
			unsigned int phy_portid=0, ext_portid=0;
			if ((1<<i) & switch_get_all_logical_portmask_without_cpuext()) {
				switch_hw_prx126_conv_portid_logical_to_private(i, &phy_portid, &ext_portid);
				if (phy_portid == 0)
					me_id = 0x100+1;
				else 
					continue;

				intel_px126_pm_eth_port_refresh(me_id);
			}
		}
		// reset wifi counters
		//iphost_get_interface_updown("wlan0", &is_wlan_up);
		//if(is_wlan_up) util_write_file("/proc/wlan0/stats", "0\n");
		//iphost_get_interface_updown("wlan1", &is_wlan_up);
		//if(is_wlan_up) util_run_by_thread("cli_client cmd 'echo 0 > /proc/wlan0/stats'", 0);
	}
	
	switch_hw_prx126_counter_global_get(&switch_mib_hw.global);

	i=0; // port id 0 for PRX126

#if 1 // 220309 LEO START: enable this partial code, which was originally disabled for compile purpose 
	for(i=0; i < SWITCH_LOGICAL_PORT_TOTAL; i++)
	{
		if( (1<<i) & switch_get_all_logical_portmask_without_cpuext() )
		{
			switch_hw_prx126_counter_port_get(i, &switch_mib_hw.port[i]);
		}
	
	}
#endif // 220309 LEO END

#if 0
	for (i=0; i < SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & switch_get_all_logical_portmask_without_cpuext())
			switch_hw_prx126_counter_port_get(i, &switch_mib_hw.port[i]);	// our func, use logical id directly
		else if ((1<<i) & switch_get_wifi_logical_portmask())
			switch_hw_prx126_counter_wifi_get(i, &switch_mib_hw.port[i]);
	}
#endif
	if (is_reset) {
		memset(&switch_mib_current, 0, sizeof(switch_mib_current));
	} else {
		for (i=0; i<total; i++)
			value_current[i] = value_current[i] + util_ull_value_diff(value_hw[i], value_hwprev[i]);
	}
	
	switch_mib_hwprev = switch_mib_hw;

	refresh_lock = 0;
	return 0;
}

int
switch_hw_prx126_pm_global_get(struct switch_counter_global_t *counter_global)
{
	*counter_global = switch_mib_current.global;
	return 0;
}

int
switch_hw_prx126_pm_port_get(int port_id, struct switch_counter_port_t *counter_port)
{
	if (((1<<port_id) & switch_get_all_logical_portmask()) == 0) // include wifi ports
		return -1;
	*counter_port = switch_mib_current.port[port_id];
	return 0;
}

///////////////////////////////////////////////////////

// type 0:tx, 1:rx, 2:tx/rx
int 
switch_hw_prx126_pm_summary_print(int fd, int type)
{
	static struct switch_counter_port_t counter_port_prev[SWITCH_LOGICAL_PORT_TOTAL];
	struct switch_counter_port_t counter_port[SWITCH_LOGICAL_PORT_TOTAL];
	char port_label[SWITCH_LOGICAL_PORT_TOTAL][8];
	unsigned int port_id;
	unsigned int all_portmask_without_cpuext = switch_get_all_logical_portmask_without_cpuext();

	static struct timeval pkt_tv;
	struct timeval now;
	unsigned long msec;

	int ret = 0;
	
	// prepare port_label	
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if (((1<<port_id) & all_portmask_without_cpuext) ==0)
			continue;
		if ((1<<port_id) & switch_get_cpu_logical_portmask()) {
			snprintf(port_label[port_id], 8, "cpu");
		} else if (port_id == switch_get_wan_logical_portid()) {
			snprintf(port_label[port_id], 8, "pon");
		} else if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			snprintf(port_label[port_id], 8, "%s", omciutil_misc_get_uniname_by_logical_portid(port_id));
		}
	}

	// snapshot all counters by port first
	if (type == -1) {	// reset counter
		switch_hw_prx126_pm_refresh(1);
	} else {
		switch_hw_prx126_pm_refresh(0);
	}
	memset(&counter_port, 0, sizeof(counter_port));
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if (((1<<port_id) & all_portmask_without_cpuext) ==0)
			continue;
		if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port[port_id])) != 0) {
			dbprintf(LOG_ERR, "switch_hw_fvt2510_pm_port_get error %x for port %d\n", ret, port_id);
			return ret;
		}
	}

	//gettimeofday(&now, NULL);
	util_get_uptime(&now);
	msec = util_timeval_diff_msec(&now, &pkt_tv);
	if (msec == 0) msec =1;
	pkt_tv = now;
	
	if (type==0 || type>3) {
		// summary
		util_fdprintf(fd, "%4s %10s %10s %10s %10s %10s %10s %11s %11s %12s\n", 
			"if", "RxPkt", "TxPkt", "EthDrop", "FwdDrop", "RxErr", "TxErr", "RxRate", "TxRate", "DropErrRatio");
		util_fdprintf(fd, "%4s %10s %10s %10s %10s %10s %10s %11s %11s %12s\n", 
			"----", "----------", "----------", "----------", "----------", "----------", "----------", "-----------", "-----------", "------------");

		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			struct switch_counter_port_t *c = &counter_port[port_id];
			unsigned long long rxpkt, txpkt, rxerr, txerr;
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;

			rxpkt = (c->ifInUcastPkts != 0) ? c->ifInUcastPkts : // 220811 LEO add condition for cpu counter
				c->ifInUcastPkts + 
				c->ifInMulticastPkts + 
				c->ifInBroadcastPkts;
			txpkt = (c->ifOutUcastPkts != 0) ? c->ifOutUcastPkts : // 220811 LEO add condition for cpu counter
				c->ifOutUcastPkts + 
				c->ifOutMulticastPkts + 
				c->ifOutBroadcastPkts;
			rxerr = (c->etherStatsCRCAlignErrors != 0) ? c->etherStatsCRCAlignErrors : // 220811 LEO add condition for cpu counter
				c->etherStatsRxUndersizePkts +
				//c->etherStatsRxOversizePkts +
				// c->etherStatsDropEvents +		// show in summary
				c->etherStatsRxUndersizeDropPkts +
				c->etherStatsFragments +
				//c->etherStatsCRCAlignErrors +		// same as dot3StatsFCSErrors+dot3StatsAligmentErrors
				//c->dot1dTpPortInDiscards +		// show in summary
				c->dot3ControlInUnknownOpcodes +
				//c->dot3StatsFrameTooLongs +
				c->dot3StatsSymbolErrors +
				c->dot3StatsFCSErrors +
				c->dot3StatsAligmentErrors;
			txerr = (c->etherStatsTxCRCAlignErrors != 0) ? c->etherStatsTxCRCAlignErrors : // 220811 LEO add condition for cpu counter
				c->etherStatsTxUndersizePkts +
				//c->etherStatsTxOversizePkts +
				c->etherStatsTxJabbers +
				//c->etherStatsCollisions +		// same as dot3Stats*Collision*
				c->dot3StatsDeferredTransmissions +
				c->dot3StatsSingleCollisionFrames +
				c->dot3StatsMultipleCollisionFrames +
				c->dot3StatsLateCollisions +
				c->dot3StatsExcessiveCollisions;

			util_fdprintf(fd, "%4s %10llu %10llu %10llu %10llu %10llu %10llu %11s %11s %12.3e\n", port_label[port_id],
				rxpkt, txpkt,
				c->etherStatsDropEvents,
				c->dot1dTpPortInDiscards,
				rxerr, txerr,
				util_bps_to_ratestr(util_ull_value_diff(c->ifInOctets, counter_port_prev[port_id].ifInOctets)*8ULL *1000/msec),
				util_bps_to_ratestr(util_ull_value_diff(c->ifOutOctets, counter_port_prev[port_id].ifOutOctets)*8ULL *1000/msec),
				((rxpkt+rxerr)==0) ? 0 : (float)(rxerr+c->etherStatsDropEvents+c->dot1dTpPortInDiscards)/(float)(rxpkt+rxerr));
		}
	}

	if (type==1 || type>3) {
		// if rx counters
		util_fdprintf(fd, "%6s %20s %10s %10s %10s %10s %11s\n", 
			"ifIn", "Octets", "UcastPkts", "McastPkts", "BcastPkts", "Discard", "Rate");
		util_fdprintf(fd, "%6s %20s %10s %10s %10s %10s %11s\n", 
			"------", "--------------------", "----------", "----------", "----------", "----------", "-----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%6s %20llu %10llu %10llu %10llu %10llu %11s\n", port_label[port_id],  
				counter_port[port_id].ifInOctets, 
				counter_port[port_id].ifInUcastPkts, 
				counter_port[port_id].ifInMulticastPkts,
				counter_port[port_id].ifInBroadcastPkts,
				counter_port[port_id].ifInDiscards,
				util_bps_to_ratestr(util_ull_value_diff(counter_port[port_id].ifInOctets, counter_port_prev[port_id].ifInOctets)*8ULL *1000/msec));
		}

		// if tx counters
		util_fdprintf(fd, "%6s %20s %10s %10s %10s %10s %11s\n", 
			"ifOut", "Octets", "UcastPkts", "McastPkts", "BcastPkts", "Discard", "Rate");
		util_fdprintf(fd, "%6s %20s %10s %10s %10s %10s %11s\n", 
			"------", "--------------------", "----------", "----------", "----------", "----------", "-----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%6s %20llu %10llu %10llu %10llu %10llu %11s\n", port_label[port_id],  
				counter_port[port_id].ifOutOctets, 
				counter_port[port_id].ifOutUcastPkts, 
				counter_port[port_id].ifOutMulticastPkts,
				counter_port[port_id].ifOutBroadcastPkts,
				counter_port[port_id].ifOutDiscards,
				util_bps_to_ratestr(util_ull_value_diff(counter_port[port_id].ifOutOctets, counter_port_prev[port_id].ifOutOctets)*8ULL *1000/msec));
		}
	}
	
	if (type==2 || type>3) {
		// eth rx counters
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s %10s %10s\n", 
			"ethRx", "64byte", "65-127", "128-255", "256-511", "512-1023", "1024-1518", "1519+");
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s %10s %10s\n", 
			"------", "----------", "----------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%6s %10llu %10llu %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].etherStatsRxPkts64Octets,
				counter_port[port_id].etherStatsRxPkts65to127Octets,
				counter_port[port_id].etherStatsRxPkts128to255Octets,
				counter_port[port_id].etherStatsRxPkts256to511Octets,
				counter_port[port_id].etherStatsRxPkts512to1023Octets,
				counter_port[port_id].etherStatsRxPkts1024to1518Octets,
				counter_port[port_id].etherStatsRxPkts1519toMaxOctets);
		}

		// eth tx counters
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s %10s %10s\n", 
			"ethTx", "64byte", "65-127", "128-255", "256-511", "512-1023", "1024-1518", "1519+");
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s %10s %10s\n", 
			"------", "----------", "----------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%6s %10llu %10llu %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].etherStatsTxPkts64Octets,
				counter_port[port_id].etherStatsTxPkts65to127Octets,
				counter_port[port_id].etherStatsTxPkts128to255Octets,
				counter_port[port_id].etherStatsTxPkts256to511Octets,
				counter_port[port_id].etherStatsTxPkts512to1023Octets,
				counter_port[port_id].etherStatsTxPkts1024to1518Octets,
				counter_port[port_id].etherStatsTxPkts1519toMaxOctets);
		}

		// dot3 counters
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s\n", 
			".3Misc", "RxOamPdu", "TxOamPdu", "RxPause", "TxPause", "TxPauseOn");
		util_fdprintf(fd, "%6s %10s %10s %10s %10s %10s\n", 
			"------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%6s %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].inOampduPkts,
				counter_port[port_id].outOampduPkts,
				counter_port[port_id].dot3InPauseFrames,
				counter_port[port_id].dot3OutPauseFrames,
				counter_port[port_id].dot3OutPauseOnFrames);
		}
		util_fdprintf(fd, "\n");
	}
		
	if (type==3 || type>3) {
		// eth rx err
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s %10s\n", 
			"ethRxErr", "DropEvents", "Undersize", "OverSize", "UderSzDrop", "Fragments", "CRCAlign");
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s %10s\n", 
			"--------", "----------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%8s %10llu %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].etherStatsDropEvents,
				counter_port[port_id].etherStatsRxUndersizePkts,
				counter_port[port_id].etherStatsRxOversizePkts,
				counter_port[port_id].etherStatsRxUndersizeDropPkts,
				counter_port[port_id].etherStatsFragments,
				counter_port[port_id].etherStatsCRCAlignErrors);
		}
		// dot3 rx err
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s %10s\n", 
			".3RxErr", "FwdDiscard", "OpcodeErr", "SymboErr", "FcsErr", "AlignErr", "TooLong");
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s %10s\n", 
			"--------", "----------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%8s %10llu %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].dot1dTpPortInDiscards,
				counter_port[port_id].dot3ControlInUnknownOpcodes,
				counter_port[port_id].dot3StatsSymbolErrors,
				counter_port[port_id].dot3StatsFCSErrors,
				counter_port[port_id].dot3StatsAligmentErrors,
				counter_port[port_id].dot3StatsFrameTooLongs);
		}

		// eth tx err
		util_fdprintf(fd, "%8s %10s %10s %10s %10s\n", 
			"ethTxErr", "UnderSize", "OverSize", "Jabber", "Collision");
		util_fdprintf(fd, "%8s %10s %10s %10s %10s\n", 
			"--------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%8s %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].etherStatsTxUndersizePkts,
				counter_port[port_id].etherStatsTxOversizePkts,
				counter_port[port_id].etherStatsTxJabbers,
				counter_port[port_id].etherStatsCollisions);
		}
		// dot3 tx err
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s\n", 
			".3TxErr", "DeferXmit", "SingCol", "MultCol", "LateCol", "ExcsCol");
		util_fdprintf(fd, "%8s %10s %10s %10s %10s %10s\n", 
			"--------", "----------", "----------", "----------", "----------", "----------");
		for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
			if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
				continue;						
			util_fdprintf(fd, "%8s %10llu %10llu %10llu %10llu %10llu\n", port_label[port_id],  
				counter_port[port_id].dot3StatsDeferredTransmissions,
				counter_port[port_id].dot3StatsSingleCollisionFrames,
				counter_port[port_id].dot3StatsMultipleCollisionFrames,
				counter_port[port_id].dot3StatsLateCollisions,
				counter_port[port_id].dot3StatsExcessiveCollisions);
		}
	}

	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ( ( ((1<<port_id)&all_portmask_without_cpuext)==0) || ( strnlen(port_label[port_id],2)==0 ) )  // 220914 LEO suppress print if no port label
			continue;
		counter_port_prev[port_id] = counter_port[port_id];
	}

	return ret;
}

// below are for OMCI pm counters
int 
switch_hw_prx126_bridge_port_pm_data(unsigned int port_id_in, struct switch_bridge_port_pm_t *bridge_port_pm)
{
	/*	G984.4				RTL8305
	// 52 bridge_port_pm_data 9.3.9		//	mac bridge port

	forwarded_frame;			//	"STAT_IfOutUcastPkts" + "STAT_IfOutMulticastPkts" + "STAT_IfOutBroadcastPkts"
	received_frame;				//	"STAT_IfInUcastPkts" + "STAT_IfInMulticastPkts" + "STAT_IfInBroadcastPkts"
	received_and_discarded;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	*/

	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));	
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_fvt2510_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		bridge_port_pm->forwarded_frame = counter_port.ifInUcastPkts + counter_port.ifInMulticastPkts + counter_port.ifInBroadcastPkts;
		bridge_port_pm->received_frame = counter_port.ifOutUcastPkts + counter_port.ifOutMulticastPkts + counter_port.ifOutBroadcastPkts;
		bridge_port_pm->received_and_discarded = counter_port.etherStatsDropEvents;
		return ret;
	}
	bridge_port_pm->forwarded_frame = counter_port.ifOutUcastPkts + counter_port.ifOutMulticastPkts + counter_port.ifOutBroadcastPkts;
	bridge_port_pm->received_frame = counter_port.ifInUcastPkts + counter_port.ifInMulticastPkts + counter_port.ifInBroadcastPkts;
	bridge_port_pm->received_and_discarded = counter_port.etherStatsDropEvents;

	return ret;
}

// this me get counters in error categories
int 
switch_hw_prx126_eth_pm_data(unsigned int port_id_in, struct switch_ethernet_pm_t *ethernet_pm)
{

	/*	G984.4					RTL8305
	//class 24 ethernet_pm 9.5.2

	fcs_error;                         // rx	dot3StatsFCSErrors
	excessive_collision_counter;       // tx	etherStatsCollisions
	late_collision_counter;            // tx	dot3StatsLateCollisions
	frames_too_long;                   // rx	dot3StatsFrameTooLongs
	buffer_overflows_on_receive;	   // rx	SWITCH_NONE_SUPPORT
	buffer_overflows_on_transmit;      // tx	SWITCH_NONE_SUPPORT
	single_collision_frame_counter;    // tx	dot3StatsSingleCollisionFrames
	multiple_collisions_frame_counter; // tx	dot3StatsMultipleCollisionFrames
	sqe_counter;                       // ?		SWITCH_NONE_SUPPORT; Signal Quality Error test (a.k.a. heartbeat) is a means of detecting a transceivers inability to detect collisions. 
	deferred_transmission_counter;     // tx	dot3StatsDeferredTransmissions
	internal_mac_transmit_error_counter; // tx	SWITCH_NONE_SUPPORT
	carrier_sense_error_counter;       // tx	SWITCH_NONE_SUPPORT
	alignment_error_counter;           // rx	dot3StatsAligmentErrors
	internal_mac_receive_error_counter; //rx	SWITCH_NONE_SUPPORT
	*/
	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_pm->fcs_error = counter_port.etherStatsCRCAlignErrors; // dot3StatsFCSErrors is always 0 in HW
		ethernet_pm->excessive_collision_counter = counter_port.dot3StatsExcessiveCollisions;
		ethernet_pm->late_collision_counter = counter_port.dot3StatsLateCollisions;
		//ethernet_pm->frames_too_long = counter_port.dot3StatsFrameTooLongs; // Not support in HW, use 1519toMax to replace
		ethernet_pm->frames_too_long = counter_port.etherStatsTxPkts1519toMaxOctets;
		ethernet_pm->buffer_overflows_on_receive = 0;
		ethernet_pm->buffer_overflows_on_transmit = 0;
		ethernet_pm->single_collision_frame_counter = counter_port.dot3StatsSingleCollisionFrames;
		ethernet_pm->multiple_collisions_frame_counter = counter_port.dot3StatsMultipleCollisionFrames;
		ethernet_pm->sqe_counter = 0;
		ethernet_pm->deferred_transmission_counter = counter_port.dot3StatsDeferredTransmissions;
		ethernet_pm->internal_mac_transmit_error_counter = 0;
		ethernet_pm->carrier_sense_error_counter = 0;
		ethernet_pm->alignment_error_counter = counter_port.dot3StatsSymbolErrors; // dot3StatsAligmentErrors is always 0 in HW
		ethernet_pm->internal_mac_receive_error_counter = 0;
		return ret;
	}
	ethernet_pm->fcs_error = counter_port.etherStatsCRCAlignErrors; // dot3StatsFCSErrors is always 0 in HW
	ethernet_pm->excessive_collision_counter = counter_port.dot3StatsExcessiveCollisions;
	ethernet_pm->late_collision_counter = counter_port.dot3StatsLateCollisions;
	//ethernet_pm->frames_too_long = counter_port.dot3StatsFrameTooLongs; // Not support in HW, use 1519toMax to replace
	ethernet_pm->frames_too_long = counter_port.etherStatsRxPkts1519toMaxOctets;
	ethernet_pm->buffer_overflows_on_receive = 0;
	ethernet_pm->buffer_overflows_on_transmit = 0;
	ethernet_pm->single_collision_frame_counter = counter_port.dot3StatsSingleCollisionFrames;
	ethernet_pm->multiple_collisions_frame_counter = counter_port.dot3StatsMultipleCollisionFrames;
	ethernet_pm->sqe_counter = 0;
	ethernet_pm->deferred_transmission_counter = counter_port.dot3StatsDeferredTransmissions;
	ethernet_pm->internal_mac_transmit_error_counter = 0;
	ethernet_pm->carrier_sense_error_counter = 0;
	ethernet_pm->alignment_error_counter = counter_port.dot3StatsSymbolErrors; // dot3StatsAligmentErrors is always 0 in HW
	ethernet_pm->internal_mac_receive_error_counter = 0;

	return ret;
}

int 
switch_hw_prx126_eth_pm_data2(unsigned int port, struct switch_ethernet_pm2_t *ethernet_pm2)
{
	/*
		still not support
	*/
	return 0;
}

// this me get counters in upstreams direction
int 
switch_hw_prx126_eth_pm_data3(unsigned int port_id_in, struct switch_ethernet_frame_pm_t *ethernet_frame_pm)
{
	/*	G984.4					RTL8305
	// 296 eth_pm_data3 9.5.4                      // uni port rx

	drop_events;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	octets;				//	"ifInOctets" use bytes, and only care valid frames
	packets;			//	"ifInUcastPkts" + broadcast_packets + multicast_packets
	broadcast_packets;		//	"STAT_EtherStatsBroadcastPkts"- STAT_IfOutBroadcastPkts
	multicast_packets;		//	"STAT_EtherStatsMulticastPkts"- STAT_IfOutMulticastPkts
	crc_errored_packets;		//	not support (not use in eth_pm_data3, but in pm up/down stream)
	undersize_packets;		//	"etherStatsUndersizePkts",
	fragments;			//	"etherStatsFragments" (not use in pm up/down stream, but in pm_data3)
	jabbers;			//	"etherStatsOversizePkts",(the same as oversize_packets in eth_pm_data3)
	packets_64_octets;		//	"etherStatsRxPkts64Octets",
	packets_65_to_127_octets;	//	"etherStatsRxPkts65to127Octets",
	packets_128_to_255_octets;	//	"etherStatsRxPkts128to255Octets",
	packets_256_to_511_octets;	//	"etherStatsRxPkts256to511Octets",
	packets_512_to_1023_octets;	//	"etherStatsRxPkts512to1023Octets",
	packets_1024_to_1518_octets;	//	"etherStatsRxPkts1024toMaxOctets",
						STAT_IfOutMulticastPkts,
						STAT_IfOutBroadcastPkts
	*/

	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_frame_pm->drop_events = counter_port.etherStatsDropEvents;
		ethernet_frame_pm->octets = counter_port.ifOutOctets;
		//"ifOutUcastPkts" + broadcast + multicast
		ethernet_frame_pm->packets = counter_port.ifOutUcastPkts + counter_port.ifOutMulticastPkts + counter_port.ifOutBroadcastPkts;
		// etherStatsBcastPkts - STAT_IfOutBroadcastPkts
		ethernet_frame_pm->broadcast_packets = counter_port.ifOutBroadcastPkts;
		// etherStatsMcastPkts - STAT_IfOutMulticastPkts
		ethernet_frame_pm->multicast_packets = counter_port.ifOutMulticastPkts;
		ethernet_frame_pm->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
		ethernet_frame_pm->undersize_packets = counter_port.etherStatsUndersizePkts;
		ethernet_frame_pm->fragments = counter_port.etherStatsFragments;

		// T-REC-G.984.4-200802-I!!PDF-E.pdf
		// 9.5.4 Ethernet performance monitoring history data 3
		// Jabbers: The total number of packets received that were longer than 1518 octets, excluding framing bits but including FCS octets, and had either a bad frame check sequence (FCS) with an integral number of octets (FCS error) or a bad FCS with a non-integral number of octets (alignment error). The range to detect jabber is between 20 ms and 150 ms. (R) (mandatory) (4 bytes)

		// RFC 2819 etherStatsJabbers description:
		//   The total number of packets received that were longer than 1518 octets (excluding framing bits, but including FCS octets), and had either a bad Frame Check Sequence (FCS) with an integral number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).
		//
		// Note that this definition of jabber is different than the definition in IEEE-802.3 section 8.2.1.5 (10BASE5) and section 10.3.1.4 (10BASE2).  These documents define jabber as the condition where any packet exceeds 20 ms.  The allowed range to detect jabber is between 20 ms and 150 ms.

		// RFC 2819 etherStatsOversizePkts description:
		//   The total number of packets received that were longer than 1518 octets (excluding framing bits, but including FCS octets) and were otherwise well formed.
		ethernet_frame_pm->jabbers = counter_port.etherStatsJabbers;
		ethernet_frame_pm->packets_64_octets = counter_port.etherStatsTxPkts64Octets;
		ethernet_frame_pm->packets_65_to_127_octets = counter_port.etherStatsTxPkts65to127Octets;
		ethernet_frame_pm->packets_128_to_255_octets = counter_port.etherStatsTxPkts128to255Octets;
		ethernet_frame_pm->packets_256_to_511_octets = counter_port.etherStatsTxPkts256to511Octets;
		ethernet_frame_pm->packets_512_to_1023_octets = counter_port.etherStatsTxPkts512to1023Octets;
		ethernet_frame_pm->packets_1024_to_1518_octets = counter_port.etherStatsTxPkts1024to1518Octets;
		return ret;
	}
	ethernet_frame_pm->drop_events = counter_port.etherStatsDropEvents;
	ethernet_frame_pm->octets = counter_port.ifInOctets;
	//"ifInUcastPkts" + broadcast + multicast
	ethernet_frame_pm->packets = counter_port.ifInUcastPkts + counter_port.ifInMulticastPkts + counter_port.ifInBroadcastPkts;
	// etherStatsBcastPkts - STAT_IfOutBroadcastPkts
	ethernet_frame_pm->broadcast_packets = counter_port.ifInBroadcastPkts;	
	// etherStatsMcastPkts - STAT_IfOutMulticastPkts
	ethernet_frame_pm->multicast_packets = counter_port.ifInMulticastPkts;
	ethernet_frame_pm->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;  
	ethernet_frame_pm->undersize_packets = counter_port.etherStatsUndersizePkts;
	ethernet_frame_pm->fragments = counter_port.etherStatsFragments;

	// T-REC-G.984.4-200802-I!!PDF-E.pdf
	// 9.5.4 Ethernet performance monitoring history data 3
	// Jabbers: The total number of packets received that were longer than 1518 octets, excluding framing bits but including FCS octets, and had either a bad frame check sequence (FCS) with an integral number of octets (FCS error) or a bad FCS with a non-integral number of octets (alignment error). The range to detect jabber is between 20 ms and 150 ms. (R) (mandatory) (4 bytes)

	// RFC 2819 etherStatsJabbers description: 
	//   The total number of packets received that were longer than 1518 octets (excluding framing bits, but including FCS octets), and had either a bad Frame Check Sequence (FCS) with an integral number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).
	//
	// Note that this definition of jabber is different than the definition in IEEE-802.3 section 8.2.1.5 (10BASE5) and section 10.3.1.4 (10BASE2).  These documents define jabber as the condition where any packet exceeds 20 ms.  The allowed range to detect jabber is between 20 ms and 150 ms.

	// RFC 2819 etherStatsOversizePkts description: 
	//   The total number of packets received that were longer than 1518 octets (excluding framing bits, but including FCS octets) and were otherwise well formed.
	ethernet_frame_pm->jabbers = counter_port.etherStatsJabbers;
	ethernet_frame_pm->packets_64_octets = counter_port.etherStatsRxPkts64Octets;
	ethernet_frame_pm->packets_65_to_127_octets = counter_port.etherStatsRxPkts65to127Octets;
	ethernet_frame_pm->packets_128_to_255_octets = counter_port.etherStatsRxPkts128to255Octets;
	ethernet_frame_pm->packets_256_to_511_octets = counter_port.etherStatsRxPkts256to511Octets;
	ethernet_frame_pm->packets_512_to_1023_octets = counter_port.etherStatsRxPkts512to1023Octets;
	ethernet_frame_pm->packets_1024_to_1518_octets = counter_port.etherStatsRxPkts1024to1518Octets;

	return ret;
}

// this me get counters in ingress/egress directory
int 
switch_hw_prx126_eth_pm_data2_private(unsigned int port_id_in, struct switch_eth_pm_data2_private_t *pm_data2_private)
{
	//65346 omci_me_ethernet_performance_monitoring_histroy_data_2_private 9.99.25
	/*
	DATA_2_PRIVATE			RTL8305

	OUTOctets;			//"IfOutOctets",
	OUTBroadcastpackets;		//"IfOutBastPkts",
	OUTMulticastpackets;		//"IfOutMcastPkts",
	OUTunicastpackets;		//"IfOutUcastPkts",
	INUnicastpackets;		//"IfInUcastPkts",
	//UPSPEED;			//"IfInOctets/sec"
	//DOWNSPEED;			//"IfOutOctets/sec"
	UNIPackets1519octets;		//not support
	INOctets;			//"IfInOctets",
	*/

	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}
#if 0
	if (switch_get_wan_logical_portid() == port_id) {
		pm_data2_private->outoctets = counter_port.ifInOctets;
		pm_data2_private->outbroadcastpackets = counter_port.ifInBroadcastPkts;
		pm_data2_private->outmulticastpackets = counter_port.ifInMulticastPkts;
		pm_data2_private->outunicastpackets = counter_port.ifInUcastPkts;
		pm_data2_private->inunicastpackets = counter_port.ifInUcastPkts;
		// FIXME! STEVE- use etherStatsRxPkts1519toMaxOctets???
		pm_data2_private->unipackets1519octets = counter_port.etherStatsTxPkts1519toMaxOctets;
		pm_data2_private->inoctets = counter_port.ifOutOctets;
		return ret;
	}
#endif
	pm_data2_private->outoctets = counter_port.ifOutOctets;
	pm_data2_private->outbroadcastpackets = counter_port.ifOutBroadcastPkts;
	pm_data2_private->outmulticastpackets = counter_port.ifOutMulticastPkts;
	pm_data2_private->outunicastpackets = counter_port.ifOutUcastPkts;
	pm_data2_private->inunicastpackets = counter_port.ifInUcastPkts;
	// FIXME! STEVE- use etherStatsRxPkts1519toMaxOctets??? 
	pm_data2_private->unipackets1519octets = counter_port.etherStatsRxPkts1519toMaxOctets;
	pm_data2_private->inoctets = counter_port.ifInOctets;

	return ret;
}

int 
switch_hw_prx126_eth_pm_data_upstream(unsigned int port_id_in, struct switch_ethernet_frame_pm_ud_t *ethernet_frame_pmu)
{
	/*	G984.4					RTL8305
	// 322 ethernet_frame_pm_upstream 9.3.30        // bridge port us (rx) In

	drop_events;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	octets;				//	"ifInOctets" use bytes, and only care valid frames
	packets;			//	"ifInUcastPkts" + broadcast_packets + multicast_packets
	broadcast_packets;		//	"STAT_IfInBroadcastPkts",
	multicast_packets;		//	"STAT_IfInMulticastPkts",
	crc_errored_packets;		//	not support (not use in eth_pm_data3, but in pm up/down stream)
	undersize_packets;		//	"STAT_InEtherStatsUnderSizePkts",
	oversize_packets;		//	"STAT_InEtherOversizeStats",
	packets_64_octets;		//	"STAT_InEtherStatsPkts64Octets",
	packets_65_to_127_octets;	//	"STAT_InEtherStatsPkts65to127Octets",
	packets_128_to_255_octets;	//	"STAT_InEtherStatsPkts128to255Octets",
	packets_256_to_511_octets;	//	"STAT_InEtherStatsPkts256to511Octets",
	packets_512_to_1023_octets;	//	"STAT_InEtherStatsPkts512to1023Octets",
	packets_1024_to_1518_octets;	//	"STAT_InEtherStatsPkts1024to1518Octets"
	*/
	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_frame_pmu->drop_events = counter_port.etherStatsDropEvents;
		ethernet_frame_pmu->octets = counter_port.ifOutOctets;
		//"ifOutUcastPkts" + broadcast + multicast
		ethernet_frame_pmu->packets = counter_port.ifOutUcastPkts + counter_port.ifOutMulticastPkts + counter_port.ifOutBroadcastPkts;
		ethernet_frame_pmu->broadcast_packets = counter_port.ifOutBroadcastPkts;
		ethernet_frame_pmu->multicast_packets = counter_port.ifOutMulticastPkts;

		// RFC 2819 etherStatsCRCAlignErrors description:
		// The total number of packets received that had a length (excluding framing bits, but including FCS octets)
		// of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral
		// number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).

		// T-REC-G.984.4-200906-I!Amd1!PDF-E.pdf
		// 9.3.30 Ethernet frame performance monitoring history data upstream
		//
		// CRC errored packets: The total number of packets received that had a length (excluding framing bits, but including FCS octets) of
		// between 64 and 1518 octets, inclusive, but had either a bad frame check sequence (FCS) with an integral number of octets (FCS error)
		// or a bad FCS with a non-integral number of octets (alignment error). (R) (mandatory) (4 bytes)

		ethernet_frame_pmu->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
		ethernet_frame_pmu->undersize_packets = counter_port.etherStatsTxUndersizePkts;
		ethernet_frame_pmu->oversize_packets = counter_port.etherStatsTxOversizePkts;
		ethernet_frame_pmu->packets_64_octets = counter_port.etherStatsTxPkts64Octets;
		ethernet_frame_pmu->packets_65_to_127_octets = counter_port.etherStatsTxPkts65to127Octets;
		ethernet_frame_pmu->packets_128_to_255_octets = counter_port.etherStatsTxPkts128to255Octets;
		ethernet_frame_pmu->packets_256_to_511_octets = counter_port.etherStatsTxPkts256to511Octets;
		ethernet_frame_pmu->packets_512_to_1023_octets = counter_port.etherStatsTxPkts512to1023Octets;
		ethernet_frame_pmu->packets_1024_to_1518_octets = counter_port.etherStatsTxPkts1024to1518Octets;

		return ret;
	}
	ethernet_frame_pmu->drop_events = counter_port.etherStatsDropEvents;
	ethernet_frame_pmu->octets = counter_port.ifInOctets;
	//"ifInUcastPkts" + broadcast + multicast
	ethernet_frame_pmu->packets = counter_port.ifInUcastPkts + counter_port.ifInMulticastPkts + counter_port.ifInBroadcastPkts;
	ethernet_frame_pmu->broadcast_packets = counter_port.ifInBroadcastPkts;
	ethernet_frame_pmu->multicast_packets = counter_port.ifInMulticastPkts;

	// RFC 2819 etherStatsCRCAlignErrors description:
	// The total number of packets received that had a length (excluding framing bits, but including FCS octets) 
	// of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral 
	// number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).

	// T-REC-G.984.4-200906-I!Amd1!PDF-E.pdf 
	// 9.3.30 Ethernet frame performance monitoring history data upstream
	//
	// CRC errored packets: The total number of packets received that had a length (excluding framing bits, but including FCS octets) of 
	// between 64 and 1518 octets, inclusive, but had either a bad frame check sequence (FCS) with an integral number of octets (FCS error) 
	// or a bad FCS with a non-integral number of octets (alignment error). (R) (mandatory) (4 bytes)
	
	ethernet_frame_pmu->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
	ethernet_frame_pmu->undersize_packets = counter_port.etherStatsRxUndersizePkts;
	ethernet_frame_pmu->oversize_packets = counter_port.etherStatsRxOversizePkts;
	ethernet_frame_pmu->packets_64_octets = counter_port.etherStatsRxPkts64Octets;
	ethernet_frame_pmu->packets_65_to_127_octets = counter_port.etherStatsRxPkts65to127Octets;
	ethernet_frame_pmu->packets_128_to_255_octets = counter_port.etherStatsRxPkts128to255Octets;
	ethernet_frame_pmu->packets_256_to_511_octets = counter_port.etherStatsRxPkts256to511Octets;
	ethernet_frame_pmu->packets_512_to_1023_octets = counter_port.etherStatsRxPkts512to1023Octets;
	ethernet_frame_pmu->packets_1024_to_1518_octets = counter_port.etherStatsRxPkts1024to1518Octets;

	// dbprintf(LOG_ERR, "----LEO---- wanport=%d, portid=%d, octets=%llu\n",
	// 	switch_get_wan_logical_portid(), port_id, ethernet_frame_pmu->octets);

	return ret;
}

int 
switch_hw_prx126_eth_pm_data_upstream_long(unsigned int port_id_in, struct switch_ethernet_frame_pm_ud_long_t *ethernet_frame_pmu)
{
	/*	G984.4					RTL8305
	// 322 ethernet_frame_pm_upstream 9.3.30        // bridge port us (rx) In

	drop_events;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	octets;				//	"ifInOctets" use bytes, and only care valid frames
	packets;			//	"ifInUcastPkts" + broadcast_packets + multicast_packets
	broadcast_packets;		//	"STAT_IfInBroadcastPkts",
	multicast_packets;		//	"STAT_IfInMulticastPkts",
	crc_errored_packets;		//	not support (not use in eth_pm_data3, but in pm up/down stream)
	undersize_packets;		//	"STAT_InEtherStatsUnderSizePkts",
	oversize_packets;		//	"STAT_InEtherOversizeStats",
	packets_64_octets;		//	"STAT_InEtherStatsPkts64Octets",
	packets_65_to_127_octets;	//	"STAT_InEtherStatsPkts65to127Octets",
	packets_128_to_255_octets;	//	"STAT_InEtherStatsPkts128to255Octets",
	packets_256_to_511_octets;	//	"STAT_InEtherStatsPkts256to511Octets",
	packets_512_to_1023_octets;	//	"STAT_InEtherStatsPkts512to1023Octets",
	packets_1024_to_1518_octets;	//	"STAT_InEtherStatsPkts1024to1518Octets"
	*/
	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);
	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;
	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_frame_pmu->drop_events = counter_port.etherStatsDropEvents;
		ethernet_frame_pmu->octets = counter_port.ifOutOctets;
		//"ifOutUcastPkts" + broadcast + multicast
		ethernet_frame_pmu->packets = counter_port.ifOutUcastPkts + counter_port.ifOutMulticastPkts + counter_port.ifOutBroadcastPkts;
		ethernet_frame_pmu->broadcast_packets = counter_port.ifOutBroadcastPkts;
		ethernet_frame_pmu->multicast_packets = counter_port.ifOutMulticastPkts;

		// RFC 2819 etherStatsCRCAlignErrors description:
		// The total number of packets received that had a length (excluding framing bits, but including FCS octets)
		// of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral
		// number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).

		// T-REC-G.984.4-200906-I!Amd1!PDF-E.pdf
		// 9.3.30 Ethernet frame performance monitoring history data upstream
		//
		// CRC errored packets: The total number of packets received that had a length (excluding framing bits, but including FCS octets) of
		// between 64 and 1518 octets, inclusive, but had either a bad frame check sequence (FCS) with an integral number of octets (FCS error)
		// or a bad FCS with a non-integral number of octets (alignment error). (R) (mandatory) (4 bytes)

		ethernet_frame_pmu->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
		ethernet_frame_pmu->undersize_packets = counter_port.etherStatsTxUndersizePkts;
		ethernet_frame_pmu->oversize_packets = counter_port.etherStatsTxOversizePkts;
		ethernet_frame_pmu->packets_64_octets = counter_port.etherStatsTxPkts64Octets;
		ethernet_frame_pmu->packets_65_to_127_octets = counter_port.etherStatsTxPkts65to127Octets;
		ethernet_frame_pmu->packets_128_to_255_octets = counter_port.etherStatsTxPkts128to255Octets;
		ethernet_frame_pmu->packets_256_to_511_octets = counter_port.etherStatsTxPkts256to511Octets;
		ethernet_frame_pmu->packets_512_to_1023_octets = counter_port.etherStatsTxPkts512to1023Octets;
		ethernet_frame_pmu->packets_1024_to_1518_octets = counter_port.etherStatsTxPkts1024to1518Octets;
		return ret;
	}
	ethernet_frame_pmu->drop_events = counter_port.etherStatsDropEvents;
	ethernet_frame_pmu->octets = counter_port.ifInOctets;
	//"ifInUcastPkts" + broadcast + multicast
	ethernet_frame_pmu->packets = counter_port.ifInUcastPkts + counter_port.ifInMulticastPkts + counter_port.ifInBroadcastPkts;
	ethernet_frame_pmu->broadcast_packets = counter_port.ifInBroadcastPkts;
	ethernet_frame_pmu->multicast_packets = counter_port.ifInMulticastPkts;

	// RFC 2819 etherStatsCRCAlignErrors description:
	// The total number of packets received that had a length (excluding framing bits, but including FCS octets) 
	// of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral 
	// number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).

	// T-REC-G.984.4-200906-I!Amd1!PDF-E.pdf 
	// 9.3.30 Ethernet frame performance monitoring history data upstream
	//
	// CRC errored packets: The total number of packets received that had a length (excluding framing bits, but including FCS octets) of 
	// between 64 and 1518 octets, inclusive, but had either a bad frame check sequence (FCS) with an integral number of octets (FCS error) 
	// or a bad FCS with a non-integral number of octets (alignment error). (R) (mandatory) (4 bytes)
	
	ethernet_frame_pmu->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
	ethernet_frame_pmu->undersize_packets = counter_port.etherStatsRxUndersizePkts;
	ethernet_frame_pmu->oversize_packets = counter_port.etherStatsRxOversizePkts;
	ethernet_frame_pmu->packets_64_octets = counter_port.etherStatsRxPkts64Octets;
	ethernet_frame_pmu->packets_65_to_127_octets = counter_port.etherStatsRxPkts65to127Octets;
	ethernet_frame_pmu->packets_128_to_255_octets = counter_port.etherStatsRxPkts128to255Octets;
	ethernet_frame_pmu->packets_256_to_511_octets = counter_port.etherStatsRxPkts256to511Octets;
	ethernet_frame_pmu->packets_512_to_1023_octets = counter_port.etherStatsRxPkts512to1023Octets;
	ethernet_frame_pmu->packets_1024_to_1518_octets = counter_port.etherStatsRxPkts1024to1518Octets;

	return ret;
}

int 
switch_hw_prx126_eth_pm_data_downstream(unsigned int port_id_in, struct switch_ethernet_frame_pm_ud_t *ethernet_frame_pmd)
{
	/*	G984.4					RTL8305
	// 321 ethernet_frame_pm_downstream 9.3.31      // bridge port ds (tx) Out

	drop_events;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	octets;				//	"ifOutOctets" use bytes, and only care valid frames
	packets;			//	"ifOutUcastPkts" + broadcast_packets + multicast_packets
	broadcast_packets;		//	"STAT_IfOutBroadcastPkts",
	multicast_packets;		//	"STAT_IfOutMulticastPkts",
	crc_errored_packets;		//	not support (not use in eth_pm_data3, but in pm up/down stream)
	undersize_packets;		//	"STAT_OutEtherStatsUnderSizePkts",
	oversize_packets;		//	"STAT_OutEtherOversizeStats",
	packets_64_octets;		//	"STAT_OutEtherStatsPkts64Octets",
	packets_65_to_127_octets;	//	"STAT_OutEtherStatsPkts65to127Octets",
	packets_128_to_255_octets;	//	"STAT_OutEtherStatsPkts128to255Octets",
	packets_256_to_511_octets;	//	"STAT_OutEtherStatsPkts256to511Octets",
	packets_512_to_1023_octets;	//	"STAT_OutEtherStatsPkts512to1023Octets",
	packets_1024_to_1518_octets;	//	"STAT_OutEtherStatsPkts1024to1518Octets"
	*/
	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);

	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;

	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_frame_pmd->drop_events = counter_port.etherStatsDropEvents;
		ethernet_frame_pmd->octets = counter_port.ifInOctets;
		ethernet_frame_pmd->packets = counter_port.ifInUcastPkts + counter_port.ifInBroadcastPkts + counter_port.ifInMulticastPkts;
		ethernet_frame_pmd->broadcast_packets = counter_port.ifInBroadcastPkts;
		ethernet_frame_pmd->multicast_packets = counter_port.ifInMulticastPkts;
		ethernet_frame_pmd->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
		ethernet_frame_pmd->undersize_packets = counter_port.etherStatsRxUndersizePkts;
		ethernet_frame_pmd->oversize_packets = counter_port.etherStatsRxOversizePkts;
		ethernet_frame_pmd->packets_64_octets = counter_port.etherStatsRxPkts64Octets;
		ethernet_frame_pmd->packets_65_to_127_octets = counter_port.etherStatsRxPkts65to127Octets;
		ethernet_frame_pmd->packets_128_to_255_octets = counter_port.etherStatsRxPkts128to255Octets;
		ethernet_frame_pmd->packets_256_to_511_octets = counter_port.etherStatsRxPkts256to511Octets;
		ethernet_frame_pmd->packets_512_to_1023_octets = counter_port.etherStatsRxPkts512to1023Octets;
		ethernet_frame_pmd->packets_1024_to_1518_octets = counter_port.etherStatsRxPkts1024to1518Octets;

		// dbprintf(LOG_ERR, "----LEO---- wanport=%d, portid=%d, octets=%llu\n", 
		// 	switch_get_wan_logical_portid(), port_id, ethernet_frame_pmd->octets);

		return ret;
	}
	ethernet_frame_pmd->drop_events = counter_port.etherStatsDropEvents;
	ethernet_frame_pmd->octets = counter_port.ifOutOctets;
	ethernet_frame_pmd->packets = counter_port.ifOutUcastPkts + counter_port.ifOutBroadcastPkts + counter_port.ifOutMulticastPkts;
	ethernet_frame_pmd->broadcast_packets = counter_port.ifOutBroadcastPkts;
	ethernet_frame_pmd->multicast_packets = counter_port.ifOutMulticastPkts;
	ethernet_frame_pmd->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
	ethernet_frame_pmd->undersize_packets = counter_port.etherStatsTxUndersizePkts;
	ethernet_frame_pmd->oversize_packets = counter_port.etherStatsTxOversizePkts;
	ethernet_frame_pmd->packets_64_octets = counter_port.etherStatsTxPkts64Octets;
	ethernet_frame_pmd->packets_65_to_127_octets = counter_port.etherStatsTxPkts65to127Octets;
	ethernet_frame_pmd->packets_128_to_255_octets = counter_port.etherStatsTxPkts128to255Octets;
	ethernet_frame_pmd->packets_256_to_511_octets = counter_port.etherStatsTxPkts256to511Octets;
	ethernet_frame_pmd->packets_512_to_1023_octets = counter_port.etherStatsTxPkts512to1023Octets;
	ethernet_frame_pmd->packets_1024_to_1518_octets = counter_port.etherStatsTxPkts1024to1518Octets;

	// dbprintf(LOG_ERR, "----LEO---- wanport=%d, portid=%d, octets=%llu\n", 
	// 	switch_get_wan_logical_portid(), port_id, ethernet_frame_pmd->octets);

	return ret;
}

int 
switch_hw_prx126_eth_pm_data_downstream_long(unsigned int port_id_in, struct switch_ethernet_frame_pm_ud_long_t *ethernet_frame_pmd)
{
	/*	G984.4					RTL8305
	// 321 ethernet_frame_pm_downstream 9.3.31      // bridge port ds (tx) Out

	drop_events;			//	"etherStatsDropEvents",(number of packets in RTL8305)
	octets;				//	"ifOutOctets" use bytes, and only care valid frames
	packets;			//	"ifOutUcastPkts" + broadcast_packets + multicast_packets
	broadcast_packets;		//	"STAT_IfOutBroadcastPkts",
	multicast_packets;		//	"STAT_IfOutMulticastPkts",
	crc_errored_packets;		//	not support (not use in eth_pm_data3, but in pm up/down stream)
	undersize_packets;		//	"STAT_OutEtherStatsUnderSizePkts",
	oversize_packets;		//	"STAT_OutEtherOversizeStats",
	packets_64_octets;		//	"STAT_OutEtherStatsPkts64Octets",
	packets_65_to_127_octets;	//	"STAT_OutEtherStatsPkts65to127Octets",
	packets_128_to_255_octets;	//	"STAT_OutEtherStatsPkts128to255Octets",
	packets_256_to_511_octets;	//	"STAT_OutEtherStatsPkts256to511Octets",
	packets_512_to_1023_octets;	//	"STAT_OutEtherStatsPkts512to1023Octets",
	packets_1024_to_1518_octets;	//	"STAT_OutEtherStatsPkts1024to1518Octets"
	*/
	struct switch_counter_port_t counter_port;
	int ret = 0;
	int port_id;

	switch_hw_prx126_pm_refresh(0);

	if (switch_get_cpu_logical_portid(0) == port_id_in)
		port_id=switch_get_wan_logical_portid();
	else
		port_id=port_id_in;

	memset(&counter_port, 0, sizeof(counter_port));
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	if (switch_get_wan_logical_portid() == port_id) {
		ethernet_frame_pmd->drop_events = counter_port.etherStatsDropEvents;
		ethernet_frame_pmd->octets = counter_port.ifInOctets;
		ethernet_frame_pmd->packets = counter_port.ifInUcastPkts + counter_port.ifInBroadcastPkts + counter_port.ifInMulticastPkts;
		ethernet_frame_pmd->broadcast_packets = counter_port.ifInBroadcastPkts;
		ethernet_frame_pmd->multicast_packets = counter_port.ifInMulticastPkts;
		ethernet_frame_pmd->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
		ethernet_frame_pmd->undersize_packets = counter_port.etherStatsRxUndersizePkts;
		ethernet_frame_pmd->oversize_packets = counter_port.etherStatsRxOversizePkts;
		ethernet_frame_pmd->packets_64_octets = counter_port.etherStatsRxPkts64Octets;
		ethernet_frame_pmd->packets_65_to_127_octets = counter_port.etherStatsRxPkts65to127Octets;
		ethernet_frame_pmd->packets_128_to_255_octets = counter_port.etherStatsRxPkts128to255Octets;
		ethernet_frame_pmd->packets_256_to_511_octets = counter_port.etherStatsRxPkts256to511Octets;
		ethernet_frame_pmd->packets_512_to_1023_octets = counter_port.etherStatsRxPkts512to1023Octets;
		ethernet_frame_pmd->packets_1024_to_1518_octets = counter_port.etherStatsRxPkts1024to1518Octets;
		return ret;
	}
	ethernet_frame_pmd->drop_events = counter_port.etherStatsDropEvents;
	ethernet_frame_pmd->octets = counter_port.ifOutOctets;
	ethernet_frame_pmd->packets = counter_port.ifOutUcastPkts + counter_port.ifOutBroadcastPkts + counter_port.ifOutMulticastPkts;
	ethernet_frame_pmd->broadcast_packets = counter_port.ifOutBroadcastPkts;
	ethernet_frame_pmd->multicast_packets = counter_port.ifOutMulticastPkts;
	ethernet_frame_pmd->crc_errored_packets = counter_port.etherStatsCRCAlignErrors;
	ethernet_frame_pmd->undersize_packets = counter_port.etherStatsTxUndersizePkts;
	ethernet_frame_pmd->oversize_packets = counter_port.etherStatsTxOversizePkts;
	ethernet_frame_pmd->packets_64_octets = counter_port.etherStatsTxPkts64Octets;
	ethernet_frame_pmd->packets_65_to_127_octets = counter_port.etherStatsTxPkts65to127Octets;
	ethernet_frame_pmd->packets_128_to_255_octets = counter_port.etherStatsTxPkts128to255Octets;
	ethernet_frame_pmd->packets_256_to_511_octets = counter_port.etherStatsTxPkts256to511Octets;
	ethernet_frame_pmd->packets_512_to_1023_octets = counter_port.etherStatsTxPkts512to1023Octets;
	ethernet_frame_pmd->packets_1024_to_1518_octets = counter_port.etherStatsTxPkts1024to1518Octets;

	return ret;
}

// the rate is calculated based on the time/counter difference between two get operation
// so the result will become the average of the period between two get
int 
switch_hw_prx126_eth_pm_rate_status_get(int port_id, unsigned long long *rate_us, unsigned long long *rate_ds)
{
	static unsigned long long prev_ifInOctets[SWITCH_LOGICAL_PORT_TOTAL] = {0};
	static unsigned long long prev_ifOutOctets[SWITCH_LOGICAL_PORT_TOTAL] = {0};
	static struct timeval prev_tv[SWITCH_LOGICAL_PORT_TOTAL];

	struct switch_counter_port_t counter_port;
	struct timeval tv;
	unsigned long msec;
	int ret = 0;

	if (port_id >= SWITCH_LOGICAL_PORT_TOTAL) {
		*rate_us = *rate_ds = 0;
		dbprintf(LOG_ERR, "switch_hw_fvt2510_pm_port_get port %d >= SWITCH_LOGICAL_PORT_TOTAL(%d)\n", port_id, SWITCH_LOGICAL_PORT_TOTAL);
		return -1;
	}

	memset(&counter_port, 0, sizeof(counter_port));
	
	switch_hw_prx126_pm_refresh(0);
	util_get_uptime(&tv);
	if ((ret = switch_hw_prx126_pm_port_get(port_id, &counter_port)) != 0) {
		dbprintf(LOG_ERR, "switch_hw_prx126_pm_port_get error %x for port %d\n", ret, port_id);
		return ret;
	}

	msec = (unsigned long)util_timeval_diff_msec(&tv, &prev_tv[port_id]);
	if (msec == 0) msec = 1;	
	*rate_us = *rate_ds = 0;
	if (switch_get_wan_logical_portid() == port_id ) {	// wan: in=ds, out=ds
		if (counter_port.ifInOctets >= prev_ifInOctets[port_id])
			*rate_ds = (counter_port.ifInOctets-prev_ifInOctets[port_id])*8ULL *1000/msec;
		if (counter_port.ifOutOctets >= prev_ifOutOctets[port_id])
			*rate_us = (counter_port.ifOutOctets-prev_ifOutOctets[port_id])*8ULL *1000/msec;
	} else {						// uni: in=us, out=ds
		if (counter_port.ifInOctets >= prev_ifInOctets[port_id])
			*rate_us = (counter_port.ifInOctets-prev_ifInOctets[port_id])*8ULL *1000/msec;
		if (counter_port.ifOutOctets >= prev_ifOutOctets[port_id])
			*rate_ds = (counter_port.ifOutOctets-prev_ifOutOctets[port_id])*8ULL *1000/msec;
	}
	
	// keep counter & time in prev[]
	prev_ifInOctets[port_id] = counter_port.ifInOctets;
	prev_ifOutOctets[port_id] = counter_port.ifOutOctets;
	prev_tv[port_id] = tv;
	
	return ret;
}
