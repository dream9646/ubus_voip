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
 * Module  : cpuport_dhcp
 * File    : cpuport_dhcp.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <linux/if_ether.h>
//#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"
#include "cpuport.h"
#include "cpuport_extract_filter.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_util_send.h"
#include "hwresource.h"

#include "switch.h"
#include "packet_gen.h"
#include "cpuport_dhcp.h"

static unsigned int dhcp_accept_total, dhcp_drop_total;
extern struct list_head format_list_head;

int dhcp_pkt_recv_qid_g = -1;
int dhcp_cmd_qid_g = -1;
int dhcp_qsetid_g = -1;
int dhcptask_loop_g = 0;
static struct fwk_thread_t dhcptask_thread_g;
unsigned short cvlan_for_65330;

#define IP_OFFSET 0x1FFF
#define IP_MF 0x2000

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct bootphdr {
	u8 op;			/* 1=request, 2=reply */
	u8 htype;		/* HW address type */
	u8 hlen;		/* HW address length */
	u8 hops;		/* Used only by gateways */
	u32 xid;		/* Transaction ID */
	u16 secs;		/* Seconds since we started */
	u16 flags;		/* Just what it says */
	u32 client_ip;		/* Client's IP address if known */
	u32 your_ip;		/* Assigned IP address */
	u32 server_ip;		/* (Next, e.g. NFS) Server's IP address */
	u32 relay_ip;		/* IP address of BOOTP relay */
	u8 hw_addr[16];		/* Client's HW address */
	u8 serv_name[64];	/* Server host name */
	u8 boot_file[128];	/* Name of boot file */
	u8 exten[312];		/* DHCP options / BOOTP vendor extensions */
};

extern struct lineidformat_t *lineidformat;
extern struct lineidparam_t *lineidparam;

int
cpuport_dhcp_count_reset(int fd)
{
	dhcp_accept_total=0 ;
	dhcp_drop_total=0 ;
	util_fdprintf(fd, "ACCEPT:[%d], DROP:[%d]\n", dhcp_accept_total, dhcp_drop_total);
	return 0;
}

int
cpuport_dhcp_count_get(int fd)
{
	util_fdprintf(fd, "ACCEPT:[%d], DROP:[%d]\n", dhcp_accept_total, dhcp_drop_total);
	return 0;
}

static
unsigned short checksum(void *addr, int count)
{
	/* Compute Internet Checksum for "count" bytes
	 *         beginning at location "addr".
	 */
	unsigned int sum = 0;
	unsigned short *source = (unsigned short *) addr;

	while (count > 1)  {
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0) {
		/* Make sure that the left-over byte is added correctly both
		 * with little and big endian hosts */
		unsigned short tmp = 0;
		*(unsigned char *) (&tmp) = * (unsigned char *) source;
		sum += tmp;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (unsigned short)~sum;
}

static
unsigned short udp_checksum(unsigned int ipsaddr, unsigned int ipdaddr, unsigned short len_udp, const unsigned char *udp_ptr)
{
	unsigned char proto_udp = 0x11;
	unsigned short check;
	unsigned char *udp_pseudo, *udp_pseudo_p;	// Pseudo Header
	unsigned short len_udp_ns;					// in network short
	int len_ipaddr = 4, len_ipproto = 1, len_udplen = 2;
	int len_pseudo;
//	int i;

//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "len_udp = %d\n", len_udp);
	len_pseudo = len_ipaddr*2 + 1 + len_ipproto + len_udplen + len_udp;		// there is a 8-bit padding before protocol
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "len_pseudo = %d\n", len_pseudo);
	udp_pseudo = malloc_safe(len_pseudo);
	udp_pseudo_p = udp_pseudo;

	memcpy(udp_pseudo_p, &ipsaddr, len_ipaddr);				// source ip
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "ipsaddr = %x\n", ipsaddr);
	udp_pseudo_p += len_ipaddr;
	memcpy(udp_pseudo_p, &ipdaddr, len_ipaddr);				// dest ip
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "ipdaddr = %x\n", ipdaddr);
	udp_pseudo_p += len_ipaddr;
	memcpy(udp_pseudo_p+1, &proto_udp, len_ipproto);		// ip protocol
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "proto = %x\n", proto_udp);
	udp_pseudo_p += len_ipproto+1;
	len_udp_ns = htons(len_udp);
	memcpy(udp_pseudo_p, &len_udp_ns, len_udplen);			// tot_len (assume tot_len in iphdr is equal to udp->len)
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "len_udp_ns = %x\n", len_udp_ns);
	udp_pseudo_p += len_udplen;
	memcpy(udp_pseudo_p, udp_ptr, len_udp);					// whole udp packet
	memset(udp_pseudo_p+6, 0, 2);			// fill 0 into udp->check
	udp_pseudo_p += len_udp;
#if 0
	for (i = 0; i < len_pseudo; i++) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02x ", *(udp_pseudo + i));
		if (i%16==15)
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
#endif
	check = checksum(udp_pseudo, len_pseudo);
