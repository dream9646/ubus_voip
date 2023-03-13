/*
 * =====================================================================================
 *
 *       Filename:  extoam_swdl.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月05日 17時29分08秒
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
#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "util_run.h"
#include "crc.h"
#include "cpuport.h"
#include "extoam.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
#include "extoam_queue.h"
#include "extoam_link.h"

extern int extoam_timer_qid_g;
extern struct extoam_link_status_t cpe_link_status_g;

struct sw_dl_proc_t cpe_rx_g = {{0},NULL,0,0,0,0,0};
struct sw_dl_proc_t cpe_tx_g = {{0},NULL,0,0,0,0,0};
FILE *dl_file=NULL;

int
extoam_swdl_file_crc_calc( FILE *fd, unsigned int *crc )
{
	return 0;
}

int
extoam_swdl_file_crc_check(struct sw_dl_proc_t *cpe_rx)
{
	unsigned int crc_accum_be = 0xFFFFFFFF;
	unsigned int crc_accum_le = 0xFFFFFFFF;

	//fd already closed !!
	//extoam_swdl_file_crc_calc( cpe_rx->fd, &crc );

	// check big endian
	crc_file_check(&crc_accum_be, cpe_rx->file_name, 1);
	if( ntohl(cpe_rx->crc) == ~crc_accum_be) {
		dbprintf(LOG_INFO, "OK: receive crc32=%x, file crc32_be=%x\n", ntohl(cpe_rx->crc), ~crc_accum_be);
		return 0;
	}

	// or check little endian
	crc_file_check(&crc_accum_le, cpe_rx->file_name, 0);
	if(ntohl(cpe_rx->crc) == ~crc_accum_le ) {
		dbprintf(LOG_INFO, "OK: receive crc32=%x, file crc32_le=%x\n", ntohl(cpe_rx->crc), ~crc_accum_le);
		return 0;
	}

	// crc eror
	dbprintf(LOG_ERR, "ERR: receive crc32=%x, file crc32_be=%x, crc32_le=%x\n", ntohl(cpe_rx->crc), ~crc_accum_be, ~crc_accum_le);
	return -1;
}

void *          
extoam_swdl_write_to_hw_by_shell_script(void *data)
{       
	struct sw_dl_proc_t *cpe_rx = (struct sw_dl_proc_t *)data;
	char cmd_buf[384];
        int ret;

	snprintf(cmd_buf, 384, "sh /etc/init.d/update_firmware.sh %s", cpe_rx->file_name);	
        
        ret=util_run_by_system(cmd_buf);
        if( WEXITSTATUS(ret) != 0 ) {
                dbprintf(LOG_ERR, "Error! func[%s] cmd[%s], ret=%d\n", __func__, cmd_buf, WEXITSTATUS(ret));
		//write to flash fail
		cpe_rx->finish_write=-1;
        }
	//finish write file to flash
	//should I reboot the system ??
	//should use try_active, if use, rc need set commit once
	cpe_rx->on_write = 0;
	cpe_rx->finish_write=1;
	dbprintf(LOG_ERR,"End write file to flash!\n");
	return 0;	
}
/*
int
extoam_swdl_file_write_flash(struct sw_dl_proc_t *cpe_rx)
{
	pthread_t system_thread;
	//int ret;

	if (pthread_create(&system_thread, NULL, extoam_swdl_write_to_hw_by_shell_script, (void *) cpe_rx)) {
		dbprintf(LOG_ERR, "Error creating pthread\n");
		//todo maybe need to do something else
		cpe_rx->on_write = 0;
		return -1;
	}
	//todo maybe need to do something else
	return 0;
}
*/

int
extoam_swdl_file_block_tx( struct sw_dl_proc_t *cpe_tx )
{
	unsigned char tx_oam_buff[BUFF_LEN];
	unsigned short len;

	len = extoam_msg_swdl_block_tx( cpe_tx, tx_oam_buff );
	if ( len < MAX_DATA_LENGTH ) 
		cpe_tx->size = ftell( cpe_tx->fd );

	cpe_tx->previous_block = cpe_tx->block;
	extoam_msg_send( tx_oam_buff, len );

	return 0;
}


