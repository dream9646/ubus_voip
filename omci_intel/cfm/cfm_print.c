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
 * File    : cfm_print.c
 *
 ******************************************************************/
 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <netinet/ip.h>
#include "list.h"
#include "util.h"
#include "conv.h"
#include "cli.h"
#include "cpuport.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_print.h"
#include "cfm_send.h"


static char *porttype_str[] = { "cpu", "uni", "wan", "iph", "dsb", "?", "?", "?" };
static char *md_format_str[] = { "?", "none", "dns", "mac+short_int", "string", "icc-based" };	// icc-based is for 32
static char *ma_format_str[] = { "?", "vid", "string", "short_int", "vpn_id" };
static char *ccm_interval_str[] ={ "disabled", "3.3 ms", "10 ms", "100 ms", "1 sec", "10 sec", "1 min", "10 min" };
static char *ccm_rmep_state_str[] ={ "NONE", "EXPECT", "DISCOVER", "HOLD", "ACTIVE", "INACTIVE", "LOST", "REMOVE" };
static char *defect_str[] = { "RDICCM", "MACStatus", "RemoteCCM", "ErrorCCM", "XconCCM", "UnexpPeriod", "AIS"};
static char alam_threshold_bitmap[] = { 0x3e, 0x7f, 0x3e, 0x3c, 0x38, 0x10, 0 };

static char *line14 = "--------------";
static char *line12 = "--------------";
static char *line10 = "----------";
static char *line9 = "---------";
static char *line8 = "--------";
static char *line7 = "-------";
static char *line6 = "------";
static char *line5 = "-----";
static char *line4 = "----";
static char *line3 = "---";

// function return str for specfic stru //////////////////////////////////////////////////////////////////////////////////////

char *
cfm_print_get_opcode_str(unsigned char opcode)
{
	switch(opcode) {
		case CFM_PDU_CCM:	return "CCM";
		case CFM_PDU_LBR:	return "LBR";
		case CFM_PDU_LBM:	return "LBM";
		case CFM_PDU_LTR:	return "LTR";
		case CFM_PDU_LTM:	return "LTM";
		case CFM_PDU_AIS:	return "AIS";
		case CFM_PDU_LCK:	return "LCK";
		case CFM_PDU_TST:	return "TST";
		case CFM_PDU_LAPS:	return "LAPS";
		case CFM_PDU_RAPS:	return "RAPS";
		case CFM_PDU_MCC:	return "MCC";
		case CFM_PDU_LMR:	return "LMR";
		case CFM_PDU_LMM:	return "LMM";
		case CFM_PDU_1DM:	return "1DM";
		case CFM_PDU_DMR:	return "DMR";
		case CFM_PDU_DMM:	return "DMM";
		case CFM_PDU_EXR:	return "EXR";
		case CFM_PDU_EXM:	return "EXM";
		case CFM_PDU_VSR:	return "VSR";
		case CFM_PDU_VSM:	return "VSM";
		case CFM_PDU_CSF:	return "CSF";
		case CFM_PDU_1SL:	return "1SL";
		case CFM_PDU_SLR:	return "SLR";
		case CFM_PDU_SLM:	return "SLM";
	}
	return "?";
}

char *
cfm_print_get_cfm_config_str(cfm_config_t *cfm_config)
{
	static char cfm_config_str[128];
	char *mpstr="MIP";

	if (cfm_config == NULL) 
		return "";	
	if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)	// mep
		mpstr = (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP)?"upMEP":"downMEP";
	snprintf(cfm_config_str, 128, "%s@%s%d(mdl=%d, mpid=%d%s)",
		mpstr, porttype_str[cfm_config->port_type &7], cfm_config->port_type_index,
		cfm_config->md_level, cfm_config->mep_id,
		cfm_config->admin_state?", admin=1":"");

	return cfm_config_str;
}
	
char *
cfm_print_get_cfm_pkt_str(cfm_hdr_t *cfm_hdr)
{ 
	static char cfm_pkt_str[128];
	char *s = cfm_pkt_str;

	if (cfm_hdr == NULL)
		return "";

	s += snprintf(s, 128, "%s(%d), mdl=%d,flags=0x%02x",
		cfm_print_get_opcode_str(cfm_hdr->opcode), cfm_hdr->opcode, 
		cfm_hdr->md_level, cfm_hdr->flags);

	// pdu specific fields
	if (cfm_hdr->opcode == CFM_PDU_CCM) {
		cfm_pdu_ccm_t *ccm_ptr = (cfm_pdu_ccm_t *)((void *)cfm_hdr + sizeof(cfm_hdr_t));
		snprintf(s, 128-strlen(cfm_pkt_str), ",rdi=%d,seq=0x%x", (cfm_hdr->flags>>7)&1, ntohl(ccm_ptr->seq));
	} else if (cfm_hdr->opcode == CFM_PDU_LBM || cfm_hdr->opcode == CFM_PDU_LBR) {
		cfm_pdu_lbr_t *lbr_ptr = (cfm_pdu_lbr_t *)((void *)cfm_hdr + sizeof(cfm_hdr_t));
		snprintf(s, 128-strlen(cfm_pkt_str), ",xid=0x%x", ntohl(lbr_ptr->xid));
	} else if (cfm_hdr->opcode == CFM_PDU_LTM || cfm_hdr->opcode == CFM_PDU_LTR) {
		cfm_pdu_ltr_t *ltr_ptr = (cfm_pdu_ltr_t *)((void *)cfm_hdr + sizeof(cfm_hdr_t));
		snprintf(s, 128-strlen(cfm_pkt_str), ",hwonly=%d,xid=0x%x",  (cfm_hdr->flags>>7)&1, ntohl(ltr_ptr->xid));
	}
	return cfm_pkt_str;
}

