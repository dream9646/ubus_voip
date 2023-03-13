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
 * File    : omciutil_misc.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>

#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "hwresource.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "omciutil_vlan.h"
#include "iphost.h"
#include "wanif.h"

//cpu or iphost port
int
omciutil_misc_determine_port_interface(struct me_t *port_me, char *ifname)
{
	struct attr_value_t attr_value1, attr_value2;
	struct meinfo_t *miptr_ip_host; 
	struct me_t *me;
	char *devname;

	if (port_me == NULL || ifname == NULL)
	{
		dbprintf(LOG_ERR, "params error!!\n");
		return -1;
	}

	//check association tp type
	meinfo_me_attr_get(port_me, 3, &attr_value1);
	meinfo_me_attr_get(port_me, 4, &attr_value2);

	if ((miptr_ip_host = meinfo_util_miptr(134)) == NULL)
	{
		dbprintf(LOG_ERR, "get miptr 134 error!!\n");
		return -1;
	}

	switch (attr_value1.data)
	{
	case 4:
		//ip host
		if ((me = meinfo_me_get_by_meid(134, attr_value2.data)) == NULL)
		{
			dbprintf(LOG_ERR, "get me error, classid=134, meid=%u\n", attr_value2);
			return -1;
		} else {
			//get the interface name from me devname
			if ((devname = meinfo_util_get_config_devname(me)) != NULL) {
				strcpy((char *)ifname, devname);
				return 0;
			}
		}
		break;
	case 11:
		//veip
		if ((me = meinfo_me_get_by_meid(329, attr_value2.data)) == NULL)
		{
			dbprintf(LOG_ERR, "get me error, classid=329, meid=%u\n", attr_value2);
			return -1;
		} else {
			//get tcp/udp pointer me
			meinfo_me_attr_get(me, 4, &attr_value2);
			if ((me = meinfo_me_get_by_meid(136, attr_value2.data)) != NULL) {
				//get ip host pointer me
				meinfo_me_attr_get(me, 4, &attr_value2);
				if ((me = meinfo_me_get_by_meid(134, attr_value2.data)) == NULL) {
					dbprintf(LOG_ERR, "get me error, classid=134, meid=%u\n", attr_value2);
					return -1;
				}
				//get the interface name from me devname
				if ((devname = meinfo_util_get_config_devname(me)) != NULL) {
					strcpy((char *)ifname, devname);
					return 0;
				}
			}
		}
		break;
	}

	//not found ifname
	return -1;
}

// find all ts belong to same tcont as ts_me
int
omciutil_misc_get_ts_neighbor_list(struct me_t *ts_me, struct me_t *ts_neighbor_list[], int list_size, int include_myself)
{
	struct meinfo_t *miptr=meinfo_util_miptr(278);
	unsigned short parent_tcont_meid=meinfo_util_me_attr_data(ts_me,1);
	unsigned short parent_ts_meid=meinfo_util_me_attr_data(ts_me,2);
	struct me_t *me;
	int i;	

	for (i=0; i<list_size; i++)
		ts_neighbor_list[i]=NULL;
	i=0;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (!include_myself && me == ts_me)
			continue;
		if ((parent_tcont_meid && meinfo_util_me_attr_data(me, 1) == parent_tcont_meid) ||
		    (parent_ts_meid && meinfo_util_me_attr_data(me, 2) == parent_ts_meid) ) {
			ts_neighbor_list[i]= me;
			i++;
			if (i==list_size)
				break;
		}
	}
	return i;
}

int
omciutil_misc_locate_ts_neighbor_by_pq_priority(int pq_priority, struct me_t *ts_me, struct me_t **ts_neighbor)
{
	struct me_t *ts_neighbor_list[16];
	int ts_total = omciutil_misc_get_ts_neighbor_list(ts_me, ts_neighbor_list, 16, 1);
	int i;

	if (pq_priority>7)		// cast omci priority 0(high)..4095(low) to 0..7
		pq_priority=7;	

	for (i=0; i<ts_total; i++) {
		unsigned char priority = meinfo_util_me_attr_data(ts_neighbor_list[i], 4);
		if (priority >7)
			priority = 7;
		if (pq_priority == priority) {
			*ts_neighbor = ts_neighbor_list[i];
			return 1;
		}
	}
	return 0;
}