//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "udp checksum = 0x%x\n\n", check);
	free_safe(udp_pseudo);
	return check;
}

static unsigned char *
find_ip_head(struct cpuport_info_t *pktinfo)
{
	unsigned char *eth_proto_ptr;
	unsigned short eth_proto = 0;

	cvlan_for_65330=0;
	eth_proto_ptr=pktinfo->frame_ptr+12;
	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {	// 81xx but not 8137, for icplus 175d with source port in 81xx
		unsigned short tci=ntohs(*(unsigned short *)(eth_proto_ptr+2));
	    	eth_proto_ptr+=4;
	    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			unsigned short tci2=ntohs(*(unsigned short *)(eth_proto_ptr+2));
			eth_proto_ptr+=4;
		    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
			dbprintf_cpuport(LOG_WARNING, "ovlan(s) 0x%x(%d) p%d, ivlan(c) 0x%x(%d) p%d\n",
						tci & 0xfff, tci & 0xfff, (tci&0xe000)>>13,
						tci2& 0xfff, tci2& 0xfff, (tci2&0xe000)>>13);
			cvlan_for_65330=(tci2 & 0xfff);
		} else {
			dbprintf_cpuport(LOG_WARNING, "cvlan 0x%x(%d) p%d\n", tci & 0xfff, tci & 0xfff, (tci&0xe000)>>13);
			cvlan_for_65330=(tci & 0xfff);
		}
	}

	if (eth_proto==ETH_P_IP) {	// IP
		eth_proto_ptr+=2;
		return eth_proto_ptr;
	} else
		dbprintf_cpuport(LOG_ERR, "Can't find IP header\n");	//should not happen
	return 0;
}

int cpuport_dhcp_generate_data_from_fiberhome_me(unsigned char **option_str, struct cpuport_info_t *pktinfo, int *len)
{
	unsigned int i;
	unsigned short classid, meid;
	unsigned char *format, *param, *tmpptr;
	struct meinfo_t *miptr;
	struct me_t *me_ptr;
	struct attr_value_t lineidformat_length, lineidformat_pointer, lineidparam_length, lineidparam_pointer, number_of_octets_format, attr, number_of_octets_param;
	char ctc_mode[]="0 0/0/0:%s.%c %a/%r/%f/%S/0/%p/%m %y", cnc_mode[]="0 0/0/0:%s.%c %B/%a/%r/%f/%S/%L/0/%p %m/%n %y";

	classid=65330;	//option 82,pppoe plus
	meid=0;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf_cpuport(LOG_ERR, "classid=%u, not supported\n", classid);
		return -1;
	}
	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf_cpuport(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u)\n",
		MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid);
		return -1;
	}

	//get format data
	meinfo_me_attr_get(me_ptr, 5, &lineidformat_length);
	meinfo_me_attr_get(me_ptr, 6, &lineidformat_pointer);
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "format:length=%llu, pointer=%llu\n", lineidformat_length.data, lineidformat_pointer.data);

	//get parameter data
	meinfo_me_attr_get(me_ptr, 7, &lineidparam_length);
	meinfo_me_attr_get(me_ptr, 8, &lineidparam_pointer);
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "param:length=%llu, pointer=%llu\n", lineidparam_length.data, lineidparam_pointer.data);

	classid=65349;//OMCI_ME_LARGE_INFO
	//classid=157;	//large string
	meid=lineidformat_pointer.data;	//format
	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf_cpuport(LOG_ERR, "classid=%u, not supported\n", classid);
		return -1;
	}

	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf_cpuport(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u)\n",
		MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid);
		return -1;
	}

	meinfo_me_attr_get(me_ptr, 1, &number_of_octets_format);
	if( number_of_octets_format.data > 375 ) {
		dbprintf_cpuport(LOG_WARNING, "Number of octets should <= 375\n");
		number_of_octets_format.data=375;
	}

	format=malloc_safe(375);
	tmpptr=format;

	attr.data=0;
	attr.ptr = malloc_safe(25);      // max value 25 possible in get
	for(i=2; i <=16; i++) {
		meinfo_me_attr_get(me_ptr, i, &attr);
		memcpy(tmpptr, attr.ptr, 25);
		tmpptr+=25;
	}
	free_safe(attr.ptr);

#if 0
	for(i=0; i < lineidparam_length.data; i++) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02X ", format[i]);
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
	dbprintf_cpuport(LOG_ERR, "format:%02X\n", lineidformat->slotid_S);
	dbprintf_cpuport(LOG_ERR, "format:%02X\n", ntohs(lineidformat->ponid_p));
	dbprintf_cpuport(LOG_ERR, "format:%02X\n", ntohs(lineidformat->onuid_authid_o));
	dbprintf_cpuport(LOG_ERR, "format:%02X\n", lineidformat->symbol_format_type);	//0=USER,1=CTC,2=CNC
