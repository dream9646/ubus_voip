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
 * File    : classf_protocol_vlan.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "classf.h"
#include "util.h"
#include "util_inet.h"
#include "metacfg_adapter.h"
#include "conv.h"
#include "switch.h"
#include "er_group.h"

#define CLASSF_PROTOCOL_VLAN_FIELD_NAME_TEMPLETE "l2_proto_vlan%u_%s"
#define CLASSF_PROTOCOL_VLAN_DHCP_FIELD_NAME_TEMPLETE "l2_proto_vlan_dhcp%u_%s"
#define CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM 32
#define CLASSF_PROTOCOL_VLAN_DHCP_ORDER CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM
#define CLASSF_PROTOCOL_VLAN_UNI_WIFI_PORT_WILDCARD "*"
#define CLASSF_PROTOCOL_VLAN_UNI_PORT_WILDCARD "?"
#define CLASSF_PROTOCOL_VLAN_LAN_PORT_WILDCARD_RANGE 3

char *protocol_vlan_fields_name[] = 
{
	"enable",
	"order",
	"uniport",
	"ethertype",
	"smac",
	"smac_mask",
	"dmac",
	"dmac_mask",
	"sip",
	"sipmask",
	"dip",
	"dipmask",
	"v6sip",
	"v6sipprefix",
	"v6dip",
	"v6dipprefix",
	"ip_proto",
	"v6nd_target_ip",
	"sport",
	"sport_max",
	"dport",
	"dport_max",
	"vlantag_enable",
	"vlantag_id",
	"vlantag_pbit",
	"vlan_mark",
	"pbit_mark",
	NULL
};

char *protocol_vlan_dhcp_fields_name[] = 
{
	"uniport",
	"smac",
	"vlan_mark",
	"pbit_mark",
	NULL
};

static struct classf_pvlan_rule_t *
classf_alloc_pvlan_rule(void)
{
	struct classf_pvlan_rule_t *pvlan_rule;

	pvlan_rule = malloc_safe(sizeof(struct classf_pvlan_rule_t));
	pvlan_rule->join_entry_id = -1;

	return pvlan_rule;
}

#ifdef OMCI_METAFILE_ENABLE
static int //0: incorrect, 1: correct
classf_protocol_vlan_check_rule_correct(struct classf_pvlan_rule_t *pvlan_rule)
{
	if (pvlan_rule == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0;
	}

	if (pvlan_rule->enable &&
		pvlan_rule->uniport_valid)
	{
		return 1; //correct
	}

	return 0; //incorrect
}
#endif

#if 0
//0: NOT exist protocol vlan rules, 1: exist protocol vlan rules
//for dscp-2-pbit to determines global mapping table
unsigned int
classf_protocol_vlan_check_rules_is_exist(void)
{
	unsigned int i;
	char key_str[128];
	char *value_ptr;
	struct kv_config_t kv_config;
	struct classf_pvlan_rule_t pvlan_rule;

	//open protocol vlan configuration source
	kv_config_init(&kv_config);
	//load from file to hash list
	if(kv_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat") !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		kv_config_release(&kv_config);
		return 0;
	}

	memset(&pvlan_rule, 0x00, sizeof(struct classf_pvlan_rule_t));

	//collect rule list
	for (i = 0; i < CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM; i++)
	{
		//enable
		sprintf(key_str, CLASSF_PROTOCOL_VLAN_FIELD_NAME_TEMPLETE, i, protocol_vlan_fields_name[0]);
		if(strlen(value_ptr = kv_config_get_value(&kv_config, key_str, 1)) > 0)
		{
			if (util_atoi(value_ptr) > 0)
			{
				pvlan_rule.enable = 1; //enable, otherwise 0
			} else {
				pvlan_rule.enable = 0; //enable, otherwise 0
			}
		}

		//uniport
		sprintf(key_str, CLASSF_PROTOCOL_VLAN_FIELD_NAME_TEMPLETE, i, protocol_vlan_fields_name[2]);
		if(strlen(value_ptr = kv_config_get_value(&kv_config, key_str, 1)) > 0)
		{
			//must have ?
			pvlan_rule.uniport = util_atoi(value_ptr);
			pvlan_rule.uniport_valid = 1;
		}

		//check correct, then add to global list
		if (classf_protocol_vlan_check_rule_correct(&pvlan_rule))
		{
			kv_config_release(&kv_config);
			return 1;
		}
	}

	kv_config_release(&kv_config);
	return 0;
}
#endif

