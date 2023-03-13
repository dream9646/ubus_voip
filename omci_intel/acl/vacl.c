/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
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
 * Module  : acl
 * File    : vacl.c
 *
 ******************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#ifndef	ACLTEST
#include "util.h"
#else
#include "vacltest_util.h"
#endif

#include "chipdef.h"
#include "list.h"
#include "vacl.h"
#include "vacl_util.h"

#include "util_inet.h"
#include "conv.h"
#include "switch.h"
#include "crc.h"
#include "cli.h"

struct vacl_conf_t vacl_conf_g;
struct vacl_user_node_t *rule_node_user_ary_p[VACL_ORDER_TOTAL];
struct vacl_ll_head_t rule_ll_head_user_ary_g[VACL_ORDER_TOTAL];
struct fwk_mutex_t vacl_nonrg_api_mutex;
struct fwk_mutex_t vacl_rg_api_mutex;

unsigned char is_acl_init_g = 0;
short ary_index_g = 0;
unsigned short meter_entry_reserve_g = 0;

unsigned int   ingress_range_vid_type_g;  //0:cvid 1:svid; default 0
unsigned short ingress_range_vid_lower_ary_g[INGRESS_RANGE_VID_INDEX_TOTAL]; //vid, 0~4095; default 0
unsigned short ingress_range_vid_upper_ary_g[INGRESS_RANGE_VID_INDEX_TOTAL]; //vid, 0~4095; default 0
unsigned short ingress_range_vid_ref_count_ary[INGRESS_RANGE_VID_INDEX_TOTAL];

unsigned int   ingress_range_l4port_type_g;  //0:dst-port 1:src-port; default 0
unsigned short ingress_range_l4port_lower_ary_g[INGRESS_RANGE_L4PORT_INDEX_TOTAL]; //l4 port, 0~65535
unsigned short ingress_range_l4port_upper_ary_g[INGRESS_RANGE_L4PORT_INDEX_TOTAL]; //l4 port, 0~65535
unsigned short ingress_range_l4port_ref_count_ary[INGRESS_RANGE_L4PORT_INDEX_TOTAL];

unsigned int   ingress_range_ip_lower_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL]; //ipv4 dip/sip; ipv6 dip/sip lsb 32-bits[31:0]
unsigned int   ingress_range_ip_upper_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL];
unsigned short ingress_range_ip_type_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL]; //IPRANGE_UNUSED=0, IPRANGE_IPV4_SIP=1, IPRANGE_IPV4_DIP=2, IPRANGE_IPV6_SIP=3, IPRANGE_IPV6_DIP=4
unsigned short ingress_range_ip_ref_count_ary[INGRESS_RANGE_IP_INDEX_TOTAL];

unsigned short ingress_range_pktlen_lower_ary_g[INGRESS_RANGE_PKTLEN_INDEX_TOTAL]; //packet length, 0~16383
unsigned short ingress_range_pktlen_upper_ary_g[INGRESS_RANGE_PKTLEN_INDEX_TOTAL]; //packet length, 0~16383
unsigned short ingress_range_pktlen_invert_g; //reverse state configuration
unsigned short ingress_range_pktlen_ref_count_ary[INGRESS_RANGE_PKTLEN_INDEX_TOTAL];

unsigned short field_selector_commit_ary_g[VACL_FIELD_SELECTOR_TOTAL];
unsigned short field_selector_format_ary_g[VACL_FIELD_SELECTOR_TOTAL];
unsigned short field_selector_offset_ary_g[VACL_FIELD_SELECTOR_TOTAL];
unsigned short ingress_field_selector_ref_count_ary_g[VACL_FIELD_SELECTOR_TOTAL];

unsigned int   ingress_range_ether_type_size_ary_g;
unsigned short ingress_range_ether_type_hwid_ary_g[INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL];
unsigned short ingress_range_ether_type_value_ary_g[INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL];
unsigned short ingress_range_ether_type_mask_ary_g[INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL];

unsigned int meter_rate_ary_g[VACL_METER_ENTRY_TOTAL];    //rate (unit 8Kbps), <0~1048568>
unsigned int meter_ifg_g;   //0:exclude 1:include; default 0
unsigned int meter_burst_ary_g[VACL_METER_ENTRY_TOTAL]; //bucket size(unit byte), <0~65535>
unsigned short meter_ref_count_ary_g[VACL_METER_ENTRY_TOTAL];

///////////////////////////////////////////////////////////////////
#ifndef RG_ENABLE
static void
ingress_range_ether_type_init(void)
{
	ingress_range_ether_type_size_ary_g = 0;
	memset(ingress_range_ether_type_hwid_ary_g,  0x0, INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL*sizeof(unsigned short));
	memset(ingress_range_ether_type_value_ary_g, 0x0, INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL*sizeof(unsigned short));
	memset(ingress_range_ether_type_mask_ary_g,  0x0, INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL*sizeof(unsigned short));
	return;
}

static int
ingress_range_mask_check(struct vacl_user_node_t *rule_p, int low, int high)
{
	int len = high - low + 1;
	if(is2pow(len) && (high == (low | (len-1)))) {
		ingress_range_ether_type_value_ary_g[ingress_range_ether_type_size_ary_g] = low;
		ingress_range_ether_type_mask_ary_g[ingress_range_ether_type_size_ary_g] = 0xffff&(~(len-1));
		/* printf("%-5d %d/0x%x\n",
		       len,
		       ingress_range_ether_type_value_ary_g[ingress_range_ether_type_size_ary_g],
		       ingress_range_ether_type_mask_ary_g[ingress_range_ether_type_size_ary_g]);
		*/
		if (rule_p->ingress_range_ether_type_number == 0) {
			rule_p->ingress_range_ether_type_index = ingress_range_ether_type_size_ary_g;
		}
		(rule_p->ingress_range_ether_type_number)++;
		ingress_range_ether_type_size_ary_g++;
		return 0;
	}
	return 1;
}

static int
ingress_range_half_down(struct vacl_user_node_t *rule_p, int low, int high, int mask)
{
	int flag, ret;

	ret = ingress_range_mask_check(rule_p, low, high);
	if(ret == 0)
		return 0;
	flag = 0;
	//printf("low=0x%x high=0x%x mask=0x%x\n", low, high, mask);
	while(!flag) {
		//printf("mask=0x%x\n", mask);
		if((low & mask) == (high & mask)) {
			mask >>= 1;
			if(mask==0) return 0;
		}
		else
			flag = 1;
	}
	ingress_range_half_down(rule_p, low, low | (mask-1), mask>>1);
	ingress_range_half_down(rule_p, high&(~(mask-1)), high, mask>>1);
	return 0;
}

static void
prepare_next_rule_node_ary_index(void)
{
	int i;

	ary_index_g = -1;
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_node_user_ary_p[i] == NULL) {
			ary_index_g = i;
			return;
		}
	}
	ary_index_g = VACL_ORDER_TOTAL;
	dbprintf_vacl(LOG_ERR, "Run out of rule node array!!!\n");
	return;
}

static int
rule_exist_crc_find(struct vacl_user_node_t **rule_p, unsigned int crc, int *sub_order)
{
	struct list_head *pos, *temp;
	int i;

	// iterate to check
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_ll_head_user_ary_g[i].count <= 0) continue;
		*sub_order = -1;
		list_for_each_safe(pos, temp, &(rule_ll_head_user_ary_g[i].list)) {
			*rule_p = list_entry(pos, struct vacl_user_node_t, list);
			(*sub_order)++;
			if (crc == (*rule_p)->crc) {
				dbprintf_vacl(LOG_DEBUG, "found existance ary_index=%d hw_order=%d order=%d sub_order=%d crc=0x%08X\n",
					 (*rule_p)->ary_index, (*rule_p)->hw_order, (*rule_p)->order, *sub_order, (*rule_p)->crc);
				return RET_YES;
			}
		}
	}
	return RET_NO;
}

