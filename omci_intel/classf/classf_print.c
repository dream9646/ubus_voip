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
 * Module  : classf
 * File    : classf_print.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "classf.h"
#include "meinfo.h"
#include "util.h"
#include "util_inet.h"

char *filter_op_type_str[] = {
	"Pass",
	"Discard",
	"Postive",
	"Negative"
};

char *filter_inspection_type_str[] = {
	"NON",
	"VID",
	"PRI",
	"TCI"
};

char *filter_op_type_ext_str[] = {
	"Untag_Pass",
	"Untag_Discard",
	"Tag_Pass",
	"Tag_Discard",
	"Tag_Postive_Tci0",
	"Tag_Postive_Tci1",
	"Tag_Postive_Tci2",
	"Tag_Postive_Tci3",
	"Tag_Postive_Tci4",
	"Tag_Postive_Tci5",
	"Tag_Postive_Tci6",
	"Tag_Postive_Tci7",
	"Tag_Postive_Tci8",
	"Tag_Postive_Tci9",
	"Tag_Postive_Tci10",
	"Tag_Postive_Tci11",
	"Tag_NE",
};

char *gem_untag_op_str[] = {
	"DSCP_to_Pbit",
	"Default_Pbit",
	"All_Pass",
	"No_Pass",
	"Invalid",
};

char *gem_dir_str[] = {
	"DI", //disable
	"US", //upstream
	"DS", //downstream
	"BI" //bidirection
};

char *reason_str[] = {
	"OK", //CLASSF_REASON_OK,

	//lan filter
	"LF_UNTAG_DISCARD", //CLASSF_REASON_LF_UNTAG_DISCARD,
	"LF_TAG_DISCARD", //CLASSF_REASON_LF_TAG_DISCARD,
	"LF_PRI_UNMATCH", //CLASSF_REASON_LF_PRI_UNMATCH,
	"LF_VID_UNMATCH", //CLASSF_REASON_LF_VID_UNMATCH,
	"LF_DE_UNMATCH", //CLASSF_REASON_LF_DE_UNMATCH,
	"LF_POSTIVE_DISCARD", //CLASSF_REASON_LF_POSTIVE_DISCARD,
	"LF_TYPE_ERROR", //CLASSF_REASON_LF_TYPE_ERROR,

	//wan filter
	"WF_UNTAG_DISCARD", //CLASSF_REASON_WF_UNTAG_DISCARD,
	"WF_TAG_DISCARD", //CLASSF_REASON_WF_TAG_DISCARD,
	"WF_PRI_UNMATCH", //CLASSF_REASON_WF_PRI_UNMATCH,
	"WF_VID_UNMATCH", //CLASSF_REASON_WF_VID_UNMATCH,
	"WF_DE_UNMATCH", //CLASSF_REASON_WF_DE_UNMATCH,
	"WF_POSTIVE_DISCARD", //CLASSF_REASON_WF_POSTIVE_DISCARD,
	"WF_TYPE_ERROR", //CLASSF_REASON_WF_TYPE_ERROR,

	//gem/pbit filter
	"GEM_UNTAG_DISCARD", //CLASSF_REASON_GEM_UNTAG_DISCARD,
	"GEM_TAG_DISCARD", //CLASSF_REASON_GEM_TAG_DISCARD,
	"GEM_TAG_PBIT_DISCARD", //CLASSF_REASON_GEM_TAG_PBIT_DISCARD,

	//misc error
	"TAGGING_ERROR", //CLASSF_REASON_TAGGING_ERROR,
	"DUPLICATE", //CLASSF_REASON_DUPLICATE,
	"INTERNAL_ERROR", //CLASSF_REASON_INTERNAL_ERROR
	"MC_TAGGING_ERROR", //CLASSF_REASON_MC_TAGGING_ERROR
	"MC_GEM_ERROR", //CLASSF_REASON_MC_GEM_ERROR
};

char *lan_port_str[] = { //-> enum env_bridge_port_type_t
	"CPU",
	"UNI",
	"WAN",
	"IPHOST",
	"DS_BACST",
	"UNUSED"
};

char *colour_scheme[] = { //988 9.2.10 attr 16
	"DISABLE",
	"INTERNAL",
	"DEI",
	"8P0D",
	"7P1D",
	"6P2D",
	"5P3D",
	"DSCP",
};

char *colour_name[] = { //rule->colour definition
	"Green",
	"Yellow",
	"Green",
	"yellow",
};

char *rule_special_case[] = { //rule->special_case definition
	"None",
	"FS",
	"Invalid",
};

static char *
classf_print_str_rule_tci(unsigned short tci, unsigned char inspection_type)
{
	static char str[32];

	switch(inspection_type)
	{
	case CLASSF_FILTER_INSPECTION_TYPE_VID:
		sprintf(str, "(x,x,%u)", (tci & 0xfff));
		break;
	case CLASSF_FILTER_INSPECTION_TYPE_PRIORITY:
		sprintf(str, "(%u,x,x)", ((tci >> 13) & 0x7));
		break;
	case CLASSF_FILTER_INSPECTION_TYPE_TCI:
		sprintf(str, "(%u,%u,%u)", ((tci >> 13) & 0x7), ((tci >> 12) & 0x1), (tci & 0xfff));
		break;
	default:
		sprintf(str, "(%u,%u,%u)", ((tci >> 13) & 0x7), ((tci >> 12) & 0x1), (tci & 0xfff));
	}

	return str;
}

static char *
classf_print_str_rule_gem(struct classf_port_gem_t *port_gem, unsigned char index)
{
	static char str[64];
	unsigned int i, n=0;

	if (port_gem == NULL || index >= CLASSF_GEM_INDEX_NUM)
	{
		str[0]=0;
		return str;
	}

	if (port_gem->gem_tagged[index] == 1)
	{
		n += sprintf(str + n, "T(meid=0x%.4x,id=%u,pbit[", port_gem->gem_ctp_me[index] ? port_gem->gem_ctp_me[index]->meid : 0xffff, port_gem->gem_port[index]);
	} else {
		n += sprintf(str + n, "U(meid=0x%.4x,id=%u,pbit[", port_gem->gem_ctp_me[index] ? port_gem->gem_ctp_me[index]->meid : 0xffff, port_gem->gem_port[index]);
	}
	for (i = 0; i < 8; i++)
	{
		if ((port_gem->pbit_mask[index] >> i ) & 0x1)
		{
			n += sprintf(str + n, "%d", i);
		}
	}
	n += sprintf(str + n, "],%s)", port_gem->dir[index] <= CLASSF_GEM_DIR_BOTH ? gem_dir_str[port_gem->dir[index]] : gem_dir_str[CLASSF_GEM_DIR_DISABLE]);

	return str;
}

