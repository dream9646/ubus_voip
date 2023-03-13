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
 * File    : extoam_msg.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>

#include "list.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_swdl.h"

extern struct extoam_link_status_t cpe_link_status_g;

int
extoam_msg_get_dst_mac( unsigned char mac[6])
{
	mac[0] = cpe_link_status_g.dpu_mac[0];
	mac[1] = cpe_link_status_g.dpu_mac[1];
	mac[2] = cpe_link_status_g.dpu_mac[2];
	mac[3] = cpe_link_status_g.dpu_mac[3];
	mac[4] = cpe_link_status_g.dpu_mac[4];
	mac[5] = cpe_link_status_g.dpu_mac[5];
	return 0;
}


static int
extoam_util_get_mac_addr(unsigned char *mac, char *devname)
{
	int fd;
	struct ifreq itf;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	strncpy(itf.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFHWADDR, &itf) < 0) {
		close(fd);
		return -1;
	} else {
		memcpy(mac, itf.ifr_hwaddr.sa_data, IFHWADDRLEN);
		close(fd);
	}
	return 0;
}

static int
extoam_msg_get_src_mac( unsigned char mac[6] )
{
	extoam_util_get_mac_addr(mac, omci_env_g.extoam_mac_ifname);
	return 0;
}

int 
extoam_msg_header_prep(unsigned short ext_opcode, unsigned short transaction_id, unsigned char *buff)
{
	struct oam_pdu_t *oam_pdu= NULL;
	oam_pdu = (struct oam_pdu_t *)buff; 
	memset( oam_pdu, 0, sizeof(struct oam_pdu_t) );
	oam_pdu->dst_mac[0] = 0x01;
	oam_pdu->dst_mac[1] = 0x80;
	oam_pdu->dst_mac[2] = 0xC2;
	oam_pdu->dst_mac[3] = 0x00;
	oam_pdu->dst_mac[4] = 0x00;
	oam_pdu->dst_mac[5] = 0x02;

	extoam_util_get_mac_addr(oam_pdu->src_mac, omci_env_g.extoam_mac_ifname);
	oam_pdu->ether_type = htons(SLOW_PROTO);
	oam_pdu->subtype = TYPE_OAM;
	oam_pdu->flags = htons(0);
	oam_pdu->code = ORG_SPECIFIC_CODE;
	oam_pdu->oui[0] = 0x11;
	oam_pdu->oui[1] = 0x11;
	oam_pdu->oui[2] = 0x11;
	oam_pdu->ext_oam.ext_code = htons(ext_opcode);
	oam_pdu->ext_oam.transaction_id =htons(transaction_id);
	return 0;
}

int 
extoam_msg_ext_header_prep( unsigned short ext_code, unsigned short link_id, 
		unsigned short transaction_id , unsigned char *buff)
{
	struct oam_pdu_t *tx_oam_pdu= NULL;
	tx_oam_pdu = (struct oam_pdu_t *)buff; 
	memset( tx_oam_pdu, 0, sizeof(struct oam_pdu_t) );
	extoam_msg_get_dst_mac( tx_oam_pdu->dst_mac );
	extoam_msg_get_src_mac( tx_oam_pdu->src_mac );
	tx_oam_pdu->ether_type = htons(SLOW_PROTO);
	tx_oam_pdu->subtype = TYPE_OAM;
	tx_oam_pdu->flags = 0;
	tx_oam_pdu->code = ORG_SPECIFIC_CODE;
	tx_oam_pdu->oui[0] = 0x11;
	tx_oam_pdu->oui[1] = 0x11;
	tx_oam_pdu->oui[2] = 0x11;
	tx_oam_pdu->ext_oam.transaction_id = htons(transaction_id);
	tx_oam_pdu->ext_oam.link_id = htons( link_id );
	tx_oam_pdu->ext_oam.ext_code = htons(ext_code);
	return 0;
}