static void
ingress_range_vid_find_index(struct vacl_user_node_t *rule_p, int *find_index, int flag_stag)
{
	int i = 0;
	unsigned short lb, ub;

	*find_index = -1;

	if (flag_stag) {
		lb = rule_p->ingress_stag_u.vid_range.lower;
		ub = rule_p->ingress_stag_u.vid_range.upper;
	}
	else {
		lb = rule_p->ingress_ctag_u.vid_range.lower;
		ub = rule_p->ingress_ctag_u.vid_range.upper;
	}
	for (i=0; (*find_index == -1) && (i<INGRESS_RANGE_VID_INDEX_TOTAL); i++) {
		if ((((ingress_range_vid_type_g>>i)&0x1) == flag_stag) &&
		    (ingress_range_vid_lower_ary_g[i] == lb) &&
		    (ingress_range_vid_upper_ary_g[i] == ub)) {
			*find_index = i;
		}
	}

	return;
}

static int
ingress_range_vid_find_available(int start, int *index)
{
	int i;

	*index = -1;
	for (i=start; i<INGRESS_RANGE_VID_INDEX_TOTAL; i++) {
		if (ingress_range_vid_ref_count_ary[i] == 0) {
			*index = i;
			return 0;
		}
	}

	return 1;
}

static void
ingress_range_l4port_find_index(struct vacl_user_node_t *rule_p, int *find_index, int flag_src)
{
	int i = 0;
	unsigned short lb, ub;

	*find_index = -1;

	if (flag_src) {
		lb = rule_p->ingress_l4sport_u.range.lower;
		ub = rule_p->ingress_l4sport_u.range.upper;
	}
	else {
		lb = rule_p->ingress_l4dport_u.range.lower;
		ub = rule_p->ingress_l4dport_u.range.upper;
	}

	for (i=0; (*find_index == -1) && (i<INGRESS_RANGE_L4PORT_INDEX_TOTAL); i++) {
		if ((((ingress_range_l4port_type_g>>i)&0x1) == flag_src) &&
		    (ingress_range_l4port_lower_ary_g[i] == lb) &&
		    (ingress_range_l4port_upper_ary_g[i] == ub)) {
			*find_index = i;
		}
	}

	return;
}

static int
ingress_range_l4port_find_available(int start, int *index)
{
	int i;

	*index = -1;
	for (i=start; i<INGRESS_RANGE_L4PORT_INDEX_TOTAL; i++) {
		if (ingress_range_l4port_ref_count_ary[i] == 0) {
			*index = i;
			return 0;
		}
	}
	return 1;
}

static void
ingress_range_ip_find_index(struct vacl_user_node_t *rule_p, int *find_index, int flag)
{
	int i = 0;
	unsigned int lb_uint, ub_uint;

	*find_index = -1;

	if (flag == INGRESS_RANGE_IP_SIPV4) {
		lb_uint = (unsigned int)rule_p->ingress_sipv4_u.range.lower.s_addr;
		ub_uint = (unsigned int)rule_p->ingress_sipv4_u.range.upper.s_addr;
	}
	else if (flag == INGRESS_RANGE_IP_DIPV4) {
		lb_uint = (unsigned int)rule_p->ingress_dipv4_u.range.lower.s_addr;
		ub_uint = (unsigned int)rule_p->ingress_dipv4_u.range.upper.s_addr;
	}
	else if (flag == INGRESS_RANGE_IP_DIPV6) {
		lb_uint = (unsigned int)(rule_p->ingress_dipv6_u.range.lower.s6_addr[15]|
					 (rule_p->ingress_dipv6_u.range.lower.s6_addr[14]<<8)|
					 (rule_p->ingress_dipv6_u.range.lower.s6_addr[13]<<16)|
					 (rule_p->ingress_dipv6_u.range.lower.s6_addr[12]<<24));
		ub_uint = (unsigned int)(rule_p->ingress_dipv6_u.range.upper.s6_addr[15]|
					 (rule_p->ingress_dipv6_u.range.upper.s6_addr[14]<<8)|
					 (rule_p->ingress_dipv6_u.range.upper.s6_addr[13]<<16)|
					 (rule_p->ingress_dipv6_u.range.upper.s6_addr[12]<<24));
	}
	else if (flag == INGRESS_RANGE_IP_SIPV6) {
		lb_uint = (unsigned int)(rule_p->ingress_sipv6_u.range.lower.s6_addr[15]|
					 (rule_p->ingress_sipv6_u.range.lower.s6_addr[14]<<8)|
					 (rule_p->ingress_sipv6_u.range.lower.s6_addr[13]<<16)|
					 (rule_p->ingress_sipv6_u.range.lower.s6_addr[12]<<24));
		ub_uint = (unsigned int)(rule_p->ingress_sipv6_u.range.upper.s6_addr[15]|
					 (rule_p->ingress_sipv6_u.range.upper.s6_addr[14]<<8)|
					 (rule_p->ingress_sipv6_u.range.upper.s6_addr[13]<<16)|
					 (rule_p->ingress_sipv6_u.range.upper.s6_addr[12]<<24));
	}
	else
		return;

	for (i=0; (*find_index == -1) && (i<INGRESS_RANGE_IP_INDEX_TOTAL); i++) {
		if ((ingress_range_ip_type_ary_g[i] == flag) &&
		    (ingress_range_ip_lower_ary_g[i] == lb_uint) &&
		    (ingress_range_ip_upper_ary_g[i] == ub_uint)) {
			*find_index = i;
		}
	}

	return;
}

static int
ingress_range_ip_find_available(int start, int *index)
{
	int i;

	*index = -1;
	for (i=start; i<INGRESS_RANGE_IP_INDEX_TOTAL; i++) {
		if (ingress_range_ip_ref_count_ary[i] == 0) {
			*index = i;
			return 0;
		}
	}
	return 1;
}

static void
ingress_range_pktlen_find_index(struct vacl_user_node_t *rule_p, int *find_index)
{
	int i = 0;

	*find_index = -1;

	for (i=0; (*find_index == -1) && (i<INGRESS_RANGE_PKTLEN_INDEX_TOTAL); i++) {
		if ((((ingress_range_pktlen_invert_g>>i)&0x1) == rule_p->ingress_pktlen_invert) &&
		    (ingress_range_pktlen_lower_ary_g[i] == rule_p->ingress_pktlen_range.lower) &&
		    (ingress_range_pktlen_upper_ary_g[i] == rule_p->ingress_pktlen_range.upper)) {
			*find_index = i;
		}
	}

	return;
}

static int
ingress_range_pktlen_find_available(int *index)
{
	int i = 0;

	*index = -1;
	for (i=0; i<INGRESS_RANGE_PKTLEN_INDEX_TOTAL; i++) {
		if (ingress_range_pktlen_ref_count_ary[i] == 0) {
			*index = i;
			return 0;
		}
	}

	return 1;
}

static void
ingress_range_vid_set(struct vacl_user_node_t *rule_p, int index, int flag_stag)
{
	if (flag_stag != 0) {
		ingress_range_vid_lower_ary_g[index] = rule_p->ingress_stag_u.vid_range.lower;
		ingress_range_vid_upper_ary_g[index] = rule_p->ingress_stag_u.vid_range.upper;
		ingress_range_vid_type_g |=  (0x1<<index);
	}
	else {
		ingress_range_vid_lower_ary_g[index] = rule_p->ingress_ctag_u.vid_range.lower;
		ingress_range_vid_upper_ary_g[index] = rule_p->ingress_ctag_u.vid_range.upper;
	}

	return;
}

static int
ingress_range_vid_unset(short index)
{
	if ((index < 0) || (index >= INGRESS_RANGE_VID_INDEX_TOTAL))
		return -1;

	ingress_range_vid_lower_ary_g[index] = 0;
	ingress_range_vid_upper_ary_g[index] = 0;
	ingress_range_vid_type_g &= (~(0x1<<index));
	ingress_range_vid_ref_count_ary[index] = 0;

	return 0;
}