#endif

	lineidformat=(struct lineidformat_t *)format;
	switch(lineidformat->symbol_format_type) {
	case 0:	//userdefine
		dbprintf_cpuport(LOG_WARNING, "user define\n");
		tmpptr=format;
		tmpptr+=sizeof(struct lineidformat_t);
		packet_gen_parse_format((char *)tmpptr, strlen((char *)tmpptr));
	break;
	case 1:	//CTC
		dbprintf_cpuport(LOG_WARNING, "CTC\n");
		packet_gen_parse_format(ctc_mode, strlen(ctc_mode));
	break;
	case 2:	//CNC
		dbprintf_cpuport(LOG_WARNING, "CNC\n");
		packet_gen_parse_format(cnc_mode, strlen(cnc_mode));
	break;
	default:
		dbprintf_cpuport(LOG_ERR, "symbol_format_type error=%02X, use CTC mode\n", lineidformat->symbol_format_type);
		packet_gen_parse_format(ctc_mode, strlen(ctc_mode));
	}

	free_safe(format);
	//dump_format_list();		//debug

	classid=65349;//OMCI_ME_LARGE_INFO
	//classid=157;	//large string
	meid=lineidparam_pointer.data;	//param
	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf_cpuport(LOG_ERR, "classid=%u, not supported\n", classid);
		free_safe(param);
		return -1;
	}

	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf_cpuport(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u)\n",
		MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid);
		free_safe(param);
		return -1;
	}

	meinfo_me_attr_get(me_ptr, 1, &number_of_octets_param);
	if( number_of_octets_param.data > 375) {	//15*25
		dbprintf_cpuport(LOG_WARNING, "number_of_octets_param should <= 375\n");
		number_of_octets_param.data=375;
	}

	param=malloc_safe(375);
	tmpptr=param;

	attr.data=0;
	attr.ptr = malloc_safe(25);      // max value 25 possible in get
	for(i=2; i <= 16; i++) {
		meinfo_me_attr_get(me_ptr, i, &attr);
		memcpy(tmpptr, attr.ptr, 25);
		tmpptr+=25;
	}
	free_safe(attr.ptr);
#if 0
	for(i=0; i < lineidparam_length.data; i++) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02X ", param[i]);
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", lineidparam->slotid_S);
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", ntohs(lineidparam->ponid_p));
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", ntohs(lineidparam->onu_authid_o));
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", ntohs(lineidparam->shelf_id_r));
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", ntohs(lineidparam->frame_id_f));
	dbprintf_cpuport(LOG_ERR, "param:%02X\n", ntohl(lineidparam->olt_manage_vlanip_O));
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "number_of_octets_param=%llu\n", number_of_octets_param.data);
	for(i=0; i < 50; i++) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02X ", lineidparam->access_point_id_a[i]);
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
#endif
	//normal case
	lineidparam=(struct lineidparam_t *)param;

	//generate data for opt82

	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "ONU portid=%d(0-3)\n", pktinfo->rx_desc.logical_port_id);
	packet_gen_generate_param(option_str, len, cvlan_for_65330);

	packet_gen_release_format_list();
	free_safe(param);
	return 0;
}

int cpuport_dhcp_generate_data_from_ericsson_me(unsigned char **option_str, struct cpuport_info_t *pktinfo, int *len)
{
	struct meinfo_t *miptr;
	struct me_t *me_ptr;
	
	if ((miptr=meinfo_util_miptr(264))==NULL) {
		return -1;
	}
	
	list_for_each_entry(me_ptr, &miptr->me_instance_list, instance_node) {
		struct me_t *pptp_me = meinfo_me_get_by_meid(11, me_ptr->meid);
		struct me_t *ipptp_me = hwresource_public2private(pptp_me);
		unsigned char port_id = meinfo_util_me_attr_data(ipptp_me, 4);
		unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;
		if(logical_port_id == port_id) {
			unsigned short meid = (unsigned short) meinfo_util_me_attr_data(me_ptr, 5);
			if ((me_ptr = meinfo_me_get_by_meid(157, meid)) == NULL) {
				dbprintf_cpuport(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u)\n",
				MEINFO_ERROR_ME_NOT_FOUND, 157, meid, meid);
				return -1;
			} else {
				int maxlen=375;	//large string 15*25;
				unsigned char *tmpptr = NULL;
				*option_str=malloc_safe(maxlen);
				tmpptr=*option_str;
				if(tmpptr != NULL) {
					unsigned char i, parts = (unsigned char) meinfo_util_me_attr_data(me_ptr, 1);
					*len = 0;
					for(i=0; i<parts; i++) {
						char *str = (char *)meinfo_util_me_attr_ptr(me_ptr, 2+i);
						int length = strlen(str);
						if (i==0 && strncmp(str, "%01", 3)==0) { // ignore heading "%01"
							str = (char *)(meinfo_util_me_attr_ptr(me_ptr, 2+i)+3);
							length -= 3;
						} else if (i==parts-1 && strncmp(str+strlen(str)-3, "%02", 3)==0) { // ignore ending "%02"
							str = (char *)meinfo_util_me_attr_ptr(me_ptr, 2+i);
							length -= 3;
						}
						strncat(tmpptr, str, length);
						*len += length;
					}
				}
			}
			break;
		}
	}
	return 0;
}

