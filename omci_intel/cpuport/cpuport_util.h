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
 * File    : cpuport_util.h
 *
 ******************************************************************/

#ifndef __CPUPORT_UTIL_H__
#define __CPUPORT_UTIL_H__

#define RET_OK					0
#define RET_YES					1
#define RET_NO					0

struct cpuport_tag4_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short tpid2;
	unsigned short tci2;
	unsigned short tpid3;
	unsigned short tci3;
	unsigned short ethertype;
};

struct cpuport_tag3_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short tpid2;
	unsigned short tci2;
	unsigned short ethertype;
};

struct cpuport_tag2_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short ethertype;
};

struct cpuport_tag1_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short ethertype;
};

struct cpuport_tag0_t {
	unsigned short ethertype;
};

struct cpuport_pkthdr_t {
	unsigned char dmac[6];
	unsigned char smac[6];
	union {
		struct cpuport_tag0_t zero;
		struct cpuport_tag1_t one;
		struct cpuport_tag2_t two;
		struct cpuport_tag3_t three;
		struct cpuport_tag4_t four;
	} tag_u;
};

/* cpuport_util.c */
char *cpuport_util_ip_proto_str(unsigned char proto);
int cpuport_util_rx_port_type_is_wan(struct cpuport_info_t *pktinfo);
int cpuport_util_get_pbit_from_tci(struct cpuport_tci_t *tci);
int cpuport_util_get_tagnum_from_tci(struct cpuport_tci_t *tci);
int cpuport_util_remove_tag( struct cpuport_info_t *cpuport_info);
int cpuport_util_replace_tag(struct cpuport_info_t *cpuport_info, struct cpuport_tci_t *dst_tci);
unsigned short cpuport_util_get_veip_meid(void);
unsigned short cpuport_util_get_iphost_meid(void);
struct me_t *cpuport_util_get_bridge_port_me_by_logical_port_id(unsigned char logical_port_id);
char cpuport_util_get_logical_port_id_by_bridge_port_me(struct me_t *me);
int cpuport_util_is_ctc_internet_bridge_enable(struct cpuport_info_t *pktinfo, char *prompt);
int cpuport_util_is_ctc_internet_bridge_lan2lan(struct cpuport_info_t *pktinfo, char *prompt);

static inline void
dbprintf_tci(int debug_level, char *prompt, struct cpuport_tci_t *tci)
{
	if (debug_level > omci_env_g.debug_level_cpuport)
		return;
	util_dbprintf(omci_env_g.debug_level_cpuport, debug_level, 0, 
		"%s: %dtag, otpid:0x%x, p:%d, de:%d, vid:0x%x(%d), itpid:0x%x, p:%d, de:%d, vid:0x%x(%d)\n",
		prompt, cpuport_util_get_tagnum_from_tci(tci),
		tci->otci.tpid, tci->otci.priority, tci->otci.de, tci->otci.vid, tci->otci.vid,
		tci->itci.tpid, tci->itci.priority, tci->itci.de, tci->itci.vid, tci->itci.vid);
}

#endif