int
classf_protocol_vlan_collect_rules_from_metafile(struct list_head *protocol_vlan_rule_list)
{
#ifdef OMCI_METAFILE_ENABLE
	unsigned int i, j;
	char key_str[128];
	char *value_ptr;
	unsigned int total_rules = 0;
	unsigned int max_order = 0;
	struct metacfg_t kv_config;
	struct classf_pvlan_rule_t *pvlan_rule;

	if (protocol_vlan_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//protocol vlan sytem part

	//open protocol vlan configuration source
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	//load from file to hash list
	if(metacfg_adapter_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat") !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}

	//collect rule list
	for (i = 0; i < CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM; i++)
	{
		//allocate rule
		pvlan_rule = classf_alloc_pvlan_rule();
		//init rule_fields
		pvlan_rule->rule_fields.filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		pvlan_rule->rule_fields.filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		pvlan_rule->rule_fields.treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		pvlan_rule->rule_fields.treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;

		//assign rule num
		pvlan_rule->pvlan_acl.rule_num = i;

		for (j = 0; protocol_vlan_fields_name[j] != NULL; j++)
		{
			sprintf(key_str, CLASSF_PROTOCOL_VLAN_FIELD_NAME_TEMPLETE, i, protocol_vlan_fields_name[j]);

			if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, key_str, 1)) > 0 )
			{
				if (!strcmp(protocol_vlan_fields_name[j], "enable"))
				{
					if (util_atoi(value_ptr) > 0)
					{
						pvlan_rule->enable = 1; //enable, otherwise 0
					} else {
						pvlan_rule->enable = 0; //enable, otherwise 0
					}
				} else if (!strcmp(protocol_vlan_fields_name[j], "order")) {
					pvlan_rule->pvlan_acl.order = util_atoi(value_ptr);
				} else if (!strcmp(protocol_vlan_fields_name[j], "uniport")) {
					//must have ?
					if (!strcmp(value_ptr, CLASSF_PROTOCOL_VLAN_UNI_WIFI_PORT_WILDCARD))
					{
						//uni + wifi ports wildcard
						pvlan_rule->uniport = 0;
						pvlan_rule->uniport_valid = 1;
						pvlan_rule->pvlan_acl.uniport_wildcard = 1;
						pvlan_rule->pvlan_acl.uniport_wildcard_range = CLASSF_PROTOCOL_VLAN_LAN_PORT_WILDCARD_RANGE;
					} else if (!strcmp(value_ptr, CLASSF_PROTOCOL_VLAN_UNI_PORT_WILDCARD)) {
						//uni ports wildcard
						pvlan_rule->uniport = 0;
						pvlan_rule->uniport_valid = 1;
						pvlan_rule->pvlan_acl.uniport_wildcard = 2;
						pvlan_rule->pvlan_acl.uniport_wildcard_range = 3; //only uni ports(0-3)
					} else {
						pvlan_rule->uniport = util_atoi(value_ptr);
						pvlan_rule->uniport_valid = 1;
					}
				} else if (!strcmp(protocol_vlan_fields_name[j], "ethertype")) {
					pvlan_rule->pvlan_acl.ethertype = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.ethertype = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "smac")) {
					conv_str_mac_to_mem(pvlan_rule->pvlan_acl.smac, value_ptr);
					pvlan_rule->pvlan_acl.care_bit.smac = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "smac_mask")) {
					conv_str_mac_to_mem(pvlan_rule->pvlan_acl.smac_mask, value_ptr);
					pvlan_rule->pvlan_acl.care_bit.smac_mask = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "dmac")) {
					conv_str_mac_to_mem(pvlan_rule->pvlan_acl.dmac, value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dmac = 1;	
				} else if (!strcmp(protocol_vlan_fields_name[j], "dmac_mask")) {
					conv_str_mac_to_mem(pvlan_rule->pvlan_acl.dmac_mask, value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dmac_mask = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "sip")) {
					pvlan_rule->pvlan_acl.sip.s_addr = inet_addr(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.sip = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "sipmask")) {
					pvlan_rule->pvlan_acl.sip_mask.s_addr = inet_addr(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.sip_mask = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "dip")) {
					pvlan_rule->pvlan_acl.dip.s_addr = inet_addr(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dip = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "dipmask")) {
					pvlan_rule->pvlan_acl.dip_mask.s_addr = inet_addr(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dip_mask = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "v6sip")) {
					if (util_inet_pton(AF_INET6, value_ptr, pvlan_rule->pvlan_acl.v6sip.s6_addr) < 0) {
						dbprintf(LOG_ERR, "util_inet_pton error %x\n");
					} else {
						pvlan_rule->pvlan_acl.care_bit.v6sip = 1;
					}
				} else if (!strcmp(protocol_vlan_fields_name[j], "v6sipprefix")) {
					pvlan_rule->pvlan_acl.v6sip_prefix = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.v6sip_prefix = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "v6dip")) {
					if (util_inet_pton(AF_INET6, value_ptr, pvlan_rule->pvlan_acl.v6dip.s6_addr) < 0) {
						dbprintf(LOG_ERR, "util_inet_pton error %x\n");
					} else {
						pvlan_rule->pvlan_acl.care_bit.v6dip = 1;
					}
				} else if (!strcmp(protocol_vlan_fields_name[j], "v6dipprefix")) {
					pvlan_rule->pvlan_acl.v6dip_prefix = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.v6dip_prefix = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "ip_proto")) {
					pvlan_rule->pvlan_acl.protocol = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.protocol = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "v6nd_target_ip")) {
					if (util_inet_pton(AF_INET6, value_ptr, pvlan_rule->pvlan_acl.v6nd_target_ip.s6_addr) < 0) {
						dbprintf(LOG_ERR, "util_inet_pton error %x\n");
					} else {
						pvlan_rule->pvlan_acl.care_bit.v6nd_target_ip = 1;
					}
				} else if (!strcmp(protocol_vlan_fields_name[j], "sport")) {
					pvlan_rule->pvlan_acl.sport = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.sport = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "sport_max")) {
					pvlan_rule->pvlan_acl.sport_ubound = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.sport_ubound = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "dport")) {
					pvlan_rule->pvlan_acl.dport = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dport = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "dport_max")) {
					pvlan_rule->pvlan_acl.dport_ubound = util_atoi(value_ptr);
					pvlan_rule->pvlan_acl.care_bit.dport_ubound = 1;
				} else if (!strcmp(protocol_vlan_fields_name[j], "vlantag_enable")) {
					pvlan_rule->vlantag_enable = util_atoi(value_ptr);
				} else if (!strcmp(protocol_vlan_fields_name[j], "vlantag_id")) {
					pvlan_rule->vlantag_id = (util_atoi(value_ptr) <= 4096 ? util_atoi(value_ptr) : 4095);
				} else if (!strcmp(protocol_vlan_fields_name[j], "vlantag_pbit")) {
					pvlan_rule->vlantag_pbit = (util_atoi(value_ptr) <= 8 ? util_atoi(value_ptr) : 7);
				} else if (!strcmp(protocol_vlan_fields_name[j], "vlan_mark")) {
					pvlan_rule->vlan_mark = (util_atoi(value_ptr) <= CLASSF_VLAN_UNTAG_ID ? util_atoi(value_ptr) : 4095); //0-4095: vid, 4096: copy from inner(transparent), 4097: untag, otherwise: set 4095
				} else if (!strcmp(protocol_vlan_fields_name[j], "pbit_mark")) {
					pvlan_rule->pbit_mark = ((util_atoi(value_ptr) <= 8 || util_atoi(value_ptr) == 10) ? util_atoi(value_ptr) : 7); //0-7: priority, 8: copy from inner(transparent), 10: dscp-to-pbit, otherwise: set 7
				}
			}
		}

		//check correct, then add to global list
		if (classf_protocol_vlan_check_rule_correct(pvlan_rule))
		{
			list_add_tail(&pvlan_rule->pr_node, protocol_vlan_rule_list);
			total_rules++;
			//reset max order
			if (pvlan_rule->pvlan_acl.order > max_order)
			{
				max_order = pvlan_rule->pvlan_acl.order;
			}
		} else {
			dbprintf(LOG_WARNING, "Collect pvlan rule incorrect, rule num = %u\n", i);
			free_safe(pvlan_rule);
		}
	}

	metacfg_adapter_config_release(&kv_config);

	//protocol vlan DHCP part

	//open protocol vlan configuration source
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	//load from file to hash list
	if(!util_file_is_available("/var/run/omci/protocol_vlan_dhcp.dat") ||
	   metacfg_adapter_config_file_load_safe(&kv_config, "/var/run/omci/protocol_vlan_dhcp.dat") !=0) //not neccessary
	{
		metacfg_adapter_config_release(&kv_config);
		return 0;
	}

	//collect rule list
	for (i = 0; i < CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM && total_rules < CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM; i++)
	{
		//allocate rule
		pvlan_rule = classf_alloc_pvlan_rule();
		//init rule_fields
		pvlan_rule->rule_fields.filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		pvlan_rule->rule_fields.filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		pvlan_rule->rule_fields.treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		pvlan_rule->rule_fields.treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;

		//assign rule num
		pvlan_rule->pvlan_acl.rule_num = CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM + i; //add CLASSF_PROTOCOL_VLAN_RULE_MAX_NUM to represent protocol vlan DHCP

		for (j = 0; protocol_vlan_dhcp_fields_name[j] != NULL; j++)
		{
			sprintf(key_str, CLASSF_PROTOCOL_VLAN_DHCP_FIELD_NAME_TEMPLETE, i, protocol_vlan_dhcp_fields_name[j]);

			if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, key_str, 1)) > 0 )
			{
				if (!strcmp(protocol_vlan_dhcp_fields_name[j], "uniport")) {
					//must have ?
					pvlan_rule->uniport = util_atoi(value_ptr);
					pvlan_rule->uniport_valid = 1;
				} else if (!strcmp(protocol_vlan_dhcp_fields_name[j], "smac")) {
					conv_str_mac_to_mem(pvlan_rule->pvlan_acl.smac, value_ptr);
					pvlan_rule->pvlan_acl.care_bit.smac = 1;
				} else if (!strcmp(protocol_vlan_dhcp_fields_name[j], "vlan_mark")) {
					pvlan_rule->vlan_mark = (util_atoi(value_ptr) <= CLASSF_VLAN_UNTAG_ID ? util_atoi(value_ptr) : 4095); //0-4095: vid, 4096: copy from inner(transparent), 4097: untag, otherwise: set 4095
				} else if (!strcmp(protocol_vlan_dhcp_fields_name[j], "pbit_mark")) {
					pvlan_rule->pbit_mark = ((util_atoi(value_ptr) <= 8 || util_atoi(value_ptr) == 10) ? util_atoi(value_ptr) : 7); //0-7: priority, 8: copy from inner(transparent), 10: dscp-to-pbit, otherwise: set 7
				}
			}
		}

		//assign enable before check rule correct
		if (pvlan_rule->uniport_valid == 1 && 
			pvlan_rule->pvlan_acl.care_bit.smac == 1)
		{
			pvlan_rule->enable = 1;
			pvlan_rule->pvlan_acl.order = max_order + 1;
		}

		//check correct, then add to global list
		if (classf_protocol_vlan_check_rule_correct(pvlan_rule))
		{
			list_add_tail(&pvlan_rule->pr_node, protocol_vlan_rule_list);
			total_rules++;
		} else {
			dbprintf(LOG_WARNING, "Collect pvlan rule incorrect, rule num = %u\n", i);
			free_safe(pvlan_rule);
		}
	}

	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;
}

