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
 * Module  : proprietary_fiberhome
 * File    : meinfo_cb_65286.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "me_related.h"

#define OMCI_POLICY_NULL	0
#define OMCI_POLICY_SP		1
#define OMCI_POLICY_WRR		2

#define QUEUE_MODE_WRR		0
#define QUEUE_MODE_SP		1
#define QUEUE_MODE_SP_WRR	2

// 9.99.20 ONU performance
static unsigned char
calc_average_weight_from_child_pq(struct me_t *ts_me)
{
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	int n=0, n_weight=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, ts_me)) {
			n_weight+=(unsigned char)meinfo_util_me_attr_data(me, 8);
			n++;
		}
	}
	if (n==0)
		return 0;	// should not happen
	return n_weight/n;
}

static unsigned short
find_highest_priority_from_child_pq(struct me_t *ts_me)
{
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	unsigned short highest_prio=0xfff;	// 0 is highest, 0xfff is lowest

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, ts_me)) {
			unsigned int related_port=ntohl(*(unsigned int*)meinfo_util_me_attr_ptr(me, 6));
			unsigned short prio=(unsigned short)(related_port & 0xffff);			
			if (prio < highest_prio)
				highest_prio = prio;	
		}
	}
	return highest_prio;
}

static int
meinfo_cb_65286_from_277(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *pq_me=NULL, *ts_me=NULL, *tcont_me=NULL;
	struct attr_value_t attr;

	if ((pq_me=meinfo_me_get_by_instance_num(277, me->meid))==NULL)	// priority queue
		return MEINFO_ERROR_ME_NOT_FOUND;
	ts_me=me_related_find_parent_me(pq_me, 278);		// traffic scheduler
	if (ts_me)
		tcont_me=me_related_find_parent_me(ts_me, 262);	// tcont

	if (util_attr_mask_get_bit(attr_mask, 1)) {	// Queue priority 
		unsigned char *fields_data=meinfo_util_me_attr_ptr(pq_me, 6);
		unsigned short priority = util_bitmap_get_value(fields_data, 4 * 8, 16, 16);
		attr.data = priority;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 2)) {	// Queue mode
		// 0:null, 1:sp, 2:wrr
		if (ts_me && tcont_me) {	// pq for wan side
			unsigned char ts_policy=meinfo_util_me_attr_data(ts_me, 3);
			unsigned char tcont_policy=meinfo_util_me_attr_data(tcont_me, 3);
		
			if (tcont_policy==OMCI_POLICY_WRR && ts_policy==OMCI_POLICY_WRR) {
				attr.data=QUEUE_MODE_WRR;	// FH WRR
			} else if (tcont_policy==OMCI_POLICY_SP && ts_policy==OMCI_POLICY_SP) {
				attr.data=QUEUE_MODE_SP;	// FH SP
			} else if (tcont_policy==OMCI_POLICY_SP && ts_policy==OMCI_POLICY_WRR) {
				attr.data=QUEUE_MODE_SP_WRR;	// FH SP+WRR
			} else {
				attr.data=QUEUE_MODE_SP_WRR;	// WRR+SP, should not happen
			}
		} else {			// pq for cpu/uni/veip....
			attr.data=QUEUE_MODE_SP;	// FH SP
		}
		meinfo_me_attr_set(me, 2, &attr, 0);	
		
	}	
	if (util_attr_mask_get_bit(attr_mask, 3)) {	// Queue weight
		attr.data = meinfo_util_me_attr_data(pq_me, 8);	
		meinfo_me_attr_set(me, 3, &attr, 0);	
	}	
	return 0;
}

static unsigned short 
meinfo_cb_65286_meid_gen(unsigned short instance_num)
{
	return instance_num;
}

static int 
meinfo_cb_65286_create(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char attr_mask2[2]={0xe0,0x0};
	return meinfo_cb_65286_from_277(me, attr_mask2);
}

static int 
meinfo_cb_65286_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char attr_mask2[2]={0xe0,0x0};
	return meinfo_cb_65286_from_277(me, attr_mask2);
}