int
omciutil_misc_get_pbit2gemport_map_by_bport_me(struct me_t *bridge_port_me, int pbit2gemport_map[8])
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(bridge_port_me, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(bridge_port_me, 4);
	struct me_t *me=meinfo_me_get_by_meid(related_classid, related_pointer);
	int i;
	
	for (i=0; i<8; i++)
		pbit2gemport_map[i] = -1;

 	if (me == NULL)
		return -1;	// no related iwtp/mapper

	if (related_classid == 130) {	// mapper
		int found = 0;
		for (i=0; i<8; i++) {
			unsigned short meid_iwtp=meinfo_util_me_attr_data(me, i+2); // iwtp meid is in mapper attr 2..9
			struct me_t *me_iwtp=meinfo_me_get_by_meid(266, meid_iwtp);
			if (me_iwtp) {
		 		unsigned short meid_gem=meinfo_util_me_attr_data(me_iwtp, 1); // gem ctp meid is in iwtp attr 1
				struct me_t *me_gem=meinfo_me_get_by_meid(268, meid_gem);
				if (me_gem) {
					unsigned short gemort_value=meinfo_util_me_attr_data(me_gem, 1); // gem port value is in gem ctp attr 1
					pbit2gemport_map[i] = gemort_value;
					found++;
				}	
			}
		}
		return found;
		
	} else if (related_classid == 266) {	// iwtp
		unsigned short meid_gem=meinfo_util_me_attr_data(me, 1); // gem ctp meid is in iwtp attr 1
		struct me_t *me_gem=meinfo_me_get_by_meid(268, meid_gem);
		if (me_gem) {
			unsigned short gemort_value=meinfo_util_me_attr_data(me_gem, 1); // gem port value is in gem ctp attr 1
			for (i=0; i < 8; i++) 
				pbit2gemport_map[i] = gemort_value;
			return 8;
		}
		return -1;
		
	} else if (related_classid == 281) {    // mcast iwtp
		// ignore as this is for downstream only	
	}
	return -1;
}

int
omciutil_misc_get_gem_me_list_by_bport_me(struct me_t *bridge_port_me, struct me_t *gemport_list[8])
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(bridge_port_me, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(bridge_port_me, 4);
	struct me_t *me=meinfo_me_get_by_meid(related_classid, related_pointer);
	int i;
	
	for (i=0; i<8; i++)
		gemport_list[i] = NULL;

 	if (me == NULL)
		return -1;	// no related iwtp/mapper

	if (related_classid == 130) {	// mapper
		int found = 0;
		for (i=0; i<8; i++) {
			unsigned short meid_iwtp=meinfo_util_me_attr_data(me, i+2); // iwtp meid is in mapper attr 2..9
			struct me_t *me_iwtp=meinfo_me_get_by_meid(266, meid_iwtp);	//9.2.4 GEM interworking termination point
			if (me_iwtp) {
		 		unsigned short meid_gem=meinfo_util_me_attr_data(me_iwtp, 1); // gem ctp meid is in iwtp attr 1
				struct me_t *me_gem=meinfo_me_get_by_meid(268, meid_gem);	//9.2.3 GEM port network CTP
				if (me_gem) {
					gemport_list[found] = me_gem;
					found++;
				}
			}
		}
		return found;
		
	// 266: 9.2.4 GEM interworking termination point, 281: 9.2.5 Multicast GEM interworking termination point
	} else if (related_classid == 266 || related_classid == 281) {
		unsigned short meid_gem=meinfo_util_me_attr_data(me, 1); // gem ctp meid is in iwtp attr 1
		struct me_t *me_gem=meinfo_me_get_by_meid(268, meid_gem);	//9.2.3 GEM port network CTP
		if (me_gem) {
			gemport_list[0] = me_gem;
			return 1;
		}
		return -1;
	}
	return -1;
}

