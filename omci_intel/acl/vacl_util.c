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
 * File    : vacl_util.c
 *
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <ctype.h>
#include "list.h"

#ifndef	ACLTEST
#include "util.h"
#else
#include "vacltest_util.h"
#endif

#include "switch.h"
#include "util_inet.h"
#include "conv.h"
#include "vacl.h"
#include "vacl_util.h"

#define UTIL_IPV6_TMP_BUFFER_LENGTH     (8)

/*action type string*/
/*
const char *action_str[] = {
	STR_DROP,
	STR_COPY,
	STR_REDIRECT,
	STR_MIRROR,
	STR_ACLTRAP,
	STR_METER,
	STR_LATCH,
	STR_IPRI,
	STR_DSCP,
	STR_DOT1P
};
*/

/*common string*/
const char *enable_ary_s[] = {
	STR_DISABLE,
	STR_ENABLE
};

/*common string*/
const char *valid_ary_s[] = {
	STR_INVALID,
	STR_VALID
};

const char *act_forward_ary_s[] = {
	STR_COPY,
	STR_REDIRECT,
	STR_MIRROR,
	STR_ACLTRAP,
};

const char *act_police_ary_s[] = {
	STR_POLICING,
	STR_ACLMIB,
};

const char *rule_revise_ary_s[] = {
	STR_NOTREVISE,
	STR_REVISE,
};

const char *ip_ary_s[] = {
	"UNUSED",
	"SIPV4",
	"DIPV4",
	"SIPV6",
	"DIPV6",
	"END",
};

char *
get_action_str_by_bit(int i)
{
	switch (i) {
	case VACL_ACT_FWD_DROP_BIT:
		return STR_DROP;
	case VACL_ACT_FWD_COPY_BIT:
		return STR_COPY;
	case VACL_ACT_FWD_REDIRECT_BIT:
		return STR_REDIRECT;
	case VACL_ACT_FWD_IGR_MIRROR_BIT:
		return STR_MIRROR;
	case VACL_ACT_FWD_TRAP_BIT:
		return STR_ACLTRAP;
	case VACL_ACT_FWD_PERMIT_BIT:
		return STR_PERMIT;
	case VACL_ACT_LOG_POLICING_BIT:
		return STR_METER;
	case VACL_ACT_INTR_LATCH_BIT:
		return STR_LATCH;
	case VACL_ACT_PRI_ACL_PRI_ASSIGN_BIT:
		return STR_IPRI;
	case VACL_ACT_PRI_DSCP_REMARK_BIT:
		return STR_DSCP;
	case VACL_ACT_PRI_1P_REMARK_BIT:
		return STR_DOT1P;
	case VACL_ACT_LOG_MIB_BIT:
		return STR_STATISTIC;
	case VACL_ACT_CVLAN_1P_REMARK_BIT:
		return "CVLAN "STR_DOT1P;
	case VACL_ACT_SVLAN_DSCP_REMARK_BIT:
		return "SVLAN "STR_DSCP;
	default:
		return "NULL";
	}
}

