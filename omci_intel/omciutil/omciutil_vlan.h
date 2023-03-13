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
 * File    : omciutil_vlan.h
 *
 ******************************************************************/

#ifndef __OMCIUTIL_VLAN_H__
#define __OMCIUTIL_VLAN_H__

#include <list.h>

#define OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST 0
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT 5
#define OMCIUTIL_VLAN_SORT_PRIORITY_HIGHEST 10
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_1 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT - 1)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT - 2)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_3 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT - 3)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_4 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT - 4)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_1 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT + 1)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_2 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT + 2)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_3 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT + 3)
#define OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_4 (OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT + 4)

/* filter priority */
enum omciutil_vlan_filter_priority_t {
	OMCIUTIL_VLAN_FILTER_PRIORITY_0 = 0,
	OMCIUTIL_VLAN_FILTER_PRIORITY_1,
	OMCIUTIL_VLAN_FILTER_PRIORITY_2,
	OMCIUTIL_VLAN_FILTER_PRIORITY_3,
	OMCIUTIL_VLAN_FILTER_PRIORITY_4,
	OMCIUTIL_VLAN_FILTER_PRIORITY_5,
	OMCIUTIL_VLAN_FILTER_PRIORITY_6,
	OMCIUTIL_VLAN_FILTER_PRIORITY_7,
	OMCIUTIL_VLAN_FILTER_PRIORITY_8,
	OMCIUTIL_VLAN_FILTER_PRIORITY_13 = 13, //for dscp2pbit reversing indicator
	OMCIUTIL_VLAN_FILTER_PRIORITY_14 = 14,
	OMCIUTIL_VLAN_FILTER_PRIORITY_15 = 15
};

/* filter tpid */
enum omciutil_vlan_filter_tpid_de_t {
	OMCIUTIL_VLAN_FILTER_TPID_DE_000 = 0,
	OMCIUTIL_VLAN_FILTER_TPID_DE_100 = 4,
	OMCIUTIL_VLAN_FILTER_TPID_DE_101 = 5,
	OMCIUTIL_VLAN_FILTER_TPID_DE_110 = 6,
	OMCIUTIL_VLAN_FILTER_TPID_DE_111 = 7,
	//these three for reversing op, translate treatment's tpid/de to filter's tpid/de
	OMCIUTIL_VLAN_FILTER_TPID_DE_1000 = 8, //tpid = output tpid, de do'nt care
	OMCIUTIL_VLAN_FILTER_TPID_DE_1001 = 9, //tpid = output tpid, de = 0
	OMCIUTIL_VLAN_FILTER_TPID_DE_1010 = 10, //tpid = output tpid, de = 1
	OMCIUTIL_VLAN_FILTER_TPID_DE_1011 = 11, //tpid = don't care, de = 0
	OMCIUTIL_VLAN_FILTER_TPID_DE_1100 = 12, //tpid = don't care, de = 1
	//only for flow pass
	OMCIUTIL_VLAN_FILTER_TPID_DE_10000 = 16, //tpid = 8100, de = 0
	OMCIUTIL_VLAN_FILTER_TPID_DE_10001 = 17, //tpid = 8100, de = 1
};

/* ZTE proprietary ethertype
 * TCP: 6
 * UDP: 7
 * ICMPv6: 8
 * ICMP: 9
 * IPoEv6: 10
 * DHCP: 11
 * IGMP: 12
 * SIP: 13
 * PWE3: 14
 * MAC-Control: 15 */

/* filter ethertype */
enum omciutil_vlan_filter_ethertype_t {
	OMCIUTIL_VLAN_FILTER_ET_IGNORE = 0,
	OMCIUTIL_VLAN_FILTER_ET_IP = 1,
	OMCIUTIL_VLAN_FILTER_ET_PPPOE = 2,
	OMCIUTIL_VLAN_FILTER_ET_ARP = 3,
	OMCIUTIL_VLAN_FILTER_ET_IPV6 = 4,
	OMCIUTIL_VLAN_FILTER_ET_IPV6_ZTE = 10,
	OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6 = 12,
	OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6 = 13,
	OMCIUTIL_VLAN_FILTER_ET_IP_ARP = 14,
	OMCIUTIL_VLAN_FILTER_ET_DEFAULT = 15 //for sort, this ethertype would fall to the bottom
};

/* treatment priority */
enum omciutil_vlan_treatment_priority_t {
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_0 = 0,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_1,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_2,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_3,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_4,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_5,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_6,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_7,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_8,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_9,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_10,
	OMCIUTIL_VLAN_TREATMENT_PRIORITY_15 = 15
};