//return value: -1: pvlan_rule1 < pvlan_rule2, 0: pvlan_rule1 == pvlan_rule2, 1: pvlan_rule1 > pvlan_rule2
static int
classf_protocol_vlan_compare_rule(struct classf_pvlan_rule_t *rule1,
	struct classf_pvlan_rule_t *rule2)
{
//	unsigned int ret;

	//check uni port
	if (rule1->uniport != rule2->uniport)
	{
		if (rule1->uniport < rule2->uniport)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//check order
	if (rule1->pvlan_acl.order != rule2->pvlan_acl.order)
	{
		if (rule1->pvlan_acl.order < rule2->pvlan_acl.order)
		{
			return -1;
		} else {
			return 1;
		}
	} else {
		return 1; //according original list order
	}

#if 0
	//check rule,
	if ((ret = omciutil_vlan_compare_rule_fields(&rule1->rule_fields, &rule2->rule_fields)) != 0)
	{
		return ret;
	}
#endif

	return 0;
}

//insert rule_candi to list by increasing order bridge lan port, then rule field, the same one was also inserted by original order
int
classf_protocol_vlan_insert_rule_to_list(struct classf_pvlan_rule_t *pvlan_rule,
	struct list_head *pvlan_rule_list)
{
	int ret;
	struct classf_pvlan_rule_t *pvlan_rule_pos;
	struct list_head *insert_point;
	
	insert_point = pvlan_rule_list;

	list_for_each_entry(pvlan_rule_pos, pvlan_rule_list, pr_node)
	{
		if ((ret = classf_protocol_vlan_compare_rule(pvlan_rule, pvlan_rule_pos)) == 0)
		{
			//the same, do nothing , move to next one
		} else {
			if (ret < 0) {
				break;
			}
			//otherwise, check next one
		}
		insert_point = &pvlan_rule_pos->pr_node;
	}

	//insert;
	list_add(&pvlan_rule->pr_node, insert_point);