static void
ingress_range_l4port_set(struct vacl_user_node_t *rule_p, int index, int flag_src)
{
	if (flag_src != 0) {
		ingress_range_l4port_lower_ary_g[index] = rule_p->ingress_l4sport_u.range.lower;
		ingress_range_l4port_upper_ary_g[index] = rule_p->ingress_l4sport_u.range.upper;
		ingress_range_l4port_type_g |=  (0x1<<index);
	}
	else {
		ingress_range_l4port_lower_ary_g[index] = rule_p->ingress_l4dport_u.range.lower;
		ingress_range_l4port_upper_ary_g[index] = rule_p->ingress_l4dport_u.range.upper;
	}
	return;
}

static int
ingress_range_l4port_unset(short index)
{
	ingress_range_l4port_lower_ary_g[index] = 0;
	ingress_range_l4port_upper_ary_g[index] = 0;
	ingress_range_l4port_type_g &= (~(0x1<<index));
	ingress_range_l4port_ref_count_ary[index] = 0;

	return 0;
}

static void
ingress_range_ip_set(struct vacl_user_node_t *rule_p, int index, int flag)
{
	if (flag == INGRESS_RANGE_IP_SIPV4) {
		ingress_range_ip_lower_ary_g[index] = (unsigned int)rule_p->ingress_sipv4_u.range.lower.s_addr;
		ingress_range_ip_upper_ary_g[index] = (unsigned int)rule_p->ingress_sipv4_u.range.upper.s_addr;
		ingress_range_ip_type_ary_g[index] = flag;
	}
	else if (flag == INGRESS_RANGE_IP_DIPV4) {
		ingress_range_ip_lower_ary_g[index] = (unsigned int)rule_p->ingress_dipv4_u.range.lower.s_addr;
		ingress_range_ip_upper_ary_g[index] = (unsigned int)rule_p->ingress_dipv4_u.range.upper.s_addr;
		ingress_range_ip_type_ary_g[index] = flag;
	}
	else if (flag == INGRESS_RANGE_IP_DIPV6) {
		ingress_range_ip_lower_ary_g[index] = (unsigned int) (rule_p->ingress_dipv6_u.range.lower.s6_addr[15]|
							  (rule_p->ingress_dipv6_u.range.lower.s6_addr[14]<<8)|
							  (rule_p->ingress_dipv6_u.range.lower.s6_addr[13]<<16)|
							  (rule_p->ingress_dipv6_u.range.lower.s6_addr[12]<<24));
		ingress_range_ip_upper_ary_g[index] = (unsigned int) (rule_p->ingress_dipv6_u.range.upper.s6_addr[15]|
							  (rule_p->ingress_dipv6_u.range.upper.s6_addr[14]<<8)|
							  (rule_p->ingress_dipv6_u.range.upper.s6_addr[13]<<16)|
							  (rule_p->ingress_dipv6_u.range.upper.s6_addr[12]<<24));
		ingress_range_ip_type_ary_g[index] = flag;
	}
	else if (flag == INGRESS_RANGE_IP_SIPV6) {
		ingress_range_ip_lower_ary_g[index] = (unsigned int) (rule_p->ingress_sipv6_u.range.lower.s6_addr[15]|
							  (rule_p->ingress_sipv6_u.range.lower.s6_addr[14]<<8)|
							  (rule_p->ingress_sipv6_u.range.lower.s6_addr[13]<<16)|
							  (rule_p->ingress_sipv6_u.range.lower.s6_addr[12]<<24));
		ingress_range_ip_upper_ary_g[index] = (unsigned int) (rule_p->ingress_sipv6_u.range.upper.s6_addr[15]|
							  (rule_p->ingress_sipv6_u.range.upper.s6_addr[14]<<8)|
							  (rule_p->ingress_sipv6_u.range.upper.s6_addr[13]<<16)|
							  (rule_p->ingress_sipv6_u.range.upper.s6_addr[12]<<24));
		ingress_range_ip_type_ary_g[index] = flag;
	}

	return;
}

static int
ingress_range_ip_unset(short index)
{
	if ((index < 0) || (index >= INGRESS_RANGE_IP_INDEX_TOTAL))
		return -1;

	ingress_range_ip_lower_ary_g[index] = 0;
	ingress_range_ip_upper_ary_g[index] = 0;
	ingress_range_ip_type_ary_g[index] = 0;
	ingress_range_ip_ref_count_ary[index] = 0;

	return 0;
}

static void
ingress_range_pktlen_set(struct vacl_user_node_t *rule_p, int index)
{
	ingress_range_pktlen_lower_ary_g[index] = rule_p->ingress_pktlen_range.lower;
	ingress_range_pktlen_upper_ary_g[index] = rule_p->ingress_pktlen_range.upper;
	if ((rule_p->ingress_care_u.bit.pktlen_invert == 1) && (rule_p->ingress_pktlen_invert))
		ingress_range_pktlen_invert_g |=  (0x1<<index);

	return;
}

static void
ingress_field_selector_set(struct vacl_user_node_t *rule_p, int index)
{
	if (rule_p->ingress_care_u.bit.ipv4_dscp_value == 1) {
		field_selector_commit_ary_g[index] = 1;
		field_selector_format_ary_g[index] = 3;
		field_selector_offset_ary_g[index] = 1;
	}
	else
		field_selector_commit_ary_g[index] = 0;
	return;
}

static int
ingress_range_pktlen_unset(int index)
{
	if ((index < 0) || (index >= INGRESS_RANGE_PKTLEN_INDEX_TOTAL))
		return -1;

	ingress_range_pktlen_lower_ary_g[index] = 0;
	ingress_range_pktlen_upper_ary_g[index] = 0;
	ingress_range_pktlen_invert_g &= (~(0x1<<index));
	ingress_range_pktlen_ref_count_ary[index] = 0;

	return 0;
}

static void
ingress_field_selector_unset(int index)
{
	field_selector_commit_ary_g[index] = 1;
	field_selector_format_ary_g[index] = 0;
	field_selector_offset_ary_g[index] = 0;
	return;
}

