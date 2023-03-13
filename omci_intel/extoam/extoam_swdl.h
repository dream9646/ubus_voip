/*
 * =====================================================================================
 *
 *       Filename:  extoam_swdl.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月05日 17時30分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */
struct sw_dl_proc_t {
	char file_name[256];
	FILE *fd;
	long file_pos;
	unsigned int size;
	unsigned short block;
	unsigned short previous_block;
	unsigned short final_block;
	unsigned short transaction_id;
	unsigned int crc;
	struct timeval start;
	struct timeval end;
	unsigned char on; //down/upload is going on or not
	unsigned char retry;
	unsigned char on_write; //write to flash
	unsigned char finish_write; //finish write to flash
	unsigned short last_block_len;
};

int extoam_swdl_file_block_tx( struct sw_dl_proc_t *cpe_tx );
int extoam_swdl_cmd_rx( struct oam_pdu_t *rx_oam_pdu , unsigned short rx_oam_len );
