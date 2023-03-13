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
 * File    : classf.h
 *
 ******************************************************************/

#ifndef _CLASSF_H_
#define _CLASSF_H_

#include "netinet/in.h"
#include "omciutil_vlan.h"
#include "fwk_mutex.h"

#define CLASSF_GEM_INDEX_NUM 16 //gem port index number for a wan port
#define CLASSF_TCI_NUM 12 //tci number for a port filter
#define CLASSF_BRIDGE_PORT_TYPE_UNI_DC ENV_BRIDGE_PORT_TYPE_UNUSED //virtual port type, uni port num DON'T CARE, for alu port 2 port mapping, to sort down to the bottom that has lower priority.
#define CLASSF_VLAN_UNTAG_ID 4097

enum classf_filter_op_type_t
{
	CLASSF_FILTER_OP_TYPE_PASS,
	CLASSF_FILTER_OP_TYPE_DISCARD,
	CLASSF_FILTER_OP_TYPE_POSTIVE,
	CLASSF_FILTER_OP_TYPE_NEGATIVE,
	CLASSF_FILTER_OP_TYPE_INVALID
};

enum classf_filter_inspection_type_t
{
	CLASSF_FILTER_INSPECTION_TYPE_UNUSED,
	CLASSF_FILTER_INSPECTION_TYPE_VID,
	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY,
	CLASSF_FILTER_INSPECTION_TYPE_TCI,
	CLASSF_FILTER_INSPECTION_TYPE_INVALID
};

/*for upstream and downstream rule candidate filter_type
untag
0: drop
1: pass
tag
2: drop
3: pass
4-15: postive with tci index
16: negative
*/
enum classf_filter_op_type_ext_t
{
	CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS,
	CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_DISCARD,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_1,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_2,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_3,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_4,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_5,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_6,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_7,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_8,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_9,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_10,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11,
	CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE,
	CLASSF_FILTER_OP_TYPE_EXT_INVALID
};

enum classf_gem_dir_t
{
	CLASSF_GEM_DIR_DISABLE,
	CLASSF_GEM_DIR_US,
	CLASSF_GEM_DIR_DS,
	CLASSF_GEM_DIR_BOTH,
	CLASSF_GEM_DIR_INVALID
};

enum classf_gem_untag_op
{
	CLASSF_GEM_UNTAG_OP_DSCP,
	CLASSF_GEM_UNTAG_OP_DEFAULT_PBIT,
	CLASSF_GEM_UNTAG_OP_ALL_PASS,
	CLASSF_GEM_UNTAG_OP_NO_PASS,
	CLASSF_GEM_UNTAG_OP_INVALID
};

enum classf_rule_candi_reason
{
	CLASSF_REASON_OK,

	//lan filter
	CLASSF_REASON_LF_UNTAG_DISCARD,
	CLASSF_REASON_LF_TAG_DISCARD,
	CLASSF_REASON_LF_PRI_UNMATCH,
	CLASSF_REASON_LF_VID_UNMATCH,
	CLASSF_REASON_LF_DE_UNMATCH,
	CLASSF_REASON_LF_NEGATIVE_DISCARD,
	CLASSF_REASON_LF_TYPE_ERROR,

	//wan filter
	CLASSF_REASON_WF_UNTAG_DISCARD,
	CLASSF_REASON_WF_TAG_DISCARD,
	CLASSF_REASON_WF_PRI_UNMATCH,
	CLASSF_REASON_WF_VID_UNMATCH,
	CLASSF_REASON_WF_DE_UNMATCH,
	CLASSF_REASON_WF_NEGATIVE_DISCARD,
	CLASSF_REASON_WF_TYPE_ERROR,

	//gem/pbit filter
	CLASSF_REASON_GEM_UNTAG_DISCARD,
	CLASSF_REASON_GEM_TAG_DISCARD,
	CLASSF_REASON_GEM_TAG_PBIT_DISCARD,

	//misc error
	CLASSF_REASON_TAGGING_ERROR,
	CLASSF_REASON_DUPLICATE,
	CLASSF_REASON_INTERNAL_ERROR,
	CLASSF_REASON_MC_TAGGING_ERROR,
	CLASSF_REASON_MC_GEM_ERROR,
	
	CLASSF_REASON_INVALID,
};

enum classf_rule_special_case
{
	CLASSF_RULE_SPECIAL_CASE_NONE,
	CLASSF_RULE_SPECIAL_CASE_FORCE_STRIP,
	CLASSF_RULE_SPECIAL_CASE_INVALID,
};

