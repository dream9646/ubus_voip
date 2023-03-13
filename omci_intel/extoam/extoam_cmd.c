/*
 * =====================================================================================
 *
 *       Filename:  extoam_cmd.c
 *
 *    Description
 *
 *        Version:  1.0
 *        Created:  西元2015年02月04日 18時13分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "extoam.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
#include "extoam_handle.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_loopback.h"

extern struct extoam_queue_t extoam_queue_g; 
extern int extoam_timer_qid_g;
extern struct extoam_link_status_t cpe_link_status_g;
extern struct sw_dl_proc_t cpe_rx_g;

static int
extoam_cmd_get_exec( struct oam_pdu_t  *rx_oam_pdu )
{
	struct oam_pdu_t *tx_oam_pdu;
	unsigned char oam_tx_buff[BUFF_LEN];	
	unsigned short *rx_obj_id_ptr ;
	unsigned char *tx_obj_ptr;	
	unsigned short i = 0,  obj_len = 0;
	unsigned short obj_num = 0 ;
	struct oam_handle_t *oam_handle; 
	unsigned int total_obj_len = 0;
	unsigned short obj_head_len = 4; // total len of obj id field + obj len field


	if ( !rx_oam_pdu ) {
		dbprintf( LOG_ERR, "OAM PDU is NULL\n" );
		return -1;
	}

	//rx pdu 
	obj_num = ntohs(*( unsigned short *)rx_oam_pdu->ext_oam.payload);
	rx_obj_id_ptr =  (unsigned short *)(rx_oam_pdu->ext_oam.payload + sizeof(unsigned short)); //to get object id

	//tx pdu
	extoam_msg_ext_header_prep( EXT_OP_GET_RESPONSE, cpe_link_status_g.link_id, ntohs(rx_oam_pdu->ext_oam.transaction_id), oam_tx_buff );
	tx_oam_pdu = (struct oam_pdu_t  *)oam_tx_buff;
	tx_obj_ptr = tx_oam_pdu->ext_oam.payload; //to get object id

	for ( i = 0 ; i < obj_num ; i++ ) {
		//GET operation
		//
		if( ( oam_handle = extoam_handle_find(ntohs(*rx_obj_id_ptr ))) != NULL ) {
			if ( oam_handle->object_get_handler != NULL ) {
				obj_len = oam_handle->object_get_handler( tx_obj_ptr + obj_head_len );
				//Full object length including Object id, object length, object value these 3 fields length
				*(unsigned short *)tx_obj_ptr = htons(oam_handle->obj_id);
				*(unsigned short *)( tx_obj_ptr+2 ) = htons(obj_len);
				total_obj_len += ( obj_head_len + obj_len ) ;	
				if ( total_obj_len > MAX_DATA_LENGTH ) {
					dbprintf(LOG_ERR, "OAM GET operation response message larger than 1400!\n");
					//extoam_queue_del (&extoam_queue_g , oam_pdu->ext_oam.transaction_id );
					//send error message(Event OAM) to DPU
					return -1;
				}
				rx_obj_id_ptr++;
				tx_obj_ptr += ( obj_head_len + obj_len);  
				obj_len = 0;
			}
		}
	}

	//send GET operation buffer
	extoam_msg_send( oam_tx_buff, MAC_LEN*2 + ETHERTYPE_LEN+OAM_HEAD_LEN+EXTOAM_HEAD_LEN+total_obj_len );

	extoam_queue_transaction_finish( &extoam_queue_g , ntohs(rx_oam_pdu->ext_oam.transaction_id) ); 

	//set timer to remove command from the transaction queue
	//extoam_queue_del (&extoam_queue_g , oam_pdu->ext_oam.transaction_id );
	unsigned short *tr_id = malloc_safe(sizeof(unsigned short));
	*tr_id = ntohs(rx_oam_pdu->ext_oam.transaction_id);
	fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_RM_QUEUE_CMD, 20000, (void *)tr_id );
	
	return 0;
}

static int
extoam_cmd_set_exec( struct oam_pdu_t  *rx_oam_pdu, unsigned short rx_oam_len )
{
	struct ext_set_request_t *ext_set_request;
	unsigned char tx_oam_buff[BUFF_LEN];	
	struct oam_pdu_t *tx_oam_pdu;
	struct oam_handle_t *oam_handle; 
	unsigned char *start, *pos;
	int fail = 0;
	unsigned short oam_len;

	if ( !rx_oam_pdu ) {
		dbprintf( LOG_ERR, "OAM PDU is NULL\n" );
		return -1;
	}

	oam_len = rx_oam_len - (MAC_LEN*2 + ETHERTYPE_LEN + OAM_HEAD_LEN + EXTOAM_HEAD_LEN);
	ext_set_request  = (struct ext_set_request_t *)rx_oam_pdu->ext_oam.payload;

	start = (unsigned char *)rx_oam_pdu->ext_oam.payload;
	pos = (unsigned char *)rx_oam_pdu->ext_oam.payload;

	while ( ext_set_request->obj_id != 0 && ( pos-start < oam_len)) {
		if( (oam_handle = extoam_handle_find(ntohs(ext_set_request->obj_id))) != NULL ) {
			if ( oam_handle->object_set_handler == NULL ) {
				dbprintf( LOG_ERR, "The object SET operation handler is NULL!\n" );
				fail = 1;
				break;
			}
			if ( oam_handle->object_set_handler( ntohs(ext_set_request->obj_len), ext_set_request->obj ) != 0 ) {
				dbprintf( LOG_ERR, "The object SET operation fail!\n" );
				fail = 1;
				break;
			}
			pos +=  sizeof(unsigned short)*2 + ext_set_request->obj_len;
			ext_set_request =(struct ext_set_request_t *)pos;
		}
	}

	extoam_msg_ext_header_prep( EXT_OP_SET_RESPONSE, cpe_link_status_g.link_id, ntohs(rx_oam_pdu->ext_oam.transaction_id), tx_oam_buff );
	tx_oam_pdu = (struct oam_pdu_t *)tx_oam_buff;
	tx_oam_pdu->ext_oam.ext_code =  htons(EXT_OP_SET_RESPONSE);

	if ( fail == 0 )
		*(unsigned short *)&tx_oam_pdu->ext_oam.payload[0] = htons(EXT_OP_SET_RET_OK); 
	else
		*(unsigned short *)&tx_oam_pdu->ext_oam.payload[0] = htons(EXT_OP_SET_RET_FAIL); 

	//mark the command finish in the transaction queue
	extoam_queue_transaction_finish( &extoam_queue_g , rx_oam_pdu->ext_oam.transaction_id ); 
	extoam_msg_send( tx_oam_buff, 60);

	//set timer to remove command from the transaction queue
	unsigned short *tr_id = malloc_safe(sizeof(unsigned short));
	*tr_id = ntohs(rx_oam_pdu->ext_oam.transaction_id);
	fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_RM_QUEUE_CMD, 30000, (void *)tr_id );
	return 0;
}

static int
extoam_cmd_get_dispatch ( struct oam_pdu_t  *rx_oam_pdu, struct list_head *obj_handler_q, struct extoam_queue_t *extoam_queue )
{
	if ( !extoam_queue_transaction_exist( extoam_queue, ntohs(rx_oam_pdu->ext_oam.transaction_id) )) {
		if (extoam_queue_add ( extoam_queue, ntohs(rx_oam_pdu->ext_oam.transaction_id) ) < 0 ) {
			//todo: send command queue full event

			return 0;
		}
		extoam_cmd_get_exec( rx_oam_pdu );
	}
	return 0;
}

static int
extoam_cmd_set_dispatch ( struct oam_pdu_t  *rx_oam_pdu, struct list_head *obj_handler_q, 
		struct extoam_queue_t *extoam_queue, unsigned short rx_oam_len )
{
	if ( extoam_queue_transaction_exist( extoam_queue, ntohs(rx_oam_pdu->ext_oam.transaction_id) )) {
		//SET cmd is found in the queue and return busy response
		unsigned char tx_oam_buff[BUFF_LEN];	
		struct oam_pdu_t *tx_oam_pdu;
		struct ext_set_response_t *ext_set_response; 
		extoam_msg_ext_header_prep( EXT_OP_SET_RESPONSE, cpe_link_status_g.link_id, rx_oam_pdu->ext_oam.transaction_id, tx_oam_buff);
		tx_oam_pdu = (struct oam_pdu_t *)tx_oam_buff;
		ext_set_response = (struct ext_set_response_t *)tx_oam_pdu->ext_oam.payload;
		ext_set_response->return_code = htons(EXT_OP_SET_RET_BUSY); 
		/* send oam_tx_buff */
		extoam_msg_send( tx_oam_buff, 60);
		return 0;
	}
	//SET operation
	extoam_cmd_set_exec( rx_oam_pdu , rx_oam_len );
	return 0;
}

