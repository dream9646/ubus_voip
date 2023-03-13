
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
 * File    : switch_hw_prx126_acl.c
 *
 ******************************************************************/
#ifndef __INTEL_PX126_TM_H__
#define __INTEL_PX126_TM_H__
#include "gpon_sw.h"
/** Traffic Scheduler update structure */
struct px126_traffic_scheduler_update_data {
	/** T-CONT pointer */
	unsigned short tcont_ptr;
	/** Traffic Scheduler pointer */
	unsigned short traffic_scheduler_ptr;
	/** Policy */
	unsigned char policy;
	/** Priority/weight */
	unsigned char priority_weight;
};

/** Arguments to Priority Queue update and create */
struct px126_priority_queue_update_data {
	/** Discard-block counter reset interval */
	unsigned short discard_block_cnt_reset_interval;
	/** Threshold value for discarded blocks due to buffer overflow */
	unsigned short thr_value_for_discarded_blocks;
	/** Weight */
	unsigned char weight;
	/** Back pressure operation */
	unsigned short back_pressure_operation;
	/** Back pressure time */
	unsigned int back_pressure_time;
	/** Back pressure occur queue threshold */
	unsigned short back_pressure_occur_thr;
	/** Back pressure clear queue threshold */
	unsigned short back_pressure_clear_thr;
	/** Packet drop queue thresholds first value */
	unsigned short pkt_drop_q_thr_green_min;
	/** Packet drop queue thresholds second value */
	unsigned short pkt_drop_q_thr_green_max;
	/** Packet drop queue thresholds third value */
	unsigned short pkt_drop_q_thr_yellow_min;
	/** Packet drop queue thresholds fourth value */
	unsigned short pkt_drop_q_thr_yellow_max;
	/** The probability of dropping a green packet */
	unsigned char pkt_drop_probability_green;
	/** The probability of dropping a yellow packet */
	unsigned char pkt_drop_probability_yellow;
	/** Queue drop averaging coefficient */
	unsigned char q_drop_averaging_coefficient;
	/** Drop precedence color marking */
	unsigned char drop_precedence_color_marking;
	/** Traffic scheduler ptr */
	unsigned short traffic_scheduler_ptr;
	/** Related port */
	unsigned int related_port;
};


int intel_px126_tm_get_by_alloc_id(unsigned short allocid,int *tcont_id) ;
int intel_px126_tm_get_allocid_by_tmidex(unsigned short tmidex,unsigned short *allocid);
int intel_px126_tm_updtae_tcont(unsigned short meid,unsigned short policy,unsigned short alloc_id,unsigned short is_create) ;
int intel_px126_tm_destory_tcont(unsigned short meid,unsigned short alloc_id) ;
int intel_px126_tm_updtae_ts(unsigned short meid,struct px126_traffic_scheduler_update_data *update_data,unsigned short is_create) ;
int intel_px126_tm_destory_ts(unsigned short meid) ;
int intel_px126_tm_updtae_queue(unsigned short meid,struct px126_priority_queue_update_data *update_data,unsigned short is_create);
int intel_px126_tm_destory_queue(unsigned short meid,struct px126_priority_queue_update_data *update_data);
int intel_px126_tm_del_queue(unsigned short meid);
int intel_px126_tm_usflow_get(int usflow_id, struct gpon_tm_usflow_config_t *usflow_config);
int intel_px126_tm_dsflow_get(int dsflow_id, struct gpon_tm_dsflow_config_t *dsflow_config);
int intel_px126_tm_usflow_set(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config);
int intel_px126_tm_dsflow_set(int dsflow_id,  struct gem_port_net_ctp_update_data *dsflow_config);
int intel_px126_tm_usflow_del(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config);
int intel_px126_tm_dsflow_del(int dsflow_id,  struct gem_port_net_ctp_update_data *dsflow_config);
int intel_px126_tm_flow_8021p_set(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg);
int intel_px126_tm_flow_8021p_del(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg);
int intel_px126_tm_flow_iwtp_set(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg);
int intel_px126_tm_flow_iwtp_del(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg);
int intel_px126_tm_flow_mcastiwtp_set(unsigned short meid, unsigned short ctp_ptr);
int intel_px126_tm_flow_mcastiwtp_destory(unsigned short meid);

#endif
