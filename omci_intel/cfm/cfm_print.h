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
 * File    : cfm_print.h
 *
 ******************************************************************/
 
#ifndef __CFM_PRINT_H__
#define __CFM_PRINT_H__

char *cfm_print_get_cfm_pkt_str(cfm_hdr_t *cfm_hdr);	// keep this, cproto wont generate this line?

/* cfm_print.c */
char *cfm_print_get_opcode_str(unsigned char opcode);
char *cfm_print_get_cfm_config_str(cfm_config_t *cfm_config);
char *cfm_print_get_tlv_str(unsigned char *tlv_start, unsigned int tlv_total_len);
char *cfm_print_get_vlan_str(cfm_pktinfo_t *cfm_pktinfo);
char *cfm_print_get_maid_str(char *maid);
char *cfm_print_get_us_time_str(unsigned long long us);
char *cfm_print_get_rmep_state_str(int state);
int cfm_print_send_frame(int debug_level, cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, int send_type);
int cfm_print_recv_frame(int debug_level, cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
void cfm_print_all_mp_status(int fd, int md_level, int mep_id);
int cfm_print_mp_detail(int fd, cfm_config_t *cfm_config);
void cfm_print_mp_pkt_counter(int fd, int opcode, int md_level, int mep_id);
void cfm_print_lbr_list(int fd, int md_level, int mep_id);
void cfm_print_ltm_list(int fd, int md_level, int mep_id);
void cfm_print_ltr_list(int fd, int md_level, int mep_id);
void cfm_print_rmep_list(int fd, int md_level, int mep_id);
void cfm_print_lm_test_list(int fd, int md_level, int mep_id);

#endif