int
extoam_cmd_dispatch ( struct oam_pdu_t  *rx_oam_pdu , unsigned short rx_oam_len, struct list_head *obj_handler_q )
{
	unsigned short ext_op_code = ntohs(rx_oam_pdu->ext_oam.ext_code);

	if ( ext_op_code != EXT_OP_LINK_DISCOVER && 
			(rx_oam_pdu->ext_oam.link_id != cpe_link_status_g.link_id || 
			 rx_oam_pdu->ext_oam.transaction_id == 0) ) {
		dbprintf(LOG_ERR,"uknown link id(0x%x) or transaction_id(0x%x)\n"
				,rx_oam_pdu->ext_oam.link_id , rx_oam_pdu->ext_oam.transaction_id);
		return 0;
	}

	switch ( ext_op_code ) {
		case EXT_OP_LINK_DISCOVER:
			extoam_link_dispatch ( rx_oam_pdu , obj_handler_q );
			break;
		case EXT_OP_SET_REQUEST:
			extoam_cmd_set_dispatch ( rx_oam_pdu, obj_handler_q, &extoam_queue_g, rx_oam_len );
			break;	
		case EXT_OP_GET_REQUEST: 
			extoam_cmd_get_dispatch ( rx_oam_pdu, obj_handler_q, &extoam_queue_g );
			break;
		case EXT_OP_SW_DL: 
			extoam_swdl_cmd_rx( rx_oam_pdu, rx_oam_len );
			break;
		case EXT_OP_LOOPBACK:
			extoam_loopback_disptach( rx_oam_pdu , rx_oam_len ); 
			break;	
		default:
			dbprintf(LOG_ERR,"unsupport extended op code 0x%x\n",rx_oam_pdu->ext_oam.ext_code);

	}

	return 0;
}