static int
ingress_field_set(struct vacl_user_node_t *rule_p)
{
	int ret = 0, index;
	unsigned short value = 0;

	if (((rule_p->ingress_care_u.bit.l4dport_lower == 1) && (rule_p->ingress_care_u.bit.l4dport_upper == 1)) ||
	    ((rule_p->ingress_care_u.bit.l4sport_lower == 1) && (rule_p->ingress_care_u.bit.l4sport_upper == 1))) {
		int index_dst = -1;
		int index_src = -1;

		if ((rule_p->ingress_care_u.bit.l4dport_lower == 1) && (rule_p->ingress_care_u.bit.l4dport_upper == 1)) {
			ingress_range_l4port_find_index(rule_p, &index_dst, 0);
			if (index_dst == -1) {
				ret = ingress_range_l4port_find_available(0, &index_dst);
				if (ret != 0)
					return 1;
			}
		}
		if ((rule_p->ingress_care_u.bit.l4sport_lower == 1) && (rule_p->ingress_care_u.bit.l4sport_upper == 1)) {
			ingress_range_l4port_find_index(rule_p, &index_src, 1);
			if (index_src == -1) {
				ret = ingress_range_l4port_find_available(index_dst+1, &index_src);
				if (ret != 0)
					return 1;
			}
		}

		if (index_dst != -1) {
			ingress_range_l4port_set(rule_p, index_dst, 0);
			rule_p->hw_template_field_bit_range_l4port |= (0x1<<index_dst);
			ingress_range_l4port_ref_count_ary[index_dst]++;
		}
		if (index_src != -1) {
			ingress_range_l4port_set(rule_p, index_src, 1);
			rule_p->hw_template_field_bit_range_l4port |= (0x1<<index_src);
			ingress_range_l4port_ref_count_ary[index_src]++;
		}
	}
	if (((rule_p->ingress_care_u.bit.ctag_vid_lower == 1) && (rule_p->ingress_care_u.bit.ctag_vid_upper == 1)) ||
	    ((rule_p->ingress_care_u.bit.stag_vid_lower == 1) && (rule_p->ingress_care_u.bit.stag_vid_upper == 1))) {
		int index_ctag = -1;
		int index_stag = -1;

		if ((rule_p->ingress_care_u.bit.ctag_vid_lower == 1) && (rule_p->ingress_care_u.bit.ctag_vid_upper == 1)) {
			ingress_range_vid_find_index(rule_p, &index_ctag, 0);
			if (index_ctag == -1) {
				ret = ingress_range_vid_find_available(0, &index_ctag);
				if (ret != 0)
					return 1;
			}
		}
		if ((rule_p->ingress_care_u.bit.stag_vid_lower == 1) && (rule_p->ingress_care_u.bit.stag_vid_upper == 1)) {
			ingress_range_vid_find_index(rule_p, &index_stag, 1);
			if (index_stag == -1) {
				ret = ingress_range_vid_find_available(index_ctag+1, &index_stag);
				if (ret != 0)
					return 1;
			}
		}

		if (index_ctag != -1) {
			ingress_range_vid_set(rule_p, index_ctag, 0);
			rule_p->hw_template_field_bit_range_vid |= (0x1<<index_ctag);
			ingress_range_vid_ref_count_ary[index_ctag]++;
		}
		if (index_stag != -1) {
			ingress_range_vid_set(rule_p, index_stag, 1);
			rule_p->hw_template_field_bit_range_vid |= (0x1<<index_stag);
			ingress_range_vid_ref_count_ary[index_stag]++;
		}
	}
	if (((rule_p->ingress_care_u.bit.dipv4_lower == 1) && (rule_p->ingress_care_u.bit.dipv4_upper == 1)) ||
	    ((rule_p->ingress_care_u.bit.sipv4_lower == 1) && (rule_p->ingress_care_u.bit.sipv4_upper == 1)) ||
	    ((rule_p->ingress_care_u.bit.sipv6_lower == 1) && (rule_p->ingress_care_u.bit.sipv6_upper == 1)) ||
	    ((rule_p->ingress_care_u.bit.dipv6_lower == 1) && (rule_p->ingress_care_u.bit.dipv6_upper == 1))) {
		int index01 = -1, index02 = -1, index03 = -1, index04 = -1;
		int tmp;

		value = 0;
		if ((rule_p->ingress_care_u.bit.dipv6_lower == 1) && (rule_p->ingress_care_u.bit.dipv6_upper == 1)) {
			// FIXME, LIGHT, leagal input check
			ingress_range_ip_find_index(rule_p, &index01, INGRESS_RANGE_IP_DIPV6);
			if (index01 == -1) {
				ret = ingress_range_ip_find_available(0, &index01);
				if (ret != 0)
					return 1;
			}
			value |= (0x1<<index01);
		}
		if ((rule_p->ingress_care_u.bit.sipv6_lower == 1) && (rule_p->ingress_care_u.bit.sipv6_upper == 1)) {
			// FIXME, LIGHT, leagal input check
			ingress_range_ip_find_index(rule_p, &index02, INGRESS_RANGE_IP_SIPV6);
			if (index02 == -1) {
				ret = ingress_range_ip_find_available(index01+1, &index02);
				if (ret != 0)
					return 1;
			}
			value |= (0x1<<index02);
		}
		if ((rule_p->ingress_care_u.bit.dipv4_lower == 1) && (rule_p->ingress_care_u.bit.dipv4_upper == 1)) {
			// FIXME, LIGHT, leagal input check
			ingress_range_ip_find_index(rule_p, &index03, INGRESS_RANGE_IP_DIPV4);
			if (index03 == -1) {
				tmp = -1;
				tmp = which_bigger(index01, index02);
				ret = ingress_range_ip_find_available(tmp+1, &index03);
				if (ret != 0)
					return 1;
			}
			value |= (0x1<<index03);
		}
		if ((rule_p->ingress_care_u.bit.sipv4_lower == 1) && (rule_p->ingress_care_u.bit.sipv4_upper == 1)) {
			// FIXME, LIGHT, leagal input check
			ingress_range_ip_find_index(rule_p, &index04, INGRESS_RANGE_IP_SIPV4);
			if (index04 == -1) {
				tmp = -1;
				tmp = which_bigger(index01, index02);
				tmp = which_bigger(tmp, index03);
				ret = ingress_range_ip_find_available(tmp+1, &index04);
				if (ret != 0)
					return 1;
			}
			value |= (0x1<<index04);
		}
		if (index01 != -1) {
			ingress_range_ip_set(rule_p, index01, INGRESS_RANGE_IP_DIPV6);
			ingress_range_ip_ref_count_ary[index01]++;
		}
		if (index02 != -1) {
			ingress_range_ip_set(rule_p, index02, INGRESS_RANGE_IP_SIPV6);
			ingress_range_ip_ref_count_ary[index02]++;
		}
		if (index03 != -1) {
			ingress_range_ip_set(rule_p, index03, INGRESS_RANGE_IP_DIPV4);
			ingress_range_ip_ref_count_ary[index03]++;
		}
		if (index04 != -1) {
			ingress_range_ip_set(rule_p, index04, INGRESS_RANGE_IP_SIPV4);
			ingress_range_ip_ref_count_ary[index04]++;
		}
		rule_p->hw_template_field_bit_range_ip = value;
	}
	if ((rule_p->ingress_care_u.bit.pktlen_lower == 1) &&
	    (rule_p->ingress_care_u.bit.pktlen_upper == 1)) {
		index = -1;
		ingress_range_pktlen_find_index(rule_p, &index);
		if (index == -1) {
			ret = ingress_range_pktlen_find_available(&index);
			if (ret != 0)
				return 1;
		}
		ingress_range_pktlen_set(rule_p, index);
		rule_p->hw_template_field_bit_range_pktlen |= (0x1<<index);
		ingress_range_pktlen_ref_count_ary[index]++;
	}
	if (rule_p->ingress_care_u.bit.ipv4_dscp_value == 1) {
		index = 7;
		ingress_field_selector_set(rule_p, index);
		rule_p->hw_template_field_bit_field_selector |= (0x1<<index);
		ingress_field_selector_ref_count_ary_g[index]++;
	}

	return 0;
}

static int
ingress_field_unset(struct vacl_user_node_t *rule_p)
{
	int i;

	if (rule_p->hw_template_field_bit_range_vid) {
		for (i=0; i<INGRESS_RANGE_VID_INDEX_TOTAL; i++) {
			if (((rule_p->hw_template_field_bit_range_vid)>>i)&0x1) {
				ingress_range_vid_ref_count_ary[i]--;
				if(ingress_range_vid_ref_count_ary[i] <= 0) {
					ingress_range_vid_unset(i);
				}
			}
		}
	}
	if (rule_p->hw_template_field_bit_range_l4port) {
		for (i=0; i<INGRESS_RANGE_L4PORT_INDEX_TOTAL; i++) {
			if (((rule_p->hw_template_field_bit_range_l4port)>>i)&0x1) {
				ingress_range_l4port_ref_count_ary[i]--;
				if(ingress_range_l4port_ref_count_ary[i] <= 0) {
					ingress_range_l4port_unset(i);
				}
			}
		}
	}
	if (rule_p->hw_template_field_bit_range_ip) {
		for (i=0; i<INGRESS_RANGE_IP_INDEX_TOTAL; i++) {
			if (((rule_p->hw_template_field_bit_range_ip)>>i)&0x1) {
				ingress_range_ip_ref_count_ary[i]--;
				if(ingress_range_ip_ref_count_ary[i] <= 0) {
					ingress_range_ip_unset(i);
				}
			}
		}
	}
	if (rule_p->hw_template_field_bit_range_pktlen) {
		for (i=0; i<INGRESS_RANGE_PKTLEN_INDEX_TOTAL; i++) {
			if (((rule_p->hw_template_field_bit_range_pktlen)>>i)&0x1) {
				ingress_range_pktlen_ref_count_ary[i]--;
				if(ingress_range_pktlen_ref_count_ary[i] <= 0) {
					ingress_range_pktlen_unset(i);
				}
			}
		}
	}
	if (rule_p->hw_template_field_bit_field_selector) {
		for (i=0; i<VACL_FIELD_SELECTOR_TOTAL; i++) {
			if (((rule_p->hw_template_field_bit_field_selector)>>i)&0x1) {
				ingress_field_selector_ref_count_ary_g[i]--;
				if(ingress_field_selector_ref_count_ary_g[i] <= 0) {
					ingress_field_selector_unset(i);
				}
			}
		}
	}
	return 0;
}
#endif

