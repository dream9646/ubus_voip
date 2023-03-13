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
 * Module  : proprietary_fiberhome
 * File    : proprietary_fiberhome_remote_telnet_session.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include "meinfo.h"
#include "util.h"
#include "util_run.h"
#include "notify_chain.h"
#include "gpon_sw.h"
#include "proprietary_fiberhome.h"

#define REMOTE_SESSION_IFACE_PHY	"pon0"
#define REMOTE_SESSION_IFACE_VID	4089
#define REMOTE_SESSION_INTERFACE	"pon0.4089"


#ifndef X86

#include "switch.h"
static int remote_telnet_session_init = 0;

int proprietary_fiberhome_set_gem_port(int enable);

static int
o5_sequence_handler(unsigned char event, unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_O5_SEQUENCE) {
		dbprintf(LOG_WARNING, "O5 sequence event, reconfigure fiberhome ssh gemport\n");
		proprietary_fiberhome_set_gem_port(1);
	}
	return 0;
}

int
proprietary_fiberhome_set_gem_port(int enable)
{
	char cmd[128];
	struct gpon_tm_usflow_config_t gpon_tm_usflow_config;
	// DS is created by OMCI ds_bcast gem port, don't create here
	
	if(!remote_telnet_session_init && enable) {		
		sprintf(cmd, "ifconfig %s up", REMOTE_SESSION_IFACE_PHY);
		util_run_by_system(cmd);
		sprintf(cmd, "vconfig add %s %d", REMOTE_SESSION_IFACE_PHY, REMOTE_SESSION_IFACE_VID);
		util_run_by_system(cmd);
		sprintf(cmd, "ifconfig %s up", REMOTE_SESSION_INTERFACE);
		util_run_by_system(cmd);
		
		// register o5 sequence handler, so flow can get reconfigured from non-o5 to o5
		notify_chain_register(NOTIFY_CHAIN_EVENT_O5_SEQUENCE, o5_sequence_handler, NULL);
		
		remote_telnet_session_init = 1;
	}
	
	memset(&gpon_tm_usflow_config, 0, sizeof(gpon_tm_usflow_config));
	
	gpon_tm_usflow_config.gemport = 1;
	gpon_tm_usflow_config.flow_type = GPON_TM_FLOW_TYPE_ETH;
	
	// Unset uptream by us-flow=126
	if( gpon_hw_g.tm_usflow_get(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config)==0) {
		gpon_tm_usflow_config.enable = 0;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		gpon_hw_g.tm_usflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config);
#endif
	}
	
	if(enable) {
		// Set uptream by us-flow=126
		gpon_tm_usflow_config.enable = 1;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		if( gpon_hw_g.tm_usflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config)!=0) {
			dbprintf(LOG_ERR, "tm_usflow_set error\n");
			return -1;
		}
#endif		
		
		// Set us-flow=126 to OMCC queue
		gpon_hw_g.tm_usflow2pq_set(GPON_SSHD_FLOW_ID, GPON_OMCI_PQ_ID);
	}
	
	return 0;
}
#endif
