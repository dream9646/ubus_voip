/* rstp_cfg.c */
int rstp_cfg_stp_enable(void);
int rstp_cfg_stp_disable(void);
int rstp_cfg_br_prio_set(unsigned short br_prio);
int rstp_cfg_br_maxage_set(unsigned short br_maxage);
int rstp_cfg_br_hello_time_set(unsigned short br_hello_time);
int rstp_cfg_br_fdelay_set(unsigned short br_fdelay);
int rstp_cfg_prt_prio_set(int port_index, unsigned long value);
int rstp_cfg_prt_pcost_set(int port_index, unsigned long value);
int rstp_cfg_prt_mcheck_set(int port_index);