///////////////////////////////////////////////////////////////////
int
meter_table_is_index_available(int index)
{
	if (meter_ref_count_ary_g[index] == 0)
		return RET_YES;
	else
		return RET_NO;
}

int
meter_table_find_index(unsigned int bucket_size, unsigned int rate, unsigned char ifg, unsigned char action_share_meter, int *find_index, int *find_index_available)
{
	int i;
	*find_index = -1;
	*find_index_available = -1;
	if ((bucket_size == 0) && (rate == 0) && (ifg == 0)) {
		if (action_share_meter < get_vacl_meter_entry_total()) {
			dbprintf_vacl(LOG_INFO, "empty meter data and user assign meter table entry (%d)\n", action_share_meter);
			*find_index = action_share_meter;
			return 0;
		}
		else {
			dbprintf_vacl(LOG_ERR, "user assign meter table entry (%d) out of boundary (%d) error\n", action_share_meter, get_vacl_meter_entry_total());
			return -1;
		}
	}

	for (i=0; i<get_vacl_meter_entry_total(); i++) {
		if ((*find_index < 0) &&
		    (meter_burst_ary_g[i] == bucket_size) &&
		    (meter_rate_ary_g[i]  == rate)  &&
		    (((meter_ifg_g>>i)&0x1) == (ifg&0x1))) {
			*find_index = i;
		}
		if (*find_index_available < 0) {
			if ((meter_burst_ary_g[i] == ACT_METER_BUCKET_SIZE_DEFAULT) &&
			    (meter_rate_ary_g[i]  == ACT_METER_RATE_DEFAULT)  &&
			    (((meter_ifg_g>>i)&0x1) == ACT_METER_IFG_DEFAULT)) {
				*find_index_available = i;
			}
		}
	}
	dbprintf_vacl(LOG_DEBUG, "find_index          =%d\n", *find_index);
	dbprintf_vacl(LOG_DEBUG, "find_index_available=%d\n", *find_index_available);
	return 0;
}

void
meter_check_fix_input(unsigned int *bucket_size, unsigned int *rate)
{
	// Meter burst size (unit byte), <0~65535>
	if (*bucket_size > ACT_METER_BUCKET_SIZE_MAX) {
		dbprintf_vacl(LOG_NOTICE, "Force set bucket_size from %d to %d\n",
			 *bucket_size, ACT_METER_BUCKET_SIZE_MAX);
		*bucket_size = ACT_METER_BUCKET_SIZE_MAX;
	}

	// Meter rate (unit 1Kbps), <0~0x1ffff*8>
	if (*rate > ACT_METER_RATE_MAX) {
		dbprintf_vacl(LOG_NOTICE, "Force set rate from %d to %d\n",
			 *rate, ACT_METER_RATE_MAX);
		*rate = ACT_METER_RATE_MAX;
	}
	return;
}

static int
action_set_meter_new_meter(unsigned int bucket_size, unsigned int rate, unsigned char ifg, unsigned short meter_index)
{
	meter_rate_ary_g[meter_index] = rate;
	meter_burst_ary_g[meter_index] = bucket_size;
	if (ifg)
		meter_ifg_g |= (0x1<<(meter_index));
	meter_ref_count_ary_g[meter_index]++;

	return 0;
}

int
action_set_meter(unsigned int bucket_size, unsigned int rate, unsigned char ifg, unsigned short *meter_index)
{
	int ret, index=-1, index_available=-1;

	ret = meter_table_find_index(bucket_size, rate, ifg, *meter_index, &index, &index_available);
	if (ret != 0) return -1;
	ret=0;
	dbprintf_vacl(LOG_INFO, "bucket_size=[%d], rate=[%d], ifg=[%d], *meter_index=[%d], index=[%d], index_available=[%d]\n",
		 bucket_size, rate, ifg, *meter_index, index, index_available);

	if (index == -1) {
		dbprintf_vacl(LOG_DEBUG, "no existing rule\n");
		if (*meter_index == ACT_METER_INDEX_AUTO_W_SHARE || *meter_index == ACT_METER_INDEX_AUTO_WO_SHARE) {
			if (index_available < 0) {
				dbprintf_vacl(LOG_ERR, "meter table FULL for auto assign meter entry\n");
				return -1;
			}
			else {
				dbprintf_vacl(LOG_NOTICE, "auto assign meter entry %d\n", index_available);
				*meter_index = index_available;
			}
		}
		else
			dbprintf_vacl(LOG_INFO, "user assign meter entry %d\n", *meter_index);
		ret=action_set_meter_new_meter(bucket_size, rate, ifg, *meter_index);
	}
	else {
		dbprintf_vacl(LOG_DEBUG, "there is existing rule\n");
		if (*meter_index == ACT_METER_INDEX_AUTO_W_SHARE) {
			*meter_index = index;
			dbprintf_vacl(LOG_NOTICE, "auto assign meter entry (%d) and share the same meter\n", index);
#ifndef RG_ENABLE
			meter_ref_count_ary_g[*meter_index]++;
#endif
		}
		else if (*meter_index == ACT_METER_INDEX_AUTO_WO_SHARE) {
			if (index_available < 0) {
				dbprintf_vacl(LOG_ERR, "meter table full for auto assign meter entry and NOT share the same meter\n");
				return -1;
			}
			*meter_index = index_available;
			dbprintf_vacl(LOG_NOTICE, "auto assign meter entry (%d) for NOT share the same meter\n", index_available);
			ret=action_set_meter_new_meter(bucket_size, rate, ifg, *meter_index);
		}
		else {
			dbprintf_vacl(LOG_DEBUG, "user assign meter entry\n");
			if (index != *meter_index) {
				dbprintf_vacl(LOG_ERR, "ERROR! user define index [%d] is not same as occupied rule [%d]\n", *meter_index, index);
				return -1;
			}
			else {
				dbprintf_vacl(LOG_DEBUG, "user define index is same as occupied rule [%d]\n", index);
				meter_ref_count_ary_g[*meter_index]++;
			}
		}
	}
	return ret;
}

///////////////////////////////////////////////////////////////////
static int
act_check(struct vacl_user_node_t *rule_p)
{
	unsigned long long act_type;

	act_type = rule_p->act_type;
	if (act_type == 0) {
		dbprintf_vacl(LOG_ERR, "action type check error for act_type=0\n");
		return 1;
	}
	act_type >>= VACL_ACT_TOTAL_BIT;
	if (act_type != 0) {
		dbprintf_vacl(LOG_ERR, "action type check error for (act_type >>= VACL_ACT_TOTAL) != 0\n");
		return 1;
	}
	return 0;
}

