/* rstp_omci.c */
int rstp_omci_stp_bridge_instance_check(void);
int rstp_omci_stp_bridge_service_enable(void);
int rstp_omci_stp_bridge_priority_get(unsigned short *bridge_priority);
int rstp_omci_stp_bridge_max_age_get(unsigned short *max_age);
int rstp_omci_stp_bridge_hello_time_get(unsigned short *hello_time);
int rstp_omci_stp_bridge_forward_delay_get(unsigned short *forward_delay);
int rstp_omci_stp_port_identifier_get(struct cpuport_desc_t desc, unsigned short *port_id);
int rstp_omci_stp_port_priority_get( struct cpuport_desc_t desc, unsigned short *priority); 
int rstp_omci_stp_port_path_cost_get(struct cpuport_desc_t desc, unsigned short *path_cost);
int rstp_omci_port_me_to_desc(struct me_t *port_me, struct cpuport_desc_t *desc);
int rstp_omci_notifychain_handler(unsigned char event, unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr);
void rstp_omci_notify_chain_init(void);
void rstp_omci_notify_chain_finish(void);