static char *
classf_print_str_pvlan_rule(struct classf_pvlan_acl_t *pvlan_acl, int join_entry_id, unsigned char hw_acl_sub_order_valid, int hw_acl_sub_order, char *wrap_string)
{
	static char str[2048];
	int n=0;
	char v6ip_str[64];

	if (pvlan_acl == NULL)
	{
		str[0] = 0;
		return str;
	}

	if (join_entry_id >= 0)
	{
		if (hw_acl_sub_order_valid)
		{
			n += sprintf(str + n, "PVLAN: order=%u, num=%u, join_entry=%d, hw_acl_sub_order=%d%s",
				pvlan_acl->order,
				pvlan_acl->rule_num,
				join_entry_id,
				hw_acl_sub_order,
				pvlan_acl->uniport_wildcard ? ", uniport wildcard" : "");
		} else {
			n += sprintf(str + n, "PVLAN: order=%u, num=%u, join_entry=%d%s",
				pvlan_acl->order,
				pvlan_acl->rule_num,
				join_entry_id,
				pvlan_acl->uniport_wildcard ? ", uniport wildcard" : "");
		}
	} else {
		if (hw_acl_sub_order_valid)
		{
			n += sprintf(str + n, "PVLAN: order=%u, num=%u, hw_acl_sub_order=%d%s",
				pvlan_acl->order,
				pvlan_acl->rule_num,
				hw_acl_sub_order,
				pvlan_acl->uniport_wildcard ? ", uniport wildcard" : "");
		} else {
			n += sprintf(str + n, "PVLAN: order=%u, num=%u%s",
				pvlan_acl->order,
				pvlan_acl->rule_num,
				pvlan_acl->uniport_wildcard ? ", uniport wildcard" : "");
		}
	}

	if (pvlan_acl->care_bit.ethertype)
	{
		n += sprintf(str + n, "%sEthertype: 0x%.4x", wrap_string, pvlan_acl->ethertype);
	}
	if (pvlan_acl->care_bit.smac)
	{
		n += sprintf(str + n, "%sSource MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", wrap_string, pvlan_acl->smac[0],
			pvlan_acl->smac[1],
			pvlan_acl->smac[2],
			pvlan_acl->smac[3],
			pvlan_acl->smac[4],
			pvlan_acl->smac[5]);
		if (pvlan_acl->care_bit.smac_mask)
		{
			n += sprintf(str + n, "%sSource MAC Mask: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", wrap_string, pvlan_acl->smac_mask[0],
				pvlan_acl->smac_mask[1],
				pvlan_acl->smac_mask[2],
				pvlan_acl->smac_mask[3],
				pvlan_acl->smac_mask[4],
				pvlan_acl->smac_mask[5]);
		}
	}
	if (pvlan_acl->care_bit.dmac)
	{
		n += sprintf(str + n, "%sDestination MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", wrap_string, pvlan_acl->dmac[0],
			pvlan_acl->dmac[1],
			pvlan_acl->dmac[2],
			pvlan_acl->dmac[3],
			pvlan_acl->dmac[4],
			pvlan_acl->dmac[5]);
		if (pvlan_acl->care_bit.dmac_mask)
		{
			n += sprintf(str + n, "%sDestination MAC Mask: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", wrap_string, pvlan_acl->dmac_mask[0],
				pvlan_acl->dmac_mask[1],
				pvlan_acl->dmac_mask[2],
				pvlan_acl->dmac_mask[3],
				pvlan_acl->dmac_mask[4],
				pvlan_acl->dmac_mask[5]);
		}
	}

	if (pvlan_acl->care_bit.sip)
	{
		n += sprintf(str + n, "%sSource IP: %s", wrap_string, inet_ntoa(pvlan_acl->sip));
		if (pvlan_acl->care_bit.sip_mask)
		{
			n += sprintf(str + n, "%sSource IP Mask: %s", wrap_string, inet_ntoa(pvlan_acl->sip_mask));
		}
	}
	if (pvlan_acl->care_bit.dip)
	{
		n += sprintf(str + n, "%sDestination IP: %s", wrap_string, inet_ntoa(pvlan_acl->dip));
		if (pvlan_acl->care_bit.dip_mask)
		{
			n += sprintf(str + n, "%sDestination IP Mask: %s", wrap_string, inet_ntoa(pvlan_acl->dip_mask));
		}
	}

	if (pvlan_acl->care_bit.v6sip)
	{
		memset(v6ip_str, 0x00, sizeof(v6ip_str));
		util_inet_ntop(AF_INET6, pvlan_acl->v6sip.s6_addr, v6ip_str, 64);
		n += sprintf(str + n, "%sv6 Source IP: %s", wrap_string, v6ip_str);
		if (pvlan_acl->care_bit.v6sip_prefix)
		{
			n += sprintf(str + n, "%sv6 Source IP Prefix: %d", wrap_string, pvlan_acl->v6sip_prefix);
		}
	}
	if (pvlan_acl->care_bit.v6dip)
	{
		memset(v6ip_str, 0x00, sizeof(v6ip_str));
		util_inet_ntop(AF_INET6, pvlan_acl->v6dip.s6_addr, v6ip_str, 64);
		n += sprintf(str + n, "%sv6 Destination IP: %s", wrap_string, v6ip_str);
		if (pvlan_acl->care_bit.v6dip_prefix)
		{
			n += sprintf(str + n, "%sv6 Destination IP Prefix: %d", wrap_string, pvlan_acl->v6dip_prefix);
		}
	}

	if (pvlan_acl->care_bit.protocol)
	{
		char protocol[8];
		switch(pvlan_acl->protocol) {
			case 1:
				strcpy(protocol, "ICMP");
				break;
			case 2:
				strcpy(protocol, "IGMP");
				break;
			case 6:
				strcpy(protocol, "TCP");
				break;
			case 17:
				strcpy(protocol, "UDP");
				break;
			default:
				strcpy(protocol, "OTHERS");
				break;
		}
		n += sprintf(str + n, "%sIP Protocol: %s(%u)", wrap_string, protocol, pvlan_acl->protocol);
	}

	if (pvlan_acl->care_bit.v6nd_target_ip)
	{
		memset(v6ip_str, 0x00, sizeof(v6ip_str));
		util_inet_ntop(AF_INET6, pvlan_acl->v6nd_target_ip.s6_addr, v6ip_str, 64);
		n += sprintf(str + n, "%sv6 ND Target IP: %s", wrap_string, v6ip_str);
	}

	if (pvlan_acl->care_bit.sport_ubound)
	{
		n += sprintf(str + n, "%sSource Port Range: %u-%u", wrap_string, pvlan_acl->care_bit.sport ? pvlan_acl->sport : 0, pvlan_acl->sport_ubound);
	} else {
		if (pvlan_acl->care_bit.sport)
		{
			n += sprintf(str + n, "%sSource Port: %u", wrap_string, pvlan_acl->sport);
		}
	}
	if (pvlan_acl->care_bit.dport_ubound)
	{
		n += sprintf(str + n, "%sDestination Port Range: %u-%u", wrap_string, pvlan_acl->care_bit.dport ? pvlan_acl->dport : 0, pvlan_acl->dport_ubound);
	} else {
		if (pvlan_acl->care_bit.dport)
		{
			n += sprintf(str + n, "%sDestination Port: %u", wrap_string, pvlan_acl->dport);
		}
	}

	return str;
}

