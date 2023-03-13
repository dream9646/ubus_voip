#define MAX_STP_PORTS	4

extern struct cpuport_desc_t stp_port[MAX_STP_PORTS];
extern int number_of_ports;

int bridge_start (void);
void bridge_shutdown(void);
int bridge_rx_bpdu (UID_MSG_T* msg, size_t msgsize);
int bridge_tx_bpdu (int port_index, unsigned char *bpdu, size_t bpdu_len);