static int is_fiberhome_65330_option82_enable()
{
	unsigned short classid, meid, data_classid;
	struct meinfo_t *miptr;
	struct me_t *me_ptr;
	struct attr_value_t option82_enable;

	classid=65330;	//fiberhome olt, option 82, pppoe plus
	meid=0;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)!=0)
		return 0;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf_cpuport(LOG_WARNING, "classid=%u, not supported\n", classid);	//too noise
		return 0;
	}
	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf_cpuport(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u)\n",
		MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid);
		return 0;
	}
	//double check if 65349 exist
	data_classid=65349;	//omci_me_large_info
	//data_classid=157;	//omci_me_large_info
	if ((miptr=meinfo_util_miptr(data_classid))==NULL) {
		dbprintf_cpuport(LOG_ERR, "Can't found classid=%u\n", data_classid);
		return 0;
	}
	//format process
	meinfo_me_attr_get(me_ptr, 1, &option82_enable);
	dbprintf_cpuport(LOG_WARNING, "option82_enable=%llu\n", option82_enable.data);
	return option82_enable.data;
}

static int is_ericsson_264_option82_enable(unsigned char logical_port_id)
{
	struct meinfo_t *miptr;
	struct me_t *me_ptr;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)!=0)
		return 0;

	if ((miptr=meinfo_util_miptr(264))==NULL) {
		return 0;
	}
	list_for_each_entry(me_ptr, &miptr->me_instance_list, instance_node) {
		struct me_t *pptp_me = meinfo_me_get_by_meid(11, me_ptr->meid);
		struct me_t *ipptp_me = hwresource_public2private(pptp_me);
		unsigned char port_id = meinfo_util_me_attr_data(ipptp_me, 4);
		if(logical_port_id == port_id) {
			unsigned short meid = (unsigned short) meinfo_util_me_attr_data(me_ptr, 5);
			return (meinfo_me_get_by_meid(157, meid)) ? 1 : 0;
		}
	}
	
	return 0;
}

static
int suboption_get(int subopt_code, unsigned char **subopt_ptr, unsigned int *subopt_len, struct cpuport_info_t *pktinfo)
{
	unsigned char *option_str;
	int option_len;
	unsigned char *subopt;
	//int i;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0) {
		if( cpuport_dhcp_generate_data_from_fiberhome_me(&option_str, pktinfo, &option_len)!=0) {	//todo:check return value
			return -1;
		}
#if 0	//for debug only
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "option_len=%d\n", option_len);
		for(i=0; i < option_len; i++) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02X ", option_str[i]);
		}
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
#endif
	
		if(option_len ==0)	//configure error use default value ????
			return -1;

		subopt = malloc_safe(option_len+4);	// include sub opt1/2 code and len
	
		if (subopt_code == 1) {			//??
			subopt[0] = 1;			// sub opt code 1, value:Circuit ID
			subopt[1] = option_len;
			memcpy(subopt+2, option_str, option_len);
			subopt[option_len+2] = 2;		// sub opt code 2
			subopt[option_len+3] = 0;		// sub opt 2, len:0
			free_safe(option_str);
		} else {
			free_safe(subopt);
			return -1;
		}
	
		*subopt_ptr = subopt;
		*subopt_len = (option_len + 4);
	} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
		if( cpuport_dhcp_generate_data_from_ericsson_me(&option_str, pktinfo, &option_len)!=0) {	//todo:check return value
			return -1;
		}
#if 0	//for debug only
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "option_len=%d\n", option_len);
		for(i=0; i < option_len; i++) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02X ", option_str[i]);
		}
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
#endif
	
		if(option_len ==0)	//configure error use default value ????
			return -1;

		subopt = malloc_safe(option_len+2);	// include sub opt1 code and len
	
		if (subopt_code == 1) {			//??
			subopt[0] = 1;			// sub opt code 1, value:Circuit ID
			subopt[1] = option_len;
			memcpy(subopt+2, option_str, option_len);
			free_safe(option_str);
		} else {
			free_safe(subopt);
			return -1;
		}
	
		*subopt_ptr = subopt;
		*subopt_len = (option_len + 2);
	}
	return 0;
}

static
int option82_gen(unsigned char opt82[], unsigned int *len_opt82, struct cpuport_info_t *pktinfo)
{
	unsigned char *sub_ptr;			// for temporary use
	unsigned int opt82_len, sub_len, opt_i;

	opt82[0] = 82;
	opt82[1] = 0;		// len
	opt_i = 2;
	opt82_len = 0;

	if( suboption_get(1, &sub_ptr, &sub_len, pktinfo) != 0 ) {	// get code 1
		return -1;
	}

	memcpy(opt82+opt_i, sub_ptr, sub_len);
	free_safe(sub_ptr);
	opt_i += sub_len;
	opt82_len += sub_len;

	opt82[1] = opt82_len;		// update len in option 82
	*len_opt82 = opt82_len + 2;

	return 0;
}