	return 0;
}

static int
classf_protocol_vlan_fill_reversing_rule(struct classf_pvlan_rule_t *pvlan_rule, struct classf_pvlan_rule_t *pvlan_rule_reversing)
{
	if (pvlan_rule == NULL ||
		pvlan_rule_reversing == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//reverse
	//common part
	pvlan_rule_reversing->enable = pvlan_rule->enable;
	pvlan_rule_reversing->uniport = pvlan_rule->uniport;
	pvlan_rule_reversing->uniport_valid = pvlan_rule->uniport_valid;
	pvlan_rule_reversing->pvlan_acl.rule_num = pvlan_rule->pvlan_acl.rule_num;
	pvlan_rule_reversing->pvlan_acl.order = pvlan_rule->pvlan_acl.order;
	pvlan_rule_reversing->pvlan_acl.uniport_wildcard = pvlan_rule->pvlan_acl.uniport_wildcard;
	pvlan_rule_reversing->pvlan_acl.uniport_wildcard_range = pvlan_rule->pvlan_acl.uniport_wildcard_range;
	pvlan_rule_reversing->join_disable = pvlan_rule->join_disable;

	//ethertype
	pvlan_rule_reversing->pvlan_acl.care_bit.ethertype = pvlan_rule->pvlan_acl.care_bit.ethertype;
	pvlan_rule_reversing->pvlan_acl.ethertype = pvlan_rule->pvlan_acl.ethertype;

	//smac -> dmac
	if (pvlan_rule->pvlan_acl.care_bit.smac)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dmac = 1;
		memcpy(pvlan_rule_reversing->pvlan_acl.dmac, pvlan_rule->pvlan_acl.smac, sizeof(pvlan_rule->pvlan_acl.smac));
	}
	//smac_mask -> dmac_mask
	if (pvlan_rule->pvlan_acl.care_bit.smac_mask)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dmac_mask = 1;
		memcpy(pvlan_rule_reversing->pvlan_acl.dmac_mask, pvlan_rule->pvlan_acl.smac_mask, sizeof(pvlan_rule->pvlan_acl.smac_mask));
	}
	//dmac -> smac
	if (pvlan_rule->pvlan_acl.care_bit.dmac)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.smac = 1;
		memcpy(pvlan_rule_reversing->pvlan_acl.smac, pvlan_rule->pvlan_acl.dmac, sizeof(pvlan_rule->pvlan_acl.dmac));
	}
	//dmac_mask -> smac_mask
	if (pvlan_rule->pvlan_acl.care_bit.dmac_mask)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.smac_mask = 1;
		memcpy(pvlan_rule_reversing->pvlan_acl.smac_mask, pvlan_rule->pvlan_acl.dmac_mask, sizeof(pvlan_rule->pvlan_acl.dmac_mask));
	}

	//sip -> dip
	if (pvlan_rule->pvlan_acl.care_bit.sip)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dip = 1;
		pvlan_rule_reversing->pvlan_acl.dip = pvlan_rule->pvlan_acl.sip;
	}
	//sip_mask -> dip_mask
	if (pvlan_rule->pvlan_acl.care_bit.sip_mask)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dip_mask = 1;
		pvlan_rule_reversing->pvlan_acl.dip_mask = pvlan_rule->pvlan_acl.sip_mask;
	}
	//dip -> sip
	if (pvlan_rule->pvlan_acl.care_bit.dip)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.sip = 1;
		pvlan_rule_reversing->pvlan_acl.sip = pvlan_rule->pvlan_acl.dip;
	}
	//dip_mask -> sip_mask
	if (pvlan_rule->pvlan_acl.care_bit.dip_mask)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.sip_mask = 1;
		pvlan_rule_reversing->pvlan_acl.sip_mask = pvlan_rule->pvlan_acl.dip_mask;
	}	

	//v6sip -> v6dip
	if (pvlan_rule->pvlan_acl.care_bit.v6sip)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.v6dip = 1;
		pvlan_rule_reversing->pvlan_acl.v6dip = pvlan_rule->pvlan_acl.v6sip;
	}
	//v6sip_prefix -> v6dip_prefix
	if (pvlan_rule->pvlan_acl.care_bit.v6sip_prefix)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.v6dip_prefix = 1;
		pvlan_rule_reversing->pvlan_acl.v6dip_prefix = pvlan_rule->pvlan_acl.v6sip_prefix;
	}
	//v6dip -> v6sip
	if (pvlan_rule->pvlan_acl.care_bit.v6dip)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.v6sip = 1;
		pvlan_rule_reversing->pvlan_acl.v6sip = pvlan_rule->pvlan_acl.v6dip;
	}
	//v6dip_prefix -> v6sip_prefix
	if (pvlan_rule->pvlan_acl.care_bit.v6dip_prefix)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.v6sip_prefix = 1;
		pvlan_rule_reversing->pvlan_acl.v6sip_prefix = pvlan_rule->pvlan_acl.v6dip_prefix;
	}
	
	//protocol
	pvlan_rule_reversing->pvlan_acl.care_bit.protocol = pvlan_rule->pvlan_acl.care_bit.protocol;
	pvlan_rule_reversing->pvlan_acl.protocol = pvlan_rule->pvlan_acl.protocol;

	//v6nd_target_ip
	pvlan_rule_reversing->pvlan_acl.care_bit.v6nd_target_ip = pvlan_rule->pvlan_acl.care_bit.v6nd_target_ip;
	pvlan_rule_reversing->pvlan_acl.v6nd_target_ip = pvlan_rule->pvlan_acl.v6nd_target_ip;

	//sport -> dport
	if (pvlan_rule->pvlan_acl.care_bit.sport)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dport = 1;
		pvlan_rule_reversing->pvlan_acl.dport = pvlan_rule->pvlan_acl.sport;
	}
	//sport_ubound -> dport_ubound
	if (pvlan_rule->pvlan_acl.care_bit.sport_ubound)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.dport_ubound = 1;
		pvlan_rule_reversing->pvlan_acl.dport_ubound = pvlan_rule->pvlan_acl.sport_ubound;
	}
	//dport -> sport
	if (pvlan_rule->pvlan_acl.care_bit.dport)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.sport = 1;
		pvlan_rule_reversing->pvlan_acl.sport = pvlan_rule->pvlan_acl.dport;
	}
	//dport_ubound -> sport_ubound
	if (pvlan_rule->pvlan_acl.care_bit.dport_ubound)
	{
		pvlan_rule_reversing->pvlan_acl.care_bit.sport_ubound = 1;
		pvlan_rule_reversing->pvlan_acl.sport_ubound = pvlan_rule->pvlan_acl.dport_ubound;
	}

	//inherit flow_vid
	pvlan_rule_reversing->pvlan_acl.flow_vid = pvlan_rule->pvlan_acl.flow_vid;

	//assigned rule_fields property
	pvlan_rule_reversing->rule_fields.bport_me = pvlan_rule->rule_fields.bport_me;
	pvlan_rule_reversing->rule_fields.owner_me = pvlan_rule->rule_fields.owner_me;
	pvlan_rule_reversing->rule_fields.sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
	pvlan_rule_reversing->rule_fields.unreal_flag = 0; //real 

	//revser rule_fields
	if (omciutil_vlan_reverse_rule_generate(&pvlan_rule->rule_fields, &pvlan_rule_reversing->rule_fields) != 0)
	{
		dbprintf(LOG_ERR, "pvlan rule tagging rule_fields reversing error, pvlan rule num = %u!!\n", pvlan_rule->pvlan_acl.rule_num);
		return -1;
	}

	return 0;
}

