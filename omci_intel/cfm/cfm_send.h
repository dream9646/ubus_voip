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
 * File    : cfm_send.h
 *
 ******************************************************************/
 
#ifndef __CFM_SEND_H__
#define __CFM_SEND_H__

#define CFM_SEND_NORMAL		0	// active send to mp direction, for ccm & cfm requests
#define CFM_SEND_OPPOSITE	1	// active send to opposite direction, for ais
#define CFM_SEND_FORWARD	2	// passive forward a received pkt, for pkt not belong to us
#define CFM_SEND_REPLY		3	// passive reply to a received pkt, for cfm replies

/* cfm_send.c */
int cfm_send_frame(cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, int send_type);
int cfm_send_lbr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
int cfm_send_lbm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei);
int cfm_send_ltr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
int cfm_send_ltm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, unsigned char ttl, unsigned char is_fdbonly);
int cfm_send_aislck(unsigned char opcode, cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags);
int cfm_send_ais(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags);
int cfm_send_lck(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags);
int cfm_send_lmr(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *entry, cfm_pktinfo_t *cfm_pktinfo);
int cfm_send_lmm(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei);
int cfm_send_dmr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
int cfm_send_dmm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei);
int cfm_send_1dm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei);
int cfm_send_tst(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei);
int cfm_send_1sl(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei);
int cfm_send_slr(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, cfm_pktinfo_t *cfm_pktinfo);
int cfm_send_slm(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei);
int cfm_send_ccm(cfm_config_t *cfm_config, char priority, char dei);
int cfm_forward_cfm(cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, cfm_pktinfo_t *cfm_pktinfo, unsigned char cfm_pdu);

#endif