char *
cfm_print_get_tlv_str(unsigned char *tlv_start, unsigned int tlv_total_len)
{
	static char tlv_str[1024];	
	unsigned char buff[64];
	char *s=tlv_str;
	unsigned char *tlv;
	int len;

	tlv_str[0] = 0;
	for (tlv = tlv_start; tlv < tlv_start + tlv_total_len; tlv += (3 + (tlv[1]<<8)+tlv[2]) ) {
		unsigned char tlv_type = tlv[0];
		unsigned short tlv_len = (tlv[1]<<8) + tlv[2];
		unsigned char *tlv_val = tlv+3;
		
		if (tlv_len == 0) {
			dbprintf_cfm(LOG_WARNING, "TLV %d len %d at offset %d, tlv_total_len=%d\n", tlv_type, tlv_len, tlv-tlv_start, tlv_total_len);
			break;
		}

		dbprintf_cfm(LOG_INFO, "TLV %d len %d at offset %d, tlv_total_len=%d\n", tlv_type, tlv_len, tlv-tlv_start, tlv_total_len);

		if (tlv_type == CFM_TLV_TYPE_END) {
			break;

		} else if (tlv_type == CFM_TLV_TYPE_SENDER_ID) {	// 1
			unsigned char *p, len;
			s+=sprintf(s, " sender_id:");
			p = tlv_val; len = p[0];
			if (len >0) {
				strncpy(buff, p+2, len);	// chassis_id
				buff[(len<64)?len:63]=0;
				s+=sprintf(s, "chassis_id=%s,", buff);
			}
			p+=(len+2); len = p[0];
			if (p < tlv_val+tlv_len && len >0) {
				strncpy(buff, p+1, len);	// mgmt_addr_domain
				buff[(len<64)?len:63]=0;
				s+=sprintf(s, "mgmt_addr_domain=%s,", buff);
			}
			p+=(len+1); len = p[0];
			if (p < tlv_val+tlv_len && len >0) {
				strncpy(buff, p+1, len);	// mgmt_addr
				buff[(len<64)?len:63]=0;
				s+=sprintf(s, "mgmt_addr=%s,", buff);
			}
			
		} else if (tlv_type == CFM_TLV_TYPE_PORT_STATUS) {	// 2
			s+=sprintf(s, " port_status:");
			if (tlv_val[0]==1)
				s+=sprintf(s, "1(blocked),");
			else if (tlv_val[0]==2)
				s+=sprintf(s, "2(up),");
			else
				s+=sprintf(s, "%d(?),", tlv_val[0]);
			
		} else if (tlv_type == CFM_TLV_TYPE_DATA) {		// 3
			s+=sprintf(s, " data:len=%d,", tlv_len);
			
		} else if (tlv_type == CFM_TLV_TYPE_INTERFACE_STATUS) {	// 4
			s+=sprintf(s, " interface_status:");
			if (tlv_val[0]==1)
				s+=sprintf(s, "1(up),");
			else if (tlv_val[0]==2)
				s+=sprintf(s, "2(down),");
			else if (tlv_val[0]==3)
				s+=sprintf(s, "3(testing),");
			else if (tlv_val[0]==4)
				s+=sprintf(s, "4(unknown),");
			else if (tlv_val[0]==5)
				s+=sprintf(s, "5(dormant),");
			else if (tlv_val[0]==6)
				s+=sprintf(s, "6(notPresent),");
			else if (tlv_val[0]==7)
				s+=sprintf(s, "7(lowerlayerdown),");
			else
				s+=sprintf(s, "%d(?),", tlv_val[0]);
				
		} else if (tlv_type == CFM_TLV_TYPE_REPLY_INGRESS) {	// 5
			unsigned char *p, len;
			s+=sprintf(s, " reply_ingress:");
			if (tlv_val[0]==1)
				s+=sprintf(s, "act=1(IngOk),");
			else if (tlv_val[0]==2)
				s+=sprintf(s, "act=2(IngDown),");
			else if (tlv_val[0]==3)
				s+=sprintf(s, "act=3(IngBlocked),");
			else if (tlv_val[0]==4)
				s+=sprintf(s, "act=4(IngVid),");
			else
				s+=sprintf(s, "act=%d(?),", tlv_val[0]);			
			
			s+=sprintf(s, "mac=%02x:%02x:%02x:%02x:%02x:%02x,", 
				tlv_val[1], tlv_val[2], tlv_val[3], tlv_val[4], tlv_val[5], tlv_val[6]);
			p = tlv_val+8; len = p[0];
			if (p < tlv_val+tlv_len && len >0) {
				strncpy(buff, p+2, len);	// port_id
				buff[(len<64)?len:63]=0;
				s+=sprintf(s, "portid=%s,", buff);
			}
			
		} else if (tlv_type == CFM_TLV_TYPE_REPLY_EGRESS) {	// 6
			unsigned char *p, len;
			s+=sprintf(s, " reply_engress:");
			if (tlv_val[0]==1)
				s+=sprintf(s, "act=1(EgrOk),");
			else if (tlv_val[0]==2)
				s+=sprintf(s, "act=2(EgrDown),");
			else if (tlv_val[0]==3)
				s+=sprintf(s, "act=3(EgrBlocked),");
			else if (tlv_val[0]==4)
				s+=sprintf(s, "act=4(EgrVid),");
			else
				s+=sprintf(s, "act=%d(?),", tlv_val[0]);			
			
			s+=sprintf(s, "mac=%02x:%02x:%02x:%02x:%02x:%02x,", 
				tlv_val[1], tlv_val[2], tlv_val[3], tlv_val[4], tlv_val[5], tlv_val[6]);
			p = tlv_val+8; len = p[0];
			if (p < tlv_val+tlv_len && len >0) {
				strncpy(buff, p+2, len);	// port_id
				buff[(len<64)?len:63]=0;
				s+=sprintf(s, "portid=%s,", buff);
			}
		} else if (tlv_type == CFM_TLV_TYPE_LTM_EGRESS_ID) {	// 7
			if (tlv_len>=8) {
				s+=sprintf(s, " ltm_egress_id:%02x%02x-%02x:%02x:%02x:%02x:%02x:%02x,",
					tlv_val[0], tlv_val[1], tlv_val[2], tlv_val[3], tlv_val[4], tlv_val[5], tlv_val[6], tlv_val[7]);
			} else {
				s+=sprintf(s, " ltm_egress_id:?,");
			}
		} else if (tlv_type == CFM_TLV_TYPE_LTR_EGRESS_ID) {	// 8
			if (tlv_len>=16) {
				s+=sprintf(s, " ltr_egress_id:last=%02x%02x-%02x:%02x:%02x:%02x:%02x:%02x,next=%02x%02x-%02x:%02x:%02x:%02x:%02x:%02x,",
					tlv_val[0], tlv_val[1], tlv_val[2], tlv_val[3], tlv_val[4], tlv_val[5], tlv_val[6], tlv_val[7],
					tlv_val[8], tlv_val[9], tlv_val[10], tlv_val[11], tlv_val[12], tlv_val[13], tlv_val[14], tlv_val[15]);
			} else {
				s+=sprintf(s, " ltm_egress_id:?,");
			}
		} else if (tlv_type == CFM_TLV_TYPE_TEST) {		// 32
			s+=sprintf(s, " test:len=%d,pattern=", tlv_len);
			if (tlv_val[0] == 0)
				s+=sprintf(s, "0(null),");
			else if (tlv_val[0] == 1)
				s+=sprintf(s, "1(null+crc32),");
			else if (tlv_val[0] == 2)
				s+=sprintf(s, "2(prbs31),");
			else if (tlv_val[0] == 3)
				s+=sprintf(s, "3(prbs31+crc32),");
			else 
				s+=sprintf(s, "%d(?),", tlv_val[0]);
			
		} else if (tlv_type == CFM_TLV_TYPE_TESTID) {		// 36
			s+=sprintf(s, " testid:0x%x,", tlv_val[0]<<24 | tlv_val[1]<<16 | tlv_val[2]<<8 | tlv_val[3]);
		} else {
			s+=sprintf(s, " tlv%d:len=%d,offset=%d,", tlv_type, tlv_len, (int)(tlv-tlv_start));
		}

		if (s - tlv_str > 1000) {
			dbprintf_cfm(LOG_ERR, "tlv result str too long (%d)\n", s - tlv_str);
			break;
		}
	}
	
	len = strlen(tlv_str);
	if (len > 0 && tlv_str[len-1] == ',')
		tlv_str[len-1] = 0;		// remove tail ,		
	
	return tlv_str;
}	

char *
cfm_print_get_vlan_str(cfm_pktinfo_t *cfm_pktinfo)
{
	static char vlan_str[32] = {0};
	if (cfm_pktinfo->vlan_num) {
		unsigned short vlan_id = 0;
		if (cfm_pktinfo->vlan_num == 1 && cfm_pktinfo->in_tci) {
			vlan_id = ntohs(*(unsigned short *)(cfm_pktinfo->in_tci)) & 0xfff;
		}
		else if (cfm_pktinfo->vlan_num == 2 && cfm_pktinfo->out_tci)
			vlan_id = ntohs(*(unsigned short *)(cfm_pktinfo->out_tci)) & 0xfff;
		snprintf(vlan_str, 15, "vlan %d", vlan_id);
	} else {
		snprintf(vlan_str, 15, "untag");
	}
	return vlan_str;
}