static int
cpuport_dhcp_option82_add(struct cpuport_info_t *pktinfo)
{
	unsigned char *ip_head;
	struct iphdr *iphdr_ptr;
	struct udphdr *udphdr_ptr;
	struct bootphdr *bootphdr_ptr;
	unsigned char *opt, *opt82, opt82data[312];
	unsigned int optcode, optlen, opt_num = 0;
	unsigned int len_opt82;				// total len
	//u8 *optvalue = NULL, *ptr = NULL;
	int i;
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;

	// find ptr of each header
	ip_head = find_ip_head(pktinfo);
	iphdr_ptr = (struct iphdr*)(ip_head);
	udphdr_ptr = (struct udphdr *)(ip_head + iphdr_ptr->ihl * 4);
	bootphdr_ptr = (struct bootphdr *)(ip_head + iphdr_ptr->ihl * 4 + sizeof(struct udphdr));
	opt = (unsigned char*)&(bootphdr_ptr->exten);

	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "opt82add cpupkt->frm_len = %d, iphdr->tot_len = %d\n", pktinfo->frame_len, ntohs(iphdr_ptr->tot_len));


	// parsing options
	i = 4;
	while ((optcode = opt[i]) != 0xff) {
		optlen = opt[i+1];
#if 0
		optvalue = (u8 *)malloc_safe(sizeof(u8)*optlen*3 + 1);
		ptr = optvalue;
		for (j = 0; j < optlen; j++, ptr+=3) sprintf(ptr, "%02x ", opt[i+2+j]);
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "option %2u, len=%2u, value=%s\n", optcode, optlen, optvalue);
		free_safe(optvalue);
#endif
		i += 2 + optlen;	// 2 = code and len fields of sub-option
		opt_num++;
	}
	opt82 = opt + i;		// record the start position for option 82

	//util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "opt82add opt_num=%d, idx_for_opt82=%d\n", opt_num, i);

	if (is_fiberhome_65330_option82_enable() || is_ericsson_264_option82_enable(logical_port_id)) {
		// get opt82 string, reserve enough space and attach it
		if( option82_gen(opt82data, &len_opt82, pktinfo) != 0 ) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "Can't generate opt82 data\n");
			return -1;
		}
	} else if (omci_env_g.cpuport_dhcp_cust_tag[0]){ // option82 length!=0
		memset(opt82data, 0, sizeof(opt82data));
		len_opt82 = omci_env_g.cpuport_dhcp_cust_tag[0] + 2; // length of option82, option(1)+length(1)+payload(in [0])
		opt82data[0] = 0x52; // option82
		for(i=0; i<len_opt82; i++)
			opt82data[i+1] = omci_env_g.cpuport_dhcp_cust_tag[i];
	}
	
	for (i = pktinfo->frame_len - 1; i >= opt82 - pktinfo->frame_ptr; i--)
		pktinfo->frame_ptr[i + len_opt82] = pktinfo->frame_ptr[i];
	memcpy(opt82, opt82data, len_opt82);

	pktinfo->frame_len += len_opt82;
#if 0
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "opt82str: ");
	for (i = 2; i < len_opt82; i++)
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%02x(%c) ", opt82data[i], opt82data[i]);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n");
#endif

	// modify len and calculate new checksum
	iphdr_ptr->tot_len = htons(ntohs(iphdr_ptr->tot_len) + len_opt82);
	iphdr_ptr->check = 0;
	iphdr_ptr->check = checksum(iphdr_ptr, iphdr_ptr->ihl*4);
	udphdr_ptr->len = htons(ntohs(udphdr_ptr->len) + len_opt82);
	if (udphdr_ptr->check != 0) {
		udphdr_ptr->check = 0;
		udphdr_ptr->check = udp_checksum(iphdr_ptr->saddr, iphdr_ptr->daddr, ntohs(udphdr_ptr->len), (unsigned char *)udphdr_ptr);
	}

	//cpuport_sniffer_dbprint_pkt(LOG_ERR, pktinfo->frame_ptr, pktinfo->frame_len);
	return 0;
}