int
omciutil_misc_get_bport_me_list_by_gemport(int gemport, struct me_t *bport_me_list[8])
{
	struct meinfo_t *miptr_gem=meinfo_util_miptr(268);
	struct meinfo_t *miptr_iwtp=meinfo_util_miptr(266);	// iwtp
	struct meinfo_t *miptr_mcast_iwtp=meinfo_util_miptr(281);	// mcast iwtp
	struct meinfo_t *miptr_mapper=meinfo_util_miptr(130);	// mapper
	struct meinfo_t *miptr_bport=meinfo_util_miptr(47);	// bport
	struct me_t *me;
	struct me_t *me_gem=NULL;
	struct me_t *me_iwtp=NULL;
	struct me_t *me_mapper;

	int i, bport_found=0;
	
	for (i=0; i<8; i++)
		bport_me_list[i] = NULL;

	// locate me_gem with matched gemport value
	list_for_each_entry(me, &miptr_gem->me_instance_list, instance_node) {
		unsigned short gemport_value=meinfo_util_me_attr_data(me, 1); // gem port value is in gem ctp attr 1
		if (gemport_value == gemport) {
			me_gem = me;
			break;
		}
	}
	if (me_gem == NULL)
		return -1;

	// locate iwtp of specific me_gem				
	list_for_each_entry(me, &miptr_iwtp->me_instance_list, instance_node) {
		if (me_related(me, me_gem)) {
			me_iwtp =me;
			break;
		}
	}
	if (me_iwtp == NULL) {	// if no iwtp, then try mcast iwtp
		list_for_each_entry(me, &miptr_mcast_iwtp->me_instance_list, instance_node) {
			if (me_related(me, me_gem)) {
				me_iwtp = me;
				break;
			}
		}
	}
	if (me_iwtp ==NULL)
		return -2;

	// locate any mapper that owns the specific iwtp
	list_for_each_entry(me_mapper, &miptr_mapper->me_instance_list, instance_node) {
		if (me_related(me_mapper, me_iwtp)) {
			// then locate bridg port owns the found mapper			
			list_for_each_entry(me, &miptr_bport->me_instance_list, instance_node) {
				if (me_related(me, me_mapper)) {
					bport_me_list[bport_found] = me;
					bport_found++;
				}
			}
		}
	}

	// locate bridg port owns the specific iwtp			
       	list_for_each_entry(me, &miptr_bport->me_instance_list, instance_node) {
		if (me_related(me, me_iwtp)) {
			bport_me_list[bport_found] = me;
			bport_found++;
		}
	}

	return bport_found;
}

int
omciutil_misc_traffic_management_option_is_rate_control(void)
{
	struct me_t *me_ont_g = meinfo_me_get_by_meid(256, 0);
	struct attr_value_t attr_management_opt={0};
	
	meinfo_me_attr_get(me_ont_g, 4, &attr_management_opt);	// traffic management option				
	return (attr_management_opt.data == 1);	// 0:prio mode, 1:rate_ctrl mode, 2:prio & rate_ctrl mode
}

int
omciutil_get_pq_pointer_for_gem_ctp(struct me_t *gem_me, int is_ratectrl_mode)
{
	struct attr_value_t attr_pq_pointer={0};

	if (is_ratectrl_mode) {
		struct me_t *igem_me=hwresource_public2private(gem_me);
		if (igem_me)
			meinfo_me_attr_get(igem_me, 5, &attr_pq_pointer);
		else
			return 0xffff;
	} else {
		meinfo_me_attr_get(gem_me, 4, &attr_pq_pointer);
	}
	return attr_pq_pointer.data;
}

struct pq_usage_t {
	char is_same_tcont;
	unsigned short meid;
	unsigned short gem_child_counter;
};