static int
classf_protocol_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(struct classf_pvlan_rule_t *pvlan_rule_reversing,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct list_head *ds_pvlan_rule_list)
{
	unsigned int i;
	unsigned char pbit_mask;

	struct classf_pvlan_rule_t *pvlan_rule_reversing_new;

	if (pvlan_rule_reversing == NULL || dscp2pbit_info == NULL || ds_pvlan_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	if (dscp2pbit_info->enable == 1 &&
		dscp2pbit_info->pbit_mask != 0 &&
		(pvlan_rule_reversing->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
			pvlan_rule_reversing->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)) //enable
	{
		//check
		if (dscp2pbit_info->pbit_mask == 0xff) //all pbit, set filter to don't care
		{
			pvlan_rule_reversing_new = classf_alloc_pvlan_rule();
			*pvlan_rule_reversing_new = *pvlan_rule_reversing;

			if (pvlan_rule_reversing->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				pvlan_rule_reversing_new->rule_fields.filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			}
			if (pvlan_rule_reversing->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				pvlan_rule_reversing_new->rule_fields.filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			}

			//insert to downstream
			classf_protocol_vlan_insert_rule_to_list(pvlan_rule_reversing_new, ds_pvlan_rule_list);
		} else {
			for (i = 0; i < 8; i++)
			{
				pbit_mask = (0x1 << i);
				if ((pbit_mask & dscp2pbit_info->pbit_mask)) // add this pbit for filter
				{
					pvlan_rule_reversing_new = classf_alloc_pvlan_rule();
					*pvlan_rule_reversing_new = *pvlan_rule_reversing;

					if (pvlan_rule_reversing->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
					{
						pvlan_rule_reversing_new->rule_fields.filter_inner.priority = i;
					}
					if (pvlan_rule_reversing->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
					{
						pvlan_rule_reversing_new->rule_fields.filter_outer.priority = i;
					}

					//insert to downstream
					classf_protocol_vlan_insert_rule_to_list(pvlan_rule_reversing_new, ds_pvlan_rule_list);
				}
			}
		}
	} else {
		dbprintf(LOG_ERR, "pvlan dscp-2-pbit not enable for rule expanding\n");
		return -1;
	}

	return 0;
}

//always be ctag
static int
classf_protocol_vlan_translate_filter_to_treatment_tag(struct omciutil_vlan_tag_fields_t *old_filter,
	struct omciutil_vlan_tag_fields_t *new_treatment,
	unsigned short vlan_mark,
	unsigned char pbit_mark)
{
	//check input
	if (old_filter == NULL || new_treatment == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//priority
	switch (old_filter->priority)
	{
	case OMCIUTIL_VLAN_FILTER_PRIORITY_0:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_1:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_2:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_3:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_4:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_5:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_6:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_7:
		new_treatment->priority = old_filter->priority;
		break;
	default:
		//assign pbit mark in pvlan rule
		new_treatment->priority = pbit_mark;
	}

	//vid
	if (old_filter->vid <= 4095)
	{
		new_treatment->vid = old_filter->vid;
	} else {
		//assign vlan mark in pvlan rule
		new_treatment->vid = vlan_mark;
	}

	//one tag filter, force to assign tpid/de as tpid = 0x8100, fixme if otherwise!!
	new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;

	return 0;
}

//only support one filter rule
static int
classf_protocol_vlan_translate_treatment_to_treatment_tag(struct omciutil_vlan_tag_fields_t *old_treatment,
	struct omciutil_vlan_tag_fields_t *old_filter_inner,
	struct omciutil_vlan_tag_fields_t *new_treatment,
	unsigned short vlan_mark,
	unsigned char pbit_mark,
	unsigned char is_outer) //0: inner, 1: outer
{
	//check input
	if (old_treatment == NULL || old_filter_inner == NULL || new_treatment == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//priority
	switch (old_treatment->priority)
	{
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
		new_treatment->priority = old_treatment->priority;
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
		//from inner
		switch (old_filter_inner->priority)
		{
		case OMCIUTIL_VLAN_FILTER_PRIORITY_0:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_1:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_2:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_3:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_4:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_5:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_6:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_7:
			new_treatment->priority = old_filter_inner->priority;
			break;
		default:
			new_treatment->priority = pbit_mark;
		}
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_10: //dscp-2-pbit
		new_treatment->priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_10;
		break;
	default:
		new_treatment->priority = pbit_mark;
	}

	//vid
	if (old_treatment->vid <= 4095)
	{
		new_treatment->vid = old_treatment->vid;
	} else if (old_treatment->vid == 4096) {
		//from inner
		if (old_filter_inner->vid <= 4095)
		{
			new_treatment->vid = old_filter_inner->vid;
		} else {
			new_treatment->vid = vlan_mark;
		}
	} else {
		new_treatment->vid = vlan_mark;
	}

	//tpid/de
	if (is_outer == 0)
	{
		//force to assign tpid/de as tpid = 0x8100, fixme if otherwise!!
		new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
	} else {
		//force to assign tpid/de as output tpid, de = 0, fixme if otherwise!!
		new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_110;
	}

	return 0;
}

static int
classf_protocol_vlan_rule_join_rule_fields(struct classf_pvlan_rule_t *pvlan_rule,
	struct omciutil_vlan_rule_fields_t *rule_fields)
{
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;
	
	if (pvlan_rule == NULL ||
		rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//assign input and output tpid
	pvlan_rule->rule_fields.input_tpid = rule_fields->input_tpid;
	pvlan_rule->rule_fields.output_tpid = rule_fields->output_tpid;

	//match, generate result list
	if (omciutil_vlan_generate_rule_result(rule_fields, &rule_result_list) < 0)
	{
		dbprintf(LOG_ERR, "could not get rule result\n");
		return -1;
	}

	//rule filter num was <= 1, this was join important assumption.
	//according the result tag count, generate the treatement of the rule by the result
	switch (rule_result_list.tag_count)
	{
	case 0:
		//do notthing, untag pass rule, necessary fields have filled outside this function while rule_fields creating.
		break;
	case 1:
		//pop-up the top-most one tag for treatment inner
		result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);

		switch (result_node->code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			classf_protocol_vlan_translate_filter_to_treatment_tag(
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_inner,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark);
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			classf_protocol_vlan_translate_treatment_to_treatment_tag(
				&rule_fields->treatment_inner,
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_inner,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark,
				0);
			break;
		default:
			dbprintf(LOG_ERR, "result code unreasonable!!\n");
			return -1;
		}

		break;
	case 2:
		//pop-up the top-most one tag for treatment outer
		result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);

		switch (result_node->code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
			classf_protocol_vlan_translate_treatment_to_treatment_tag(
				&rule_fields->treatment_outer,
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_outer,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark,
				1);

			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			classf_protocol_vlan_translate_treatment_to_treatment_tag(
				&rule_fields->treatment_inner,
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_outer,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark,
				1);
			break;
		default:
			dbprintf(LOG_ERR, "result code unreasonable\n");
			return -1;
		}

		//pop-up the second tag for treatment inner
		result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);

		switch (result_node->code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			classf_protocol_vlan_translate_filter_to_treatment_tag(
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_inner,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark);
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			classf_protocol_vlan_translate_treatment_to_treatment_tag(
				&rule_fields->treatment_inner,
				&rule_fields->filter_inner,
				&pvlan_rule->rule_fields.treatment_inner,
				pvlan_rule->vlan_mark,
				pvlan_rule->pbit_mark,
				0);
			break;
		default:
			dbprintf(LOG_ERR, "result code unreasonable\n");
			return -1;
		}
	default: // > 2
		dbprintf(LOG_ERR, "result tag count unreasonable\n");
		return -1;
	}

	//set join entry id
	pvlan_rule->join_entry_id = rule_fields->entry_id;

	//release the result
	omciutil_vlan_release_rule_result(&rule_result_list);
	
	return 0;
}

static int
classf_protocol_vlan_rule_join_1tag(struct classf_pvlan_rule_t *pvlan_rule,
	struct list_head *tagging_rule_list)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct omciutil_vlan_rule_fields_t *rule_fields_join;
	int vlan_match, ethertype_match, pvlan_rule_ethertype, rule_ethertype;
	unsigned short vlan_mark;
	unsigned char pbit_mark;

	unsigned join_prefer; //without ethertype join: 0, with ethertype join: 1

	if (pvlan_rule == NULL ||
		tagging_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//init
	rule_fields_join = NULL;
	join_prefer = 0;

/*
pvlan_rule_ethertype: 0: non-pppoe, 1: pppoe
rule_ethertype: 0: don't care, 1: non-pppoe, 2: pppoe

ethertype_match determination:
pvlan_rule_ethertype  rule_ethertype  ethertype_match
                0                     0                       0
                0                     1                       1
                0                     2                       0
                1                     0                       0
                1                     1                       0
                1                     2                       1
*/
	if (pvlan_rule->pvlan_acl.care_bit.ethertype == 1 &&
		(pvlan_rule->pvlan_acl.ethertype == 0x8863 ||
		pvlan_rule->pvlan_acl.ethertype == 0x8864))
	{
		pvlan_rule_ethertype = 1; //pppoe
	} else {
		pvlan_rule_ethertype = 0; //non-pppoe
	}

	//decide vlan_mark and pbit_mark
	if (pvlan_rule->vlan_mark == 4096) //copy from inner
	{
		if (pvlan_rule->rule_fields.filter_inner.vid == 4096) //vid don't care
		{
			vlan_mark = 4096;
		} else {
			vlan_mark = pvlan_rule->rule_fields.filter_inner.vid;
		}
	} else { // pvlan_rule->vlan_mark < 4096
		vlan_mark = pvlan_rule->vlan_mark;
	}

	if (pvlan_rule->pbit_mark == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8) //copy from inner
	{
		if (pvlan_rule->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8) //pbit don't care
		{
			pbit_mark = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
		} else {
			pbit_mark = pvlan_rule->rule_fields.filter_inner.priority;
		}
	} else { // pvlan_rule->pbit_mark < 8
		pbit_mark = pvlan_rule->pbit_mark;
	}

	
	list_for_each_entry(rule_fields, tagging_rule_list, rule_node)
	{
		vlan_match = 0;
		ethertype_match= 0;
		
		//check filter number
		if (omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 &&
			rule_fields->treatment_tag_to_remove <= 1 && //skip drop and unreasonable rules
			((rule_fields->filter_inner.vid == vlan_mark || //vid and priority hit
				rule_fields->filter_inner.vid == 4096) && //don't care
				(rule_fields->filter_inner.priority == pbit_mark ||
				rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 || //don't care
				rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)))
		{
			vlan_match = 1;
		}

		//only vlan match could go to check ethertype match,
		//and was the candicate to join with pvlan rule
		if (vlan_match)
		{
			switch (rule_fields->filter_ethertype)
			{
				case OMCIUTIL_VLAN_FILTER_ET_PPPOE:
				case OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6:
					rule_ethertype = 2; //pppoe
					break;
				case OMCIUTIL_VLAN_FILTER_ET_IP:
				case OMCIUTIL_VLAN_FILTER_ET_ARP:
				case OMCIUTIL_VLAN_FILTER_ET_IPV6:
				case OMCIUTIL_VLAN_FILTER_ET_IPV6_ZTE:
				case OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6:
				case OMCIUTIL_VLAN_FILTER_ET_IP_ARP:
					rule_ethertype = 1; //non-pppoe
					break;
				default:
					rule_ethertype = 0; //don't care
			}

			//decide ethertype_match
			if (pvlan_rule_ethertype == 0 && rule_ethertype == 0)
			{
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 0) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 1) {
				ethertype_match = 1;
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 2) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 0) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 1) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 2) {
				ethertype_match = 1;
			}

			if (rule_fields_join == NULL)
			{
				rule_fields_join = rule_fields;
				join_prefer = ethertype_match;
			} else {
				if (ethertype_match > join_prefer)
				{
					//update rule_fields_join
					rule_fields_join = rule_fields;
					join_prefer = ethertype_match;
				}
			}
		}
	}

	if (rule_fields_join != NULL)
	{
		return classf_protocol_vlan_rule_join_rule_fields(pvlan_rule, rule_fields_join);
	} else {
		return -1;
	}
}