/* treatment tpid */
enum omciutil_vlan_treatment_tpid_de_t {
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_000 = 0,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_001 = 1,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_010 = 2,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_011 = 3,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_100 = 4,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_101 = 5,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_110 = 6,
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_111 = 7,
	//these three for reversing op, translate filter's tpid/de to treatment's tpid/de
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000 = 8, //tpid = input tpid, de = 0
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001 = 9, //tpid = input tpid, de = 1
	//these two for 9.3.12, fill the treatment that the tpid as 8100
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 = 10, // de = 0;
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 = 11, // de = 1;
	//
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100 = 12, //tpid = input tpid, de = copy from inner
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101 = 13, //tpid = input tpid, de = copy from outer
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110 = 14, //tpid = 88a8, de = 0
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111 = 15, //tpid = 88a8, de = 1
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000 = 16, //tpid = copy from inner, de = 0
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001 = 17, //tpid = copy from inner, de = 1
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010 = 18, //tpid = copy from outer, de = 0
	OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011 = 19, //tpid = copy from outer, de = 1
};

enum omciutil_vlan_tag_code_t
{
	OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER,
	OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER,
	OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER,
	OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER,
	OMCIUTIL_VLAN_TAG_CODE_INVALID
};


#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
typedef enum omciutil_vlan_direct_t
{
    OMCIUTIL_VLAN_DIRECTION_US = 0,
    OMCIUTIL_VLAN_DIRECTION_DS,
    OMCIUTIL_VLAN_DIRECTION_END
}omciutil_vlan_direct;
#endif

struct omciutil_vlan_tag_fields_t
{
	unsigned char priority;
	unsigned short vid;
	unsigned char tpid_de;
	unsigned short pad;
};

struct omciutil_vlan_rule_l2_filter_t
{
	//upstream: source mac, downstream: destination mac.
	unsigned short index;
	unsigned char mac_mask[6];
	unsigned char mac[6];
	unsigned int lan_mask; //to mark logical lan ports with the same l2 mac criterion
};

struct omciutil_vlan_rule_fields_t
{
	unsigned int entry_id;
	struct omciutil_vlan_tag_fields_t filter_outer;
	struct omciutil_vlan_tag_fields_t filter_inner;
	unsigned char filter_ethertype;
	unsigned char treatment_tag_to_remove;
	struct omciutil_vlan_tag_fields_t treatment_outer;
	struct omciutil_vlan_tag_fields_t treatment_inner;
	unsigned short input_tpid;
	unsigned short output_tpid;
	unsigned char l2_filter_enable; //0: disable, 1: enable
	struct omciutil_vlan_rule_l2_filter_t l2_filter;
	unsigned char unreal_flag; //0:real, 1: unreal
	unsigned char sort_priority; //for proprietary me, 0: the lowest priority
	struct me_t *owner_me;	// me that the rule comes from
	struct me_t *bport_me;	// bridge port me that the rule is related 
	unsigned char veip_flag; //0: nothing, 1: mark for veip bridge rules
	unsigned char tlan_workaround_flag; 	// 0: nothing,
										// 1: transparent rule to add transparent double tags rule,
										// 2: mark for forcing tpid to 0x9100 on condition untag adds 1 tag,
										// 3: mark for forcing tpid to 0x9100 on condition 1 tag adds 1 tag,
										// 4 : change tag rule to add change double tags rule.
	unsigned short tlan_vid; //store tlan vid (meid)
	struct list_head rule_node;
};

struct omciutil_vlan_rule_result_node_t
{
	enum omciutil_vlan_tag_code_t code;
	struct list_head node;
};

struct omciutil_vlan_rule_result_list_t
{
	unsigned char tag_count;
	struct list_head tag_list;
};

struct omciutil_vlan_cb_t
{
	int (*omciutil_vlan_collect_rules_by_port)
		(int action, struct me_t *issue_bridge_port_me, struct me_t *issue_me, struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode);
};

struct omciutil_vlan_dscp2pbit_info_t
{
	unsigned char table[64];
	unsigned char pbit_mask;
	unsigned char enable;
	struct me_t *from_me;
};

#define CLASSF_TLAN_TABLE_MAX 16

struct omciutil_vlan_tlan_table_t
{
	unsigned char total;
	unsigned short vid[CLASSF_TLAN_TABLE_MAX];
};