int
which_bigger(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

void
_int2Str(char* strBuffer,unsigned char* strIdx,unsigned char num)
{
    unsigned char quo,quo2,rem, rem2;

    quo = num /10;
    rem = num %10;

    if(quo)
    {
        quo2 = quo/10;
        if(quo2)
        {
            rem2 = quo%10;
            *(strBuffer+*strIdx) = '0' + quo2;
            *strIdx = *strIdx + 1;
            *(strBuffer+*strIdx) = '0' + rem2;
            *strIdx = *strIdx + 1;
        }
        else
        {
            *(strBuffer+*strIdx) = '0' + quo;
            *strIdx = *strIdx + 1;
        }
    }

    *(strBuffer+*strIdx) = '0' + rem;

    *strIdx = *strIdx + 1;

}

int
is2pow(int num)
{
	int flag=0;
	if(num==0) return 0;
	while(num) {
		if(flag && num&1)
			return 0;
		if(num&1) flag=1;
		num >>= 1;
	}
	return 1;
}

int
get_id_num(char *token)
{
	if (token == NULL) return -1;
	if (*token>='0' &&  *token<='9') return util_atoi(token);
	else return -1;
}

////////////////////////////////////
char *
vacl_util_inet_mactoa(const unsigned char *mac)
{
	static char str[6*sizeof "123"];

	if (NULL == mac) {
		sprintf(str,"NULL");
		return str;
	}

	sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return str;
} /* end of acl_util_mac2str */

/*IPv4 address to string*/
char *
vacl_util_inet_ntoa(unsigned int ina)
{
	static char buf[16];

	sprintf(buf, "%d.%d.%d.%d", ((ina>>24)&0xff), ((ina>>16)&0xff), ((ina>>8)&0xff), (ina&0xff));
	return (buf);
}

/*IPv6 tail 4 bytes to string*/
char *
vacl_util_inet6_ntoa_tail4(unsigned int ina)
{
	static char buf[16];

	sprintf(buf, "%02x:%02x:%02x:%02x",  ((ina>>24)&0xff), ((ina>>16)&0xff), ((ina>>8)&0xff), (ina&0xff));
	return (buf);
}

/*IPv6 address to string*/
char *
vacl_util_inet6_ntoa(const unsigned char *ipv6)
{
	static char buf[8*sizeof "FFFF:"];
	unsigned int  i;
	unsigned short  ipv6_ptr[UTIL_IPV6_TMP_BUFFER_LENGTH] = {0};

	for (i = 0; i < UTIL_IPV6_TMP_BUFFER_LENGTH ;i++) {
		ipv6_ptr[i] = ipv6[i*2+1];
		ipv6_ptr[i] |=  ipv6[i*2] << 8;
	}

	sprintf(buf, "%x:%x:%x:%x:%x:%x:%x:%x", ipv6_ptr[0], ipv6_ptr[1], ipv6_ptr[2], ipv6_ptr[3], ipv6_ptr[4], ipv6_ptr[5], ipv6_ptr[6], ipv6_ptr[7]);
	return (buf);
}

int
vacl_util_mcast_bw_to_meter(unsigned int mcast_bandwidth, unsigned int *meter_rate)
{
	unsigned int mcast_bw;
	mcast_bw = mcast_bandwidth<<3; // byte per second * 8 bits
	if (mcast_bw == 0 || mcast_bw > 0x3fffe000)
		*meter_rate = ACT_METER_RATE_MAX;
	else if (mcast_bw < 0x2000)  // < 8 kbps
			*meter_rate = 0;
		else
			*meter_rate = (mcast_bw>>13)<<3;
	return 0;
}

unsigned int
vacl_util_portidx_mask_max(void)
{
	int i;
	unsigned int portmask_with_cpu;
	portmask_with_cpu = ((~switch_get_cpu_logical_portmask()) & switch_get_all_logical_portmask()) | (1<<switch_get_cpu_logical_portid(0));
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		portmask_with_cpu >>= 1;
		if (portmask_with_cpu == 0)
			break;
	}
	return (i+1);
}

////////////////////////////////////
int
vacl_ingress_dmac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask)
{
	int ret = 0;

	if (rule_p == NULL)
		return -1;
	if ((ret = conv_str_mac_to_mem((char *)(rule_p->ingress_dstmac_value), addr)) != 0) {
		dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
		return ret;
	}
	if ((ret = conv_str_mac_to_mem((char *)(rule_p->ingress_dstmac_mask), mask)) != 0) {
		dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
		return ret;
	}

	rule_p->ingress_care_u.bit.dstmac_value = 1;
	rule_p->ingress_care_u.bit.dstmac_mask = 1;
	return 0;
}

int
vacl_ingress_smac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask)
{
	int ret = 0;

	if (rule_p == NULL)
		return -1;

	if ((ret = conv_str_mac_to_mem((char *)(rule_p->ingress_srcmac_value), addr)) != 0) {
		dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
		return ret;
	}
	if ((ret = conv_str_mac_to_mem((char *)(rule_p->ingress_srcmac_mask), mask)) != 0) {
		dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
		return ret;
	}
	rule_p->ingress_care_u.bit.srcmac_value = 1;
	rule_p->ingress_care_u.bit.srcmac_mask = 1;
	return 0;
}

int
vacl_ingress_dipv4_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask)
{
	if (rule_p == NULL)
		return -1;

	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_dipv4_u.addr.value.s_addr), addr);
	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_dipv4_u.addr.mask.s_addr), mask);

	rule_p->ingress_care_u.bit.dipv4_value = 1;
	rule_p->ingress_care_u.bit.dipv4_mask = 1;
	return 0;
}

int
vacl_ingress_sipv4_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask)
{
	if (rule_p == NULL)
		return -1;

	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_sipv4_u.addr.value.s_addr), addr);
	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_sipv4_u.addr.mask.s_addr), mask);

	rule_p->ingress_care_u.bit.sipv4_value = 1;
	rule_p->ingress_care_u.bit.sipv4_mask = 1;
	return 0;
}

int
vacl_ingress_dipv4_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub)
{
	if (rule_p == NULL)
		return -1;

	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_dipv4_u.range.lower), lb);
	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_dipv4_u.range.upper), ub);

	rule_p->ingress_care_u.bit.dipv4_lower = 1;
	rule_p->ingress_care_u.bit.dipv4_upper = 1;

	return 0;
}

