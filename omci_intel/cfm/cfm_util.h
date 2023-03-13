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
 * Module  : cfm
 * File    : cfm_util.h
 *
 ******************************************************************/
 
#ifndef __CFM_UTIL_H__
#define __CFM_UTIL_H__

extern struct fwk_mutex_t cfm_util_get_time_mutex_g;

/* cfm_util.c */
unsigned long long cfm_util_get_time_in_us(void);
int cfm_util_us_to_64bit_ieee1588(unsigned long long us, struct ieee1588_time *now);
int cfm_util_ieee1588_64bit_to_us(struct ieee1588_time now, unsigned long long *us);
unsigned int cfm_util_ccm_interval_to_ms(int ccm_interval);
int cfm_util_get_send_priority_by_defect_status(unsigned char defect_status);
int cfm_util_find_tlv(unsigned char *tlv_start, unsigned int tlv_total_len, int find_tlv_type, unsigned short *find_tlv_len, unsigned char **find_tlv_val);
int cfm_util_get_mac_addr(cfm_config_t *cfm_config, unsigned char *mac, char *devname);
int cfm_util_get_rdi_ais_lck_alarm_gen(cfm_config_t *cfm_config, unsigned char *rdi_gen, unsigned char *ais_gen, unsigned char *lck_gen, unsigned char *alarm_gen);
int cfm_util_get_mhf_creation(cfm_config_t *cfm_config);
int cfm_util_get_sender_id_permission(cfm_config_t *cfm_config);
int cfm_util_set_md_name(cfm_config_t *cfm_config, int format, char *string, char *string2);
int cfm_util_set_ma_name(cfm_config_t *cfm_config, int format, char *string, char *string2);

#endif
