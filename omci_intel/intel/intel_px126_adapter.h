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

#ifndef __INTEL_PX126_ADAPTER_H__
#define __INTEL_PX126_ADAPTER_H__

#include "cpuport.h"
void intel_adapter_register_omcc_rcv_cb(void (*omcc_raw_rcv)(void *msg,unsigned short len,unsigned int crc ));
void intel_adapter_omcc_send(char* omci_msg,unsigned short len,unsigned int crc_mic);
int intel_adapter_config_dbglvl(unsigned short is_set, unsigned short dbgmodule,unsigned short* dbglvl);
int intel_adapter_pa_mapper_dump(void);
int intel_adapter_cpuport_send(struct cpuport_info_t *cpuport_info);
int intel_adapter_cpuport_recv(struct cpuport_info_t *cpuport_info);

void intel_adapter_omci_init(void);
void intel_adapter_omci_exit(void);

#endif