int
extoam_swdl_file_tx_start( struct sw_dl_proc_t *cpe_tx, struct oam_pdu_t *rx_oam_pdu )
{
	char *file_name=NULL;

	if ( rx_oam_pdu == NULL ) {
		dbprintf(LOG_ERR,"OAM is NULL!\n");
		return -1;
	}

	if (cpe_tx->transaction_id ) {
		dbprintf(LOG_ERR,"another file upload is goning on.\n");
		return -1;
	}

	if ( (file_name = (char *)rx_oam_pdu->ext_oam.payload + sizeof(unsigned short)) != NULL ) {
		char *dirc,*basec, *bname, *dname;
		struct stat st;
		dirc = strdup(file_name);
		basec = strdup(file_name);
		bname = basename(basec);
		dname = dirname(dirc);
		memset(&cpe_tx->file_name,0,sizeof(char)*256);
		if ( *file_name != '.' && *dname == '.' ) {
			snprintf(cpe_tx->file_name,strlen(bname)+6, "/tmp/%s", bname); //get the file with default path
		}else if ( *file_name == '/' )
			strcpy(cpe_tx->file_name, file_name); //get the file with absolute path
		else{
			dbprintf(LOG_ERR,"File not found!\n");
			return -1;
		}
		if (stat(cpe_tx->file_name, &st ) != 0 ) { // test if file exist
			dbprintf(LOG_ERR,"File not found!\n");
			return -1;
		}
		cpe_tx->fd = fopen( cpe_tx->file_name, "r" );
		if ( !cpe_tx->fd ) {
			dbprintf(LOG_ERR,"cannot open file %s\n", cpe_tx->file_name );
			return -1;
		}else
			dbprintf(LOG_ERR,"Read file %s\n", cpe_tx->file_name );
	} else {
		dbprintf(LOG_ERR,"File not found!\n");
		return -1;
	}

	//File CRC caculate
	extoam_swdl_file_crc_calc(cpe_tx->fd, &cpe_tx->crc);

	cpe_tx->on = 1;
	cpe_tx->block = 0;
	cpe_tx->previous_block = 0;
	fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_TX_TIMEOUT, 500, NULL);

	/*
	fseek(cpe_tx->fd, 0L, SEEK_END);
	cpe_tx->size = ftell(cpe_tx->fd);
	fseek(cpe_tx->fd, 0L, SEEK_SET);
	*/
	cpe_tx->transaction_id = ntohs(rx_oam_pdu->ext_oam.transaction_id);
	gettimeofday( &cpe_tx->start, NULL );

	dbprintf(LOG_WARNING,"file upload Start!\n");

	return 0;
}


int
extoam_swdl_file_tx_end( struct sw_dl_proc_t *cpe_tx )
{
	if ( cpe_tx->transaction_id == 0) {
		dbprintf(LOG_ERR,"CPE upload transaction is not exist!\n");
		return -1;
	}

	if ( !cpe_tx->fd ) {
		dbprintf(LOG_ERR,"CPE file TX descriptor is not found!\n");
		return -1;
	}else
		fclose(cpe_tx->fd);

	cpe_tx->fd = NULL;
	cpe_tx->on = 0;
	cpe_tx->transaction_id = 0;
	gettimeofday( &cpe_tx->end, NULL );

	dbprintf(LOG_ERR,"software download End!\n");
	return 0;
}

static int
extoam_swdl_enddl_request( struct sw_dl_proc_t *cpe_tx )
{
	unsigned char tx_buff[BUFF_LEN];
	memset(tx_buff,0,sizeof(tx_buff));
	extoam_msg_swdl_enddl_request( cpe_link_status_g.link_id, cpe_tx->transaction_id, cpe_tx->size, cpe_tx->crc, tx_buff );
	extoam_msg_send( tx_buff, 60);
	return 0;
}

