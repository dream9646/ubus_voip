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
 * File    : batchtab_cb_calix_uni_policing.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"
#include "tm.h"
#include "proprietary_calix.h"

#define UNI_POLICING_ACL_ORDER	27

//#define SHARE_SAME_RATE_METER	1
//#define ONLY_TAGGED_RULES	1
#define UNI_POLICING_ACL_MAX	32

struct uni_policing_acl_t {
	unsigned int portmask;
	unsigned short vlan;
	unsigned char pbit;
	unsigned short etype;
	unsigned int td;
	unsigned int meter_rate;
	unsigned int meter_bucket_size;
	unsigned int meter_ifg;
	int hw_meter_table_entry;
	int hw_acl_table_entry;
};

struct uni_policing_info_t {
	struct uni_policing_acl_t acl[UNI_POLICING_ACL_MAX];
	int acl_meter_id[UNI_POLICING_ACL_MAX];
};

static struct uni_policing_info_t uni_policing_info_g;

////////////////////////////////////////////////////

static int
vlan_to_td(unsigned int vid, unsigned char pbit, unsigned int *us_td, unsigned int *us_pir, unsigned int *us_pbs)
{
	// 171->84->47->(130)->266->268->280
	struct meinfo_t *miptr = meinfo_util_miptr(84);
	struct me_t *meptr_84 = NULL;
	int is_found = 0;
	
	// 171->84
	list_for_each_entry(meptr_84, &miptr->me_instance_list, instance_node) {
		unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
		if((meinfo_util_me_attr_data(meptr_84, 3) == 1) &&
		   (vid == util_bitmap_get_value(bitmap, 24*8, 4, 12))) {
			is_found = 1;
			//dbprintf_bat(LOG_ERR,"84=0x%04x\n",meptr_84->meid);
			break;
		}
	}
	if(is_found) {
		// 84->47
		struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meptr_84->meid);
		unsigned char tp_type = 0;
		if(meptr_47) {
			tp_type = meinfo_util_me_attr_data(meptr_47, 3);
			//dbprintf_bat(LOG_ERR,"47=0x%04x tp_type=%d\n",meptr_47->meid, tp_type);
		}
		if(meptr_47 && tp_type == 3) { // 802.1p
			// 47->130
			struct me_t *meptr_130 = meinfo_me_get_by_meid(130, meinfo_util_me_attr_data(meptr_47, 4));
			if(meptr_130) {
				//dbprintf_bat(LOG_ERR,"130=0x%04x\n",meptr_130->meid);
				// 130->266
				struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_130 , pbit+2));
				if(meptr_266) {
					// 266->268
					struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
					//dbprintf_bat(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
					if(meptr_268) {
						// 268->280
						struct me_t *td_us = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 5));
						//struct me_t *td_ds = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 9));
						//dbprintf_bat(LOG_ERR,"268=0x%04x\n",meptr_268->meid);
						if(td_us != NULL) {
							//dbprintf_bat(LOG_ERR,"us 280=0x%04x\n",td_us->meid);
							*us_td = td_us->meid;
							*us_pir = meinfo_util_me_attr_data(td_us, 2);
							*us_pbs = meinfo_util_me_attr_data(td_us, 4);
							//*ustd_meptr = td_us;
							if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
								// workaround for old (exa) olt which cannot assign pir issue
								if(proprietary_calix_get_olt_version()==0) { // EXA
									if(*us_pir == 0) {
										struct meinfo_t *miptr_268 = meinfo_util_miptr(268);
										struct me_t *gem_port = NULL;
										unsigned short tcont_ptr = (unsigned short) meinfo_util_me_attr_data(meptr_268, 2);
										list_for_each_entry(gem_port, &miptr_268->me_instance_list, instance_node) {
											unsigned short tcont = (unsigned short) meinfo_util_me_attr_data(gem_port, 2);
											struct me_t *iwtp = meinfo_me_get_by_meid(266, gem_port->meid);
											if(me_related(meptr_130, iwtp) && (tcont == tcont_ptr)) {
												struct me_t *new_td_us = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(gem_port, 5));
												if(*us_pir < meinfo_util_me_attr_data(new_td_us, 2)) {
													*us_td = new_td_us->meid;
													*us_pir = meinfo_util_me_attr_data(new_td_us, 2);
													*us_pbs = meinfo_util_me_attr_data(new_td_us, 4);
												}
											}
										}
									}
								}
							}
						}
						/*if(td_ds != NULL) {
							//dbprintf_bat(LOG_ERR,"ds 280=0x%04x\n",td_ds->meid);
							*ds_td = td_ds->meid;
							*ds_pir = meinfo_util_me_attr_data(td_ds, 2);
							*ds_pbs = meinfo_util_me_attr_data(td_ds, 4);
							*dstd_meptr = td_ds;
						}*/
					}
				}
			}
		} else if (meptr_47 && tp_type == 5) { // GEM IWTP
			// 47->266
			struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_47 , 4));
			if(meptr_266) {
				// 266->268
				struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
				//dbprintf_bat(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
				if(meptr_268) {
					// 268->280
					struct me_t *td_us = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 5));
					//struct me_t *td_ds = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 9));
					//dbprintf_bat(LOG_ERR,"268=0x%04x\n",meptr_268->meid);
					if(td_us != NULL) {
						//dbprintf_bat(LOG_ERR,"us 280=0x%04x\n",td_us->meid);
						*us_td = td_us->meid;
						*us_pir = meinfo_util_me_attr_data(td_us, 2);
						*us_pbs = meinfo_util_me_attr_data(td_us, 4);
						//*ustd_meptr = td_us;
					}
					/*if(td_ds != NULL) {
						//dbprintf_bat(LOG_ERR,"ds 280=0x%04x\n",td_ds->meid);
						*ds_td = td_ds->meid;
						*ds_pir = meinfo_util_me_attr_data(td_ds, 2);
						*ds_pbs = meinfo_util_me_attr_data(td_ds, 4);
						*dstd_meptr = td_ds;
					}*/
				}
			}
		}
	}
	return 0;
}

