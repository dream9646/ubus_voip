/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : cpuport
 * File    : cpuport_sniffer.h
 *
 ******************************************************************/

#ifndef __CPUPORT_SNIFFER_H__
#define __CPUPORT_SNIFFER_H__


/* cpuport_sniffer.c */
int cpuport_sniffer_config_set(int fd, int argc, char *argv[]);
void cpuport_sniffer_config_get(int fd);
void cpuport_sniffer_print_pkthdr(int fd, char *prompt, unsigned char *pkt, int pkt_len, int is_ip);
void cpuport_sniffer_print_pkt(int fd, char *prompt, unsigned char *pkt, int pkt_len, int is_ip);
void cpuport_sniffer_dbprint_pkt(int debug_level, char *prompt, unsigned char *pkt, int pkt_len, int is_ip);

#endif
