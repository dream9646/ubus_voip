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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_classf.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include
#include "intel_px126_util.h"

#include "pon_adapter.h"

//#include "me/omci_mac_bridge_service_profile.h"
#include "omci/pon_adapter_omci.h"
#include "omci/me/pon_adapter_mac_bridge_service_profile.h"
#include "omci/me/pon_adapter_mac_bridge_port_config_data.h"
#include "omci/me/pon_adapter_vlan_tagging_filter_data.h"
#include "omci/me/pon_adapter_dot1p_mapper.h"
#include "omci/me/pon_adapter_ext_vlan.h"
#include "omci/me/pon_adapter_vlan_tagging_operation_config_data.h"


//5vt include
#include "classf.h"
#include "util.h"
#include "switch_hw_prx126.h"

// 220927 LEO START workaround for zeroing counter after configured
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"
#include "intel_px126_pm.h"
// 220927 LEO END

#include "me_related.h"
#include "omciutil_misc.h"
#include "hwresource.h"
#include "omcimain.h"
#include "proprietary_alu.h"

/** Maximum number of entries in the VLAN Filter List*/
#define PRX126_CLASSF_OMCI_VLAN_TAGGING_FILTER_LIST_SIZE 12
/** Special VID=0 for ALCL OLTs */
#define PRX126_CLASSF_OMCI_IOP_ALU_VID0	0
#define PRX126_CLASSF_OMCC_VERSION_G988_2010 0x0080 /* 0xA0 */


enum prx126_classf_me_relationship
{
	PRX126_CLASSF_ME_RELATIONSHIP_NONE,
	PRX126_CLASSF_ME_RELATIONSHIP_EXPLICIT,
	PRX126_CLASSF_ME_RELATIONSHIP_IMPLICIT,
};

struct prx126_classf_bridge_info_wan_port_t
{
	struct me_t *me_bridge;
	struct me_t *me_bridge_port;

	unsigned short bridge_id;
	unsigned char relationship;

	unsigned char port_type_index;
	unsigned char logical_port_id;
	struct classf_port_gem_t port_gem;
	struct classf_port_filter_t port_filter;

	struct list_head wp_node;
};

struct prx126_classf_bridge_info_lan_port_t
{
	struct me_t *me_bridge;
	struct me_t *me_bridge_port;

	unsigned short bridge_id;
	unsigned char relationship;

	unsigned char port_type;
	unsigned char port_type_index;
	unsigned char port_type_sort; 							//for rule sorting
	unsigned char port_type_index_sort; 					//for rule sorting
	unsigned char logical_port_id; 							//for mapping to physical port
	struct classf_port_filter_t port_filter;
	struct classf_port_tagging_t port_tagging;

	struct classf_port_protocol_vlan_t port_protocol_vlan;	//protocol vlan rule

	unsigned char vopi_if_index;									// the VOPI interface index


	struct list_head lp_node;
};

struct prx126_classf_bridge_info_t
{
	struct me_t *me_bridge;
	unsigned short bridge_id;

	struct list_head wan_port_list; 						// node is classf_bridge_info_wan_port_t
	struct list_head lan_port_list; 						// node is classf_bridge_info_lan_port_t

	struct list_head b_node;
};

#if 1
/** Received Frame VLAN Tagging Operation Table Entry. See \ref
 *  omci_me_ext_vlan_cfg_data::rx_vlan_oper_table
 */
struct prx126_classf_omci_rx_vlan_oper_table {
	unsigned int filter_outer_prio : 4;				/** Filter on outer VLAN priority. */
	unsigned int filter_outer_vid : 13;				/** Filter on outer VLAN ID. */
	unsigned int filter_outer_tpid_de : 3;			/** Filter on outer TPID. */
	unsigned int word1_padding : 12;				/** Padding. */

	unsigned int filter_inner_prio : 4;				/** Filter on inner VLAN priority. */
	unsigned int filter_inner_vid : 13;				/** Filter on inner VLAN ID. */
	unsigned int filter_inner_tpid_de : 3;			/** Filter on inner TPID. */
	unsigned int word2_padding : 8;					/** Padding. */

	unsigned int filter_ether_type : 4;				/** Filter on Ethertype. */

	unsigned int treatment_tags_remove : 2;			/** Treatment, VLAN Tags to be removed. */
	unsigned int word3_padding : 10;				/** Padding. */

	unsigned int treatment_outer_prio : 4;			/** Treatment of outer VLAN priority. */
	unsigned int treatment_outer_vid : 13;			/** Treatment of outer VLAN ID. */
	unsigned int treatment_outer_tpid_de : 3;		/** Treatment of outer TPID. */
	unsigned int word4_padding : 12;				/** Padding. */

	unsigned int treatment_inner_prio : 4;			/** Treatment of inner VLAN priority. */
	unsigned int treatment_inner_vid : 13;			/** Treatment of inner VLAN ID. */
	unsigned int treatment_inner_tpid_de : 3;		/** Treatment of inner TPID. */
};


/** Received Frame VLAN Tagging Operation Filter. */
struct prx126_classf_omci_rx_vlan_oper_filter {
	/** Filter on outer VLAN priority. */
	unsigned int filter_outer_prio : 4;
	/** Filter on outer VLAN ID. */
	unsigned int filter_outer_vid : 13;
	/** Filter on outer TPID. */
	unsigned int filter_outer_tpid_de : 3;
	/** Padding. */
	unsigned int word1_padding : 12;

	/** Filter on inner VLAN priority. */
	unsigned int filter_inner_prio : 4;
	/** Filter on inner VLAN ID. */
	unsigned int filter_inner_vid : 13;
	/** Filter on inner TPID. */
	unsigned int filter_inner_tpid_de : 3;
	/** Padding. */
	unsigned int word2_padding : 8;
	/** Filter on Ethertype. */
	unsigned int filter_ether_type : 4;
} ;

#else
#if defined(USE_PRAGMA_PACK)
#pragma pack(push, 1)
#endif

/** Received Frame VLAN Tagging Operation Table Entry. See \ref
 *  omci_me_ext_vlan_cfg_data::rx_vlan_oper_table
 */
struct prx126_classf_omci_rx_vlan_oper_table {
	unsigned int filter_outer_prio : 4;				/** Filter on outer VLAN priority. */
	unsigned int filter_outer_vid : 13;				/** Filter on outer VLAN ID. */
	unsigned int filter_outer_tpid_de : 3;			/** Filter on outer TPID. */
	unsigned int word1_padding : 12;				/** Padding. */

	unsigned int filter_inner_prio : 4;				/** Filter on inner VLAN priority. */
	unsigned int filter_inner_vid : 13;				/** Filter on inner VLAN ID. */
	unsigned int filter_inner_tpid_de : 3;			/** Filter on inner TPID. */
	unsigned int word2_padding : 8;					/** Padding. */

	unsigned int filter_ether_type : 4;				/** Filter on Ethertype. */

	unsigned int treatment_tags_remove : 2;			/** Treatment, VLAN Tags to be removed. */
	unsigned int word3_padding : 10;				/** Padding. */

	unsigned int treatment_outer_prio : 4;			/** Treatment of outer VLAN priority. */
	unsigned int treatment_outer_vid : 13;			/** Treatment of outer VLAN ID. */
	unsigned int treatment_outer_tpid_de : 3;		/** Treatment of outer TPID. */
	unsigned int word4_padding : 12;				/** Padding. */