enum classf_rule_aggregation_code
{
	CLASSF_RULE_AGGREGATION_NORMAL, //0, definitely, also for normal rules
	CLASSF_RULE_AGGREGATION_1PBIT_MASK, //unmask less significant 1bit priority mask, to aggregate two-rules
	CLASSF_RULE_AGGREGATION_PBIT_WILDCARD, //add one more rule, pbit wildcard, use pbit 0 gem port to forward, overwrite pbit assignment -> 0
	CLASSF_RULE_AGGREGATION_SKIP, //skip, save acl usage
};

struct classf_filter_op_map_t
{
	unsigned char op;
	unsigned char tag_rx;
	unsigned char tag_tx;
	unsigned char untag_rx;
	unsigned char untag_tx;
	unsigned char inspection_type;	
};

//for wan and lan ports
struct classf_port_filter_t
{
	struct me_t *filter_me; //NULL is no filter me found
	struct me_t *bridge_port_me; //bridge port me
	struct me_t *bridge_me; //bridge me
	//untag
	unsigned char untag_rx; //0:pass, 1:discard
	unsigned char untag_tx; //0:pass, 1:discard
	//tag
	unsigned char tag_rx; //0:pass, 1:discard, 2: postive, 3: negative
	unsigned char tag_tx; //0:pass, 1:discard, 2: postive, 3: negative
	unsigned char vlan_tci_num;
	unsigned short vlan_tci[CLASSF_TCI_NUM];
	unsigned char vlan_inspection_type; //filter_inspection_type_t
	unsigned char low_priority; //0: default, 1: low priority
};

struct classf_port_gem_t
{
	struct me_t *bridge_port_me; //bridge wan port me
	struct me_t *bridge_me; //bridge med
	unsigned char gem_type; //0: regular data, 1: multicast
	unsigned char gem_num;
	unsigned char gem_index_total;
	unsigned short gem_port[CLASSF_GEM_INDEX_NUM]; //8 pbits + 8 untag pbits
	unsigned char gem_tagged[CLASSF_GEM_INDEX_NUM]; //0: untagged, 1: tagged
	struct me_t *gem_ctp_me[CLASSF_GEM_INDEX_NUM];
	unsigned char pbit_mask[CLASSF_GEM_INDEX_NUM];
	unsigned char dir[CLASSF_GEM_INDEX_NUM]; // 1: US, 2: DS, 3: BOTH
	unsigned char colour_marking_us[CLASSF_GEM_INDEX_NUM]; //0: disable, 2: de, 3: 8P0D, 4: 7P1D, 5: 6P2D, 6: 5P3D, otherwise: disable
	unsigned char untag_op; //0: dscp, 1: default pbit, 3:untag all pass, 4: untag no pass
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;
};

struct classf_port_tagging_t
{
	unsigned char us_tagging_rule_default; //0: NOT default, 1: default
	unsigned char ds_tagging_rule_default; //0: NOT default, 1: default
	struct list_head us_tagging_rule_list;
	struct list_head ds_tagging_rule_list;
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;
};

struct classf_filter_result_t
{
	struct list_head expected_filter_list; //main filter list
	struct list_head unexpected_filter_list; //list negative filter dependent on main filter
};

struct classf_filter_node_t
{
	struct omciutil_vlan_tag_fields_t filter;
	unsigned char colour; //0: green(default), 1: yellow, 2: de green, 3: de yellow
	struct list_head f_node;
};

struct classf_port_protocol_vlan_t
{
	struct list_head us_pvlan_rule_list; // node is classf_pvlan_rule_t
	struct list_head ds_pvlan_rule_list; // node is classf_pvlan_rule_t
};

struct classf_pvlan_acl_care_bit_t
{
	unsigned int ethertype:1;
	//mac
	unsigned int smac:1;
	unsigned int smac_mask:1;
	unsigned int dmac:1;
	unsigned int dmac_mask:1;
	//ip
	unsigned int sip:1;
	unsigned int sip_mask:1;
	unsigned int dip:1;
	unsigned int dip_mask:1;
	//ipv6
	unsigned int v6sip:1;
	unsigned int v6sip_prefix:1;
	unsigned int v6dip:1;
	unsigned int v6dip_prefix:1;
	//tcp=1/udp=0
	unsigned int protocol:1;
	unsigned int v6nd_target_ip:1;
	unsigned int sport:1;
	unsigned int sport_ubound:1;
	unsigned int dport:1;
	unsigned int dport_ubound:1;
};