// find the most idle pq for specific gem ctp
// most idle means the pq with least number of gem related
int 
omciutil_misc_most_idle_pq_under_same_tcont(struct me_t *me, unsigned short tcont_meid)
{
	struct meinfo_t *miptr_gem = meinfo_util_miptr(268);
	struct meinfo_t *miptr_pq = meinfo_util_miptr(277);
	struct me_t *me_gem, *me_pq, *me_tcont=meinfo_me_get_by_meid(262, tcont_meid);
	int is_rate_ctrl_mode = omciutil_misc_traffic_management_option_is_rate_control();
	struct pq_usage_t pq_usage[512];
	int i,found_instance_num = -1, found_counter = 9999;

	if (me_tcont == NULL)
		return -1;
		
        // init usage with 0
        memset(pq_usage, 0, sizeof(pq_usage));

        // find all pq related to the same tcont
	list_for_each_entry(me_pq, &miptr_pq->me_instance_list, instance_node) {
		pq_usage[me_pq->instance_num].meid = me_pq->meid;
		if (me_related(me_pq, me_tcont))
			pq_usage[me_pq->instance_num].is_same_tcont = 1;
	}

	// collect gem child couter
	list_for_each_entry(me_gem, &miptr_gem->me_instance_list, instance_node) {	
		if (me_gem->meid != me->meid) {	// ignore the gem itself
			unsigned short pq_meid = omciutil_get_pq_pointer_for_gem_ctp(me_gem, is_rate_ctrl_mode);
			if ((me_pq = meinfo_me_get_by_meid(277, pq_meid)) != NULL)
				pq_usage[me_pq->instance_num].gem_child_counter++;
		}
	}
	
	// return pq with least number of gem child
	for (i=0; i<512; i++) {
		if (pq_usage[i].is_same_tcont &&
		    pq_usage[i].gem_child_counter < found_counter ) {
			found_instance_num = i;
			found_counter = pq_usage[i].gem_child_counter;
		}
	}
	if (found_instance_num >= 0)
		return pq_usage[found_instance_num].meid;
	return -1;
}

// when in rate ctrl mode, gem has no pointer to pq, 
// the gem-pq relation is establish through pq pointer in igem me
// we reassign its value whenever 
// a. gem me tcont pointer is changed/assigned
// b. pq me related port or ts pointer is changed/assigned
int
omciutil_misc_reassign_pq_for_gem_ctp_in_rate_control_mode(struct me_t *gem_me)
{
	struct me_t *igem_me=hwresource_public2private(gem_me);
	struct attr_value_t attr_tcont_pointer={0}, attr_pq_pointer={0};
	int old_pq_meid, new_pq_meid;

	meinfo_me_attr_get(igem_me, 5, &attr_pq_pointer);
	old_pq_meid = attr_pq_pointer.data;
	if (attr_pq_pointer.data != 0xffff) {
		// remove gem from old pq
		attr_pq_pointer.data = 0xffff;
		meinfo_me_attr_set(igem_me, 5, &attr_pq_pointer, 0);
	}

	// locate new pq for this gem by searching all pq with same parent tcont as the gem me
	meinfo_me_attr_get(gem_me, 2, &attr_tcont_pointer);
	if ((new_pq_meid = omciutil_misc_most_idle_pq_under_same_tcont(gem_me, attr_tcont_pointer.data)) >0) {
		attr_pq_pointer.data = new_pq_meid;
		meinfo_me_attr_set(igem_me, 5, &attr_pq_pointer, 0);
		if (new_pq_meid != old_pq_meid) {
			if (old_pq_meid == 0xffff)
				dbprintf(LOG_DEBUG, "gem 0x%x is added to pq 0x%x\n", gem_me->meid, new_pq_meid);
			else
				dbprintf(LOG_DEBUG, "gem 0x%x is moved from pq 0x%x to 0x%x\n", gem_me->meid, old_pq_meid, new_pq_meid);
			return 1;
		} else {
			return 0;
		}
	}
	
	dbprintf(LOG_DEBUG, "gem 0x%x is not related to any pq\n", gem_me->meid);
	return -1;
}

