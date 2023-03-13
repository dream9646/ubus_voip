/*
 * =====================================================================================
 *
 *       Filename:  extoam_cmd.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月25日 11時03分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

struct extoam_cmd_t {
	unsigned short transaction_id;
//	unsigned short object_id
	struct timeval time;
	struct list_head node;
	unsigned char finish;
};

int extoam_cmd_dispatch ( struct oam_pdu_t  *rx_oam_pdu , unsigned short rx_oam_len, struct list_head *obj_handler_q );
