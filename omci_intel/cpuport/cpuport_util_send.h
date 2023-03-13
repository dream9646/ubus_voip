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
 * Module  : cpuport
 * File    : cpuport_util_send.h
 *
 ******************************************************************/

#ifndef __CPUPORT_UTIL_SEND_H__
#define __CPUPORT_UTIL_SEND_H__

/* cpuport_util_send.c */
void dbprintf_cpuport_send_wan_handle_tag(struct cpuport_info_t *pktinfo, struct me_t *equiv_ingress_bport_me, char *prompt, char *msg);
void dbprintf_cpuport_send_lan_handle_tag(struct cpuport_info_t *pktinfo, struct me_t *equiv_egress_bport_me, char *prompt, char *msg);
int cpuport_util_send_multi_wan(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *), int (lan_to_wan_cb)(unsigned char pbit_filter_mask, struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *)));
int cpuport_util_send_multi_lan(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *), int (wan_to_lan_cb)(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *)));
int cpuport_util_send_multi_lan_from_lan(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *));

#endif