int
vacl_ingress_sipv4_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub)
{
	if (rule_p == NULL)
		return -1;

	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_sipv4_u.range.lower), lb);
	conv_str_ipv4_to_mem((char *)&(rule_p->ingress_sipv4_u.range.upper), ub);

	rule_p->ingress_care_u.bit.sipv4_lower = 1;
	rule_p->ingress_care_u.bit.sipv4_upper = 1;

	return 0;
}

int
vacl_ingress_sipv6_addr_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask)
{
	if (rule_p == NULL)
		return -1;

	util_inet_pton(AF_INET6, addr, rule_p->ingress_sipv6_u.addr.value.s6_addr);
	util_inet_pton(AF_INET6, mask, rule_p->ingress_sipv6_u.addr.mask.s6_addr);

	rule_p->ingress_care_u.bit.sipv6_value = 1;
	rule_p->ingress_care_u.bit.sipv6_mask = 1;
	return 0;
}

int
vacl_ingress_dipv6_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub)
{
	if (rule_p == NULL)
		return -1;

	util_inet_pton(AF_INET6, lb, rule_p->ingress_dipv6_u.range.lower.s6_addr);
	util_inet_pton(AF_INET6, ub, rule_p->ingress_dipv6_u.range.upper.s6_addr);

	rule_p->ingress_care_u.bit.dipv6_lower = 1;
	rule_p->ingress_care_u.bit.dipv6_upper = 1;

	return 0;
}

int
vacl_ingress_sipv6_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub)
{
	if (rule_p == NULL)
		return -1;

	util_inet_pton(AF_INET6, lb, rule_p->ingress_sipv6_u.range.lower.s6_addr);
	util_inet_pton(AF_INET6, ub, rule_p->ingress_sipv6_u.range.upper.s6_addr);

	rule_p->ingress_care_u.bit.sipv6_lower = 1;
	rule_p->ingress_care_u.bit.sipv6_upper = 1;

	return 0;
}

char *
vacl_ingress_care_tag_str_get(enum vacl_ingress_care_tag_e tag)
{
	static char name[20];
	bzero(name, 20);
	switch(tag) {
        case VACL_INGRESS_CARE_TAG_PPPOE: strcpy(name,"PPPoE");break;
        case VACL_INGRESS_CARE_TAG_CTAG: strcpy(name,"CTag");break;
        case VACL_INGRESS_CARE_TAG_STAG: strcpy(name,"STag");break;
        case VACL_INGRESS_CARE_TAG_IPV4: strcpy(name,"IPv4");break;
        case VACL_INGRESS_CARE_TAG_IPV6: strcpy(name,"IPv6");break;
        case VACL_INGRESS_CARE_TAG_TCP: strcpy(name,"TCP");break;
        case VACL_INGRESS_CARE_TAG_UDP: strcpy(name,"UDP");break;
        default: strcpy(name,"Unknown");break;
	}
	return name;
}

int
vacl_util_multicast_node_compose(struct vacl_user_node_t *rule_p, unsigned int portmask, unsigned int meter_rate, int flag_ipv6)
{
	if (rule_p == NULL) {
		dbprintf_vacl(LOG_ERR, "null pointer\n");
		return -1;
	}
	memset(rule_p, 0, sizeof(struct vacl_user_node_t));
	vacl_user_node_init(rule_p);
	if (!flag_ipv6) {
		vacl_ingress_dmac_str_set(rule_p, "01:00:5e:00:00:00", "ff:ff:ff:80:00:00");
	}
	else {
		vacl_ingress_dmac_str_set(rule_p, "33:33:00:00:00:00", "ff:ff:00:00:00:00");
	}
	rule_p->act_type |= VACL_ACT_LOG_POLICING_MASK;
	rule_p->act_meter_rate = meter_rate;
	rule_p->act_meter_bucket_size = ACT_METER_BUCKET_SIZE_DEFAULT;
	rule_p->act_meter_ifg = ACT_METER_IFG_DEFAULT;
	rule_p->hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE; //means auto assign meter entry
	rule_p->ingress_active_portmask = portmask;

	int fd = util_dbprintf_get_console_fd();
	vacl_user_dump(fd, rule_p);
	return 0;
}

int
vacl_ingress_dscp_ipv4_value_set(struct vacl_user_node_t *rule_p, int value)
{
	if (rule_p == NULL)
		return -1;
	rule_p->ingress_ipv4_dscp_u.value = value;
	rule_p->ingress_care_u.bit.ipv4_dscp_value = 1;
	return 0;
}

////////////////////////////////////////////////