static int
action_set(struct vacl_user_node_t *rule_p)
{
	int ret=0;
	unsigned short u_hw_meter_table_entry=0;

	if ((ret = act_check(rule_p)) != 0)
		return ret;

	if (rule_p->act_type & VACL_ACT_LOG_POLICING_MASK) {
		dbprintf_vacl(LOG_DEBUG, "act_type & VACL_ACT_LOG_POLICING_MASK\n");
		u_hw_meter_table_entry = rule_p->hw_meter_table_entry;
		meter_check_fix_input(&(rule_p->act_meter_bucket_size),
				      &(rule_p->act_meter_rate));
		ret = action_set_meter(rule_p->act_meter_bucket_size,
				       rule_p->act_meter_rate,
				       rule_p->act_meter_ifg,
				       &u_hw_meter_table_entry);
		rule_p->hw_meter_table_entry = u_hw_meter_table_entry;
		dbprintf_vacl(LOG_DEBUG, "ret=%d hw_meter_table_entry=%d\n", ret, rule_p->hw_meter_table_entry);
		dbprintf_vacl(LOG_DEBUG, "meter_ref_count_ary_g[%d]=%d\n", rule_p->hw_meter_table_entry, meter_ref_count_ary_g[rule_p->hw_meter_table_entry]);
	}
	return ret;
}

static int
action_unset(struct vacl_user_node_t *rule_p)
{
	int ret = 0;

	ret = act_check(rule_p);
	if (ret != 0) return 1;

	if ((rule_p->act_type & VACL_ACT_LOG_POLICING_MASK) &&
	    (rule_p->hw_meter_table_entry >= 0) &&
	    (rule_p->hw_meter_table_entry < get_vacl_meter_entry_total()) &&
	    (meter_ref_count_ary_g[rule_p->hw_meter_table_entry] > 0)) {
		meter_ref_count_ary_g[rule_p->hw_meter_table_entry]--;
		if (meter_ref_count_ary_g[rule_p->hw_meter_table_entry] <= 0) {
			meter_rate_ary_g[rule_p->hw_meter_table_entry] = ACT_METER_RATE_DEFAULT;
			meter_burst_ary_g[rule_p->hw_meter_table_entry] = ACT_METER_BUCKET_SIZE_DEFAULT;
			meter_ref_count_ary_g[rule_p->hw_meter_table_entry] = 0x0;
			meter_ifg_g &= (~(0x1<<(rule_p->hw_meter_table_entry)));
			switch_hw_g.vacl_meter_clear(rule_p->hw_meter_table_entry);
		}
	}
	return 0;
}

// 0: ok, 1: issued, -1: err
static int
vacl_add_handler(struct vacl_user_node_t *rule_p, int *sub_order)
{
	int ret;
	struct vacl_user_node_t *node_p;
	unsigned int crc, crc_accum = 0xFFFFFFFF;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init! init it.\n");
		_vacl_init();
	}
	if ((vacl_conf_g.count_rules >= vacl_conf_g.count_rules_max) || (ary_index_g >= VACL_ORDER_TOTAL)) {
		dbprintf_vacl(LOG_ERR, "acl rule out of range total num >= %d\n", VACL_ENTRY_TOTAL);
		return VACL_ADD_RET_FULL;
	}
	if (ary_index_g < 0) {
		dbprintf_vacl(LOG_ERR, "run out of acl node ary\n");
		return VACL_ADD_RET_FULL;
	}
	*sub_order = -1;
	crc = crc_be_update(crc_accum, (char *)rule_p + VACL_USER_NODE_INTERNAL_VAR_SIZE, sizeof(struct vacl_user_node_t) - VACL_USER_NODE_INTERNAL_VAR_SIZE);
	node_p = NULL;
	if (rule_exist_crc_find(&node_p, crc, sub_order)) {
		dbprintf_vacl(LOG_DEBUG, "acl rule found, ary_index=%d hw_order=%d order=%d sub_order=%d crc=0x%08X\n",
			 rule_p->ary_index, rule_p->hw_order, rule_p->order, *sub_order, rule_p->crc);
		return VACL_ADD_RET_DUPLICATE;
	}
	rule_p->crc = crc;
	rule_p->ary_index = ary_index_g;

	ret = action_set(rule_p);
	if (ret != 0) {
		dbprintf_vacl(LOG_DEBUG, "set_action fail %d\n", ret);
		return -1;
	}

	ret = ingress_field_set(rule_p);
	if (ret != 0) {
		dbprintf_vacl(LOG_DEBUG, "set_field fail %d\n", ret);
		action_unset(rule_p);
		return -1;
	}

	node_p = (struct vacl_user_node_t *)malloc_safe(sizeof(struct vacl_user_node_t));
	memset(node_p, 0x0, sizeof(struct vacl_user_node_t));
	memcpy(node_p, rule_p, sizeof(struct vacl_user_node_t));
	list_add_tail(&node_p->list, &rule_ll_head_user_ary_g[rule_p->order].list);
	*sub_order = rule_ll_head_user_ary_g[rule_p->order].count;
	rule_ll_head_user_ary_g[rule_p->order].count ++;
	rule_node_user_ary_p[ary_index_g] = node_p;

	vacl_conf_g.count_rules++;
	prepare_next_rule_node_ary_index();

	return 0;
}

static int
vacl_del_handler(int index)
{
	struct vacl_user_node_t *node_p;
	node_p = rule_node_user_ary_p[index];
	if (node_p) {
		list_del(&(node_p->list));
	
		ingress_field_unset(node_p);
		action_unset(node_p);
		rule_ll_head_user_ary_g[node_p->order].count--;
	}
	vacl_conf_g.count_rules--;

	free_safe(rule_node_user_ary_p[index]);
	rule_node_user_ary_p[index] = NULL;

	prepare_next_rule_node_ary_index();

	return 0;
}

static int
rule_exist_hworder_find(struct vacl_user_node_t **rule_p, unsigned short hw_order, int *sub_order)
{
	struct list_head *pos, *temp;
	int i;

	// iterate to check
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_ll_head_user_ary_g[i].count <= 0) continue;
		*sub_order = -1;
		list_for_each_safe(pos, temp, &(rule_ll_head_user_ary_g[i].list)) {
			*rule_p = list_entry(pos, struct vacl_user_node_t, list);
			(*sub_order)++;
			if (hw_order == (*rule_p)->hw_order) {
				dbprintf_vacl(LOG_DEBUG, "found existance ary_index=%d hw_order=%d order=%d sub_order=%d crc=0x%08X\n",
					 (*rule_p)->ary_index, (*rule_p)->hw_order, (*rule_p)->order, *sub_order, (*rule_p)->crc);
				return 1;
			}
		}
	}
	return 0;
}

static void
user_entry_free_all(void)
{
	int i;
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_node_user_ary_p[i] != NULL) {
			free_safe(rule_node_user_ary_p[i]);
			rule_node_user_ary_p[i] = NULL;
		}
	}
	return;
}

////////////////////////////////////////////////////////////////////
int
vacl_user_node_init(struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL) {
		dbprintf_vacl(LOG_ERR, "vacl init NULL pointer user node fatal error\n");
		return -1;
	}
	memset(rule_p, 0x0, sizeof(struct vacl_user_node_t));
	rule_p->valid = 1;
	rule_p->invert = 0;
	rule_p->hw_meter_table_entry = get_vacl_meter_entry_total();
	rule_p->hw_template_field_bit_range_vid = 0;
	rule_p->hw_template_field_bit_range_l4port = 0;
	rule_p->hw_template_field_bit_range_ip = 0;
	rule_p->hw_template_field_bit_range_pktlen = 0;
	rule_p->hw_template_field_bit_field_selector = 0;
	rule_p->ingress_range_ether_type_index = 0;
	rule_p->ingress_range_ether_type_number = 0;
	rule_p->template_index = VACL_DEFAULT_TEMPLATE_INDEX;

	return 0;
}

int
vacl_init_check(void)
{
	if (is_acl_init_g == 0)
		return -1;
	else
		return 0;
}