static int
classf_protocol_vlan_rule_join_untag(struct classf_pvlan_rule_t *pvlan_rule,
	struct list_head *tagging_rule_list)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct omciutil_vlan_rule_fields_t *rule_fields_join;
	int vlan_match, ethertype_match, pvlan_rule_ethertype, rule_ethertype;

	unsigned join_prefer; //without ethertype join: 0, with ethertype join: 1

	if (pvlan_rule == NULL ||
		tagging_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//init
	rule_fields_join = NULL;
	join_prefer = 0;

/*
pvlan_rule_ethertype: 0: non-pppoe, 1: pppoe
rule_ethertype: 0: don't care, 1: non-pppoe, 2: pppoe

ethertype_match determination:
pvlan_rule_ethertype  rule_ethertype  ethertype_match
                0                     0                       0
                0                     1                       1
                0                     2                       0
                1                     0                       0
                1                     1                       0
                1                     2                       1
*/
	if (pvlan_rule->pvlan_acl.care_bit.ethertype == 1 &&
		(pvlan_rule->pvlan_acl.ethertype == 0x8863 ||
		pvlan_rule->pvlan_acl.ethertype == 0x8864))
	{
		pvlan_rule_ethertype = 1; //pppoe
	} else {
		pvlan_rule_ethertype = 0; //non-pppoe
	}
	
	list_for_each_entry(rule_fields, tagging_rule_list, rule_node)
	{
		vlan_match = 0;
		ethertype_match= 0;
		
		//check filter number
		if (omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 0 &&
			rule_fields->treatment_tag_to_remove == 0)//skip drop and unreasonable rules
		{
			vlan_match = 1;
		}

		//only vlan match could go to check ethertype match,
		//and was the candicate to join with pvlan rule
		if (vlan_match)
		{
			switch (rule_fields->filter_ethertype)
			{
				case OMCIUTIL_VLAN_FILTER_ET_PPPOE:
				case OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6:
					rule_ethertype = 2; //pppoe
					break;
				case OMCIUTIL_VLAN_FILTER_ET_IP:
				case OMCIUTIL_VLAN_FILTER_ET_ARP:
				case OMCIUTIL_VLAN_FILTER_ET_IPV6:
				case OMCIUTIL_VLAN_FILTER_ET_IPV6_ZTE:
				case OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6:
				case OMCIUTIL_VLAN_FILTER_ET_IP_ARP:
					rule_ethertype = 1; //non-pppoe
					break;
				default:
					rule_ethertype = 0; //don't care
			}

			//decide ethertype_match
			if (pvlan_rule_ethertype == 0 && rule_ethertype == 0)
			{
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 0) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 1) {
				ethertype_match = 1;
			} else if (pvlan_rule_ethertype == 0 && rule_ethertype == 2) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 0) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 1) {
				ethertype_match = 0;
			} else if (pvlan_rule_ethertype == 1 && rule_ethertype == 2) {
				ethertype_match = 1;
			}

			if (rule_fields_join == NULL)
			{
				rule_fields_join = rule_fields;
				join_prefer = ethertype_match;
			} else {
				if (ethertype_match > join_prefer)
				{
					//update rule_fields_join
					rule_fields_join = rule_fields;
					join_prefer = ethertype_match;
				}
			}
		}
	}

	if (rule_fields_join != NULL)
	{
		return classf_protocol_vlan_rule_join_rule_fields(pvlan_rule, rule_fields_join);
	} else {
		return -1;
	}
}

