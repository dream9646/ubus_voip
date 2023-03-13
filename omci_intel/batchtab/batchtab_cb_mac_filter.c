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
 * Module  : batchtab
 * File    : batchtab_cb_mac_filter.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include "util.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "me_related.h"
#include "switch.h"
#include "hwresource.h"

#define	MAC_FILTER_ACL_ORDER	29


static struct batchtab_cb_mac_filter_t batchtab_cb_mac_filter_g;

static int
mac_filter_table_add(int pos, unsigned int port_id, struct attr_table_header_t *table_header)
{
	struct batchtab_cb_mac_filter_t *t = &batchtab_cb_mac_filter_g;
	struct attr_table_entry_t *table_entry_pos = NULL;

	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
		if (table_entry_pos->entry_data_ptr != NULL) {
			unsigned char add = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 8, 2);
			unsigned char dir = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 14, 1);
			unsigned char filter = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 15, 1);
			if (add==1 && filter==1) { // add&filtering
				int i;
				unsigned char mac[IFHWADDRLEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				for (i=0; i<6; i++) mac[i] = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 16+i*8, 8);
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
					unsigned char rma_prefix[5] = {0x01, 0x80, 0xC2, 0x00, 0x00};
					if((memcmp(mac, rma_prefix, 5)==0) && (mac[5]<=0x3F)) {
						// l2cp mac will be processed by sw path calix_l2cp batchtab, skip to add into hw acl
						dbprintf_bat(LOG_ERR, "l2cp mac (%02x), skip to add into hw acl\n", mac[5]);
						continue;
					}
				}
				if(pos >= VACL_NUM_MAX) {
					dbprintf_bat(LOG_ERR, "numbers of MAC filter > VACL_NUM_MAX=%d\n", VACL_NUM_MAX);
					continue;
				}
				t->collect_mac_filter[pos].is_used=1;
				t->collect_mac_filter[pos].port_id=port_id;
				t->collect_mac_filter[pos].dir=dir;
				memcpy(t->collect_mac_filter[pos].mac_addr, mac, sizeof(mac));
				pos++;
			}
		}
	}
	
	return pos;
}

static int
batchtab_cb_mac_filter_init(void)
{
	int pos;
	memset(&batchtab_cb_mac_filter_g, 0, sizeof(batchtab_cb_mac_filter_g));
	for(pos=0; pos < VACL_NUM_MAX; pos++) {
		batchtab_cb_mac_filter_g.collect_mac_filter[pos].hw_rule_entry = -1; // hw_rule_entry has to be -1 because 0 is a v
	}
	return 0;
}

static int
batchtab_cb_mac_filter_finish(void)
{
	return 0;
}

static int
batchtab_cb_mac_filter_gen(void **table_data)
{
	struct batchtab_cb_mac_filter_t *t = &batchtab_cb_mac_filter_g;

	struct meinfo_t *miptr49=meinfo_util_miptr(49);
	struct me_t *meptr49;
	int pos = 0;

	if (miptr49==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=49\n", MEINFO_ERROR_CLASSID_UNDEF);
		return MEINFO_ERROR_CLASSID_UNDEF;  //class_id out of range
	}

	list_for_each_entry(meptr49, &miptr49->me_instance_list, instance_node) {
		struct me_t *meptr47 = meinfo_me_get_by_meid(47, meptr49->meid);
		unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(meptr47, 3);
		if (tp_type == 3 || tp_type == 5 || tp_type == 6) { // wan -> lan
			struct meinfo_t *miptr45 = meinfo_util_miptr(45);
			struct me_t *meptr45 = NULL;
			list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
				if(me_related(meptr47, meptr45)) {
					struct meinfo_t *miptr47 = meinfo_util_miptr(47);
					struct me_t *meptr = NULL;
					list_for_each_entry(meptr, &miptr47->me_instance_list, instance_node) {
						tp_type = (unsigned char)meinfo_util_me_attr_data(meptr, 3);
						if(me_related(meptr, meptr45) && (tp_type == 1)) {
							struct me_t *ibridgeport_me = hwresource_public2private(meptr);
							struct attr_table_header_t *table_header = NULL;
							unsigned int port_id = 0;
							
							if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
								port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
							} else {
								dbprintf_bat(LOG_ERR, "bridge port is null\n");
								return -1;
							}
							
							if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr49, 1)) == NULL) {
								dbprintf_bat(LOG_ERR, "mac filter table is empty\n");
								return -1;
							}
							pos = mac_filter_table_add(pos, port_id, table_header);
						}
					}
					break;
				}
			}
		} else { // lan -> wan
			struct attr_table_header_t *table_header = NULL;
			unsigned int port_id = switch_get_wan_logical_portid();
			if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr49, 1)) == NULL) {
				dbprintf_bat(LOG_ERR, "mac filter table is empty\n");
				return -1;
			}
			pos = mac_filter_table_add(pos, port_id, table_header);
		}
	}

	*table_data = t;
	return 0;
}

