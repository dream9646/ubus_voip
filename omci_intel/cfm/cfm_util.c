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
 * Module  : cfm
 * File    : cfm_util.c
 *
 ******************************************************************/

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <net/if.h>

#include "list.h"
#include "conv.h"
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "hwresource.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "gpon_sw.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "proprietary_alu.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_bport.h"

// mutex for cfm_util_get_time_in_us()
struct fwk_mutex_t cfm_util_get_time_mutex_g;

unsigned long long
cfm_util_get_time_in_us(void)
{
	static unsigned long long prev;
	unsigned long long now=0;

	fwk_mutex_lock(&cfm_util_get_time_mutex_g);

	{
		struct timespec tv;
		int i;
		for (i=0; i<10; i++) {	// for EINTR when system is busy in processing interrupt
			if (clock_gettime(CLOCK_MONOTONIC, &tv) == 0) {
				now = tv.tv_sec*1000000ULL + tv.tv_nsec/1000;
				break;
			}
		}
	}
	if (now < prev)
		dbprintf_cfm(LOG_CRIT, "prev=%llu, now=%llu, time wrap around %lluus?\n", prev, now, prev-now);

	if (now == 0) {
		// get current time failed
		dbprintf_cfm(LOG_CRIT, "get current time failed\n");
		now = prev;
	} else {
		prev = now;
	}

	fwk_mutex_unlock(&cfm_util_get_time_mutex_g);
	return now;
}

int
cfm_util_us_to_64bit_ieee1588(unsigned long long us, struct ieee1588_time *now)
{
	now->sec = htonl((us/1000000) & 0xFFFFFFFF);
	now->nanosec = htonl(((us%1000000)*1000) & 0xFFFFFFFF);
	return 0;
}

int
cfm_util_ieee1588_64bit_to_us(struct ieee1588_time now, unsigned long long *us)
{
	*us = ntohl(now.sec)*1000000ULL + ntohl(now.nanosec)/1000;
	return 0;
}

unsigned int
cfm_util_ccm_interval_to_ms(int ccm_interval)
{
	switch(ccm_interval) {
		case CCM_INTERVAL_DISABLED:	return 10*60*1000;
		case CCM_INTERVAL_3_33_MS:	return 3;
		case CCM_INTERVAL_10_MS:	return 10;
		case CCM_INTERVAL_100_MS:	return 100;
		case CCM_INTERVAL_1_SECOND:	return 1000;
		case CCM_INTERVAL_10_SECOND:	return 10*1000;
		case CCM_INTERVAL_1_MINUTE:	return 60*1000;
		case CCM_INTERVAL_10_MINUTE:	return 10*60*1000;
	}
	return 1000;	// default recommended by 1ag
}

int
cfm_util_get_send_priority_by_defect_status(unsigned char defect_status)
{
	static char defect_priority[7] = {
		PRIORITY_DEFECT_RDI_CCM,
		PRIORITY_DEFECT_MAC_STATUS,
		PRIORITY_DEFECT_REMOTE_CCM,
		PRIORITY_DEFECT_ERROR_CCM,
		PRIORITY_DEFECT_XCON_CCM,
		PRIORITY_DEFECT_UNEXP_PERIOD,
		PRIORITY_DEFECT_AIS
	};
	int i, prio = 0;
	for (i=0; i<7; i++) {
		if (defect_status & (1<<i) && prio < defect_priority[i])
			prio = defect_priority[i];
	}
	return prio;
}

