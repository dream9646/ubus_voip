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
 * File    : cpuport_frame.c
 *
 ******************************************************************/

#include <sys/time.h>
#include <netinet/ip.h>
#include "util.h"
#include "list.h"
#include "env.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#include "cpuport_sniffer.h"

int
cpuport_frame_is_available(struct timeval *timeout)
{
	if (cpuport_hw_g.frame_recv_is_available == NULL)
		return -2;

	return cpuport_hw_g.frame_recv_is_available(timeout);
}

int
cpuport_frame_recv(struct cpuport_info_t *pktinfo , unsigned short buf_len)
{
	
	int ret = -1;

	if ( cpuport_hw_g.frame_recv  == NULL )
		return -2;

	ret = cpuport_hw_g.frame_recv( pktinfo ,buf_len);
	if ( ret <0 )	return -1;
	if ( ret == 1 )	return 1;
	if ( ret == 0 )	return 0;
	return 0;
}

int 
cpuport_frame_recv_abort_select(void)
{
	if ( cpuport_hw_g.frame_recv_abort_select == NULL)
		return -1;
	return cpuport_hw_g.frame_recv_abort_select();
}

int 
cpuport_frame_recv_abort_loop(void)
{
	if ( cpuport_hw_g.frame_recv_abort_loop == NULL)
		return -1;
	return cpuport_hw_g.frame_recv_abort_loop();
}

int 
cpuport_frame_send( struct cpuport_info_t *pktinfo)
{
	if ( cpuport_hw_g.frame_send == NULL )
		return -2;
	cpuport_sniffer_dbprint_pkt(LOG_NOTICE, "cpu_tx", pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->tx_desc.router_mode);
	if (((1<<pktinfo->tx_desc.logical_port_id) & switch_get_linkup_logical_portmask()) == 0) {
		dbprintf_cpuport(LOG_NOTICE, "send to logical port %d canceled because of link down\n", pktinfo->tx_desc.logical_port_id);
		return 0;
	}	
	return cpuport_hw_g.frame_send (pktinfo, CPUPORT_XMIT_RAW);
}