	unsigned int treatment_inner_prio : 4;			/** Treatment of inner VLAN priority. */
	unsigned int treatment_inner_vid : 13;			/** Treatment of inner VLAN ID. */
	unsigned int treatment_inner_tpid_de : 3;		/** Treatment of inner TPID. */
} __PACKED__;

#if defined(USE_PRAGMA_PACK)
#pragma pack(pop)
#endif

/** Received Frame VLAN Tagging Operation Filter. */
struct prx126_classf_omci_rx_vlan_oper_filter {
	/** Filter on outer VLAN priority. */
	unsigned int filter_outer_prio : 4;
	/** Filter on outer VLAN ID. */
	unsigned int filter_outer_vid : 13;
	/** Filter on outer TPID. */
	unsigned int filter_outer_tpid_de : 3;
	/** Padding. */
	unsigned int word1_padding : 12;

	/** Filter on inner VLAN priority. */
	unsigned int filter_inner_prio : 4;
	/** Filter on inner VLAN ID. */
	unsigned int filter_inner_vid : 13;
	/** Filter on inner TPID. */
	unsigned int filter_inner_tpid_de : 3;
	/** Padding. */
	unsigned int word2_padding : 8;
	/** Filter on Ethertype. */
	unsigned int filter_ether_type : 4;
} __PACKED__;
#endif

struct prx126_classf_rx_vlan_oper_table_entry_t {
	struct prx126_classf_omci_rx_vlan_oper_table data;	/** Table entry */
	unsigned int idx;									/** Entry sorted index*/
	bool def;											/** Entry is default*/
};

struct prx126_classf_rx_vlan_oper_list_entry_t
{
	struct me_t *me;
	unsigned short classid;
	unsigned short meid;

	struct prx126_classf_rx_vlan_oper_table_entry_t table_entry;	/** Table entry */

	struct list_head node;
};

/** Internal data */
struct prx126_classf_ext_vlan_idata_t
{
	int entries_num;		/** Number of entries in Received frame VLAN tagging operation table */
	int def_entries_num;	/** Number of default entries in Received frame VLAN tagging operation table */

	struct list_head entry_list;		/** Received frame VLAN tagging operation table (list head) */
};


struct prx126_classf_me_info_t
{
	struct me_t *me;
	unsigned short classid;
	unsigned short meid;

	void *internal_data;	/** Managed Entity internal data (not defined by the ITU) */

	struct list_head me_node;
};

struct intel_omci_context *prx126_classf_context;
struct pa_node *prx126_classf_pa_node_pon = NULL;
struct pa_node *prx126_classf_pa_node_pon_net = NULL;


struct list_head prx126_classf_me_45_list;
struct list_head prx126_classf_me_280_list;
struct list_head prx126_classf_me_298_list;
struct list_head prx126_classf_me_268_list;
struct list_head prx126_classf_me_266_list;
struct list_head prx126_classf_me_281_list;
struct list_head prx126_classf_me_47_list;
struct list_head prx126_classf_me_84_list;
struct list_head prx126_classf_me_130_list;
struct list_head prx126_classf_me_171_list;
struct list_head prx126_classf_me_171_list;
struct list_head prx126_classf_me_78_list;
//struct list_head prx126_classf_me_246_list;


//struct list_head prx126_classf_ext_vlan_internal_data_list;		/** Received frame VLAN tagging operation table (list head), keep config data */
//int prx126_classf_ext_vlan_entries_num;					/** Number of entries in Received frame VLAN tagging operation table */
//int prx126_classf_ext_vlan_def_entries_num;				/** Number of default entries in Received frame VLAN tagging operation table */


/** Maximum number of Ext VLAn default rules*/
#define PRX126_CLASSF_OMCI_ONU_EXT_VLAN_DEFAULT_RULE_MAX	3

/** \todo add endianness handling
*/
static const struct prx126_classf_omci_rx_vlan_oper_table
prx126_classf_rx_vlan_def[PRX126_CLASSF_OMCI_ONU_EXT_VLAN_DEFAULT_RULE_MAX] = {
#if (IFXOS_BYTE_ORDER == IFXOS_BIG_ENDIAN)
	{
		15, 4096,  0, 0,
		15, 4096,  0, 0, 0,
		0,     0, 15, 0, 0,
		0,    15, 0,  0
	},
	{
		15, 4096,  0, 0,
		14, 4096,  0, 0, 0,
		0,     0, 15, 0, 0,
		0,    15,  0, 0
	},
	{
		14, 4096,  0, 0,
		14, 4096,  0, 0, 0,
		0,     0, 15, 0, 0,
		0, 15, 0, 0
	}
#else
	{
		0, 0, 4096,   15,
		0, 0,    0, 4096, 15,
		0, 0,   15,    0,  0,
		0, 0,   15,  0
	},
	{
		0, 0, 4096,   15,
		0, 0,    0, 4096, 14,
		0, 0,   15,    0,  0,
		0, 0,   15,  0
	},
	{
		0, 0, 4096,   14,
		0, 0,    0, 4096, 14,
		0, 0,   15,    0,  0,
		0, 0,   15,  0
	}
#endif
};


int
switch_hw_prx126_classf_init(void)
{
	struct pa_node *np = NULL,*n = NULL;


	INIT_LIST_HEAD(&prx126_classf_me_45_list);
	INIT_LIST_HEAD(&prx126_classf_me_47_list);
	INIT_LIST_HEAD(&prx126_classf_me_84_list);
	INIT_LIST_HEAD(&prx126_classf_me_130_list);
	INIT_LIST_HEAD(&prx126_classf_me_171_list);
	INIT_LIST_HEAD(&prx126_classf_me_78_list);


	prx126_classf_context = intel_omci_get_omci_context();

	list_for_each_entry_safe( np, n ,&prx126_classf_context->pa_list ,entries)
	{
		if (PA_EXISTS(np->pa_ops, omci_me_ops, ani_g))
		{
			prx126_classf_pa_node_pon = np;
			continue;
		}

		if (PA_EXISTS(np->pa_ops, omci_me_ops, gem_itp))
		{
			prx126_classf_pa_node_pon_net = np;
			continue;
		}
	}

	if( NULL == prx126_classf_pa_node_pon || NULL == prx126_classf_pa_node_pon_net )
	{
		dbprintf(LOG_ERR, "pa_node error! pon / pon_net: 0x%X / 0x%X", prx126_classf_pa_node_pon, prx126_classf_pa_node_pon_net);
	}

	return 0;
}


