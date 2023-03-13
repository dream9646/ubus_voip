/******************************************************************
 *
 * Copyright (C) 2019 5V Technologies Ltd.
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
 * Module  : omci_bm
 * File    : omci_bm.h
 *
 ******************************************************************/

// omci_bm_init() will search signature file in /tmp, /etc, /etc/voip, /nvram
// find the interface whose signature matches the signature file

// omci_bm_check() will check the omci_bm_init result
// and compare if the signature interface is pon0 or the one specified by signature info

// note: 
// bind_mac/host/try_bin_mac_host usage
// a. generate signature file
//    echo [mac_addr] | ./try_bin_mac_host sign [private_key_file] [6byte_info_str] > signature_file
// b. verify signature file
//    echo [mac_addr] | ./try_bin_mac_host verify [signature_file]
// c. the mac_addr in above must be UPPER case
//    when putting ifname as info, if ifname is shorter than 6 byte, use - instead space
//    eg: pon1-- for ifname pon1

#ifndef __OMCI_BM_H__
#define __OMCI_BM_H__

#include <sys/sysinfo.h>
#include "gpon_sw.h"
#include "iphost.h"
#include "conv.h"

//#define OMCI_BM_DEBUG 1

#ifdef OMCI_BM_DEBUG
#define omci_bm_printf(fmt, args...)	dbprintf(LOG_ERR, fmt, ##args)
#else
#define omci_bm_printf(fmt, args...)
#endif

/* omci_bm.c */
int omci_bm_init(void);
int omci_bm_getmac(unsigned char *mac);
int omci_bm_getinfo(unsigned char *info);

/* omcimain.c */
int omci_stop_shutdown(void);

static inline void
omci_bm_system_disable(void)
{
#ifndef X86
	// disable all uni, so lan is disconnected
	if (switch_hw_g.port_enable_set) {
		unsigned int portmask = switch_get_uni_logical_portmask();
		int portid;
		for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
			if ((1<<portid) & portmask)
				switch_hw_g.port_enable_set(portid, 0);
		}
	}
	// diable onu, so wan is disconnected
	if (gpon_hw_g.onu_deactivate)
		gpon_hw_g.onu_deactivate();
#endif
	// delete me 11, 134, 329, 268 so omci wont work
	meinfo_me_delete_by_classid(11);	// pptp-uni
	meinfo_me_delete_by_classid(134);	// iphost
	meinfo_me_delete_by_classid(329);	// veip
	meinfo_me_delete_by_classid(268);	// gem port ctp

	// omci shutdown & exit, so system would reboot when hw watchdog timeout(~20s)
	omci_stop_shutdown();
	exit(0);
}