static char *
classf_print_str_rule_candi_us(struct classf_rule_candi_t *rule_candi, char *tab_string)
{
	static char str[2048];
	int n=0;
	char wrap_string[32];
	char wrap_string1[32];

	if (rule_candi == NULL)
	{
		str[0] = 0;
		return str;
	}

	sprintf(wrap_string, "\n%s\t\t", tab_string);
	sprintf(wrap_string1, "\n%s\t\t\t", tab_string);
	n += sprintf(str + n, "%sUS: %u, reason=%s, lan_port_meid=0x%.4x,\n", tab_string, rule_candi->entry_id, reason_str[rule_candi->reason], rule_candi->lan_port->me_bridge_port->meid);
	if (rule_candi->pvlan_acl == NULL)
	{
		n += sprintf(str + n, "%s\t[%u]: %s\n", tab_string, rule_candi->rule_fields->entry_id, omciutil_vlan_str_desc(rule_candi->rule_fields, wrap_string, wrap_string, 0));
		switch(rule_candi->dscp2pbit_enable)
		{
		case 1: //from inner treatment
			n += sprintf(str + n, "%s\t\tDSCP2PBIT from INNER treatment\n", tab_string);
			break;
		case 2: //from outer treatment
			n += sprintf(str + n, "%s\t\tDSCP2PBIT from OUTER treatment\n", tab_string);
			break;
		default:
			; //do nothing	
		}
	}else {
		n += sprintf(str + n, "%s\t[%u]: %s\n", tab_string, rule_candi->rule_fields->entry_id, classf_print_str_pvlan_rule(rule_candi->pvlan_acl, -1, 0, 0, wrap_string));
		n += sprintf(str + n, "%s\t\t%s\n", tab_string, omciutil_vlan_str_desc(rule_candi->rule_fields, wrap_string1, wrap_string1, 0));
		switch(rule_candi->dscp2pbit_enable)
		{
		case 1: //from inner treatment
			n += sprintf(str + n, "%s\t\t\tDSCP2PBIT from INNER treatment\n", tab_string);
			break;
		case 2: //from outer treatment
			n += sprintf(str + n, "%s\t\t\tDSCP2PBIT from OUTER treatment\n", tab_string);
			break;
		default:
			; //do nothing	
		}
	}
	
	if (rule_candi->lan_filter_valid == 0) // no filter
	{
		n += sprintf(str + n, "%s\tlan_filt=NULL, ", tab_string);
	} else {
		if (rule_candi->lan_filter_type >= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0 && 
			rule_candi->lan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11)
		{
			n += sprintf(str + n, "%s\tlan_filt=%s%s, ", tab_string, filter_op_type_ext_str[rule_candi->lan_filter_type],
				classf_print_str_rule_tci(rule_candi->lan_port->port_filter.vlan_tci[rule_candi->lan_filter_type - CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0],
					rule_candi->lan_port->port_filter.vlan_inspection_type));
		} else {
			n += sprintf(str + n, "%s\tlan_filt=%s, ", tab_string, rule_candi->lan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE ? filter_op_type_ext_str[rule_candi->lan_filter_type] : "NON");
		}
	}
	
	n += sprintf(str + n, "wan_port_meid=0x%.4x,\n", rule_candi->wan_port->me_bridge_port ? rule_candi->wan_port->me_bridge_port->meid : 0xffff);
	if (rule_candi->wan_filter_valid == 0) // no filter
	{
		n += sprintf(str + n, "%s\twan_filt=NULL, ", tab_string);
	} else {
		if (rule_candi->wan_filter_type >= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0 && 
			rule_candi->wan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11)
		{
			n += sprintf(str + n, "%s\twan_filt=%s%s, ", tab_string, filter_op_type_ext_str[rule_candi->wan_filter_type],
				classf_print_str_rule_tci(rule_candi->wan_port->port_filter.vlan_tci[rule_candi->wan_filter_type - CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0],
					rule_candi->wan_port->port_filter.vlan_inspection_type));
		} else {
			n += sprintf(str + n, "%s\twan_filt=%s, ", tab_string, rule_candi->wan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE ? filter_op_type_ext_str[rule_candi->wan_filter_type] : "NON");
		}
	}
	n += sprintf(str + n, "gem_index=%u-%s", rule_candi->gem_index,
		classf_print_str_rule_gem(&rule_candi->wan_port->port_gem, rule_candi->gem_index));
	
	return str;
}