static int
classf_protocol_vlan_rule_join(struct classf_pvlan_rule_t *pvlan_rule,
	struct list_head *tagging_rule_list)
{
	if (pvlan_rule == NULL ||
		tagging_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (pvlan_rule->vlan_mark <= 4096)
	{
		//involve 1tag join
		return classf_protocol_vlan_rule_join_1tag(pvlan_rule, tagging_rule_list);
	} else if (pvlan_rule->vlan_mark == CLASSF_VLAN_UNTAG_ID) {
		//involve untag join
		return classf_protocol_vlan_rule_join_untag(pvlan_rule, tagging_rule_list);
	} else {
		//error
		dbprintf(LOG_ERR, "unnecessary pvlan rule join, order=%d\n", pvlan_rule->pvlan_acl.order);
		return -1;
	}
}

void
classf_protocol_vlan_rule_generate_by_port(struct classf_bridge_info_lan_port_t *lan_port,
	struct list_head *veip_rule_list, //maybe NULL
	struct list_head *global_pvlan_rule_list,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	unsigned int count;
	struct classf_pvlan_rule_t *pvlan_rule_pos, *pvlan_rule_n;
	struct classf_pvlan_rule_t *pvlan_rule_new;
	struct classf_pvlan_rule_t *pvlan_rule_reversing;
	struct list_head rule_list_ds_temp;

	if (lan_port == NULL ||
		global_pvlan_rule_list == NULL ||
		dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	INIT_LIST_HEAD(&rule_list_ds_temp);

	list_for_each_entry_safe(pvlan_rule_pos, pvlan_rule_n, global_pvlan_rule_list, pr_node)
	{
		//retrive from global list, move to upstream list
		if (pvlan_rule_pos->enable &&
			pvlan_rule_pos->uniport_valid &&
			((pvlan_rule_pos->pvlan_acl.uniport_wildcard == 0 &&
			pvlan_rule_pos->uniport == lan_port->logical_port_id) ||
			(pvlan_rule_pos->pvlan_acl.uniport_wildcard > 0 &&
			lan_port->logical_port_id <= pvlan_rule_pos->pvlan_acl.uniport_wildcard_range)))
		{
			if (pvlan_rule_pos->pvlan_acl.uniport_wildcard > 0)
			{
				//copy from global list, add to upstream list, enable join for the pvlan only at first time.
				pvlan_rule_new = classf_alloc_pvlan_rule();
				*pvlan_rule_new = *pvlan_rule_pos;
				if (pvlan_rule_pos->join_disable == 0)
				{
					pvlan_rule_pos->join_disable = 1;
				}
			} else {
				//del from global list, add to upstream list.
				list_del(&pvlan_rule_pos->pr_node);
				pvlan_rule_new = pvlan_rule_pos;
			}
			
			//assigned rule_fields property
			pvlan_rule_new->rule_fields.bport_me = lan_port->me_bridge_port;
			pvlan_rule_new->rule_fields.owner_me = NULL;
			pvlan_rule_new->rule_fields.sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			pvlan_rule_new->rule_fields.unreal_flag = 0; //real

			//fill one tag match, and remove this tag.
			//this is the definitaion of protocol vlan, then add tag by marking or join.
			if (pvlan_rule_new->vlantag_enable == 1)
			{		
				pvlan_rule_new->rule_fields.filter_inner.priority = pvlan_rule_new->vlantag_pbit;
				pvlan_rule_new->rule_fields.filter_inner.vid= pvlan_rule_new->vlantag_id;

				//remove this tag definitely
				pvlan_rule_new->rule_fields.treatment_tag_to_remove = 1;
			}

			//join
			if (!(veip_rule_list != NULL && 
				!list_empty(veip_rule_list) &&
				pvlan_rule_new->vlan_mark <= 4096 && //mark vid <= 4096
				pvlan_rule_new->pbit_mark != OMCIUTIL_VLAN_TREATMENT_PRIORITY_10) || //mark pri not 10(to be done, maybe)
				classf_protocol_vlan_rule_join(pvlan_rule_new, veip_rule_list) < 0) //join fail
			{
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
				{
					//did not add this to any port
					continue;
				} 

				if (pvlan_rule_new->vlan_mark == CLASSF_VLAN_UNTAG_ID) //not add a tag
				{
					//did not add a tag
					pvlan_rule_new->rule_fields.treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
					pvlan_rule_new->rule_fields.treatment_inner.vid = 0;
					pvlan_rule_new->rule_fields.treatment_inner.tpid_de = 0;	
				} else {
					//add a tag, convert to vlan mark and pbit mark rule directly
					pvlan_rule_new->rule_fields.treatment_inner.priority = pvlan_rule_new->pbit_mark;
					pvlan_rule_new->rule_fields.treatment_inner.vid = pvlan_rule_new->vlan_mark;

					//assign tpid_de to 0x8100
					pvlan_rule_new->rule_fields.treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;

					//untag, transparent, reset the inner treatment
					if (pvlan_rule_new->vlantag_enable != 1 &&
						(pvlan_rule_new->vlan_mark == 4096 ||
						pvlan_rule_new->pbit_mark == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8))
					{
						//reset
						pvlan_rule_new->rule_fields.treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
						pvlan_rule_new->rule_fields.treatment_inner.vid = 0;
						pvlan_rule_new->rule_fields.treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;				
					}
				}
			}

			//set flow_vid
			pvlan_rule_new->pvlan_acl.flow_vid = pvlan_rule_new->vlan_mark;

			//insert to upstream
			classf_protocol_vlan_insert_rule_to_list(pvlan_rule_new, &lan_port->port_protocol_vlan.us_pvlan_rule_list); 

			//reversing, add to downstream list
			pvlan_rule_reversing = classf_alloc_pvlan_rule();
			if (classf_protocol_vlan_fill_reversing_rule(pvlan_rule_new, pvlan_rule_reversing) == 0)
			{
				//check for dscp-2-pbit reversing
				if (pvlan_rule_reversing->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
					pvlan_rule_reversing->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
				{
					//expand dscp-2-pbit bitmap
					if (classf_protocol_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(pvlan_rule_reversing, dscp2pbit_info, &lan_port->port_protocol_vlan.ds_pvlan_rule_list) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
						classf_protocol_vlan_insert_rule_to_list(pvlan_rule_reversing, &lan_port->port_protocol_vlan.ds_pvlan_rule_list);
					} else {
						//expand ok, free original one
						free_safe(pvlan_rule_reversing);
					}
				} else {
					//insert to downstream
					classf_protocol_vlan_insert_rule_to_list(pvlan_rule_reversing, &lan_port->port_protocol_vlan.ds_pvlan_rule_list);
				}
			}else {
				free_safe(pvlan_rule_reversing);
			}
		}
	}

	if ((omci_env_g.tag_pbit_workaround != 0) &&
		((1 << lan_port->logical_port_id) & omci_env_g.tag_pbit_portmask))
	{
		switch (omci_env_g.tag_pbit_workaround)
		{
		case 1:
		case 2:
			list_for_each_entry(pvlan_rule_pos, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
			{
				if (omci_env_g.tag_pbit_workaround == 1)
				{
					//for pbit don't care
					omciutil_vlan_pbit_dont_care_workaround(&pvlan_rule_pos->rule_fields);
				} else {
					//for remap pbit
					omciutil_vlan_pbit_remap_workaround(&pvlan_rule_pos->rule_fields);
				}
			}
			break;
		case 3: //add one more don't care rule
		case 4: //add one more transparent rule
			list_replace_init(&lan_port->port_protocol_vlan.ds_pvlan_rule_list, &rule_list_ds_temp);
			list_for_each_entry_safe(pvlan_rule_pos, pvlan_rule_n, &rule_list_ds_temp, pr_node)
			{
				list_del(&pvlan_rule_pos->pr_node);
						
				pvlan_rule_new = classf_alloc_pvlan_rule();
				*pvlan_rule_new = *pvlan_rule_pos;

				if (omci_env_g.tag_pbit_workaround == 3)
				{
					//modify for pbit don't care
					omciutil_vlan_pbit_dont_care_workaround(&pvlan_rule_new->rule_fields);
				} else { 
					//modify for pbit transparent
					omciutil_vlan_pbit_dont_care_workaround_transparent(&pvlan_rule_new->rule_fields);
				}

				//add original one to ds_list
				classf_protocol_vlan_insert_rule_to_list(pvlan_rule_pos, &lan_port->port_protocol_vlan.ds_pvlan_rule_list);
				
				//add new one to ds_list
				classf_protocol_vlan_insert_rule_to_list(pvlan_rule_new, &lan_port->port_protocol_vlan.ds_pvlan_rule_list);
			}
			break;
		default:
			;
		}
	}
	
	//assign seq num to each rule_field for us/ds pvlan rule lists, 
	count = 0;
	list_for_each_entry(pvlan_rule_pos, &lan_port->port_protocol_vlan.us_pvlan_rule_list, pr_node)
	{
		pvlan_rule_pos->rule_fields.entry_id = count++;
	}
	count = 0;
	list_for_each_entry(pvlan_rule_pos, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
	{
		pvlan_rule_pos->rule_fields.entry_id = count++;
	}

	return;
}