int
cfm_util_find_tlv(unsigned char *tlv_start, unsigned int tlv_total_len, int find_tlv_type, unsigned short *find_tlv_len, unsigned char **find_tlv_val)
{
	unsigned char *tlv;

	for (tlv = tlv_start; tlv < tlv_start + tlv_total_len; tlv += (3 + (tlv[1]<<8)+tlv[2]) ) {
		unsigned char tlv_type = tlv[0];
		unsigned short tlv_len = (tlv[1]<<8) + tlv[2];
		unsigned char *tlv_val = tlv+3;

		if (tlv_len == 0) {
			dbprintf_cfm(LOG_WARNING, "TLV %d len %d? at offset %d, tlv_total_len=%d\n", tlv_type, tlv_len, tlv-tlv_start, tlv_total_len);
			return -1;	// error
		}
		if (tlv_type == CFM_TLV_TYPE_END) {
			break;
		} else if (tlv_type == find_tlv_type) {
			dbprintf_cfm(LOG_INFO, "TLV %d len %d at offset %d, tlv_total_len=%d\n", tlv_type, tlv_len, tlv-tlv_start, tlv_total_len);
			*find_tlv_len = tlv_len;
			*find_tlv_val = tlv_val;
			return 1;	// found
		}
	}

	return 0;	// not found
}

// get/set attributes in cfm_config /////////////////////////////////////////////////////////////////////////////////

int
cfm_util_get_mac_addr(cfm_config_t *cfm_config, unsigned char *mac, char *devname)
{
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) {
		unsigned char cfm_mac[IFHWADDRLEN] = {0x12, 0x61, 0x00, 0x00, 0x00, 0x00};
#ifndef X86
		char vendor_id[8];
		unsigned int serial_number=0;

		if ( gpon_hw_g.onu_serial_number_get)
			gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);

		cfm_mac[2] = (serial_number >> 24) & 0xFF;
		cfm_mac[3] = (serial_number >> 16) & 0xFF;
		cfm_mac[4] = (serial_number >> 8) & 0xFF;
		cfm_mac[5] = (serial_number >> 0) & 0xFF;
#endif
		cfm_mac[1] += ((cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_UNI) ? cfm_config->port_type_index : cfm_bport_find_peer_index(cfm_config));
		memcpy(mac, cfm_mac, IFHWADDRLEN);
	} else {
		int fd;
		struct ifreq itf;

		if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			return -1;

		strncpy(itf.ifr_name, devname, IFNAMSIZ-1);
		if(ioctl(fd, SIOCGIFHWADDR, &itf) < 0) {
			close(fd);
			return -1;
		} else
			memcpy(mac, itf.ifr_hwaddr.sa_data, IFHWADDRLEN);
		close(fd);
	}

	return 0;
}

int
cfm_util_get_rdi_ais_lck_alarm_gen(cfm_config_t *cfm_config, unsigned char *rdi_gen, unsigned char *ais_gen, unsigned char *lck_gen, unsigned char *alarm_gen)
{
	unsigned long long now = cfm_util_get_time_in_us();
	unsigned int ais_interval_us = ((cfm_config->eth_ais_control&1)?60:1) *1000 *1000;
	unsigned char is_recv_lck = (now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5)?1:0;
	unsigned char defect = cfm_config->defect_status;
	unsigned char alarm = cfm_config->alarm_status;

	*rdi_gen = *ais_gen = *lck_gen = *alarm_gen = 0;

	if (cfm_config->defect_status & (1<<DEFECT_AIS)) {
		defect &= (~omci_env_g.cfm_defect_mask_supressed_by_ais);
		alarm &= (~omci_env_g.cfm_defect_mask_supressed_by_ais);
	}
	if (is_recv_lck) {
		defect &= (~omci_env_g.cfm_defect_mask_supressed_by_lck);
		alarm &= (~omci_env_g.cfm_defect_mask_supressed_by_lck);
	}
	if (defect&omci_env_g.cfm_defect_mask_in_rdi_gen)
		*rdi_gen = 1;
	if ((defect&omci_env_g.cfm_defect_mask_in_ais_gen) || is_recv_lck)
		*ais_gen = 1;
	if (cfm_config->admin_state)
		*lck_gen = 1;
	if (alarm) {
		static unsigned char alarm_threshold_bitmap[] = {
			ALARM_THRESHOLD_0, ALARM_THRESHOLD_1, ALARM_THRESHOLD_2, ALARM_THRESHOLD_3,
			ALARM_THRESHOLD_4, ALARM_THRESHOLD_5, ALARM_THRESHOLD_6
		};
		unsigned char alarm_allowed_mask = alarm_threshold_bitmap[0];
		if (cfm_config->fault_alarm_threshold >= 1 &&
		    cfm_config->fault_alarm_threshold <= 6) {
		    	alarm_allowed_mask = alarm_threshold_bitmap[cfm_config->fault_alarm_threshold];
		}
		if (alarm & alarm_allowed_mask)
			*alarm_gen = 1;
	}
	return 0;
}