static char *
classf_print_str_rule_candi_ds(struct classf_rule_candi_t *rule_candi, char *tab_string)
{
	static char str[2048];
	int n=0;
	char wrap_string[32];
	char wrap_string1[32];

	if (rule_candi == NULL)
	{
		str[0] = 0;
		return str;
	}

	sprintf(wrap_string, "\n%s\t\t", tab_string);
	sprintf(wrap_string1, "\n%s\t\t\t", tab_string);
	n += sprintf(str + n, "%sDS: %u, reason=%s, wan_port_meid=0x%.4x,\n", tab_string, rule_candi->entry_id, reason_str[rule_candi->reason], rule_candi->wan_port->me_bridge_port ? rule_candi->wan_port->me_bridge_port->meid : 0xffff);
	n += sprintf(str + n, "%s\tgem_index=%u-%s, ", tab_string, rule_candi->gem_index,
		classf_print_str_rule_gem(&rule_candi->wan_port->port_gem, rule_candi->gem_index));
	if (rule_candi->wan_filter_valid == 0) // no filter
	{
		n += sprintf(str + n, "wan_filt=NULL,\n");
	} else {
		if (rule_candi->wan_filter_type >= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0 && 
			rule_candi->wan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11)
		{
			n += sprintf(str + n, "wan_filt=%s%s,\n", filter_op_type_ext_str[rule_candi->wan_filter_type],
				classf_print_str_rule_tci(rule_candi->wan_port->port_filter.vlan_tci[rule_candi->wan_filter_type - CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0],
					rule_candi->wan_port->port_filter.vlan_inspection_type));
		} else {
			n += sprintf(str + n, "wan_filt=%s,\n", rule_candi->wan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE ? filter_op_type_ext_str[rule_candi->wan_filter_type] : "NON");
		}
	}
	
	n += sprintf(str + n, "%s\tlan_port_meid=0x%.4x, ", tab_string, rule_candi->lan_port->me_bridge_port->meid);
	if (rule_candi->lan_filter_valid == 0) // no filter
	{
		n += sprintf(str + n, "lan_filt=NULL,\n");
	} else {
		if (rule_candi->lan_filter_type >= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0 && 
			rule_candi->lan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11)
		{
			n += sprintf(str + n, "lan_filt=%s%s,\n", filter_op_type_ext_str[rule_candi->lan_filter_type],
				classf_print_str_rule_tci(rule_candi->lan_port->port_filter.vlan_tci[rule_candi->lan_filter_type-CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0],
					rule_candi->lan_port->port_filter.vlan_inspection_type));
		} else {
			n += sprintf(str + n, "lan_filt=%s,\n", rule_candi->lan_filter_type <= CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE ? filter_op_type_ext_str[rule_candi->lan_filter_type] : "NON");
		}
	}
	if (rule_candi->pvlan_acl == NULL)
	{
		n += sprintf(str + n, "%s\t[%u]: %s", tab_string, rule_candi->rule_fields->entry_id, omciutil_vlan_str_desc(rule_candi->rule_fields, wrap_string, wrap_string, 0));
	} else {
		n += sprintf(str + n, "%s\t[%u]: %s\n", tab_string, rule_candi->rule_fields->entry_id, classf_print_str_pvlan_rule(rule_candi->pvlan_acl, -1, 0, 0, wrap_string));
		n += sprintf(str + n, "%s\t\t%s", tab_string, omciutil_vlan_str_desc(rule_candi->rule_fields, wrap_string1, wrap_string1, 0));
	}

	return str;
}

static char *
classf_print_str_rule_us(struct classf_rule_t *rule, char *tab_string)
{
	static char str[2048], wrap_line[16], wrap_line1[16];
	int n=0, i;
	struct classf_entry_node_t *entry_node_p;

	if (rule == NULL)
	{
		str[0] = 0;
		return str;
	}

	sprintf(wrap_line, "\n%s\t", tab_string);
	sprintf(wrap_line1, "\n%s\t\t", tab_string);
	n += sprintf(str + n, "lan_port_meid=0x%.4x, wan_port_meid=0x%.4x,\n", rule->lan_port->me_bridge_port->meid, rule->wan_port->me_bridge_port ? rule->wan_port->me_bridge_port->meid : 0xffff);
	n += sprintf(str + n, "%sgem_index=%u-%s, rule_candi_id=%u", tab_string, rule->gem_index,
		classf_print_str_rule_gem(&rule->wan_port->port_gem, rule->gem_index),
		rule->rule_candi_entry_id);
	{
		if (!list_empty(&rule->classify_entry_list))
		{
			n += sprintf(str + n, ", hw_entry_id=(");
			i = 0;
			list_for_each_entry(entry_node_p, &rule->classify_entry_list, e_node)
			{
				if (i == 0)
				{
					n += sprintf(str + n, "%d", entry_node_p->id);
				} else {
					n += sprintf(str + n, ", %d", entry_node_p->id);
				}
				i++;
			}
			n += sprintf(str + n, ")");
		}
		n += sprintf(str + n, "\n");
	}

	if (rule->pvlan_acl == NULL)
	{
		n += sprintf(str + n, "%s[%u]: %s", tab_string, rule->rule_fields.entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line, wrap_line, 1));
		switch(rule->dscp2pbit_enable)
		{
		case 1: //from inner treatment
			n += sprintf(str + n, "\n%s\tDSCP2PBIT from INNER treatment", tab_string);
			break;
		case 2: //from outer treatment
			n += sprintf(str + n, "\n%s\tDSCP2PBIT from OUTER treatment", tab_string);
			break;
		default:
			; //do nothing	
		}
	} else {
		n += sprintf(str + n, "%s[%u]: %s\n", tab_string, rule->rule_fields.entry_id, classf_print_str_pvlan_rule(rule->pvlan_acl, -1, 1, rule->hw_acl_sub_order, wrap_line));
		n += sprintf(str + n, "%s\t%s", tab_string, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
		switch(rule->dscp2pbit_enable)
		{
		case 1: //from inner treatment
			n += sprintf(str + n, "\n%s\t\tDSCP2PBIT from INNER treatment", tab_string);
			break;
		case 2: //from outer treatment
			n += sprintf(str + n, "\n%s\t\tDSCP2PBIT from OUTER treatment", tab_string);
			break;
		default:
			; //do nothing	
		}
	}
	if (rule->hw_stream_id < 0)
	{
		n += sprintf(str + n, "\n%s--> Flow: Non", tab_string);
	} else {
		n += sprintf(str + n, "\n%s--> Flow: %d", tab_string, rule->hw_stream_id);
		if (rule->wan_port != NULL &&
			rule->wan_port->port_gem.colour_marking_us[rule->gem_index] >= 2 &&
			rule->wan_port->port_gem.colour_marking_us[rule->gem_index] <= 6)
		{
			n += sprintf(str + n, " (%s: %s)", colour_scheme[rule->wan_port->port_gem.colour_marking_us[rule->gem_index]], colour_name[rule->colour]);
		}
			
	}

	if (rule->tlan_info.flag == 1)
	{
		n += sprintf(str + n, ", af=%d", rule->tlan_info.aggregation_flag);
	}

	return str;
}

