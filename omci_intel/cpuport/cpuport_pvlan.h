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
 * File    : cpuport_pvlan.h
 *
 ******************************************************************/
 
#ifndef __CPUPORT_PVLAN_H__
#define __CPUPORT_PVLAN_H__

/* cpuport_pvlan.c */
int cpuport_pvlan_vtagging(struct me_t *bridge_port, unsigned char is_upstream, struct cpuport_tci_t *src_tci, struct cpuport_tci_t *dst_tci, struct cpuport_info_t *cpuport_info);

#endif
