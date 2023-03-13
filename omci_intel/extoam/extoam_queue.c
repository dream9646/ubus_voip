/*
 * =====================================================================================
 *
 *       Filename:  extoam_queue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月02日 17時45分34秒
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
#include "list.h"
#include "util.h"
#include "extoam.h"
#include "extoam_cmd.h"
#include "extoam_queue.h"

struct extoam_queue_t extoam_queue_g; 

int
extoam_queue_init (struct extoam_queue_t *extoam_queue )
{
	memset( extoam_queue, 0, sizeof(struct extoam_queue_t)); 
	INIT_LIST_HEAD( &extoam_queue->node );
	extoam_queue->count = 0; 
	return 0;
}

int
extoam_queue_add ( struct extoam_queue_t *extoam_queue , unsigned short transaction_id )
{
	struct extoam_cmd_t *pos, *n, *extoam_cmd;


	if ( extoam_queue->count > 4096 ) {
		dbprintf(LOG_ERR,"Command queue is full.\n");
		return -1;
	}

	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_entry_safe( pos, n, &extoam_queue->node, node) {
		if ( pos->transaction_id == transaction_id ) 
			return 0;
	}

	extoam_cmd = (struct extoam_cmd_t *)malloc_safe(sizeof( struct extoam_cmd_t ));

	extoam_cmd->transaction_id = transaction_id;  	
	//extoam_cmd->object_id = object_id;  	
	//extoam_cmd->op = op;  	
	gettimeofday( &extoam_cmd->time, NULL );

	INIT_LIST_NODE( &extoam_cmd->node );
	list_add_tail( &extoam_cmd->node, &extoam_queue->node );
	extoam_queue->count++;
	return 0;
}

int
extoam_queue_del (struct extoam_queue_t *extoam_queue , unsigned short transaction_id )
{
	struct list_head *pos, *n;
	struct extoam_cmd_t *extoam_cmd;

	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_safe( pos, n, &extoam_queue->node ) {
		extoam_cmd = list_entry( pos, struct extoam_cmd_t, node );
		if ( extoam_cmd->transaction_id == transaction_id  ) {
			list_del(pos);
			free_safe( extoam_cmd );
			if ( extoam_queue->count > 0 )
				extoam_queue->count--;
			return 0;
		}
	}
	return 0;
}

struct extoam_cmd_t *
extoam_queue_transaction_search( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ) 
{
	struct extoam_cmd_t *pos, *n ;
	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_entry_safe( pos, n, &extoam_queue->node, node) {
		if ( pos->transaction_id == transaction_id ) {
			return pos;
		}
	}
	return NULL;
}

int
extoam_queue_transaction_exist( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ) 
{
	struct extoam_cmd_t *pos, *n ;
	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_entry_safe( pos, n, &extoam_queue->node, node) {
		if ( pos->transaction_id == transaction_id ) {
			return 1;
		}
	}
	return 0;
}

int
extoam_queue_transaction_finish( struct extoam_queue_t *extoam_queue , unsigned short transaction_id ) 
{
	struct extoam_cmd_t *pos, *n ;
	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_entry_safe( pos, n, &extoam_queue->node, node) {
		if ( pos->transaction_id == transaction_id ) {
			pos->finish = 1;
			return 1;
		}
	}
	return 0;
}

int
extoam_queue_update (struct extoam_queue_t *extoam_queue )
{
	return 0;
}

int
extoam_queue_flush (struct extoam_queue_t *extoam_queue )
{
	struct list_head *pos, *n;
	struct extoam_cmd_t *extoam_cmd;

	if (list_empty(&extoam_queue->node))
		return 0;
	//Add the OAM to the list if the transaction id is not in the list
	list_for_each_safe( pos, n, &extoam_queue->node ) {
		extoam_cmd = list_entry( pos, struct extoam_cmd_t, node );
		list_del(pos);
		free_safe( extoam_cmd );
	}
	extoam_queue->count = 0;
	return 0;
}