struct me_t *
omciutil_misc_find_pptp_related_veip(struct me_t *pptp)
{
	char *devname_pptp = meinfo_util_get_config_devname(pptp);
	char *endptr;
	int instance_num;

	if (devname_pptp == NULL || strncmp(devname_pptp, "veip", 4) != 0 )
		return NULL;

	instance_num = strtol(devname_pptp+4, &endptr, 0);
	if (endptr == devname_pptp+4 || *endptr != 0 )	// no digit found after "veip" or some trail after digits
		return NULL;
	
	return meinfo_me_get_by_instance_num(329, instance_num);
}


struct me_t *
omciutil_misc_find_me_related_bport(struct me_t *me)
{
	struct meinfo_t *miptr = NULL;
	struct me_t *bport_me = NULL;

	if (me && (miptr = meinfo_util_miptr(47)) != NULL) {
		list_for_each_entry(bport_me, &miptr->me_instance_list, instance_node) {
			if (me_related(bport_me, me))
				return bport_me;
		}
	}
	return NULL;
}

// find wanif me related bport -> mcast_subscriber -> operation profile 
// wanif me could be veip, iphost or even pptp_uni
int
omciutil_misc_me_igmp_us_tci_tag_control_collect(struct me_t *wanif_me, 
					unsigned short us_tci[], unsigned char us_tag_control[], int size)
{
	struct meinfo_t *miptr = meinfo_util_miptr(309);
	struct me_t *bport_me, *mcast_subscriber_me, *me;
	int total = 0;

	if (miptr == NULL)
		return -1;
	if ((bport_me = omciutil_misc_find_me_related_bport(wanif_me)) == NULL)
		return -2;
	if ((mcast_subscriber_me = meinfo_me_get_by_meid(310, bport_me->meid)) == NULL)
		return 0;
		
	// find all possible mcast_op_profile(309) that the mcast_subscriber_me(310) related to
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(mcast_subscriber_me, me)) {
			us_tci[total] = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 4), 2*8, 0, 16);
			us_tag_control[total] = (unsigned char)meinfo_util_me_attr_data(me, 5);
			total++;
		}
	}
	return total;
}