char *
cfm_print_get_maid_str(char *maid)
{
	static char maid_str[256];
	char md_name[49] = {0};
	char ma_name[49] = {0};
	char *mdptr, *maptr;

	if (maid[0] != MD_NAME_FORMAT_NONE) {	// MD format
		mdptr = maid;
		maptr = maid + maid[1] + 2;
		if (mdptr[1] + 2 + maptr[1]+2 > 48)
			return "? ?";	// md + ma should <=48 byte				
	} else if (maid[0] == MD_NAME_FORMAT_NONE) {
		mdptr = NULL;
		maptr = maid + 1;
		if (1 + maptr[1]+2 > 48)
			return "?";	// md + ma should <=48 byte
	} else {
		mdptr = NULL;
		maptr = NULL;
	}
	if (mdptr) {
		// 0:md_format, 1:md_len, 2..x-1:md_name, x:ma_format, x+1:ma_len, x+2:ma_name
		switch(mdptr[0]) { 	// MD format
			case MD_NAME_FORMAT_MACADDR_SHORTINT:	
				snprintf(md_name, 48, "%s 0x%02x%02x", util_str_macaddr(mdptr+2), mdptr[8], mdptr[9]);
				break;
			case MD_NAME_FORMAT_DNS:				
			case MD_NAME_FORMAT_STRING:
			case MD_NAME_FORMAT_ICC:
			default:
				snprintf(md_name, mdptr[1]+1, "%s", mdptr+2);
		}
	}
	if (maptr) {
		switch (maptr[0]) {	// MA format
			case MA_NAME_FORMAT_VID:
				snprintf(ma_name, 48, "%d", (maptr[2]<<8)+maptr[3]);
			case MA_NAME_FORMAT_SHORTINT:
				snprintf(ma_name, 48, "0x%02x%02x", maptr[2], maptr[3]);
				break;
			case MA_NAME_FORMAT_VPNID:
				snprintf(ma_name, 48, "0x%02x%02x%02x 0x%02x%02x%02x%02x", 
					maptr[2],maptr[3],maptr[4], maptr[5],maptr[6],maptr[7],maptr[8]);
			case MA_NAME_FORMAT_STRING:
			default:	
				snprintf(ma_name, maptr[1]+1, "%s", maptr+2);
		}
	}
	if (mdptr && maptr)
		snprintf(maid_str, 128, "%s %s", md_name, ma_name);
	else if (maptr)
		snprintf(maid_str, 128, "%s", ma_name);
	else
		snprintf(maid_str, 128, "%s", "");
	return maid_str;
}			

char *
cfm_print_get_us_time_str(unsigned long long us)
{
	static char timestr[16];
	unsigned long long ms = us/1000;
	
	if (ms > 1000*60*60*24) {
		snprintf(timestr, 32, "%uday %uh", (unsigned int)(ms/(1000*60*60*24)), (unsigned int)(ms/(1000*60*60)%24));
	} else if (ms > 1000*60*60) {
		snprintf(timestr, 32, "%uh %umin",  (unsigned int)(ms/(1000*60*60)), (unsigned int)(ms/(1000*60)%60));
	} else if (ms > 1000*60) {
		snprintf(timestr, 32, "%umin %us", (unsigned int)(ms/(1000*60)), (unsigned int)(ms/1000%60));
	} else if (ms >1000) {
		snprintf(timestr, 32, "%usec", (unsigned int)(ms/1000));
	} else {
		snprintf(timestr, 32, "%ums", (unsigned int)(ms));
	}
	return timestr;
}

char *
cfm_print_get_rmep_state_str(int state)
{
	if (state >=0 && state <=7)
		return ccm_rmep_state_str[state];
	return "?";
}

// function print output to fd for specific stru //////////////////////////////////////////////////////////////////////////////

int
cfm_print_send_frame(int debug_level, cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, int send_type)
{
	char *send_type_str;
	int vlan_num;

	cfm_hdr_t *cfm_hdr;
	unsigned short vlan_id;
	char vlan_str[16];

	if (omci_env_g.debug_level_cfm < debug_level)
		return 0;
		
	switch (send_type) {
		case CFM_SEND_NORMAL:	send_type_str = "send"; break;
		case CFM_SEND_OPPOSITE:	send_type_str = "send_opposite"; break;
		case CFM_SEND_FORWARD:	send_type_str = "forward"; break;
		case CFM_SEND_REPLY:	send_type_str = "reply"; break;
		default:		send_type_str = "?";
	}

	if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == CFM_ETHERTYPE) 
		vlan_num = 0;
	else if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET)) == CFM_ETHERTYPE) 
		vlan_num = 1;
	else if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2)) == CFM_ETHERTYPE) 
		vlan_num = 2;
	else {
		dbprintf_cfm(LOG_ERR, "%s %s non-CFM to logical port %d\n",
			cfm_print_get_cfm_config_str(cfm_config),
			send_type_str, pktinfo->tx_desc.logical_port_id);
		return -1;
	}

	cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * vlan_num);
	vlan_id = ntohs(*(unsigned short *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN)) & 0xfff;
	if (vlan_num) {
		snprintf(vlan_str, 15, "vlan %d", vlan_id);
	} else {
		snprintf(vlan_str, 15, "untag");
	}

	util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, 
		"%s %s %d bytes %s to logical port %d %s ----->\n",
		cfm_print_get_cfm_config_str(cfm_config),
		send_type_str, pktinfo->frame_len,
		cfm_print_get_cfm_pkt_str(cfm_hdr), 
		pktinfo->tx_desc.logical_port_id, vlan_str);

	{	
		char *tlv_ptr=(unsigned char *)((void *)cfm_hdr + sizeof(cfm_hdr_t) + cfm_hdr->first_tlv_offset);
		int tlv_total_len = (pktinfo->frame_ptr+pktinfo->frame_len) - (unsigned char *)tlv_ptr;
		char *tlv_str = cfm_print_get_tlv_str(tlv_ptr, tlv_total_len);
		if (strlen(tlv_str))
			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "SendTLV:%s\n", tlv_str);
	}	

	return 0;
}

// if cfm_config is null, assume drop action
int
cfm_print_recv_frame(int debug_level, cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	cfm_hdr_t *cfm_hdr = cfm_pktinfo->cfm_hdr;

	if (omci_env_g.debug_level_cfm < debug_level)
		return 0;
	
	util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, 
			"%s%s%s %d bytes %s from %s logical port %d %s <=====\n",
			cfm_config?cfm_print_get_cfm_config_str(cfm_config):"",
			cfm_config?" ":"",
			cfm_config?"recv":"Drop", 
			cfm_pktinfo->frame_len, 
			cfm_print_get_cfm_pkt_str(cfm_hdr), 
			util_str_macaddr(cfm_pktinfo->src_mac_ptr), 
			cfm_pktinfo->rx_desc.logical_port_id, cfm_print_get_vlan_str(cfm_pktinfo));
	{	
		char *tlv_ptr=(unsigned char *)((void *)cfm_hdr + sizeof(cfm_hdr_t) + cfm_hdr->first_tlv_offset);
		int tlv_total_len = (cfm_pktinfo->frame_ptr+cfm_pktinfo->frame_len) - (unsigned char *)tlv_ptr;
		char *tlv_str = cfm_print_get_tlv_str(tlv_ptr, tlv_total_len);
		if (strlen(tlv_str))
			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "RecvTLV:%s\n", tlv_str);
	}	
	
	return 0;
}

// return str to be diplayed as short as possible in a formatted space
static char *
short_ccm_interval_str(unsigned char interval)
{
	static char *ccm_interval_str2[] ={ "-", "3.3ms", "10ms", "100ms", "1s", "10s", "1m", "10m" };
	static char interval_str[16] = {0};
	snprintf(interval_str, 15, "%d(%s)", interval, ccm_interval_str2[interval&7]);	
	return interval_str;
}