struct classf_pvlan_acl_t
{
	//rule number from configuration
	unsigned int rule_num;
	//match priority
	unsigned int order;
	//uni port(0,1,2,3) and wifi port (6) wildcard flag, for hw setting
	unsigned char uniport_wildcard;
	unsigned char uniport_wildcard_range;
	//ACL part
	//ethertype
	unsigned short ethertype;
	//source mac
	unsigned char smac[6];
	//source mac mask
	unsigned char smac_mask[6];
	//destination mac
	unsigned char dmac[6];
	//destination mac mask
	unsigned char dmac_mask[6];
	//source ip
	struct in_addr sip;
	//source ip mask
	struct in_addr sip_mask;
	//destination ip
	struct in_addr dip;
	//destination ip mask
	struct in_addr dip_mask;
	//v6 source ip
	struct in6_addr v6sip;
	//v6 source ip prefix
	unsigned char v6sip_prefix;
	//v6 destination ip
	struct in6_addr v6dip;
	//v6 destination ip prefix
	unsigned char v6dip_prefix;
	//ip protocol
	unsigned char protocol; //tcp/udp
	//v6 icmp nd target address
	struct in6_addr v6nd_target_ip;
	//source port
	unsigned short sport;
	unsigned short sport_ubound;
	//destination port
	unsigned short dport;
	unsigned short dport_ubound;
	//flow vlan id, 0~4095, 4097 untag
	unsigned short flow_vid; //flow vid for traffic statistic, this is also some kind of criterion
	unsigned char fake_arp_to_ip; //flag to mark faking arp ip as ip so it can be matched by ipv4 acl rule
	struct classf_pvlan_acl_care_bit_t care_bit;
};

struct classf_pvlan_rule_t
{
	unsigned char enable;
	unsigned char join_disable; //0: default, join , 1: do not join.
	unsigned char uniport; //logical uni port number
	unsigned char uniport_valid;
	unsigned char vlantag_enable;
	unsigned short vlantag_id;
	unsigned char vlantag_pbit;
	unsigned short vlan_mark;
	unsigned char pbit_mark;
	int join_entry_id;
	struct classf_pvlan_acl_t pvlan_acl;
	struct omciutil_vlan_rule_fields_t rule_fields;
	struct list_head pr_node;
};

struct classf_rule_candi_t
{
	unsigned int entry_id;
	unsigned char reason; //after determining

	//wan
	struct classf_bridge_info_wan_port_t *wan_port; //bridge wan port info
	//gem
	unsigned char gem_index; //gem port index
	//wan vlan filter
	unsigned char wan_filter_valid; 
	unsigned char wan_filter_type; //classf_filter_op_type_ext_t

	//lan
	struct classf_bridge_info_lan_port_t *lan_port; //bridge lan port info
	//lan vlan filter
	unsigned char lan_filter_valid;
	unsigned char lan_filter_type; //classf_filter_op_type_ext_t
	//vlan tagging
	struct omciutil_vlan_rule_fields_t *rule_fields;
	unsigned char dscp2pbit_enable; //only for us, 0: disable, 1: from inner treatment, 2: from outer treatment

	//protocol vlan acl
	struct classf_pvlan_acl_t *pvlan_acl;

	struct list_head rc_node;
};

//to list acl or classify entry list
struct classf_entry_node_t
{
	int id;
	struct list_head e_node;
};

struct classf_rule_fields_tpid_t
{
	unsigned short outer_filter_tpid;
	unsigned short inner_filter_tpid;
	unsigned short outer_treatment_tpid;
	unsigned short inner_treatment_tpid;
};

struct classf_rule_tlan_t
{
	unsigned char flag; //0: non-TLS rule, 1: TLS rule
	unsigned int lan_mask; //for upstream
	unsigned int aggregation_flag;
	void *aggregation_data; //store additional data for aggregation
};

