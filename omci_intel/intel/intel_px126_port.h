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
#ifndef __INTEL_PX126_PORT_H__
#define __INTEL_PX126_PORT_H__


int intel_px126_port_inf_set(unsigned short meid);
int intel_px126_port_inf_del(unsigned short meid);
int intel_px126_port_inf_update(unsigned short meid,struct switch_pptp_eth_uni_data *updata);
int intel_px126_port_unlock(unsigned short meid);
int intel_px126_port_lock(unsigned short meid);
int intel_px126_op_status_get(unsigned short meid,unsigned char * status);

#endif

