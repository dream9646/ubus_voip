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
 * Module  : proprietary_calix
 * File    : batchtab_cb_calix_apas.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>

#include "meinfo.h"
#include "util.h"
#include "util_run.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"
#include "proprietary_calix.h"

int calix_apas_enable_g = 0;

////////////////////////////////////////////////////

static int
batchtab_cb_calix_apas_init(void)
{
	calix_apas_enable_g = 0;
	return 0;
}

static int
batchtab_cb_calix_apas_finish(void)
{
	calix_apas_enable_g = 0;
	return 0;
}

static int
batchtab_cb_calix_apas_gen(void **table_data)
{
	int portid = 0, fb_mask = 0;
	int calix_apas_enable_new = 0;
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
		return 0;
	
	// Check whether APAS function is enabled:
	// 1. When ONT detected all wired ports are moved into FB partition
	// 2. Video service is provisioned
	// 3. MAC OUI rule is provisioned
	for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if ((1<<portid) & switch_get_uni_logical_portmask()) {
			struct me_t *pptp_me = NULL;
			extern struct me_t *get_pptp_uni_by_logical_portid(int);
			if ((pptp_me = get_pptp_uni_by_logical_portid(portid)) != NULL) {
				struct me_t *unig_me =  meinfo_me_get_by_meid(264, pptp_me->meid);
				unsigned short veip_meid = (unsigned short)meinfo_util_me_attr_data(unig_me, 4);
				if (veip_meid == 0x301) {
					// veip 0x301 is in full-bridge
					fb_mask |= (1<<portid);
				}
			}
		}
	}
	
	if (fb_mask == switch_get_uni_logical_portmask()) {
		struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
		struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
		if (!list_empty(&miptr_309->me_instance_list) && 
		    !list_empty(&miptr_310->me_instance_list)) {
			struct meinfo_t *miptr_249 = meinfo_util_miptr(249);
			struct me_t *meptr_249 = NULL;
			list_for_each_entry(meptr_249, &miptr_249->me_instance_list, instance_node) {
				struct attr_table_header_t *tab_header = meinfo_util_me_attr_ptr(meptr_249, 5);
				if(!list_empty(&tab_header->entry_list)) {
					calix_apas_enable_new = 1;
					break;
				}
			}
		}
	}

	if (calix_apas_enable_g != calix_apas_enable_new) {
		calix_apas_enable_g = calix_apas_enable_new;	
		// disable UNI egress shaper in APAS mode is checked and implemented in tm/td.c
		batchtab_crosstab_update("td");
		batchtab_crosstab_update("isolation_set");
	}
	
	// clear protocol classify at first
	util_run_by_system("echo proto clear > /proc/ifsw");
	if(calix_apas_enable_g) {
		struct meinfo_t *miptr = meinfo_util_miptr(249);
		struct me_t *meptr = NULL;
		list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
			struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(meptr, 5);
			struct attr_table_entry_t *entry = NULL;
			list_for_each_entry(entry, &tab_header->entry_list, entry_node) {
				unsigned char mac[IFHWADDRLEN], mask[IFHWADDRLEN], cmd[128];
				unsigned char MAYBE_UNUSED pbit;
				unsigned int vlan, i;
				for(i=0; i<IFHWADDRLEN; i++) {
					mac[i] = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 64+8*i, 8);
					mask[i] = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 16+8*i, 8);
				}
				vlan = (unsigned int) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 115, 13);
				pbit = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 144, 4);
				// protocol classify by mac
				sprintf(cmd, "echo proto %s 2 %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x > /proc/ifsw", 
					"add", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mask[0], mask[1], mask[2], mask[3], mask[4], mask[5]);
				util_run_by_system(cmd);
				// protocol classify by vlan
				sprintf(cmd, "echo proto %s 4 %d 0xfff > /proc/ifsw", 
					"add", vlan);
				util_run_by_system(cmd);
				/*// protocol classify by pbit
				sprintf(cmd, "echo proto %s 6 %d 0x7 > /proc/ifsw", 
					"add", pbit);
				util_run_by_system(cmd);
				// protocol classify by tci
				sprintf(cmd, "echo proto %s 10 %d 0xffff > /proc/ifsw", 
					"add", (pbit<<13|vlan));
				util_run_by_system(cmd);*/
			}
		}
		miptr = meinfo_util_miptr(309);
		list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
			struct attr_table_header_t *dyn_acl_tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(meptr, 7);
			struct attr_table_header_t *sta_acl_tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(meptr, 8);
			struct attr_table_entry_t *entry = NULL;
			list_for_each_entry(entry, &dyn_acl_tab_header->entry_list, entry_node) {
				unsigned char cmd[128];
				unsigned char MAYBE_UNUSED pbit;
				unsigned int tci, vlan;
				tci = (unsigned int) util_bitmap_get_value(entry->entry_data_ptr, 24*8, 32, 16);
				vlan = tci & 0xfff;
				pbit = (tci >> 13) & 0x7;
				// protocol classify by vlan
				sprintf(cmd, "echo proto %s 4 %d 0xfff > /proc/ifsw", 
					"add", vlan);
				util_run_by_system(cmd);
				/*// protocol classify by pbit
				sprintf(cmd, "echo proto %s 6 %d 0x7 > /proc/ifsw", 
					"add", pbit);
				util_run_by_system(cmd);
				// protocol classify by tci
				sprintf(cmd, "echo proto %s 10 %d 0xffff > /proc/ifsw", 
					"add", tci);
				util_run_by_system(cmd);*/
			}
			list_for_each_entry(entry, &sta_acl_tab_header->entry_list, entry_node) {
				unsigned char cmd[128];
				unsigned char MAYBE_UNUSED pbit;
				unsigned int tci, vlan;
				tci = (unsigned int) util_bitmap_get_value(entry->entry_data_ptr, 24*8, 32, 16);
				vlan = tci & 0xfff;
				pbit = (tci >> 13) & 0x7;
				// protocol classify by vlan
				sprintf(cmd, "echo proto %s 4 %d 0xfff > /proc/ifsw", 
					"add", vlan);
				util_run_by_system(cmd);
				/*// protocol classify by pbit
				sprintf(cmd, "echo proto %s 6 %d 0x7 > /proc/ifsw", 
					"add", pbit);
				util_run_by_system(cmd);
				// protocol classify by tci
				sprintf(cmd, "echo proto %s 10 %d 0xffff > /proc/ifsw", 
					"add", tci);
				util_run_by_system(cmd);*/
			}
		}
		util_run_by_system("echo proto_unmatch_action 2 > /proc/ifsw");
	} else {
		util_run_by_system("echo proto_unmatch_action 3 > /proc/ifsw");
	}
	
	*table_data = &calix_apas_enable_g;
	return 0;
}

static int
batchtab_cb_calix_apas_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_calix_apas_dump(int fd, void *table_data)
{
	int *enable = table_data;
	
	util_fdprintf(fd, "[calix_apas] enable=%d\n", (table_data) ? *enable : 0);
	return 0;
}

static int
batchtab_cb_calix_apas_hw_sync(void *table_data)
{
	// the 'calix_apas' hw sync will change apas_enable flag which are referred by batchtab 'classf'
	batchtab_crosstab_update("classf");
	
	return 0;
}

// exported function
int
batchtab_cb_calix_apas_is_enabled(void)
{
	return calix_apas_enable_g;
}

// public register function /////////////////////////////////////
int
batchtab_cb_calix_apas_register(void)
{
	return batchtab_register("calix_apas", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_calix_apas_init,
		batchtab_cb_calix_apas_finish,
		batchtab_cb_calix_apas_gen,
		batchtab_cb_calix_apas_release,
		batchtab_cb_calix_apas_dump,
		batchtab_cb_calix_apas_hw_sync);
}
