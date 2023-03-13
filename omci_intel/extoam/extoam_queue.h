/*
 * =====================================================================================
 *
 *       Filename:  extoam_queue.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月02日 17時47分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies:w
 *
 *
 * =====================================================================================
 */

struct extoam_queue_t {
	unsigned short count;
	struct list_head node;
};

int extoam_queue_init (struct extoam_queue_t *extoam_queue );
int extoam_queue_add ( struct extoam_queue_t *extoam_queue , unsigned short transaction_id );
int extoam_queue_del (struct extoam_queue_t *extoam_queue , unsigned short transaction_id );
int extoam_queue_transaction_exist ( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ); 
int extoam_queue_update (struct extoam_queue_t *extoam_queue );
struct extoam_cmd_t* extoam_queue_transaction_search( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ); 
int extoam_queue_transaction_finish( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ); 
int extoam_queue_flush (struct extoam_queue_t *extoam_queue );