static char *
classf_print_str_rule_ds(struct classf_rule_t *rule, char *tab_string)
{
	static char str[2048], wrap_line[16], wrap_line1[16];
	int n=0, i;
	struct classf_entry_node_t *entry_node_p;

	if (rule == NULL)
	{
		str[0] = 0;
		return str;
	}

	sprintf(wrap_line, "\n%s\t", tab_string);
	sprintf(wrap_line1, "\n%s\t\t", tab_string);
	n += sprintf(str + n, "wan_port_meid=0x%.4x, gem_index=%u-%s,\n", rule->wan_port->me_bridge_port ? rule->wan_port->me_bridge_port->meid : 0xffff,
		rule->gem_index,
		classf_print_str_rule_gem(&rule->wan_port->port_gem, rule->gem_index));
	n += sprintf(str + n, "%slan_port_meid=0x%.4x, rule_candi_id=%u", tab_string, rule->lan_port->me_bridge_port->meid, rule->rule_candi_entry_id);

	{
		if (!list_empty(&rule->classify_entry_list))
		{
			n += sprintf(str + n, ", hw_entry_id=(");
			i = 0;
			list_for_each_entry(entry_node_p, &rule->classify_entry_list, e_node)
			{
				if (i == 0)
				{
					n += sprintf(str + n, "%d", entry_node_p->id);
				} else {
					n += sprintf(str + n, ", %d", entry_node_p->id);
				}
				i++;
			}
			n += sprintf(str + n, ")");
		}
	}

	if (rule->special_case != CLASSF_RULE_SPECIAL_CASE_NONE)
	{
		n += sprintf(str + n, ", sc=%s\n", rule_special_case[rule->special_case]);
	} else {
		n += sprintf(str + n, "\n");
	}

	if (rule->tlan_info.flag == 1)
	{
		n += sprintf(str + n, "%s--> Flow: %d, Mask: %.2X, af=%d\n", tab_string, rule->hw_stream_id, rule->hw_stream_id_mask, rule->tlan_info.aggregation_flag);
	} else {
		n += sprintf(str + n, "%s--> Flow: %d, Mask: %.2X\n", tab_string, rule->hw_stream_id, rule->hw_stream_id_mask);
	}
	if (rule->pvlan_acl == NULL)
	{
		n += sprintf(str + n, "%s[%u]: %s", tab_string, rule->rule_fields.entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line, wrap_line, 0));
	} else {
		n += sprintf(str + n, "%s[%u]: %s\n", tab_string, rule->rule_fields.entry_id, classf_print_str_pvlan_rule(rule->pvlan_acl, -1, 1, rule->hw_acl_sub_order, wrap_line));
		n += sprintf(str + n, "%s\t%s", tab_string, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
	}

	return str;
}

static char *
classf_print_str_bridge_lan_port(struct classf_bridge_info_lan_port_t *lan_port)
{
	static char str[2048];
	int n=0, i;

	if (lan_port == NULL)
	{
		str[0] = 0;
		return str;
	}
#if 0 // Remove VEIP related workaround
	if (lan_port->port_type == ENV_BRIDGE_PORT_TYPE_CPU &&
		omci_env_g.classf_veip_orig_tci_map[lan_port->port_type_index]!= 0)
	{
		n += sprintf(str + n, "bp_meid=0x%.4x, ptype=%s, ptype_index=%u, pid=%u, pon_port_tci=(%u, %u, %u)\n",
			lan_port->me_bridge_port->meid,
			lan_port->port_type <= ENV_BRIDGE_PORT_TYPE_UNUSED ? lan_port_str[lan_port->port_type] : lan_port_str[ENV_BRIDGE_PORT_TYPE_UNUSED], 
			lan_port->port_type_index,
			lan_port->port_id,
			(omci_env_g.classf_veip_orig_tci_map[lan_port->port_type_index] >> 13) & 0x7,
			(omci_env_g.classf_veip_orig_tci_map[lan_port->port_type_index] >> 12) & 0x1,
			omci_env_g.classf_veip_orig_tci_map[lan_port->port_type_index] & 0xfff);
	} else 
#endif
	{
		n += sprintf(str + n, "bp_meid=0x%.4x, ptype=%s, ptype_index=%u, pid=%u,\n",
			lan_port->me_bridge_port->meid,
			lan_port->port_type < ENV_BRIDGE_PORT_TYPE_UNUSED ? lan_port_str[lan_port->port_type] : \
			(omci_env_g.port2port_enable && lan_port->port_type == CLASSF_BRIDGE_PORT_TYPE_UNI_DC) ? "UNI_DC" : lan_port_str[ENV_BRIDGE_PORT_TYPE_UNUSED], 
			lan_port->port_type_index,
			lan_port->logical_port_id);
	}
	if (lan_port->port_filter.filter_me != NULL)
	{
		n += sprintf(str + n, "\tfilt: meid=0x%.4x, UntagRx=%s, UntagTx=%s, TagRx=%s, TagTx=%s,\n", lan_port->port_filter.filter_me->meid,
			lan_port->port_filter.untag_rx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[lan_port->port_filter.untag_rx] : "NO",
			lan_port->port_filter.untag_tx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[lan_port->port_filter.untag_tx] : "NO",
			lan_port->port_filter.tag_rx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[lan_port->port_filter.tag_rx] : "NO",
			lan_port->port_filter.tag_tx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[lan_port->port_filter.tag_tx] : "NO");
		n += sprintf(str + n, "\t\ttci_num=%u, vtype=%s, tci=", lan_port->port_filter.vlan_tci_num,
			lan_port->port_filter.vlan_inspection_type <= CLASSF_FILTER_INSPECTION_TYPE_TCI ? filter_inspection_type_str[lan_port->port_filter.vlan_inspection_type] : "NON");
		for (i = 0; i < lan_port->port_filter.vlan_tci_num; i++)
		{
			if (i == 0)
			{
				n += sprintf(str + n, "%u%s", i, classf_print_str_rule_tci(lan_port->port_filter.vlan_tci[i], CLASSF_FILTER_INSPECTION_TYPE_TCI));
			} else {
				n += sprintf(str + n, ", %u%s", i, classf_print_str_rule_tci(lan_port->port_filter.vlan_tci[i], CLASSF_FILTER_INSPECTION_TYPE_TCI));
			}
		}
	} else {
		n += sprintf(str + n, "\tfilt: NO FILTER!!");
	}

	if (omci_env_g.classf_dscp2pbit_mode == 0 &&
		lan_port->port_tagging.dscp2pbit_info.enable == 1)
	{
		n += sprintf(str + n, "\tdscp-2-pbit mapping table, class_id=%u, me_id=0x%.4x\n",
			lan_port->port_tagging.dscp2pbit_info.from_me != NULL ? lan_port->port_tagging.dscp2pbit_info.from_me->classid : 0,
			lan_port->port_tagging.dscp2pbit_info.from_me != NULL ? lan_port->port_tagging.dscp2pbit_info.from_me->meid : 0);
		n += sprintf(str + n, "\tdscp :      pbit      \n");
		n += sprintf(str + n, "\t----------------------\n");
		for (i = 0; i < 64; i = i + 8)
		{
			n += sprintf(str + n, "\t%2u-%2u: %1u %1u %1u %1u %1u %1u %1u %1u\n", i , i +7,
				lan_port->port_tagging.dscp2pbit_info.table[i],
				lan_port->port_tagging.dscp2pbit_info.table[i + 1],
				lan_port->port_tagging.dscp2pbit_info.table[i + 2],
				lan_port->port_tagging.dscp2pbit_info.table[i + 3],
				lan_port->port_tagging.dscp2pbit_info.table[i + 4],
				lan_port->port_tagging.dscp2pbit_info.table[i + 5],
				lan_port->port_tagging.dscp2pbit_info.table[i + 6],
				lan_port->port_tagging.dscp2pbit_info.table[i + 7]);
		}
	}
	
	return str;
}

