#define RSTP_EVENT_STP_IN_ONE_SEC 0
#define RSTP_EVENT_PORT_CONNECTION_CHECK 1


enum rstp_cmd_t {
	RSTP_CMD_ME_ADD_MSG,
	RSTP_CMD_ME_UPDATE_MSG,
	RSTP_CMD_ME_DEL_MSG,
	RSTP_CMD_TERMINATE
};

struct rstp_cmd_msg_t {
	enum rstp_cmd_t cmd;
	unsigned int msglen;
	unsigned char *usr_data;
	unsigned short classid;
	unsigned short meid;
	struct list_head q_entry;
};
extern int rstp_pkt_recv_qid_g ;
extern int rstp_cmd_qid_g ;
extern int rstp_qsetid_g ;
extern int rstp_timer_qid_g ;

extern int rstptask_loop_g ;

/* rstp.c */
int rstp_port_link_status_change(int port_index);
int rstp_pkt_filter(struct cpuport_info_t *pktinfo);
int rstp_init(void);
int rstp_shutdown(void);
int rstp_start(void);
int rstp_stop(void);