static int
cpuport_dhcp_option82_del(struct cpuport_info_t *pktinfo)
{
	unsigned char *ip_head;
	struct iphdr *iphdr_ptr;
	struct udphdr *udphdr_ptr;
	struct bootphdr *bootphdr_ptr;
	unsigned char *opt, *opt82;
	unsigned int optcode, optlen;
	unsigned int len_opt82;
//	u8 *optvalue = NULL, *ptr = NULL;
	int i;//, j;

	// find ptr of each header
	ip_head = find_ip_head(pktinfo);
	iphdr_ptr = (struct iphdr*)(ip_head);
	udphdr_ptr = (struct udphdr *)(ip_head + iphdr_ptr->ihl * 4);
	bootphdr_ptr = (struct bootphdr *)(ip_head + iphdr_ptr->ihl * 4 + sizeof(struct udphdr));
	opt = (unsigned char*)&(bootphdr_ptr->exten);

	// find option82
	i = 4;
	while ((optcode = opt[i]) != 0xff) {
		optlen = opt[i+1];
#if 0
		optvalue = (u8 *)malloc_safe(sizeof(u8)*optlen*3 + 1);
		ptr = optvalue;
		for (j = 0; j < optlen; j++, ptr+=3) sprintf(ptr, "%02x ", opt[i+2+j]);
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "option %2u(%d), len=%2u, value=%s\n", optcode, optcode, optlen, optvalue);
		free_safe(optvalue);
#endif
		if (optcode == 82) break;
		i += 2 + optlen;		// 2 = code and len fields of sub-option
	}
	if (optcode == 0xff) {		// there is no option82 in dhcp packet
		util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "[cpuport_dhcp] receive a dhcp packet with no option 82\n");
		return 0;
	}
	len_opt82 = optlen + 2;
	opt82 = opt + i;

	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "opt82del found opt82, len=%u\n", opt82[1]);
	// move the following data backward
	for (i = opt82 - pktinfo->frame_ptr + len_opt82; i < pktinfo->frame_len; i++)
		pktinfo->frame_ptr[i - len_opt82] = pktinfo->frame_ptr[i];

	pktinfo->frame_len -= len_opt82;

	// modify len and calculate new checksum
	iphdr_ptr->tot_len = htons(ntohs(iphdr_ptr->tot_len) - len_opt82);
	iphdr_ptr->check = 0;
	iphdr_ptr->check = checksum(iphdr_ptr, iphdr_ptr->ihl*4);
	udphdr_ptr->len = htons(ntohs(udphdr_ptr->len) - len_opt82);
	if (udphdr_ptr->check != 0) {
		udphdr_ptr->check = 0;
		udphdr_ptr->check = udp_checksum(iphdr_ptr->saddr, iphdr_ptr->daddr, ntohs(udphdr_ptr->len), (unsigned char *)udphdr_ptr);
	}

	//cpuport_sniffer_dbprint_pkt(LOG_ERR, pktinfo->frame_ptr, pktinfo->frame_len);

	return 0;
}

//Drop DHCP packet from LAN,and type OFFER, ACK, NAK
static int
cpuport_dhcp_pkt_filter( struct cpuport_info_t *pktinfo )
{
	//unsigned short dhcp_type = 0;
	unsigned short eth_proto=0;
	unsigned char *eth_proto_ptr, *nh, is_dhcp_packet=0;
	unsigned char *opt;
	unsigned short sp,dp;

	//dbprintf(LOG_ERR, "Packet was extract found\n\n");

	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;

	eth_proto_ptr=pktinfo->frame_ptr+12;
	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {	// 81xx but not 8137, for icplus 175d with source port in 81xx
		unsigned short tci=ntohs(*(unsigned short *)(eth_proto_ptr+2));
	    	eth_proto_ptr+=4;
	    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			unsigned short tci2=ntohs(*(unsigned short *)(eth_proto_ptr+2));
			eth_proto_ptr+=4;
		    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
			//dbprintf(LOG_WARNING, "ovlan %d p%d\n", tci & 0xfff, (tci&0xe000)>>13);
			//dbprintf(LOG_WARNING, "ivlan %d p%d\n", tci2& 0xfff, (tci2&0xe000)>>13);
			cvlan_for_65330=(tci2 & 0xfff);
		} else {
			//dbprintf(LOG_WARNING, "cvlan %d p%d\n", tci & 0xfff, (tci&0xe000)>>13);
			cvlan_for_65330=(tci & 0xfff);
		}
	}

	if (ntohs(eth_proto) != 0x0800 )
		return CPUPORT_UNMATCH;

	nh=eth_proto_ptr+2;

	if (eth_proto==ETH_P_IP) {
		// IP
		struct iphdr *ip=(struct iphdr*)(nh);
		//unsigned char *s=(unsigned char*)&(ip->saddr);
		//unsigned char *d=(unsigned char*)&(ip->daddr);
		char frag_more;
		unsigned short frag_offset;

		frag_more=(ip->frag_off & htons(IP_MF))?1:0;
		frag_offset=(ntohs(ip->frag_off) & IP_OFFSET) << 3;

		if (ip->protocol==IPPROTO_UDP) {
			struct udphdr *udp=(struct udphdr *)(nh+ip->ihl*4);
			sp=ntohs(udp->source);
			dp=ntohs(udp->dest);

			if ((sp==67 && dp==68) || (sp==68 && dp==67) ) {
				//reply (sp==67 && dp==68), //request: sp==68 && dp==67
				if (!frag_more && frag_offset==0) {
					struct bootphdr *bootp=(struct bootphdr *)(nh+ip->ihl*4+sizeof(struct udphdr));
					opt=(unsigned char*)&(bootp->exten);
					if (opt[0]==99 && opt[1]==130 && opt[2]==83 && opt[3]==99) {
						// dhcp cookie
						is_dhcp_packet=1;
						dbprintf_cpuport(LOG_NOTICE, "Packet is dhcp\n\n");
					}
				}
			}
		}
	}

	if( !is_dhcp_packet ) {
		dbprintf_cpuport(LOG_DEBUG, "Packet not dhcp\n\n");
		return CPUPORT_UNMATCH;
	}

	//follow are dhcp packets
	if((omci_env_g.cpuport_dhcp_server_onlan == 0) &&
		!cpuport_util_rx_port_type_is_wan(pktinfo) &&	//packet come from lan side
		(sp==67 && dp==68) &&	//reply packet
		((opt[4]==0x35 && opt[5]==1) && (// assume dhcp type is 1st option
		opt[6]==2 || 	//OFFER
		opt[6]==5 ||	//ACK
		opt[6]==6))) {	//NAK
			dbprintf_cpuport(LOG_WARNING, "DROP: %d\n", opt[6]);
			dhcp_drop_total++;
			return CPUPORT_DROP;
	} else {
		dbprintf_cpuport(LOG_WARNING, "found dhcp, maybe need to add opt82\n\n");
#if 0
		if (opt[4] == 0x35 && opt[5] == 1) {	// stvn
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "dhcpfilter recv ");
			if (opt[6] == 1)
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", "DISCOVER");
			else if (opt[6] == 2)
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", "OFFER");
			else if (opt[6] == 3)
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", "REQUEST");
			else if (opt[6] == 5)
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", "ACK");
			else if (opt[6] == 8)
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", "INFORM");
			else
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "opt[6]=%d\n", opt[6]);
		}
		else {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "dhcpfilter opt[4]=%u opt[5]=%u opt[6]=%u\n", opt[4], opt[5], opt[6]);
		}