static char *
classf_print_str_bridge_wan_port(struct classf_bridge_info_wan_port_t *wan_port)
{
	static char str[2048];
	int n=0, i;

	if (wan_port == NULL)
	{
		str[0] = 0;
		return str;
	}

	n += sprintf(str + n, "bp_meid=0x%.4x, ptype_index=%u, pid=%u,\n", wan_port->me_bridge_port->meid, wan_port->port_type_index, wan_port->logical_port_id);
	if (wan_port->port_filter.filter_me != NULL)
	{
		n += sprintf(str + n, "\tfilt: meid=0x%.4x, UntagRx=%s, UntagTx=%s, TagRx=%s, TagTx=%s,\n", wan_port->port_filter.filter_me->meid,
			wan_port->port_filter.untag_rx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[wan_port->port_filter.untag_rx] : "NO",
			wan_port->port_filter.untag_tx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[wan_port->port_filter.untag_tx] : "NO",
			wan_port->port_filter.tag_rx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[wan_port->port_filter.tag_rx] : "NO",
			wan_port->port_filter.tag_tx <= CLASSF_FILTER_OP_TYPE_NEGATIVE ? filter_op_type_str[wan_port->port_filter.tag_tx] : "NO");
		n += sprintf(str + n, "\t\ttci_num=%u, vtype=%s, tci=", wan_port->port_filter.vlan_tci_num,
			wan_port->port_filter.vlan_inspection_type <= CLASSF_FILTER_INSPECTION_TYPE_TCI ? filter_inspection_type_str[wan_port->port_filter.vlan_inspection_type] : "NON");
		for (i = 0; i < wan_port->port_filter.vlan_tci_num; i++)
		{
			if (i == 0)
			{
				n += sprintf(str + n, "%u%s", i, classf_print_str_rule_tci(wan_port->port_filter.vlan_tci[i], CLASSF_FILTER_INSPECTION_TYPE_TCI));
			} else {
				n += sprintf(str + n, ", %u%s", i, classf_print_str_rule_tci(wan_port->port_filter.vlan_tci[i], CLASSF_FILTER_INSPECTION_TYPE_TCI));
			}
		}
	} else {
		n += sprintf(str + n, "\tfilt: NO FILTER!!");
	}
	n += sprintf(str + n, "\n\tgem: num=%u, index_total=%u, untag_op=%s\n", wan_port->port_gem.gem_num, wan_port->port_gem.gem_index_total,
		wan_port->port_gem.untag_op < CLASSF_GEM_UNTAG_OP_INVALID ? gem_untag_op_str[wan_port->port_gem.untag_op] : "Invalid");
	for (i = 0; i < wan_port->port_gem.gem_index_total; i++)
	{
		n += sprintf(str + n, "\t\t%u: %s\n", i, classf_print_str_rule_gem(&wan_port->port_gem, i));
	}

	//show dscp to pbit table
	if (omci_env_g.classf_dscp2pbit_mode == 0 &&
		wan_port->port_gem.dscp2pbit_info.enable == 1)
	{
		n += sprintf(str + n, "\tdscp-2-pbit mapping table, class_id=%u, me_id=0x%.4x\n",
			wan_port->port_gem.dscp2pbit_info.from_me != NULL ? wan_port->port_gem.dscp2pbit_info.from_me->classid : 0,
			wan_port->port_gem.dscp2pbit_info.from_me != NULL ? wan_port->port_gem.dscp2pbit_info.from_me->meid : 0);
		n += sprintf(str + n, "\tdscp :      pbit      \n");
		n += sprintf(str + n, "\t----------------------\n");
		for (i = 0; i < 64; i = i + 8)
		{
			n += sprintf(str + n, "\t%2u-%2u: %1u %1u %1u %1u %1u %1u %1u %1u\n", i , i +7,
				wan_port->port_gem.dscp2pbit_info.table[i],
				wan_port->port_gem.dscp2pbit_info.table[i + 1],
				wan_port->port_gem.dscp2pbit_info.table[i + 2],
				wan_port->port_gem.dscp2pbit_info.table[i + 3],
				wan_port->port_gem.dscp2pbit_info.table[i + 4],
				wan_port->port_gem.dscp2pbit_info.table[i + 5],
				wan_port->port_gem.dscp2pbit_info.table[i + 6],
				wan_port->port_gem.dscp2pbit_info.table[i + 7]);
		}
	}

	return str;
}