void
cfm_print_all_mp_status(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config = NULL;
	char *mp_dir_str;

	unsigned int total_mp = 0;
	unsigned int total_tx = 0, total_rx = 0, total_drop = 0, total_fwd = 0;
	unsigned int srvmep_tx = 0, srvmep_rx = 0, srvmep_drop = 0, srvmep_fwd = 0;
	unsigned char opcode;

	util_fdprintf(fd, 
		"%3s %8s %4s %4s "
		"%3s %4s %3s %9s %3s "
		"%3s %8s %10s %1s %3s "
		"%3s %8s %3s %4s %4s "
		"%5s %5s\n",
		"idx", "bport", "dir", "type", 
		"MDL", "MPid", "adm", "Vid/Assoc", "pri", 
		"CCM", "interval", "seq", "T", "#db",
		"AIS", "interval", "pri", "cMDL", "sMEP", 
		"#dfct", "#alrm");
	util_fdprintf(fd, 
		"%3s %8s %4s %4s "
		"%3s %4s %3s %9s %3s "
		"%3s %8s %10s %1s %3s "
		"%3s %8s %3s %4s %4s "
		"%5s %5s\n",
		line3, line8, line4, line4, 
		line3, line4, line3, line9, line3, 
		line3, line8, line10, "-", line3,
		line3, line8, line3, line4, line4, 
		line5, line5);
			
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {		    

			if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) {	// MEP
				mp_dir_str = (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP)?"up":"down";
			} else {	// MIP
				mp_dir_str = "-"; 
			}
			util_fdprintf(fd, 
				"%3d %4s%d(%d) %4s %4s ",
				cfm_config->index, porttype_str[cfm_config->port_type%8], 
				cfm_config->port_type_index, cfm_config->logical_port_id,
				mp_dir_str, (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)?"MEP":"MIP");
			util_fdprintf(fd, 
				"%3u %4u %3d %4u/%-4u %3d ",
				cfm_config->md_level, cfm_config->mep_id, cfm_config->admin_state, 
				cfm_config->pri_vlan, cfm_config->assoc_vlans[0], cfm_config->priority);
	
			if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) {	// MEP
				int rmep_total = 0;
				cfm_pkt_rmep_entry_t *entry;
	
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node)
					rmep_total++;
				util_fdprintf(fd, 
					"%3d %8s %10u %1d %3d ",
					(cfm_config->mep_control & MEP_CONTROL_CCM_ENABLE)?1:0, 
					short_ccm_interval_str(cfm_config->ccm_interval), 
					cfm_config->CCIsentCCMs, 
					cfm_config->lmm_dual_test_enable?1:0,
					rmep_total);
				util_fdprintf(fd, 
					"%3d %4d(%2s) %3d %4d %4d ",		
					(cfm_config->mep_control & MEP_CONTROL_AIS_ENABLE)?1:0, 
					cfm_config->eth_ais_control&1, (cfm_config->eth_ais_control&1)?"1m":"1s",
					(cfm_config->eth_ais_control>>1)&7, (cfm_config->eth_ais_control>>4)&7, 
					(cfm_config->mep_control & MEP_CONTROL_SERVER_MEP_ENABLE)?1:0);
				util_fdprintf(fd, 
					" 0x%02x  0x%02x\n",
					cfm_config->defect_status, cfm_config->alarm_status);
			} else {
				util_fdprintf(fd, 
					"%3s %8s %10s %1s %3s "
					"%3s %8s %3s %4s %4s "
					"%5s %5s\n",
					"-", "-", "-", "-", "-",
					"-", "-", "-", "-", "-", 
					"-", "-");
			}

			for (opcode=0; opcode<= CFM_PDU_MAX; opcode++) {
				total_tx += cfm_config->cfm_pkt_pm[opcode].tx;
				total_rx += cfm_config->cfm_pkt_pm[opcode].rx;
				total_drop += cfm_config->cfm_pkt_pm[opcode].drop;
				total_fwd += cfm_config->cfm_pkt_pm[opcode].fwd;
			}
			total_mp++;
		}
	}
	
	for (opcode=0; opcode<= CFM_PDU_MAX; opcode++) {
		srvmep_tx += server_mep_cfm_config_g.cfm_pkt_pm[opcode].tx;
		srvmep_rx += server_mep_cfm_config_g.cfm_pkt_pm[opcode].rx;
		srvmep_drop += server_mep_cfm_config_g.cfm_pkt_pm[opcode].drop;
		srvmep_fwd += server_mep_cfm_config_g.cfm_pkt_pm[opcode].fwd;
	}

	if (srvmep_tx || srvmep_rx || srvmep_drop || srvmep_fwd) {
		util_fdprintf(fd, "total %d mep/mip: tx %u, rx %u, drop %u, fwd %u   srvmep: tx %u. rx %u, drop %u, fwd %u\n",
				total_mp, total_tx, total_rx, total_drop, total_fwd, srvmep_tx, srvmep_rx, srvmep_drop, srvmep_fwd);
	} else {
		util_fdprintf(fd, "total %d mep/mip: tx %u, rx %u, drop %u, fwd %u\n",
				total_mp, total_tx, total_rx, total_drop, total_fwd);
	}
}		

static void
cfm_print_cfm_ccm_pdu(int fd, cfm_hdr_t *cfm_hdr)
{
	cfm_pdu_ccm_t *ccm=(cfm_pdu_ccm_t *)((void*)cfm_hdr+sizeof(cfm_hdr_t));
	
	util_fdprintf(fd, "MDL=%d, rdi%s, period=%d, seq=0x%x, MEPid=%lu, MAid=%s\n", 
		cfm_hdr->md_level, ((cfm_hdr->flags>>7)&1)?"+":"-", cfm_hdr->flags&7,
		ntohl(ccm->seq), ntohs(ccm->mep_id), cfm_print_get_maid_str(ccm->ma_id));
}