// wanif me could be veip, iphost or even pptp_uni
int
omciutil_misc_me_rg_tag_ani_tag_collect(struct me_t *wanif_me, 
					unsigned short rg_tag_list[], 
					unsigned short ani_tag_list[], int size)
{
	struct batchtab_cb_tagging_t *t;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos, *rule_fields_n;
	struct list_head *rule_list = NULL;
	struct me_t *bport_me;

	int i, total = 0;

	if ((bport_me = omciutil_misc_find_me_related_bport(wanif_me)) == NULL)
		return 0;

	t = batchtab_table_data_get("tagging");
	if (t == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "batchtab_cb_tagging == NULL\n");
		return 0;
	}

	for ( i = 0 ; i < t->total ; i++ ) {
		if ( t->tagging_bport[i].bport_meid == bport_me->meid)
			break;
	}
	if (i == t->total) {	// wanif me related bport_me not found
		batchtab_table_data_put("tagging");
		return 0;
	}

	// tag match ////////////////////////////////////
	rule_list = &(t->tagging_bport[i].rule_list_us);
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, rule_list, rule_node) {

		if (!rule_fields_pos->owner_me || rule_fields_pos->owner_me->classid != 171)	// only collect rule from calssid 171
			continue;

		if (rule_fields_pos->filter_outer.priority != 15)	// ingress 2 tag ///////////////////////////////////
			continue;

		if (rule_fields_pos->filter_inner.priority != 15 ) {	// ingress 1 tag ///////////////////////////////////
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IGNORE_UNTAG) {
				if (rule_fields_pos->filter_inner.vid == 0)
					continue;
			}
		 
			if (rule_fields_pos->filter_inner.priority == 14)	// skip 1 tag default rule
				continue;
			if (rule_fields_pos->filter_inner.vid == 4096) 		// skip ingress vid dont care
				continue;
			if (rule_fields_pos->treatment_tag_to_remove >1)	// skip remove 2 tag from 1 tag?
				continue;
			
			if (rule_fields_pos->treatment_tag_to_remove == 1) {	// remove 1 tag 
				if (rule_fields_pos->treatment_outer.priority != 15)	// add 2 tag
					continue;				
				if (rule_fields_pos->treatment_inner.priority == 15)	// add 0 tag
					continue;

				// ingress 1 tag: rm 1 tag 				// add 1 tag
				if (rule_fields_pos->treatment_inner.vid == 4097)	// skip if refer to ingress outer vid?
					continue;	
				if (rule_fields_pos->treatment_inner.vid == 4096) {	// refer to ingress inner vid
					rg_tag_list[total] = rule_fields_pos->filter_inner.vid;
					ani_tag_list[total] = rule_fields_pos->filter_inner.vid;
				} else {						// assign inner vid
					rg_tag_list[total] = rule_fields_pos->filter_inner.vid;
					ani_tag_list[total] = rule_fields_pos->treatment_inner.vid;
				}
				// filter priority must be preserved if it is explicitly specfied, or RG output won't match the 171 rule
				// in other case, the filter priority is 8 (wildcard), then we set pbit 0 and mark de bit for RG vlan
				if (rule_fields_pos->filter_inner.priority <=7)
					rg_tag_list[total] |= (rule_fields_pos->filter_inner.priority<<13);
				if (rule_fields_pos->filter_inner.priority == 8)
					rg_tag_list[total] |= (1<<12);
				// treatment priority would be usefull only if different rg vlan is mapped to same ani vlan with different pbit
				// in that case treatment priority would be explicitly specified
				if (rule_fields_pos->treatment_inner.priority <=7)
					ani_tag_list[total] |= (rule_fields_pos->treatment_inner.priority<<13);

			} else {	// treatment_tag_to_remove==0		// remove 0 tag
				if (rule_fields_pos->treatment_outer.priority != 15)	// add 2 tag
					continue;				
				if (rule_fields_pos->treatment_inner.priority != 15) {	// add 1 tag
					if (rule_fields_pos->treatment_inner.vid == 4097)	// skip if refer to ingress outer vid?
						continue;	
					// ingress 1 tag: rm 0 tag && add 1 tag
					if (rule_fields_pos->treatment_inner.vid == 4096) {	// refer to ingress inner vid
						rg_tag_list[total] = rule_fields_pos->filter_inner.vid;
						ani_tag_list[total] = rule_fields_pos->filter_inner.vid;
					} else {						// assign inner vid
						rg_tag_list[total] = rule_fields_pos->filter_inner.vid;
						ani_tag_list[total] = rule_fields_pos->treatment_inner.vid;
					}
					// filter priority must be preserved if it is explicitly specfied, or RG output won't match the 171 rule
					// in other case, the filter priority is 8 (wildcard), then we set pbit 0 and mark de bit for RG vlan
					if (rule_fields_pos->filter_inner.priority <=7)
						rg_tag_list[total] |= (rule_fields_pos->filter_inner.priority<<13);
					if (rule_fields_pos->filter_inner.priority == 8)
						rg_tag_list[total] |= (1<<12);
					// treatment priority would be usefull only if different rg vlan is mapped to same ani vlan with different pbit
					// in that case treatment priority would be explicitly specified
					if (rule_fields_pos->treatment_inner.priority <=7)
						ani_tag_list[total] |= (rule_fields_pos->treatment_inner.priority<<13);
						
				} else { 						// add 0 tag
					// ingress 1 tag: rm 0 tag && add 0 tag
					rg_tag_list[total] = rule_fields_pos->filter_inner.vid;
					// filter priority must be preserved if it is explicitly specfied, or RG output won't match the 171 rule
					// in other case, the filter priority is 8 (wildcard), then we set pbit 0 and mark de bit for RG vlan
					if (rule_fields_pos->filter_inner.priority <=7)
						rg_tag_list[total] |= (rule_fields_pos->filter_inner.priority<<13);
					if (rule_fields_pos->filter_inner.priority == 8)
						rg_tag_list[total] |= (1<<12);
					ani_tag_list[total] = rg_tag_list[total];
				}
			}

		} else { // filter_outer.priority==15 && filter_inner.priority==15   // ingress untag /////////////////////////
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IGNORE_UNTAG) {
				continue;
			}
		
			if (rule_fields_pos->treatment_tag_to_remove !=0)	// skip remove tag on untag?
				continue;
			if (rule_fields_pos->treatment_outer.priority != 15)	// add 2 tag
				continue;
			if (rule_fields_pos->treatment_inner.priority == 15)	// add 0 tag 
				continue;
			//add one tag				
			if (rule_fields_pos->treatment_inner.priority == 8||
			    rule_fields_pos->treatment_inner.priority == 9)	// skip refer to ingress inner/outer prio?
				continue;
			if (rule_fields_pos->treatment_inner.vid == 4096 ||
			    rule_fields_pos->treatment_inner.vid == 4097)	// skip refer to ingress inner/outer vid?
				continue;
			// ingress 0 tag: rm 0 tag && add 1 tag								
			rg_tag_list[total] = 0;
			ani_tag_list[total] = rule_fields_pos->treatment_inner.vid;
			// treatment priority would be usefull only if different rg vlan is mapped to same ani vlan with different pbit
			// in that case treatment priority would be explicitly specified
			if (rule_fields_pos->treatment_inner.priority <=7)
				ani_tag_list[total] |= (rule_fields_pos->treatment_inner.priority<<13);
		}
		total++;
		if (total == size)
			break;
	}
	batchtab_table_data_put("tagging");
	return total;
}