static int
print_ingress_care_tag(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	util_fdprintf(fd, "Care Tags: ");
	if (rule_p->ingress_care_tag_ctag_value && rule_p->ingress_care_tag_ctag_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_CTAG));

	if (rule_p->ingress_care_tag_ipv4_value && rule_p->ingress_care_tag_ipv4_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_IPV4));

	if (rule_p->ingress_care_tag_ipv6_value && rule_p->ingress_care_tag_ipv6_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_IPV6));

	if (rule_p->ingress_care_tag_pppoe_value && rule_p->ingress_care_tag_pppoe_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_PPPOE));

	if (rule_p->ingress_care_tag_stag_value && rule_p->ingress_care_tag_stag_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_STAG));

	if (rule_p->ingress_care_tag_tcp_value && rule_p->ingress_care_tag_tcp_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_TCP));

	if (rule_p->ingress_care_tag_udp_value && rule_p->ingress_care_tag_udp_mask)
		util_fdprintf(fd, "%s ", vacl_ingress_care_tag_str_get(VACL_INGRESS_CARE_TAG_UDP));

	util_fdprintf(fd, "\n");

	return RET_OK;
}

static int
print_act_type(int fd, struct vacl_user_node_t *rule_p)
{
	int i;
	unsigned long long act_type, mask;

	if (rule_p == NULL)
		return -1;

	act_type = rule_p->act_type;
	mask=1ULL;
	for (i=0; i<VACL_ACT_TOTAL_BIT && act_type!=0; i++) {
		if (act_type & mask) {
			util_fdprintf(fd, "Action: %s", get_action_str_by_bit(i));
			if ((act_type & VACL_ACT_PRI_ACL_PRI_ASSIGN_MASK) ||
			    (act_type & VACL_ACT_PRI_DSCP_REMARK_MASK) ||
			    (act_type & VACL_ACT_PRI_1P_REMARK_MASK))
				util_fdprintf(fd, " 0x%x", rule_p->act_pri_data);
			else if (act_type & VACL_ACT_LOG_MIB_MASK)
				util_fdprintf(fd, " 0x%x", rule_p->act_statistic_index);
			else if (act_type & VACL_ACT_CVLAN_1P_REMARK_MASK)
				util_fdprintf(fd, " 0x%x", rule_p->act_cvlan_1p_remark);
			else if (act_type & VACL_ACT_SVLAN_DSCP_REMARK_MASK)
				util_fdprintf(fd, " 0x%x", rule_p->act_svlan_dscp_remark);
			util_fdprintf(fd, "\n");
			if ((act_type & VACL_ACT_FWD_COPY_MASK) ||
			    (act_type & VACL_ACT_FWD_REDIRECT_MASK) ||
			    (act_type & VACL_ACT_FWD_IGR_MIRROR_MASK)) {
				util_fdprintf(fd, "	Forward port mask: 0x%x(logical)\n", rule_p->act_forward_portmask);
			}
			else if (act_type & VACL_ACT_LOG_POLICING_MASK) {
				if (rule_p->hw_meter_table_entry == ACT_METER_INDEX_AUTO_W_SHARE) {
					util_fdprintf(fd, "	Meter table entry id			: auto assign, share same meter\n");
				}
				else if (rule_p->hw_meter_table_entry == ACT_METER_INDEX_AUTO_WO_SHARE) {
					util_fdprintf(fd, "	Meter table entry id			: auto assign, NOT share same meter\n");
				}
				else {
					util_fdprintf(fd, "	Meter table entry id			: %d\n", rule_p->hw_meter_table_entry);
				}
				util_fdprintf(fd, "	Meter burst size (unit byte), <0~65535>	: %d\n", rule_p->act_meter_bucket_size);
				util_fdprintf(fd, "	Meter rate (unit 1Kbps), <1~1048568>	: %d\n", rule_p->act_meter_rate);
				util_fdprintf(fd, "	Meter ifg include			: %d\n", rule_p->act_meter_ifg);
			}
			act_type &= ~mask;
		}
		mask<<=1;
	}
	return 0;
}

static int
print_ingress_mac_addr(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.dstmac_value == 1)
		util_fdprintf(fd, "dstmac              = %s\n", vacl_util_inet_mactoa(rule_p->ingress_dstmac_value));
	if (rule_p->ingress_care_u.bit.dstmac_mask == 1)
		util_fdprintf(fd, "dstmac_mask         = %s\n", vacl_util_inet_mactoa(rule_p->ingress_dstmac_mask));

	if (rule_p->ingress_care_u.bit.srcmac_value == 1)
		util_fdprintf(fd, "srcmac              = %s\n", vacl_util_inet_mactoa(rule_p->ingress_srcmac_value));
	if (rule_p->ingress_care_u.bit.srcmac_mask == 1)
		util_fdprintf(fd, "srcmac_mask         = %s\n", vacl_util_inet_mactoa(rule_p->ingress_srcmac_mask));

	return 0;
}