#endif
		return CPUPORT_MATCH;
	}
}

static int
dhcp_send_wan( struct cpuport_info_t *pktinfo)
{
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;
	if (is_fiberhome_65330_option82_enable() || is_ericsson_264_option82_enable(logical_port_id) || omci_env_g.cpuport_dhcp_cust_tag[0])
		cpuport_dhcp_option82_add(pktinfo);
	cpuport_util_send_multi_wan(pktinfo, "dhcp-to-wan", cpuport_util_is_ctc_internet_bridge_enable, NULL);
	return 0;
}

/* packet from wan side send to all uni port and cpu */
static int
dhcp_send_lan(struct cpuport_info_t *pktinfo)
{
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;
	if (is_fiberhome_65330_option82_enable() || is_ericsson_264_option82_enable(logical_port_id) || omci_env_g.cpuport_dhcp_cust_tag[0])
		cpuport_dhcp_option82_del(pktinfo);
	cpuport_util_send_multi_lan(pktinfo, "dhcp-to-lan", cpuport_util_is_ctc_internet_bridge_enable, NULL);
	return 0;
}

/* packet from lan side send to other uni ports and cpu */
static int
dhcp_send_other_lan( struct cpuport_info_t *pktinfo)
{
	cpuport_util_send_multi_lan_from_lan(pktinfo, "dhcp-to-other-lan", cpuport_util_is_ctc_internet_bridge_lan2lan);
	return 0;
}

static void *
cpuport_dhcp_function(void *ptr)
{
	struct list_head *list_node = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	//unsigned char pbit_val;
	int num;

	util_set_thread_name("omci-dhcp");
	omci_env_g.taskname[ENV_TASK_NO_DHCP] = "DHCP";
	omci_env_g.taskid[ENV_TASK_NO_DHCP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_DHCP] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_DHCP] = 0;
	omci_env_g.taskts[ENV_TASK_NO_DHCP] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_DHCP] = 0; //idle

	while ( dhcptask_loop_g )
	{
		omci_env_g.taskloopcount[ENV_TASK_NO_DHCP]++;
		omci_env_g.taskts[ENV_TASK_NO_DHCP] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_DHCP] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_DHCP)) {
			msleep(100);
			continue;
		}
		if ((num = fwk_msgq_select(dhcp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0) {
			int i = 0;
			omci_env_g.taskstate[ENV_TASK_NO_DHCP] = 1; //running
			for( i = 0 ; i < num ; i ++ ) {
				if ( fwk_msgq_set_isset(dhcp_qsetid_g, dhcp_pkt_recv_qid_g) ) {
					if (fwk_msgq_recv(dhcp_pkt_recv_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "dhcp_pkt_recv_qid_g recv err\n");
					} else {
						pktinfo = list_entry(list_node, struct cpuport_info_t, node);
						pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_DHCP];
						util_get_uptime(&pktinfo->process_time);
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_add(pktinfo);	

						//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "dhcp_rx", pktinfo->frame_ptr, pktinfo->frame_len);
						if (!pktinfo->rx_desc.bridge_port_me ) {
							if (!cpuport_util_rx_port_type_is_wan(pktinfo) && omci_env_g.localbridging_default) {
								dbprintf_cpuport(LOG_DEBUG, "packet dhcp come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
								dhcp_send_other_lan(pktinfo);
							}
						} else {
							if( cpuport_util_rx_port_type_is_wan(pktinfo)) {
								dbprintf_cpuport(LOG_DEBUG, "packet dhcp come from wan side and send to lan\n");
								dhcp_send_lan(pktinfo);
							} else {
								dbprintf_cpuport(LOG_DEBUG, "packet dhcp come from lan side port=%d and send to wan\n", pktinfo->rx_desc.logical_port_id);
								if (omci_env_g.localbridging_default) {
									dbprintf_cpuport(LOG_DEBUG, "packet dhcp come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
									dhcp_send_other_lan(pktinfo);
								}
								dhcp_send_wan(pktinfo);
							}
							dhcp_accept_total++ ;
							//dbprintf(LOG_ERR, "ACCEPT count:[%d]\n", dhcp_accept_total);
						}

						util_get_uptime(&pktinfo->forward_time);
						if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
							dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_sync(pktinfo);	

						free_safe(pktinfo->buf_ptr);
						free_safe(pktinfo);
					}
				}

				if ( fwk_msgq_set_isset(dhcp_qsetid_g, dhcp_cmd_qid_g) ) {

					if (fwk_msgq_recv(dhcp_cmd_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "dhcp_cmd_qid_g recv err\n");

					} else {
						struct dhcp_cmd_msg_t *dhcp_cmd_msg;
						dhcp_cmd_msg = list_entry(list_node, struct dhcp_cmd_msg_t, q_entry);

						if (dhcp_cmd_msg->cmd == DHCP_CMD_LIMIT_CHANGE) {
						}

						if (dhcp_cmd_msg->cmd == DHCP_CMD_TERMINATE) {
							dhcptask_loop_g = 0;
						}
						free_safe(dhcp_cmd_msg);
					}
				}
			}
		}
	}
	omci_env_g.taskstate[ENV_TASK_NO_DHCP] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_DHCP] = 0;

	dbprintf_cpuport(LOG_ERR, "task terminate\n");
	return 0;
}