static int
switch_hw_prx126_classf_vlan_tagging_filter_data_add(void)
{
	struct meinfo_t *miptr_84;
	struct me_t *me_pos_84;
	struct me_t *me_47;
	struct prx126_classf_me_info_t *me_info;
	struct attr_value_t attr_value;
	struct attr_value_t attr_filter_list={0, NULL};
	struct attr_value_t attr_filter_op={0, NULL};
	struct attr_value_t attr_filter_entry_total={0, NULL};
	unsigned char *bitmap;
	unsigned int i;
	unsigned short filter_list[PRX126_CLASSF_OMCI_VLAN_TAGGING_FILTER_LIST_SIZE] = {0};
	unsigned char entries_num,tp_type;
	unsigned char forwarding_oper;
	bool use_g988;
	int ret = INTEL_PON_ERROR;


	if ((miptr_84 = meinfo_util_miptr(84)) == NULL) // 84, vlan_tag_filter_data
	{
		dbprintf(LOG_ERR, "get 84 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_84, &miptr_84->me_instance_list, instance_node)
	{
		me_info = malloc_safe(sizeof(struct prx126_classf_me_info_t));
		if( NULL == me_info)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}

		me_info->me = me_pos_84;
		me_info->classid = 84;
		me_info->meid = me_pos_84->meid;

		meinfo_me_attr_get(me_pos_84, 1, &attr_filter_list);
		meinfo_me_attr_get(me_pos_84, 2, &attr_filter_op);
		forwarding_oper = attr_filter_op.data;
		meinfo_me_attr_get(me_pos_84, 3, &attr_filter_entry_total);
		entries_num = attr_filter_entry_total.data;
		if ((bitmap=(unsigned char *) attr_filter_list.ptr))
		{
			for (i=0; i<attr_filter_entry_total.data; i++)
			{
				filter_list[i] = util_bitmap_get_value(bitmap, 24*8, i*16, 16);

#if 0
				//alu workaround, vid=0xfff, priority=0x7, set op type for all pass
				if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU &&
					(proprietary_alu_get_olt_version() == 1) &&
					(((filter_list[i] & 0xe000) >> 13) == 0x7) &&
					((filter_list[i] & 0x0fff) == 0xfff))
				{
					unsigned char attr_mask[2]={0, 0};
					alu_workaround_flag = 1;
					attr_filter_op.data = 0; // do not filter any vlan
					meinfo_me_attr_set(me_pos_84, 2, &attr_filter_op, 0);
					util_attr_mask_set_bit(attr_mask, 2);
					meinfo_me_flush(me_pos_84, attr_mask);
				}
#endif
			}
		}

		me_47 = omciutil_misc_find_me_related_bport(me_pos_84);
		if(me_47 != NULL)
		{
			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU &&
				(proprietary_alu_get_olt_version() == 1))
			{
				meinfo_me_attr_get(me_47, 3, &attr_value);
				tp_type = attr_value.data;

				if (tp_type == 3 || tp_type == 5 || tp_type == 6) {
					bool alu_vid_match = false;

					/* search if ALU special VID already present */
					for (i = 0; i < entries_num; i++) {
						if (filter_list[i] == PRX126_CLASSF_OMCI_IOP_ALU_VID0) {
							alu_vid_match = true;
							break;
						}
					}

					if (!alu_vid_match) {
						/* IOP: add special VID=0 entry for ALCL OLTs */
						if (attr_filter_entry_total.data < PRX126_CLASSF_OMCI_VLAN_TAGGING_FILTER_LIST_SIZE)
						{
							filter_list[attr_filter_entry_total.data] = PRX126_CLASSF_OMCI_IOP_ALU_VID0;
							entries_num++;
						} else {
							dbprintf(LOG_ERR, "OMCI_ERROR_RESOURCE_NOT_FOUND!\n");
							return -1;
						}
					}
				}
			}

			use_g988 = (omci_get_omcc_version() == PRX126_CLASSF_OMCC_VERSION_G988_2010)? true : false;
			fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
			PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, vlan_tag_filter_data, update),
					(me_pos_84->meid, filter_list, entries_num,
					forwarding_oper, use_g988));
			fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
		}

		// insert;
		list_add_tail(&me_info->me_node, &prx126_classf_me_84_list);
	}

	return ret;
}

static int
switch_hw_prx126_classf_vlan_tagging_filter_data_del(void)
{
	struct prx126_classf_me_info_t *me_info_pos;
	struct prx126_classf_me_info_t *me_info_n;
	int ret = INTEL_PON_ERROR;


	list_for_each_entry_safe(me_info_pos, me_info_n, &prx126_classf_me_84_list, me_node)
	{
		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, vlan_tag_filter_data,
						 destroy), (me_info_pos->meid));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);

		list_del(&me_info_pos->me_node);
		free_safe(me_info_pos);
	}

	return ret;
}

static int switch_hw_prx126_classf_tbl_entry_uid_cmp(const void *p1, const void *p2)
{
	return memcmp(*(const struct prx126_classf_omci_rx_vlan_oper_table **)p1,
		      *(const struct prx126_classf_omci_rx_vlan_oper_table **)p2,
		      8);
}

static void switch_hw_prx126_classf_dump_entry(struct me_t  *me,
			      const char *prefix,
			      struct prx126_classf_omci_rx_vlan_oper_table *entry)
{
	dbprintf(LOG_ERR, "%u@%u %s\n"
		   "filter_outer_prio=%d,\n"
		   "filter_outer_vid=%d,\n"
		   "filter_outer_tpid_de=%d,\n"
		   "word1_padding=%d \n"

		   "filter_inner_prio=%d,\n"
		   "filter_inner_vid=%d,\n"
		   "filter_inner_tpid_de=%d,\n"
		   "word2_padding=%d,\n"
		   "filter_ether_type=%d \n"

		   "treatment_tags_remove=%d,\n"
		   "word3_padding=%d,\n"
		   "treatment_outer_prio=%d,\n"
		   "treatment_outer_vid=%d,\n"
		   "treatment_outer_tpid_de=%d \n"

		   "word4_padding=%d,\n"
		   "treatment_inner_prio=%d,\n"
		   "treatment_inner_vid=%d\n,"
		   "treatment_inner_tpid_de=%d \n",
		me->classid,
		me->meid,
		prefix,

		entry->filter_outer_prio,
		entry->filter_outer_vid,
		entry->filter_outer_tpid_de,
		entry->word1_padding,

		entry->filter_inner_prio,
		entry->filter_inner_vid,
		entry->filter_inner_tpid_de,
		entry->word2_padding,
		entry->filter_ether_type,

		entry->treatment_tags_remove,
		entry->word3_padding,
		entry->treatment_outer_prio,
		entry->treatment_outer_vid,
		entry->treatment_outer_tpid_de,

		entry->word4_padding,
		entry->treatment_inner_prio,
		entry->treatment_inner_vid,
		entry->treatment_inner_tpid_de);
}


static bool switch_hw_prx126_classf_rule_is_zero(const struct prx126_classf_omci_rx_vlan_oper_table *r)
{
	return (r->filter_outer_prio == 15 && r->filter_inner_prio == 15) ?
								   true : false;
}

static bool switch_hw_prx126_classf_rule_is_one(const struct prx126_classf_omci_rx_vlan_oper_table *r)
{
	return (r->filter_outer_prio == 15 && r->filter_inner_prio != 15) ?
								   true : false;
}


