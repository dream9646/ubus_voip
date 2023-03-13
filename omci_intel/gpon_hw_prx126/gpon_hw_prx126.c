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
 * File    : gpon_hw_prx126.c
 *
 ******************************************************************/

#include "util.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"

static int
gpon_hw_prx126_init(void)
{
	// leave this to board_init.sh
	//rtk_ponmac_init();
	//rtk_gpon_initial(1);

	// leave this to end of omci_start
	//rtk_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
	
	gpon_hw_prx126_history_init();
	
	return 0;
}

static int
gpon_hw_prx126_shutdown(void)
{
	// skip below 2
	//rtk_gpon_deActivate();
	//rtk_gpon_deinitial();

	return 0;
}

////////////////////////////////////////////////////////////////////
struct gpon_hw_t gpon_hw_fvt_g={
	.init = gpon_hw_prx126_init,
	.shutdown = gpon_hw_prx126_shutdown,
	/* gpon_hw_prx126_i2c.c */
	.i2c_open =  gpon_hw_prx126_i2c_open,
	.i2c_read =  gpon_hw_prx126_i2c_read,
	.i2c_write =  gpon_hw_prx126_i2c_write,
	.i2c_close =  gpon_hw_prx126_i2c_close,
	.eeprom_open = gpon_hw_prx126_eeprom_open,
	.eeprom_data_get = gpon_hw_prx126_eeprom_data_get,
	.eeprom_data_set = gpon_hw_prx126_eeprom_data_set,
	
	/* gpon_hw_prx126_onu.c */
	.onu_mgmt_mode_get = gpon_hw_prx126_onu_mgmt_mode_get,
	.onu_mgmt_mode_set = gpon_hw_prx126_onu_mgmt_mode_set,
	.onu_serial_number_get = gpon_hw_prx126_onu_serial_number_get,
	.onu_serial_number_set = gpon_hw_prx126_onu_serial_number_set,
	.onu_password_get = gpon_hw_prx126_onu_password_get,
	.onu_password_set = gpon_hw_prx126_onu_password_set,	
	.onu_activate = gpon_hw_prx126_onu_activate,
	.onu_deactivate = gpon_hw_prx126_onu_deactivate, 
	.onu_state_get = gpon_hw_prx126_onu_state_get, 
	.onu_id_get = gpon_hw_prx126_onu_id_get, 
	.onu_alarm_event_get = gpon_hw_prx126_onu_alarm_event_get,
	.onu_gem_blocksize_get = gpon_hw_prx126_onu_gem_blocksize_get,
	.onu_gem_blocksize_set = gpon_hw_prx126_onu_gem_blocksize_set,
	.onu_sdsf_get = gpon_hw_prx126_onu_sdsf_get,	
	.onu_superframe_seq_get = gpon_hw_prx126_onu_superframe_seq_get,	
	/* gpon_hw_prx126_tm.c */
	.tm_pq_config_get = gpon_hw_prx126_tm_pq_config_get,
	.tm_pq_config_set = gpon_hw_prx126_tm_pq_config_set,
	.tm_pq_config_del = gpon_hw_prx126_tm_pq_config_del,
	.tm_ts_config_get = gpon_hw_prx126_tm_ts_config_get,
	.tm_ts_config_set = gpon_hw_prx126_tm_ts_config_set,
	.tm_tcont_config_get = gpon_hw_prx126_tm_tcont_config_get,
	.tm_tcont_config_set = gpon_hw_prx126_tm_tcont_config_set,
	.tm_tcont_config_del = gpon_hw_prx126_tm_tcont_config_del,
	.tm_tcont_add_by_allocid = gpon_hw_prx126_tm_tcont_add_by_allocid, 
	.tm_tcont_get_by_allocid = gpon_hw_prx126_tm_tcont_get_by_allocid, 
	.tm_tcont_del_by_allocid = gpon_hw_prx126_tm_tcont_del_by_allocid, 
	.tm_dsflow_get = gpon_hw_prx126_tm_dsflow_get,
	.tm_dsflow_set = gpon_hw_prx126_tm_dsflow_set,
	.tm_dsflow_del = gpon_hw_prx126_tm_dsflow_del,
	.tm_usflow_get = gpon_hw_prx126_tm_usflow_get,
	.tm_usflow_set = gpon_hw_prx126_tm_usflow_set,
	.tm_usflow_del = gpon_hw_prx126_tm_usflow_del,
	.tm_usflow2pq_get = gpon_hw_prx126_tm_usflow2pq_get,
	.tm_usflow2pq_set = gpon_hw_prx126_tm_usflow2pq_set,
	.tm_flow_8021p_set = gpon_hw_prx126_tm_flow_8021p_set,
	.tm_flow_iwtp_set  = gpon_hw_prx126_tm_flow_iwtp_set,
	.tm_flow_mcastiwtp_set = gpon_hw_prx126_tm_flow_mcastiwtp_set,
	/* gpon_hw_prx126_pm.c */
	.counter_global_get = gpon_hw_prx126_counter_global_get,
	.counter_tcont_get = gpon_hw_prx126_counter_tcont_get,
	.counter_dsflow_get = gpon_hw_prx126_counter_dsflow_get,
	.counter_usflow_get = gpon_hw_prx126_counter_usflow_get,
	.pm_refresh = gpon_hw_prx126_pm_refresh,
	.pm_global_get = gpon_hw_prx126_pm_global_get,
	.pm_tcont_get = gpon_hw_prx126_pm_tcont_get,
	.pm_dsflow_get = gpon_hw_prx126_pm_dsflow_get,
	.pm_usflow_get = gpon_hw_prx126_pm_usflow_get,
	/* gpon_hw_prx126_util.c */
	.util_protect_start = gpon_hw_prx126_util_protect_start,
	.util_protect_end = gpon_hw_prx126_util_protect_end,
	.util_clearpq_dummy = gpon_hw_prx126_util_clearpq_dummy,
	.util_clearpq = gpon_hw_prx126_util_clearpq,
	.util_reset_dbru = gpon_hw_prx126_util_reset_dbru,
	.util_reset_serdes = gpon_hw_prx126_util_reset_serdes,
	.util_recovery = gpon_hw_prx126_util_recovery,
	.util_omccmiss_detect = gpon_hw_prx126_util_omccmiss_detect,
	.util_tcontmiss_detect = gpon_hw_prx126_util_tcontmiss_detect,
	.util_o2stuck_detect = gpon_hw_prx126_util_o2stuck_detect,
	.util_idlebw_is_overalloc = gpon_hw_prx126_util_idlebw_is_overalloc,
	/* gpon_hw_prx126_history.c */
	.history_init = gpon_hw_prx126_history_init,
	.history_add = gpon_hw_prx126_history_add,
	.history_print = gpon_hw_prx126_history_print,
};