int
cfm_print_mp_detail(int fd, cfm_config_t *cfm_config)
{
	unsigned long long now = cfm_util_get_time_in_us();
	unsigned int ais_interval_us;
	int i;
	
	if (cfm_config == NULL)
		return -1;

	util_fdprintf(fd, "index: %d, ", cfm_config->index);
	util_fdprintf(fd, "macaddr: %s\n", util_str_macaddr(cfm_config->macaddr));
	// port info
	util_fdprintf(fd, "port_type: %d(%s), ", cfm_config->port_type, porttype_str[cfm_config->port_type%8]);
	util_fdprintf(fd, "port_type_index: %d, ", cfm_config->port_type_index);
	util_fdprintf(fd, "logical_port_id: %d\n", cfm_config->logical_port_id);
	// mep/mip & md level	
	if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) {	// MEP
		util_fdprintf(fd, "%sMEP id: %d(0x%x), ", 
			(cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP)?"up":"down", 
			cfm_config->mep_id,  cfm_config->mep_id);	
	} else {	// MIP
		util_fdprintf(fd, "MIP id: %d(0x%x), ", cfm_config->mep_id,  cfm_config->mep_id);
	}
	util_fdprintf(fd, "MD level: %d\n", cfm_config->md_level);

	util_fdprintf(fd, "\n");
		
	// MD
	if (cfm_config->md_format == MD_NAME_FORMAT_NONE) {
		util_fdprintf(fd, "MD format: %d(%s), MD name:\n", 
			cfm_config->md_format, md_format_str[cfm_config->md_format]);
	} else if (cfm_config->md_format == MD_NAME_FORMAT_DNS ||
	           cfm_config->md_format == MD_NAME_FORMAT_STRING) {
	        char md_name[49] = {0};	
		strncpy(md_name, cfm_config->md_name_1, 25); md_name[25]=0;
		strncat(md_name, cfm_config->md_name_2, 48-strlen(md_name)); md_name[48]=0;
		util_fdprintf(fd, "MD format: %d(%s), MD name: %s\n", 
			cfm_config->md_format, md_format_str[cfm_config->md_format], md_name);
	} else if (cfm_config->md_format == MD_NAME_FORMAT_MACADDR_SHORTINT) {
		util_fdprintf(fd, "MD format: %d(%s), MD name: %s 0x%02x%02x\n", 
			cfm_config->md_format, md_format_str[cfm_config->md_format],
			util_str_macaddr(cfm_config->md_name_1), cfm_config->md_name_1[6], cfm_config->md_name_1[7]);
	} else { // MD_NAME_FORMAT_ICC is not supported
		util_fdprintf(fd, "MD format: %d(?), MD name: %c%c%c%c%c%c%c%c%c%c%c%c%c...\n",  cfm_config->md_format, 
			cfm_config->md_name_1[0], cfm_config->md_name_1[1], cfm_config->md_name_1[2], cfm_config->md_name_1[3],
			cfm_config->md_name_1[4], cfm_config->md_name_1[5], cfm_config->md_name_1[6], cfm_config->md_name_1[7],
			cfm_config->md_name_1[8], cfm_config->md_name_1[9], cfm_config->md_name_1[10], cfm_config->md_name_1[11],
			cfm_config->md_name_1[12]);
	}
	
	// MA	
	if (cfm_config->ma_format == MA_NAME_FORMAT_VID ||
	    cfm_config->ma_format == MA_NAME_FORMAT_SHORTINT) {
		util_fdprintf(fd, "MA format: %d(%s), MA name: 0x%02x%02x\n", 
			cfm_config->ma_format, ma_format_str[cfm_config->ma_format], cfm_config->ma_name_1[0], cfm_config->ma_name_1[1]);
	} else if (cfm_config->ma_format == MA_NAME_FORMAT_STRING) {
	        char ma_name[49] = {0};	
		strncpy(ma_name, cfm_config->ma_name_1, 25); ma_name[25]=0;
		strncat(ma_name, cfm_config->ma_name_2, 48-strlen(ma_name)); ma_name[48]=0;
		util_fdprintf(fd, "MA format: %d(%s), MA name: %s\n", 
			cfm_config->ma_format, ma_format_str[cfm_config->ma_format], ma_name);
	} else if (cfm_config->ma_format == MA_NAME_FORMAT_VPNID) {
		unsigned char *m = cfm_config->ma_name_1;
		util_fdprintf(fd, "MA format: %d(%s), MA name: 0x%02x%02x%02x 0x%02x%02x%02x%02x\n", 
			cfm_config->ma_format, ma_format_str[cfm_config->ma_format],
			m[0],m[1],m[2], m[3],m[4],m[5],m[6]);
	} else {
		util_fdprintf(fd, "MA format: %d(?), MA name: %c%c%c%c%c%c%c%c%c%c%c%c%c...\n",  cfm_config->ma_format, 
			cfm_config->ma_name_1[0], cfm_config->ma_name_1[1], cfm_config->ma_name_1[2], cfm_config->ma_name_1[3],
			cfm_config->ma_name_1[4], cfm_config->ma_name_1[5], cfm_config->ma_name_1[6], cfm_config->ma_name_1[7],
			cfm_config->ma_name_1[8], cfm_config->ma_name_1[9], cfm_config->ma_name_1[10], cfm_config->ma_name_1[11],
			cfm_config->ma_name_1[12]);
	}

	// sender_id_permission
	{
		char *sender_id_permission_str[] = { "?", "none", "chassis", "manage", "chassis+manage", "defer" };
		char *s = "?";
		if (cfm_config->ma_sender_id_permission <=5)
			s = sender_id_permission_str[cfm_config->ma_sender_id_permission];
		util_fdprintf(fd, "sender_id_permission: %d(%s)\n", cfm_config->ma_sender_id_permission, s);
	}

	// admin lock
	util_fdprintf(fd, "admin locked: %d\n", (cfm_config->admin_state)?1:0);	
	// vlan
	util_fdprintf(fd, "primary vlan: %d, priority: %d\n", cfm_config->pri_vlan, cfm_config->priority);
	util_fdprintf(fd, "primary pbit: %d, pbit filter: %s\n", cfm_config->pri_pbit, (cfm_config->pbit_filter)?"enabled":"disabled");
	{
		unsigned short *v=cfm_config->assoc_vlans;
		util_fdprintf(fd, "assoc vlan(s): %d %d %d %d  %d %d %d %d  %d %d %d %d\n", 
			v[0], v[1], v[2], v[3],  v[4], v[5], v[6], v[7],  v[8], v[9], v[10], v[11]);
	}
	// ltr
	util_fdprintf(fd, "LBR enabled: %d, ", (cfm_config->mep_control2 & MEP_CONTROL2_LBR_ENABLE)?1:0);
	util_fdprintf(fd, "LTR enabled: %d, ", (cfm_config->mep_control2 & MEP_CONTROL2_LTR_ENABLE)?1:0);
	util_fdprintf(fd, "LBM xid: %d(0x%x), ", cfm_config->nextLBMtransID, cfm_config->nextLBMtransID);
	util_fdprintf(fd, "LTM xid: %d(0x%x)\n", cfm_config->nextLTMtransID, cfm_config->nextLTMtransID);

	// dmm/lmm
	util_fdprintf(fd, "DMM enabled: %d, ", (cfm_config->mep_control2 & MEP_CONTROL2_DMM_ENABLE)?1:0);
	util_fdprintf(fd, "LMM enabled: %d\n", (cfm_config->mep_control2 & MEP_CONTROL2_LMM_ENABLE)?1:0);
	
	// data tlv
	util_fdprintf(fd, "Data TLV payload format: %d (0:skip,1:data,2:test), ", cfm_config->payload_format);
	util_fdprintf(fd, "length: %d, pattern: 0x%04x\n", cfm_config->tlv_data_length, cfm_config->payload_pattern);
	
	if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE)	// MIP
		return 0;	// info below is for mep only

	// ccm 
	util_fdprintf(fd, "CCM enabled: %d, ", (cfm_config->mep_control & MEP_CONTROL_CCM_ENABLE)?1:0);	
	util_fdprintf(fd, "interval: %d(%s), ", cfm_config->ccm_interval, ccm_interval_str[cfm_config->ccm_interval&7]);
	util_fdprintf(fd, "seq number: %d(0x%x), ", cfm_config->CCIsentCCMs, cfm_config->CCIsentCCMs);
	util_fdprintf(fd, "auto discovery enable: %d, ", cfm_config->auto_discovery);
	util_fdprintf(fd, "lmm_dual test enable: %d\n", cfm_config->lmm_dual_test_enable);
	// ais
	util_fdprintf(fd, "AIS enabled: %d, ", (cfm_config->mep_control & MEP_CONTROL_AIS_ENABLE)?1:0);	
	util_fdprintf(fd, "interval: %s, ", (cfm_config->eth_ais_control&1)?"1 minute":"1 sec");	
	util_fdprintf(fd, "priority: %d, ", (cfm_config->eth_ais_control>>1)&7);	
	util_fdprintf(fd, "client MD level: %d%s\n", (cfm_config->eth_ais_control>>4)&7, (((cfm_config->eth_ais_control>>4)&7)==0)?"(auto)":"");
	// server mep
	util_fdprintf(fd, "serverMEP enabled: %d\n", (cfm_config->mep_control & MEP_CONTROL_SERVER_MEP_ENABLE)?1:0);
	// peer_mep_id
	{
		unsigned short *p=cfm_config->peer_mep_ids;
		util_fdprintf(fd, "peer_mep_id(s): %d %d %d %d  %d %d %d %d  %d %d %d %d\n", 
			p[0], p[1], p[2], p[3],  p[4], p[5], p[6], p[7],  p[8], p[9], p[10], p[11]);
	}
	util_fdprintf(fd, "\n");	
	
	// alarm config
	util_fdprintf(fd, "alarm_declaration_soak_time: %d ms\n", cfm_config->alarm_declaration_soak_time);
	util_fdprintf(fd, "alarm_clear_soak_time: %d ms\n", cfm_config->alarm_clear_soak_time);
	util_fdprintf(fd, "fault_alarm_threshold: %d (allowed alarm_mask=0x%02x)\n", 
		cfm_config->fault_alarm_threshold, alam_threshold_bitmap[cfm_config->fault_alarm_threshold]);
	// defect/alarm status
	util_fdprintf(fd, "%12s %4s %6s %15s  %5s %7s %12s\n", "condition", "prio", "DEFECT", "age since", "ALARM", "allowed", "supressed by");
	util_fdprintf(fd, "%12s %4s %6s %15s  %5s %7s %12s\n", "------------", line4, line6, "---------------", line5, line7, "------------");
	ais_interval_us = ((cfm_config->eth_ais_control&1)?60:1) *1000 *1000;
	for (i=0; i<DEFECT_INVALID; i++) {
		static char defect_priority[7] = {
			PRIORITY_DEFECT_RDI_CCM,
			PRIORITY_DEFECT_MAC_STATUS,
			PRIORITY_DEFECT_REMOTE_CCM,
			PRIORITY_DEFECT_ERROR_CCM,
			PRIORITY_DEFECT_XCON_CCM,
			PRIORITY_DEFECT_UNEXP_PERIOD,
			PRIORITY_DEFECT_AIS
		};
		char supressed_str[16] = {0};
		if ((omci_env_g.cfm_defect_mask_supressed_by_ais & (1<<i)) && (cfm_config->defect_status & (1<<DEFECT_AIS)))
			strcat(supressed_str, "ais- ");
		if ((omci_env_g.cfm_defect_mask_supressed_by_lck & (1<<i)) && (now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5)) 
			strcat(supressed_str, "lck- ");			
		util_fdprintf(fd, "%12s %4d %6d %15s  %5d %7s %12s\n", 
			defect_str[i], defect_priority[i], 
			(cfm_config->defect_status & (1<<i))?1:0, 
			cfm_print_get_us_time_str(now - cfm_config->defect_status_timestamp[i]),
			(cfm_config->alarm_status & (1<<i))?1:0, 
			(alam_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<i))?"+ ":"",
			supressed_str
			);
	}
	// print LCK recv status after the last line of defect
	util_fdprintf(fd, "%12s %4s %6d %15s  %5s %7s %12s\n", 
		"LCK", "-", 
		(now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5)?1:0, 
		cfm_print_get_us_time_str(now - cfm_config->recv_lck_timestamp),
		"-", "- ", "-");

	{
		unsigned char rdi_gen, ais_gen, lck_gen, alarm_gen;
		cfm_util_get_rdi_ais_lck_alarm_gen(cfm_config, &rdi_gen, &ais_gen, &lck_gen, &alarm_gen);
		util_fdprintf(fd, "defect_status=0x%02x, alarm_status=0x%02x, rdi_gen:%s, ais_gen:%s, lck_gen:%s, alarm_gen:%s\n",
			cfm_config->defect_status, cfm_config->alarm_status, 
			rdi_gen?"Y":"N", ais_gen?"Y":"N", lck_gen?"Y":"N", alarm_gen?"Y":"N");
	}
	util_fdprintf(fd, "\n");	

	// misc error statistics	
	util_fdprintf(fd, "highest_priority_defect_observed: %d\n", cfm_config->highest_priority_defect_observed);
	util_fdprintf(fd, "invalid_senderid_ccm_count: %lu\n", cfm_config->invalid_senderid_ccm_count);
	util_fdprintf(fd, "invalid_port_status_ccm_count: %lu\n", cfm_config->invalid_port_status_ccm_count);
	util_fdprintf(fd, "invalid_interface_status_ccm_count: %lu\n", cfm_config->invalid_interface_status_ccm_count);
	util_fdprintf(fd, "out_of_seqence_ccm_count: %lu\n", cfm_config->out_of_seqence_ccm_count);
	util_fdprintf(fd, "unexpected_ltr_count: %lu\n", cfm_config->unexpected_ltr_count);
	util_fdprintf(fd, "unexpected_period_timestamp=%lld\n", cfm_config->defect_unexp_period_timestamp);
	// last err_ccm/xcon_ccm pkt
	if (cfm_config->last_recv_err_ccm_msg_len) {
		util_fdprintf(fd, "last recv_err_ccm: len=%d\n", cfm_config->last_recv_err_ccm_msg_len);
		cfm_print_cfm_ccm_pdu(fd, (cfm_hdr_t *)cfm_config->last_recv_err_ccm_msg);     
	}
	if (cfm_config->last_xcon_err_ccm_msg_len) {
		util_fdprintf(fd, "last xcon_err_ccm: len=%d\n", cfm_config->last_xcon_err_ccm_msg_len); 
		cfm_print_cfm_ccm_pdu(fd, (cfm_hdr_t *)cfm_config->last_xcon_err_ccm_msg);     
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void
cfm_print_mp_pkt_counter_8(int fd, int md_level, int mep_id, 
	char *category, unsigned char opcode1, unsigned char opcode2)
{
	cfm_config_t *cfm_config;
	char *opcode1_str, *opcode2_str;
	
	if (opcode1 > CFM_PDU_MAX)
		opcode1 = CFM_PDU_INVALID;
	opcode1_str = cfm_print_get_opcode_str(opcode1);
	
	if (opcode2 > CFM_PDU_MAX)
		opcode2 = CFM_PDU_INVALID;
	opcode2_str = cfm_print_get_opcode_str(opcode2);

	util_fdprintf(fd, "%s:\n", category);
	util_fdprintf(fd, "%3s %3s %4s %5s tx %5s rx %4s drp %4s fwd %5s tx %5s rx %4s drp %4s fwd\n",
		"idx", "MDL", "MPid", 
		opcode1_str, opcode1_str, opcode1_str, opcode1_str, 
		opcode2_str, opcode2_str, opcode2_str, opcode2_str);
	util_fdprintf(fd, "%3s %3s %4s %8s %8s %8s %8s %8s %8s %8s %8s\n",
		line3, line3, line4, line8, line8, line8, line8, line8, line8, line8, line8);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			util_fdprintf(fd, "%3u %3u %4u %8u %8u %8u %8u %8u %8u %8u %8u\n",
				cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
				cfm_config->cfm_pkt_pm[opcode1].tx,
				cfm_config->cfm_pkt_pm[opcode1].rx,
				cfm_config->cfm_pkt_pm[opcode1].drop,
				cfm_config->cfm_pkt_pm[opcode1].fwd,
				cfm_config->cfm_pkt_pm[opcode2].tx,
				cfm_config->cfm_pkt_pm[opcode2].rx,
				cfm_config->cfm_pkt_pm[opcode2].drop,
				cfm_config->cfm_pkt_pm[opcode2].fwd);
		}
	}
	if (md_level == server_mep_cfm_config_g.md_level &&
	    mep_id == server_mep_cfm_config_g.mep_id) {
	    	cfm_config = &server_mep_cfm_config_g;
		util_fdprintf(fd, "%3u %3u %4u %8u %8u %8u %8u %8u %8u %8u %8u\n",
			cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
			cfm_config->cfm_pkt_pm[opcode1].tx,
			cfm_config->cfm_pkt_pm[opcode1].rx,
			cfm_config->cfm_pkt_pm[opcode1].drop,
			cfm_config->cfm_pkt_pm[opcode1].fwd,
			cfm_config->cfm_pkt_pm[opcode2].tx,
			cfm_config->cfm_pkt_pm[opcode2].rx,
			cfm_config->cfm_pkt_pm[opcode2].drop,
			cfm_config->cfm_pkt_pm[opcode2].fwd);
	}
}
static void
cfm_print_mp_pkt_counter_6(int fd, int md_level, int mep_id, 
	char *category, unsigned char opcode)
{
	cfm_config_t *cfm_config;
	char *opcode_str;

	if (opcode > CFM_PDU_MAX)
		opcode = CFM_PDU_INVALID;
	opcode_str = cfm_print_get_opcode_str(opcode);

	util_fdprintf(fd, "%s:\n", category);
	util_fdprintf(fd, "%3s %3s %4s %7s tx %7s rx %6s tx %6s rx %6s drp %6s fwd\n",
		"idx", "MDL", "MPid", 
		opcode_str, opcode_str, "RDI", "RDI", opcode_str, opcode_str);
	util_fdprintf(fd, "%3s %3s %4s %10s %10s %10s %10s %10s %10s\n",
		line3, line3, line4, line10, line10, line10, line10, line10, line10);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id))
			util_fdprintf(fd, "%3u %3u %4u %10u %10u %10u %10u %10u %10u\n",
				cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
				cfm_config->cfm_pkt_pm[opcode].tx,
				cfm_config->cfm_pkt_pm[opcode].rx,
				cfm_config->cfm_pkt_pm[opcode].tx_rdi,
				cfm_config->cfm_pkt_pm[opcode].rx_rdi,
				cfm_config->cfm_pkt_pm[opcode].drop,
				cfm_config->cfm_pkt_pm[opcode].fwd);
	}
	if (md_level == server_mep_cfm_config_g.md_level &&
	    mep_id == server_mep_cfm_config_g.mep_id) {
	    	cfm_config = &server_mep_cfm_config_g;
		util_fdprintf(fd, "%3u %3u %4u %10u %10u %10u %10u %10u %10u\n",
			cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
			cfm_config->cfm_pkt_pm[opcode].tx,
			cfm_config->cfm_pkt_pm[opcode].rx,
			cfm_config->cfm_pkt_pm[opcode].tx_rdi,
			cfm_config->cfm_pkt_pm[opcode].rx_rdi,
			cfm_config->cfm_pkt_pm[opcode].drop,
			cfm_config->cfm_pkt_pm[opcode].fwd);
	}
}
static void
cfm_print_mp_pkt_counter_4(int fd, int md_level, int mep_id, 
	char *category, unsigned char opcode)
{
	cfm_config_t *cfm_config;
	char *opcode_str;

	if (opcode > CFM_PDU_MAX)
		opcode = CFM_PDU_INVALID;
	opcode_str = cfm_print_get_opcode_str(opcode);

	util_fdprintf(fd, "%s:\n", category);
	util_fdprintf(fd, "%3s %3s %4s %7s tx %7s rx %6s drp %6s fwd\n",
		"idx", "MDL", "MPid", 
		opcode_str, opcode_str, opcode_str, opcode_str);
	util_fdprintf(fd, "%3s %3s %4s %10s %10s %10s %10s\n",
		line3, line3, line4, line10, line10, line10, line10);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id))
			util_fdprintf(fd, "%3u %3u %4u %10u %10u %10u %10u\n",
				cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
				cfm_config->cfm_pkt_pm[opcode].tx,
				cfm_config->cfm_pkt_pm[opcode].rx,
				cfm_config->cfm_pkt_pm[opcode].drop,
				cfm_config->cfm_pkt_pm[opcode].fwd);
	}
	if (md_level == server_mep_cfm_config_g.md_level &&
	    mep_id == server_mep_cfm_config_g.mep_id) {
	    	cfm_config = &server_mep_cfm_config_g;
		util_fdprintf(fd, "%3u %3u %4u %10u %10u %10u %10u\n",
			cfm_config->index, cfm_config->md_level, cfm_config->mep_id,
			cfm_config->cfm_pkt_pm[opcode].tx,
			cfm_config->cfm_pkt_pm[opcode].rx,
			cfm_config->cfm_pkt_pm[opcode].drop,
			cfm_config->cfm_pkt_pm[opcode].fwd);
	}
}