static int switch_hw_prx126_classf_rule_cmp(const struct prx126_classf_omci_rx_vlan_oper_table *r1,
		    const struct prx126_classf_omci_rx_vlan_oper_table *r2)
{
	struct prx126_classf_omci_rx_vlan_oper_filter f1, f2;

	memcpy(&f1, r1, sizeof(struct prx126_classf_omci_rx_vlan_oper_filter));
	memcpy(&f2, r2, sizeof(struct prx126_classf_omci_rx_vlan_oper_filter));

	/* Check for untagged rule */
	if (switch_hw_prx126_classf_rule_is_zero(r1) && switch_hw_prx126_classf_rule_is_zero(r2)) {
		/* Ignore inner and outer VID filter */
		f1.filter_inner_vid = 0;
		f1.filter_outer_vid = 0;
		f2.filter_inner_vid = 0;
		f2.filter_outer_vid = 0;
		/* Ignore inner and outer TPID/DEI filter */
		f1.filter_inner_tpid_de = 0;
		f1.filter_outer_tpid_de = 0;
		f2.filter_inner_tpid_de = 0;
		f2.filter_outer_tpid_de = 0;
	}

	/* Check for single-tagged rule */
	if (switch_hw_prx126_classf_rule_is_one(r1) && switch_hw_prx126_classf_rule_is_one(r2)) {
		/* Ignore outer VID filter */
		f1.filter_outer_vid = 0;
		f2.filter_outer_vid = 0;

		/* Ignore outer TPID/DEI filter */
		f1.filter_outer_tpid_de = 0;
		f2.filter_outer_tpid_de = 0;
	}

	return memcmp(&f1, &f2, sizeof(struct prx126_classf_omci_rx_vlan_oper_filter));
}

static int
switch_hw_prx126_classf_hwtc_rule_add(struct intel_omci_context *context,
		  struct prx126_classf_me_info_t *me_info,
	      const uint8_t ds_mode,
	      const bool default_rule,
	      const struct prx126_classf_omci_rx_vlan_oper_table *rule)
{
	enum pon_adapter_errno ret;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct pon_adapter_ext_vlan_filter *filter;
	uint32_t i;
	struct prx126_classf_rx_vlan_oper_list_entry_t *entry_pos, *entry_pos_n;
	struct prx126_classf_rx_vlan_oper_list_entry_t *le;


	me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info->internal_data;

	dbprintf(LOG_ERR, "%u@%u Delete entry request!\n", me_info->me->classid, me_info->me->meid);

	list_for_each_entry_safe(entry_pos, entry_pos_n, &me_idata->entry_list, node)
	{
		if (switch_hw_prx126_classf_rule_cmp(&entry_pos->table_entry.data, rule) == 0)
		{
			switch_hw_prx126_classf_dump_entry(entry_pos->me, "Delete entry", &entry_pos->table_entry.data);

			if (entry_pos->table_entry.def)
			{
				--me_idata->def_entries_num;
			}
			--me_idata->entries_num;

			list_del(&entry_pos->node);
			free_safe(entry_pos);

			dbprintf(LOG_ERR, "%u@%u Removed table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);

			break;
		}
	}


	le = malloc_safe(sizeof(struct prx126_classf_rx_vlan_oper_list_entry_t));
	if( NULL == le)
	{
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}

	++me_idata->entries_num;

	memcpy(&le->table_entry.data, rule, sizeof(le->table_entry.data));

	switch_hw_prx126_classf_dump_entry(me_info->me, "Add entry", &le->table_entry.data);

	if (default_rule)
	{
		++me_idata->def_entries_num;

		le->table_entry.def = true;

		/* insert new entry to the tail */
		list_add_tail(&le->node, &me_idata->entry_list);
	}
	else
	{
		le->table_entry.def = false;

		/* insert new entry to the head */
		list_add(&le->node, &me_idata->entry_list);
	}

	dbprintf(LOG_ERR, "%u@%u Added table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);


	/* update entries index */
	i = 0;
	list_for_each_entry(entry_pos, &me_idata->entry_list, node)
	{
		le->table_entry.idx = i++;
	}

	filter = malloc_safe( sizeof(struct pon_adapter_ext_vlan_filter) * me_idata->entries_num );
	if( NULL == filter)
	{
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}


	i = 0;
	list_for_each_entry(le,&me_idata->entry_list, node)
	{
		filter[i].filter_outer_priority 	= le->table_entry.data.filter_outer_prio;
		filter[i].filter_outer_vid 			= le->table_entry.data.filter_outer_vid;
		filter[i].filter_outer_tpid_de 		= le->table_entry.data.filter_outer_tpid_de;
		filter[i].filter_inner_priority		= le->table_entry.data.filter_inner_prio;
		filter[i].filter_inner_vid			= le->table_entry.data.filter_inner_vid;
		filter[i].filter_inner_tpid_de		= le->table_entry.data.filter_inner_tpid_de;
		filter[i].filter_ethertype			= le->table_entry.data.filter_ether_type;
		filter[i].treatment_tags_to_remove	= le->table_entry.data.treatment_tags_remove;
		filter[i].treatment_outer_priority	= le->table_entry.data.treatment_outer_prio;
		filter[i].treatment_outer_vid		= le->table_entry.data.treatment_outer_vid;
		filter[i].treatment_outer_tpid_de	= le->table_entry.data.treatment_outer_tpid_de;
		filter[i].treatment_inner_priority	= le->table_entry.data.treatment_inner_prio;
		filter[i].treatment_inner_vid		= le->table_entry.data.treatment_inner_vid;
		filter[i].treatment_inner_tpid_de	= le->table_entry.data.treatment_inner_tpid_de;
		i++;
	}

	fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
	PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan, rules_add),
		    (me_info->me->meid, ds_mode, filter,
		    me_idata->entries_num,
			1));
//		    omci_iop_mask_isset(prx126_classf_context, OMCI_IOP_OPTION_3)));
	fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
	if (ret != PON_ADAPTER_SUCCESS)
	{
		dbprintf(LOG_ERR, "%u@%u ERR(%d) Couldn't add hwtc rule!\n", me_info->me->classid, me_info->me->meid, ret);
	}

	free_safe(filter);

	if( PON_ADAPTER_SUCCESS != ret)
	{
		dbprintf(LOG_ERR, "switch_hw_prx126_classf_hwtc_rule_add(), , meid:%d, fail!", me_info->me->meid);
		return ret;
	}

	return ret;
}

