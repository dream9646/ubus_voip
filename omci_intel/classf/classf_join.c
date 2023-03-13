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
 * File    : classf_join.c
 *
 ******************************************************************/
 
#include <string.h>

#include "classf.h"
#include "util.h"
#include "meinfo.h"

static struct classf_rule_candi_t *
classf_alloc_rule_candi(void)
{
	struct classf_rule_candi_t *rule_candi;

	rule_candi = malloc_safe(sizeof(struct classf_rule_candi_t));

	return rule_candi;
}

int
classf_release_rule_candi_list(struct list_head *rule_candi_list)
{
	struct classf_rule_candi_t *rule_candi_pos, *rule_candi_n;

	if (rule_candi_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	list_for_each_entry_safe(rule_candi_pos, rule_candi_n, rule_candi_list, rc_node)
	{
		list_del(&rule_candi_pos->rc_node);
		free_safe(rule_candi_pos);
	}

	return 0;
}

//return value: -1: rule_candi1 < rule_candi2, 0: rule_candi1 == rule_candi2, 1: rule_candi1 > rule_candi2
static int
classf_compare_rule_candi(struct classf_rule_candi_t *rule_candi1,
	struct classf_rule_candi_t *rule_candi2)
{
	unsigned int ret;
	struct classf_pvlan_acl_care_bit_t care_bit = {0};

	//check lan port
	if (rule_candi1->lan_port->port_type_sort != rule_candi2->lan_port->port_type_sort)
	{
		if (rule_candi1->lan_port->port_type_sort < rule_candi2->lan_port->port_type_sort)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_candi1->lan_port->port_type_index_sort != rule_candi2->lan_port->port_type_index_sort)
	{
		if (rule_candi1->lan_port->port_type_index_sort < rule_candi2->lan_port->port_type_index_sort)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//compare pvlan acl, according the order value
	if (rule_candi1->pvlan_acl != rule_candi2->pvlan_acl)
	{
		if (rule_candi1->pvlan_acl == NULL)
		{
			return 1;
		}
		if (rule_candi2->pvlan_acl == NULL)
		{
			return -1;
		}
	}
	
	//all pvlan_acl not NULL
	if (rule_candi1->pvlan_acl != NULL &&
		rule_candi2->pvlan_acl != NULL)
	{	
		if (memcmp(&rule_candi1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0 &&
			memcmp(&rule_candi2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0)
		{
			return 1;
		} else if (memcmp(&rule_candi1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0 &&
			memcmp(&rule_candi2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0) {
			return -1;
		} else if (memcmp(&rule_candi1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0 &&
			memcmp(&rule_candi2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0) {
			if (rule_candi1->pvlan_acl->order != rule_candi2->pvlan_acl->order)
			{
				if (rule_candi1->pvlan_acl->order < rule_candi2->pvlan_acl->order)
				{
					return -1;
				} else {
					return 1;
				}
			} else {
				return 1; //according original list order
			}
		} else if (memcmp(&rule_candi1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0 &&
			memcmp(&rule_candi2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0) {
			if (rule_candi1->pvlan_acl->order != rule_candi2->pvlan_acl->order)
			{
				if (rule_candi1->pvlan_acl->order < rule_candi2->pvlan_acl->order)
				{
					return -1;
				} else {
					return 1;
				}
			} else {
				return 1; //according original list order
			}
		}
	}

	//check tagging rule,
	if ((ret = omciutil_vlan_compare_rule_fields(rule_candi1->rule_fields, rule_candi2->rule_fields)) != 0)
	{
		return ret;
	}

	//check lan filter
	if (rule_candi1->lan_filter_valid != rule_candi2->lan_filter_valid)
	{
		if (rule_candi1->lan_filter_valid < rule_candi2->lan_filter_valid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_candi1->lan_filter_type != rule_candi2->lan_filter_type)
	{
		if (rule_candi1->lan_filter_type < rule_candi2->lan_filter_type)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//check wan port,
	if (rule_candi1->wan_port->port_type_index != rule_candi2->wan_port->port_type_index)
	{
		if (rule_candi1->wan_port->port_type_index < rule_candi2->wan_port->port_type_index)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//check lan filter
	if (rule_candi1->wan_filter_valid != rule_candi2->wan_filter_valid)
	{
		if (rule_candi1->wan_filter_valid < rule_candi2->wan_filter_valid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_candi1->wan_filter_type != rule_candi2->wan_filter_type)
	{
		if (rule_candi1->wan_filter_type < rule_candi2->wan_filter_type)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//check gem port, by index??
	if (rule_candi1->gem_index != rule_candi2->gem_index)
	{
		if (rule_candi1->gem_index < rule_candi2->gem_index)
		{
			return -1;
		} else {
			return 1;
		}
	}

	return 0;
}

//insert rule_candi to list by increasing order bridge lan port, then rule field, reject the same one
int
classf_insert_rule_candi_to_list(struct classf_rule_candi_t *rule_candi,
	struct list_head *rule_candi_list)
{
	int ret;
	struct classf_rule_candi_t *rule_candi_pos;
	struct list_head *insert_point;
	
	insert_point = rule_candi_list;

	list_for_each_entry(rule_candi_pos, rule_candi_list, rc_node)
	{
		if ((ret = classf_compare_rule_candi(rule_candi, rule_candi_pos)) == 0)
		{
			//the same, no need to insert
			return -1;
		} else {
			if (ret < 0) {
				break;
			}
			//otherwise, check next one
		}
		insert_point = &rule_candi_pos->rc_node;
	}

	//insert;
	list_add(&rule_candi->rc_node, insert_point);

	return 0;
}

static int
classf_bridge_join_gen_lan_rule_candi_by_filter(unsigned char op,
	struct classf_bridge_info_lan_port_t *lan_port,
	struct omciutil_vlan_rule_fields_t *rule_fields,
	struct classf_pvlan_acl_t *pvlan_acl, //null, maybe
	struct list_head *rule_candi_list)
{
	int i;
	struct classf_rule_candi_t *rule_candi;

	if (lan_port == NULL ||
		rule_fields == NULL ||
		rule_candi_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//join with tag filter
	switch (op)
	{
	case CLASSF_FILTER_OP_TYPE_PASS: //pass
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		break;
	case CLASSF_FILTER_OP_TYPE_DISCARD: //discard
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_DISCARD;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		break;
	case CLASSF_FILTER_OP_TYPE_POSTIVE: //postive
		for (i = 0; i < lan_port->port_filter.vlan_tci_num; i++)
		{
			rule_candi = classf_alloc_rule_candi(); //upstream, invalid
			rule_candi->lan_port = lan_port;
			rule_candi->rule_fields = rule_fields;
			rule_candi->pvlan_acl = pvlan_acl;
			rule_candi->lan_filter_valid = 1;
			rule_candi->lan_filter_type = i + CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0;

			list_add_tail(&rule_candi->rc_node, rule_candi_list);
		}

		break;
	case CLASSF_FILTER_OP_TYPE_NEGATIVE: //negative
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		break;
	default: //pass
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);
	}

	return 0;
}

static int
classf_bridge_join_gen_us_lan_rule_candi_by_tagging_filter(
	struct classf_bridge_info_lan_port_t *lan_port,
	struct omciutil_vlan_rule_fields_t *rule_fields,
	struct classf_pvlan_acl_t *pvlan_acl, //null, maybe
	struct list_head *rule_candi_list)
{
	struct classf_rule_candi_t *rule_candi;

	if (lan_port == NULL ||
		rule_fields == NULL ||
		rule_candi_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (lan_port->port_filter.filter_me != NULL)
	{
		//join with untag filter
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		if (lan_port->port_filter.untag_rx == 1) //discard
		{
			rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD;
		} else {
			rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS;
		}

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		//join with tag filter
		if (classf_bridge_join_gen_lan_rule_candi_by_filter(lan_port->port_filter.tag_rx,
			lan_port,
			rule_fields,
			pvlan_acl,
			rule_candi_list) < 0)
		{
			dbprintf(LOG_ERR, "gem lan rule_candi by filter error, op=%u!!\n", lan_port->port_filter.tag_rx);
		}
	} else {
		//join with no filter found, set lan_filter_valid to 0
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 0;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);
	}

	return 0;
}

static int
classf_bridge_join_gen_ds_lan_rule_candi_by_tagging_filter(
	struct classf_bridge_info_lan_port_t *lan_port,
	struct omciutil_vlan_rule_fields_t *rule_fields,
	struct classf_pvlan_acl_t *pvlan_acl, //null, maybe
	struct list_head *rule_candi_list)
{
	struct classf_rule_candi_t *rule_candi;

	if (lan_port == NULL ||
		rule_fields == NULL ||
		rule_candi_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (lan_port->port_filter.filter_me != NULL)
	{
		//join with untag filter
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 1;
		if (lan_port->port_filter.untag_tx == 1) //discard
		{
			rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD;
		} else {
			rule_candi->lan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS;
		}

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		//join with tag filter
		if (classf_bridge_join_gen_lan_rule_candi_by_filter(lan_port->port_filter.tag_tx,
			lan_port,
			rule_fields,
			pvlan_acl,
			rule_candi_list) < 0)
		{
			dbprintf(LOG_ERR, "gem lan rule_candi by filter error, op=%u!!\n", lan_port->port_filter.tag_tx);
		}
	} else {
		//join with no filter found, set lan_filter_valid to 0
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->lan_port = lan_port;
		rule_candi->rule_fields = rule_fields;
		rule_candi->pvlan_acl = pvlan_acl;
		rule_candi->lan_filter_valid = 0;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);
	}

	return 0;
}

static int
classf_bridge_join_gen_wan_rule_candi_by_filter(unsigned char op,
	struct classf_bridge_info_wan_port_t *wan_port,
	unsigned char gem_index,
	struct list_head *rule_candi_list)
{
	int i;
	struct classf_rule_candi_t *rule_candi;

	if (wan_port == NULL ||
		rule_candi_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	switch (op)
	{
	case CLASSF_FILTER_OP_TYPE_PASS: //pass
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->wan_port = wan_port;
		rule_candi->gem_index = gem_index;
		rule_candi->wan_filter_valid = 1;
		rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS;
		list_add_tail(&rule_candi->rc_node, rule_candi_list);
		break;
	case CLASSF_FILTER_OP_TYPE_DISCARD: //discard
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->wan_port = wan_port;
		rule_candi->gem_index = gem_index;
		rule_candi->wan_filter_valid = 1;
		rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_DISCARD;
		list_add_tail(&rule_candi->rc_node, rule_candi_list);
		break;
	case CLASSF_FILTER_OP_TYPE_POSTIVE: //postive
		for (i = 0; i < wan_port->port_filter.vlan_tci_num; i++)
		{
			rule_candi = classf_alloc_rule_candi(); //upstream, invalid
			rule_candi->wan_port = wan_port;
			rule_candi->gem_index = gem_index;
			rule_candi->wan_filter_valid = 1;
			rule_candi->wan_filter_type = i + CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0;
			list_add_tail(&rule_candi->rc_node, rule_candi_list);
		}

		break;
	case CLASSF_FILTER_OP_TYPE_NEGATIVE: //negative
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->wan_port = wan_port;
		rule_candi->gem_index = gem_index;
		rule_candi->wan_filter_valid = 1;
		rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE;

		list_add_tail(&rule_candi->rc_node, rule_candi_list);

		break;
	default: //pass
		rule_candi = classf_alloc_rule_candi(); //upstream, invalid
		rule_candi->wan_port = wan_port;
		rule_candi->gem_index = gem_index;
		rule_candi->wan_filter_valid = 1;
		rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS;
		list_add_tail(&rule_candi->rc_node, rule_candi_list);
	}

	return 0;
}

static int
classf_bridge_join_us(struct classf_bridge_info_t *bridge_info)
{
	struct classf_bridge_info_lan_port_t *lan_port;
	struct classf_bridge_info_wan_port_t *wan_port;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct classf_pvlan_rule_t *pvlan_rule;
	struct list_head  rule_candi_list_lan, rule_candi_list_wan;
	struct classf_rule_candi_t *rule_candi, *rule_candi_wan_pos, *rule_candi_lan_pos;
	unsigned int i;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	INIT_LIST_HEAD(&rule_candi_list_lan);
	INIT_LIST_HEAD(&rule_candi_list_wan);

	//lan side
	list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
	{
		//join lan port filter and tagging rule
		list_for_each_entry(rule_fields, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
		{
			classf_bridge_join_gen_us_lan_rule_candi_by_tagging_filter(
				lan_port,
				rule_fields,
				NULL,
				&rule_candi_list_lan);
		}

		//join lan port filter and protocol vlan rule
		list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.us_pvlan_rule_list, pr_node)
		{
			if (pvlan_rule->join_disable != 0)
			{
				//skip
				continue;
			}
			
			classf_bridge_join_gen_us_lan_rule_candi_by_tagging_filter(
				lan_port,
				&pvlan_rule->rule_fields,
				&pvlan_rule->pvlan_acl,
				&rule_candi_list_lan);
		}
	}

	//wan side
	list_for_each_entry(wan_port, &bridge_info->wan_port_list, wp_node)
	{
		for (i = 0; i < wan_port->port_gem.gem_index_total; i++)
		{
			if (wan_port->port_gem.dir[i] != CLASSF_GEM_DIR_US &&
				wan_port->port_gem.dir[i] != CLASSF_GEM_DIR_BOTH) //DS, DISABLE
			{
				continue;
			}

			if (wan_port->port_gem.gem_tagged[i] == 0)
			{
				//untag
				rule_candi = classf_alloc_rule_candi(); //upstream, invalid
				rule_candi->wan_port = wan_port;
				rule_candi->gem_index = i;
				
				if (wan_port->port_filter.filter_me != NULL)
				{
					rule_candi->wan_filter_valid = 1;
					if (wan_port->port_filter.untag_tx == 1) //discard
					{
						rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD;
					} else {
						rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS;
					}
				} else {
					rule_candi->wan_filter_valid = 0;
				}

				list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);
			} else {
				//tag
				if (wan_port->port_filter.filter_me != NULL)
				{
					//join with tag filter
					if (classf_bridge_join_gen_wan_rule_candi_by_filter(wan_port->port_filter.tag_tx,
						wan_port,
						i,
						&rule_candi_list_wan) < 0)
					{
						dbprintf(LOG_ERR, "gem wan rule_candi by filter error, op=%u!!\n", wan_port->port_filter.tag_tx);
					}
				} else {
					//join with no filter found, set wan_filter_valid to 0
					rule_candi = classf_alloc_rule_candi(); //upstream, invalid
					rule_candi->wan_port = wan_port;
					rule_candi->gem_index = i;
					rule_candi->wan_filter_valid = 0;
					list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);
				}
			}
		}
	}

	//join lan-wan list
	list_for_each_entry(rule_candi_wan_pos, &rule_candi_list_wan, rc_node) //wan
	{
		list_for_each_entry(rule_candi_lan_pos, &rule_candi_list_lan, rc_node) //lan
		{
			rule_candi = classf_alloc_rule_candi(); //upstream, invalid

			//wan part
			rule_candi->wan_port = rule_candi_wan_pos->wan_port;
			rule_candi->gem_index = rule_candi_wan_pos->gem_index;
			rule_candi->wan_filter_valid = rule_candi_wan_pos->wan_filter_valid;
			rule_candi->wan_filter_type = rule_candi_wan_pos->wan_filter_type;

			//lan part
			rule_candi->lan_port = rule_candi_lan_pos->lan_port;
			rule_candi->lan_filter_valid = rule_candi_lan_pos->lan_filter_valid;
			rule_candi->lan_filter_type = rule_candi_lan_pos->lan_filter_type;
			rule_candi->rule_fields = rule_candi_lan_pos->rule_fields;
			rule_candi->pvlan_acl = rule_candi_lan_pos->pvlan_acl;

			if (classf_insert_rule_candi_to_list(rule_candi, &bridge_info->rule_candi_list_us) < 0)
			{
				dbprintf(LOG_WARNING, "the same rule candi, drop it!!\n");
				free_safe(rule_candi);
			}
		}
	}

	//release wan/lan list
	classf_release_rule_candi_list(&rule_candi_list_wan);
	classf_release_rule_candi_list(&rule_candi_list_lan);

	return 0;
}

static int
classf_bridge_join_ds(struct classf_bridge_info_t *bridge_info)
{
	struct classf_bridge_info_lan_port_t *lan_port;
	struct classf_bridge_info_wan_port_t *wan_port;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct classf_pvlan_rule_t *pvlan_rule;
	struct list_head  rule_candi_list_lan, rule_candi_list_wan;
	struct classf_rule_candi_t *rule_candi, *rule_candi_wan_pos, *rule_candi_lan_pos;
	unsigned int i;
	unsigned int ds_gem_found;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	INIT_LIST_HEAD(&rule_candi_list_lan);
	INIT_LIST_HEAD(&rule_candi_list_wan);

	//wan side
	list_for_each_entry(wan_port, &bridge_info->wan_port_list, wp_node)
	{
		if (omci_env_g.classf_ds_gem_dg == 1) //downgrade
		{
			//has downstream gem for a wan port
			ds_gem_found = 0;
			for (i = 0; i < wan_port->port_gem.gem_index_total; i++)
			{
				if (wan_port->port_gem.gem_tagged[i] == 1 && //tagged
					(wan_port->port_gem.dir[i] == CLASSF_GEM_DIR_DS ||
					wan_port->port_gem.dir[i] == CLASSF_GEM_DIR_BOTH)) //DS
				{
					ds_gem_found = 1;
					break;
				}
			}
			if (ds_gem_found)
			{
				if (wan_port->port_filter.filter_me != NULL)
				{
					//join with untag
					rule_candi = classf_alloc_rule_candi(); //downstream, invalid
					rule_candi->wan_port = wan_port;
					rule_candi->gem_index = i;
					rule_candi->wan_filter_valid = 1;
					if (wan_port->port_filter.untag_rx == 1) //discard
					{
						rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD;
					} else {
						rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS;
					}

					list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);

					//join with tag
					if (classf_bridge_join_gen_wan_rule_candi_by_filter(wan_port->port_filter.tag_rx,
						wan_port,
						i,
						&rule_candi_list_wan) < 0)
					{
						dbprintf(LOG_ERR, "gem wan rule_candi by filter error, op=%u!!\n", wan_port->port_filter.tag_rx);
					}
				} else {
					//join with no filter found, set wan_filter_valid to 0
					rule_candi = classf_alloc_rule_candi(); //upstream, invalid
					rule_candi->wan_port = wan_port;
					rule_candi->gem_index = i;
					rule_candi->wan_filter_type = 0;

					list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);
				}
			} else {
				continue;
			}
		} else {
			for (i = 0; i < wan_port->port_gem.gem_index_total; i++)
			{
				if (wan_port->port_gem.dir[i] != CLASSF_GEM_DIR_DS &&
					wan_port->port_gem.dir[i] != CLASSF_GEM_DIR_BOTH) //US, DISABLE
				{
					continue;
				}

				if (wan_port->port_gem.gem_tagged[i] == 1) //skip gem uptagged, for untag, ds gem ports do not care tag/untag and pbit
				{
					if (wan_port->port_filter.filter_me != NULL)
					{
						//join with untag
						rule_candi = classf_alloc_rule_candi(); //downstream, invalid
						rule_candi->wan_port = wan_port;
						rule_candi->gem_index = i;
						rule_candi->wan_filter_valid = 1;
						if (wan_port->port_filter.untag_rx == 1) //discard
						{
							rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD;
						} else {
							rule_candi->wan_filter_type = CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS;
						}

						list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);

						//join with tag
						if (classf_bridge_join_gen_wan_rule_candi_by_filter(wan_port->port_filter.tag_rx,
							wan_port,
							i,
							&rule_candi_list_wan) < 0)
						{
							dbprintf(LOG_ERR, "gem wan rule_candi by filter error, op=%u!!\n", wan_port->port_filter.tag_rx);
						}
					} else {
						//join with no filter found, set wan_filter_valid to 0
						rule_candi = classf_alloc_rule_candi(); //upstream, invalid
						rule_candi->wan_port = wan_port;
						rule_candi->gem_index = i;
						rule_candi->wan_filter_type = 0;

						list_add_tail(&rule_candi->rc_node, &rule_candi_list_wan);
					}
				}
			}
		}
	}

	//lan side
	list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
	{
		//join lan port filter and tagging rule
		list_for_each_entry(rule_fields, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
		{
			classf_bridge_join_gen_ds_lan_rule_candi_by_tagging_filter(
				lan_port,
				rule_fields,
				NULL,
				&rule_candi_list_lan);
		}

		//join lan port filter and protocol vlan rule
		list_for_each_entry(pvlan_rule, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
		{
			if (pvlan_rule->join_disable != 0)
			{
				//skip
				continue;
			}

			classf_bridge_join_gen_ds_lan_rule_candi_by_tagging_filter(
				lan_port,
				&pvlan_rule->rule_fields,
				&pvlan_rule->pvlan_acl,
				&rule_candi_list_lan);
		}
	}

	//join lan-wan list
	list_for_each_entry(rule_candi_wan_pos, &rule_candi_list_wan, rc_node) //wan
	{
		list_for_each_entry(rule_candi_lan_pos, &rule_candi_list_lan, rc_node) //lan
		{
			rule_candi = classf_alloc_rule_candi(); //upstream, invalid
			//wan part
			rule_candi->wan_port = rule_candi_wan_pos->wan_port;
			rule_candi->gem_index = rule_candi_wan_pos->gem_index;
			rule_candi->wan_filter_valid = rule_candi_wan_pos->wan_filter_valid;
			rule_candi->wan_filter_type = rule_candi_wan_pos->wan_filter_type;

			//lan part
			rule_candi->lan_port = rule_candi_lan_pos->lan_port;
			rule_candi->lan_filter_valid = rule_candi_lan_pos->lan_filter_valid;
			rule_candi->lan_filter_type = rule_candi_lan_pos->lan_filter_type;
			rule_candi->rule_fields = rule_candi_lan_pos->rule_fields;
			rule_candi->pvlan_acl = rule_candi_lan_pos->pvlan_acl;

			if (classf_insert_rule_candi_to_list(rule_candi, &bridge_info->rule_candi_list_ds) < 0)
			{
				dbprintf(LOG_WARNING, "the same rule candi, drop it!!\n");
				free_safe(rule_candi);
			}		
		}
	}

	//release wan/lan list
	classf_release_rule_candi_list(&rule_candi_list_wan);
	classf_release_rule_candi_list(&rule_candi_list_lan);
	
	return 0;
}

int
classf_bridge_join(struct classf_bridge_info_t *bridge_info)
{
	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//upstream join
	//rules * lan filters * wan filters * gems
	if (classf_bridge_join_us(bridge_info) < 0)
	{
		dbprintf(LOG_ERR, "classf bridge rule join error for upstream!!, bridge meid=0x%.4x\n", bridge_info->me_bridge->meid);
	}

	//downstream join
	//gems * wan filters * lan filters * rules
	if (classf_bridge_join_ds(bridge_info) < 0)
	{
		dbprintf(LOG_ERR, "classf bridge rule join error for downstream!!, bridge meid=0x%.4x\n", bridge_info->me_bridge->meid);
	}

	return 0;
}