static int
batchtab_cb_mac_filter_release(void *table_data)
{
	int pos, ret=0;
	struct batchtab_cb_mac_filter_t *t = (struct batchtab_cb_mac_filter_t *)table_data;

	if(!table_data)
	{
		return 0;
	}

	for(pos=0; pos < VACL_NUM_MAX; pos++) {
		if(t->collect_mac_filter[pos].is_used != 0) {
			int count;
			vacl_del_order(MAC_FILTER_ACL_ORDER, &count);
		}
		memset( &t->collect_mac_filter[pos], 0x0, sizeof(struct collect_mac_filter_t));
		t->collect_mac_filter[pos].hw_rule_entry = -1; // hw_rule_entry has to be -1 because 0 is a valid entry
	}
	switch_hw_g.vacl_commit(&ret);

	return ret;
}

static int
batchtab_cb_mac_filter_dump(int fd, void *table_data)
{
	struct batchtab_cb_mac_filter_t *t = (struct batchtab_cb_mac_filter_t *)table_data;
	int pos;

	if(t == NULL) 
		return 0;

	util_fdprintf(fd, "\n");

	for(pos=0; pos < VACL_NUM_MAX; pos++) {

		if(t->collect_mac_filter[pos].is_used==0) 
			continue;

		util_fdprintf(fd, "MAC=%02x:%02x:%02x:%02x:%02x:%02x port=%d dir=%d hw_rule_entry=%d\n", 
		t->collect_mac_filter[pos].mac_addr[0], t->collect_mac_filter[pos].mac_addr[1], 
		t->collect_mac_filter[pos].mac_addr[2], t->collect_mac_filter[pos].mac_addr[3], 
		t->collect_mac_filter[pos].mac_addr[4], t->collect_mac_filter[pos].mac_addr[5], 
		t->collect_mac_filter[pos].port_id, t->collect_mac_filter[pos].dir,
		t->collect_mac_filter[pos].hw_rule_entry);
	}

	return 0;
}

static int
batchtab_cb_mac_filter_hw_sync(void *table_data)
{
	struct batchtab_cb_mac_filter_t *t = (struct batchtab_cb_mac_filter_t *)table_data;
	int pos, ret=0;

	if(t == NULL) {
		return 0;
	}

	for(pos=0; pos < VACL_NUM_MAX; pos++) {
		if(t->collect_mac_filter[pos].is_used==0) 
			continue;

		{
			struct vacl_user_node_t acl_rule;
			int i, sub_order=0, count; 

			memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
			vacl_user_node_init(&acl_rule);

			if((t->collect_mac_filter[pos].dir) == 0) {
				for(i=0; i<6; i++) {
					acl_rule.ingress_dstmac_value[i] = t->collect_mac_filter[pos].mac_addr[i];
					acl_rule.ingress_dstmac_mask[i] = 0xFF;
				}
				acl_rule.ingress_care_u.bit.dstmac_value = 1;
				acl_rule.ingress_care_u.bit.dstmac_mask = 1;
			} else {
				for(i=0; i<6; i++) {
					acl_rule.ingress_srcmac_value[i] = t->collect_mac_filter[pos].mac_addr[i];
					acl_rule.ingress_srcmac_mask[i] = 0xFF;
				}
				acl_rule.ingress_care_u.bit.srcmac_value = 1;
				acl_rule.ingress_care_u.bit.srcmac_mask = 1;
			}

			acl_rule.act_type |= VACL_ACT_FWD_DROP_MASK;
			acl_rule.ingress_active_portmask = 1 << t->collect_mac_filter[pos].port_id;
			acl_rule.order = MAC_FILTER_ACL_ORDER;
			if(t->collect_mac_filter[pos].hw_rule_entry >= 0) {
				dbprintf_bat(LOG_ERR, "To vacl_del_order() for t->collect_mac_filter[%d].hw_rule_entry=(%d)\n", pos, t->collect_mac_filter[pos].hw_rule_entry);
				vacl_del_order(MAC_FILTER_ACL_ORDER, &count);
			}
			vacl_add(&acl_rule, &sub_order);
			switch_hw_g.vacl_commit(&ret);
			t->collect_mac_filter[pos].hw_rule_entry=sub_order;
			dbprintf_bat(LOG_ERR, "hw_rule_entry=%d\n", sub_order);
		}
	}

	return ret;
}

// public register function /////////////////////////////////////
int
batchtab_cb_mac_filter_register(void)
{
	return batchtab_register("mac_filter", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_mac_filter_init,
			batchtab_cb_mac_filter_finish,
			batchtab_cb_mac_filter_gen,
			batchtab_cb_mac_filter_release,
			batchtab_cb_mac_filter_dump,
			batchtab_cb_mac_filter_hw_sync);
}