int
cpuport_dhcp_init(void)
{
	int ret = 0;
	if ((dhcp_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_DHCP_RECV" ) ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "dhcp recv queue alloc error(%d)\n",dhcp_pkt_recv_qid_g );
		return (-1);
	}
	if ((dhcp_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_DHCP_CMD") ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "dhcp cmd queue alloc error(%d)\n",dhcp_cmd_qid_g );
		return (-1);
	}
	if ((dhcp_qsetid_g = fwk_msgq_set_alloc())< 0)
	{
		fwk_msgq_release( dhcp_pkt_recv_qid_g );
		fwk_msgq_release( dhcp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set alloc error(%d)\n", dhcp_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( dhcp_qsetid_g ))< 0)
	{
		fwk_msgq_release( dhcp_pkt_recv_qid_g );
		fwk_msgq_release( dhcp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set zero error(%d)\n", dhcp_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( dhcp_qsetid_g ,dhcp_pkt_recv_qid_g ))<0 )
	{
		fwk_msgq_set_release( dhcp_qsetid_g );
		fwk_msgq_release( dhcp_pkt_recv_qid_g );
		fwk_msgq_release( dhcp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( dhcp_qsetid_g ,dhcp_cmd_qid_g))<0 )
	{
		fwk_msgq_set_release( dhcp_qsetid_g );
		fwk_msgq_release( dhcp_pkt_recv_qid_g );
		fwk_msgq_release( dhcp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
cpuport_dhcp_shutdown(void)
{
	fwk_msgq_set_release( dhcp_qsetid_g );
	fwk_msgq_release( dhcp_pkt_recv_qid_g );
	fwk_msgq_release( dhcp_cmd_qid_g );
	return 0;
}


int
cpuport_dhcp_start(void)
{
	int ret;
#ifndef X86	//for build x86 version
	//only implement for core swith now
	switch_hw_g.dhcp_extract_enable_set(ENABLED);
#endif
	INIT_LIST_HEAD(&format_list_head);
	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_DHCP, "dhcp", cpuport_dhcp_pkt_filter, dhcp_pkt_recv_qid_g) ) {
		dbprintf_cpuport(LOG_ERR, "cpuport_extract_filter_register error(%d)\n");
		return -1;
	}

	dhcptask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	dhcptask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	dhcptask_thread_g.stack_size = 128*1024;
	dhcptask_thread_g.join = 1;
	sprintf(dhcptask_thread_g.name, "omci-DHCP");
	dhcptask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&dhcptask_thread_g, cpuport_dhcp_function, NULL)) < 0) {
		dbprintf_cpuport(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}

	return 0;
}

int
cpuport_dhcp_stop(void)
{
	int ret;

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_DHCP))
		return -1;

	dhcptask_loop_g = 0;	//start loop
	struct dhcp_cmd_msg_t *dhcp_cmd_msg;
	dhcp_cmd_msg = malloc_safe(sizeof (struct dhcp_cmd_msg_t));
	dhcp_cmd_msg->cmd = DHCP_CMD_TERMINATE;
	INIT_LIST_NODE(&dhcp_cmd_msg->q_entry);
	if (fwk_msgq_send(dhcp_cmd_qid_g, &dhcp_cmd_msg->q_entry) < 0)
	{
		free_safe(dhcp_cmd_msg);
	}
	if ((ret = fwk_thread_join(&dhcptask_thread_g)) < 0) {
		dbprintf_cpuport(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}
