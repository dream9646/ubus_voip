/*
 * =====================================================================================
 *
 *       Filename:  extoam_object.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年03月12日 19時25分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */


struct object_handler_t {
	unsigned short obj_id;
	unsigned short type; //0:string 1:int
	unsigned short (*object_get_handler)( unsigned char *response_buff);
	int (*object_set_handler)(unsigned short buf_len, unsigned char *buff);
};