int
cfm_util_get_mhf_creation(cfm_config_t *cfm_config)
{
	if (cfm_config->ma_mhf_creation == MA_MHF_CREATION_DEFER)
		return cfm_config->md_mhf_creation;
	else
		return cfm_config->ma_mhf_creation;
}

int
cfm_util_get_sender_id_permission(cfm_config_t *cfm_config)
{
	if (cfm_config->ma_sender_id_permission == MA_SENDER_ID_PERMISSION_DEFER)
		return cfm_config->md_sender_id_permission;
	else
		return cfm_config->ma_sender_id_permission;
}

static int
cfm_util_string_is_number(char *string)
{
	char *s = string;
	int i;

	if (s[0] == '+' || s[0] == '-')
		s++;
	if (strncmp(s, "0x", 2) == 0 ||
	    strncmp(s, "0X", 2) == 0) {
	    	s+=2;
		for (i=0; i<strlen(s); i++)
			if (!isxdigit(s[i]))
				return 0;
	} else {
		for (i=0; i<strlen(s); i++)
			if (!isdigit(s[i]))
				return 0;
	}
	return 1;
}

// string2 is only used for format == MD_NAME_FORMAT_MACADDR_SHORTINT
int
cfm_util_set_md_name(cfm_config_t *cfm_config, int format, char *string, char *string2)
{
	if (format == MD_NAME_FORMAT_NONE ||	// 1
	    format == MD_NAME_FORMAT_DNS ||	// 2
	    format == MD_NAME_FORMAT_STRING) {	// 4
	    	if (format == MD_NAME_FORMAT_NONE) {
	    		if (string == NULL || strlen(string) == 0 || strcmp(string, "NULL")==0) {
				cfm_config->md_name_1[0] = cfm_config->md_name_2[0] = 0;
	    			return 0;
			} else {
				return -1;
			}
	    	} else if (format == MD_NAME_FORMAT_DNS) {
	    		char fqdn[64], *fqdn_array[8];
			int i, n;
			strncpy(string, fqdn, 63); fqdn[63]=0;
			if ((n = conv_str2array(fqdn, ".", fqdn_array, 8)) <= 1)
				return -1;
			// check each part of fqdn_array
			for (i=0; i<n; i++) {
				char *s = fqdn_array[i];
				int j, len = strlen(s);
				if (len <= 1)	// len should > 1
					return -1;
				for (j=0; j<len; j++) {
		    			if (s[j] >=0 && s[j]<=31)	// invisible char
	    					return -1;
					if (i>=1 && !isalnum(s[j]) && s[j]!='_')
						return -1;	// shoulde contain alphabet, digit or underscore only
				}
			}
	    	} else if (format == MD_NAME_FORMAT_STRING) {
	    		int i;
	    		for (i=0; i<strlen(string); i++) {
	    			if (string[i] >=0 && string[i]<=31)	// invisible char
	    				return -1;
			}
		}
		if (strlen(string) > 25) {
			memcpy(cfm_config->md_name_1, string, 25);
			memcpy(cfm_config->md_name_2, string+25, 48-25);	// name wont be longer than 48 bytes
			cfm_config->md_name_2[24] = 0;
		} else if (strlen(string) > 0) {
			memcpy(cfm_config->md_name_1, string, 25);
			cfm_config->md_name_2[0] = 0;
		}
		return 0;
	} else if (format == MD_NAME_FORMAT_MACADDR_SHORTINT) {// 3
		unsigned char macaddr[6];
		int val;
		if (conv_str_mac_to_mem((char *)macaddr, string)<0)
			return -1;
		if (string2 == NULL || !cfm_util_string_is_number(string2))
			return -1;
		val = util_atoi(string2);
		if (val>65535 || val <0)
			return -1;
		memcpy(cfm_config->md_name_1, macaddr, 6);
		cfm_config->md_name_1[0] = (val >> 8) & 0xff;
		cfm_config->md_name_1[1] = val & 0xff;
		cfm_config->md_name_2[0] = 0;
		return 0;
	} else if (format == MD_NAME_FORMAT_ICC) {	// 32
		int i;
    		for (i=0; i<strlen(string); i++) {
    			if (string[i] >=0 && string[i]<=31)	// invisible char
    				return -1;
			}
		if (strlen(string) > 0) {
			strncpy(cfm_config->md_name_1, string, 13);
			cfm_config->md_name_2[0] = 0;
		}
		return 0;
	}
	return -1;
}

