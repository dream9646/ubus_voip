/*
 * =====================================================================================
 *
 *       Filename:  gfast_handle.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 11時06分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

#include "list.h"
#include "util.h"
#include "extoam_handle.h"
#include "extoam_object.h"


struct list_head oam_handler_list_g;
extern struct object_handler_t object_handles[];

int
extoam_handler_init( void )
{
	int i = 0;
	INIT_LIST_HEAD(&oam_handler_list_g);
	for ( i = 0 ; object_handles[i].obj_id != 0 ; i++ ) {
		extoam_handler_register( object_handles[i].obj_id, 
				object_handles[i].object_get_handler, object_handles[i].object_set_handler);
	}
	return 0;
}

int
extoam_handler_deinit( void )
{
	struct list_head *pos, *n;
	struct oam_handle_t *oam_handler;

	list_for_each_safe( pos, n, &oam_handler_list_g ) {
		oam_handler = list_entry(pos, struct oam_handle_t, node);
		list_del(pos);
		free_safe(oam_handler); 
	}
	return 0;
}		/* -----  end of function gfast_oam_handler_deinit  ----- */

int 
extoam_handler_register( unsigned short object_id, unsigned short (*obj_get_handler)(unsigned char *response_buff), int (*obj_set_handler)(unsigned short buf_len, unsigned char *buff))
{
	struct oam_handle_t *oam_handler;
	oam_handler = malloc_safe(sizeof(struct oam_handle_t));
	oam_handler->obj_id = object_id;
	oam_handler->object_get_handler = obj_get_handler;
	oam_handler->object_set_handler = obj_set_handler;
	list_add_tail( &oam_handler->node, &oam_handler_list_g);
	return 0;
}

int
extoam_handler_unregister( unsigned short object_id)
{
	struct list_head *pos, *n;
	struct oam_handle_t *oam_handler;
	list_for_each_safe( pos, n, &oam_handler_list_g){
		oam_handler = list_entry(pos, struct oam_handle_t, node);
		if (oam_handler->obj_id == object_id ) {
			list_del(pos);
			free_safe(oam_handler); 
			break;
		}
	}
	return 0;
}

struct oam_handle_t *
extoam_handle_find(unsigned short object_id)
{
	struct list_head *pos, *n;
	struct oam_handle_t *oam_handler;
	list_for_each_safe( pos, n, &oam_handler_list_g){
		oam_handler = list_entry(pos, struct oam_handle_t, node);
		if (oam_handler->obj_id == object_id ) {
			return oam_handler; 
		}
	}
	return NULL;
}