static int
switch_hw_prx126_classf_alcl_rule_add(struct intel_omci_context *context,
		  struct prx126_classf_me_info_t *me_info,
	      const uint8_t ds_mode,
	      const bool default_rule,
	      const struct prx126_classf_omci_rx_vlan_oper_table *rule)
{
	enum pon_adapter_errno ret;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct prx126_classf_rx_vlan_oper_list_entry_t *le;
	bool rule_overridden, rule_found;
	uint32_t sort_idx, sort_idx_def, sort_num, i;
	struct prx126_classf_omci_rx_vlan_oper_table **sort_array;
	struct pon_adapter_ext_vlan_filter *filter;


	me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info->internal_data;

	rule_overridden = false;
	list_for_each_entry(le,&me_idata->entry_list, node)
	{
		if (switch_hw_prx126_classf_rule_cmp(&le->table_entry.data, rule) == 0)
		{
			/* override entry */
			switch_hw_prx126_classf_dump_entry(me_info->me, "Override entry (old)", &le->table_entry.data);

			memcpy(&le->table_entry.data, rule, sizeof(le->table_entry.data));

			switch_hw_prx126_classf_dump_entry(me_info->me, "Override entry (new)", &le->table_entry.data);

			rule_overridden = true;

			dbprintf(LOG_ERR, "%u@%u Overridden table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);

			break;
		}

	}

	if (!rule_overridden) {
		/* insert new entry to the head */
		le = malloc_safe( sizeof(struct prx126_classf_rx_vlan_oper_list_entry_t));
		if( NULL == le)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}

		++me_idata->entries_num;

		memcpy(&le->table_entry.data, rule, sizeof(le->table_entry.data));

		switch_hw_prx126_classf_dump_entry(me_info->me, "Add entry", &le->table_entry.data);

		if (default_rule)
		{
			++me_idata->def_entries_num;
			le->table_entry.def = true;
		}
		else
		{
			le->table_entry.def = false;
		}

		list_add_tail(&le->node, &me_idata->entry_list);

		dbprintf(LOG_ERR, "%u@%u Added table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);
	}

	sort_array = malloc_safe(sizeof(*sort_array) * me_idata->entries_num);
	if( NULL == sort_array)
	{
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}

	/* fill rule array */
	sort_idx = 0;
	sort_idx_def = 0;
	sort_num = me_idata->entries_num - me_idata->def_entries_num;

	list_for_each_entry(le, &me_idata->entry_list, node)
	{
		if (!le->table_entry.def)
		{
			sort_array[sort_idx] = &le->table_entry.data;
			sort_idx++;
		}
		else
		{
			/* default entry at the end*/
			sort_array[sort_num + sort_idx_def] = &le->table_entry.data;

			sort_idx_def++;
		}
	}

	/* sort only non-default entries*/
	qsort(sort_array, sort_num, sizeof(*sort_array), switch_hw_prx126_classf_tbl_entry_uid_cmp);

	/* update entries index */
	list_for_each_entry(le, &me_idata->entry_list, node)
	{
		rule_found = false;
		for (sort_idx = 0; sort_idx < me_idata->entries_num; sort_idx++) {
			if (memcmp(&le->table_entry.data, sort_array[sort_idx], 8) == 0)
			{
				le->table_entry.idx = sort_idx;
				rule_found = true;
				break;
			}
		}

		/* this should normally not happen*/
		if (!rule_found)
		{
			dbprintf(LOG_ERR, "%u@%u Can't find table entry in the sorted list!\n", me_info->me->classid, me_info->me->meid);

			//error = OMCI_ERROR_INVALID_VAL;

			free_safe(sort_array);

			//return error;
			return -1;
		}
	}

	filter = malloc_safe(sizeof(struct pon_adapter_ext_vlan_filter) * me_idata->entries_num);
	if( NULL == filter)
	{
		free_safe(sort_array);
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}

	for (i = 0; i < me_idata->entries_num; i++)
	{
		filter[i].filter_outer_priority		= sort_array[i]->filter_outer_prio;
		filter[i].filter_outer_vid			= sort_array[i]->filter_outer_vid;
		filter[i].filter_outer_tpid_de		= sort_array[i]->filter_outer_tpid_de;
		filter[i].filter_inner_priority		= sort_array[i]->filter_inner_prio;
		filter[i].filter_inner_vid			= sort_array[i]->filter_inner_vid;
		filter[i].filter_inner_tpid_de		= sort_array[i]->filter_inner_tpid_de;
		filter[i].filter_ethertype			= sort_array[i]->filter_ether_type;
		filter[i].treatment_tags_to_remove	= sort_array[i]->treatment_tags_remove;
		filter[i].treatment_outer_priority	= sort_array[i]->treatment_outer_prio;
		filter[i].treatment_outer_vid		= sort_array[i]->treatment_outer_vid;
		filter[i].treatment_outer_tpid_de	= sort_array[i]->treatment_outer_tpid_de;
		filter[i].treatment_inner_priority	= sort_array[i]->treatment_inner_prio;
		filter[i].treatment_inner_vid		= sort_array[i]->treatment_inner_vid;
		filter[i].treatment_inner_tpid_de	= sort_array[i]->treatment_inner_tpid_de;
	}

	fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
	PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan, rules_add),
		    (me_info->me->meid, ds_mode, filter,
		    me_idata->entries_num,
		    1));
		    //omci_iop_mask_isset(context, OMCI_IOP_OPTION_3)));
	fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
	if (ret != PON_ADAPTER_SUCCESS)
	{
		dbprintf(LOG_ERR, "%u@%u ERR(%d) Couldn't add alcl rule!\n", me_info->me->classid, me_info->me->meid, ret);
	}

	free_safe(sort_array);
	free_safe(filter);

	if( PON_ADAPTER_SUCCESS != ret)
	{
		dbprintf(LOG_ERR, "switch_hw_prx126_classf_alcl_rule_add(), , meid:%d, fail!", me_info->me->meid);
		return ret;
	}

	return ret;
}

// 221005 LEO START workaround for zeroing counter after configured 5 seconds
void *
counter_reset_thread_func(void *arg)
{
	sleep(5);
	gpon_hw_prx126_pm_refresh(1);
	return 0;
}
// 221005 LEO END

