/*
 * =====================================================================================
 *
 *       Filename:  extoam_loopback.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年03月10日 15時03分59秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

#define EXTOAM_LOOPBACK_HEAD_LEN 2
struct loopback_t {
	unsigned short local_transaction_id;
	unsigned int timer_id;
	unsigned int *loopback_data;
};

int extoam_loopback_disptach( struct oam_pdu_t *rx_oam_pdu , unsigned short rx_oam_len );

int extoam_loopback_init( struct loopback_t *loopback);
int extoam_loopback_deinit( struct loopback_t *loopback);
int extoam_loopback_local_send( struct extoam_link_status_t *cpe_link_status);

