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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "util.h"

#include "intel_px126_util.h"
#include "intel_px126_mcc_util.h"
#include "cpuport.h"

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

void intel_adapter_register_omcc_rcv_cb(void (*omcc_raw_rcv)(void *msg,unsigned short len,unsigned int crc ))
{
	intel_omci_register_omcc_msg_cb(omcc_raw_rcv);	
}

void intel_adapter_omcc_send(char* omci_msg,unsigned short len,unsigned int crc_mic)
{
	//dbprintf(LOG_ERR, "1111message len=%d-0x%x\n", len, crc_mic);
	intel_omci_omcc_msg_send(omci_msg,len,crc_mic);	
}

int intel_adapter_cpuport_recv(struct cpuport_info_t *cpuport_info)
{
	struct mcc_pkt  mcc_pkt_d;
	int ret;
	memset(&mcc_pkt_d,0x0,sizeof(struct mcc_pkt));

	mcc_pkt_d.data= cpuport_info->frame_ptr;
	mcc_pkt_d.len = cpuport_info->buf_len;
	ret = intel_mcc_pkt_receive(&mcc_pkt_d);
	cpuport_info->frame_len = mcc_pkt_d.len;
	cpuport_info->rx_desc.logical_port_id = (mcc_pkt_d.llinfo.dir_us)?0:2;
	if(ret!=INTEL_PON_SUCCESS)
		return -1;
	return 1;
}

int intel_adapter_cpuport_send(struct cpuport_info_t *cpuport_info)
{
	struct mcc_pkt  mcc_pkt_d;
	int ret;
	memset(&mcc_pkt_d,0x0,sizeof(struct mcc_pkt));

	mcc_pkt_d.data= cpuport_info->frame_ptr;
	mcc_pkt_d.len = cpuport_info->frame_len;
	mcc_pkt_d.llinfo.dir_us = (cpuport_info->tx_desc.logical_port_id == 2)?1:0;
	ret = intel_mcc_pkt_send(&mcc_pkt_d);
	
	if(ret!=INTEL_PON_SUCCESS)
		return -1;
	return 0;
}

int intel_adapter_config_dbglvl(unsigned short is_set, unsigned short dbgmodule,unsigned short* dbglvl)
{
	int std_fd = -1;

	if (*dbglvl <= 3) {
		std_fd = open("/dev/console", O_RDWR);
	} else {
		std_fd = open("/dev/null", O_RDWR);
	}

	if (std_fd < 0) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,  "errno=%d (%s)\n", errno, strerror(errno));
		return INTEL_PON_ERROR;
	}
	//dup2(std_fd, 0);	// standrad input set 
	dup2(std_fd, 1);	// standrad ouput set
	dup2(std_fd, 2);	// standrad error set
	close(std_fd);

	return intel_omci_config_dbglvl(is_set,dbgmodule,dbglvl);	
}


int intel_adapter_pa_mapper_dump(void)
{
	return intel_omci_pa_mapper_dump();
}

///////////////////////////////////
void intel_adapter_omci_init(void)
{
	intel_omci_init();
}

void intel_adapter_omci_exit(void)
{

}
