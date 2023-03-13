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
 * Module  : cli
 * File    : cli_acl.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stdio.h"
#include "list.h"
#include "conv.h"
#include "util.h"
#include "util_inet.h"
#include "cli.h"
#include "meinfo.h"
#include "er_group.h"
#include "vacl.h"
#include "vacl_util.h"
#include "revision.h"
#include "switch.h"
#include "clitask.h"


#define VACL_CLI_VERSION "v5.4.0"

char *vacl_version_str_tag=TAG_REVISION;
char *vacl_version_str_time=__TIME__;
char *vacl_version_str_date=__DATE__;


static inline int cli_act_key_share_meter(char *key)
{
	if (key==NULL)
		return 0;
	return (strcmp(key, VACL_ACT_QOS_SHARE_METER_STR)==0 || strcmp(key, VACL_ACT_QOS_SHARE_METER_S1_STR)==0);
}

static inline int cli_act_key_fwd_port(char *key)
{
	if (key==NULL)
		return 0;
	return (strcmp(key, VACL_ACT_FWD_PORT_STR)==0 || strcmp(key, VACL_ACT_FWD_PORT_S1_STR)==0);
}

static inline int cli_infld_key_care_tag(char *key)
{
	if (key==NULL)
		return 0;
	return (strcmp(key, VACL_CLI_CARE_TAG)==0 || strcmp(key, VACL_CLI_CARE_TAG_S1)==0 || strcmp(key, VACL_CLI_CARE_TAG_S2)==0 || strcmp(key, VACL_CLI_CARE_TAG_S3)==0);
}

static char *
omci_vacl_cli_version_str(void)
{
	static char version_str[128];
	snprintf(version_str, 128, "%s (tag %s, %s, %s)",
		 VACL_CLI_VERSION, TAG_REVISION, __TIME__, __DATE__);
	return version_str;
}

void
cli_vacl_version(int fd)
{
	util_fdprintf(fd, "vacl %s\n", omci_vacl_cli_version_str());
}

int
cli_vacl_util_init(void)
{
	struct clitask_client_info_t *client_info = clitask_get_thread_private();
	if (client_info == NULL)
		return 0;
	if (client_info->is_set_init_l != 1) {
		memset(&(client_info->acl_rule), 0, sizeof(struct vacl_user_node_t));
		vacl_user_node_init(&(client_info->acl_rule));
	}
	client_info->is_set_init_l = 1;
	return 0;
}

int
cli_vacl_util_clear(void)
{
	struct clitask_client_info_t *client_info = clitask_get_thread_private();
	client_info->is_set_init_l = 0;
	cli_vacl_util_init();
	return 0;
}

