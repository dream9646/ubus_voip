/*
 * =====================================================================================
 *
 *       Filename:  extoam_link.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月03日 11時54分53秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "extoam.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
#include "extoam_queue.h"
#include "extoam_link.h"

struct extoam_link_status_t cpe_link_status_g;
extern int extoam_timer_qid_g; 
extern struct extoam_queue_t extoam_queue_g; 
extern struct sw_dl_proc_t cpe_rx_g;
extern struct sw_dl_proc_t cpe_tx_g;

int
extoam_link_init(struct  extoam_link_status_t *cpe_link_status)
{
	memset( cpe_link_status, 0 , sizeof(struct extoam_link_status_t));
	cpe_link_status->link_id = 0xffff;
	cpe_link_status->dpu_port = omci_env_g.extoam_port;
	cpe_link_status->local_transaction_id = 1 << 15;
	return 0;
}

int
extoam_link_discover(struct extoam_link_status_t *cpe_link_status, unsigned char **oam_tx_buff)
{
	struct oam_pdu_t *oam_pdu= NULL;
	struct ext_link_discover_t *ext_link_discover;

	*oam_tx_buff = malloc_safe(sizeof(unsigned char)*BUFF_LEN);
	extoam_msg_header_prep(EXT_OP_LINK_DISCOVER ,  0, *oam_tx_buff);

	oam_pdu = (struct oam_pdu_t *)(*oam_tx_buff); 
	ext_link_discover = (struct ext_link_discover_t *)oam_pdu->ext_oam.payload;
	ext_link_discover->link_op = htons(LINK_DISCOVER);

	cpe_link_status->link_state = LINK_DISCOVER;
	gettimeofday( &cpe_link_status->last_phase, NULL );

	cpe_link_status->link_state_for_timer = LINK_DISCOVER;

	if ( cpe_link_status->last_link_state_timer_id  == 0 ) {
		cpe_link_status->last_link_state_timer_id = 
			fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_CHECK_LINK_STAT, 5000, NULL );
	}

	return 0;
}



static int
extoam_link_request( struct extoam_link_status_t *cpe_link_status, unsigned char **oam_tx_buff )
{
	struct oam_pdu_t *tx_oam_pdu= NULL;
	struct ext_link_discover_t *ext_link_discover;

	if ( cpe_link_status->link_state == LINK_UNLINK )
		return 0;

	*oam_tx_buff = malloc_safe(sizeof(unsigned char)*BUFF_LEN);
	extoam_msg_ext_header_prep( EXT_OP_LINK_DISCOVER , 0,0, *oam_tx_buff );
	//extoam_msg_header_prep(EXT_OP_LINK_DISCOVER ,  0, *oam_tx_buff);
	tx_oam_pdu = (struct oam_pdu_t *)(*oam_tx_buff); 
	ext_link_discover = (struct ext_link_discover_t *)tx_oam_pdu->ext_oam.payload;
	ext_link_discover->link_op = htons(LINK_REQUEST);
	ext_link_discover->link_number = htons(cpe_link_status->link_id); 

	cpe_link_status->link_state = LINK_REQUEST;
	gettimeofday( &cpe_link_status->last_phase, NULL );
	return 0;
}

static int
extoam_link_offer(struct extoam_link_status_t *cpe_link_status, unsigned char *oam_rx_buff, unsigned char **oam_tx_buff)
{
	//struct timeval current_time; 
	struct oam_pdu_t *rx_oam_pdu= NULL;
	struct ext_link_discover_t *ext_link_discover;

	// maybe receive more than 1 LINK_OFFER from DPU
	//// and the state maybe already in LINK_OFFER
	if ( cpe_link_status->link_state == LINK_UNLINK  || cpe_link_status->link_state == LINK_OFFER)
		return 0;
	else if ( cpe_link_status->link_state != LINK_DISCOVER ) {
		cpe_link_status->link_state = LINK_UNLINK;
		extoam_link_discover( cpe_link_status , oam_tx_buff);
	} 

	//only LINK_DISCOVER state will be going on
	//gettimeofday( &current_time, NULL );

	if ( cpe_link_status->link_state == LINK_DISCOVER ) {
		if ( oam_rx_buff == NULL ) {
			dbprintf(LOG_ERR,"RX buffer is NULL\n");
			return -1;
		}
		rx_oam_pdu = (struct oam_pdu_t *)oam_rx_buff; 
		cpe_link_status->link_state = LINK_OFFER;
		ext_link_discover = (struct ext_link_discover_t *)rx_oam_pdu->ext_oam.payload;
		cpe_link_status->link_id = ntohs(ext_link_discover->link_number); 
		//memcpy( &cpe_link_status->last_phase , &current_time, sizeof(struct timeval) );
		extoam_link_request( cpe_link_status, oam_tx_buff );
	}
	return 0;
}

static int
extoam_link_ack(struct extoam_link_status_t *cpe_link_status, unsigned char *rx_oam_buff, unsigned char **oam_tx_buff)
{
	struct oam_pdu_t *rx_oam_pdu= NULL;
	struct ext_link_discover_t *ext_link_discover;


	if ( cpe_link_status->link_state == LINK_UNLINK )
		return 0;
	else if ( cpe_link_status->link_state == LINK_REQUEST ) { 
		if ( rx_oam_buff == NULL )
			return -1;
		rx_oam_pdu = (struct oam_pdu_t *)rx_oam_buff; 
		cpe_link_status->link_state = LINK_ACK;
		ext_link_discover = (struct ext_link_discover_t *)rx_oam_pdu->ext_oam.payload;
		if ( ntohs(rx_oam_pdu->ext_oam.link_id) !=  cpe_link_status->link_id ) {
			cpe_link_status->link_state = LINK_UNLINK;
			extoam_link_discover( cpe_link_status, oam_tx_buff );
			return 0;
		}
		cpe_link_status->link_id = ntohs(ext_link_discover->link_number); 
	}else if ( cpe_link_status->link_state == LINK_ACK ) 
		return 0;
	else{
		cpe_link_status->link_state = LINK_UNLINK;
		extoam_link_discover( cpe_link_status, oam_tx_buff );
	}
	
	return 0;
}

//reset link.
int
extoam_link_reset(struct extoam_link_status_t *cpe_link_status,struct extoam_queue_t *extoam_queue,unsigned char **oam_tx_buff)
{
	if (cpe_link_status->last_reset_timer_id != 0) {//to limit link reset at most 1 time per second.
		dbprintf(LOG_ERR,"link reset is limited to 1 pkt/sec\n");
		return 0;
	}

	if (cpe_tx_g.fd) {
		fclose(cpe_tx_g.fd);
		cpe_tx_g.on = 0;
		cpe_tx_g.transaction_id = 0;
	}

	extoam_queue_flush ( extoam_queue );
	cpe_link_status->link_id = 0xffff;
	cpe_link_status->link_state = LINK_UNLINK;
	fwk_timer_delete(cpe_link_status->last_link_state_timer_id );
	cpe_link_status->last_link_state_timer_id = 0;
	extoam_link_discover(cpe_link_status, oam_tx_buff);
//	if (cpe_link_status->last_reset_timer_id == 0 ) {
//		cpe_link_status->last_reset_timer_id = 
//			fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_CHECK_LINK_RESET, 2000, NULL );
//	}
	return 0;
}

int 
extoam_link_dispatch ( struct oam_pdu_t  *rx_oam_pdu , struct list_head *obj_handler_q )
{
	struct ext_link_discover_t *ext_link_discover;
	unsigned char *tx_oam_buf=NULL; 
	ext_link_discover = (struct ext_link_discover_t *)rx_oam_pdu->ext_oam.payload;
	unsigned short link_op = ntohs(ext_link_discover->link_op);
	switch ( link_op ) {
		case LINK_OFFER:
			if ( *(unsigned int *)&cpe_link_status_g.dpu_mac[0] == 0 
					&& *(unsigned short *)&cpe_link_status_g.dpu_mac[4] == 0 ) {
				cpe_link_status_g.dpu_mac[0] = rx_oam_pdu->src_mac[0];
				cpe_link_status_g.dpu_mac[1] = rx_oam_pdu->src_mac[1];
				cpe_link_status_g.dpu_mac[2] = rx_oam_pdu->src_mac[2];
				cpe_link_status_g.dpu_mac[3] = rx_oam_pdu->src_mac[3];
				cpe_link_status_g.dpu_mac[4] = rx_oam_pdu->src_mac[4];
				cpe_link_status_g.dpu_mac[5] = rx_oam_pdu->src_mac[5];
			}
			extoam_link_offer( &cpe_link_status_g, (unsigned char *)rx_oam_pdu, &tx_oam_buf );
			break;
		case LINK_ACK:
			extoam_link_ack( &cpe_link_status_g, (unsigned char *)rx_oam_pdu, &tx_oam_buf );
			break;
		case LINK_RESET:
			cpe_link_status_g.dpu_mac[0] = rx_oam_pdu->src_mac[0];
			cpe_link_status_g.dpu_mac[1] = rx_oam_pdu->src_mac[1];
			cpe_link_status_g.dpu_mac[2] = rx_oam_pdu->src_mac[2];
			cpe_link_status_g.dpu_mac[3] = rx_oam_pdu->src_mac[3];
			cpe_link_status_g.dpu_mac[4] = rx_oam_pdu->src_mac[4];
			cpe_link_status_g.dpu_mac[5] = rx_oam_pdu->src_mac[5];
			extoam_link_reset( &cpe_link_status_g, &extoam_queue_g, &tx_oam_buf );
			break;
		default:
			dbprintf(LOG_ERR,"unknown LINK_OP(0x%x)\n",link_op);
			break;
	}
	if ( tx_oam_buf ) {
		extoam_msg_send( tx_oam_buf , 60 );
		free_safe(tx_oam_buf);
	}
	
	return 0;
}

