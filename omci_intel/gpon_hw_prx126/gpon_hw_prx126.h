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
 * Module  : gpon_hw
 * File    : gpon_hw_prx126.h
 *
 ******************************************************************/

#ifndef __GPON_HW_PRX126_H__
#define __GPON_HW_PRX126_H__

#include "chipdef.h"

/* gpon_hw_prx126_conv.c */
int gpon_hw_prx126_conv_pqid_seq_to_private_clear(unsigned seq_pqid, unsigned int *private_pqid);
int gpon_hw_prx126_conv_pqid_seq_to_private(unsigned seq_pqid, unsigned int *private_pqid, unsigned int *tcont_id);
int gpon_hw_prx126_conv_pqid_private_to_seq(unsigned int tcont_id, unsigned int private_pqid, unsigned int *seq_pqid);

/* gpon_hw_prx126_history.c */
int gpon_hw_prx126_history_init(void);
int gpon_hw_prx126_history_add(void);
int gpon_hw_prx126_history_print(int fd, int type);

/* gpon_hw_prx126_i2c.c */
int gpon_hw_prx126_i2c_open(int i2c_port);
int gpon_hw_prx126_i2c_read(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
int gpon_hw_prx126_i2c_write(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
int gpon_hw_prx126_i2c_close(int i2c_port);
int gpon_hw_prx126_eeprom_open(int i2c_port, unsigned int devaddr);
int gpon_hw_prx126_eeprom_data_get(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
int gpon_hw_prx126_eeprom_data_set(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);

/* gpon_hw_prx126_onu.c */
int gpon_hw_prx126_onu_mgmt_mode_get(int *mgmt_mode);
int gpon_hw_prx126_onu_mgmt_mode_set(int mgmt_mode);
int gpon_hw_prx126_onu_serial_number_get(char *vendor_id, unsigned int *serial_number);
int gpon_hw_prx126_onu_serial_number_set(char *vendor_id, unsigned int *serial_number);
int gpon_hw_prx126_onu_password_get(char *password);
int gpon_hw_prx126_onu_password_set(char *password);
int gpon_hw_prx126_onu_activate(unsigned int state);
int gpon_hw_prx126_onu_deactivate(void);
int gpon_hw_prx126_onu_state_get(unsigned int *state);
int gpon_hw_prx126_onu_id_get(unsigned short *id);
int gpon_hw_prx126_onu_alarm_event_get(struct gpon_onu_alarm_event_t *alarm_event);
int gpon_hw_prx126_onu_gem_blocksize_get(int *gem_blocksize);
int gpon_hw_prx126_onu_gem_blocksize_set(int gem_blocksize);
int gpon_hw_prx126_onu_sdsf_get(int sd_threshold_ind, int sf_threshold_ind, int *sd_state, int *sf_state);
int gpon_hw_prx126_onu_superframe_seq_get(unsigned long long *superframe_sequence);
/* gpon_hw_prx126_pm.c */
int gpon_hw_prx126_counter_global_get(struct gpon_counter_global_t *counter_global);
int gpon_hw_prx126_counter_tcont_get(int tcont_id, struct gpon_counter_tcont_t *counter_tcont);
int gpon_hw_prx126_counter_dsflow_get(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow);
int gpon_hw_prx126_counter_usflow_get(int usflow_id, struct gpon_counter_usflow_t *counter_usflow);
int gpon_hw_prx126_pm_refresh(int is_reset);
int gpon_hw_prx126_pm_global_get(struct gpon_counter_global_t *counter_global);
int gpon_hw_prx126_pm_tcont_get(int tcont_id, struct gpon_counter_tcont_t *counter_tcont);
int gpon_hw_prx126_pm_dsflow_get(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow);
int gpon_hw_prx126_pm_usflow_get(int usflow_id, struct gpon_counter_usflow_t *counter_usflow);

/* gpon_hw_prx126_tm.c */
int gpon_hw_prx126_tm_pq_config_get(int pq_id, struct gpon_tm_pq_config_t *pq_config);
int gpon_hw_prx126_tm_pq_config_set(int pq_id, struct gpon_tm_pq_config_t *pq_config,int create_only);
int gpon_hw_prx126_tm_pq_config_del(int pq_id, struct gpon_tm_pq_config_t *pq_config);
int gpon_hw_prx126_tm_ts_config_get(int ts_id, struct gpon_tm_ts_config_t *ts_config);
int gpon_hw_prx126_tm_ts_config_set(int ts_id, struct gpon_tm_ts_config_t *ts_config,int create_only);
int gpon_hw_prx126_tm_ts_config_del(int ts_id, struct gpon_tm_ts_config_t *ts_config);
int gpon_hw_prx126_tm_tcont_config_get( struct gpon_tm_tcont_config_t *tcont_config);
int gpon_hw_prx126_tm_tcont_config_set(struct gpon_tm_tcont_config_t *tcont_config,int create_only);
int gpon_hw_prx126_tm_tcont_config_del(struct gpon_tm_tcont_config_t *tcont_config);
int gpon_hw_prx126_tm_tcont_add_by_allocid(int allocid, int *tcont_id);
int gpon_hw_prx126_tm_tcont_get_by_allocid(int allocid, int *tcont_id);
int gpon_hw_prx126_tm_tcont_del_by_allocid(int allocid);
int gpon_hw_prx126_tm_dsflow_get(int dsflow_id, struct gpon_tm_dsflow_config_t *dsflow_config);
int gpon_hw_prx126_tm_dsflow_set(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config);
int gpon_hw_prx126_tm_dsflow_del(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config);
int gpon_hw_prx126_tm_usflow_get(int usflow_id, struct gpon_tm_usflow_config_t *usflow_config);
int gpon_hw_prx126_tm_usflow_set(int usflow_id, struct gem_port_net_ctp_update_data *usflow_config);
int gpon_hw_prx126_tm_usflow_del(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config); 

int gpon_hw_prx126_tm_usflow2pq_get(int usflow_id, int *pq_id);
int gpon_hw_prx126_tm_usflow2pq_set(int usflow_id, int pq_id);
int gpon_hw_prx126_tm_flow_8021p_set(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg,int is_set);
int gpon_hw_prx126_tm_flow_iwtp_set(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg,int is_set);
int gpon_hw_prx126_tm_flow_mcastiwtp_set(unsigned short meid, unsigned short ctp_ptr,int is_set);

/* gpon_hw_prx126_util.c */
int gpon_hw_prx126_util_clearpq_dummy(void);
void gpon_hw_prx126_util_clearpq(unsigned int page_threshold);
void gpon_hw_prx126_util_reset_dbru(void);
void gpon_hw_prx126_util_protect_start(unsigned int *state);
void gpon_hw_prx126_util_protect_end(unsigned int state);
void gpon_hw_prx126_util_reset_serdes(int msec);
void gpon_hw_prx126_util_recovery(void);
int gpon_hw_prx126_util_omccmiss_detect(void);
int gpon_hw_prx126_util_tcontmiss_detect(void);
int gpon_hw_prx126_util_o2stuck_detect(void);
int gpon_hw_prx126_util_idlebw_is_overalloc(unsigned short idlebw_guard_parm[]);

#endif	// __GPON_HW_PRX126_H__
