/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF EXTENDED OAM
 * Module  : extoam
 * File    : extoam_event.c
 *
 ******************************************************************/
#include "util.h"
#include "list.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
int
extoam_event_send( struct extoam_link_status_t *cpe_link_status, unsigned short event_id )
{
	unsigned char tx_oam_buff[BUFF_LEN];	
	extoam_msg_ext_header_prep(EXT_OP_EVENT, cpe_link_status->link_id, ++cpe_link_status->local_transaction_id, tx_oam_buff);
	*(unsigned short *)(tx_oam_buff + MAC_LEN*2+ETHERTYPE_LEN+OAM_HEAD_LEN+EXTOAM_HEAD_LEN) = htons(event_id);
	extoam_msg_send( tx_oam_buff, MAC_LEN*2+ETHERTYPE_LEN+OAM_HEAD_LEN+EXTOAM_HEAD_LEN+2);
	return 0;
}