static int
switch_hw_prx126_classf_comm_rule_add(struct intel_omci_context *context,
		  struct prx126_classf_me_info_t *me_info,
	      const uint8_t ds_mode,
	      const bool default_rule,
	      const struct prx126_classf_omci_rx_vlan_oper_table *rule)
{
	enum pon_adapter_errno ret;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct prx126_classf_rx_vlan_oper_list_entry_t *le;
	bool rule_overridden, rule_found;
	uint32_t sort_idx, sort_idx_def, sort_num, i;
	struct pon_adapter_ext_vlan_filter *filter = NULL;
	struct prx126_classf_omci_rx_vlan_oper_table **sort_array;


	me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info->internal_data;

	rule_overridden = false;
	list_for_each_entry(le, &me_idata->entry_list, node)
	{
		if (switch_hw_prx126_classf_rule_cmp(&le->table_entry.data, rule) == 0)
		{
			/* override entry */
			switch_hw_prx126_classf_dump_entry(me_info->me, "Override entry (old)", &le->table_entry.data);

			memcpy(&le->table_entry.data, rule, sizeof(le->table_entry.data));

			switch_hw_prx126_classf_dump_entry(me_info->me, "Override entry (new)", &le->table_entry.data);

			rule_overridden = true;

			dbprintf(LOG_ERR, "%u@%u Overridden table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);

			break;
		}
	}

	if (!rule_overridden)
	{
		/* insert new entry to the head */
		le = malloc_safe( sizeof(struct prx126_classf_rx_vlan_oper_list_entry_t));
		if( NULL == le)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}

		++me_idata->entries_num;

		memcpy(&le->table_entry.data, rule, sizeof(le->table_entry.data));

		switch_hw_prx126_classf_dump_entry(me_info->me, "Add entry", &le->table_entry.data);

		if (default_rule)
		{
			++me_idata->def_entries_num;
			le->table_entry.def = true;
		}
		else
		{
			le->table_entry.def = false;
		}

		list_add_tail(&le->node, &me_idata->entry_list);

		dbprintf(LOG_ERR, "%u@%u Added table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, me_idata->entries_num);
		// 221005 LEO START workaround for zeroing counter after configured 5 seconds
		pthread_t counter_reset_thread = 0;
		pthread_create(&counter_reset_thread, 0, counter_reset_thread_func, 0);
		// 221005 LEO END

	}

	sort_array = malloc_safe(sizeof(*sort_array) * me_idata->entries_num);
	if( NULL == sort_array)
	{
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}

	/* fill rule array */
	sort_idx = 0;
	sort_idx_def = 0;
	sort_num = me_idata->entries_num - me_idata->def_entries_num;

	list_for_each_entry(le, &me_idata->entry_list, node)
	{
		if (!le->table_entry.def)
		{
			sort_array[sort_idx] = &le->table_entry.data;
			sort_idx++;
		}
		else
		{
			/* default entry at the end*/
			sort_array[sort_num + sort_idx_def] = &le->table_entry.data;

			sort_idx_def++;
		}
	}

	/* sort only non-default entries*/
	qsort(sort_array, sort_num, sizeof(*sort_array), switch_hw_prx126_classf_tbl_entry_uid_cmp);

	/* update entries index */
	list_for_each_entry(le, &me_idata->entry_list, node)
	{
		rule_found = false;
		for (sort_idx = 0; sort_idx < me_idata->entries_num; sort_idx++)
		{
			if (memcmp(&le->table_entry.data, sort_array[sort_idx], 8) == 0)
			{
				le->table_entry.idx = sort_idx;
				rule_found = true;
				break;
			}
		}

		/* this should normally not happen*/
		if (!rule_found)
		{
			dbprintf(LOG_ERR, "%u@%u Can't find table entry in the sorted list!\n", me_info->me->classid, me_info->me->meid);

			//error = OMCI_ERROR_INVALID_VAL;

			free_safe(sort_array);

 			//return error;
 			return -1;
		}
	}

	filter = malloc_safe(sizeof(struct pon_adapter_ext_vlan_filter) * me_idata->entries_num);
	if( NULL == filter)
	{
		free_safe(sort_array);
		dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
		return -1;
	}

	for (i = 0; i < me_idata->entries_num; i++)
	{
		filter[i].filter_outer_priority		= sort_array[i]->filter_outer_prio;
		filter[i].filter_outer_vid			= sort_array[i]->filter_outer_vid;
		filter[i].filter_outer_tpid_de		= sort_array[i]->filter_outer_tpid_de;
		filter[i].filter_inner_priority		= sort_array[i]->filter_inner_prio;
		filter[i].filter_inner_vid			= sort_array[i]->filter_inner_vid;
		filter[i].filter_inner_tpid_de		= sort_array[i]->filter_inner_tpid_de;
		filter[i].filter_ethertype			= sort_array[i]->filter_ether_type;
		filter[i].treatment_tags_to_remove	= sort_array[i]->treatment_tags_remove;
		filter[i].treatment_outer_priority	= sort_array[i]->treatment_outer_prio;
		filter[i].treatment_outer_vid		= sort_array[i]->treatment_outer_vid;
		filter[i].treatment_outer_tpid_de	= sort_array[i]->treatment_outer_tpid_de;
		filter[i].treatment_inner_priority	= sort_array[i]->treatment_inner_prio;
		filter[i].treatment_inner_vid		= sort_array[i]->treatment_inner_vid;
		filter[i].treatment_inner_tpid_de	= sort_array[i]->treatment_inner_tpid_de;
	}

	fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
	PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan, rules_add),
		    (me_info->me->meid, ds_mode, filter,
		    me_idata->entries_num,
		    1));
		    //omci_iop_mask_isset(context, OMCI_IOP_OPTION_2)));
	fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
	if (ret != PON_ADAPTER_SUCCESS)
	{
		dbprintf(LOG_ERR, "%u@%u ERR(%d) Couldn't add comm rule!\n", me_info->me->classid, me_info->me->meid, ret);
	}

	free_safe(sort_array);
	free_safe(filter);

	if( PON_ADAPTER_SUCCESS != ret)
	{
		dbprintf(LOG_ERR, "switch_hw_prx126_classf_comm_rule_add(), , meid:%d, fail!", me_info->me->meid);
		return ret;
	}

	return ret;
}



/** Add/Delete/Clear multicast address table

   \param[in] context     OMCI context pointer
   \param[in] me          Managed Entity pointer
   \param[in] tbl_entry   Table entry
   \param[in] ds_mode     Downstream mode
   \param[in] def         Default entry
*/
static int
switch_hw_prx126_classf_rx_vlan_oper_table_entry_set(struct intel_omci_context *context,
			     struct prx126_classf_me_info_t *me_info,
			     const struct prx126_classf_omci_rx_vlan_oper_table *tbl_entry,
			     const uint8_t ds_mode,
			     const bool def)
{
	enum pon_adapter_errno ret;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct prx126_classf_omci_rx_vlan_oper_table entry;
	bool entry_found;
	unsigned int entry_idx = 0;
	struct pon_adapter_ext_vlan_filter *filter;
#if defined(OMCI_SWAP)
	uint32_t data32[4];
#endif
	struct prx126_classf_rx_vlan_oper_list_entry_t *entry_pos, *entry_pos_n;


	memcpy(&entry, tbl_entry, sizeof(struct prx126_classf_omci_rx_vlan_oper_table));

	me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info->internal_data;

#if defined(OMCI_SWAP)
	/* The default rules are already defined in host byte order.
	   No need to swap them. See rx_vlan_def */
	if (!def) {
		memcpy(&data32, &entry, sizeof(data32));
		data32[0] = ntoh32(data32[0]);
		data32[1] = ntoh32(data32[1]);
		data32[2] = ntoh32(data32[2]);
		data32[3] = ntoh32(data32[3]);
		memcpy(&entry, &data32, sizeof(data32));
	}
#endif

	if (entry.treatment_tags_remove == 0x3
	    && entry.treatment_outer_prio == 0xf
	    && entry.treatment_outer_vid == 0x1fff
	    && entry.treatment_outer_tpid_de == 0x7
	    && entry.treatment_inner_prio == 0xf
	    && entry.treatment_inner_vid == 0x1fff
	    && entry.treatment_inner_tpid_de == 0x7) {
		/* delete entry */

		dbprintf(LOG_ERR, "%u@%u Delete entry request!\n", me_info->me->classid, me_info->me->meid);

		entry_found = false;
		//prx126_classf_ext_vlan_internal_data.next;
		list_for_each_entry_safe(entry_pos, entry_pos_n, &me_idata->entry_list, node)
		{
			if (entry_pos->table_entry.def == 0 && memcmp(&entry_pos->table_entry.data, &entry, 8) == 0)
			{
				switch_hw_prx126_classf_dump_entry(entry_pos->me, "Delete entry", &entry_pos->table_entry.data);
				entry_found = true;
				entry_idx = entry_pos->table_entry.idx;
				filter = malloc_safe( sizeof(struct pon_adapter_ext_vlan_filter) );
	
				if( NULL == filter)
				{
					dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
					return -1;
				}

				filter->filter_outer_priority 	 = entry_pos->table_entry.data.filter_outer_prio;
				filter->filter_outer_vid 		 = entry_pos->table_entry.data.filter_outer_vid;
				filter->filter_outer_tpid_de 	 = entry_pos->table_entry.data.filter_outer_tpid_de;
				filter->filter_inner_priority	 = entry_pos->table_entry.data.filter_inner_prio;
				filter->filter_inner_vid		 = entry_pos->table_entry.data.filter_inner_vid;
				filter->filter_inner_tpid_de	 = entry_pos->table_entry.data.filter_inner_tpid_de;
				filter->filter_ethertype		 = entry_pos->table_entry.data.filter_ether_type;
				filter->treatment_tags_to_remove = entry_pos->table_entry.data.treatment_tags_remove;
				filter->treatment_outer_priority = entry_pos->table_entry.data.treatment_outer_prio;
				filter->treatment_outer_vid		 = entry_pos->table_entry.data.treatment_outer_vid;
				filter->treatment_outer_tpid_de	 = entry_pos->table_entry.data.treatment_outer_tpid_de;
				filter->treatment_inner_priority = entry_pos->table_entry.data.treatment_inner_prio;
				filter->treatment_inner_vid		 = entry_pos->table_entry.data.treatment_inner_vid;
				filter->treatment_inner_tpid_de	 = entry_pos->table_entry.data.treatment_inner_tpid_de;


				if (entry_pos->table_entry.def)
				{
					--me_idata->def_entries_num;
				}
				--me_idata->entries_num;

				list_del(&entry_pos->node);
				free_safe(entry_pos);

				dbprintf(LOG_ERR, "%u@%u Removed table entry (entries num = %zu)!\n", me_info->me->classid, me_info->me->meid, --me_idata->entries_num);

				break;
			}
		}

		if (entry_found) {
			list_for_each_entry_safe(entry_pos, entry_pos_n, &me_idata->entry_list, node)
			{
				if (entry_pos->table_entry.idx > entry_idx)
					entry_pos->table_entry.idx--;
			}

			fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
			PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan,
						   rule_remove),
					(me_info->me->meid, filter, ds_mode));
			fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
			free_safe(filter);

			if (ret != PON_ADAPTER_SUCCESS) {
				dbprintf(LOG_ERR, "%u@%u DRV ERR(%d) Can't delete table entry!\n", ret);
				return ret;
			}
		}
	} else {
		/* set entry */
		dbprintf(LOG_ERR, "%u@%u Set entry request!\n", me_info->me->classid, me_info->me->meid);

		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_HUAWEI)) // huawei
		{
			ret = switch_hw_prx126_classf_hwtc_rule_add(prx126_classf_context, me_info, ds_mode, def, &entry);
		}
		else if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
		   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
		{
			ret = switch_hw_prx126_classf_alcl_rule_add(context, me_info, ds_mode, def, &entry);

		}
		else
		{
			ret = switch_hw_prx126_classf_comm_rule_add(context, me_info, ds_mode, def, &entry);
		}

		if( ret !=0 )
		{
			return -1;
		}
	}


	return ret;
}