static int
print_ingress_ipv4_addr(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.dipv4_value == 1)
		util_fdprintf(fd, "dipv4 value         = %s\n", inet_ntoa(rule_p->ingress_dipv4_u.addr.value));
	if (rule_p->ingress_care_u.bit.dipv4_mask == 1)
		util_fdprintf(fd, "dipv4 mask          = %s\n", inet_ntoa(rule_p->ingress_dipv4_u.addr.mask));

	if (rule_p->ingress_care_u.bit.sipv4_value == 1)
		util_fdprintf(fd, "sipv4 value         = %s\n", inet_ntoa(rule_p->ingress_sipv4_u.addr.value));
	if (rule_p->ingress_care_u.bit.sipv4_mask == 1)
		util_fdprintf(fd, "sipv4 mask          = %s\n", inet_ntoa(rule_p->ingress_sipv4_u.addr.mask));

	return 0;
}

static int
print_ingress_ipv6_addr(int fd, struct vacl_user_node_t *rule_p)
{
	char str[128];

	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.dipv6_value == 1)
		util_fdprintf(fd, "dipv6 value         = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_dipv6_u.addr.value.s6_addr, str, 128));
	if (rule_p->ingress_care_u.bit.dipv6_mask == 1)
		util_fdprintf(fd, "dipv6 mask          = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_dipv6_u.addr.mask.s6_addr, str, 128));

	if (rule_p->ingress_care_u.bit.sipv6_value == 1)
		util_fdprintf(fd, "sipv6 value         = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_sipv6_u.addr.value.s6_addr, str, 128));
	if (rule_p->ingress_care_u.bit.sipv6_mask == 1)
		util_fdprintf(fd, "sipv6 mask          = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_sipv6_u.addr.mask.s6_addr, str, 128));

	return 0;
}

static int
print_ingress_stag(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if ((rule_p->ingress_care_u.bit.stag_value == 1) &&
	    (rule_p->ingress_care_u.bit.stag_mask == 1)) {
	    if ((rule_p->ingress_stag_u.vid.mask) & 0x0fff) {
		util_fdprintf(fd, "stag vid/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_stag_u.vid.value) & 0x0fff,
			      (rule_p->ingress_stag_u.vid.mask) & 0x0fff);
	    }
	    if ((rule_p->ingress_stag_dei_mask) & 0x01) {
		util_fdprintf(fd, "stag cfi/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_stag_dei_value & 0x01),
			      (rule_p->ingress_stag_dei_mask & 0x01));
	    }
	    if ((rule_p->ingress_stag_pri_mask) & 0x07) {
		util_fdprintf(fd, "stag pri/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_stag_pri_value & 0x07),
			      (rule_p->ingress_stag_pri_mask & 0x07));
	    }
	}
	return 0;
}

static int
print_ingress_ctag(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if ((rule_p->ingress_care_u.bit.ctag_value == 1) &&
	    (rule_p->ingress_care_u.bit.ctag_mask == 1)) {
	    if ((rule_p->ingress_ctag_u.vid.mask) & 0x0fff) {
		util_fdprintf(fd, "ctag vid/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_ctag_u.vid.value) & 0x0fff,
			      (rule_p->ingress_ctag_u.vid.mask) & 0x0fff);
	    }
	    if ((rule_p->ingress_ctag_cfi_mask) & 0x01) {
		util_fdprintf(fd, "ctag cfi/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_ctag_cfi_value & 0x01),
			      (rule_p->ingress_ctag_cfi_mask & 0x01));
	    }
	    if ((rule_p->ingress_ctag_pri_mask) & 0x07) {
		util_fdprintf(fd, "ctag pri/mask       = 0x%x/0x%x\n",
			      (rule_p->ingress_ctag_pri_value & 0x07),
			      (rule_p->ingress_ctag_pri_mask & 0x07));
	    }
	}
	return 0;
}

static int
print_ingress_range_vid(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.ctag_vid_lower == 1) {
		util_fdprintf(fd, "ctag vid lb         = %d\n", rule_p->ingress_ctag_u.vid_range.lower);
	}
	if (rule_p->ingress_care_u.bit.ctag_vid_upper == 1) {
		util_fdprintf(fd, "ctag vid ub         = %d\n", rule_p->ingress_ctag_u.vid_range.upper);
	}
	if (rule_p->ingress_care_u.bit.stag_vid_lower == 1) {
		util_fdprintf(fd, "stag vid lb         = %d\n", rule_p->ingress_stag_u.vid_range.lower);
	}
	if (rule_p->ingress_care_u.bit.stag_vid_upper == 1) {
		util_fdprintf(fd, "stag vid ub         = %d\n", rule_p->ingress_stag_u.vid_range.upper);
	}
#if 0
	if (i) {
		util_fdprintf(fd, "-------------------------\n");
		util_fdprintf(fd, "   lb\tub\tc/s-tag\n");
		for (i=0; i<INGRESS_RANGE_VID_INDEX_TOTAL; i++) {
			if (range_vid_lower_ary_g[i] > 0)
				util_fdprintf(fd, "%02d %d\t%d\t%d\n",
				      i, range_vid_lower_ary_g[i], range_vid_upper_ary_g[i], (range_vid_type_g>>i)&1);
		}
		util_fdprintf(fd, "-------------------------\n");
	}
#endif
	return 0;
}

static int
print_ingress_range_l4port(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.l4dport_lower == 1) {
		util_fdprintf(fd, "l4 dst-port lb      = %d\n", rule_p->ingress_l4dport_u.range.lower);
	}
	if (rule_p->ingress_care_u.bit.l4dport_upper == 1) {
		util_fdprintf(fd, "l4 dst-port ub      = %d\n", rule_p->ingress_l4dport_u.range.upper);
	}
	if (rule_p->ingress_care_u.bit.l4sport_lower == 1) {
		util_fdprintf(fd, "l4 src-port lb      = %d\n", rule_p->ingress_l4sport_u.range.lower);
	}
	if (rule_p->ingress_care_u.bit.l4sport_upper == 1) {
		util_fdprintf(fd, "l4 src-port ub      = %d\n", rule_p->ingress_l4sport_u.range.upper);
	}
#if 0
	if (i) {
		util_fdprintf(fd, "-------------------------\n");
		util_fdprintf(fd, "   lb\tub\tdst/src\n");
		for (i=0; i<INGRESS_RANGE_L4PORT_INDEX_TOTAL; i++) {
			if (range_l4port_lower_ary_g[i] > 0)
				util_fdprintf(fd, "%02d %d\t%d\t%d\n",
				      i, range_l4port_lower_ary_g[i], range_l4port_upper_ary_g[i], (range_l4port_type_g>>i)&1);
		}
		util_fdprintf(fd, "-------------------------\n");
	}
#endif
	return 0;
}

static int
print_ingress_range_ip(int fd, struct vacl_user_node_t *rule_p)
{
	char str[128];

	if (rule_p == NULL)
		return -1;

	memset(str, 0x0, 128);
	if (rule_p->ingress_care_u.bit.dipv4_lower == 1) {
		util_fdprintf(fd, "dip lb              = %s\n", inet_ntoa(rule_p->ingress_dipv4_u.range.lower));
	}
	if (rule_p->ingress_care_u.bit.dipv4_upper == 1) {
		util_fdprintf(fd, "dip ub              = %s\n", inet_ntoa(rule_p->ingress_dipv4_u.range.upper));
	}

	if (rule_p->ingress_care_u.bit.sipv4_lower == 1) {
		util_fdprintf(fd, "sip lb              = %s\n", inet_ntoa(rule_p->ingress_sipv4_u.range.lower));
	}
	if (rule_p->ingress_care_u.bit.sipv4_upper == 1) {
		util_fdprintf(fd, "sip ub              = %s\n", inet_ntoa(rule_p->ingress_sipv4_u.range.upper));
	}

	if (rule_p->ingress_care_u.bit.dipv6_lower == 1) {
		util_fdprintf(fd, "dip6 lb             = %s\n", util_inet_ntop(AF_INET6, rule_p->ingress_dipv6_u.range.lower.s6_addr, str, 128));
	}
	if (rule_p->ingress_care_u.bit.dipv6_upper == 1) {
		util_fdprintf(fd, "dip6 ub             = %s\n", util_inet_ntop(AF_INET6, rule_p->ingress_dipv6_u.range.upper.s6_addr, str, 128));
	}

	if (rule_p->ingress_care_u.bit.sipv6_lower == 1) {
		util_fdprintf(fd, "sip6 lb             = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_sipv6_u.range.lower.s6_addr, str, 128));
	}
	if (rule_p->ingress_care_u.bit.sipv6_upper == 1) {
		util_fdprintf(fd, "sip6 ub             = %s\n", util_inet_ntop(AF_INET6,rule_p->ingress_sipv6_u.range.upper.s6_addr, str, 128));
	}
#if 0
	if (i) {
		util_fdprintf(fd, "--------------------------------------------------\n");
		util_fdprintf(fd, "   lb              ub              sip/dip\n");
		for (i=0; i<INGRESS_RANGE_IP_INDEX_TOTAL; i++) {
			if ((range_ip_type_ary_g[i] == INGRESS_RANGE_IP_SIPV4) || (range_ip_type_ary_g[i] == INGRESS_RANGE_IP_DIPV4)) {
				util_fdprintf(fd, "%-2d %-15s ", i, vacl_util_inet_ntoa(range_ip_lower_ary_g[i]));
				util_fdprintf(fd, "%-15s %s\n", vacl_util_inet_ntoa(range_ip_upper_ary_g[i]), ip_ary_s[range_ip_type_ary_g[i]]);
			}
			else if ((range_ip_type_ary_g[i] == INGRESS_RANGE_IP_SIPV6) || (range_ip_type_ary_g[i] == INGRESS_RANGE_IP_DIPV6)) {
				util_fdprintf(fd, "%-2d %-15s ", i, vacl_util_inet6_ntoa_tail4(range_ip_lower_ary_g[i]));
				util_fdprintf(fd, "%-15s %s\n", vacl_util_inet6_ntoa_tail4(range_ip_upper_ary_g[i]), ip_ary_s[range_ip_type_ary_g[i]]);
			}
		}
		util_fdprintf(fd, "--------------------------------------------------\n");
	}
#endif
	return 0;
}

static int
print_ingress_range_pktlen(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.pktlen_lower == 1) {
		util_fdprintf(fd, "pkt len lb          = %d\n", rule_p->ingress_pktlen_range.lower);
	}
	if (rule_p->ingress_care_u.bit.pktlen_upper == 1) {
		util_fdprintf(fd, "pkt len ub          = %d\n", rule_p->ingress_pktlen_range.upper);
	}
	if (rule_p->ingress_care_u.bit.pktlen_invert == 1) {
		util_fdprintf(fd, "invert              = %d\n", rule_p->ingress_pktlen_invert);
	}
#if 0
	if (i) {
		util_fdprintf(fd, "-------------------------\n");
		util_fdprintf(fd, "   lb\tub\tinvert\n");
		for (i=0; i<INGRESS_RANGE_PKTLEN_INDEX_TOTAL; i++) {
			if (range_pktlen_lower_ary_g[i] > 0)
				util_fdprintf(fd, "%02d %d\t%d\t%d\n",
				      i, range_pktlen_lower_ary_g[i], range_pktlen_upper_ary_g[i], (range_pktlen_invert_g>>i)&1);
		}
		util_fdprintf(fd, "-------------------------\n");
	}
#endif
	return 0;
}

static int
print_ingress_range_ether_type(int fd, struct vacl_user_node_t *rule_p)
{
	int i;
	if (rule_p == NULL)
		return -1;

	if ((rule_p->ingress_care_u.bit.ether_type_lower == 1) &&
	    (rule_p->ingress_care_u.bit.ether_type_upper == 1)) {
		util_fdprintf(fd, "ether type lb       = 0x%04x(%d)\n",
			      rule_p->ingress_ether_type_u.range.lower,
			      rule_p->ingress_ether_type_u.range.lower);

		util_fdprintf(fd, "ether type ub       = 0x%04x(%d)\n",
			      rule_p->ingress_ether_type_u.range.upper,
			      rule_p->ingress_ether_type_u.range.upper);

		if (rule_p->ingress_range_ether_type_number) {
			util_fdprintf(fd, "-------------------------\n");
			util_fdprintf(fd, "hw-id\tvalue\tmask\n");
			for (i=0; i<rule_p->ingress_range_ether_type_number; i++) {
				util_fdprintf(fd, " %02d\t0x%04x\t0x%04x\n",
					      ingress_range_ether_type_hwid_ary_g[i + rule_p->ingress_range_ether_type_index],
					      ingress_range_ether_type_value_ary_g[i + rule_p->ingress_range_ether_type_index],
					      ingress_range_ether_type_mask_ary_g[i + rule_p->ingress_range_ether_type_index]);
			}
			util_fdprintf(fd, "-------------------------\n");
		}
	}
	return 0;
}

static int
print_ingress_ether_type(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.ether_type_value == 1) {
		util_fdprintf(fd, "ether_type          = 0x%04x\n",
			      rule_p->ingress_ether_type_u.etype.value);
	}
	if (rule_p->ingress_care_u.bit.ether_type_mask == 1) {
		util_fdprintf(fd, "ether_type_mask     = 0x%04x\n",
			      rule_p->ingress_ether_type_u.etype.mask);
	}

	return 0;
}