void
classf_print_show_port_pair(int fd, unsigned char flag, struct classf_info_t *classf_info, unsigned short meid_lan, unsigned short meid_wan)
{
	struct classf_bridge_info_t *bridge_info;
	struct classf_bridge_info_lan_port_t *lan_port;
	struct classf_bridge_info_wan_port_t *wan_port;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct classf_pvlan_rule_t *pvlan_rule;
	struct classf_rule_candi_t *rule_candi;
	struct classf_rule_t *rule;
	char *wrap_line = "\n\t\t\t";
	char *wrap_line1 = "\n\t\t\t\t";

	unsigned int found_lan, found_wan;

	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	dbprintf(LOG_ERR, "meid_lan=0x%.4x, meid_wan=0x%.4x\n", meid_lan, meid_wan);

	fwk_mutex_lock(&classf_info->mutex);

	list_for_each_entry(bridge_info, &classf_info->bridge_info_list, b_node)
	{
		found_lan = found_wan = 0;
		//check port exist in this bridge
		list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
		{
			if (lan_port->me_bridge_port->meid == meid_lan)
			{
				found_lan = 1;
				break;
			}
		}

		if (found_lan)
		{
			list_for_each_entry(wan_port, &bridge_info->wan_port_list, wp_node)
			{
				if (wan_port->me_bridge_port->meid == meid_wan)
				{
					found_wan = 1;
					break;
				}
			}
		} else {
			continue;
		}

		if (found_lan == 1 && found_wan == 1)
		{
			//show bridge info
			util_fdprintf(fd, "\n=====Bridge: meid=0x%x, Specific Port Pair Classification=====\n", bridge_info->me_bridge->meid);

			util_fdprintf(fd, "LAN Port:\n");
			util_fdprintf(fd, "%s\n", classf_print_str_bridge_lan_port(lan_port));
			//show tagging rule
			util_fdprintf(fd, "\tUpstream Tagging Rules:\n");
			list_for_each_entry(rule_fields, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", rule_fields->entry_id, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
			util_fdprintf(fd, "\tDownstream Tagging Rules:\n");
			list_for_each_entry(rule_fields, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", rule_fields->entry_id, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
			//show protocol vlan rule
			util_fdprintf(fd, "\tUpstream Protocol Vlan Rules:\n");
			list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.us_pvlan_rule_list, pr_node)
			{
				//show pvlan rule
				util_fdprintf(fd, "\t\t[%u]: %s\n", pvlan_rule->rule_fields.entry_id, classf_print_str_pvlan_rule(&pvlan_rule->pvlan_acl, pvlan_rule->join_entry_id, 0, 0, wrap_line));
				util_fdprintf(fd, "\t\t\t%s\n", omciutil_vlan_str_desc(&pvlan_rule->rule_fields, wrap_line1, wrap_line1, 0));
			}
			util_fdprintf(fd, "\tDownstream Protocol Vlan Rules:\n");
			list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
			{
				//show pvlan rule
				util_fdprintf(fd, "\t\t[%u]: %s\n", pvlan_rule->rule_fields.entry_id, classf_print_str_pvlan_rule(&pvlan_rule->pvlan_acl, pvlan_rule->join_entry_id, 0, 0, wrap_line));
				util_fdprintf(fd, "\t\t\t%s\n", omciutil_vlan_str_desc(&pvlan_rule->rule_fields, wrap_line1, wrap_line1, 0));
			}

			util_fdprintf(fd, "WAN Port:\n");
			util_fdprintf(fd, "%s\n", classf_print_str_bridge_wan_port(wan_port));

			if (flag == 1)
			{
				//show rule candi us
				util_fdprintf(fd, "\nClassification Upstream Rule Candidates:\n");
				list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_us, rc_node)
				{
					//check lan-wan port pair
					if (rule_candi->lan_port->me_bridge_port->meid == meid_lan &&
						(rule_candi->wan_port->me_bridge_port == NULL ||
						rule_candi->wan_port->me_bridge_port->meid == meid_wan))
					{
						util_fdprintf(fd, "%s\n", classf_print_str_rule_candi_us(rule_candi, "\t"));

						//show rule result drive from this candidate
						if ((rule_candi->reason == CLASSF_REASON_OK) && (rule_candi->pvlan_acl == NULL))
						{
							list_for_each_entry(rule, &bridge_info->rule_list_us, r_node)
							{
								if (rule->rule_candi_entry_id == rule_candi->entry_id)
								{
									util_fdprintf(fd, "\t\t\t==>%u, %s\n", rule->entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
								}
							}
						}
					}
				}

				//show rule candi ds
				util_fdprintf(fd, "\nClassification Downstream Rule Candidates:\n");
				list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_ds, rc_node)
				{
					//check lan-wan port pair
					if (rule_candi->lan_port->me_bridge_port->meid == meid_lan &&
						(rule_candi->wan_port->me_bridge_port == NULL ||
						rule_candi->wan_port->me_bridge_port->meid == meid_wan))
					{
						util_fdprintf(fd, "%s\n", classf_print_str_rule_candi_ds(rule_candi, "\t"));

						//show rule result drive from this candidate
						if ((rule_candi->reason == CLASSF_REASON_OK) && (rule_candi->pvlan_acl == NULL))
						{
							list_for_each_entry(rule, &bridge_info->rule_list_ds, r_node)
							{
								if (rule->rule_candi_entry_id == rule_candi->entry_id)
								{
									util_fdprintf(fd, "\t\t\t==>%u, %s\n", rule->entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
								}
							}
						}
					}
				}
			}

			//show rule us
			util_fdprintf(fd, "\nClassification Upstream Rules:\n");
			list_for_each_entry(rule, &bridge_info->rule_list_us, r_node)
			{
				//check lan-wan port pair
				if (rule->lan_port->me_bridge_port->meid == meid_lan &&
					(rule->wan_port->me_bridge_port == NULL ||
					rule->wan_port->me_bridge_port->meid == meid_wan))
				{
					util_fdprintf(fd, "\tUS: %u, %s\n", rule->entry_id, classf_print_str_rule_us(rule, "\t\t"));
				}
			}

			//show rule ds
			util_fdprintf(fd, "\nClassification Downstream Rules:\n");
			list_for_each_entry(rule, &bridge_info->rule_list_ds, r_node)
			{
				//check lan-wan port pair
				if (rule->lan_port->me_bridge_port->meid == meid_lan &&
					(rule->wan_port->me_bridge_port == NULL ||
					rule->wan_port->me_bridge_port->meid == meid_wan))
				{
					util_fdprintf(fd, "\tDS: %u, %s\n", rule->entry_id, classf_print_str_rule_ds(rule, "\t\t"));
				}
			}

			break;
		}
	}

	fwk_mutex_unlock(&classf_info->mutex);

	return;
}

void
classf_print_show_bridge(int fd, unsigned char flag, struct classf_info_t *classf_info, unsigned short meid)
{
	struct classf_bridge_info_t *bridge_info;
	struct classf_bridge_info_lan_port_t *lan_port;
	struct classf_bridge_info_wan_port_t *wan_port;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct classf_pvlan_rule_t *pvlan_rule;
	struct classf_rule_candi_t *rule_candi;
	struct classf_rule_t *rule;
	unsigned int count;
	char *wrap_line = "\n\t\t\t";
	char *wrap_line1 = "\n\t\t\t\t";
	int i;

	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	fwk_mutex_lock(&classf_info->mutex);

	list_for_each_entry(bridge_info, &classf_info->bridge_info_list, b_node)
	{
		if (meid == 0xffff || bridge_info->me_bridge->meid == meid)
		{
			if (flag == 0 || flag == 1 || flag == 2)
			{
				//show bridge info
				util_fdprintf(fd, "\n=====Bridge: meid=0x%x, Classification=====\n", bridge_info->me_bridge->meid);

				count = 0;
				util_fdprintf(fd, "LAN Port:\n");
				list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
				{
					util_fdprintf(fd, "%u: %s\n", count++, classf_print_str_bridge_lan_port(lan_port));
					//show tagging rule
					util_fdprintf(fd, "\tUpstream Tagging Rules:\n");
					list_for_each_entry(rule_fields, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
					{
						util_fdprintf(fd, "\t\t[%u]: %s\n", rule_fields->entry_id, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
					}
					util_fdprintf(fd, "\tDownstream Tagging Rules:\n");
					list_for_each_entry(rule_fields, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
					{
						util_fdprintf(fd, "\t\t[%u]: %s\n", rule_fields->entry_id, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
					}
					//show protocol vlan rule
					util_fdprintf(fd, "\tUpstream Protocol Vlan Rules:\n");
					list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.us_pvlan_rule_list, pr_node)
					{
						//show pvlan rule
						util_fdprintf(fd, "\t\t[%u]: %s\n", pvlan_rule->rule_fields.entry_id, classf_print_str_pvlan_rule(&pvlan_rule->pvlan_acl, pvlan_rule->join_entry_id, 0, 0, wrap_line));
						util_fdprintf(fd, "\t\t\t%s\n", omciutil_vlan_str_desc(&pvlan_rule->rule_fields, wrap_line1, wrap_line1, 0));
					}
					util_fdprintf(fd, "\tDownstream Protocol Vlan Rules:\n");
					list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
					{
						//show pvlan rule
						util_fdprintf(fd, "\t\t[%u]: %s\n", pvlan_rule->rule_fields.entry_id, classf_print_str_pvlan_rule(&pvlan_rule->pvlan_acl, pvlan_rule->join_entry_id, 0, 0, wrap_line));
						util_fdprintf(fd, "\t\t\t%s\n", omciutil_vlan_str_desc(&pvlan_rule->rule_fields, wrap_line1, wrap_line1, 0));
					}
				}

				count = 0;
				util_fdprintf(fd, "WAN Port:\n");
				list_for_each_entry(wan_port, &bridge_info->wan_port_list, wp_node)
				{
					util_fdprintf(fd, "%u: %s\n", count++, classf_print_str_bridge_wan_port(wan_port));
				}
			}

			if (flag == 1)
			{
				//show rule candi us
				util_fdprintf(fd, "\nClassification Upstream Rule Candidates:\n");
				list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_us, rc_node)
				{
					util_fdprintf(fd, "%s\n", classf_print_str_rule_candi_us(rule_candi, "\t"));

					//show rule result drive from this candidate
					if ((rule_candi->reason == CLASSF_REASON_OK) && (rule_candi->pvlan_acl == NULL))
					{
						list_for_each_entry(rule, &bridge_info->rule_list_us, r_node)
						{
							if (rule->rule_candi_entry_id == rule_candi->entry_id)
							{
								util_fdprintf(fd, "\t\t\t==>%u, %s\n", rule->entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
							}
						}
					}
				}

				//show rule candi ds
				util_fdprintf(fd, "\nClassification Downstream Rule Candidates:\n");
				list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_ds, rc_node)
				{
					util_fdprintf(fd, "%s\n", classf_print_str_rule_candi_ds(rule_candi, "\t"));

					//show rule result drive from this candidate
					if ((rule_candi->reason == CLASSF_REASON_OK) && (rule_candi->pvlan_acl == NULL))
					{
						list_for_each_entry(rule, &bridge_info->rule_list_ds, r_node)
						{
							if (rule->rule_candi_entry_id == rule_candi->entry_id)
							{
								util_fdprintf(fd, "\t\t\t==>%u, %s\n", rule->entry_id, omciutil_vlan_str_desc(&rule->rule_fields, wrap_line1, wrap_line1, 0));
							}
						}
					}
				}
			}

			if (flag == 0 || flag == 1 || flag == 3)
			{
				//show rule us
				util_fdprintf(fd, "\nClassification Upstream Rules:\n");
				list_for_each_entry(rule, &bridge_info->rule_list_us, r_node)
				{
					util_fdprintf(fd, "\tUS: %u, %s\n", rule->entry_id, classf_print_str_rule_us(rule, "\t\t"));
				}

				//show rule ds
				util_fdprintf(fd, "\nClassification Downstream Rules:\n");
				list_for_each_entry(rule, &bridge_info->rule_list_ds, r_node)
				{
					util_fdprintf(fd, "\tDS: %u, %s\n", rule->entry_id, classf_print_str_rule_ds(rule, "\t\t"));
				}
			}
		}
	}

	if (flag == 0 || flag == 1 || flag == 2)
	{
		//show global dscp table
		if (omci_env_g.classf_dscp2pbit_mode != 0 &&
			classf_info->system_info.dscp2pbit_info.enable == 1)
		{
			util_fdprintf(fd, "\n=====Global dscp-2-pbit mapping table, class_id=%u, me_id=0x%.4x=====\n",
				classf_info->system_info.dscp2pbit_info.from_me ? classf_info->system_info.dscp2pbit_info.from_me->classid : 0,
				classf_info->system_info.dscp2pbit_info.from_me ? classf_info->system_info.dscp2pbit_info.from_me->meid : 0);
			util_fdprintf(fd, "\tdscp :      pbit      \n");
			util_fdprintf(fd, "\t----------------------\n");
			for (i = 0; i < 64; i = i + 8)
			{
				util_fdprintf(fd, "\t%2u-%2u: %1u %1u %1u %1u %1u %1u %1u %1u\n", i , i +7,
					classf_info->system_info.dscp2pbit_info.table[i],
					classf_info->system_info.dscp2pbit_info.table[i + 1],
					classf_info->system_info.dscp2pbit_info.table[i + 2],
					classf_info->system_info.dscp2pbit_info.table[i + 3],
					classf_info->system_info.dscp2pbit_info.table[i + 4],
					classf_info->system_info.dscp2pbit_info.table[i + 5],
					classf_info->system_info.dscp2pbit_info.table[i + 6],
					classf_info->system_info.dscp2pbit_info.table[i + 7]);
			}
		}

		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 &&
				omci_env_g.classf_tlan_pattern == 1)//enable
		{
			util_fdprintf(fd, "\n=====Global tlan vid table=====\n");
			for (i = 0; i < classf_info->system_info.tlan_info.tlan_table.total && i < CLASSF_TLAN_TABLE_MAX; i++)
			{
				util_fdprintf(fd, "%2d %4d\n", i, classf_info->system_info.tlan_info.tlan_table.vid[i]);
			}
			util_fdprintf(fd, "\n=====uni tlan list=====\n");
			i = 0;
			list_for_each_entry(rule_fields, &classf_info->system_info.tlan_info.uni_tlan_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", i++, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
			util_fdprintf(fd, "\n=====uni nontlan list=====\n");
			i = 0;
			list_for_each_entry(rule_fields, &classf_info->system_info.tlan_info.uni_nontlan_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", i++, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
			util_fdprintf(fd, "\n=====ani tlan list=====\n");
			i = 0;
			list_for_each_entry(rule_fields, &classf_info->system_info.tlan_info.ani_tlan_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", i++, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
			util_fdprintf(fd, "\n=====ani nontlan list=====\n");
			i = 0;
			list_for_each_entry(rule_fields, &classf_info->system_info.tlan_info.ani_nontlan_list, rule_node)
			{
				util_fdprintf(fd, "\t\t[%u]: %s\n", i++, omciutil_vlan_str_desc(rule_fields, wrap_line, wrap_line, 0));
			}
		}
	}

	fwk_mutex_unlock(&classf_info->mutex);
	
	return;
}


