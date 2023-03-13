/*
 * =====================================================================================
 *
 *       Filename:  gfast_oam.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月08日 18時46分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */
#define	SLOW_PROTO		0x8809	/*  */
#define	TYPE_OAM		0x03	/*  */
#define	ORG_SPECIFIC_CODE	0xFE	/*  */

#define	EXT_OP_LINK_DISCOVER 	0x1	/*  */
#define	EXT_OP_SW_DL		0x2	/*  */
#define	EXT_OP_EVENT		0x3	/*  */
#define	EXT_OP_LOOPBACK		0x4	/*  */
#define	EXT_OP_GET_REQUEST	0x5	/*  */
#define	EXT_OP_GET_RESPONSE	0x6	/*  */
#define	EXT_OP_SET_REQUEST	0x7	/*  */
#define	EXT_OP_SET_RESPONSE	0x8	/*  */

#define	EXT_OP_SET_RET_OK	0x1	/*  */
#define	EXT_OP_SET_RET_FAIL	0x2	/*  */
#define	EXT_OP_SET_RET_BUSY	0x3	/*  */

#define TIMER_EVENT_CHECK_LINK_STAT	0x1
#define TIMER_EVENT_RM_QUEUE_CMD	0x2 
#define TIMER_EVENT_FILE_RX_TIMEOUT	0x3 
#define TIMER_EVENT_FILE_TX_TIMEOUT	0x4 
#define TIMER_EVENT_FILE_TX_END_TIMEOUT	0x5 
#define TIMER_EVENT_CHECK_LINK_RESET	0x6
#define TIMER_EVENT_LOOPBACK_CHECK	0x7


#define	SW_DL_WRT_REQ		0x01	/*  */
#define	SW_DL_RD_REQ		0x02	/*  */
#define	SW_DL_TRAN_DATA		0x03	/*  */
#define	SW_DL_TRAN_ACK		0x04	/*  */
#define	SW_DL_ERR		0x05	/*  */
#define	SW_DL_END_DL_REQ	0x06	/*  */
#define	SW_DL_END_DL_RESP	0x07	/*  */
#define	SW_DL_ACTIVATE_IMG_REQ	0x08	/*  */
#define	SW_DL_ACTIVATE_IMG_RESP	0x09	/*  */
#define	SW_DL_COMMIT_IMG_REQ	0x0a	/*  */
#define	SW_DL_COMMIT_IMG_RESP	0x0b	/*  */

#define SW_DL_AR_ACK	0x0
#define SW_DL_AR_RESEND	0x1

#define	SW_DL_RET_OK		0x00	/*  */
#define	SW_DL_RET_BUSY		0x01	/*  */
#define	SW_DL_RET_CRC_ERR	0x02	/*  */

#define MAX_OBJ_NUM	100
#define MAX_DATA_LENGTH	1400
#define BUFF_LEN	1500
#define MAC_LEN		6
#define ETHERTYPE_LEN	2
#define OAM_HEAD_LEN	7	
#define EXTOAM_HEAD_LEN	6

#define	OBJ_ID_ITAS_ETH_IF_STATUS	0x1	/*  */
#define	OBJ_ID_ITAS_ETH_IF_CFG		0x2	/*  */


/*
#define LINK_DISCOVER	0x1
#define LINK_OFFER	0x2
#define LINK_REQUEST	0x3
#define LINK_ACK	0x4
#define LINK_RESET	0x5
*/

struct itas_eth_status_t {
	unsigned char op_status;
 	unsigned char duplex;                   
	unsigned char autonego;                
	unsigned char speed;                  
	unsigned int in_pkt_cnt;
	unsigned int in_ucast_cnt;
	unsigned int in_nucast_cnt;
	unsigned int in_discard_cnt;
	unsigned int in_err_cnt;
	unsigned int out_pkt_cnt;
	unsigned int out_ucast_cnt;
	unsigned int out_nucast_cnt;
	unsigned int out_discard_cnt;
	unsigned int out_err_cnt;
};

struct itas_eth_cfg_t {
	unsigned char op_status;
 	unsigned char duplex;                   
	unsigned char autonego;                
	unsigned char speed;                  
};

struct ext_link_discover_t {
	unsigned short link_op;
	unsigned short link_number;
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;

struct ext_get_request_t {
	unsigned short obj_num;
	unsigned short obj_id[MAX_OBJ_NUM];
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;

struct ext_get_response_t {
	unsigned short obj_id;
	unsigned short obj_len;
	unsigned char obj[MAX_DATA_LENGTH];
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;

struct ext_set_request_t {
	unsigned short obj_id;
	unsigned short obj_len;
	unsigned char obj[MAX_DATA_LENGTH];
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;

struct ext_set_response_t {
	unsigned short return_code;
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;


struct file_transfer_t {
	unsigned short dl_op;
	unsigned short csum;
	unsigned short block_num;
	unsigned char data[MAX_DATA_LENGTH];
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;

struct file_transfer_ack_t {
	unsigned short dl_op;
	unsigned char ack_resend;
	unsigned short block_num;
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */

struct end_dl_request_t {
	unsigned short dl_op;
	unsigned int file_size;
	unsigned int crc;
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;				/* ----------  end of struct end_dl_request_t ---------- */

struct end_dl_response_t {
	unsigned short dl_op;
	unsigned char response_code;
}__attribute__((packed));				/* ----------  end of struct file_transfer_ack  ---------- */;				/* ----------  end of struct end_dl_response_t ---------- */

struct ext_oam_t {
	unsigned short link_id;
	unsigned short transaction_id;
	unsigned short ext_code;
	unsigned char payload[MAX_DATA_LENGTH];
} __attribute__((packed));

struct oam_pdu_t {
	char dst_mac[6];
	char src_mac[6];
	unsigned short ether_type;
	unsigned char subtype;
	unsigned short flags;
	unsigned char code;
	unsigned char oui[3];
	struct ext_oam_t ext_oam;
} __attribute__((packed));

enum extoam_command_t {
	EXTOAM_CMD_TERMINATE
};

struct extoam_cmd_msg_t {
	enum extoam_command_t cmd;
	struct list_head q_entry;
};