int
omciutil_misc_tag_list_lookup(unsigned short tag, int tag_list_total, unsigned short tag_list[])
{
	if (tag_list_total > 0) {
		int i;
		// first: try to match pbit & vid 
		for (i=0; i < tag_list_total; i++) {
			if ((tag_list[i]&0xefff) == (tag&0xefff))
				return i;
		}
		// second: match vid only
		for (i=0; i < tag_list_total; i++) {
			if ((tag_list[i]&0xfff) == (tag&0xfff))
				return i;
		}
	}
	return -1;
}

int
omciutil_misc_get_veip_rg_tag_by_ani_tag(struct me_t *veip, unsigned short ani_tag)
{
	unsigned short rg_tag_list[16], ani_tag_list[16];
	int total, tag_index = -1;
	if (veip == NULL)
		return -1;
	total = omciutil_misc_me_rg_tag_ani_tag_collect(veip, rg_tag_list, ani_tag_list, 16);
	if (total > 0)
		tag_index = omciutil_misc_tag_list_lookup(ani_tag, total, ani_tag_list);
	if (tag_index < 0)
		return -1;
	return rg_tag_list[tag_index];
}

// return value is the rg vlan pbit for the derived devname, -1 means error
int
omciutil_misc_get_devname_by_veip_ani_tag(struct me_t *veip, unsigned short ani_tag, char *devname, int size)
{
	char *veip_devname;
	int rg_tag;

	if ((veip_devname = meinfo_util_get_config_devname(veip)) == NULL || strlen(veip_devname) == 0)
		return -1;	// veip devname invalid
	if (iphost_interface_is_exist(wanif_devname_base(veip_devname)) == 0)
		return -2;	// veip devname basedev doesn't exist (because basedev could not be created by vconfig)
	rg_tag = omciutil_misc_get_veip_rg_tag_by_ani_tag(veip, ani_tag);
	if (rg_tag < 0) {
		//rg_tag = ani_tag; // if no vlan translation found, assume rg vlan == ani vlan
		return -3;
	}
	devname[size-1] = 0;
	if ((rg_tag&0xfff) == 0) {
		snprintf(devname, size, "%s", wanif_devname_base(veip_devname));
	} else {
		snprintf(devname, size, "%s.%d", wanif_devname_base(veip_devname), rg_tag&0xfff);
	}
	return (rg_tag>>13) & 0x7;
}