int
_vacl_del_all(void)
{
	int i;

	user_entry_free_all();
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		rule_node_user_ary_p[i] = NULL;
		rule_ll_head_user_ary_g[i].count = 0;
		INIT_LIST_HEAD(&rule_ll_head_user_ary_g[i].list);
	}
	ary_index_g = 0;
	vacl_conf_g.count_rules = 0;

	memset(ingress_range_vid_lower_ary_g, 0x0, INGRESS_RANGE_VID_INDEX_TOTAL*sizeof(short));
	memset(ingress_range_vid_upper_ary_g, 0x0, INGRESS_RANGE_VID_INDEX_TOTAL*sizeof(short));
	ingress_range_vid_type_g = 0x0;
	memset(ingress_range_vid_ref_count_ary, 0x0, INGRESS_RANGE_VID_INDEX_TOTAL*sizeof(short));

	memset(ingress_range_l4port_lower_ary_g, 0x0, INGRESS_RANGE_L4PORT_INDEX_TOTAL*sizeof(short));
	memset(ingress_range_l4port_upper_ary_g, 0x0, INGRESS_RANGE_L4PORT_INDEX_TOTAL*sizeof(short));
	ingress_range_l4port_type_g = 0x0;
	memset(ingress_range_l4port_ref_count_ary, 0x0, INGRESS_RANGE_L4PORT_INDEX_TOTAL*sizeof(short));

	memset(ingress_range_ip_lower_ary_g, 0x0, INGRESS_RANGE_IP_INDEX_TOTAL*sizeof(int));
	memset(ingress_range_ip_upper_ary_g, 0x0, INGRESS_RANGE_IP_INDEX_TOTAL*sizeof(int));
	memset(ingress_range_ip_type_ary_g, 0x0, INGRESS_RANGE_IP_INDEX_TOTAL*sizeof(short));
	memset(ingress_range_ip_ref_count_ary, 0x0, INGRESS_RANGE_IP_INDEX_TOTAL*sizeof(short));

	memset(ingress_range_pktlen_lower_ary_g, 0x0, INGRESS_RANGE_PKTLEN_INDEX_TOTAL*sizeof(short));
	memset(ingress_range_pktlen_upper_ary_g, 0x0, INGRESS_RANGE_PKTLEN_INDEX_TOTAL*sizeof(short));
	ingress_range_pktlen_invert_g = 0;
	memset(ingress_range_pktlen_ref_count_ary, 0x0, INGRESS_RANGE_PKTLEN_INDEX_TOTAL*sizeof(short));

	memset(field_selector_commit_ary_g, 0x0, VACL_FIELD_SELECTOR_TOTAL*sizeof(short));
	memset(field_selector_format_ary_g, 0x0, VACL_FIELD_SELECTOR_TOTAL*sizeof(short));
	memset(field_selector_offset_ary_g, 0x0, VACL_FIELD_SELECTOR_TOTAL*sizeof(short));
	memset(ingress_field_selector_ref_count_ary_g, 0x0, VACL_FIELD_SELECTOR_TOTAL*sizeof(short));
	field_selector_commit_ary_g[7]=1;

	ingress_range_ether_type_init();

	switch_hw_g.vacl_clear(); // clean all hw relative data and records

	// meter table clear first, then reset table entry
	memset(meter_ref_count_ary_g, 0x0, get_vacl_meter_entry_total()*sizeof(short));
	for (i=0; i<get_vacl_meter_entry_total(); i++) {
		meter_rate_ary_g[i] = ACT_METER_RATE_DEFAULT;
		meter_burst_ary_g[i] = ACT_METER_BUCKET_SIZE_DEFAULT;
	}
	meter_ifg_g = 0;

	// specific features
	vacl_conf_g.mode128 = VACL_DEFAULT_RULE_MODE;
	vacl_conf_g.count_rules_max = CHIP_MAX_NUM_OF_ACL_RULE_ENTRY;
	return 0;
}

int
vacl_del_all(void)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_del_all();
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_init(void)
{
	switch_hw_g.vacl_init();
	_vacl_del_all();
	vacl_port_enable_set(switch_get_all_logical_portmask());
	vacl_port_permit_set(switch_get_all_logical_portmask());
	is_acl_init_g = 1;
	meter_entry_reserve_g = VACL_METER_ENTRY_INIT_RESERVE;
	return 0;
}

int
vacl_init(void)
{
	fwk_mutex_create(&vacl_nonrg_api_mutex);

	return _vacl_init();
}

int
vacl_shutdown(void)
{

	fwk_mutex_destroy(&vacl_nonrg_api_mutex);

	return switch_hw_g.vacl_shutdown();
}

int
vacl_port_enable_set(unsigned int portmask)
{
	unsigned int port;
	dbprintf_vacl(LOG_NOTICE, "portmask=0x%x\n", portmask);
 	vacl_conf_g.portmask_enable = portmask;
	for (port = 0; port <= SWITCH_LOGICAL_PORT_TOTAL; port ++) {
		if ((1<<port) & switch_get_all_logical_portmask_without_cpuext())
			switch_hw_g.vacl_port_enable_set(port, (portmask>>port)&1);
	}
	return 0;
}

int
vacl_port_permit_set(unsigned int portmask)
{
	unsigned int port;
	dbprintf_vacl(LOG_NOTICE, "portmask=0x%x\n", portmask);
 	vacl_conf_g.portmask_permit = portmask;
	for (port = 0; port <= SWITCH_LOGICAL_PORT_TOTAL; port ++) {
		if ((1<<port) & switch_get_all_logical_portmask_without_cpuext())
			switch_hw_g.vacl_port_permit_set(port, (portmask>>port)&1);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////
int
_vacl_add(struct vacl_user_node_t *rule_p, int *sub_order)
{
	int ret=0;
	int i, _sub_order;
	struct vacl_user_node_t *node_p;

	ret = switch_hw_g.vacl_rule_evaluate(rule_p);
	if (ret != RET_OK)
		return -1;

	if ((rule_p->ingress_care_u.bit.ether_type_lower == 1) &&
	    (rule_p->ingress_care_u.bit.ether_type_upper == 1)) {
		rule_p->ingress_range_ether_type_number = 0;
		rule_p->ingress_range_ether_type_index = 0;
		ingress_range_half_down(rule_p, rule_p->ingress_ether_type_u.range.lower,
				rule_p->ingress_ether_type_u.range.upper, BITWISE_2BYTE);
		ret = 0;
		*sub_order = 0;
		for (i=0; (!ret) && (i<(rule_p->ingress_range_ether_type_number)); i++) {
			node_p = (struct vacl_user_node_t *)malloc_safe(sizeof(struct vacl_user_node_t));
			memset(node_p, 0x0, sizeof(struct vacl_user_node_t));
			memcpy(node_p, rule_p, sizeof(struct vacl_user_node_t));
			node_p->ingress_care_u.bit.ether_type_lower = 0;
			node_p->ingress_care_u.bit.ether_type_upper = 0;
			node_p->ingress_care_u.bit.ether_type_value = 1;
			node_p->ingress_care_u.bit.ether_type_mask = 1;
			node_p->ingress_ether_type_u.etype.value = ingress_range_ether_type_value_ary_g[i + rule_p->ingress_range_ether_type_index];
			node_p->ingress_ether_type_u.etype.mask  = ingress_range_ether_type_mask_ary_g[i + rule_p->ingress_range_ether_type_index];
			ret = vacl_add_handler(node_p, &_sub_order);
			if (*sub_order == 0) *sub_order = _sub_order;
		}
	}
	else
		ret = vacl_add_handler(rule_p, sub_order);

	return ret;
}

int
vacl_add(struct vacl_user_node_t *rule_p, int *sub_order)
{
	int ret=0;

	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_add(rule_p, sub_order);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);

	return ret;
}
////////////////////////////////////////////////////////////////////
int
_vacl_del_crc(unsigned int crc, int *hw_order)
{
	int sub_order;
	struct vacl_user_node_t *node_p;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		return 1;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty\n");
		return VACL_DEL_RET_EMPTY;
	}
	sub_order = -1;
	node_p = NULL;
	if (rule_exist_crc_find(&node_p, crc, &sub_order) == 0) {
		dbprintf_vacl(LOG_DEBUG, "This acl rule does not exist!\n");
		return VACL_DEL_RET_MISS;
	}
        *hw_order = node_p->hw_order;
	vacl_del_handler(node_p->ary_index);

	return 0;
}