struct omciutil_vlan_tlan_info_t
{
	struct omciutil_vlan_tlan_table_t tlan_table;
	struct list_head uni_tlan_list; //node is "struct omciutil_vlan_rule_fields_t"
	struct list_head uni_nontlan_list; //node is "struct omciutil_vlan_rule_fields_t"
	struct list_head ani_tlan_list; //node is "struct omciutil_vlan_rule_fields_t"
	struct list_head ani_nontlan_list; //node is "struct omciutil_vlan_rule_fields_t"
};

extern struct omciutil_vlan_cb_t omciutil_vlan_cb_g;

/* omciutil_vlan.c */
int omciutil_vlan_release_rule_list(struct list_head *rule_list);
int omciutil_vlan_get_rule_filter_tag_num(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_get_rule_treatment_tag_num(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_cb_register(struct omciutil_vlan_cb_t *omciutil_vlan_cb);
int omciutil_vlan_cb_unregister(void);
//int omciutil_vlan_check_171_dscp2pbit(struct me_t *me);
int omciutil_vlan_analyse_dscp2pbit_bitmap(char *dscp2pbit_bitmap, unsigned char *dscp2pbit_table, unsigned char *pbit_mask);
int omciutil_vlan_get_171_dscp2pbit_info(struct me_t *me, struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, unsigned char force_flag);

/* omciutil_vlan_collect.c */
int omciutil_vlan_collect_171_dscp2pbit_usage_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	unsigned char force_flag);
int omciutil_vlan_collect_130_dscp2pbit_usage_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info);
int  omciutil_vlan_collect_dscp2pbit_info_global(struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info);
int omciutil_vlan_collect_dscp2pbit_info_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info);
void omciutil_vlan_collect_rule_fields_from_table_entry(unsigned char *entry, unsigned short input_tpid, unsigned short output_tpid, struct omciutil_vlan_rule_fields_t *rule_fields);
void omciutil_vlan_collect_rule_fields_from_l2_table_entry(unsigned char *entry, unsigned short input_tpid, unsigned short output_tpid, struct omciutil_vlan_rule_fields_t *rule_fields);
void omciutil_vlan_collect_249_rules_fill_default_fields(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_pbit_dont_care_workaround(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_pbit_remap_workaround(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_pbit_dont_care_workaround_transparent(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_collect_rules_by_port(int action, struct me_t *issue_bridge_port_me, struct me_t *issue_me, struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode);
int omciutil_vlan_insert_rule_fields_to_list(struct omciutil_vlan_rule_fields_t *rule_fields, struct list_head *rule_list);
void omciutil_vlan_collect_tlan_vid(struct omciutil_vlan_tlan_table_t *tlan_table);

/* omciutil_vlan_compare.c */
int omciutil_vlan_compare_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields1, struct omciutil_vlan_rule_fields_t *rule_fields2);

/* omciutil_vlan_pbitmap.c */
int omciutil_vlan_pbitmap_ds_get(struct me_t *me_bport, char *pbitmap, unsigned char tag_mode);

/* omciutil_vlan_reverse.c */
int omciutil_vlan_generate_rule_result(struct omciutil_vlan_rule_fields_t *rule_fields, struct omciutil_vlan_rule_result_list_t *rule_result_list);
int omciutil_vlan_reverse_rule_generate(struct omciutil_vlan_rule_fields_t *rule_fields, struct omciutil_vlan_rule_fields_t *rule_fields_reverse);
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
int omciutil_vlan_generate_reverse_filter(struct omciutil_vlan_rule_fields_t *rule_fields, enum omciutil_vlan_tag_code_t code, struct omciutil_vlan_tag_fields_t *new_filter, omciutil_vlan_direct dir);
#else
int omciutil_vlan_generate_reverse_filter(struct omciutil_vlan_rule_fields_t *rule_fields, enum omciutil_vlan_tag_code_t code, struct omciutil_vlan_tag_fields_t *new_filter);
#endif
int omciutil_vlan_release_rule_result(struct omciutil_vlan_rule_result_list_t *rule_result_list);

/* omciutil_vlan_str.c */
char *omciutil_vlan_str_desc(struct omciutil_vlan_rule_fields_t *rule_fields, char *head_wrap_string, char *treat_wrap_string, unsigned char lan_mask_flag);
char *omciutil_vlan_str_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields);

/* omciutil_vlan_valid.c */
int omciutil_vlan_is_valid_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields);
int omciutil_vlan_is_valid_treat_priority(unsigned char priority, int rule_tag_num);
int omciutil_vlan_is_valid_treat_vid(unsigned short vid, int rule_tag_num);

#endif

