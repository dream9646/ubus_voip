/*
 * =====================================================================================
 *
 *       Filename:  extoam_link.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月03日 11時57分48秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */
#define LINK_UNLINK	0x0
#define LINK_DISCOVER	0x1
#define LINK_OFFER	0x2
#define LINK_REQUEST	0x3
#define LINK_ACK	0x4
#define LINK_RESET	0x5
//link state: 0 -> non-link
//	      1 -> discover 
//	      2 -> offer 
//	      3 -> request 
//	      4 -> ack
struct  extoam_link_status_t {
	unsigned char dpu_mac[6];
	unsigned char dpu_port;
	unsigned short link_id;
	unsigned char link_state;
	unsigned char trap_state;
	struct timeval last_phase;
	unsigned char link_state_for_timer; //in order to check if the link state is changed correctly in the future
	int last_link_state_timer_id; //in order to check if the link state is changed correctly in the future
	int last_reset_timer_id; 
	unsigned short local_transaction_id;
};

int extoam_link_init(struct  extoam_link_status_t *cpe_link_status);
int extoam_link_reset(struct extoam_link_status_t *cpe_link_status, struct extoam_queue_t *extoam_queue, unsigned char **oam_tx_buff);
int extoam_link_dispatch ( struct oam_pdu_t  *rx_oam_pdu , struct list_head *obj_handler_q );
int extoam_link_discover(struct extoam_link_status_t *cpe_link_status, unsigned char **oam_tx_buff);
