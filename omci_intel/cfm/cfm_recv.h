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
 * File    : cfm_recv.h
 *
 ******************************************************************/
 
#ifndef __CFM_RECV_H__
#define __CFM_RECV_H__

/* cfm_recv.c */
cfm_pkt_lbr_entry_t *cfm_recv_lbr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
cfm_pkt_ltr_entry_t *cfm_recv_ltr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
cfm_config_t *cfm_recv_get_cfm_config(cfm_pktinfo_t *cfm_pktinfo);
int cfm_recv_pkt_process(struct cpuport_info_t *pktinfo, cfm_pktinfo_t *cfm_pktinfo);

#endif