static int
add_port_rate_to_acl(struct uni_policing_acl_t acl_table[], unsigned char port, unsigned short vlan, unsigned char pbit, unsigned int etype, unsigned int td, unsigned int rate, unsigned int bucket_size)
{
	int i, p = -1;
	
	if (rate == 0)
		return -1;
	
	for (i=0; i<UNI_POLICING_ACL_MAX; i++) {
		// share the same meter
		if (acl_table[i].vlan == vlan && acl_table[i].pbit == pbit && acl_table[i].etype == etype && \
		acl_table[i].td == td && acl_table[i].meter_rate == rate && acl_table[i].portmask) {
			acl_table[i].portmask |= (1 << port);
			return i;
		}
		if(omci_env_g.uni_policing_aggregate_acl) {
			// share the acl
			if (acl_table[i].vlan == vlan && acl_table[i].pbit == 8 && acl_table[i].etype == etype && \
			acl_table[i].td == td && acl_table[i].meter_rate == rate && acl_table[i].portmask) {
				return i;
			}
			// remember the transparent pbit acl
			if (acl_table[i].vlan == vlan && acl_table[i].pbit == 8 && acl_table[i].etype == etype && \
			acl_table[i].portmask) {
				p = i;
			}
		}
		// since rule are added to acl table from 0..n
		// so if we encounter an entry with portmask 0, which means it is end of table
		if (acl_table[i].portmask == 0) {
			switch (etype) {
				case 1: // IPoE
					acl_table[i].etype = 0x0800;
					break;
				case 2: // PPPoE
					add_port_rate_to_acl(acl_table, port, vlan, pbit, 0x8863, td, rate, bucket_size);
					add_port_rate_to_acl(acl_table, port, vlan, pbit, 0x8864, td, rate, bucket_size);
					return 0;
				case 3: // ARP
					acl_table[i].etype = 0x0806;
					break;
				case 4: // IPv6
					acl_table[i].etype = 0x86DD;
					break;
				default:
					acl_table[i].etype = etype;
					break;
			}
			acl_table[i].portmask |= (1 << port);
			acl_table[i].vlan = vlan;
			acl_table[i].pbit = pbit;
			acl_table[i].td = td;
			acl_table[i].meter_rate = (rate>8) ? rate : 8;
			if (bucket_size == 0) bucket_size = omci_env_g.uni_policing_default_bs;
			else if (bucket_size > 0xFFFE) bucket_size = 0xFFFE;
			acl_table[i].meter_bucket_size = (bucket_size) ? bucket_size : METER_MAX_BURST_BYTES;
			acl_table[i].meter_ifg = 0;
			acl_table[i].hw_meter_table_entry = -1;
			acl_table[i].hw_acl_table_entry = -1;
			if(omci_env_g.uni_policing_aggregate_acl && p >= 0) {
				// swap priority of exact pbit acl and transparent pbit acl (put transparent rule to lower priority)
				struct uni_policing_acl_t tmp_acl_table;
				memcpy(&tmp_acl_table, &acl_table[i], sizeof(struct uni_policing_acl_t));
				memcpy(&acl_table[i], &acl_table[p], sizeof(struct uni_policing_acl_t));
				memcpy(&acl_table[p], &tmp_acl_table, sizeof(struct uni_policing_acl_t));
				return p;
			}
			return i;
		}
	}
	return -1;
}