static int
cli_util_vaclset_ingress_field(int fd, struct vacl_user_node_t *acl_rule_p, char *name, char *val)
{
	int ret = CLI_OK;

	cli_vacl_util_init();
	if (strcmp(name, "dmac")==0) {
		if ((ret = conv_str_mac_to_mem((char *)(acl_rule_p->ingress_dstmac_value), val)) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dstmac_value = 1;
	}
	else if (strcmp(name, "dmac_m")==0) {
		if ((ret = conv_str_mac_to_mem((char *)(acl_rule_p->ingress_dstmac_mask), val)) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dstmac_mask = 1;
	}
	else if (strcmp(name, "smac")==0) {
		if ((ret = conv_str_mac_to_mem((char *)(acl_rule_p->ingress_srcmac_value), val)) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.srcmac_value = 1;
	}
	else if (strcmp(name, "smac_m")==0) {
		if ((ret = conv_str_mac_to_mem((char *)(acl_rule_p->ingress_srcmac_mask), val)) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_mac_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.srcmac_mask = 1;
	}
	else if (strcmp(name, "sip")==0 || strcmp(name, "sipv4")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_sipv4_u.addr.value.s_addr), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv4_value = 1;
	}
	else if (strcmp(name, "sip_m")==0 || strcmp(name, "sipv4_m")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_sipv4_u.addr.mask.s_addr), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv4_mask = 1;
	}
	else if (strcmp(name, "dip")==0 || strcmp(name, "dipv4")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_dipv4_u.addr.value.s_addr), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv4_value = 1;
	}
	else if (strcmp(name, "dip_m")==0 || strcmp(name, "dipv4_m")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_dipv4_u.addr.mask.s_addr), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv4_mask = 1;
	}
	else if (strcmp(name, "sip6")==0 || strcmp(name, "sipv6")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_sipv6_u.addr.value.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv6_value = 1;
	}
	else if (strcmp(name, "sip6_m")==0 || strcmp(name, "sipv6_m")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_sipv6_u.addr.mask.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv6_mask = 1;
	}
	else if (strcmp(name, "dip6")==0 || strcmp(name, "dipv6")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_dipv6_u.addr.value.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv6_value = 1;
	}
	else if (strcmp(name, "dip6_m")==0 || strcmp(name, "dipv6_m")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_dipv6_u.addr.mask.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv6_mask = 1;
	}
	else if (strcmp(name, "svid_lb")==0) {
		acl_rule_p->ingress_stag_u.vid_range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.stag_vid_lower = 1;
	}
	else if (strcmp(name, "svid_ub")==0) {
		acl_rule_p->ingress_stag_u.vid_range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.stag_vid_upper = 1;
	}
	else if (strcmp(name, "cvid_lb")==0) {
		acl_rule_p->ingress_ctag_u.vid_range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ctag_vid_lower = 1;
	}
	else if (strcmp(name, "cvid_ub")==0) {
		acl_rule_p->ingress_ctag_u.vid_range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ctag_vid_upper = 1;
	}
	else if (strcmp(name, "sport_lb")==0 || strcmp(name, "l4sport_lb")==0) {
		acl_rule_p->ingress_l4sport_u.range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.l4sport_lower = 1;
	}
	else if (strcmp(name, "sport_ub")==0 || strcmp(name, "l4sport_ub")==0) {
		acl_rule_p->ingress_l4sport_u.range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.l4sport_upper = 1;
	}
	else if (strcmp(name, "dport_lb")==0 || strcmp(name, "l4dport_lb")==0) {
		acl_rule_p->ingress_l4dport_u.range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.l4dport_lower = 1;
	}
	else if (strcmp(name, "dport_ub")==0 || strcmp(name, "l4dport_ub")==0) {
		acl_rule_p->ingress_l4dport_u.range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.l4dport_upper = 1;
	}
	else if (strcmp(name, "dip_lb")==0 || strcmp(name, "dipv4_lb")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_dipv4_u.range.lower), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv4_lower = 1;
	}
	else if (strcmp(name, "dip_ub")==0 || strcmp(name, "dipv4_ub")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_dipv4_u.range.upper), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv4_upper = 1;
	}
	else if (strcmp(name, "sip_lb")==0 || strcmp(name, "sipv4_lb")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_sipv4_u.range.lower), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv4_lower = 1;
	}
	else if (strcmp(name, "sip_ub")==0 || strcmp(name, "sipv4_ub")==0) {
		if (conv_str_ipv4_to_mem((char *)&(acl_rule_p->ingress_sipv4_u.range.upper), val) != 0) {
			dbprintf_vacl(LOG_ERR, "conv_str_ipv4_to_mem error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv4_upper = 1;
	}
	else if (strcmp(name, "sip6_lb")==0 || strcmp(name, "sipv6_lb")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_sipv6_u.range.lower.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv6_lower = 1;
	}
	else if (strcmp(name, "sip6_ub")==0 || strcmp(name, "sipv6_ub")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_sipv6_u.range.upper.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.sipv6_upper = 1;
	}
	else if (strcmp(name, "dip6_lb")==0 || strcmp(name, "dipv6_lb")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_dipv6_u.range.lower.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv6_lower = 1;
	}
	else if (strcmp(name, "dip6_ub")==0 || strcmp(name, "dipv6_ub")==0) {
		if (util_inet_pton(AF_INET6, val, acl_rule_p->ingress_dipv6_u.range.upper.s6_addr) < 0) {
			dbprintf_vacl(LOG_ERR, "util_inet_pton error %x\n", ret);
			return ret;
		}
		acl_rule_p->ingress_care_u.bit.dipv6_upper = 1;
	}
	else if (strcmp(name, "pktlen_lb")==0) {
		acl_rule_p->ingress_pktlen_range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.pktlen_lower = 1;
	}
	else if (strcmp(name, "pktlen_ub")==0) {
		acl_rule_p->ingress_pktlen_range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.pktlen_upper = 1;
	}
	else if (strcmp(name, "pktlen_invert")==0) {
		acl_rule_p->ingress_pktlen_invert = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.pktlen_invert = 1;
	}
	else if (strcmp(name, VACL_CLI_FLD_GEMIDX)==0 || strcmp(name, VACL2_CLI_FLD_STREAM_ID)==0) {
		acl_rule_p->ingress_gem_llid_value = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.gem_llid_value = 1;
	}
	else if (strcmp(name, VACL_CLI_FLD_GEMIDX_MASK)==0 || strcmp(name, VACL2_CLI_FLD_STREAM_ID_MASK)==0) {
		acl_rule_p->ingress_gem_llid_mask = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.gem_llid_mask = 1;
	}
	else if (strcmp(name, "ethertype")==0 || strcmp(name, "et")==0) {
		acl_rule_p->ingress_ether_type_u.etype.value = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ether_type_value = 1;
	}
	else if (strcmp(name, "ethertype_m")==0 || strcmp(name, "et_m")==0) {
		acl_rule_p->ingress_ether_type_u.etype.mask = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ether_type_mask = 1;
	}
	else if (strcmp(name, "ethertype_lb")==0 || strcmp(name, "et_lb")==0) {
		acl_rule_p->ingress_ether_type_u.range.lower = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ether_type_lower = 1;
	}
	else if (strcmp(name, "ethertype_ub")==0 || strcmp(name, "et_ub")==0) {
		acl_rule_p->ingress_ether_type_u.range.upper = util_atoi(val);
		acl_rule_p->ingress_care_u.bit.ether_type_upper = 1;
	}
	else if (strcmp(name, VACL_CLI_FLD_PROTOCOL_VALUE)==0) {
		acl_rule_p->ingress_protocol_value = util_atoi(val);
		acl_rule_p->ingress_protocol_value_mask = VACL_FLD_PROTOCOL_VALUE_MAX;
		acl_rule_p->ingress_care_u.bit.protocol_value = 1;
		acl_rule_p->ingress_care_u.bit.protocol_value_mask = 1;
	}
	else if (strcmp(name, VACL_CLI_FLD_DSCP_IPV4)==0) {
		vacl_ingress_dscp_ipv4_value_set(acl_rule_p, util_atoi(val));
	}
	else {
		util_fdprintf(fd, "Not support [%s] [%s]\n", name, val);
	}
	return ret;
}

static int
cli_util_vaclset_ingress_field_l2tag(int fd, struct vacl_user_node_t *acl_rule_p, int flag_stag, char *val_s, char *mask_s)
{
	int ret = CLI_OK;

	cli_vacl_util_init();
	if (flag_stag != 0) {
		if (val_s != NULL) {
			acl_rule_p->ingress_stag_u.vid.value = util_atoi(val_s)&0xfff;
			acl_rule_p->ingress_stag_dei_value   = (util_atoi(val_s) >>12)&0x1;
			acl_rule_p->ingress_stag_pri_value   = (util_atoi(val_s) >>13)&0x7;
			acl_rule_p->ingress_care_u.bit.stag_value = 1;
		}
		if (mask_s != NULL) {
			acl_rule_p->ingress_stag_u.vid.mask  = util_atoi(mask_s)&0xfff;
			acl_rule_p->ingress_stag_dei_mask    = (util_atoi(mask_s)>>12)&0x1;
			acl_rule_p->ingress_stag_pri_mask    = (util_atoi(mask_s)>>13)&0x7;
			acl_rule_p->ingress_care_u.bit.stag_mask = 1;
		}
	}
	else {
		if (val_s != NULL) {
			acl_rule_p->ingress_ctag_u.vid.value = util_atoi(val_s)&0xfff;
			acl_rule_p->ingress_ctag_cfi_value   = (util_atoi(val_s) >>12)&0x1;
			acl_rule_p->ingress_ctag_pri_value   = (util_atoi(val_s) >>13)&0x7;
			acl_rule_p->ingress_care_u.bit.ctag_value = 1;
		}
		if (mask_s != NULL) {
			acl_rule_p->ingress_ctag_u.vid.mask  = util_atoi(mask_s)&0xfff;
			acl_rule_p->ingress_ctag_cfi_mask    = (util_atoi(mask_s)>>12)&0x1;
			acl_rule_p->ingress_ctag_pri_mask    = (util_atoi(mask_s)>>13)&0x7;
			acl_rule_p->ingress_care_u.bit.ctag_mask = 1;
		}
	}
	return ret;
}

static int
cli_util_vaclset_ingress_port(int fd, struct vacl_user_node_t *acl_rule_p, char *port_s)
{
	int ret = CLI_OK;

	cli_vacl_util_init();
	acl_rule_p->ingress_active_portmask = util_atoi(port_s);
	return ret;
}

static int
cli_util_vacl_set_action_meter(int fd, struct vacl_user_node_t *acl_rule_p, char *burst_s, char *rate_s, char *ifg_s, int index)
{
	if (index < 0) {
		dbprintf_vacl(LOG_ERR, "meter table index(%d) error\n", index);
		return CLI_ERROR_OTHER_CATEGORY;
	}
	acl_rule_p->act_type |= VACL_ACT_LOG_POLICING_MASK;
	acl_rule_p->act_meter_bucket_size = util_atoi(burst_s)*ACT_METER_BUCKET_SIZE_UNIT;
	acl_rule_p->act_meter_rate = util_atoi(rate_s)*ACT_METER_RATE_UNIT;
	acl_rule_p->act_meter_ifg = util_atoi(ifg_s);
	acl_rule_p->hw_meter_table_entry = index;
	dbprintf_vacl(LOG_INFO, "burst:%d rate:%d ifg:%d hw_meter_table_entry:%d\n",
		 acl_rule_p->act_meter_bucket_size,
		 acl_rule_p->act_meter_rate,
		 acl_rule_p->act_meter_ifg,
		 acl_rule_p->hw_meter_table_entry);
	return 0;
}

static int
cli_util_vacl_set_action_pri(int fd, struct vacl_user_node_t *acl_rule_p, unsigned long long act, int val)
{
	acl_rule_p->act_type |= act;
	acl_rule_p->act_pri_data = val;
	return 0;
}

static int
cli_util_vaclset_action(int fd, struct vacl_user_node_t *acl_rule_p, char *act_s)
{
	int ret=0;

	if (strcmp(act_s, VACL_ACT_DROP_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_DROP_MASK;

	else if (strcmp(act_s, VACL_ACT_FWD_COPY_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_COPY_MASK;

	else if (strcmp(act_s, VACL_ACT_FWD_REDIRECT_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_REDIRECT_MASK;

	else if (strcmp(act_s, VACL_ACT_FWD_MIRROR_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_IGR_MIRROR_MASK;

	else if (strcmp(act_s, VACL_ACT_TRAP_CPU_STR)==0 || strcmp(act_s, VACL_ACT_TRAP_PS_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_TRAP_MASK;

	else if (strcmp(act_s, VACL_ACT_LATCH_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_INTR_LATCH_MASK;

	else if (strcmp(act_s, VACL2_ACT_PERMIT_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_FWD_PERMIT_MASK;

	else if (strcmp(act_s, VACL_ACT_STATISTIC_STR)==0)
		acl_rule_p->act_type |= VACL_ACT_LOG_MIB_MASK;

	else
		ret = -1;
	return ret;
}

static int
cli_util_vaclset_ingress_care_tag(int fd, struct vacl_user_node_t *acl_rule_p, char *ct_val_s)
{
	if (strcmp(ct_val_s, "ctag") == 0) {
		acl_rule_p->ingress_care_tag_ctag_value = 1;
		acl_rule_p->ingress_care_tag_ctag_mask = 1;
	}
	else if (strcmp(ct_val_s, "stag") == 0) {
		acl_rule_p->ingress_care_tag_stag_value = 1;
		acl_rule_p->ingress_care_tag_stag_mask = 1;
	}
	else if (strcmp(ct_val_s, "ipv4") == 0) {
		acl_rule_p->ingress_care_tag_ipv4_value = 1;
		acl_rule_p->ingress_care_tag_ipv4_mask = 1;
	}
	else if (strcmp(ct_val_s, "ipv6") == 0) {
		acl_rule_p->ingress_care_tag_ipv6_value = 1;
		acl_rule_p->ingress_care_tag_ipv6_mask = 1;
	}
	else if (strcmp(ct_val_s, "pppoe") == 0) {
		acl_rule_p->ingress_care_tag_pppoe_value = 1;
		acl_rule_p->ingress_care_tag_pppoe_mask = 1;
	}
	else if (strcmp(ct_val_s, "udp") == 0) {
		acl_rule_p->ingress_care_tag_udp_value = 1;
		acl_rule_p->ingress_care_tag_udp_mask = 1;
	}
	else if (strcmp(ct_val_s, "tcp") == 0) {
		acl_rule_p->ingress_care_tag_tcp_value = 1;
		acl_rule_p->ingress_care_tag_tcp_mask = 1;
	}
	else
		return CLI_ERROR_OTHER_CATEGORY;
	return 0;
}



// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_vacl_set_help(int fd)
{
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <string>\n",
		      VACL_CLI_FLD_DMAC,VACL_CLI_FLD_DMAC_MASK,VACL_CLI_FLD_SMAC,VACL_CLI_FLD_SMAC_MASK);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <tci_val_mask>\n",
		      VACL_CLI_FLD_STAG,VACL_CLI_FLD_STAG_MASK,VACL_CLI_FLD_CTAG,VACL_CLI_FLD_CTAG_MASK);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <string>\n",
		      VACL_CLI_FLD_SIPV4,VACL_CLI_FLD_SIPV4_MASK,VACL_CLI_FLD_DIPV4,VACL_CLI_FLD_DIPV4_MASK);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <string>\n",
		      VACL_CLI_FLD_SIPV6,VACL_CLI_FLD_SIPV6_MASK,VACL_CLI_FLD_DIPV6,VACL_CLI_FLD_DIPV6_MASK);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <string>\n",
		      VACL_CLI_FLD_SIPV4_LB,VACL_CLI_FLD_SIPV4_UB,VACL_CLI_FLD_DIPV4_LB,VACL_CLI_FLD_DIPV4_UB);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <string>\n",
		      VACL_CLI_FLD_SIPV6_LB,VACL_CLI_FLD_SIPV6_UB,VACL_CLI_FLD_DIPV6_LB,VACL_CLI_FLD_DIPV6_UB);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <number>\n",
		      VACL_CLI_FLD_CVID_LB,VACL_CLI_FLD_CVID_UB,VACL_CLI_FLD_SVID_LB,VACL_CLI_FLD_SVID_UB);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <number>\n",
		      VACL_CLI_FLD_L4SPORT_LB,VACL_CLI_FLD_L4SPORT_UB,VACL_CLI_FLD_L4DPORT_LB,VACL_CLI_FLD_L4DPORT_UB);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s] <number>\n",
		      VACL_CLI_FLD_PKTLEN_LB,VACL_CLI_FLD_PKTLEN_UB,VACL_CLI_FLD_PKTLEN_INVERT);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <number>\n",
		      VACL_CLI_FLD_GEMIDX,VACL_CLI_FLD_GEMIDX_MASK, VACL2_CLI_FLD_STREAM_ID,VACL2_CLI_FLD_STREAM_ID_MASK);
	util_fdprintf(fd, "vacl set in-fld [%s|%s|%s|%s] <number>\n",
		      VACL_CLI_FLD_ETHERTYPE,VACL_CLI_FLD_ETHERTYPE_MASK,VACL_CLI_FLD_ETHERTYPE_S1,VACL_CLI_FLD_ETHERTYPE_MASK_S1);
	util_fdprintf(fd, "vacl set in-fld [%s|%s] <number>\n",
		      VACL_CLI_FLD_ETHERTYPE_LB,VACL_CLI_FLD_ETHERTYPE_UB);
	util_fdprintf(fd, "vacl set in-fld %s <number>\n",
		      VACL_CLI_FLD_PROTOCOL_VALUE);
	util_fdprintf(fd, "vacl set in-fld %s <number>\n",
		      VACL_CLI_FLD_DSCP_IPV4);
	util_fdprintf(fd,
		      "vacl set invert <0|1>\n"
		      "vacl set portmask <port_mask>, logical portmask, p5,p4,...,p1,p0\n"
		      "vacl set order <number>, user define rule order 0~255\n"
		      "vacl set in-care_tag <var>, <var>=ctag,stag,ipv4,ipv6,pppoe,tcp,udp\n");
	util_fdprintf(fd, "vacl set %s <number>\n", VACL_CLI_METER_RESERVE);
	util_fdprintf(fd, "vacl set act help, for more act help\n");
	return;
}

void
cli_vacl_set_act_help(int fd)
{
	util_fdprintf(fd, "vacl set act %s, for Drop\n", VACL_ACT_DROP_STR);
	util_fdprintf(fd,
		      "vacl set act [%s|%s|%s], for Forward Copy, Redirect, Mirror\n"
		      "vacl set act %s <port_mask>, used for forward(Copy, Redirect, Mirror) logical port mask\n",
		      VACL_ACT_FWD_COPY_STR,VACL_ACT_FWD_REDIRECT_STR,VACL_ACT_FWD_MIRROR_STR,VACL_ACT_FWD_PORT_STR);

	util_fdprintf(fd, "vacl set act [%s|%s], for Trap-to-CPU\n", VACL_ACT_TRAP_CPU_STR, VACL_ACT_TRAP_PS_STR);
	util_fdprintf(fd,
		      "vacl set act %s\n"
		      "vacl set act %s %s <bucket_size> <rate> <ifg_include>, for share meter auto index and share same meter\n"
		      "vacl set act %s %s <bucket_size> <rate> <ifg_include> share 1, for share meter auto index and share same meter\n"
		      "vacl set act %s %s <bucket_size> <rate> <ifg_include> share 0, for share meter auto index and no share same meter\n"
		      "vacl set act %s %s <bucket_size> <rate> <ifg_include> index <number>, for share meter index assignment\n"
		      "vacl set act %s %s index <number>, for share meter existence index\n"
		      "                <bucket_size> 0~65535 byte\n"
		      "                <rate>        1~131071, 1 means 8Kbps, 2 16Kbps, ...\n"
		      "                <ifg_include> 0~1\n",
		      VACL_ACT_QOS_STR,
		      VACL_ACT_QOS_STR,VACL_ACT_QOS_SHARE_METER_STR,
		      VACL_ACT_QOS_STR,VACL_ACT_QOS_SHARE_METER_STR,
		      VACL_ACT_QOS_STR,VACL_ACT_QOS_SHARE_METER_STR);
	util_fdprintf(fd, "vacl set act %s, for Latch\n", VACL_ACT_LATCH_STR);
	util_fdprintf(fd, "vacl set act %s, for Forward_Copy/Port_none\n", VACL2_ACT_PERMIT_STR);
	util_fdprintf(fd, "vacl set act %s %s <number>, 0~7  internal ACL priority assignment\n",
		      VACL_ACT_QOS_STR, VACL_ACT_QOS_ACL_PRIORITY_STR);
	util_fdprintf(fd, "vacl set act %s %s <number>, 0~63 DSCP remarking priority assignment\n",
		      VACL_ACT_QOS_STR, VACL_ACT_QOS_PRI_DSCP_STR);
	util_fdprintf(fd, "vacl set act %s %s <number>, 0~7  dot1p remarking priority assignment\n",
		      VACL_ACT_QOS_STR, VACL_ACT_QOS_PRI_1P_STR);
	util_fdprintf(fd, "vacl set act %s <number>\n",
		      VACL_ACT_STATISTIC_STR);
	util_fdprintf(fd, "vacl set act %s %s <number>, 0~7 dot1p remark\n",
		      VACL_ACT_CVLAN_STR, VACL_ACT_1P_REMARK_STR);
	util_fdprintf(fd, "vacl set act %s %s <number>, 0~63 DSCP remark\n",
		      VACL_ACT_SVLAN_STR, VACL_ACT_DSCP_REMARK_STR);
	return;
}

void
cli_vacl_del_help(int fd)
{
	util_fdprintf(fd,
		"vacl del all\n"
		"vacl del hw_order <order>\n"
		"vacl del crc <crc>\n"
		"vacl del order <order>\n");
	return;
}

void
cli_vacl_hw_set_help(int fd)
{
	util_fdprintf(fd, "vacl hw_set port [enable|permit] <logical_portmask>\n");
	return;
}

static void
cli_vacl_long_help(int fd)
{
	cli_vacl_version(fd);
	util_fdprintf(fd, "vacl init|set|add|del|commit|dump|ver|clear|show\n");
	util_fdprintf(fd, "     dump [ |user]\n");
	util_fdprintf(fd, "     dump [hw_order|order|user [hw_order|order]] <number>\n");
	util_fdprintf(fd, "     set [help|subcmd...]\n");
	util_fdprintf(fd, "     hw_init|hw_commit|hw_get\n");
	util_fdprintf(fd, "     hw_set|hw_get [subcmd...]\n");
	util_fdprintf(fd, "     del [hw_order|crc] <number>\n");
	util_fdprintf(fd, "     valid [0|1] [hw_order|crc] <number>\n");
	util_fdprintf(fd, "     valid [0|1] all\n");
}

void
cli_vacl_help(int fd)
{
	util_fdprintf(fd, "vacl help|[subcmd...]\n");
}

int
cli_vacl_cmdline(int fd, int argc, char *argv[])
{
	int ret, id;
	int sub_order;
	struct clitask_client_info_t *client_info = clitask_get_thread_private();
	struct vacl_user_node_t *acl_rule_p;
	acl_rule_p=(struct vacl_user_node_t *)&(client_info->acl_rule);

	if (strcmp(argv[0], "vacl")==0 || strcmp(argv[0], "va")==0) {
		if (argc==1) {
			switch_hw_g.vacl_dump(fd, -1, -1);
			return CLI_OK;
		}
		else if (argc==2 && strcmp(argv[1], "help")==0) {
			cli_vacl_long_help(fd);
			return CLI_OK;
		}
		else if (argc==2 && (strcmp(argv[1], "ver")==0)) {
			cli_vacl_version(fd);
			return CLI_OK;
		}
		else if (argc==2 &&  strcmp(argv[1], "init")==0) {
			_vacl_init();
			return CLI_OK;
		}
		else if (argc==2 &&  strcmp(argv[1], "clear")==0) {
			return cli_vacl_util_clear();
		}
		else if (argc==2 &&  strcmp(argv[1], "show")==0) {
			vacl_user_dump(fd, acl_rule_p);
			return 0;
		}
		else if (argc==2 && (strcmp(argv[1], "hw_init")==0 || strcmp(argv[1], "hi")==0)) {
			_vacl_init();
			return 0;
		}
		else if ((strcmp(argv[1], "hw_set")==0) || (strcmp(argv[1], "hs")==0)) {
			if (argc>4 && strcmp(argv[2], "port")==0 ) {
				if (strcmp(argv[3], "enable")==0 || strcmp(argv[3], "en")==0) {
					vacl_port_enable_set(util_atoi(argv[4]));
					return CLI_OK;
				}
				else if (strcmp(argv[3], "permit")==0 || strcmp(argv[3], "pe")==0) {
					vacl_port_permit_set(util_atoi(argv[4]));
					return CLI_OK;
				}
			}
		}
		else if ((strcmp(argv[1], "hw_get")==0) || (strcmp(argv[1], "hg")==0)) {
			if (argc==3 && strcmp(argv[2], "port")==0) {
				switch_hw_g.vacl_port_enable_print(fd);
				switch_hw_g.vacl_port_permit_print(fd);
				return CLI_OK;
			}
			else if (argc>3 && strcmp(argv[2], "port")==0) {
				if (strcmp(argv[3], "enable")==0 || strcmp(argv[3], "en")==0) {
					switch_hw_g.vacl_port_enable_print(fd);
					return CLI_OK;
				}
				else if (strcmp(argv[3], "permit")==0 || strcmp(argv[3], "pe")==0) {
					switch_hw_g.vacl_port_permit_print(fd);
					return CLI_OK;
				}
			}
		}
		else {
			if (vacl_init_check() != 0) {
				util_fdprintf(fd, "Please acl init first.\n");
				return CLI_OK;
			}
			else if (argc>=2 && (strcmp(argv[1], "dump")==0 || strcmp(argv[1], "get")==0)) {
				if (argc==2) {
					switch_hw_g.vacl_dump(fd, -1, -1);
					return CLI_OK;
				}
				else if (argc==4 && strcmp(argv[2], "hw_order")==0) {
					switch_hw_g.vacl_dump(fd, util_atoi(argv[3]), -1);
					return CLI_OK;
				}
				else if (argc==4 && strcmp(argv[2], "order")==0) {
					switch_hw_g.vacl_dump(fd, -1, util_atoi(argv[3]));
					return CLI_OK;
				}
				else if (argc==3 && strcmp(argv[2], "user")==0) {
					vacl_dump(fd, -1, -1);
					return CLI_OK;
				}
				else if (argc==5 && strcmp(argv[2], "user")==0 && strcmp(argv[3], "hw_order")==0) {
					vacl_dump(fd, util_atoi(argv[4]), -1);
					return CLI_OK;
				}
				else if (argc==5 && strcmp(argv[2], "user")==0 && strcmp(argv[3], "order")==0) {
					vacl_dump(fd, -1, util_atoi(argv[4]));
					return CLI_OK;
				}
				else if (argc==3 && (strcmp(argv[2], "portmask")==0)) {
					return vacl_dump_portmask(fd);
				}
				else if (argc==3 && (strcmp(argv[2], "portidx")==0)) {
					return vacl_dump_portidx(fd);
				}
				else if (argc==3 && (strcmp(argv[2], VACL_CLI_METER_RESERVE)==0)) {
					util_fdprintf(fd, "%s=%d\n", VACL_CLI_METER_RESERVE, meter_entry_reserve_g);
					return CLI_OK;
				}
			}
			else if (argc>=3 && strcmp(argv[1], "set")==0) {
				if (strcmp(argv[2], "help")==0) {
					cli_vacl_set_help(fd);
					return 0;
				}
				else if (strcmp(argv[2], "fld")==0 || strcmp(argv[2], "in-fld")==0) {
					if (argc==6 && strcmp(argv[3], VACL_CLI_FLD_STAG)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 1, argv[4], argv[5]);
					else if (argc==6 && strcmp(argv[3], VACL_CLI_FLD_CTAG)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 0, argv[4], argv[5]);
					else if (argc==5 && strcmp(argv[3], VACL_CLI_FLD_STAG)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 1, argv[4], NULL);
					else if (argc==5 && strcmp(argv[3], VACL_CLI_FLD_STAG_MASK)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 1, NULL, argv[4]);
					else if (argc==5 && strcmp(argv[3], VACL_CLI_FLD_CTAG)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 0, argv[4], NULL);
					else if (argc==5 && strcmp(argv[3], VACL_CLI_FLD_CTAG_MASK)==0)
						return cli_util_vaclset_ingress_field_l2tag(fd, acl_rule_p, 0, NULL, argv[4]);
					else if (argc>=5)
						return cli_util_vaclset_ingress_field(fd, acl_rule_p, argv[3], argv[4]);
				}
				else if (strcmp(argv[2], "act")==0) {
					cli_vacl_util_init();
					if (strcmp(argv[3], "help")==0) {
						cli_vacl_set_act_help(fd);
						return 0;
					}
					else if (argc>=6 && strcmp(argv[3], VACL_ACT_QOS_STR)==0) {
						if ((argc==8 && cli_act_key_share_meter(argv[4])) ||
						    (argc==10 && cli_act_key_share_meter(argv[4])
						     && strcmp(argv[8], "share")==0
						     && *argv[9]=='1')) {
							return cli_util_vacl_set_action_meter(fd, acl_rule_p,  argv[5], argv[6], argv[7], ACT_METER_INDEX_AUTO_W_SHARE);
						}
						else if ((argc==10 && cli_act_key_share_meter(argv[4])
							  && strcmp(argv[8], "share")==0
							  && *argv[9]=='0')) {
							return cli_util_vacl_set_action_meter(fd, acl_rule_p, argv[5], argv[6], argv[7], ACT_METER_INDEX_AUTO_WO_SHARE);
						}
						else if (argc==10 && strcmp(argv[4], VACL_ACT_QOS_SHARE_METER_STR)==0 && strcmp(argv[8], "index")==0)
							return cli_util_vacl_set_action_meter(fd, acl_rule_p, argv[5], argv[6], argv[7], atoi(argv[9]));
						else if (argc==7 && strcmp(argv[4], VACL_ACT_QOS_SHARE_METER_STR)==0 && strcmp(argv[5], "index")==0)
							return cli_util_vacl_set_action_meter(fd, acl_rule_p, "0", "0", "0", atoi(argv[6]));
						else if (argc==6 && strcmp(argv[4], VACL_ACT_QOS_ACL_PRIORITY_STR)==0)
							return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_ACL_PRI_ASSIGN_MASK, util_atoi(argv[5]));
						else if (argc==6 && strcmp(argv[4], VACL_ACT_QOS_PRI_DSCP_STR)==0)
							return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_DSCP_REMARK_MASK, util_atoi(argv[5]));
						else if (argc==6 && strcmp(argv[4], VACL_ACT_QOS_PRI_1P_STR)==0)
							return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_1P_REMARK_MASK, util_atoi(argv[5]));
					}
					else if (argc==5 && cli_act_key_fwd_port(argv[3])) {
						acl_rule_p->act_forward_portmask = util_atoi(argv[4]);
						return 0;
					}
					else if (argc>=4 && (*argv[3])>='0' && (*argv[3])<='9') {
						// compatible older version, 0.vacl 1.set 2.act 3.<number> ...
						id = get_id_num(argv[3]);
						switch (id) {
						case VACL_ACT_DROP_ID:
							return cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_DROP_STR);
						case VACL_ACT_FWD_COPY_ID:
							cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_FWD_COPY_STR);
							if(argc==6 && cli_act_key_fwd_port(argv[4]))
								acl_rule_p->act_forward_portmask = util_atoi(argv[5]);
							return 0;
						case VACL_ACT_FWD_REDIRECT_ID:
							cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_FWD_REDIRECT_STR);
							if(argc==6 && cli_act_key_fwd_port(argv[4]))
								acl_rule_p->act_forward_portmask = util_atoi(argv[5]);
							return 0;
						case VACL_ACT_FWD_MIRROR_ID:
							cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_FWD_MIRROR_STR);
							if(argc==6 && cli_act_key_fwd_port(argv[4]))
								acl_rule_p->act_forward_portmask = util_atoi(argv[5]);
							return 0;
						case VACL_ACT_TRAP_CPU_ID:
							return cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_TRAP_CPU_STR);
						case VACL_ACT_LATCH_ID:
							return cli_util_vaclset_action(fd, acl_rule_p, VACL_ACT_LATCH_STR);
						case VACL_ACT_SHARE_METER_ID:
							if (argc==7)
								// 0.vacl 1.set 2.act 3.meter 4.bucket_size 5.rate 6.ifg_include
								return cli_util_vacl_set_action_meter(fd, acl_rule_p, argv[4], argv[5], argv[6], ACT_METER_INDEX_AUTO_W_SHARE);
						case VACL_ACT_PRI_IPRI_ID:
							if (argc==5)
								return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_ACL_PRI_ASSIGN_MASK, util_atoi(argv[4]));
						case VACL_ACT_PRI_DSCP_ID:
							if (argc==5)
								return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_DSCP_REMARK_MASK, util_atoi(argv[4]));
						case VACL_ACT_PRI_1P_ID:
							if (argc==5)
								return cli_util_vacl_set_action_pri(fd, acl_rule_p, VACL_ACT_PRI_1P_REMARK_MASK, util_atoi(argv[4]));
						default:
							break;

						}
					}
					else if (argc==7 && cli_act_key_share_meter(argv[3])) {
						// backward compatible, 0.vacl 1.set 2.act 3.meter 4.bucket_size 5.rate 6.ifg_include
						return cli_util_vacl_set_action_meter(fd, acl_rule_p, argv[4], argv[5], argv[6], ACT_METER_INDEX_AUTO_W_SHARE);
					}
					else if (argc==5 && strcmp(argv[3], VACL_ACT_STATISTIC_STR)==0) {
						acl_rule_p->act_type |= VACL_ACT_LOG_MIB_MASK;
						acl_rule_p->act_statistic_index = util_atoi(argv[4]);
						return 0;
					}
					else if (argc>=6 && strcmp(argv[3], VACL_ACT_CVLAN_STR)==0) {
						if (argc==6 && strcmp(argv[4], VACL_ACT_1P_REMARK_STR)==0) {
						// (0)vacl (1)set (2)act (3)cvlan (4)CACT 3bits:dot1p_remark (5)CVIDX_CACT 6bits
							acl_rule_p->act_type |= VACL_ACT_CVLAN_1P_REMARK_MASK;
							acl_rule_p->act_cvlan_1p_remark = util_atoi(argv[5]);
							return 0;
						}
					}
					else if (argc>=6 && strcmp(argv[3], VACL_ACT_SVLAN_STR)==0) {
						if (argc==6 && strcmp(argv[4], VACL_ACT_DSCP_REMARK_STR)==0) {
						// (0)vacl (1)set (2)act (3)svlan (4)CACT 3bits:dscp_remark (5)CVIDX_CACT 6bits
							acl_rule_p->act_type |= VACL_ACT_SVLAN_DSCP_REMARK_MASK;
							acl_rule_p->act_svlan_dscp_remark = util_atoi(argv[5]);
							return 0;
						}
					}
					else
						return cli_util_vaclset_action(fd, acl_rule_p, argv[3]);
				}
				else if (strcmp(argv[2], VACL_CLI_FLD_PORTMASK)==0 ||
					 strcmp(argv[2], VACL2_CLI_FLD_PORTMASK)==0 ||
					 strcmp(argv[2], VACL2_CLI_FLD_PORTMASK_S1)==0) {
					return cli_util_vaclset_ingress_port(fd, acl_rule_p, argv[3]);
				}
				else if (strcmp(argv[2], "order")==0) {
					acl_rule_p->order = util_atoi(argv[3]);
					if( acl_rule_p->order > 255) {
						util_fdprintf(fd, "order value range is 0~255\n");
						acl_rule_p->order = 0;
					}
					return 0;
				}
				else if (argc==4 && strcmp(argv[2], VACL_CLI_INVERT)==0) {
					id = util_atoi(argv[3]);
					if (id < 0) {
						acl_rule_p->invert = 0;
						util_fdprintf(fd, "%s should be 0 or 1\n", VACL_CLI_INVERT);
					}
					else if (id == 0)
						acl_rule_p->invert = 0;
					else
						acl_rule_p->invert = 1;
					return 0;
				}
				else if (cli_infld_key_care_tag(argv[2])) {
					if (argc==4)
						return cli_util_vaclset_ingress_care_tag(fd, acl_rule_p, argv[3]);
				}
				else if (argc==4 && strcmp(argv[2], VACL_CLI_METER_RESERVE)==0) {
					meter_entry_reserve_g = util_atoi(argv[3]);
					return CLI_OK;
				}
			}
			else if (strcmp(argv[1], "add")==0) {
				client_info->is_set_init_l = 0;
				ret = vacl_add(acl_rule_p, &sub_order);
				if (ret == 0) {
					util_fdprintf(fd, "Add this rule at order(%d) position %d temporarily\n",
						      acl_rule_p->order, sub_order);
				} else if (ret == VACL_ADD_RET_FULL) {
					util_fdprintf(fd, "This rule set is full.\n");
				} else if (ret == VACL_ADD_RET_DUPLICATE) {
					util_fdprintf(fd, "This rule is already added.\n");
				} else if (ret == -1) {
					util_fdprintf(fd, "Out of resource to add this rule.\n");
				}
				return cli_vacl_util_clear();
			}
			else if (strcmp(argv[1], "del")==0) {
				int hw_order = 0xffff;
				client_info->is_set_init_l = 0;
				if (strcmp(argv[2], "help")==0) {
					cli_vacl_del_help(fd);
					return 0;
				}
				else if (argc==3 && strcmp(argv[2], "all")==0) {
					vacl_del_all();
					util_fdprintf(fd, "Delete all sw/hw acl records done.\n");
					return 0;
				}
				else if (argc==4 && (strcmp(argv[2], "hw_order")==0 || strcmp(argv[2], "ho")==0)) {
					hw_order = util_atoi(argv[3]);
					util_fdprintf(fd, "Delete rule hw_order = %d\n", hw_order);
					ret = vacl_del_hworder(hw_order);
				}
				else if (argc==4 && strcmp(argv[2], "crc")==0) {
					util_fdprintf(fd, "Delete rule crc = 0x%X\n", (unsigned int)util_atoll(argv[3]));
					ret = vacl_del_crc((unsigned int)util_atoll(argv[3]), &hw_order);
				}
				else if (argc==4 && strcmp(argv[2], "order")==0) {
					int count = 0;
					ret = vacl_del_order(util_atoi(argv[3]), &count);
					util_fdprintf(fd, "Deleted %d rules order = %d\n", count, util_atoi(argv[3]));
				}
				if (ret == 0) {
					util_fdprintf(fd, "Delete this rule temporary, please hw_commit to complete it.\n");
				} else if (ret == VACL_DEL_RET_EMPTY) {
					util_fdprintf(fd, "The rule set is empty.\n");
				} else if (ret == VACL_DEL_RET_MISS) {
					util_fdprintf(fd, "This rule does not exist.\n");
				}
				return 0;
			}
			else if ((strcmp(argv[1], "valid")==0) || (strcmp(argv[1], "va")==0)) {
				unsigned short valid = util_atoi(argv[2]);
				if (argc==4 && strcmp(argv[3], "all")==0) {
					vacl_valid_all(valid);
					util_fdprintf(fd, "%s all acl rules, please hw_commit to complete it.\n",
						      (valid==0)?"Invalid":"Valid");
					return 0;
				}
				if (argc==5 && (strcmp(argv[3], "hw_order")==0 || strcmp(argv[3], "ho")==0)) {
					int hw_order;
					hw_order = util_atoi(argv[4]);
					vacl_valid_hworder(hw_order, valid);
					util_fdprintf(fd, "%s acl rules hw_order=%d, please hw_commit to complete it.\n",
						      (valid==0)?"Invalid":"Valid", hw_order);
					return 0;
				}
				else if (argc==5 && strcmp(argv[3], "crc")==0) {
					unsigned int crc;
					crc = (unsigned int)util_atoll(argv[4]);
					vacl_valid_crc(crc, valid);
					util_fdprintf(fd, "%s acl rules crc=0x%x, please hw_commit to complete it.\n",
						      (valid==0)?"Invalid":"Valid", crc);
					return 0;
				}
				return 0;
			}
			else if (argc==2 && (strcmp(argv[1], "commit")==0 || strcmp(argv[1], "co")==0)) {
				vacl_commit();
				return CLI_OK;
			}
			else if (argc==2 && (strcmp(argv[1], "hw_commit")==0 || strcmp(argv[1], "hc")==0)) {
				ret = 0;
				switch_hw_g.vacl_commit(&ret);
				util_fdprintf(fd, "%d rules hw_commit-ed.\n", ret);
				return CLI_OK;
			}
		}
	}
	return CLI_ERROR_OTHER_CATEGORY;
}