void
cfm_print_mp_pkt_counter(int fd, int opcode, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	
	if (mep_id == -1) {	// clear all counters to zero
		list_for_each_entry(cfm_config, &cfm_config_list, node)
			memset(&cfm_config->cfm_pkt_pm, 0, sizeof(cfm_config->cfm_pkt_pm));
		memset(&server_mep_cfm_config_g.cfm_pkt_pm, 0, sizeof(server_mep_cfm_config_g.cfm_pkt_pm));
		return;
	}
					
	if (opcode == 0xff || opcode == CFM_PDU_LBM || opcode == CFM_PDU_LBR) {
		cfm_print_mp_pkt_counter_8(fd, md_level, mep_id, "Loopback", CFM_PDU_LBM, CFM_PDU_LBR);
	}
	if (opcode == 0xff || opcode == CFM_PDU_LTM || opcode == CFM_PDU_LTR) {
		cfm_print_mp_pkt_counter_8(fd, md_level, mep_id, "Linktrace", CFM_PDU_LTM, CFM_PDU_LTR);
	}
	if (opcode == 0xff || opcode == CFM_PDU_CCM) {
		cfm_print_mp_pkt_counter_6(fd, md_level, mep_id, "Continuity Check", CFM_PDU_CCM);
	}
		
	// counters below are for y.1731 only		
	if(omci_env_g.cfm_y1731_enable == 0)
		return;
	
	if (opcode == 0xff || opcode == CFM_PDU_AIS) {
		cfm_print_mp_pkt_counter_4(fd, md_level, mep_id, "Alarm Indication", CFM_PDU_AIS);
	}
	if (opcode == 0xff || opcode == CFM_PDU_LCK) {
		cfm_print_mp_pkt_counter_4(fd, md_level, mep_id, "Lock", CFM_PDU_LCK);
	}
	if (opcode == 0xff || opcode == CFM_PDU_DMM || opcode == CFM_PDU_DMR) {
		cfm_print_mp_pkt_counter_8(fd, md_level, mep_id, "Delay Measurement", CFM_PDU_DMM, CFM_PDU_DMR);
	}
	if (opcode == 0xff || opcode == CFM_PDU_1DM) {
		cfm_print_mp_pkt_counter_4(fd, md_level, mep_id, "One-way Delay Measurement", CFM_PDU_1DM);
	}
	if (opcode == 0xff || opcode == CFM_PDU_TST) {
		cfm_print_mp_pkt_counter_4(fd, md_level, mep_id, "Test signal", CFM_PDU_TST);
	}
	if (opcode == 0xff || opcode == CFM_PDU_LMM || opcode == CFM_PDU_LMR) {
		cfm_print_mp_pkt_counter_8(fd, md_level, mep_id, "Loss Measurement", CFM_PDU_LMM, CFM_PDU_LMR);
	}
	if (opcode == 0xff || opcode == CFM_PDU_SLM || opcode == CFM_PDU_SLR) {
		cfm_print_mp_pkt_counter_8(fd, md_level, mep_id, "Synthetic Loss Measurement", CFM_PDU_SLM, CFM_PDU_SLR);
	}
	if (opcode == 0xff || opcode == CFM_PDU_1SL) {
		cfm_print_mp_pkt_counter_4(fd, md_level, mep_id, "One-way Synthetic Loss Measurement", CFM_PDU_1SL);
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////

void
cfm_print_lbr_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	cfm_pkt_lbr_entry_t *entry;

	util_fdprintf(fd, "%3s %3s %4s  %17s %10s %9s\n",
		"idx", "MDL", "MPid", "LBR MAC", "xid", "rsptime");
	util_fdprintf(fd, "%3s %3s %4s  %17s %10s %9s\n",
		line3, line3, line4, "-----------------", line10, line9);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			list_for_each_entry(entry, &cfm_config->cfm_recv_lbr_list, node) {
				unsigned int resptime = entry->lbr_timestamp - cfm_config->cfm_send_lbm_timestamp;
				util_fdprintf(fd, "%3u %3u %4u  %17s 0x%08x %3u.%03ums\n",
					cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
					util_str_macaddr(entry->lbr_mac), 
					ntohl(entry->u.lbr.lbr_pdu.xid),
					resptime/1000, resptime%1000);
			}
		}
	}
	return;
}