static void
switch_hw_prx126_classf_vlan_collect_rule_fields_from_table_entry(unsigned char *entry,
	struct prx126_classf_omci_rx_vlan_oper_table *rx_vlan_oper_table)
{
	int i, entry_size, bit_start = 0;
	struct attrinfo_table_t *attrinfo_table_ptr;


	attrinfo_table_ptr = meinfo_util_aitab_ptr(171, 6);
	entry_size = attrinfo_table_ptr->entry_byte_size;

	//extract entry fields
 	for (i = 0; i < attrinfo_table_ptr->field_total; i++)
	{
		if ( i == 0) {
			bit_start = 0;
		} else {
			bit_start += attrinfo_table_ptr->field[i-1].bit_width;
		}
		switch(i)
		{
		case 0:
			//rule_fields->filter_outer.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_outer_prio = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 1:
			//rule_fields->filter_outer.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_outer_vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 2:
			//rule_fields->filter_outer.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_outer_tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 3:
			//rule_fields->filter_outer.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->word1_padding = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 4:
			//rule_fields->filter_inner.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_inner_prio = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 5:
			//rule_fields->filter_inner.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_inner_vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 6:
			//rule_fields->filter_inner.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_inner_tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 7:
			//rule_fields->filter_inner.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->word2_padding = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 8:
			//rule_fields->filter_ethertype = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->filter_ether_type = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 9:
			//rule_fields->treatment_tag_to_remove = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_tags_remove = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 10:
			//rule_fields->treatment_outer.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->word3_padding = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 11:
			//rule_fields->treatment_outer.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_outer_prio = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 12:
			//rule_fields->treatment_outer.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start,  attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_outer_vid = util_bitmap_get_value(entry, 8*entry_size, bit_start,  attrinfo_table_ptr->field[i].bit_width);
			break;
		case 13:
			//rule_fields->treatment_outer.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_outer_tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 14:
			//rule_fields->treatment_inner.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->word4_padding = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 15:
			//rule_fields->treatment_inner.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_inner_prio = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 16:
			//rule_fields->treatment_inner.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_inner_vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 17:
			//rule_fields->treatment_inner.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			rx_vlan_oper_table->treatment_inner_tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		default:
			; //do nothing
		}
	}

	return;
}

static int
switch_hw_prx126_classf_extended_vlan_config_data_add(void)
{
	struct meinfo_t *miptr_171;
	struct me_t *me_pos_171;
	struct prx126_classf_me_info_t *me_info;
	struct prx126_classf_omci_rx_vlan_oper_table rx_vlan_oper_table;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct attr_value_t attr_value={0, NULL};
	struct pon_adapter_ext_vlan_update update_data;
	int entry_count;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	int i;
	int ret = INTEL_PON_ERROR;


	if ((miptr_171 = meinfo_util_miptr(171)) == NULL) // 171, Extended_VLAN_tagging_operation
	{
		dbprintf(LOG_ERR, "get 171 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_171, &miptr_171->me_instance_list, instance_node)
	{
		me_info = malloc_safe(sizeof(struct prx126_classf_me_info_t));
		if( NULL == me_info)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}

		me_info->me = me_pos_171;
		me_info->classid = 171;
		me_info->meid = me_pos_171->meid;

		me_info->internal_data = malloc_safe(sizeof(struct prx126_classf_ext_vlan_idata_t));
		if( NULL == me_info->internal_data)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}
		me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info->internal_data;

		INIT_LIST_HEAD(&me_idata->entry_list);
		me_idata->entries_num = 0;
		me_idata->def_entries_num = 0;

		meinfo_me_attr_get(me_pos_171, 1, &attr_value); //Association_type
		update_data.association_type = attr_value.data;
		meinfo_me_attr_get(me_pos_171, 7, &attr_value); //Associated_ME_pointer
		update_data.associated_ptr = attr_value.data;
		meinfo_me_attr_get(me_pos_171, 3, &attr_value); //Input_TPID
		update_data.input_tpid = attr_value.data;
		meinfo_me_attr_get(me_pos_171, 4, &attr_value); //Output_TPID
		update_data.output_tpid = attr_value.data;
		meinfo_me_attr_get(me_pos_171, 5, &attr_value); //Downstream_mode
		update_data.ds_mode = attr_value.data;
		meinfo_me_attr_get(me_pos_171, 8, &attr_value); //Association_type
		update_data.dscp = attr_value.ptr;

		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret,
			(omci_me_ops, ext_vlan, update),
			(&update_data, me_pos_171->meid));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
		if (ret != 0) {
			dbprintf(LOG_ERR, "ext_vlan, update fail!");
			return -1;
		}

		entry_count = meinfo_me_attr_get_table_entry_count(me_pos_171, 6);

		/* Create default entries*/
		if (!entry_count)
		{
			for (i = 0; i < PRX126_CLASSF_OMCI_ONU_EXT_VLAN_DEFAULT_RULE_MAX; i++)
			{
				ret = switch_hw_prx126_classf_rx_vlan_oper_table_entry_set(prx126_classf_context, me_info,
									 &prx126_classf_rx_vlan_def[i],
									 update_data.ds_mode,
									 true);
				if( ret != 0)
				{
					return -1;
				}
			}
		}

		//get tagging table
		if ((table_header = (struct attr_table_header_t *) me_pos_171->attr[6].value.ptr) == NULL) {
			dbprintf(LOG_ERR, "attr value was not table\n");
			return (-1);
		}

		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				//fill to entry fields
				memset(&rx_vlan_oper_table, 0x0, sizeof(struct prx126_classf_omci_rx_vlan_oper_table));
				switch_hw_prx126_classf_vlan_collect_rule_fields_from_table_entry((unsigned char *)table_entry_pos->entry_data_ptr, &rx_vlan_oper_table);

				ret = switch_hw_prx126_classf_rx_vlan_oper_table_entry_set(prx126_classf_context, me_info,
										 &rx_vlan_oper_table,
										 update_data.ds_mode,
										 false);

					if( PON_ADAPTER_SUCCESS != ret)
					{
					dbprintf(LOG_ERR, "mac_bp_config_data, meid:%u, destory fail!", me_info->meid);
						return ret;
					}
				}
			}

		// insert;
		list_add_tail(&me_info->me_node, &prx126_classf_me_171_list);
	}

	return ret;
}