static int
print_ingress_gemport_llid(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.gem_llid_value == 1) {
		util_fdprintf(fd, "gemport_llid        = 0x%04x\n", rule_p->ingress_gem_llid_value);
	}
	if (rule_p->ingress_care_u.bit.gem_llid_mask == 1) {
		util_fdprintf(fd, "gemport_llid_mask   = 0x%04x\n", rule_p->ingress_gem_llid_mask);
	}

	return 0;
}

static int
print_ingress_protocol_value(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.protocol_value == 1) {
		util_fdprintf(fd, "protocol value/mask = 0x%x(%d)/0x%x\n",
			      rule_p->ingress_protocol_value,
			      rule_p->ingress_protocol_value,
			      rule_p->ingress_protocol_value_mask);
	}

	return 0;
}

static int
print_ingress_dscp_ipv4_value(int fd, struct vacl_user_node_t *rule_p)
{
	if (rule_p == NULL)
		return -1;

	if (rule_p->ingress_care_u.bit.ipv4_dscp_value == 1) {
		util_fdprintf(fd, "ipv4 DSCP value = 0x%x(%d)\n",
			      rule_p->ingress_ipv4_dscp_u.value,
			      rule_p->ingress_ipv4_dscp_u.value);
	}

	return 0;
}

void
vacl_user_dump(int fd, struct vacl_user_node_t *rule_p)
{
	util_fdprintf(fd, "hw_order:%d, order:%d, crc:0x%08x, valid:%d, invert:%d\n", rule_p->hw_order, rule_p->order, rule_p->crc, rule_p->valid, rule_p->invert);
	util_fdprintf(fd, "logical ingress active portmask: 0x%02x\n", rule_p->ingress_active_portmask);
	print_ingress_care_tag(fd, rule_p);
	print_act_type(fd, rule_p);
	util_fdprintf(fd, "template id: %d", rule_p->template_index);
	if (rule_p->template_field_use)
		util_fdprintf(fd, ", field_use: 0x%02X", rule_p->template_field_use);
	util_fdprintf(fd, "\n");
	print_ingress_mac_addr(fd, rule_p);
	print_ingress_ipv4_addr(fd, rule_p);
	print_ingress_ipv6_addr(fd, rule_p);
	print_ingress_stag(fd, rule_p);
	print_ingress_ctag(fd, rule_p);
	print_ingress_range_vid(fd, rule_p);
	print_ingress_range_l4port(fd, rule_p);
	print_ingress_range_ip(fd, rule_p);
	print_ingress_range_pktlen(fd, rule_p);
	print_ingress_range_ether_type(fd, rule_p);
	print_ingress_ether_type(fd, rule_p);
	print_ingress_gemport_llid(fd, rule_p);
	print_ingress_protocol_value(fd, rule_p);
	print_ingress_dscp_ipv4_value(fd, rule_p);

	return;
}