int
extoam_swdl_ack_rx( struct sw_dl_proc_t *cpe_tx, struct oam_pdu_t *rx_oam_pdu )
{
	struct file_transfer_ack_t *file_transfer_ack;
	if ( !rx_oam_pdu ) {
		dbprintf( LOG_WARNING, "RX OAM is NULL\n" );
		return -1;
	}

	if ( cpe_tx->transaction_id != ntohs(rx_oam_pdu->ext_oam.transaction_id )) {
		dbprintf(LOG_ERR,"Software download: transaction id not match!\n");
		if (cpe_tx->fd)
			fclose(cpe_tx->fd);
		return -1;
	}

	file_transfer_ack = (struct file_transfer_ack_t *)rx_oam_pdu->ext_oam.payload; 
	
	if ( file_transfer_ack->ack_resend == SW_DL_AR_ACK ) {
		if ( cpe_tx->size != 0 ) { //DPU file read finish
			extoam_swdl_enddl_request( cpe_tx );
			//fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_TX_END_TIMEOUT, 30000, NULL );
		}else if ( ntohs(file_transfer_ack->block_num) == (cpe_tx->block+1) ) {
			cpe_tx->retry = 0;
			cpe_tx->block++;
			extoam_swdl_file_block_tx( cpe_tx );
		}else
			return -1;	
	}else if ( ntohs(file_transfer_ack->ack_resend) == SW_DL_AR_RESEND ) {
		if ( ntohs(file_transfer_ack->block_num) == cpe_tx->block ) 
			extoam_swdl_file_block_tx( cpe_tx );
	}
	return 0;
}

int
extoam_swdl_ack_tx( struct sw_dl_proc_t *cpe_rx, struct oam_pdu_t *rx_oam_pdu )
{
	unsigned char tx_buff[BUFF_LEN]={0};
	struct file_transfer_t *file_transfer=NULL;
	if (*(unsigned short *)rx_oam_pdu->ext_oam.payload == SW_DL_WRT_REQ ) {
		extoam_msg_swdl_ack( cpe_link_status_g.link_id, ntohs(rx_oam_pdu->ext_oam.transaction_id) , 0, SW_DL_AR_ACK, tx_buff );
	}else{
		file_transfer = (struct file_transfer_t *)rx_oam_pdu->ext_oam.payload;
		extoam_msg_swdl_ack( cpe_link_status_g.link_id, ntohs(rx_oam_pdu->ext_oam.transaction_id) , ntohs(file_transfer->block_num) , SW_DL_AR_ACK, tx_buff );
	}
	//send tx_buff
	extoam_msg_send( tx_buff, 60);
	return 0;
}

int
extoam_swdl_file_rx_start( struct sw_dl_proc_t *cpe_rx, struct oam_pdu_t *rx_oam_pdu )
{
	char *fname = rx_oam_pdu->ext_oam.payload + sizeof(unsigned short);
	if ( rx_oam_pdu == NULL ) {
		dbprintf(LOG_ERR,"OAM is NULL!\n");
		return -1;
	}

	if (cpe_rx->transaction_id ) {
		dbprintf(LOG_ERR,"another CPE image download is goning on.\n");
		return -1;
	}

	snprintf( cpe_rx->file_name,256,"/tmp/%s", fname );
	cpe_rx->fd = fopen( cpe_rx->file_name, "w" );
	if ( !cpe_rx->fd ) {
		dbprintf(LOG_ERR,"cannot open file!\n");
		return -1;
	}

	cpe_rx->on = 1;
	cpe_rx->transaction_id = ntohs(rx_oam_pdu->ext_oam.transaction_id);
	cpe_rx->block = 0;
	gettimeofday( &cpe_rx->start, NULL );

	dbprintf(LOG_ERR,"software download Start!\n");
	return 0;
}


static int
extoam_swdl_file_rx_end( struct sw_dl_proc_t *cpe_rx )
{
	if ( cpe_rx->transaction_id == 0) {
		dbprintf(LOG_ERR,"CPE download transaction is not exist!\n");
		return -1;
	}


	gettimeofday( &cpe_rx->end, NULL );

	//File CRC check
	if(extoam_swdl_file_crc_check(cpe_rx) != 0 ) {
		dbprintf(LOG_ERR,"Error: Download file(%s) crc check fail!\n", cpe_rx->file_name);
		//todo maybe need to do something else
		return -1;
	}
	
	dbprintf(LOG_ERR,"Start write file to flash!\n");
	cpe_rx->on_write = 1;
	cpe_rx->finish_write=0;
	//extoam_swdl_file_write_flash(cpe_rx);

	return 0;
}