unsigned short
extoam_msg_swdl_block_tx( struct sw_dl_proc_t *cpe_tx, unsigned char *buff )
{
	struct file_transfer_t *file_transfer;
	struct oam_pdu_t *oam_pdu = (struct oam_pdu_t *)buff;
	unsigned short len = 0;

	extoam_msg_ext_header_prep( EXT_OP_SW_DL, cpe_link_status_g.link_id ,cpe_tx->transaction_id, buff );
	file_transfer = (struct file_transfer_t *)oam_pdu->ext_oam.payload;
	file_transfer->dl_op = SW_DL_TRAN_DATA;

	file_transfer->block_num = htons(cpe_tx->block+1);
	if ( cpe_tx->retry > 0 )  //resend last block
		fseek( cpe_tx->fd, -cpe_tx->last_block_len, SEEK_CUR );


	cpe_tx->last_block_len =  fread( file_transfer->data, sizeof(unsigned char), MAX_DATA_LENGTH, cpe_tx->fd );
	len = cpe_tx->last_block_len; // data lenghth
	len += (sizeof(struct oam_pdu_t) - MAX_DATA_LENGTH + 6 ); // data + header length

	return len;
}


int 
extoam_msg_swdl_ack( unsigned short link_id, unsigned short transaction_id ,unsigned short block_num, unsigned char ack_resend, unsigned char *buff )
{
	struct file_transfer_ack_t *file_transfer_ack;
	struct oam_pdu_t *oam_pdu = (struct oam_pdu_t *)buff; 
	
	extoam_msg_ext_header_prep( EXT_OP_SW_DL, link_id, transaction_id, buff );

	file_transfer_ack = (struct file_transfer_ack_t *)(oam_pdu->ext_oam.payload );
	file_transfer_ack->dl_op = htons(SW_DL_TRAN_ACK); 
	file_transfer_ack->ack_resend = ack_resend;  
	file_transfer_ack->block_num = htons(block_num);  
	
	return 0;
}

int 
extoam_msg_swdl_enddl_response(unsigned short link_id, unsigned short transaction_id, unsigned char response_code , unsigned char *buff)
{
	struct end_dl_response_t *end_dl_response;
	struct oam_pdu_t *oam_pdu = (struct oam_pdu_t *)buff; 

	extoam_msg_ext_header_prep(  EXT_OP_SW_DL, link_id, transaction_id, buff );

	end_dl_response = (struct end_dl_response_t *)(oam_pdu->ext_oam.payload );
	end_dl_response->dl_op = htons(SW_DL_END_DL_RESP); 
	end_dl_response->response_code = response_code;  

	return 0;
}

int 
extoam_msg_swdl_enddl_request(unsigned short link_id, unsigned short transaction_id, unsigned int file_size, unsigned short crc, unsigned char *buff)
{

	struct end_dl_request_t *end_dl_request;
	struct oam_pdu_t *oam_pdu = (struct oam_pdu_t *)buff; 

	extoam_msg_ext_header_prep( EXT_OP_SW_DL ,link_id, transaction_id , buff );

	end_dl_request = (struct end_dl_request_t *)(oam_pdu->ext_oam.payload );
	end_dl_request->dl_op = htons(SW_DL_END_DL_REQ); 
	end_dl_request->file_size = htonl(file_size);  
	end_dl_request->crc = htons(crc);  

	return 0;
}

int
extoam_msg_send( unsigned char *tx_buff, unsigned short len )
{
	struct cpuport_info_t pktinfo;
	//pktinfo.tx_desc.port_id = cpe_link_status_g.dpu_port;
	pktinfo.tx_desc.logical_port_id = cpe_link_status_g.dpu_port;
	pktinfo.frame_ptr = tx_buff;
	pktinfo.buf_ptr = tx_buff;
	pktinfo.frame_len = len;
	pktinfo.buf_len = BUFF_LEN;
	cpuport_frame_send( &pktinfo );
	return 0;
}