int
vacl_dump(int fd, int hw_order, int order)
{
	struct list_head *pos, *temp;
	int i;

	if (!is_acl_init_g) {
		is_acl_init_g = 1;
		_vacl_init();
		return 0;
	}
	// iterate to dump
	for (i=0; i<VACL_ORDER_TOTAL; i++) {
		if (rule_ll_head_user_ary_g[i].count <= 0) continue;
		list_for_each_safe(pos, temp, &(rule_ll_head_user_ary_g[i].list)) {
			struct vacl_user_node_t *rule_p = list_entry(pos, struct vacl_user_node_t, list);
			if (order >= 0 && order == rule_p->order)
				goto dump_it;
			else if (hw_order >= 0 && hw_order == rule_p->hw_order)
				goto dump_it;
			else if (hw_order == -1 && order == -1)
				goto dump_it;
			else
				continue;
dump_it:
			util_fdprintf(fd, "##############################################\n");
			dbprintf_vacl(LOG_DEBUG, "ary_index(%d/%d)\n", rule_p->ary_index, vacl_conf_g.count_rules-1);
			vacl_user_dump(fd, rule_p);
		}
	}
	return 0;
}

int
vacl_dump_portmask(int fd)
{
	util_fdprintf(fd, "all_logical_portmask_without_cpuext=0x%x\n", switch_get_all_logical_portmask_without_cpuext());
	util_fdprintf(fd, "all_logical_portmask =0x%x\n", switch_get_all_logical_portmask());
	util_fdprintf(fd, "wifi_logical_portmask=0x%x\n", switch_get_wifi_logical_portmask());
	util_fdprintf(fd, "cpu_logical_portmask =0x%x\n", switch_get_cpu_logical_portmask());
	util_fdprintf(fd, "uni_logical_portmask =0x%x\n", switch_get_uni_logical_portmask());
	util_fdprintf(fd, "wan_logical_portmask =0x%x\n", switch_get_wan_logical_portmask());
	util_fdprintf(fd, "~cpu&all=0x%x\n", ~switch_get_cpu_logical_portmask()&switch_get_all_logical_portmask());
	return 0;
}

int
vacl_dump_portidx(int fd)
{
	int i;
	util_fdprintf(fd, "wan_logical_portid=%d\n", switch_get_wan_logical_portid());
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (0<=switch_get_uni_logical_portid(i))
			util_fdprintf(fd, "uni_logical_portid[%d]=%d\n", i,switch_get_uni_logical_portid(i));
	}
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (0<=switch_get_cpu_logical_portid(i))
			util_fdprintf(fd, "cpu_logical_portid[%d]=%d\n", i,switch_get_cpu_logical_portid(i));
	}
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (0<=switch_get_wifi_logical_portid(i))
			util_fdprintf(fd, "wifi_logical_portid[%d]=%d\n", i,switch_get_wifi_logical_portid(i));
	}
	util_fdprintf(fd, "portidx_mask_max=%d\n", vacl_util_portidx_mask_max());
	return 0;
}