int
extoam_swdl_file_block_rx( struct sw_dl_proc_t *cpe_rx, struct oam_pdu_t *rx_oam_pdu , unsigned short size )
{

	struct file_transfer_t *block;
	unsigned short *block_num;

	if ( cpe_rx->transaction_id != ntohs(rx_oam_pdu->ext_oam.transaction_id )) {
		dbprintf(LOG_ERR,"Software download: transaction id not match!\n");
		if (cpe_rx->fd)
			fclose(cpe_rx->fd);
		return -1;
	}

	if ( cpe_rx->on != 1 ) {
		dbprintf(LOG_ERR,"Software download: the transaction is not start!\n");
		return -1;
	}

	block = (struct file_transfer_t *)rx_oam_pdu->ext_oam.payload ;

	if ( ntohs( block->block_num ) == cpe_rx->block+1 ) {
		cpe_rx->retry = 0;
		if ( size < 1400 ) {
			fwrite(&block->data,sizeof(unsigned char), size-33, cpe_rx->fd);
			cpe_rx->size = ftell( cpe_rx->fd);
			if ( !cpe_rx->fd ) {
				dbprintf(LOG_ERR,"CPE file TX descriptor is not found!\n");
				return -1;
			}else{
				fclose(cpe_rx->fd);
			}
		}else {
			fwrite(&block->data,sizeof(unsigned char), size-33, cpe_rx->fd);
		}
		cpe_rx->block++;
		//send transfer ACK
		extoam_swdl_ack_tx( cpe_rx, rx_oam_pdu );

		//set timer to check timeout (0.5 sec)
		if ( cpe_rx->fd ) {
			block_num = malloc_safe(sizeof(unsigned short));
			*block_num = cpe_rx->block;
			fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_RX_TIMEOUT, 500, (void *)block_num );
		}
		//cpe_rx->rx_ack_retry++;
	}

	return 0;
}

int
extoam_swdl_enddl_response( struct sw_dl_proc_t *cpe_rx, unsigned char return_code) 
{
	unsigned char tx_buff[BUFF_LEN];
	memset(tx_buff,0,sizeof(tx_buff));
	extoam_msg_swdl_enddl_response( cpe_link_status_g.link_id, cpe_rx->transaction_id, return_code, tx_buff );
	extoam_msg_send( tx_buff, 60);
	cpe_rx->on = 0;
	cpe_rx->transaction_id = 0;
	return 0;
}


int
extoam_swdl_cmd_rx( struct oam_pdu_t *rx_oam_pdu , unsigned short rx_oam_len ) 
{

	unsigned short *block_num;
	if ( !rx_oam_pdu ) {
		dbprintf( LOG_ERR, "rx oam pdu is NULL\n");
		return -1;
	}

	switch ( ntohs( *(unsigned short *) rx_oam_pdu->ext_oam.payload ) ) {
		case SW_DL_WRT_REQ: 
			block_num = malloc_safe(sizeof(unsigned short));
			*block_num = 0;
			extoam_swdl_file_rx_start( &cpe_rx_g, rx_oam_pdu );
			extoam_swdl_ack_tx( &cpe_rx_g, rx_oam_pdu );
			fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_RX_TIMEOUT, 500, (void *)block_num );
			break;
		case SW_DL_RD_REQ: 
			if (extoam_swdl_file_tx_start( &cpe_tx_g, rx_oam_pdu ) < 0)
				break;
			extoam_swdl_file_block_tx( &cpe_tx_g);
			break;
		case SW_DL_TRAN_DATA: 
			extoam_swdl_file_block_rx( &cpe_rx_g, rx_oam_pdu , rx_oam_len);
			break;
		case SW_DL_TRAN_ACK: 
			extoam_swdl_ack_rx( &cpe_tx_g, rx_oam_pdu );
			break;
		case SW_DL_END_DL_REQ: 
			if ( cpe_rx_g.on_write == 1 ) {
				//send BUSY response to DPU
				//return 0;
				extoam_swdl_enddl_response( &cpe_rx_g, SW_DL_RET_BUSY); 
			}else{
				struct end_dl_request_t *end_dl_request;
				end_dl_request = (struct end_dl_request_t *)rx_oam_pdu->ext_oam.payload; 
				cpe_rx_g.crc = ntohl(end_dl_request->crc );
				extoam_swdl_file_rx_end( &cpe_rx_g );
				extoam_swdl_enddl_response( &cpe_rx_g, SW_DL_RET_BUSY); 
			}
			
			break;
		case SW_DL_END_DL_RESP: 
			extoam_swdl_file_tx_end( &cpe_tx_g );
			break;
		case SW_DL_ACTIVATE_IMG_REQ: 
			break;
		case SW_DL_COMMIT_IMG_REQ: 
			break;
		default:
			dbprintf( LOG_ERR, "Software download operation not allowed!\n");
	}	

	return 0;
}