int
vacl_del_crc(unsigned int crc, int *hw_order)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_del_crc(crc, hw_order);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_del_hworder(int hw_order)
{
	int sub_order;
	struct vacl_user_node_t *node_p;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		return 1;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty\n");
		return VACL_DEL_RET_EMPTY;
	}
	sub_order = -1;
	node_p = NULL;
	if (rule_exist_hworder_find(&node_p, hw_order, &sub_order) == 0) {
		dbprintf_vacl(LOG_DEBUG, "This acl rule does not exist!\n");
		return VACL_DEL_RET_MISS;
	}
	vacl_del_handler(node_p->ary_index);

	return 0;
}

int
vacl_del_hworder(int hw_order)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_del_hworder(hw_order);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_del(struct vacl_user_node_t *rule_p, int *hw_order)
{
	unsigned int crc, crc_accum = 0xFFFFFFFF;

	crc = crc_be_update(crc_accum, (char *)rule_p + VACL_USER_NODE_INTERNAL_VAR_SIZE, sizeof(struct vacl_user_node_t) - VACL_USER_NODE_INTERNAL_VAR_SIZE);
	return _vacl_del_crc(crc, hw_order);
}

int
vacl_del(struct vacl_user_node_t *rule_p, int *hw_order)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_del(rule_p, hw_order);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_del_order(int order, int *count)
{
	struct vacl_user_node_t *node_p;
	struct list_head *pos, *temp;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		return 1;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty!\n");
		return VACL_DEL_RET_EMPTY;
	}

	if (order >= VACL_ORDER_TOTAL) {
		dbprintf_vacl(LOG_DEBUG, "user define order %d exceeds %d limit!\n", order, VACL_ORDER_TOTAL);
		return 1;
	}

	if (rule_ll_head_user_ary_g[order].count <= 0) {
		dbprintf_vacl(LOG_DEBUG, "user define order %d no rule to delete!!\n", order);
		return 0;
	}
	*count = 0;
	list_for_each_safe(pos, temp, &(rule_ll_head_user_ary_g[order].list)) {
		node_p = list_entry(pos, struct vacl_user_node_t, list);
		vacl_del_handler(node_p->ary_index);
		(*count)++;
	}
	return 0;
}

int
vacl_del_order(int order, int *count)
{
	int ret=0;

	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_del_order(order, count);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);

	return ret;
}
////////////////////////////////////////////////////////////////////
void
_vacl_valid_all(unsigned int valid)
{
	int i;
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_node_user_ary_p[i] != NULL) {
			if (valid == 0)
				rule_node_user_ary_p[i]->valid = 0;
			else
				rule_node_user_ary_p[i]->valid = 1;
		}
	}
	return;
}

void
vacl_valid_all(unsigned int valid)
{
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	_vacl_valid_all(valid);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
}

int
_vacl_valid_crc(unsigned int crc, unsigned int valid)
{
	int sub_order;
	struct vacl_user_node_t *node_p;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		return 0;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty\n");
		return 0;
	}
	sub_order = -1;
	node_p = NULL;
	if (rule_exist_crc_find(&node_p, crc, &sub_order) == 0) {
		dbprintf_vacl(LOG_DEBUG, "This acl rule does not exist!\n");
		return 0;
	}
	if (valid == 0)
		node_p->valid = 0;
	else
		node_p->valid = 1;

	return 0;
}

int
vacl_valid_crc(unsigned int crc, unsigned int valid)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_valid_crc(crc, valid);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_valid_hworder(int hw_order, unsigned int valid)
{
	int sub_order;
	struct vacl_user_node_t *node_p;

	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		return 1;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty\n");
		return 0;
	}
	sub_order = -1;
	node_p = NULL;
	if (rule_exist_hworder_find(&node_p, hw_order, &sub_order) == 0) {
		dbprintf_vacl(LOG_DEBUG, "This acl rule does not exist!\n");
		return 0;
	}
	if (valid == 0)
		node_p->valid = 0;
	else
		node_p->valid = 1;

	return 0;
}

int
vacl_valid_hworder(int hw_order, unsigned int valid)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_valid_hworder(hw_order, valid);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

int
_vacl_is_enable(struct vacl_user_node_t *rule_p, int *disable_code)
{
	int sub_order;
	struct vacl_user_node_t *node_p;
	unsigned int crc, crc_accum = 0xFFFFFFFF;

	*disable_code = 1;
	if (vacl_init_check() != 0) {
		dbprintf_vacl(LOG_DEBUG, "No acl init!\n");
		*disable_code = 1;
		return RET_OK;
	}

	if (vacl_conf_g.count_rules <= 0) {
		dbprintf_vacl(LOG_DEBUG, "acl rule empty\n");
		*disable_code = 2;
		return RET_OK;
	}

	crc = crc_be_update(crc_accum, (char *)rule_p + \
			    VACL_USER_NODE_INTERNAL_VAR_SIZE, \
			    sizeof(struct vacl_user_node_t) - \
			    VACL_USER_NODE_INTERNAL_VAR_SIZE);
	node_p = NULL;
	sub_order = -1;
	if (RET_NO == rule_exist_crc_find(&node_p, crc, &sub_order)) {
		dbprintf_vacl(LOG_DEBUG, "acl rule NOT found in sw database\n");
		*disable_code = 3;
		return RET_OK;
	}
	// FIXME LIGHT to check valid bit & hw-commit
	*disable_code = 0;
	return RET_OK;
}

int
vacl_is_enable(struct vacl_user_node_t *rule_p, int *disable_code)
{
	int ret=0;

	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_is_enable(rule_p, disable_code);
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);

	return ret;
}
////////////////////////////////////////////////////////////////////
int
_vacl_commit(void)
{
	int i, vacl_hw_entry_id_max = 0, valid;
	struct list_head *pos, *temp;
	struct vacl_user_node_t *rule_p;

	if (vacl_conf_g.count_rules < 1)
		return 0;
	ingress_range_ether_type_init();
	vacl_hw_entry_id_max = VACL_HW_ENTRY_ID_MIN;
	// iterate to decide hw entry id
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_ll_head_user_ary_g[i].count <= 0) continue;
		list_for_each_safe(pos, temp, &(rule_ll_head_user_ary_g[i].list)) {
			rule_p = list_entry(pos, struct vacl_user_node_t, list);
			valid=1;
			for (; vacl_hw_entry_id_max<VACL_ORDER_TOTAL; vacl_hw_entry_id_max++) {
				switch_hw_g.vacl_rule_valid_get(vacl_hw_entry_id_max, &valid);
				if (valid==0) break;
			}
			if (valid) {
				dbprintf_vacl(LOG_ERR, "run out of acl rules(%d)!\n", vacl_hw_entry_id_max);
				return -1;
			}
			else {
				rule_p->hw_order = vacl_hw_entry_id_max;
				vacl_hw_entry_id_max++;
			}
		}
	}
	return 0;
}

int
vacl_commit(void)
{
	int ret=0;
	fwk_mutex_lock(&vacl_nonrg_api_mutex);
	ret=_vacl_commit();
	fwk_mutex_unlock(&vacl_nonrg_api_mutex);
	return ret;
}

////////////////////////////////////////////////////////////////////