// string2 is only used for format == MA_NAME_FORMAT_VPNID
int
cfm_util_set_ma_name(cfm_config_t *cfm_config, int format, char *string, char *string2)
{
	if (format == MA_NAME_FORMAT_VID) { 		// 1
	    	int val;
		if (!cfm_util_string_is_number(string))
			return -1;
		val = util_atoi(string);
		if (val>4095 || val <0)
			return -1;
		cfm_config->ma_name_1[0] = (val >> 8) & 0xff;
		cfm_config->ma_name_1[1] = val & 0xff;
		cfm_config->ma_name_2[0] = 0;
		return 0;
	} else if (format == MA_NAME_FORMAT_SHORTINT) {	// 3
	    	int val;
		if (!cfm_util_string_is_number(string))
			return -1;
		val = util_atoi(string);
		if (val>65535 || val <0)
			return -1;
		cfm_config->ma_name_1[0] = (val >> 8) & 0xff;
		cfm_config->ma_name_1[1] = val & 0xff;
		cfm_config->ma_name_2[0] = 0;
		return 0;
	} else if (format == MA_NAME_FORMAT_STRING) {	// 2
		int i;
    		for (i=0; i<strlen(string); i++) {
    			if (string[i] >=0 && string[i]<=31)	// invisible char
    				return -1;
			}
		if (strlen(string) > 24) {
			memcpy(cfm_config->ma_name_1, string, 25);
			strncpy(cfm_config->ma_name_2, string+25, 48-25);	// name wont be longer than 48 bytes
			cfm_config->ma_name_2[24] = 0;
		} else if (strlen(string) > 0) {
			strncpy(cfm_config->ma_name_1, string, 25);
			cfm_config->ma_name_2[0] = 0;
		}
		return 0;
	} else if (format == MA_NAME_FORMAT_VPNID) {	// 4
	    	unsigned int vpn_oui;	// 3 byte
	    	unsigned int vpn_index;	// 4 byte
		if (string2 == NULL)
			return -1;
		if (!cfm_util_string_is_number(string))
			return -1;
		if (!cfm_util_string_is_number(string2))
			return -1;
		vpn_oui = util_atoi(string);
		vpn_index = util_atoi(string2);
		cfm_config->ma_name_1[0] = (vpn_oui >> 16) & 0xff;
		cfm_config->ma_name_1[1] = (vpn_oui >> 8) & 0xff;
		cfm_config->ma_name_1[2] = vpn_oui & 0xff;
		cfm_config->ma_name_1[3] = (vpn_index >> 24) & 0xff;
		cfm_config->ma_name_1[4] = (vpn_index >> 16) & 0xff;
		cfm_config->ma_name_1[5] = (vpn_index >> 8) & 0xff;
		cfm_config->ma_name_1[6] = vpn_index & 0xff;
		cfm_config->ma_name_2[0] = 0;
		return 0;
	} else if (format == MA_NAME_FORMAT_ICC) {	// 32
		int i;
    		for (i=0; i<strlen(string); i++) {
    			if (string[i] >=0 && string[i]<=31)	// invisible char
    				return -1;
			}
		if (strlen(string) > 0) {
			strncpy(cfm_config->ma_name_1, string, 13);
			cfm_config->ma_name_2[0] = 0;
		}
		return 0;
	}
	return -1;
}