// the omci_bm_check routine will check the result of omci_bm_init(), 
// and compare if signature mac is pon0 or ifname specified in signature info
// ps: similar check logic is implemented in omci_bm_test as a stand alone program
static inline int 
omci_bm_check(int period)	// unit: seconds
{
	struct sysinfo s_info;
	long now;
	static long scheduled_checktime = 0;
	unsigned char sig_mac[6];
	unsigned char sig_info[8];
	int ret = 0;
#ifdef OMCI_BM_DEBUG
	period=60;
#endif
	
	sysinfo(&s_info);
	now = s_info.uptime;
	
	if (scheduled_checktime == 0)
		scheduled_checktime = now + period/2 + random()%period;
	omci_bm_printf("now=%ld, scheduled_checktime=%ld\n", now, scheduled_checktime);

	// check mac
	if (now > scheduled_checktime) {
		if (omci_bm_getmac(sig_mac) <0) {
			ret = -1;
			omci_bm_printf("sig_mac err\n");
			omci_bm_system_disable();
		}
		ret = 1;
		omci_bm_printf("sig_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			sig_mac[0],sig_mac[1],sig_mac[2],sig_mac[3],sig_mac[4],sig_mac[5]);
	}
	// check info
	if (now > scheduled_checktime) {
		if (omci_bm_getinfo(sig_info) <0) {
			ret = -2;
			omci_bm_printf("sig_info err\n");
			omci_bm_system_disable();
		}
		ret = 2;
		omci_bm_printf("sig_info=%s\n", sig_info);
	}
	// check ifmac 
	if (now > scheduled_checktime) {
		unsigned char ifmac[6];
		int match = 0;

		if (match==0) {
			char *output = NULL;
			if (util_readcmd(FW_PRINTENV_BIN " ethaddr", &output) > 0) {
				unsigned char macaddr[18] = {0};
				util_trim(output);
				strncpy(macaddr, output+strlen("ethaddr="), 18);
				if (output)
					free_safe(output);
				if (conv_str_mac_to_mem(ifmac, macaddr) ==0 && memcmp(ifmac, sig_mac, 6) == 0) {
					match = 1;
					omci_bm_printf("sig_mac %s found\n", macaddr);
				}
#ifdef OMCI_BM_DEBUG
				else {
					omci_bm_printf("macaddr=%s\n", macaddr);
					omci_bm_printf("ifmac=%02x:%02x:%02x:%02x:%02x:%02x\n", ifmac[0],ifmac[1],ifmac[2],ifmac[3],ifmac[4],ifmac[5]);
					omci_bm_printf("sig_mac=%02x:%02x:%02x:%02x:%02x:%02x\n", sig_mac[0],sig_mac[1],sig_mac[2],sig_mac[3],sig_mac[4],sig_mac[5]);
				}
#endif
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'p'; ifname[1] = 'o'; ifname[2] = 'n'; ifname[3] = '0';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'g'; ifname[1] = 'p'; ifname[2] = 'o'; ifname[3] = 'n'; ifname[4] = '1';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'e'; ifname[1] = 't'; ifname[2] = 'h'; ifname[3] = '0';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			int i;
			for (i=5; i>0; i--) {	// replace - with 0 so sig_info could be used as ifname
				if (sig_info[i] == '-') sig_info[i] = 0;
			}
			if (iphost_get_hwaddr(sig_info, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", sig_info);
			}
		}
		if (match==0) {
			ret = -3;
			omci_bm_printf("sig_mac interface not found!\n");
			omci_bm_system_disable();
		} else {
			ret = 3;
		}
	}

	// reschedule checktime if it is out of range (<now or >now+2*period)
	if (scheduled_checktime < now || scheduled_checktime > now + 2*period)
		scheduled_checktime = now + period/2 + random()%period;
	
	return ret;
}

static inline int 
omci_bm_check2(int period)	// unit: seconds
{
	struct sysinfo s_info;
	long now;
	static long scheduled_checktime = 0;
	unsigned char sig_mac[6];
	unsigned char sig_info[8];
	int ret = 0;
#ifdef OMCI_BM_DEBUG
	period=90;
#endif
	
	sysinfo(&s_info);
	now = s_info.uptime;
	
	if (scheduled_checktime == 0)
		scheduled_checktime = now + period/2 + random()%period;
	omci_bm_printf("now=%ld, scheduled_checktime=%ld\n", now, scheduled_checktime);

	// check mac content
	if (now > scheduled_checktime) {
		omci_bm_getmac(sig_mac);
		if (sig_mac[0] == 0x88 && sig_mac[1] == 0x62 && sig_mac[2] == 0x27 &&
		    sig_mac[3] == 0x88 && sig_mac[4] == 0x81 && sig_mac[5] == 0x18) {
			ret = -1;
			omci_bm_printf("sig_mac err\n");
			omci_bm_system_disable();
		}
		ret = 1;
		omci_bm_printf("sig_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			sig_mac[0],sig_mac[1],sig_mac[2],sig_mac[3],sig_mac[4],sig_mac[5]);
	}
	// check info content
	if (now > scheduled_checktime) {
		omci_bm_getinfo(sig_info);
		if (sig_info[0] == '5' &&  sig_info[1] == 'v' && sig_info[2] == 't' &&   
		    sig_info[3] == 'e' &&  sig_info[4] == 'c' && sig_info[5] == 'h') {
			ret = -2;
			omci_bm_printf("sig_info err\n");
			omci_bm_system_disable();
		}
		ret = 2;
		omci_bm_printf("sig_info=%s\n", sig_info);
	}
	// check ifmac 
	if (now > scheduled_checktime) {
		unsigned char ifmac[6];
		int match = 0;

		if (match==0) {
			char *output = NULL;
			if (util_readcmd(FW_PRINTENV_BIN " ethaddr", &output) > 0) {
				unsigned char macaddr[18] = {0};
				util_trim(output);
				strncpy(macaddr, output+strlen("ethaddr="), 18);
				if (output)
					free_safe(output);
				if (conv_str_mac_to_mem(ifmac, macaddr) ==0 && memcmp(ifmac, sig_mac, 6) == 0) {
					match = 1;
					omci_bm_printf("sig_mac %s found\n", macaddr);
				}
#ifdef OMCI_BM_DEBUG
				else {
					omci_bm_printf("macaddr=%s\n", macaddr);
					omci_bm_printf("ifmac=%02x:%02x:%02x:%02x:%02x:%02x\n", ifmac[0],ifmac[1],ifmac[2],ifmac[3],ifmac[4],ifmac[5]);
					omci_bm_printf("sig_mac=%02x:%02x:%02x:%02x:%02x:%02x\n", sig_mac[0],sig_mac[1],sig_mac[2],sig_mac[3],sig_mac[4],sig_mac[5]);
				}
#endif
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'p'; ifname[1] = 'o'; ifname[2] = 'n'; ifname[3] = '0';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'g'; ifname[1] = 'p'; ifname[2] = 'o'; ifname[3] = 'n'; ifname[4] = '1';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			char ifname[8] = {0};
			ifname[0] = 'e'; ifname[1] = 't'; ifname[2] = 'h'; ifname[3] = '0';
			if (iphost_get_hwaddr(ifname, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", ifname);
			}
		}
		if (match==0) {
			int i;
			for (i=5; i>0; i--) {	// replace - with 0 so sig_info could be used as ifname
				if (sig_info[i] == '-') sig_info[i] = 0;
			}
			if (iphost_get_hwaddr(sig_info, ifmac) == 0 && memcmp(ifmac, sig_mac, 6) == 0) {
				match = 1;
				omci_bm_printf("sig_mac interface %s found\n", sig_info);
			}
		}
		if (match==0) {
			ret = -3;
			omci_bm_printf("sig_mac interface not found!\n");
			omci_bm_system_disable();
		} else {
			ret = 3;
		}
	}

	// reschedule checktime if it is out of range (<now or >now+2*period)
	if (scheduled_checktime < now || scheduled_checktime > now + 2*period)
		scheduled_checktime = now + period/2 + random()%period;
	
	return ret;
}

#endif
