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
 * File    : cpuport_vtagging_vfilter.c
 *
 ******************************************************************/

#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "util.h"
#include "list.h"
#include "env.h"
#include "meinfo.h"
#include "omciutil_vlan.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "hwresource.h"
#include "classf.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_vtagging_vfilter.h"
#include "cpuport_pvlan.h"

/// vtagging realted ////////////////////////////////////////////////////////

int
cpuport_vtagging_match(struct cpuport_tci_t *src_tci, struct omciutil_vlan_rule_fields_t *filter_rule )
{
	unsigned char tag_num = cpuport_util_get_tagnum_from_tci(src_tci);;
	unsigned short output_tpid;

	switch ( tag_num ) {
		case 2:
			if ( filter_rule->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15
					|| filter_rule->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if ( filter_rule->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_8
					&& filter_rule->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_14
					&& filter_rule->filter_outer.priority != src_tci->otci.priority)
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if (filter_rule->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_8
					&& filter_rule->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_14
					&& filter_rule->filter_inner.priority != src_tci->itci.priority )
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if ( filter_rule->filter_outer.vid != 4096 && filter_rule->filter_outer.vid != src_tci->otci.vid)
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if ( filter_rule->filter_inner.vid != 4096 && filter_rule->filter_inner.vid != src_tci->itci.vid)
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			switch ( filter_rule->filter_outer.tpid_de ) {
				case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
					if ( src_tci->otci.tpid != 0x8100)
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
					if ( src_tci->otci.tpid != filter_rule->input_tpid )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
					if ( src_tci->otci.tpid != filter_rule->input_tpid || src_tci->otci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
					if ( src_tci->otci.tpid != filter_rule->input_tpid || src_tci->otci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1000:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1001:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid || src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1010:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid || src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1011:
					if (  src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1100:
					if (  src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				default:
					return CPUPORT_VTAGGING_RULE_UNMATCH;
			}

			switch ( filter_rule->filter_inner.tpid_de ) {
				case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
					if ( src_tci->itci.tpid != 0x8100)
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
					if ( src_tci->itci.tpid != filter_rule->input_tpid )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
					if ( src_tci->itci.tpid != filter_rule->input_tpid || src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
					if ( src_tci->itci.tpid != filter_rule->input_tpid || src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				default:
					return CPUPORT_VTAGGING_RULE_UNMATCH;
			}
			break;
		case 1:
			if ( filter_rule->filter_outer.priority != 15 || filter_rule->filter_inner.priority == 15 )
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if ( filter_rule->filter_inner.priority != 8
					&& filter_rule->filter_inner.priority != 14
					&& filter_rule->filter_inner.priority != src_tci->itci.priority )
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			if ( filter_rule->filter_inner.vid != 4096 && filter_rule->filter_inner.vid != src_tci->itci.vid)
				return CPUPORT_VTAGGING_RULE_UNMATCH;

			switch ( filter_rule->filter_inner.tpid_de ) {
				case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
					if ( src_tci->itci.tpid != 0x8100)
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
					if ( src_tci->itci.tpid != filter_rule->input_tpid )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
					if ( src_tci->itci.tpid != filter_rule->input_tpid || src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
					if ( src_tci->itci.tpid != filter_rule->input_tpid || src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1000:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1001:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid || src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1010:
					if (filter_rule->output_tpid == 0)
						output_tpid  = 0x8100;
					else
						output_tpid  = filter_rule->output_tpid;
					if ( src_tci->itci.tpid != output_tpid || src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1011:
					if (  src_tci->itci.de != 0 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				case OMCIUTIL_VLAN_FILTER_TPID_DE_1100:
					if (  src_tci->itci.de != 1 )
						return CPUPORT_VTAGGING_RULE_UNMATCH;
					break;
				default:
					return CPUPORT_VTAGGING_RULE_UNMATCH;
			}
			break;
		case 0:
			if ( filter_rule->filter_inner.priority != 15)
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		default:
			return CPUPORT_VTAGGING_RULE_UNMATCH;
	}
	switch ( filter_rule->filter_ethertype ) {
		case OMCIUTIL_VLAN_FILTER_ET_IGNORE:
			break;
		case OMCIUTIL_VLAN_FILTER_ET_IP:
			if ( src_tci->ethertype != 0x0800 )
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_PPPOE:
			if ( src_tci->ethertype != 0x8863 && src_tci->ethertype != 0x8864 )
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_ARP:
			if ( src_tci->ethertype != 0x0806 )
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_IPV6:
		case OMCIUTIL_VLAN_FILTER_ET_IPV6_ZTE:
			if ( src_tci->ethertype != 0x86DD )
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_IP_ARP:
			if ( src_tci->ethertype != 0x0800 && src_tci->ethertype != 0x0806)
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6:
			if ( src_tci->ethertype != 0x8863 && src_tci->ethertype != 0x8864 && src_tci->ethertype != 0x86DD)
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6:
			if ( src_tci->ethertype != 0x0800 && src_tci->ethertype != 0x0806 && src_tci->ethertype != 0x86DD)
				return CPUPORT_VTAGGING_RULE_UNMATCH;
			break;
		case OMCIUTIL_VLAN_FILTER_ET_DEFAULT: // ethertype other than above
			break;
		default:
			return CPUPORT_VTAGGING_RULE_UNMATCH;
	}
	return CPUPORT_VTAGGING_RULE_MATCH;
}

static int
cpuport_vtagging_match_l2(unsigned char is_upstream, struct cpuport_info_t *cpuport_info, struct omciutil_vlan_rule_l2_filter_t *l2_filter )
{
	int i;
	
	if (is_upstream){ //upstream
		//source mac
		for ( i=0 ; i<6; i++ ) {
			if ((l2_filter->mac_mask[i] & cpuport_info->src_mac[i]) != (l2_filter->mac_mask[i] & l2_filter->mac[i]))
				return CPUPORT_VTAGGING_RULE_UNMATCH;
		}
	} else { //downstream
		//destination mac
		for ( i=0 ; i<6; i++ ) {
			if ((l2_filter->mac_mask[i] & cpuport_info->dst_mac[i]) != (l2_filter->mac_mask[i] & l2_filter->mac[i]))
				return CPUPORT_VTAGGING_RULE_UNMATCH;
		}
	}
	
	return CPUPORT_VTAGGING_RULE_MATCH;
}

static unsigned char
cpuport_vtagging_treatment_dscp2pbit(unsigned char dscp_value)
{
	struct classf_info_t *t = batchtab_table_data_get("classf");
	unsigned char pbit;
	if ( !t) {
		dbprintf_cpuport(LOG_NOTICE, "classf_info not found\n");
		return 0;
	}

	pbit = t->system_info.dscp2pbit_info.table[dscp_value];
	batchtab_table_data_put("classf");
	return pbit;
}

int
cpuport_vtagging_treatment(struct cpuport_tci_t *src_tci, struct omciutil_vlan_rule_fields_t *tag_rule ,struct cpuport_tci_t *dst_tci)
{
	unsigned char tag_num = cpuport_util_get_tagnum_from_tci(src_tci);

	*dst_tci = *src_tci;

	switch (tag_num) {
		case 0:
			switch (tag_rule->treatment_inner.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					dst_tci->itci.priority = tag_rule->treatment_inner.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_10:
					dst_tci->itci.priority = cpuport_vtagging_treatment_dscp2pbit(src_tci->dscp);
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			if ( tag_rule->treatment_inner.vid <= 4095 )
				dst_tci->itci.vid = tag_rule->treatment_inner.vid;
			else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;

			switch (tag_rule->treatment_inner.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					dst_tci->itci.tpid = 0x8100;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					dst_tci->itci.tpid = tag_rule->output_tpid;
					dst_tci->itci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					dst_tci->itci.tpid = tag_rule->output_tpid;
					dst_tci->itci.de = 1;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 1;
					break;
				default:
					break;
			}

			switch (tag_rule->treatment_outer.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					dst_tci->otci.priority = tag_rule->treatment_outer.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					break;
			}

			if ( tag_rule->treatment_outer.vid < 4095 ) {
				dst_tci->otci.vid = tag_rule->treatment_outer.vid;
			}else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;

			switch (tag_rule->treatment_outer.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					dst_tci->otci.tpid = 0x8100;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					dst_tci->otci.tpid = tag_rule->output_tpid;
					dst_tci->otci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					dst_tci->otci.tpid = tag_rule->output_tpid;
					dst_tci->otci.de = 1;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 1;
					break;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}
			break;
		case 1:
			if ( tag_rule->treatment_tag_to_remove >= 1) {
				dst_tci->itci.tpid = 0;
				dst_tci->itci.priority = 0;
				dst_tci->itci.de = 0;
				dst_tci->itci.vid = 0;
			}
			switch (tag_rule->treatment_inner.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->itci.priority = tag_rule->treatment_inner.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->otci.priority = tag_rule->treatment_inner.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
					if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->itci.priority = src_tci->itci.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->otci.priority = src_tci->itci.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_10:
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			if ( tag_rule->treatment_inner.vid < 4095 ) {
				if (tag_rule->treatment_tag_to_remove == 1)
					dst_tci->itci.vid = tag_rule->treatment_inner.vid;
				else if (tag_rule->treatment_tag_to_remove == 0)
					dst_tci->otci.vid = tag_rule->treatment_inner.vid;
			}else if (tag_rule->treatment_inner.vid == 4096 ) {
				if (tag_rule->treatment_tag_to_remove == 1)
					dst_tci->itci.vid = src_tci->itci.vid;
				else if (tag_rule->treatment_tag_to_remove == 0)
					dst_tci->otci.vid = src_tci->itci.vid;
			}else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;
			
			switch (tag_rule->treatment_inner.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->itci.tpid = src_tci->itci.tpid;
						dst_tci->itci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->otci.tpid = src_tci->itci.tpid ;
						dst_tci->otci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->itci.tpid = 0x8100;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->otci.tpid = 0x8100;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 0;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 1;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100: //tpid = input tpid, de = copy from inner
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->itci.tpid = tag_rule->input_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->itci.tpid = tag_rule->input_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					}
					break;	
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 1;
					break;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			switch (tag_rule->treatment_outer.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.priority = tag_rule->treatment_outer.priority;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.priority = tag_rule->treatment_outer.priority;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.priority = src_tci->itci.priority;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.priority = src_tci->itci.priority;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			if ( tag_rule->treatment_outer.vid < 4095 ) {
				if (tag_rule->treatment_tag_to_remove == 1) {
					dst_tci->otci.vid = tag_rule->treatment_outer.vid;
				} else if (tag_rule->treatment_tag_to_remove == 0) {
					dst_tci->ext1_tci.vid = tag_rule->treatment_outer.vid;
				}
			} else if (tag_rule->treatment_outer.vid == 4096 ) {
				if (tag_rule->treatment_tag_to_remove == 1) {
					dst_tci->otci.vid = src_tci->itci.vid;
				} else if (tag_rule->treatment_tag_to_remove == 0) {
					dst_tci->ext1_tci.vid = src_tci->itci.vid;
				}
			}else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;

			switch (tag_rule->treatment_outer.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = src_tci->itci.tpid;
						dst_tci->otci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = src_tci->itci.tpid ;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = 0x8100;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = 0x8100;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 0;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 1;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 1;
					break;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}
			break;
		case 2:
			if ( tag_rule->treatment_tag_to_remove == 2){
				dst_tci->otci.tpid = 0;
				dst_tci->otci.priority = 0;
				dst_tci->otci.de = 0;
				dst_tci->otci.vid = 0;
				dst_tci->itci.tpid = 0;
				dst_tci->itci.priority = 0;
				dst_tci->itci.de = 0;
				dst_tci->itci.vid = 0;
			}else if ( tag_rule->treatment_tag_to_remove == 1) {
				dst_tci->otci.tpid = 0;
				dst_tci->otci.priority = 0;
				dst_tci->otci.de = 0;
				dst_tci->otci.vid = 0;
			}
			switch (tag_rule->treatment_inner.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					if (tag_rule->treatment_tag_to_remove == 2)
						dst_tci->itci.priority = tag_rule->treatment_inner.priority;
					else if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->otci.priority = tag_rule->treatment_inner.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->ext1_tci.priority = tag_rule->treatment_inner.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
					if (tag_rule->treatment_tag_to_remove == 2)
						dst_tci->itci.priority = src_tci->itci.priority;
					else if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->otci.priority = src_tci->itci.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->ext1_tci.priority = src_tci->itci.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_9:
					if (tag_rule->treatment_tag_to_remove == 2)
						dst_tci->itci.priority = src_tci->otci.priority;
					else if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->otci.priority = src_tci->otci.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->ext1_tci.priority = src_tci->otci.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			if ( tag_rule->treatment_inner.vid < 4095 ) {
				if (tag_rule->treatment_tag_to_remove == 2)
					dst_tci->itci.vid = tag_rule->treatment_inner.vid;
				else if (tag_rule->treatment_tag_to_remove == 1)
					dst_tci->otci.vid = tag_rule->treatment_inner.vid;
				else if (tag_rule->treatment_tag_to_remove == 2)
					dst_tci->ext1_tci.vid = tag_rule->treatment_inner.vid;
			}else if (tag_rule->treatment_inner.vid == 4096 ) {
				if (tag_rule->treatment_tag_to_remove == 2)
					dst_tci->itci.vid = src_tci->itci.vid;
				else if (tag_rule->treatment_tag_to_remove == 1)
					dst_tci->otci.vid = src_tci->itci.vid;
				else if (tag_rule->treatment_tag_to_remove == 0)
					dst_tci->ext1_tci.vid = src_tci->itci.vid;
			}else if (tag_rule->treatment_inner.vid == 4097 ) {
				if (tag_rule->treatment_tag_to_remove == 2)
					dst_tci->itci.vid = src_tci->otci.vid;
				else if (tag_rule->treatment_tag_to_remove == 1)
					dst_tci->otci.vid = src_tci->otci.vid;
				else if (tag_rule->treatment_tag_to_remove == 0)
					dst_tci->ext1_tci.vid = src_tci->otci.vid;
			}else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;

			switch (tag_rule->treatment_inner.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = src_tci->itci.tpid;
						dst_tci->itci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = src_tci->itci.tpid ;
						dst_tci->otci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = src_tci->itci.tpid ;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = src_tci->otci.tpid;
						dst_tci->itci.de = src_tci->otci.de;
					}else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = src_tci->otci.tpid ;
						dst_tci->otci.de = src_tci->otci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = src_tci->otci.tpid ;
						dst_tci->ext1_tci.de = src_tci->otci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = src_tci->otci.de;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->otci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = src_tci->otci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					if (tag_rule->treatment_tag_to_remove == 2)
						dst_tci->itci.tpid = 0x8100;
					else if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->otci.tpid = 0x8100;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->ext1_tci.tpid = 0x8100;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 0;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->itci.tpid = tag_rule->output_tpid;
						dst_tci->itci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 1;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->itci.tpid = 0x8100;
					dst_tci->itci.de = 1;
					break;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			switch (tag_rule->treatment_outer.priority ) {
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.priority = tag_rule->treatment_outer.priority;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.priority = tag_rule->treatment_outer.priority;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.priority = tag_rule->treatment_outer.priority;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.priority = src_tci->itci.priority;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.priority = src_tci->itci.priority;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.priority = src_tci->itci.priority;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_9:
					if (tag_rule->treatment_tag_to_remove == 2)
						dst_tci->otci.priority = src_tci->otci.priority;
					else if (tag_rule->treatment_tag_to_remove == 1)
						dst_tci->ext1_tci.priority = src_tci->otci.priority;
					else if (tag_rule->treatment_tag_to_remove == 0)
						dst_tci->ext2_tci.priority = src_tci->otci.priority;
					break;
				case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
					return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}

			if ( tag_rule->treatment_outer.vid < 4095 ) {
				if (tag_rule->treatment_tag_to_remove == 2) {
					dst_tci->otci.vid = tag_rule->treatment_outer.vid;
				} else if (tag_rule->treatment_tag_to_remove == 1) {
					dst_tci->ext1_tci.vid = tag_rule->treatment_outer.vid;
				} else if (tag_rule->treatment_tag_to_remove == 0) {
					dst_tci->ext2_tci.vid = tag_rule->treatment_outer.vid;
				}
			} else if (tag_rule->treatment_outer.vid == 4096 ) {
				if (tag_rule->treatment_tag_to_remove == 2) {
					dst_tci->otci.vid = src_tci->itci.vid;
				} else if (tag_rule->treatment_tag_to_remove == 1) {
					dst_tci->ext1_tci.vid = src_tci->itci.vid;
				} else if (tag_rule->treatment_tag_to_remove == 0) {
					dst_tci->ext2_tci.vid = src_tci->itci.vid;
				}
			} else if (tag_rule->treatment_outer.vid == 4097 ) {
				if (tag_rule->treatment_tag_to_remove == 2) {
					dst_tci->otci.vid = src_tci->otci.vid;
				} else if (tag_rule->treatment_tag_to_remove == 1) {
					dst_tci->ext1_tci.vid = src_tci->otci.vid;
				} else if (tag_rule->treatment_tag_to_remove == 0) {
					dst_tci->ext2_tci.vid = src_tci->otci.vid;
				}
			}else
				return CPUPORT_VTAGGING_TREATMENT_FAIL;

			switch (tag_rule->treatment_outer.tpid_de) {
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = src_tci->itci.tpid;
						dst_tci->otci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = src_tci->itci.tpid ;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = src_tci->itci.tpid ;
						dst_tci->ext2_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = src_tci->otci.tpid;
						dst_tci->otci.de = src_tci->otci.de;
					}else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = src_tci->otci.tpid ;
						dst_tci->ext1_tci.de = src_tci->otci.de;
					}else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = src_tci->otci.tpid ;
						dst_tci->ext2_tci.de = src_tci->otci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = src_tci->itci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext2_tci.de = src_tci->itci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = src_tci->otci.de;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = src_tci->otci.de;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext2_tci.de = src_tci->otci.de;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = 0x8100;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = 0x8100;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = 0x8100;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 0;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext2_tci.de = 0;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
					if (tag_rule->treatment_tag_to_remove == 2) {
						dst_tci->otci.tpid = tag_rule->output_tpid;
						dst_tci->otci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 1) {
						dst_tci->ext1_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext1_tci.de = 1;
					} else if (tag_rule->treatment_tag_to_remove == 0) {
						dst_tci->ext2_tci.tpid = tag_rule->output_tpid;
						dst_tci->ext2_tci.de = 1;
					}
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 0;
					break;
				case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
					dst_tci->otci.tpid = 0x8100;
					dst_tci->otci.de = 1;
					break;
				default:
					return CPUPORT_VTAGGING_TREATMENT_FAIL;
			}
			break;
		default:
			return CPUPORT_VTAGGING_TREATMENT_FAIL;
	}

	if (dst_tci->ext2_tci.tpid | dst_tci->ext2_tci.priority | dst_tci->ext2_tci.de | dst_tci->ext2_tci.vid )
		return CPUPORT_VTAGGING_TREATMENT_4TAGS;
	else if (dst_tci->ext1_tci.tpid | dst_tci->ext1_tci.priority | dst_tci->ext1_tci.de | dst_tci->ext1_tci.vid )
		return CPUPORT_VTAGGING_TREATMENT_3TAGS;

	return CPUPORT_VTAGGING_TREATMENT_SUCCESS;
}

static int
cpuport_vtagging (struct me_t *port, unsigned char is_upstream, struct cpuport_tci_t *src_tci, struct cpuport_tci_t *dst_tci, struct cpuport_info_t *cpuport_info)
{
	//struct me_t *ibridge_port_me=NULL;
	//unsigned int port_type;
	struct batchtab_cb_tagging_t *t = batchtab_table_data_get("tagging");
	struct omciutil_vlan_rule_fields_t *rule_fields_pos, *rule_fields_n;
	struct list_head *rule_list = NULL;
	int result = 0;
	unsigned char i = 0;

	// default assume dst_tci is the same as src_tci
	*dst_tci = *src_tci;

	if (t == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "batchtab_cb_tagging == NULL\n");
		return CPUPORT_VTAGGING_RULE_UNMATCH;
	}

	if ( port == NULL ) {
		dbprintf_cpuport(LOG_ERR, "port == NULL\n");
		batchtab_table_data_put("tagging");
		return CPUPORT_VTAGGING_RULE_UNMATCH;
	}

	//ibridge_port_me = hwresource_public2private(port);
	//port_type = meinfo_util_me_attr_data(ibridge_port_me, 4);	//attr 4 = port type

	for ( i = 0 ; i < t->total ; i++ ) {
		if ( t->tagging_bport[i].bport_meid == port->meid )
			break;
	}
	if (i ==  t->total ) {
		batchtab_table_data_put("tagging");
		return CPUPORT_VTAGGING_RULE_UNMATCH;
	}

	// tag match ////////////////////////////////////
	rule_list = (is_upstream) ? &(t->tagging_bport[i].rule_list_us) : &(t->tagging_bport[i].rule_list_ds);
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, rule_list, rule_node) {
		if ((rule_fields_pos->l2_filter_enable == 0 ||
			(cpuport_vtagging_match_l2(is_upstream, cpuport_info, &rule_fields_pos->l2_filter) == CPUPORT_VTAGGING_RULE_MATCH)) &&
		 	((result=cpuport_vtagging_match(src_tci,rule_fields_pos)) == CPUPORT_VTAGGING_RULE_MATCH)) {
			util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "Rule%d: %s\n", 
				rule_fields_pos->entry_id, omciutil_vlan_str_desc(rule_fields_pos, "\n\t", "", 0));
				
			// tag treatment ////////////////////////////////////
			result = cpuport_vtagging_treatment( src_tci, rule_fields_pos, dst_tci);
			if (result == CPUPORT_VTAGGING_TREATMENT_FAIL) {
				//dbprintf_cpuport(LOG_NOTICE, "vtagging rule drops pkt!\n");
				*dst_tci = *src_tci;
			}
			batchtab_table_data_put("tagging");
			return result;
		}
	}

	batchtab_table_data_put("tagging");
	dbprintf_cpuport(LOG_NOTICE, "vtagging rule not found for the pkt\n");
	return CPUPORT_VTAGGING_RULE_UNMATCH;
}

/// vfilter realted ////////////////////////////////////////////////////////

// always pass
static inline int
action_a(int is_ingress, int tag_match)
{
	return CPUPORT_VFILTER_PASS;
}
// always block
static inline int
action_c(int is_ingress, int tag_match)
{
	return CPUPORT_VFILTER_BLOCK;
}
// egress negative filter
static inline int
action_g(int is_ingress, int tag_match)
{
	if (is_ingress)
		return CPUPORT_VFILTER_PASS;
	if (tag_match)
		return CPUPORT_VFILTER_BLOCK;
	return CPUPORT_VFILTER_PASS;
}
// bidirection postive filter
static inline int
action_h(int is_ingress, int tag_match)
{
	if (tag_match)
		return CPUPORT_VFILTER_PASS;
	return CPUPORT_VFILTER_BLOCK;
}
// egress postive filter, and unknow DA drop
// we are unable to know if pkt DA is unknown in switch, so we don't check if DA is unknow
static inline int
action_j(int is_ingress, int tag_match)
{
	if (is_ingress)
		return CPUPORT_VFILTER_PASS;
	if (tag_match)
		return CPUPORT_VFILTER_PASS;
	return CPUPORT_VFILTER_BLOCK;
}
static int
cpuport_vfilter_execute( struct batchtab_cb_filtering_bport_t *b, int is_ingress, struct cpuport_tci_t *src_tci)
{
	int tag_num = cpuport_util_get_tagnum_from_tci(src_tci);
	int vid_match = 0, pri_match = 0, tci_match = 0;
	int i;

	if (tag_num) {
		switch (b->op_mode) {
			// vid match ////////////////////////////////////
			case 0x3:
			case 0x4:
			case 0x5:
			case 0x6:
			case 0xf:
			case 0x10:
			case 0x16:
			case 0x17:
			case 0x1c:
			case 0x1d:
				for ( i=0 ; i < b->entry_num ; i++ ) {
					if (tag_num == 1) {
						if(src_tci->itci.vid == b->vid[i])
							vid_match = 1;
					} else {
						if(src_tci->otci.vid == b->vid[i])
							vid_match = 1;
					}
				}
				break;
			// pri match ////////////////////////////////////
			case 0x7:
			case 0x8:
			case 0x9:
			case 0xa:
			case 0x11:
			case 0x12:
			case 0x18:
			case 0x19:
			case 0x1e:
			case 0x1f:
				for ( i=0 ; i < b->entry_num ; i++ ) {
					if (tag_num == 1) {
						if(src_tci->itci.priority== b->pri[i])
							pri_match = 1;
					} else {
						if(src_tci->otci.priority== b->pri[i])
							pri_match = 1;
					}
				}
				break;
			// tci match ////////////////////////////////////
			case 0xb:
			case 0xc:
			case 0xd:
			case 0xe:
			case 0x13:
			case 0x14:
			case 0x1a:
			case 0x1b:
			case 0x20:
			case 0x21:
				for ( i=0 ; i < b->entry_num ; i++ ) {
					if (tag_num == 1) {
						if(src_tci->itci.priority == b->pri[i]
							&& src_tci->itci.de == b->cfi[i]
							&& src_tci->itci.vid == b->vid[i])
							tci_match = 1;
					} else {
						if(src_tci->otci.priority == b->pri[i]
							&& src_tci->otci.de == b->cfi[i]
							&& src_tci->otci.vid == b->vid[i])
							tci_match = 1;
					}
				}
				break;
		}
	}

	switch (b->op_mode) {
		// always forward
		case 0x0:	return (tag_num)?action_a(is_ingress, 0):action_a(is_ingress, 0);
		// forward untag
		case 0x1:	return (tag_num)?action_c(is_ingress, 0):action_a(is_ingress, 0);
		// forward tag
		case 0x2:
		case 0x15:	return (tag_num)?action_a(is_ingress, 0):action_c(is_ingress, 0);

		// vid match, bidir positive
		case 0x3:
		case 0xf:
		case 0x1c:	return (tag_num)?action_h(is_ingress, vid_match):action_a(is_ingress, vid_match);
		case 0x4:
		case 0x10:
		case 0x1d:	return (tag_num)?action_h(is_ingress, vid_match):action_c(is_ingress, vid_match);
		// vid match, egress negative
		case 0x5:	return (tag_num)?action_g(is_ingress, vid_match):action_a(is_ingress, vid_match);
		case 0x6:	return (tag_num)?action_g(is_ingress, vid_match):action_c(is_ingress, vid_match);

		// pri match, bidir positive
		case 0x7:
		case 0x11:
		case 0x1e:	return (tag_num)?action_h(is_ingress, pri_match):action_a(is_ingress, pri_match);
		case 0x8:
		case 0x12:
		case 0x1f:	return (tag_num)?action_h(is_ingress, pri_match):action_c(is_ingress, pri_match);
		// pri match, egress negative
		case 0x9:	return (tag_num)?action_g(is_ingress, pri_match):action_a(is_ingress, pri_match);
		case 0xa:	return (tag_num)?action_g(is_ingress, pri_match):action_c(is_ingress, pri_match);

		// tci match, bidir positive
		case 0xb:
		case 0x13:
		case 0x20:	return (tag_num)?action_h(is_ingress, tci_match):action_a(is_ingress, tci_match);
		case 0xc:
		case 0x14:
		case 0x21:	return (tag_num)?action_h(is_ingress, tci_match):action_c(is_ingress, tci_match);
		//tci match, egress negative
		case 0xd:	return (tag_num)?action_g(is_ingress, tci_match):action_a(is_ingress, tci_match);
		case 0xe:	return (tag_num)?action_g(is_ingress, tci_match):action_c(is_ingress, tci_match);

		// vid match, egress positive
		case 0x16:	return (tag_num)?action_j(is_ingress, vid_match):action_a(is_ingress, vid_match);
		case 0x17:      return (tag_num)?action_j(is_ingress, vid_match):action_c(is_ingress, vid_match);
		// pri match, egress positive
		case 0x18:	return (tag_num)?action_j(is_ingress, pri_match):action_a(is_ingress, pri_match);
		case 0x19:      return (tag_num)?action_j(is_ingress, pri_match):action_c(is_ingress, pri_match);
		// tci match, egress positive
		case 0x1a:	return (tag_num)?action_j(is_ingress, tci_match):action_a(is_ingress, tci_match);
		case 0x1b:      return (tag_num)?action_j(is_ingress, tci_match):action_c(is_ingress, tci_match);
	}
	return CPUPORT_VFILTER_PASS;
}

int
cpuport_vfilter(struct me_t *src_port, int is_ingress, struct cpuport_tci_t *src_tci)
{
	struct batchtab_cb_filtering_t *f = batchtab_table_data_get("filtering");
	unsigned char i;
	int ret;

	if (f == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "batchtab_cb_filtering == NULL\n");
		return CPUPORT_VFILTER_PASS;
	}
	if (src_port == NULL) {
		dbprintf_cpuport(LOG_ERR, "src port == NULL\n");
		return CPUPORT_VFILTER_BLOCK;
	}

	//vlan filtering
	for ( i = 0 ; i < f->total ; i++ ) {
		if ( f->filtering_bport[i].bport_meid == src_port->meid )
			break;
	}
	if (i == f->total) {
		batchtab_table_data_put("filtering");
		return CPUPORT_VFILTER_PASS;
	}

	ret = cpuport_vfilter_execute(&f->filtering_bport[i], is_ingress, src_tci);
	batchtab_table_data_put("filtering");
	return ret;
}

/// cpuport_vtagging_vfilter, the caller of pvlan_vtagging, vtagging & vfilter //////////////
int
cpuport_vtagging_vfilter(struct me_t *src_port, struct me_t *dst_port, struct cpuport_tci_t *src_tci, struct cpuport_tci_t *dst_tci, struct cpuport_info_t *cpuport_info)
{
	struct cpuport_tci_t mid_tci;
	char *src_name="?", *dst_name="?";
	int is_pvlan_matched = 0;
	int is_upstream, ret;

	*dst_tci = *src_tci;

	if ( src_port==NULL || (src_name=hwresource_devname(src_port))==NULL)
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	if ( dst_port==NULL || (dst_name=hwresource_devname(dst_port))==NULL)
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	{
		struct me_t *ibridge_port_me = hwresource_public2private(src_port);
		unsigned int port_type = meinfo_util_me_attr_data(ibridge_port_me, 4);	//attr 4 = port type
		if (port_type == ENV_BRIDGE_PORT_TYPE_WAN || port_type == ENV_BRIDGE_PORT_TYPE_DS_BCAST)
			is_upstream = 0;	// src from wan/bcast means downstream
		else
			is_upstream = 1;
	}

	dbprintf_tci(LOG_NOTICE, "src_tci", src_tci);

	if (is_upstream) {
		ret = cpuport_pvlan_vtagging(src_port , is_upstream, src_tci, &mid_tci, cpuport_info);
		if (ret <0 || ret == CPUPORT_VTAGGING_RULE_UNMATCH) {
			ret = cpuport_vtagging( src_port, is_upstream, src_tci, &mid_tci, cpuport_info);
		} else {
			is_pvlan_matched = 1;
		}
	} else {
		ret = cpuport_vtagging( src_port, is_upstream, src_tci, &mid_tci, cpuport_info);
	}
	if (ret == CPUPORT_VTAGGING_TREATMENT_FAIL ) {
		dbprintf_cpuport(LOG_NOTICE, "bport 0x%x(%s)->0x%x(%s), ingress %svtagging DROP!\n",
			 src_port->meid, src_name, dst_port->meid, dst_name, is_pvlan_matched?"pvlan ":"");
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	} else if  (ret <0 || ret == CPUPORT_VTAGGING_RULE_UNMATCH) {
		if (is_upstream && omci_env_g.classf_add_default_rules == 0) {
			dbprintf_cpuport(LOG_WARNING, "bport 0x%x(%s)->0x%x(%s), ingress %svtagging unmatch DROP!\n",
				src_port->meid, src_name, dst_port->meid, dst_name, is_pvlan_matched?"pvlan ":"");
			return CPUPORT_VTAGGING_VFILTER_BLOCK;
		}
	}
	dbprintf_tci(LOG_NOTICE, "mid_tci", &mid_tci);

	if (cpuport_vfilter(src_port, 1, &mid_tci) == CPUPORT_VFILTER_BLOCK) {
		dbprintf_cpuport(LOG_NOTICE, "bport 0x%x(%s)->0x%x(%s), ingress vfilter DROP!\n",
			 src_port->meid, src_name, dst_port->meid, dst_name);
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	}
	if (cpuport_vfilter(dst_port, 0, &mid_tci) == CPUPORT_VFILTER_BLOCK) {
		dbprintf_cpuport(LOG_NOTICE, "bport 0x%x(%s)->0x%x(%s), egress vfilter DROP!\n",
			 src_port->meid, src_name, dst_port->meid, dst_name);
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	}

	if (is_upstream == 0) {
		ret = cpuport_pvlan_vtagging(dst_port , is_upstream, &mid_tci, dst_tci, cpuport_info);
		if (ret <0 || ret == CPUPORT_VTAGGING_RULE_UNMATCH) {
			ret = cpuport_vtagging(dst_port, is_upstream, &mid_tci, dst_tci, cpuport_info);
		} else {
			is_pvlan_matched = 1;
		}
	} else {
		ret = cpuport_vtagging( dst_port, is_upstream, &mid_tci, dst_tci, cpuport_info);
	}
	if (ret == CPUPORT_VTAGGING_TREATMENT_FAIL ) {
		dbprintf_cpuport(LOG_NOTICE, "bport 0x%x(%s)->0x%x(%s), egress %svtagging DROP!\n",
			 src_port->meid, src_name, dst_port->meid, dst_name, is_pvlan_matched?"pvlan ":"");
		return CPUPORT_VTAGGING_VFILTER_BLOCK;
	} else if  (ret <0 || ret == CPUPORT_VTAGGING_RULE_UNMATCH) {
		if (is_upstream == 0 && omci_env_g.classf_add_default_rules == 0 && omci_env_g.classf_ds_unmatch == 1) {
			dbprintf_cpuport(LOG_WARNING, "bport 0x%x(%s)->0x%x(%s), egress %svtagging unmatch DROP!\n",
				src_port->meid, src_name, dst_port->meid, dst_name, is_pvlan_matched?"pvlan ":"");
			return CPUPORT_VTAGGING_VFILTER_BLOCK;
		}
	}
	
	dbprintf_tci(LOG_NOTICE, "dst_tci", dst_tci);

	dbprintf_cpuport(LOG_NOTICE, "bport 0x%x(%s)->0x%x(%s), %svtagging & vfilter PASS!\n",
		 src_port->meid, src_name, dst_port->meid, dst_name, is_pvlan_matched?"pvlan ":"");

	return CPUPORT_VTAGGING_VFILTER_PASS;
}