void
cfm_print_ltm_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	cfm_pkt_ltm_entry_t *entry;

	util_fdprintf(fd, "%3s %3s %4s  %17s %10s\n",
		"idx", "MDL", "MPid", "LTM MAC", "xid");
	util_fdprintf(fd, "%3s %3s %4s  %17s %10s\n",
		line3, line3, line4, "-----------------", line10);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			list_for_each_entry(entry, &cfm_config->cfm_recv_ltm_list, node) {
				util_fdprintf(fd, "%3u %3u %4u  %17s 0x%08x\n",
					cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
					util_str_macaddr(entry->ltm_mac), 
					ntohl(entry->xid));
			}
		}
	}
	return;
}

void
cfm_print_ltr_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	cfm_pkt_ltr_entry_t *entry;

	util_fdprintf(fd, "%3s %3s %4s  %17s %10s %3s %9s\n",
		"idx", "MDL", "MPid", "LTR MAC", "xid", "ttl", "rsptime");
	util_fdprintf(fd, "%3s %3s %4s  %17s %10s %3s %9s\n",
		line3, line3, line4, "-----------------", line10, line3, line9);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
				unsigned int resptime = entry->ltr_timestamp - cfm_config->cfm_send_ltm_timestamp;
				util_fdprintf(fd, "%3u %3u %4u  %17s 0x%08x %3u %3u.%03ums\n",
					cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
					util_str_macaddr(entry->ltr_mac), 
					ntohl(entry->u.ltr.ltr_pdu.xid), entry->u.ltr.ltr_pdu.ttl,
					resptime/1000, resptime%1000);
			}
		}
	}
	return;
}

