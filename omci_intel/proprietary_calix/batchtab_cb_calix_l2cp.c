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
 * File    : batchtab_cb_calix_l2cp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "switch.h"
#include "hwresource.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_calix.h"

unsigned long long calix_l2cp_g[SWITCH_LOGICAL_PORT_TOTAL] = {0};

////////////////////////////////////////////////////

static int
batchtab_cb_calix_l2cp_init(void)
{
	memset(calix_l2cp_g, 0, sizeof(unsigned long long)*SWITCH_LOGICAL_PORT_TOTAL);
	return 0;
}

static int
batchtab_cb_calix_l2cp_finish(void)
{
	memset(calix_l2cp_g, 0, sizeof(unsigned long long)*SWITCH_LOGICAL_PORT_TOTAL);
	return 0;
}

static int
batchtab_cb_calix_l2cp_gen(void **table_data)
{
	int portid = 0;
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
		return 0;
	
	for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		struct me_t *pptp_me = NULL;
		
		if (((1<<portid) & switch_get_uni_logical_portmask()) && ((pptp_me = meinfo_me_get_by_instance_num(11, portid)) != NULL)) {
			unsigned char bmi = 1, bpdu = 1, garp = 1, all_lans = 1;
			unsigned long long l2cp_mask = 0xffffffffffffffffULL;
			unsigned char found_uni = 0, found_ani = 0;
			struct meinfo_t *miptr45 = meinfo_util_miptr(45), *miptr251 = meinfo_util_miptr(251);
			struct me_t *meptr45 = NULL, *meptr251 = NULL;
			struct me_t *ipptp_me = hwresource_public2private(pptp_me);
			
			list_for_each_entry(meptr251, &miptr251->me_instance_list, instance_node) {
				if(me_related(meptr251, pptp_me)) {
					bpdu = (unsigned char) meinfo_util_me_attr_data(meptr251, 10);
					garp = (unsigned char) meinfo_util_me_attr_data(meptr251, 11);
					all_lans = (unsigned char) meinfo_util_me_attr_data(meptr251, 12);
					//dbprintf(LOG_ERR, "me251=0x%x bpdu=%d garp=%d all_lans=%d\n", meptr251->meid, bpdu,garp,all_lans);
					found_uni++;
					break;
				}
			}
			
			list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
				if(me_related(meptr45, meptr251)) {
					struct meinfo_t *miptr47 = meinfo_util_miptr(47);
					struct me_t *meptr47 = NULL;
					list_for_each_entry(meptr47, &miptr47->me_instance_list, instance_node) {
						unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(meptr47, 3);
						if(me_related(meptr47, meptr45) && (tp_type == 3 || tp_type == 5)) { // only check me49/me79 for ani mac bridge port
							struct me_t *me49 =  meinfo_me_get_by_meid(49, meptr47->meid);
							struct me_t *me79 =  meinfo_me_get_by_meid(79, meptr47->meid);
							struct attr_table_header_t *mac_filter_table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me49, 1);
							bmi &= (unsigned char) meinfo_util_me_attr_data(me79, 8) ? 0 : 1;
							if(bmi==0 && l2cp_mask==0xffffffffffffffffULL) l2cp_mask = 0; // initial l2cp_mask according to bmi (filter or forward)
							if(mac_filter_table_header) {
								struct attr_table_entry_t *table_entry_pos = NULL;
								list_for_each_entry(table_entry_pos, &mac_filter_table_header->entry_list, entry_node) {
									if (table_entry_pos->entry_data_ptr != NULL) {
										unsigned char add = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 8, 2);
										unsigned char dir = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 14, 1);
										unsigned char filter = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 15, 1);
										unsigned char i, mac_addr[6], l2cp_addr[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};
										unsigned long long mask = (bmi==1)?0xffffffffffffffffULL:0;
										for (i=0; i<6; i++) mac_addr[i] = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 16+i*8, 8);
										if(memcmp(l2cp_addr, mac_addr, 5) == 0) { // l2cp
											mask = ((add==1) & (filter==1) & (dir==0)) ? mask & ~(1ULL << mac_addr[5]) : mask | (1ULL << mac_addr[5]);
											l2cp_mask = (bmi==1) ? (l2cp_mask&mask) : (l2cp_mask|mask);
										}
									}
								}
							}
							//dbprintf(LOG_ERR, "me49=0x%x l2cp_mask=0x%0llx bmi=%d\n", me49->meid, l2cp_mask, bmi);
							found_ani++;
						}
					}
					break;
				}
			}
			
			if(found_uni && found_ani) {
				unsigned long long rma_forward_mask = omci_env_g.rma_forward_mask;
				
				// 1. Forward all: ME79 forward. ME49 empty. ME251 forward.
				// 2. Deny all: ME79 filter. ME49 empty. ME251 filter.
				// 3. Forward all, deny some: ME79 forward. ME49 has filter table. ME251 don't care.
				// 4. Deny all, forward some: ME79 filter. ME49 has forward table. ME251 don't care.
				
				if(l2cp_mask != 0xffffffffffffffffULL) { // me49 not empty case, do not check me251
					if(bmi) { // if me79 is forwarded, check whether me49 is filter table
						rma_forward_mask |= 0xffffffffffffffffULL;
						rma_forward_mask &= l2cp_mask;
					} else { // if me79 is filtered, check whether me49 is forward table
						rma_forward_mask &= ~0xffffffffffffffffULL;
						rma_forward_mask |= l2cp_mask;
					}
				} else { // me49 empty case, check me251
					rma_forward_mask = (bmi) ? rma_forward_mask | 0xffffffffffffffffULL : rma_forward_mask & ~0xffffffffffffffffULL;
					rma_forward_mask = (bpdu) ? rma_forward_mask | 0xffffULL : rma_forward_mask & ~0xffffULL;
					rma_forward_mask = (garp) ? rma_forward_mask | 0xffff00000000ULL : rma_forward_mask & ~0xffff00000000ULL;
					rma_forward_mask = (all_lans) ? rma_forward_mask | 0x10000ULL : rma_forward_mask & ~0x10000ULL;
				}
				//rma_forward_mask &= ~0x2ULL; // disable flow control bit by default (01:80:c2:00:00:01)
				if (ipptp_me) {
					unsigned char logical_port_id = meinfo_util_me_attr_data(ipptp_me, 4);
					calix_l2cp_g[logical_port_id] = rma_forward_mask;
					//dbprintf(LOG_ERR, "portid=%d logical_port_id=%d found_uni=%d found_ani=%d rma_forward_mask=0x%0llx\n", portid, logical_port_id, found_uni, found_ani, calix_l2cp_g[logical_port_id]);
				}
			}
		}
	}

	*table_data = &calix_l2cp_g;
	return 0;
}

static int
batchtab_cb_calix_l2cp_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_calix_l2cp_dump(int fd, void *table_data)
{
	int i;
	
	util_fdprintf(fd, "global(ds) rma_forward_mask=0x%0llx\n", omci_env_g.rma_forward_mask);
	for(i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ((1<<i) & switch_get_uni_logical_portmask())
			util_fdprintf(fd, "port %d(us) rma_forward_mask=0x%0llx\n", i, calix_l2cp_g[i]);
	}
	return 0;
}

static int
batchtab_cb_calix_l2cp_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_calix_l2cp_register(void)
{
	return batchtab_register("calix_l2cp", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_calix_l2cp_init,
		batchtab_cb_calix_l2cp_finish,
		batchtab_cb_calix_l2cp_gen,
		batchtab_cb_calix_l2cp_release,
		batchtab_cb_calix_l2cp_dump,
		batchtab_cb_calix_l2cp_hw_sync);
}