static int
switch_hw_prx126_classf_extended_vlan_config_data_del(void)
{

	struct prx126_classf_me_info_t *me_info_pos;
	struct prx126_classf_me_info_t *me_info_n;
	struct prx126_classf_rx_vlan_oper_list_entry_t *le;
	struct prx126_classf_rx_vlan_oper_list_entry_t *le_n;
	struct prx126_classf_ext_vlan_idata_t *me_idata;
	struct attr_value_t attr_value;
	int ret = INTEL_PON_ERROR;


	list_for_each_entry_safe(me_info_pos, me_info_n, &prx126_classf_me_171_list, me_node)
	{
		meinfo_me_attr_get(me_info_pos->me, 5, &attr_value); //Downstream_mode

		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan, rule_clear_all),
			   (me_info_pos->meid, attr_value.data));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);

		me_idata = (struct prx126_classf_ext_vlan_idata_t *)me_info_pos->internal_data;

		/* clear RX VLAN operation table */
		list_for_each_entry_safe(le, le_n, &me_idata->entry_list, node)
		{
			--me_idata->entries_num;
			if (le->table_entry.def)
			{
				--me_idata->def_entries_num;
			}


			list_del(&le->node);
			free_safe(le);
		}

		free_safe(me_info_pos->internal_data);

		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, ext_vlan, destroy),
				(me_info_pos->meid));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);

		list_del(&me_info_pos->me_node);
		free_safe(me_info_pos);
	}

	return ret;
}

static int
switch_hw_prx126_classf_vlan_tagging_operation_config_data_add(void)
{
	struct meinfo_t *miptr_78;
	struct me_t *me_pos_78;
	struct prx126_classf_me_info_t *me_info;
	struct attr_value_t attr_value;
	struct pa_vlan_tagging_operation_config_data_update_data vlan_tag_update_data;
	int ret = INTEL_PON_ERROR;


	if ((miptr_78 = meinfo_util_miptr(78)) == NULL) // 78, vlan_tag_oper_cfg_data
	{
		dbprintf(LOG_ERR, "get 84 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_78, &miptr_78->me_instance_list, instance_node)
	{
		me_info = malloc_safe(sizeof(struct prx126_classf_me_info_t));
		if( NULL == me_info)
		{
			dbprintf(LOG_ERR, "Fail: malloc_safe()\n");
			return -1;
		}

		me_info->me = me_pos_78;
		me_info->classid = 78;
		me_info->meid = me_pos_78->meid;

		meinfo_me_attr_get(me_pos_78, 1, &attr_value);
		vlan_tag_update_data.us_vlan_tag_oper_mode = attr_value.data;
		meinfo_me_attr_get(me_pos_78, 2, &attr_value);
		vlan_tag_update_data.us_vlan_tag_tci_value = attr_value.data;
		meinfo_me_attr_get(me_pos_78, 3, &attr_value);
		vlan_tag_update_data.ds_vlan_tag_oper_mode = attr_value.data;
		meinfo_me_attr_get(me_pos_78, 4, &attr_value);
		vlan_tag_update_data.association_type = attr_value.data;
		meinfo_me_attr_get(me_pos_78, 5, &attr_value);
		vlan_tag_update_data.association_ptr = attr_value.data;

		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, vlan_tag_oper_cfg_data, update), (me_pos_78->meid, &vlan_tag_update_data));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
		if( PON_ADAPTER_SUCCESS != ret)
		{
			dbprintf(LOG_ERR, "switch_hw_prx126_classf_vlan_tagging_operation_config_data_add(), , meid:%d, update fail!", me_pos_78->meid);
		}

		// insert;
		list_add_tail(&me_info->me_node, &prx126_classf_me_78_list);
	}

	return 0;
}

static int
switch_hw_prx126_classf_vlan_tagging_operation_config_data_del(void)
{
	struct prx126_classf_me_info_t *me_info_pos;
	struct prx126_classf_me_info_t *me_info_n;
	int ret = INTEL_PON_ERROR;


	list_for_each_entry_safe(me_info_pos, me_info_n, &prx126_classf_me_78_list, me_node)
	{
		fwk_mutex_lock(&prx126_classf_pa_node_pon_net->pa_lock);
		PA_CALL_ALL(prx126_classf_context, ret, (omci_me_ops, vlan_tag_oper_cfg_data,
			destroy), (me_info_pos->meid));
		fwk_mutex_unlock(&prx126_classf_pa_node_pon_net->pa_lock);
		if( PON_ADAPTER_SUCCESS != ret)
		{
			dbprintf(LOG_ERR, "switch_hw_prx126_classf_vlan_tagging_operation_config_data_del, meid:%u, destory fail!", me_info_pos->meid);
		}

		list_del(&me_info_pos->me_node);
		free_safe(me_info_pos);
	}

	return 0;

}


static int
switch_hw_prx126_classf_reset(void)
{
	switch_hw_prx126_classf_vlan_tagging_filter_data_del();
	switch_hw_prx126_classf_extended_vlan_config_data_del();
	switch_hw_prx126_classf_vlan_tagging_operation_config_data_del();

	return 0;
}


static int
switch_hw_prx126_classf_create(void)
{
	switch_hw_prx126_classf_vlan_tagging_filter_data_add();
	switch_hw_prx126_classf_extended_vlan_config_data_add();
	switch_hw_prx126_classf_vlan_tagging_operation_config_data_add();

	return 0;
}

// classf enter point, input is all bridges' rules info, each bridge's lan ports were different
//*****analyse for hw dependent requirement
//(1) find hw dependent stag tpid
//(2) check ethertype rules
//(3) find the rules for downstream two tags with outer tag was the same on a lan port
//(4) separate rules to 3 parts,
int
switch_hw_prx126_classf_manage(struct classf_info_t *classf_info)
{

	if(!classf_info->is_hwsync)
	{
		switch_hw_prx126_classf_reset();
	}
	else
	{
		switch_hw_prx126_classf_create();
	}

	return 0;
}

//type: 0: all, 1: acl-hit, 2: ethertype, 3: basic
//flag: 0: hw entries, 1: help
int
switch_hw_prx126_classf_print(int fd, unsigned char type, unsigned char flag)
{
	return 0;
}