struct classf_rule_t
{
	unsigned int entry_id;
	struct classf_bridge_info_wan_port_t *wan_port; //bridge wan port info
	unsigned char gem_index; //gem port index
	struct classf_bridge_info_lan_port_t *lan_port; //bridge lan port info
	unsigned int uni_port_mask; //uni port mask in the same bridge
	struct omciutil_vlan_rule_fields_t rule_fields; //final tagging rule
	struct classf_rule_fields_tpid_t fields_tpid;
	unsigned char colour; //0: green(default), 1: yellow, 2: de green, 3: de yellow
	struct classf_rule_tlan_t tlan_info; //store tlan information 
	//protocol vlan
	struct classf_pvlan_acl_t *pvlan_acl;
	unsigned char dscp2pbit_enable; //only for us, 0: disable, 1: from inner treatment, 2: from outer treatment
	unsigned int rule_candi_entry_id; //from which classf rule candi
	//for hw information
	int hw_stream_id; //store hw flow(stream) id
	unsigned short hw_stream_id_mask;
	int hw_acl_sub_order; //store hw acl table entry number
	int hw_stat_id; //store hw statistic id
	enum classf_rule_special_case special_case; //mark for special case while setting rule to hw
	struct list_head acl_entry_list; // node is classf_entry_node_t
	struct list_head classify_entry_list; // node is classf_entry_node_t
	struct list_head r_node;
	struct list_head r_node_cpu; //for veip (and iphost) rules list
};

struct classf_bridge_info_wan_port_t
{
	struct me_t *me_bridge;
	struct me_t *me_bridge_port;
	//unsigned char port_id;
	unsigned char port_type_index;
	unsigned char logical_port_id;
	struct classf_port_gem_t port_gem;
	struct classf_port_filter_t port_filter;
	struct list_head wp_node;
};

struct classf_bridge_info_lan_port_t
{
	struct me_t *me_bridge;
	struct me_t *me_bridge_port;
	unsigned char port_type;
	unsigned char port_type_index;
	unsigned char port_type_sort; //for rule sorting
	unsigned char port_type_index_sort; //for rule sorting
	//unsigned char port_id; //for mapping to physical port
	unsigned char logical_port_id; //for mapping to physical port
	struct classf_port_filter_t port_filter;
	struct classf_port_tagging_t port_tagging;
	//protocol vlan rule
	struct classf_port_protocol_vlan_t port_protocol_vlan;

	struct list_head lp_node;
};

struct classf_bridge_info_t
{
	unsigned char cpu_port_num;
	struct me_t *me_bridge;
	struct list_head wan_port_list; // node is classf_bridge_info_wan_port_t
	struct list_head lan_port_list; // node is classf_bridge_info_lan_port_t
	struct list_head rule_candi_list_us; // node is classf_rule_candi_t
	struct list_head rule_candi_list_ds; // node is classf_rule_candi_t
	struct list_head rule_list_us; // node is classf_rule_t
	struct list_head rule_list_ds; // node is classf_rule_t
	struct list_head b_node;
};

struct classf_brwan_info_t
{
	unsigned char enable;
	unsigned char portmask;
	unsigned char wifimask;
	unsigned short vid;
};

struct classf_system_info_t
{
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;
	struct list_head protocol_vlan_rule_list; //collect all rules first
	struct omciutil_vlan_tlan_info_t tlan_info;
	struct classf_brwan_info_t brwan_info;
};

struct classf_info_t
{
	unsigned int is_hwsync;
	struct list_head bridge_info_list; // node is classf_bridge_info_t
	struct classf_system_info_t system_info;
	struct fwk_mutex_t mutex;
};

int classf_rule_collect(struct classf_info_t *classf_info);
int classf_protocol_vlan_collect_rules_from_metafile(struct list_head *protocol_vlan_rule_list);
void classf_protocol_vlan_rule_generate_by_port(struct classf_bridge_info_lan_port_t *lan_port,
	struct list_head *veip_rule_list, //maybe NULL
	struct list_head *global_pvlan_rule_list,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info);

void classf_rule_release(struct classf_info_t *classf_info);

int classf_bridge_join(struct classf_bridge_info_t *bridge_info);

int classf_insert_rule_to_list_for_cpu(struct classf_rule_t *rule, struct list_head *rule_list);
int classf_bridge_rule_determine(struct classf_bridge_info_t *bridge_info, struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, struct omciutil_vlan_tlan_table_t *tlan_table);
int classf_release_rule(struct classf_rule_t *rule);

void classf_print_show_port_pair(int fd, unsigned char flag, struct classf_info_t *classf_info, unsigned short meid_lan, unsigned short meid_wan);
void classf_print_show_bridge(int fd, unsigned char flag, struct classf_info_t *classf_info, unsigned short meid);

#endif //#ifndef _CLASSF_H_