static int
batchtab_cb_calix_uni_policing_init(void)
{
	int pos;
	memset(&uni_policing_info_g, 0, sizeof(struct uni_policing_info_t));
	for(pos=0; pos < UNI_POLICING_ACL_MAX; pos++) {
		uni_policing_info_g.acl[pos].hw_meter_table_entry = -1; // hw_meter_table_entry has to be -1 because 0 is a valid value
		uni_policing_info_g.acl[pos].hw_acl_table_entry = -1; // hw_acl_table_entry has to be -1 because 0 is a valid value
	}
	return 0;
}

static int
batchtab_cb_calix_uni_policing_finish(void)
{
	return 0;
}

static int
batchtab_cb_calix_uni_policing_gen(void **table_data)
{
	struct uni_policing_info_t *t = &uni_policing_info_g;
	struct meinfo_t *miptr_251=meinfo_util_miptr(251);
	struct me_t *me;
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
		return 0;
	
	if (table_data == NULL)
		return 0;
	
	// update acl table for portmask and rate/bucket
	list_for_each_entry(me, &miptr_251->me_instance_list, instance_node) {
		struct switch_port_info_t port_info;
		struct me_t *me_uni = meinfo_me_get_by_meid(11, me->meid);
		
		if (me_uni != NULL) {
			if (switch_get_port_info_by_me(me_uni, &port_info) != 0)	// no port_info for this pptp uni
				continue;
			
			if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
				unsigned int enable = (unsigned int) meinfo_util_me_attr_data(me, 15);
				unsigned int uni_policing_pbit_mask = (unsigned int) meinfo_util_me_attr_data(me, 16);
				struct attr_table_header_t *table_header = NULL;	
				struct attr_table_entry_t *table_entry_pos;
				struct me_t *meptr = NULL;
				struct meinfo_t *miptr = meinfo_util_miptr(171);
				if(!uni_policing_pbit_mask) uni_policing_pbit_mask = omci_env_g.uni_policing_pbit_mask;
				list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
					if (me_related(meptr, me_uni)) {
						if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr, 6)) == NULL)
							continue;
						list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
						{
							if (table_entry_pos->entry_data_ptr != NULL)
							{
								unsigned short filter_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 36, 13);
								unsigned char filter_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 32, 4);
								unsigned char filter_outer_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 0, 4);
								unsigned char filter_ethertype = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 60, 4);
								unsigned short treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 112, 13);
								unsigned char treatment_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 108, 4);
								unsigned int p, uni_policing_pbit_num = 0;
								for(p=0; p<8; p++) {
									if (((uni_policing_pbit_mask >> p) & 1) == 1)
										uni_policing_pbit_num++;
								}
								if(treatment_inner_pbit==8) { // transparent
									unsigned int i, td[8], rate[8], bs[8];
									memset(td, 0, sizeof(td));
									memset(rate, 0, sizeof(rate));
									memset(bs, 0, sizeof(bs));
									for(i=0; i<8; i++) {
										vlan_to_td(treatment_inner_vid, i, &td[i], &rate[i], &bs[i]);
										rate[i] = rate[i]*8/1000; // Byte/s -> kbps
										bs[i] = bs[i]*8/1000; // Byte/s -> kbps
									}
									if(filter_inner_pbit==8 && td[0]==td[1] && td[1]==td[2] && td[2]==td[3] && \
									td[3]==td[4] && td[4]==td[5] && td[5]==td[6] && td[6]==td[7]) {
										if(enable == 1 && uni_policing_pbit_mask)
											add_port_rate_to_acl(t->acl, port_info.logical_port_id, filter_inner_vid, filter_inner_pbit, filter_ethertype, td[0], rate[0], bs[0]);
									} else if(omci_env_g.uni_policing_aggregate_acl && uni_policing_pbit_num > 4) { // if used pbit is more than 4
										unsigned int i = 0, j = 0, td[8], rate[8], bs[8], max_rate = 0;
										memset(td, 0, sizeof(td));
										memset(rate, 0, sizeof(rate));
										memset(bs, 0, sizeof(bs));
										for(i=0; i<8; i++) {
											if (((uni_policing_pbit_mask >> i) & 1) == 0) // exclude pbit which doesn't need policing
												continue;
											vlan_to_td(treatment_inner_vid, i, &td[i], &rate[i], &bs[i]);
											rate[i] = rate[i]*8/1000; // Byte/s -> kbps
											bs[i] = bs[i]*8/1000; // Byte/s -> kbps
											if(max_rate < rate[i]) {
												max_rate= rate[i];
												j = i;
											}
										}
										if(enable == 1 && uni_policing_pbit_mask) {
											for(i=0; i<8; i++) {
												if (((uni_policing_pbit_mask >> i) & 1) == 0) // put rate limit free pbit to unlimited rate acl
													add_port_rate_to_acl(t->acl, port_info.logical_port_id, filter_inner_vid, i, filter_ethertype, MEINFO_INSTANCE_DEFAULT, CHIP_RATE_MAX, METER_MAX_BURST_BYTES);
											}
											add_port_rate_to_acl(t->acl, port_info.logical_port_id, filter_inner_vid, 8, filter_ethertype, td[j], rate[j], bs[j]);
										}
									} else {
										for(i=0; i<8; i++) {
											if((enable == 1) && ((uni_policing_pbit_mask >> i) & 1))
												add_port_rate_to_acl(t->acl, port_info.logical_port_id, filter_inner_vid, i, filter_ethertype, td[i], rate[i], bs[i]);
										}
									}
								} else { // translation
									unsigned int rate = 0, td = 0, bs = 0;
									unsigned char pbit;
									switch (treatment_inner_pbit) {
										case 8: // copy inner
										case 10: // dscp to pbit, assume filter_inner_pbit
										case 15: // do not add tag
											pbit = filter_inner_pbit;
											break;
										case 9: // copy outer
											pbit = filter_outer_pbit;
											break;
										default:
											pbit = treatment_inner_pbit;
											break;
									}
									vlan_to_td(treatment_inner_vid, pbit, &td, &rate, &bs);
									rate = rate*8/1000; // Byte/s -> kbps
									bs = bs*8/1000; // Byte/s -> kbps
#ifdef ONLY_TAGGED_RULES // only for tagged rules
									unsigned char untag = ((filter_inner_vid == 4096) && (filter_inner_pbit == 15)) ? 1 : 0;
									if((enable == 1) && ((uni_policing_pbit_mask >> pbit) & 1) && (untag == 0)) {
#else
									if((enable == 1) && ((uni_policing_pbit_mask >> pbit) & 1)) {
#endif
										add_port_rate_to_acl(t->acl, port_info.logical_port_id, filter_inner_vid, filter_inner_pbit, filter_ethertype, td, rate, bs);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	*table_data = t;
	return 0;
}

static int
batchtab_cb_calix_uni_policing_release(void *table_data)
{
	int pos;
	struct uni_policing_info_t *t = (struct uni_policing_info_t *)table_data;

	if(!table_data)
	{
		return 0;
	}

	for(pos=0; pos<UNI_POLICING_ACL_MAX; pos++) {
		if(t->acl[pos].portmask != 0) {
#ifndef X86
			int count;
			vacl_del_order(UNI_POLICING_ACL_ORDER, &count);
#endif
		}
		memset( &t->acl[pos], 0x0, sizeof(struct uni_policing_acl_t));
		t->acl[pos].hw_meter_table_entry = -1; // hw_meter_table_entry has to be -1 because 0 is a valid entry
		t->acl[pos].hw_acl_table_entry = -1; // hw_acl_table_entry has to be -1 because 0 is a valid entry
	}

	return 0;
}

static int
batchtab_cb_calix_uni_policing_dump(int fd, void *table_data)
{
	struct uni_policing_info_t *t = (struct uni_policing_info_t *)table_data;
	int pos, stpid = 0x88a8;
	struct meinfo_t *miptr_251=meinfo_util_miptr(251);
	struct me_t *me;

	if(t == NULL) 
		return 0;
#ifndef X86
//  edwin, TBD
//	rtk_svlan_tpidEntry_get(0, &stpid);
#endif
	list_for_each_entry(me, &miptr_251->me_instance_list, instance_node) {
		struct switch_port_info_t port_info;
		struct me_t *me_uni = meinfo_me_get_by_meid(11, me->meid);
		
		if (me_uni != NULL) {
			if (switch_get_port_info_by_me(me_uni, &port_info) != 0)	// no port_info for this pptp uni
				continue;
			
			if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
				unsigned int enable = (unsigned int) meinfo_util_me_attr_data(me, 15);
				unsigned int uni_policing_pbit_mask = (unsigned int) meinfo_util_me_attr_data(me, 16);
				util_fdprintf(fd, "port=%d[%d] enable=%d uni_policing_pbit_mask=0x%02x\n", me->instance_num, port_info.logical_port_id, enable, uni_policing_pbit_mask);
			}
		}
	}
	util_fdprintf(fd, "global uni_policing_pbit_mask=0x%02x\n", omci_env_g.uni_policing_pbit_mask);

	for(pos=0; pos<UNI_POLICING_ACL_MAX; pos++) {
		if(t->acl[pos].portmask==0) 
			continue;
		
		util_fdprintf(fd, "[%d]: portmask=0x%x %c-vlan=%d pbit=%d etype=0x%04x td=0x%x rate=%d bucket=%d hw_acl_table_entry=%d hw_meter_table_entry=%d\n", 
		pos, t->acl[pos].portmask, (stpid==0x8100)?'s':'c', t->acl[pos].vlan, t->acl[pos].pbit, t->acl[pos].etype, t->acl[pos].td, t->acl[pos].meter_rate, t->acl[pos].meter_bucket_size, t->acl[pos].hw_acl_table_entry,t->acl[pos].hw_meter_table_entry);
	}

	return 0;
}

static int
batchtab_cb_calix_uni_policing_hw_sync(void *table_data)
{
	int ret = 0;
#ifndef X86
	struct uni_policing_info_t *t = table_data;
	int pos;
	
	if (t == NULL)
		return ret;

	for(pos=0; pos<UNI_POLICING_ACL_MAX; pos++) {
		if (t->acl[pos].portmask == 0)
			continue;
		else
#ifndef RG_ENABLE
		{
			struct vacl_user_node_t acl_rule;
			int sub_order=0, count, i, stpid;

			memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
			vacl_user_node_init(&acl_rule);

			acl_rule.ingress_active_portmask = t->acl[pos].portmask;
// edwin, TBD
#if 1
			stpid = 0x8100;
#else
			rtk_svlan_tpidEntry_get(0, &stpid);
#endif
			if(stpid == 0x8100) {
				acl_rule.ingress_care_u.bit.stag_value = (t->acl[pos].vlan<4096 || t->acl[pos].pbit<8) ? 1 : 0;
				acl_rule.ingress_care_u.bit.stag_mask = (t->acl[pos].vlan<4096 || t->acl[pos].pbit<8) ? 1 : 0;
				acl_rule.ingress_stag_u.vid.value = (t->acl[pos].vlan<4096) ? t->acl[pos].vlan : 0;
				acl_rule.ingress_stag_u.vid.mask = (t->acl[pos].vlan<4096) ? 0xfff : 0;
				acl_rule.ingress_stag_pri_value = (t->acl[pos].pbit<8) ? t->acl[pos].pbit : 0;
				acl_rule.ingress_stag_pri_mask = (t->acl[pos].pbit<8) ? 0x7 : 0;
			} else {
				acl_rule.ingress_care_u.bit.ctag_value = (t->acl[pos].vlan<4096 || t->acl[pos].pbit<8) ? 1 : 0;
				acl_rule.ingress_care_u.bit.ctag_mask = (t->acl[pos].vlan<4096 || t->acl[pos].pbit<8) ? 1 : 0;
				acl_rule.ingress_ctag_u.vid.value = (t->acl[pos].vlan<4096) ? t->acl[pos].vlan : 0;
				acl_rule.ingress_ctag_u.vid.mask = (t->acl[pos].vlan<4096) ? 0xfff : 0;
				acl_rule.ingress_ctag_pri_value = (t->acl[pos].pbit<8) ? t->acl[pos].pbit : 0;
				acl_rule.ingress_ctag_pri_mask = (t->acl[pos].pbit<8) ? 0x7 : 0;
			}
			acl_rule.ingress_care_u.bit.ether_type_value = (t->acl[pos].etype!=0) ? 1 : 0;
			acl_rule.ingress_care_u.bit.ether_type_mask = (t->acl[pos].etype!=0) ? 1 : 0;
			acl_rule.ingress_ether_type_u.etype.value = (t->acl[pos].etype!=0) ? t->acl[pos].etype : 0;
			acl_rule.ingress_ether_type_u.etype.mask = (t->acl[pos].etype!=0) ? 0xffff : 0;
			acl_rule.act_meter_rate = t->acl[pos].meter_rate*omci_env_g.tm_gemus_rate_factor/100;
			acl_rule.act_meter_bucket_size = t->acl[pos].meter_bucket_size;
			acl_rule.act_meter_ifg = t->acl[pos].meter_ifg;
			for (i=0; i<pos; i++) {
				// share the same meter
				if (t->acl[i].td == t->acl[pos].td) {
					acl_rule.hw_meter_table_entry = t->acl[pos].hw_meter_table_entry = t->acl[i].hw_meter_table_entry;
					break;
				}
			}
			if (t->acl[pos].hw_meter_table_entry == -1) {
#ifdef SHARE_SAME_RATE_METER
				acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE; // share the same meter
#else
				acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_WO_SHARE; // not share the same meter
#endif
			}
			acl_rule.act_type |= VACL_ACT_LOG_POLICING_MASK;
			acl_rule.order = UNI_POLICING_ACL_ORDER;
			if(t->acl[pos].hw_acl_table_entry >= 0) {
				dbprintf_bat(LOG_ERR, "To vacl_del_order() for t->acl[%d].hw_acl_table_entry=(%d)\n", pos, t->acl[pos].hw_acl_table_entry);
				vacl_del_order(UNI_POLICING_ACL_ORDER, &count);
			}
			vacl_add(&acl_rule, &sub_order);
			switch_hw_g.vacl_commit(&ret);
			t->acl[pos].hw_acl_table_entry=sub_order;
			t->acl[pos].hw_meter_table_entry=acl_rule.hw_meter_table_entry;
			dbprintf_bat(LOG_ERR, "hw_acl_table_entry=%d hw_meter_table_entry=%d\n", sub_order, acl_rule.hw_meter_table_entry);
		}
#endif
	}
#endif
	return ret;
}

// public register function /////////////////////////////////////
int
batchtab_cb_calix_uni_policing_register(void)
{
	return batchtab_register("calix_uni_policing", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_calix_uni_policing_init,
		batchtab_cb_calix_uni_policing_finish,
		batchtab_cb_calix_uni_policing_gen,
		batchtab_cb_calix_uni_policing_release,
		batchtab_cb_calix_uni_policing_dump,
		batchtab_cb_calix_uni_policing_hw_sync);
}
