/*
 * =====================================================================================
 *
 *       Filename:  gfast_handle.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 10時26分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */
extern struct list_head oam_handler_list_g;

struct oam_handle_t {
	unsigned short obj_id;
	unsigned short done;
	unsigned short (*object_get_handler)( unsigned char *response_buff);
	int (*object_set_handler)(unsigned short buf_len, unsigned char *buff);
	struct list_head node;
};				/* ----------  end of struct oam_handle_t  ---------- */

int extoam_handler_init( void );
int extoam_handler_deinit( void );
int extoam_handler_register( unsigned short object_id, unsigned short (*obj_get_handler)(unsigned char *response_buff), int (*obj_set_handler)(unsigned short buf_len, unsigned char *buff));
int extoam_handler_unregister( unsigned short object_id);
struct oam_handle_t *extoam_handle_find(unsigned short object_id);
