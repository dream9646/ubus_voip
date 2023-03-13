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
 * Module  : omciutil
 * File    : omciutil_vlan_str.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "me_related.h"

//24
static int
omciutil_vlan_str_desc_filter_tpid_de(char *str, unsigned char tpid_de, char *prefix)
{
	int n=0;

	switch(tpid_de) {
	case 0:	// dont care
		break;
	case 4:
		n+=sprintf(str+n, "%stpid==0x8100", prefix); break;
	case 5:
		n+=sprintf(str+n, "%stpid==input_tpid", prefix); break;
	case 6:
		n+=sprintf(str+n, "%stpid==input_tpid,de==0", prefix); break;
	case 7:
		n+=sprintf(str+n, "%stpid==input_tpid,de==1", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1000:
		n+=sprintf(str+n, "%stpid==output_tpid", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1001:
		n+=sprintf(str+n, "%stpid==output_tpid,de==0", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1010:
		n+=sprintf(str+n, "%stpid==output_tpid,de==1", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1011:
		n+=sprintf(str+n, "%stpid==don't care,de==0", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1100:
		n+=sprintf(str+n, "%stpid==don't care,de==1", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10000:
		n+=sprintf(str+n, "%stpid==0x8100,de==0", prefix); break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10001:
		n+=sprintf(str+n, "%stpid==0x8100,de==1", prefix); break;
	default:
		n+=sprintf(str+n, "%stpid invalid?", prefix); break;
	}		
	return n;
}

//13
static int
omciutil_vlan_str_desc_treat_priority(char *str, unsigned char priority, char *prefix, int rule_tag_num)
{
	int n=0;

	if (priority<=7) {
		n+=sprintf(str+n, "%spri=%u", prefix, priority);
	} else if (priority==8) {
		n+=sprintf(str+n, "%spri=ipri%s", prefix, (rule_tag_num>=1)?"":"?");
	} else if (priority==9) {
		n+=sprintf(str+n, "%spri=opri%s", prefix, (rule_tag_num>=2)?"":"?");
	} else if (priority==10) {
		n+=sprintf(str+n, "%spri=dscp_map", prefix);
	} else {
		n+=sprintf(str+n, "%spri invalid?", prefix);
	}
	return n;
}

//13
static int
omciutil_vlan_str_desc_treat_vid(char *str, unsigned short vid, char *prefix, int rule_tag_num)
{
	int n=0;
	
	if (vid<=4095) {
		n+=sprintf(str+n, "%svid=%u", prefix, vid);
	} else if (vid==4096) {
		n+=sprintf(str+n, "%svid=ivid%s", prefix, (rule_tag_num>=1)?"":"?");
	} else if (vid==4097) {
		n+=sprintf(str+n, "%svid=ovid%s", prefix, (rule_tag_num>=2)?"":"?");
	} else {
		n+=sprintf(str+n, "%svid invalid?", prefix);
	}
	return n;
}

//29
static int
omciutil_vlan_str_desc_treat_tpid_de(char *str, unsigned char tpid_de, char *prefix, int rule_tag_num)
{
	int n=0;

	switch(tpid_de) {
	case 0:
		n+=sprintf(str+n, "%stpid_de=itpid_de%s", prefix, (rule_tag_num>=1)?"":"?");
		break;
	case 1:
		n+=sprintf(str+n, "%stpid_de=otpid_de%s", prefix, (rule_tag_num>=2)?"":"?");
		break;
	case 2:
		n+=sprintf(str+n, "%stpid=output_tpid,de=itag_de%s", prefix, (rule_tag_num>=1)?"":"?");
		break;
	case 3:
		n+=sprintf(str+n, "%stpid=output_tpid,de=otag_de%s", prefix, (rule_tag_num>=2)?"":"?");
		break;
	case 4:
		n+=sprintf(str+n, "%stpid=0x8100", prefix);
		break;
	case 6:
		n+=sprintf(str+n, "%stpid=output_tpid,de=0", prefix);
		break;
	case 7:
		n+=sprintf(str+n, "%stpid=output_tpid,de=1", prefix);
		break;		
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000:
		n+=sprintf(str+n, "%stpid=input_tpid,de=0", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001:
		n+=sprintf(str+n, "%stpid=input_tpid,de=1", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
		n+=sprintf(str+n, "%stpid=0x8100,de=0", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
		n+=sprintf(str+n, "%stpid=0x8100,de=1", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100:
		n+=sprintf(str+n, "%stpid=input_tpid,de=itag_de%s", prefix, (rule_tag_num>=1)?"":"?");
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101:
		n+=sprintf(str+n, "%stpid=input_tpid,de=otag_de%s", prefix, (rule_tag_num>=2)?"":"?");
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110:
		n+=sprintf(str+n, "%stpid=0x88a8,de=0", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111:
		n+=sprintf(str+n, "%stpid=0x88a8,de=1", prefix);
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000:
		n+=sprintf(str+n, "%stpid=copy_inner%s,de=0", prefix, (rule_tag_num>=1)?"":"?");
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001:
		n+=sprintf(str+n, "%stpid=copy_inner%s,de=1", prefix, (rule_tag_num>=1)?"":"?");
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010:
		n+=sprintf(str+n, "%stpid=copy_outer%s,de=0", prefix, (rule_tag_num>=2)?"":"?");
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011:
		n+=sprintf(str+n, "%stpid=copy_outer%s,de=1", prefix, (rule_tag_num>=2)?"":"?");
		break;
	default:
		n+=sprintf(str+n, "%stpid invalid?", prefix);
		break;
	}
	return n;
}

// the max possible output is 263 byte, so use str[512]
char *
omciutil_vlan_str_desc(struct omciutil_vlan_rule_fields_t *rule_fields, char *head_wrap_string, char *treat_wrap_string, unsigned char lan_mask_flag)
{
	static char str[512];
	int rule_tag_num=-1;
	int n=0;

	str[0]=0;
	if (rule_fields==NULL)
		return str;

	if (rule_fields->l2_filter_enable)
	{
		//show l2 filter
		if (lan_mask_flag != 0)
		{
			n+=sprintf(str+n, "L2: idx=%d,mac=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x,mac_m=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x,uni_mask=0x%x", rule_fields->l2_filter.index,
				rule_fields->l2_filter.mac[0],
				rule_fields->l2_filter.mac[1],
				rule_fields->l2_filter.mac[2],
				rule_fields->l2_filter.mac[3],
				rule_fields->l2_filter.mac[4],
				rule_fields->l2_filter.mac[5],
				rule_fields->l2_filter.mac_mask[0],
				rule_fields->l2_filter.mac_mask[1],
				rule_fields->l2_filter.mac_mask[2],
				rule_fields->l2_filter.mac_mask[3],
				rule_fields->l2_filter.mac_mask[4],
				rule_fields->l2_filter.mac_mask[5],
				rule_fields->l2_filter.lan_mask);
		} else {
			n+=sprintf(str+n, "L2: idx=%d,mac=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x,mac_m=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", rule_fields->l2_filter.index,
				rule_fields->l2_filter.mac[0],
				rule_fields->l2_filter.mac[1],
				rule_fields->l2_filter.mac[2],
				rule_fields->l2_filter.mac[3],
				rule_fields->l2_filter.mac[4],
				rule_fields->l2_filter.mac[5],
				rule_fields->l2_filter.mac_mask[0],
				rule_fields->l2_filter.mac_mask[1],
				rule_fields->l2_filter.mac_mask[2],
				rule_fields->l2_filter.mac_mask[3],
				rule_fields->l2_filter.mac_mask[4],
				rule_fields->l2_filter.mac_mask[5]);
		}
		n+=sprintf(str+n, "%s", head_wrap_string);
	}
	
	n+=sprintf(str+n, "MATCH ");
	// match
	if (rule_fields->filter_outer.priority==15) {
		if (rule_fields->filter_inner.priority==15) {
			n+=sprintf(str+n, "untag%s:", (rule_fields->filter_ethertype==0)?"(default)":"");
			rule_tag_num=0;
		} else if (rule_fields->filter_inner.priority==14) {
			n+=sprintf(str+n, "1tag%s:", (rule_fields->filter_ethertype==0)?"(default)":"");
			rule_tag_num=1;
		} else if (rule_fields->filter_inner.priority<=8) {
			n+=sprintf(str+n, "1tag:");
			rule_tag_num=1;
		} else {		
			n+=sprintf(str+n, "invalid?:");
		}
	} else if (rule_fields->filter_outer.priority==14) {
		if (rule_fields->filter_inner.priority==15) {
			n+=sprintf(str+n, "1tag(default)?:");
		} else if (rule_fields->filter_inner.priority==14) {
			n+=sprintf(str+n, "2tag%s:", (rule_fields->filter_ethertype==0)?"(default)":"");
			rule_tag_num=2;
		} else if (rule_fields->filter_inner.priority<=8) {
			n+=sprintf(str+n, "2tag(default)?:");
			rule_tag_num=2;
		} else {		
			n+=sprintf(str+n, "invalid?:");
		}
	} else if (rule_fields->filter_outer.priority<=8) {
		if (rule_fields->filter_inner.priority==15) {
			n+=sprintf(str+n, "1tag?:");
		} else if (rule_fields->filter_inner.priority==14) {
			n+=sprintf(str+n, "2tag(default)?:");
			rule_tag_num=2;
		} else if (rule_fields->filter_inner.priority<=8) {
			n+=sprintf(str+n, "2tag:");
			rule_tag_num=2;
		} else {		
			n+=sprintf(str+n, "invalid?:");
		}
	} else {		
		n+=sprintf(str+n, "invalid?:");
	}	
	// match outer
	if (rule_fields->filter_outer.priority==14) {
		if (rule_fields->filter_outer.vid<=4095)
			n+=sprintf(str+n, ",ovid==%u", rule_fields->filter_outer.vid);
		if (rule_fields->filter_outer.tpid_de && rule_fields->filter_outer.tpid_de!=5)
			n+=omciutil_vlan_str_desc_filter_tpid_de(str+n, rule_fields->filter_outer.tpid_de, ",o");
	} else if (rule_fields->filter_outer.priority!=15) {
		if (rule_fields->filter_outer.priority<=7)
			n+=sprintf(str+n, " opri==%u", rule_fields->filter_outer.priority);
		if (rule_fields->filter_outer.vid<=4095)
			n+=sprintf(str+n, ",ovid==%u", rule_fields->filter_outer.vid);
		n+=omciutil_vlan_str_desc_filter_tpid_de(str+n, rule_fields->filter_outer.tpid_de, ",o");
	}	
	// match inner
	if (rule_fields->filter_inner.priority==14) {
		if (rule_fields->filter_inner.vid<=4095)
			n+=sprintf(str+n, ",ivid==%u", rule_fields->filter_inner.vid);
		if (rule_fields->filter_inner.tpid_de && rule_fields->filter_inner.tpid_de!=5)
			n+=omciutil_vlan_str_desc_filter_tpid_de(str+n, rule_fields->filter_inner.tpid_de, ",i");
	} else if (rule_fields->filter_inner.priority!=15) {
		if (rule_fields->filter_inner.priority<=7)
			n+=sprintf(str+n, " ipri==%u", rule_fields->filter_inner.priority);
		if (rule_fields->filter_inner.vid<=4095)
			n+=sprintf(str+n, ",ivid==%u", rule_fields->filter_inner.vid);
		n+=omciutil_vlan_str_desc_filter_tpid_de(str+n, rule_fields->filter_inner.tpid_de, ",i");
	}
	// match ether type
	if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_IP) {
		n+=sprintf(str+n, ",ip");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_PPPOE) {
		n+=sprintf(str+n, ",pppoe");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_ARP) {
		n+=sprintf(str+n, ",arp");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_IP_ARP) {
		n+=sprintf(str+n, ",ip/arp");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_IPV6 || rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_IPV6_ZTE) {
		n+=sprintf(str+n, ",ipv6");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6) {
		n+=sprintf(str+n, ",ip/arp/ipv6");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6) {
		n+=sprintf(str+n, ",pppoe/ipv6");
	} else if (rule_fields->filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_DEFAULT) {
		n+=sprintf(str+n, ",def");
	} 
	
	n+=sprintf(str+n, "  %sTREAT ", treat_wrap_string);

	// treatment
	if (rule_fields->treatment_tag_to_remove>=3) {
		// drop
		n+=sprintf(str+n, "DROP!");
	} else {
		//treatment remove,add 
		if (rule_fields->treatment_tag_to_remove>0) {
			n+=sprintf(str+n, "-%dtag%s", rule_fields->treatment_tag_to_remove,
					(rule_fields->treatment_tag_to_remove>rule_tag_num || 
					 rule_fields->treatment_tag_to_remove>2)?"?":"");
		}
		if (rule_fields->treatment_outer.priority!=15) {
			if (rule_fields->treatment_inner.priority!=15)
				n+=sprintf(str+n, "%s+2tag:", rule_fields->treatment_tag_to_remove?",":"");
			else
				n+=sprintf(str+n, "%s+1tag:", rule_fields->treatment_tag_to_remove?",":"");
		} else {
			if (rule_fields->treatment_inner.priority!=15)
				n+=sprintf(str+n, "%s+1tag:", rule_fields->treatment_tag_to_remove?",":"");
			else
				n+=sprintf(str+n, ":");
		}
		//treatment outer
		if (rule_fields->treatment_outer.priority!=15 && rule_fields->treatment_tag_to_remove!=3) {
			n+=omciutil_vlan_str_desc_treat_priority(str+n, rule_fields->treatment_outer.priority, " o", rule_tag_num);
			n+=omciutil_vlan_str_desc_treat_vid(str+n, rule_fields->treatment_outer.vid, ",o", rule_tag_num);
			n+=omciutil_vlan_str_desc_treat_tpid_de(str+n, rule_fields->treatment_outer.tpid_de, ",o", rule_tag_num);
		}
		//treatment inner
		if (rule_fields->treatment_inner.priority!=15 && rule_fields->treatment_tag_to_remove!=3) {
			n+=omciutil_vlan_str_desc_treat_priority(str+n, rule_fields->treatment_inner.priority, " i", rule_tag_num);
			n+=omciutil_vlan_str_desc_treat_vid(str+n, rule_fields->treatment_inner.vid, ",i", rule_tag_num);
			n+=omciutil_vlan_str_desc_treat_tpid_de(str+n, rule_fields->treatment_inner.tpid_de, ",i", rule_tag_num);
		}
	}	
#if 0
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		if (rule_tag_num==0 && rule_fields->filter_inner.vid>=4081 && rule_fields->filter_inner.vid<=4088)
			n+=sprintf(str+n, " (ALU)");
	}
#endif
	if (rule_fields->owner_me)
		n+=sprintf(str+n, " ([%d]0x%x)", rule_fields->owner_me->classid, rule_fields->owner_me->meid);

	return str;
}

// the max possible output is 102 byte, so use str[256]
char *
omciutil_vlan_str_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	static char str[256];
	
	str[0]=0;
	if (rule_fields) {
		sprintf(str, "f[%2u, %4u, %1u, %2u, %4u, %1u], %1u, %1u, t[%2u, %4u, %1u, %2u, %4u, %1u], output_tpid=0x%.4x, input_tpid=0x%.4x\n",
			rule_fields->filter_outer.priority, rule_fields->filter_outer.vid, rule_fields->filter_outer.tpid_de,
			rule_fields->filter_inner.priority, rule_fields->filter_inner.vid, rule_fields->filter_inner.tpid_de,
			rule_fields->filter_ethertype,
			rule_fields->treatment_tag_to_remove,
			rule_fields->treatment_outer.priority, rule_fields->treatment_outer.vid, rule_fields->treatment_outer.tpid_de,
			rule_fields->treatment_inner.priority, rule_fields->treatment_inner.vid, rule_fields->treatment_inner.tpid_de,
			rule_fields->output_tpid, rule_fields->input_tpid);
	}
	return str;
}
