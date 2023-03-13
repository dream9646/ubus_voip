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
 * Module  : batchtab
 * File    : batchtab_cb.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "meinfo.h"
#include "util.h"
#include "list.h"
#include "fwk_mutex.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_alu.h"
#include "proprietary_calix.h"
#include "proprietary_ericsson.h"
#include "me_related.h"

// tm protect will be started in the start of tm, td, gemflow, classf hwsync
// but it will be ended only in the end of classf hwsync
// the batchtab relation are tm -> td -> gemflow -> classf
                                   
static int tm_protect_is_started = 0;
static unsigned int tm_protect_state;
static struct fwk_mutex_t tm_protect_mutex;
static struct timeval tv_start;

void
batchtab_cb_tm_protect_start(void)
{
	if (omci_env_g.tm_protect_enable) {
		// stop input & clear pq before making change to tm related hw
		fwk_mutex_lock(&tm_protect_mutex);
		if (!tm_protect_is_started) {			
			tm_protect_is_started = 1;
			if (omci_env_g.debug_level_bat >= LOG_WARNING) {
				util_get_uptime(&tv_start);
				dbprintf_bat(LOG_WARNING, "tm_protect start(start=%d.%06d)\n", tv_start.tv_sec, tv_start.tv_usec);
			}
			gpon_hw_g.util_protect_start(&tm_protect_state);
		}
		fwk_mutex_unlock(&tm_protect_mutex);
		if (gpon_hw_g.util_clearpq)
			gpon_hw_g.util_clearpq(0);
	}
}

void
batchtab_cb_tm_protect_stop(void)
{
	if (omci_env_g.tm_protect_enable) {
		// restore change to tm related hw
		fwk_mutex_lock(&tm_protect_mutex);
		if (tm_protect_is_started) {			
			gpon_hw_g.util_protect_end(tm_protect_state);
			if (omci_env_g.debug_level_bat >= LOG_WARNING) {
				struct timeval tv_end;
				util_get_uptime(&tv_end);		
				dbprintf_bat(LOG_WARNING, "tm_protect takes %d ms(start=%d.%06d, end=%d.%06d)\n", 
					(int)util_timeval_diff_usec(&tv_end, &tv_start)/1000, 
					tv_start.tv_sec, tv_start.tv_usec, tv_end.tv_sec, tv_end.tv_usec);
			}
			tm_protect_is_started = 0;
		}
		fwk_mutex_unlock(&tm_protect_mutex);
	}
}

int
batchtab_cb_init(void)
{
	batchtab_cb_filtering_register();
	batchtab_cb_tagging_register();

	fwk_mutex_create(&tm_protect_mutex);	
	batchtab_cb_tm_register();
	batchtab_cb_gemflow_register();
	batchtab_cb_classf_register();
	batchtab_cb_td_register();
	
	batchtab_cb_wan_register();
	batchtab_cb_isolation_set_register();
	batchtab_cb_mac_filter_register();

	batchtab_cb_linkready_register();
	batchtab_cb_wanif_register();
	batchtab_cb_autouni_register();
	batchtab_cb_autoveip_register();
	batchtab_cb_mcastmode_register();
	batchtab_cb_mcastbw_register();
	batchtab_cb_mcastconf_register();
	if (omci_env_g.voip_enable != ENV_VOIP_DISABLE)
		batchtab_cb_pots_register();
	batchtab_cb_lldp_register();

	batchtab_cb_hardbridge_register();

	if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
	   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
		//batchtab_cb_alu_tls_register();
		batchtab_cb_alu_sp_register();
		batchtab_cb_alu_ds_mcastvlan_register();
		batchtab_cb_alu_ont_mode_register();
		batchtab_cb_alu_tlsv6_register();
	}
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		batchtab_cb_calix_tlan_register();
		batchtab_cb_calix_l2cp_register();
		batchtab_cb_calix_apas_register();
		batchtab_cb_calix_uni_policing_register();
		batchtab_cb_wand2p_register();
	}
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) {
		batchtab_cb_ericsson_pvlan_register();
	}
	
	return 0;
}

int
batchtab_cb_finish(void)
{
	batchtab_unregister("filtering"); 
	batchtab_unregister("tagging"); 

	batchtab_unregister("tm"); 
	batchtab_unregister("gemflow"); 
	batchtab_unregister("classf"); 
	batchtab_unregister("td"); 
	fwk_mutex_destroy(&tm_protect_mutex);	

	batchtab_unregister("wan"); 
	batchtab_unregister("isolation_set"); 
	batchtab_unregister("mac_filter"); 

	batchtab_unregister("linkready"); 
	batchtab_unregister("wanif"); 
	batchtab_unregister("autouni"); 
	batchtab_unregister("autoveip"); 
	batchtab_unregister("mcastmode"); 
	batchtab_unregister("mcastbw");
	batchtab_unregister("mcastconf");
	if (omci_env_g.voip_enable != ENV_VOIP_DISABLE)
		batchtab_unregister("pots_mkey");
	batchtab_unregister("lldp");

	batchtab_unregister("hardbridge");

	if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
	   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
		//batchtab_unregister("alu_tls");
		batchtab_unregister("alu_sp");
		batchtab_unregister("alu_ds_mcastvlan");
		batchtab_unregister("alu_ont_mode");
		batchtab_unregister("tlsv6");
	}
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		batchtab_unregister("calix_tlan");
		batchtab_unregister("calix_l2cp");
		batchtab_unregister("calix_apas");
		batchtab_unregister("wand2p");
	}
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) {
		batchtab_unregister("ericsson_pvlan");
	}
	
	return 0;
}