void
cfm_print_rmep_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	cfm_pkt_rmep_entry_t *entry;

	util_fdprintf(fd, "%3s %3s %5s  %17s %6s %8s %3s %10s %8s %13s\n",
		"idx", "MDL", "MEPid", "CCM_MAC", "rMEPid", "interval", "rdi", "seq", "state", "last CCM age");
	util_fdprintf(fd, "%3s %3s %5s  %17s %6s %8s %3s %10s %8s %13s\n",
		line3, line3, line5, "-----------------", line6, line8, line3, line10, line8, "-------------");
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				unsigned long long now = cfm_util_get_time_in_us();
				util_fdprintf(fd, "%3u %3u %5u  %17s %6u %8s %3d %10u %8s %13s\n",
					cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
					util_str_macaddr(entry->rmep_mac), 
					entry->rmep_id, short_ccm_interval_str(entry->rmep_interval),
					entry->rmep_rdi, entry->rmep_seq, 
					cfm_print_get_rmep_state_str(entry->rmep_state),
					cfm_print_get_us_time_str(abs(now - entry->rmep_timestamp)));
			}
		}
	}
	return;
}

static int
cfm_lm_expect_len( int fd, unsigned int time_len, char time_len_str[12] )
{
	unsigned int sec=0;
	unsigned int min=0;
	unsigned int hour=0;
	unsigned int day=0;

	memset( time_len_str, 0, 12);
	if ( time_len == 0xffff )
		strcpy( time_len_str, "0xffff");
	else {
		sec = time_len%60;
		min = time_len/60;
		if ( min >= 60 ) {
			hour = min/60;
			min = min%60;
			if (hour >= 24) {
				day = hour/24;
				hour = hour%24;
			}
		}

		if ( day != 0 ) 
			snprintf( time_len_str, 12, "%ud%uh",day, hour);
		else if ( hour != 0 )
			snprintf( time_len_str, 12, "%uh%um", hour, min);
		else if ( min != 0 )
			snprintf( time_len_str, 12, "%um%us", min, sec);
		else 
			snprintf( time_len_str, 12, "%us", sec);
	}
	return 0;
}

void
cfm_print_lm_test_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	cfm_pkt_rmep_entry_t *entry;
	char start_time[15];
	char expect_len[12];
	char end_time[15];

	util_fdprintf(fd, "%3s %3s %5s %6s %10s %2s %14s %12s %14s %8s %8s %8s %8s\n",
		"idx", "MDL", "MEPid", "rMEPid", "test", "go", "start time", "expect len", "end time", "near tx", "near rx", "far tx", "far rx");
	util_fdprintf(fd, "%3s %3s %5s %6s %10s %2s %14s %12s %14s %8s %8s %8s %8s\n",
		line3, line3, line5, line6, line10, "--", line14, line12, line14, line8, line8, line8, line8 );
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if ( entry->eth_lm_single_start_time != 0 ) {
					strftime( start_time, sizeof(start_time), "%m/%d %H:%M:%S",localtime(&entry->eth_lm_single_start_time));
					if ( entry->eth_lm_single_stop_time != 0 )
						strftime( end_time, sizeof(end_time), "%m/%d %H:%M:%S",localtime(&entry->eth_lm_single_stop_time));

					cfm_lm_expect_len( fd, entry->eth_lm_single_expect_time, expect_len );
					util_fdprintf(fd, "%3u %3u %5u %6u %10s %2u %14s %12s %14s %8u %8u %8u %8u\n",
						cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
						entry->rmep_id, "lmm_single",
						entry->eth_lm_single_going_on,
						start_time, expect_len, 
						entry->eth_lm_single_stop_time?end_time:"0", 
						entry->eth_lm_single_tx_nloss, entry->eth_lm_single_rx_nloss, 
						entry->eth_lm_single_tx_floss, entry->eth_lm_single_rx_floss );
				}
				if ( entry->eth_lm_dual_start_time != 0 ) {
					strftime( start_time, sizeof(start_time), "%m/%d %H:%M:%S", localtime(&entry->eth_lm_dual_start_time));
					if ( entry->eth_lm_dual_stop_time != 0 )
						strftime( end_time, sizeof(end_time), "%m/%d %H:%M:%S",localtime(&entry->eth_lm_dual_stop_time));
					cfm_lm_expect_len( fd, entry->eth_lm_dual_expect_time, expect_len );
					util_fdprintf(fd, "%3u %3u %5u %6u %10s %2u %14s %12s %14s %8u %8u %8u %8u\n",
						cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
						entry->rmep_id, "lmm_dual",
						entry->eth_lm_dual_going_on,
						start_time, expect_len, 
						entry->eth_lm_dual_stop_time?end_time:"-", 
						entry->eth_lm_dual_tx_nloss, entry->eth_lm_dual_rx_nloss, 
						entry->eth_lm_dual_tx_floss, entry->eth_lm_dual_rx_floss );
				}
				if ( entry->eth_slm_single_start_time != 0 ) {
					strftime( start_time, sizeof(start_time), "%m/%d %H:%M:%S", localtime(&entry->eth_slm_single_start_time));
					if ( entry->eth_slm_single_stop_time != 0 )
						strftime( end_time, sizeof(end_time), "%m/%d %H:%M:%S",localtime(&entry->eth_slm_single_stop_time));
					cfm_lm_expect_len( fd, entry->eth_slm_single_expect_time, expect_len );
					util_fdprintf(fd, "%3u %3u %5u %6u %10s %2u %14s %12s %14s %8u %8u %8u %8u\n",
						cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
						entry->rmep_id, "slm_single",
						entry->eth_slm_single_going_on,
						start_time, expect_len, 
						entry->eth_slm_single_stop_time?end_time:"-", 
						entry->eth_slm_single_tx_nloss, entry->eth_slm_single_rx_nloss, 
						entry->eth_slm_single_tx_floss, entry->eth_slm_single_rx_floss );
				}
				if ( entry->eth_slm_dual_start_time != 0 ) {
					strftime( start_time, sizeof(start_time), "%m/%d %H:%M:%S", localtime(&entry->eth_slm_dual_start_time));
					if ( entry->eth_slm_dual_stop_time != 0 )
						strftime( end_time, sizeof(end_time), "%m/%d %H:%M:%S",localtime(&entry->eth_slm_dual_stop_time));
					cfm_lm_expect_len( fd, entry->eth_slm_dual_expect_time, expect_len );
					util_fdprintf(fd, "%3u %3u %5u %6u %10s %2u %14s %12s %14s %8u %8u %8s %8s\n",
						cfm_config->index, cfm_config->md_level, cfm_config->mep_id, 
						entry->rmep_id, "slm_dual",
						entry->eth_slm_dual_going_on,
						start_time, expect_len, 
						entry->eth_slm_dual_stop_time?end_time:"-", 
						entry->eth_slm_dual_tx_nloss, entry->eth_slm_dual_rx_nloss, 
						"-", "-" );
				}
			}
		}
	}
	return;
}