static int 
meinfo_cb_65286_set(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char priority	= meinfo_util_me_attr_data(me, 1);
	unsigned short queue_mode = meinfo_util_me_attr_data(me, 2);	// 0: wrr, 1:sp, 2:sp+wrr
	unsigned short weight =	 meinfo_util_me_attr_data(me, 3);

	struct me_t *pq_me=NULL, *ts_me=NULL, *tcont_me=NULL;
	unsigned char pq_attr_mask[2]={0};
	unsigned char ts_attr_mask[2]={0};
	unsigned char tcont_attr_mask[2]={0};
	
	if ((pq_me=meinfo_me_get_by_instance_num(277, me->meid))==NULL)	// priority queue
		return MEINFO_ERROR_ME_NOT_FOUND;
	ts_me=me_related_find_parent_me(pq_me, 278);		// traffic scheduler
	if (ts_me)
		tcont_me=me_related_find_parent_me(ts_me, 262);	// tcont

	if (util_attr_mask_get_bit(attr_mask, 1)) {	// Queue priority 
		unsigned char *fields_data=meinfo_util_me_attr_ptr(pq_me, 6);
		util_bitmap_set_value(fields_data, 4 * 8, 16, 16, priority);
		util_attr_mask_set_bit(pq_attr_mask, 6);
	}

	if (util_attr_mask_get_bit(attr_mask, 2)) {	// Queue mode
		struct attr_value_t attr_tcont_policy={0, NULL};
		struct attr_value_t attr_ts_policy={0, NULL};
		
		if (tcont_me && ts_me) {	// pq for wan side
			if (queue_mode==QUEUE_MODE_WRR) {
				attr_tcont_policy.data=OMCI_POLICY_WRR;
				attr_ts_policy.data=OMCI_POLICY_WRR;
			} else if (queue_mode==QUEUE_MODE_SP) {
				attr_tcont_policy.data=OMCI_POLICY_SP;
				attr_ts_policy.data=OMCI_POLICY_SP;
			} else if (queue_mode==QUEUE_MODE_SP_WRR) {
				attr_tcont_policy.data=OMCI_POLICY_SP;
				attr_ts_policy.data=OMCI_POLICY_WRR;				
			}
			meinfo_me_attr_set(tcont_me, 3, &attr_tcont_policy, 0);	
			util_attr_mask_set_bit(tcont_attr_mask, 3);
			meinfo_me_attr_set(ts_me, 3, &attr_ts_policy, 0);	
			util_attr_mask_set_bit(ts_attr_mask, 3);
		}	// else pq for lan side is always SP, which can not be changed
	}

	if (util_attr_mask_get_bit(attr_mask, 3)) {	// Queue weight
		struct attr_value_t attr_pq_weight={0, NULL};
		attr_pq_weight.data = weight;
		meinfo_me_attr_set(pq_me, 8, &attr_pq_weight, 0);	
		util_attr_mask_set_bit(pq_attr_mask, 8);
	}

	// update ts weight or priority based on latest pq state
	if (util_attr_mask_get_bit(attr_mask, 1) ||
	    util_attr_mask_get_bit(attr_mask, 2) ||
	    util_attr_mask_get_bit(attr_mask, 3)) {	     
		if (ts_me) {	// pq for wan side
			if (queue_mode==QUEUE_MODE_SP ||	// tcont policy sp
			    queue_mode==QUEUE_MODE_SP_WRR) {	// tcont policy sp
				struct attr_value_t attr_ts_priority={0, NULL};
				attr_ts_priority.data=find_highest_priority_from_child_pq(ts_me);
				meinfo_me_attr_set(ts_me, 4, &attr_ts_priority, 0);	// update ts priority with highest prio of pq
				util_attr_mask_set_bit(ts_attr_mask, 4);
			} else if (queue_mode==QUEUE_MODE_WRR) {	// tcont policy wrr
				struct attr_value_t attr_ts_weight={0, NULL};
				attr_ts_weight.data=calc_average_weight_from_child_pq(ts_me);
				meinfo_me_attr_set(ts_me, 4, &attr_ts_weight, 0);	// update ts weight with average weight of pq
				util_attr_mask_set_bit(ts_attr_mask, 4);
			}
		}
	}
	
	// call cb and hw after mib write
	if (pq_attr_mask[0]||pq_attr_mask[1])
		meinfo_me_flush(pq_me, pq_attr_mask);
	if (tcont_attr_mask[0]||ts_attr_mask[0]) {
		meinfo_me_flush(tcont_me, tcont_attr_mask);
		meinfo_me_flush(ts_me, ts_attr_mask);
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65286 = {
	.meid_gen_cb	= meinfo_cb_65286_meid_gen,
	.create_cb	= meinfo_cb_65286_create,
	.get_cb		= meinfo_cb_65286_get,
	.set_cb		= meinfo_cb_65286_set,
};
