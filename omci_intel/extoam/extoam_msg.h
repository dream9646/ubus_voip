/*
 * =====================================================================================
 *
 *       Filename:  extoam_msg.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  西元2015年02月03日 14時56分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

int extoam_msg_ext_header_prep( unsigned short ext_code, unsigned short link_id, unsigned short transaction_id , unsigned char *buff);
int extoam_msg_header_prep(unsigned short ext_opcode, unsigned short transaction_id, unsigned char *buff);
int extoam_tx_if_mac_get(char *ifname,unsigned char mac[6]);
unsigned short extoam_msg_swdl_block_tx( struct sw_dl_proc_t *cpe_tx, unsigned char *buff );
int extoam_msg_swdl_ack( unsigned short link_id, unsigned short transaction_id ,unsigned short block_num, unsigned char ack_resend, unsigned char *buff );
int extoam_msg_swdl_enddl_request(unsigned short link_id, unsigned short transaction_id, unsigned int file_size, unsigned short crc, unsigned char *buff);
int extoam_msg_swdl_enddl_response(unsigned short link_id,unsigned short transaction_id, unsigned char response_code , unsigned char *buff);
int extoam_msg_send( unsigned char *tx_buff, unsigned short len );