struct me_t *
omciutil_misc_get_classid_me_by_devname_base(int classid, char *devname)
{
	char devname_base[33];
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me, *veip_me;
		
	strncpy(devname_base, wanif_devname_base(devname), 33);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		// check for me 134, 329
		if (me->classid == 134 || me->classid == 329) {
			char *me_devname = meinfo_util_get_config_devname(me);
			if (me_devname && strcmp(wanif_devname_base(me_devname), devname_base) == 0)
				return me;
		} else {	// check for me 340, 65317
			veip_me = meinfo_me_get_by_meid(329, me->meid);
			if (veip_me) {
				char *veip_devname = meinfo_util_get_config_devname(veip_me);
				if (veip_devname && strcmp(wanif_devname_base(veip_devname), devname_base) == 0)
					return me;
			}
		}
	}
	return NULL;
}

int
omciutil_misc_get_rg_tag_list_index_by_devname_vlan(unsigned short rg_tag_list[], int list_size, char *devname)
{
	int devname_vlan = wanif_devname_vlan(devname);
	int i;
	
	if (devname_vlan < 0)	// untag
		devname_vlan = 0;	
	for (i=0; i<list_size; i++) {
		if ((rg_tag_list[i]&0xfff) == devname_vlan)
			return i;
	}
	return -1;
}

#ifdef OMCI_METAFILE_ENABLE
int
omciutil_misc_locate_wan_new_devname_index(struct metacfg_t *kv_config, char *prefix)
{
	char key[32], *devname;
	struct meinfo_t *miptr;
	struct me_t *me;
	int i, index, used[8] = {0};

	// find wan devname used in metafile dev_wan[0-7]_interface
	for (i=0; i<8; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i); 
		devname = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (devname && strncmp(devname, prefix, strlen(prefix)) == 0) {
			index = devname[strlen(prefix)] - '0';	// assume devname index is 0..7, only 1 digit
			if (index >=0 && index <=7)
				used[index] = 1;
		}
	}

	// wan devname used in me 134 (iphost)
	miptr=meinfo_util_miptr(134);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		devname = meinfo_util_get_config_devname(me);
		if (devname && strncmp(devname, prefix, strlen(prefix)) == 0) {
			index = devname[strlen(prefix)] - '0';	// assume devname index is 0..7, only 1 digit
			if (index >=0 && index <=7)
				used[index] = 1;
		}
	}
	
	// wan devname used in me 329 (veip)
	miptr=meinfo_util_miptr(329);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		devname = meinfo_util_get_config_devname(me);
		if (devname && strncmp(devname, prefix, strlen(prefix)) == 0) {
			index = devname[strlen(prefix)] - '0';	// assume devname index is 0..7, only 1 digit
			if (index >=0 && index <=7)
				used[index] = 1;
		}
	}

	for (i=0; i<8; i++) {
		if (used[i] == 0)
			return i;
	}
	return -1;
}
#endif

char *
omciutil_misc_get_uniname_by_logical_portid(unsigned int logical_portid)
{
	 struct meinfo_t *miptr=meinfo_util_miptr(413);	// private pptp uni
	 struct me_t *me;
	 
	 list_for_each_entry(me, &miptr->me_instance_list, instance_node) {	
		unsigned char uni_logical_portid = meinfo_util_me_attr_data(me, 4);
		if (uni_logical_portid == logical_portid) {
			char *devname = meinfo_util_get_config_devname(me);
			if (devname == NULL)
				return "uni?";
			return devname;
		}
	}
	return NULL;
}
