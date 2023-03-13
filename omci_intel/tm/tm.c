/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : map tcont-ts-pq 3tier prio/weight to tcont-pq 2tier
 * Module  : tm
 * File    : tm.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>

#include "util.h"
#include "tm.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "me_related.h"
#include "gpon_sw.h"
#include "omciutil_misc.h"

// this tm module tries to 
// 1. collect tcont->ts->pq related to tm->omcitree first
// 2. calc the queiv prio/weight for per pq in tm->omcitree 
//    this will convert tcont-ts-pq (3tier) to tcont-pq (2tier)
// 3. iterate pq in omci tree, alloc phy pq in  alloc_map_prepare
//    this will map per omci_pq:omci_gem tp phy_pq, so multiple phy pq may be mapped to one omci pq
// 4. sync to hw based on alloc_map

// routines for omci data stru access ///////////////////////////////////////////////////

static int
is_pq_related_by_gem(struct me_t *me_pq)
{
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct me_t *me_gem;

	list_for_each_entry(me_gem, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me_pq))
			return 1;
	}
	return 0;
}

static int
is_ts_related_by_gem(struct me_t *me_ts)
{
	struct meinfo_t *miptr_gem=meinfo_util_miptr(268);
	struct meinfo_t *miptr_pq=meinfo_util_miptr(277);
	struct me_t *me_gem, *me_pq;

	list_for_each_entry(me_gem, &miptr_gem->me_instance_list, instance_node) {
		list_for_each_entry(me_pq, &miptr_pq->me_instance_list, instance_node) {
			if (me_related(me_gem, me_pq) && me_related(me_pq, me_ts))
				return 1;
		}
	}
	return 0;
}

static int
ts_has_upstream_pq(struct me_t *me_ts)
{
  	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	int total=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->meid & 0x8000 &&	// upstream
			me_related(me, me_ts)) {
			total++;
		}
	}
	return total;
}

struct gem_cir_pir_t {
	struct me_t *me_gem;
	unsigned int cir;
	unsigned int pir;
};
static int
gem_cir_pir_compare(const void *p1, const void *p2)
{
	struct gem_cir_pir_t *g1 = (struct gem_cir_pir_t *)p1;
	struct gem_cir_pir_t *g2 = (struct gem_cir_pir_t *)p2;	 

	if (g1->cir > g2->cir) {
		return 1;
	} else if (g1->cir < g2->cir) {
		return -1;
	} else if (g1->pir > g2->pir) {
		return 1;
	} else if (g1->pir < g2->pir) {
		return -1;
	}
	return 0;
}
static int
me_pq_related_me_gem_sorted(struct me_t *me_pq, struct me_t *me_gem_list[], int list_size)
{
	struct meinfo_t *miptr_gem=meinfo_util_miptr(268);

	struct gem_cir_pir_t g[TM_PER_PQ_RELATED_FLOW_MAX];
	struct me_t *me;
	int i, total = 0;

	memset(g, 0, sizeof(g));
	list_for_each_entry(me, &miptr_gem->me_instance_list, instance_node) {
		if (me_related(me, me_pq)) {
			struct me_t *me_td = meinfo_me_get_by_meid(280, (unsigned short)meinfo_util_me_attr_data(me, 5)); // attr5 is td us
			g[total].me_gem = me;
			if (me_td) {
				g[total].cir = (unsigned int)meinfo_util_me_attr_data(me_td, 1);
				g[total].pir = (unsigned int)meinfo_util_me_attr_data(me_td, 2);
			}
			total++;
			if (total >=TM_PER_PQ_RELATED_FLOW_MAX)
				break;
		}
	}
	qsort(g, total, sizeof(struct gem_cir_pir_t), gem_cir_pir_compare);
	for (i=0; i<total && i<list_size; i++) {
		me_gem_list[i] = g[i].me_gem;
	}
	return i;
}

// routines for tm_pq_gem_qos_method iudjement ///////////////////////////////////////////////////

static int
gem_has_effective_related_td(struct me_t *me_gem)
{
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me)) {
			if ((unsigned short)meinfo_util_me_attr_data(me_gem, 5)==me->meid) { //US
				unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
				unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
				if((cir > 0 && cir < 125000000) || (pir > 0 && pir < 125000000)) { //1Gbps
					return 1;
				}
			}
		}
	}
	return 0;
}
static int
pq_has_effective_related_td(struct me_t *me_pq)
{
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me, *meptr=meinfo_me_get_by_meid(65323, me_pq->meid);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meptr && me_related(meptr, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			if((cir > 0 && cir < 125000000) || (pir > 0 && pir < 125000000)) { //1Gbps
				return 1;
			}
		}
	}
	return 0;
}
/*
if one pq has multiple gem port and each of them has useless td(cir=0, pir>=wirespeed) => return 0; tm_pq_gem_qos_method=0
if one pq has multiple gem port and each of them has the same td value(cir,pir) => return 1; tm_pq_gem_qos_method=2 (Bug 3792)
if one pq has multiple gem port and each of them has different td value(cir, pir) => return 1; tm_pq_gem_qos_method=2
*/
static int
tcont_has_effective_related_td(unsigned short tcont_meid)
{
	struct meinfo_t *miptr_tcont=meinfo_util_miptr(262);
	struct meinfo_t *miptr_ts=meinfo_util_miptr(278);
	struct meinfo_t *miptr_pq=meinfo_util_miptr(277);
	struct meinfo_t *miptr_gem=meinfo_util_miptr(268);
	struct me_t *me_tcont;
	struct me_t *me_ts;
	struct me_t *me_pq;
	struct me_t *me_gem;

	list_for_each_entry(me_tcont, &miptr_tcont->me_instance_list, instance_node) {
		if (tcont_meid != me_tcont->meid)
			continue;
		list_for_each_entry(me_ts, &miptr_ts->me_instance_list, instance_node) {
			if (!me_related(me_ts, me_tcont))
				continue;
			list_for_each_entry(me_pq, &miptr_pq->me_instance_list, instance_node) {
				if (!me_related(me_pq, me_ts))
					continue;
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) {
					if (pq_has_effective_related_td(me_pq))
						return 1;
				}
				list_for_each_entry(me_gem, &miptr_gem->me_instance_list, instance_node) {
					if (!me_related(me_gem, me_pq))
						continue;
					if (gem_has_effective_related_td(me_gem))
						return 1;
				}
			}
		}

	}
	return 0;
}

// omcitree related: collect tcont/ts/pq, fill equiv prio/weight ///////////////////////////////////////////////

static int
tm_omcitree_collect_ts_related_pq(struct tm_ts_node_t *tm_ts_node, int collect_active_only)
{
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	unsigned char number_of_pq_in_ts=0;
	//char *devname;


	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, tm_ts_node->me_ts)) {
			unsigned int related_port;
			struct tm_pq_node_t *tm_pq_node, *exist_pq_node;
			unsigned char found_pos=0;

			if (collect_active_only && !is_pq_related_by_gem(me))
				continue;

			//meinfo_me_refresh(me, (unsigned char[]){0xff, 0xff});

			//allocate tm_ts node and keep ts's me_t
			tm_pq_node= malloc_safe(sizeof(struct tm_pq_node_t));
			tm_pq_node->me_pq=me;
			//green
			tm_pq_node->green_q.schedule_id=-1;
			tm_pq_node->green_q.queue_id=-1;
			tm_pq_node->green_q.phy_queueid=-1;  

			//yellow
			tm_pq_node->yellow_q.schedule_id=-1;
			tm_pq_node->yellow_q.queue_id=-1;
			tm_pq_node->yellow_q.phy_queueid=-1;  

			//keep gem port id
			//tm_pq_related_gem(me);

			related_port=ntohl(*(unsigned int*)meinfo_util_me_attr_ptr(me, 6));
			dbprintf_bat(LOG_DEBUG, "\n\t\t<[277]pq:0x%x", me->meid);

			if (meinfo_util_me_attr_data(tm_ts_node->me_ts, 3)==2) {	// ts is wrr
				tm_pq_node->weight=(unsigned char)meinfo_util_me_attr_data(me, 8);
				dbprintf_bat(LOG_NOTICE, ",weight(omci)=%d", tm_pq_node->weight);

				if (omci_env_g.invert_weight_enable & 1) {
					int weight=tm_pq_node->weight;

					tm_pq_node->weight=(weight==1) ? 255 : (255/(weight?weight:1));//omci invert
					dbprintf_bat(LOG_NOTICE, ",(invert)=%d", tm_pq_node->weight);
				}

			} else {					// ts is sp
				tm_pq_node->priority=(unsigned short)(related_port & 0xffff);
				dbprintf_bat(LOG_NOTICE, ",prio(omci)=%d", tm_pq_node->priority);

				if (omci_env_g.invert_priority_enable & 1) {
					// cast omci:0(high)-4095(low) to 0(high)-7(low)
					if (tm_pq_node->priority >= 8)
						tm_pq_node->priority= 7;

					// omci definition: 0(high)..7(low), invert:7(high)..0(low)
					tm_pq_node->priority= 7 - tm_pq_node->priority;
					dbprintf_bat(LOG_NOTICE, ",(invert)=%d", tm_pq_node->priority);
				}

				//insert sort
				list_for_each_entry(exist_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
                        		if(tm_pq_node->priority < exist_pq_node->priority) {      //from lowest to highest
                                		found_pos=1;
						list_add_tail(&tm_pq_node->tm_pq_entry_node, &exist_pq_node->tm_pq_entry_node);
                                		break;
					}
				}
			}

			if( found_pos == 0) {
				list_add_tail(&tm_pq_node->tm_pq_entry_node, &tm_ts_node->tm_pq_list);
			}
			number_of_pq_in_ts++;
		}
	}
	tm_ts_node->number_of_pq_in_ts=number_of_pq_in_ts;
	return 0;
}

static int
tm_omcitree_collect_tcont_related_ts(struct tm_tcont_node_t *tm_tcont_node, int collect_active_only)
{
	struct meinfo_t *miptr=meinfo_util_miptr(278);
	struct me_t *me;
	struct tm_ts_node_t *tm_ts_node;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, tm_tcont_node->me_tcont) && ts_has_upstream_pq(me)) {
			if (collect_active_only && !is_ts_related_by_gem(me))
				continue;
			//meinfo_me_refresh(me, (unsigned char[]){0xff, 0xff});
			//allocate tm_ts node and keep ts's me_t
			if((tm_ts_node= malloc_safe(sizeof(struct tm_ts_node_t))) != NULL) {
				INIT_LIST_HEAD(&tm_ts_node->tm_pq_list);
				tm_ts_node->me_ts=me;
				dbprintf_bat(LOG_DEBUG, "\n\t<[278]ts:0x%x,%s", me->meid, (meinfo_util_me_attr_data(me, 3)==2)?"WRR":"SP");

				/* Policy: 0 Null; 1 Strict priority; 2 WRR Weighted round robin */
				tm_ts_node->policy_ts=meinfo_util_me_attr_data(me, 3);

				if (meinfo_util_me_attr_data(tm_tcont_node->me_tcont, 3)==2) {	// tcont is wrr
					tm_ts_node->weight=meinfo_util_me_attr_data(me, 4);
					dbprintf_bat(LOG_DEBUG, ",weight=%d", meinfo_util_me_attr_data(me, 4));
				} else {					// tcont is sp
					tm_ts_node->priority=meinfo_util_me_attr_data(me, 4);
					dbprintf_bat(LOG_DEBUG, ",prio=%d", meinfo_util_me_attr_data(me, 4));
				}

				list_add_tail(&tm_ts_node->tm_ts_entry_node, &tm_tcont_node->tm_ts_list);

				tm_omcitree_collect_ts_related_pq(tm_ts_node, collect_active_only);
			}
		}
	}
	return 0;
}

// meid: 0xffff: for unused
static int
tm_omcitree_collect_by_tcont(struct batchtab_cb_tm_t *tm, unsigned short meid, int collect_active_only)
{
	struct meinfo_t *miptr=meinfo_util_miptr(262);
	struct me_t *me;
	struct tm_tcont_node_t *tm_tcont_node;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meid==0xffff || meid==me->meid) {
			//meinfo_me_refresh(me, (unsigned char[]){0xff, 0xff});
			if (collect_active_only && ((meinfo_util_me_attr_data(me, 1)==0xff) ||((meinfo_util_me_attr_data(me, 1)==0xffff))))
				continue;
			dbprintf_bat(LOG_DEBUG, "[262]t-cont:0x%x,%s,allocid=0x%x\n", 
				me->meid, (meinfo_util_me_attr_data(me, 3)==2)?"WRR":"SP", 
				(unsigned short)meinfo_util_me_attr_data(me, 1));

			//allocate tm_tcont node and keep tcont's me_t
			tm_tcont_node= malloc_safe(sizeof(struct tm_tcont_node_t));
			INIT_LIST_HEAD(&tm_tcont_node->tm_ts_list);
			tm_tcont_node->me_tcont=me;
			/* Policy: 0 Null; 1 Strict priority; 2 WRR Weighted round robin */
			tm_tcont_node->policy_tcont=meinfo_util_me_attr_data(me, 3);

			//if we have effective td, we guess tm_pq_gem_qos_method will be 2, else use 0
			if(tcont_has_effective_related_td(me->meid)==1) {
				tm_tcont_node->tm_pq_gem_qos_method=2;
			} else {
				tm_tcont_node->tm_pq_gem_qos_method=0;
			}

			list_add_tail(&tm_tcont_node->tm_tcont_entry_node, &tm->omcitree);
			tm_omcitree_collect_tcont_related_ts(tm_tcont_node, collect_active_only);
		}
	}
	return 0;
}

// set equiv prio,weight for per pq in omcitree
static int 
tm_omcitree_calc_pq_prio_weight(struct batchtab_cb_tm_t *tm)
{
	struct tm_tcont_node_t *tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node;
	struct tm_pq_node_t *tm_pq_node;
	unsigned char policy_merge=0;

	list_for_each_entry(tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {
		unsigned char number_of_pq_in_tcont=0;

		dbprintf_bat(LOG_NOTICE, "t-cont:%d, policy:%d\n", tm_tcont_node->me_tcont->instance_num, tm_tcont_node->policy_tcont);

		list_for_each_entry(tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			//unsigned int sum_of_weight=0;
			unsigned char number_of_pq_in_ts=tm_ts_node->number_of_pq_in_ts, pq_position=0;

			if(tm_tcont_node->policy_tcont==1 &&  tm_ts_node->policy_ts == 1)
				policy_merge=TCONT_SP_TS_SP;
			else if(tm_tcont_node->policy_tcont==1 &&  tm_ts_node->policy_ts == 2)
				policy_merge=TCONT_SP_TS_WRR;
			else if(tm_tcont_node->policy_tcont==2 &&  tm_ts_node->policy_ts == 1)
				policy_merge=TCONT_WRR_TS_SP;
			else if(tm_tcont_node->policy_tcont==2 &&  tm_ts_node->policy_ts == 2)
				policy_merge=TCONT_WRR_TS_WRR;
			else {
				dbprintf_bat(LOG_ERR, "t-cont:%d, policy:%d\n", tm_tcont_node->me_tcont->instance_num, tm_tcont_node->policy_tcont);
				dbprintf_bat(LOG_ERR, "Error policy type\n");
				continue;
			}
			//keep for dump
			tm_ts_node->policy_merge=policy_merge;

			dbprintf_bat(LOG_NOTICE, "ts:%d, policy:%d (W:%d,P:%d)\n", 
				tm_ts_node->policy_ts, tm_ts_node->me_ts->instance_num, tm_ts_node->weight, tm_ts_node->priority);

			/* 
			//need calculate summation first
			if (policy_merge== TCONT_SP_TS_WRR) {
				list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
					sum_of_weight+=tm_pq_node->weight;
				}
				dbprintf_bat(LOG_ERR, "sum_of_weight=%d\n", sum_of_weight);
			}
			else if (policy_merge==TCONT_WRR_TS_SP) {
				list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
					sum_of_priority+=tm_pq_node->priority;
				}
				dbprintf_bat(LOG_ERR, "sum_of_priority=%d\n", sum_of_priority);
			}
			*/

			list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
				pq_position++;	//first entry position is 1
				number_of_pq_in_tcont++;
				dbprintf_bat(LOG_NOTICE, "pq:%d, (W:%d,P:%d)\n", tm_pq_node->me_pq->classid, tm_pq_node->weight, tm_pq_node->priority);
				switch(policy_merge) {
				case TCONT_SP_TS_SP:
					tm_pq_node->priority_h=tm_ts_node->priority;
					tm_pq_node->priority_l=tm_pq_node->priority;
					tm_pq_node->weight_h=1;
					tm_pq_node->weight_l=1;
				break;

				case TCONT_SP_TS_WRR:
					tm_pq_node->priority_h=tm_ts_node->priority;
					//tm_pq_node->priority_l=0xfff+(0xf000*(sum_of_weight - tm_pq_node->weight)/sum_of_weight);
					tm_pq_node->priority_l=0xfff;
					tm_pq_node->weight_h=1;
					tm_pq_node->weight_l=tm_pq_node->weight;
				break;

				case TCONT_WRR_TS_WRR:
					tm_pq_node->priority_h=0;
					tm_pq_node->priority_l=0;
					tm_pq_node->weight_h=tm_ts_node->weight;
					tm_pq_node->weight_l=tm_pq_node->weight;
				break;

				case TCONT_WRR_TS_SP:
					tm_pq_node->priority_h=0;
					//tm_pq_node->priority_l=tm_pq_node->priority;
					tm_pq_node->priority_l=0;	//in the end, we just look sp as wrr in this type
					tm_pq_node->weight_h=tm_ts_node->weight;

					//hope sum of priority is 255, notice: need another way when 8 pq 
					if(number_of_pq_in_ts == 1 ) {
						tm_pq_node->weight_l=255;
					} else {
						if(pq_position == 1) { 	//first entry, priority highest
							//255-(8+4+2+1)==> (8+4+2+1)==2^(5-1)-1
							tm_pq_node->weight_l=255-(pow(2,(number_of_pq_in_ts-1))-1);
						} else {
							tm_pq_node->weight_l=pow(2,(number_of_pq_in_ts-pq_position));
						}
					}
				break;
				}
			}
		}
		tm_tcont_node->number_of_pq_in_tcont=number_of_pq_in_tcont;
	}
	return 0;
}

// multi_prio_rr_tree related ////////////////////////////////////////////////////////////////////

// alloc pq to multi_prio_rr_tree based on equiv prio/weight info in omcitree
static int 
tm_multi_prio_rr_tree_add_pq(struct tm_pq_node_t *tm_pq_node, int pos, struct batchtab_cb_tm_t *tm)
{
	unsigned int equiv_prio;
	unsigned short equiv_weight;
	unsigned char is_already_add_sp=0;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node, *exist_equiv_prio_node;
	struct tm_pq_node_t *exist_tm_pq_node;

	equiv_prio=tm_pq_node->priority_h<<16 | tm_pq_node->priority_l;
	equiv_weight=tm_pq_node->weight_h * tm_pq_node->weight_l;
	tm_pq_node->equiv_weight=equiv_weight;
	tm_pq_node->equiv_priority=equiv_prio;

	dbprintf_bat(LOG_NOTICE, "\t\tpq:%d, (W:%d,P:%d)(Ph=%d,Pl=%d,Wh=%d,Wl=%d)\n", 
		tm_pq_node->me_pq->instance_num, tm_pq_node->weight, tm_pq_node->priority, 
		tm_pq_node->priority_h, tm_pq_node->priority_l, tm_pq_node->weight_h, tm_pq_node->weight_l);

	//insert node to tm->multi_prio_rr_tree[i].tm_equiv_prio_list according to equiv_priority
	list_for_each_entry(exist_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {
		if(equiv_prio < exist_equiv_prio_node->equiv_priority) {      //add new equiv_prio_node
			tm_equiv_prio_node= malloc_safe(sizeof(struct tm_equiv_prio_node_t));
			INIT_LIST_HEAD(&tm_equiv_prio_node->tm_pq_list2);
			tm_equiv_prio_node->equiv_priority=equiv_prio;
			list_add_tail(&tm_equiv_prio_node->tm_equiv_prio_entry_node, &exist_equiv_prio_node->tm_equiv_prio_entry_node);
			list_add_tail(&tm_pq_node->tm_pq_entry_node2, &tm_equiv_prio_node->tm_pq_list2);
			is_already_add_sp=1;
			break;
		} else if(equiv_prio == exist_equiv_prio_node->equiv_priority) { //add to tm_pq_list2 
			unsigned char is_already_add_weight=0;
			is_already_add_sp=1;
			//insert node to tm->multi_prio_rr_tree[i].tm_equiv_prio_list's sp list according to equiv_weight
			list_for_each_entry(exist_tm_pq_node, &exist_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				if(equiv_weight <= exist_tm_pq_node->equiv_weight) {
					list_add_tail(&tm_pq_node->tm_pq_entry_node2, &exist_tm_pq_node->tm_pq_entry_node2);
					is_already_add_weight=1;
					break;
				}
			}

			if(is_already_add_weight ==0) {
				list_add_tail(&tm_pq_node->tm_pq_entry_node2, &exist_equiv_prio_node->tm_pq_list2);
			}
			break;
		}
	}

	if( is_already_add_sp == 0) {
		tm_equiv_prio_node= malloc_safe(sizeof(struct tm_equiv_prio_node_t));
		INIT_LIST_HEAD(&tm_equiv_prio_node->tm_pq_list2);
		tm_equiv_prio_node->equiv_priority=equiv_prio;
		list_add_tail(&tm_equiv_prio_node->tm_equiv_prio_entry_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list);
		list_add_tail(&tm_pq_node->tm_pq_entry_node2, &tm_equiv_prio_node->tm_pq_list2);
	}
	return 0;
}

static int 
tm_multi_prio_rr_tree_add_pq_from_omcitree(struct batchtab_cb_tm_t *tm)
{
	struct tm_tcont_node_t *tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node;
	struct tm_pq_node_t *tm_pq_node;
	int i=0;

	list_for_each_entry(tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {
		dbprintf_bat(LOG_NOTICE, "t-cont:inst:%d,policy:%d, number_of_pq_in_tcont=%d\n", 
			tm_tcont_node->me_tcont->instance_num, tm_tcont_node->policy_tcont, tm_tcont_node->number_of_pq_in_tcont);

		if( tm_tcont_node->number_of_pq_in_tcont == 0)
			continue;

		//keep tm_tcont_node ptr
		tm->multi_prio_rr_tree[i].tm_tcont_node_ptr=tm_tcont_node;
		list_for_each_entry(tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
				tm_multi_prio_rr_tree_add_pq(tm_pq_node, i, tm);
			}
		}
		i++;
	}
	return 0;
}

// hwpq alloc_map related ///////////////////////////////////////////////////////////////

static int 
tm_hwpq_allocmap_alloc_one_phy_pq(struct batchtab_cb_tm_t *tm, int tcont_id, short *schedule_id, short *queue_id, short *phy_queueid)
{
	//for GPON the schedule id is T-CONT
	int group=tcont_id/GPON_TCONT_ID_PER_GROUP;
	int id=tcont_id%GPON_TCONT_ID_PER_GROUP;
	int qid;

	if( group >= CHIP_GPON_TCONT_GROUP) {
		dbprintf_bat(LOG_ERR, "group_id=%d should between 0-3", group);
		return -1;
	}

	//it is ok to reuse schedule_id
	if(tm->hwpq_allocmap[group].phy_schedule_id[id].isused==0) {
		tm->hwpq_allocmap[group].phy_schedule_id[id].isused=1;
		tm->hwpq_allocmap[group].phy_schedule_id_total++;
	}

	//max queue number in each group is GPON_PQ_ID_PER_GROUP
	//found empty phy_pq_id
	for(qid=0; qid < GPON_PQ_ID_PER_GROUP; qid++) {
		if(tm->hwpq_allocmap[group].phy_pq_id[qid].isused==0) {
			dbprintf_bat(LOG_NOTICE, "found free qid=%d\n", qid);
			break;	//found empty 
		}
	}
	if(qid>=GPON_PQ_ID_PER_GROUP) {	//should never happen
		dbprintf_bat(LOG_ERR, "Can't found free qid\n");
		return -1;	//resource can't found
	}

	*schedule_id=tm->hwpq_allocmap[group].phy_schedule_id[id].value;	//return schedule_id
	*queue_id=qid;			//return position of pq id
	*phy_queueid=tm->hwpq_allocmap[group].phy_pq_id[qid].value;		//return position of physical pq
	tm->hwpq_allocmap[group].phy_pq_id[qid].isused=1;

	tm->hwpq_allocmap[group].phy_pq_id_total++;

	dbprintf_bat(LOG_NOTICE, "\t\ttcont_id=%d, {sch_id=%d, queue_id=%d}, phy_queueid=%d\n", 
		tcont_id, *schedule_id, *queue_id, *phy_queueid); 

	return 0;
}

// allocate hw pqs from chip for one logical pq in omci
static int
tm_hwpq_allocmap_alloc_one_omci_pq(struct batchtab_cb_tm_t *tm, int tcont_id, struct tm_equiv_prio_node_t *tm_equiv_prio_node, struct tm_pq_node_t *tm_pq_node, char tm_pq_gem_qos_method)
{
	if (tm_pq_node->me_pq == NULL) {
		dbprintf_bat(LOG_ERR, "\t\ttcont_id=%d, me_pq:null?\n", tcont_id);
		return -1;
	}

	if(tm_equiv_prio_node->is_treatment_as_wrr) {//physical pq use wrr(at least 2 pq)
		//max weight in hardware is 128
		if(tm_equiv_prio_node->sum_of_equiv_weight_for_wrr==0) {
			dbprintf_bat(LOG_ERR, "Error!! weight of wrr is all zero!!\n");
			tm_equiv_prio_node->sum_of_equiv_weight_for_wrr=128;
		}
		//todo: need to check with smartbit
		//separate to green and yellow
		//tm_pq_node->hw_weight=tm_pq_node->hw_weight/2;

		tm_pq_node->hw_weight=(128*tm_pq_node->equiv_weight)/tm_equiv_prio_node->sum_of_equiv_weight_for_wrr;

		dbprintf_bat(LOG_NOTICE, "\t\tWRR me_pq:0x%x, hw_wt=%d\n", tm_pq_node->me_pq->meid, tm_pq_node->hw_weight);
	} else {				//physical pq use sp
		dbprintf_bat(LOG_NOTICE, "\t\tSP me_pq:0x%x\n", tm_pq_node->me_pq->meid);

	}

	if (tm_pq_gem_qos_method== 0) {
		//allocat green pq, targe: {sch XXX, qid XXX}, physical queue id XXX
		if( (tm_hwpq_allocmap_alloc_one_phy_pq(tm, tcont_id, &tm_pq_node->green_q.schedule_id, &tm_pq_node->green_q.queue_id, &tm_pq_node->green_q.phy_queueid) < 0)) {
			dbprintf_bat(LOG_ERR, "q resource alloc fail\n");
			return -1;
		}

	} else if(tm_pq_gem_qos_method==1) {
		//allocat green pq, targe: {sch XXX, qid XXX}, physical queue id XXX
		if( (tm_hwpq_allocmap_alloc_one_phy_pq(tm, tcont_id, &tm_pq_node->green_q.schedule_id, &tm_pq_node->green_q.queue_id, &tm_pq_node->green_q.phy_queueid) < 0)) {
			dbprintf_bat(LOG_ERR, "green q resource alloc fail\n");
			return -1;
		}
		//allocat yellow pq, targe: {sch XXX, qid XXX}, physical queue id XXX
		if( (tm_hwpq_allocmap_alloc_one_phy_pq(tm, tcont_id, &tm_pq_node->yellow_q.schedule_id, &tm_pq_node->yellow_q.queue_id, &tm_pq_node->yellow_q.phy_queueid) < 0)) {
			dbprintf_bat(LOG_ERR, "yellow q resource alloc fail\n");
			return -1;
		}
	} else if(tm_pq_gem_qos_method==2) {
		// todo: for gemflow_q
		int i;
		for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {
			//allocat green pq, targe: {sch XXX, qid XXX}, physical queue id XXX
			if( (tm_hwpq_allocmap_alloc_one_phy_pq(tm, tcont_id, &tm_pq_node->gemport_q[i].schedule_id, &tm_pq_node->gemport_q[i].queue_id, &tm_pq_node->gemport_q[i].phy_queueid) < 0)) {
				dbprintf_bat(LOG_ERR, "queue resource alloc fail for gem meid=[0x%x]\n", tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid);
				return -1;
			}
			dbprintf_bat(LOG_WARNING, "me_gem_sorted_by_cir_pir[%d]=[0x%x],physical_queue_id=%d\n", 
				i, tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid, tm_pq_node->gemport_q[i].phy_queueid);
		}
	}

	return 0;
}

static int 
tm_hwpq_allocmap_set_default(struct batchtab_cb_tm_t *tm)
{
	int i,j,k;
	int phy_schid=0;
	int phy_pqid=0;	//todo maybe initial value=1

	//default tm_hw_reasouce_map
	for(i=0; i < CHIP_GPON_TCONT_GROUP; i++) {
		tm->hwpq_allocmap[i].phy_schedule_id_total=0;
		tm->hwpq_allocmap[i].phy_pq_id_total=0;
		for(j=0; j < GPON_TCONT_ID_PER_GROUP; j++) {
			tm->hwpq_allocmap[i].phy_schedule_id[j].value=phy_schid;
			tm->hwpq_allocmap[i].phy_schedule_id[j].isused=0;
			phy_schid++;
		}
		//max queue number in each group is GPON_PQ_ID_PER_GROUP
		for(k=0; k < GPON_PQ_ID_PER_GROUP; k++) {
			tm->hwpq_allocmap[i].phy_pq_id[k].value=phy_pqid;
			tm->hwpq_allocmap[i].phy_pq_id[k].isused=0;
			phy_pqid++;
		}
	}
	//skip GPON_OMCI_PQ_ID is used by driver
	tm->hwpq_allocmap[GPON_OMCI_PQ_ID/GPON_PQ_ID_PER_GROUP].phy_pq_id[GPON_OMCI_PQ_ID%GPON_PQ_ID_PER_GROUP].isused=1;
	tm->hwpq_allocmap[GPON_OMCI_PQ_ID/GPON_PQ_ID_PER_GROUP].phy_pq_id_total=1;
	return 0;
}

static int 
tm_hwpq_allocmap_check(struct batchtab_cb_tm_t *tm, int tcont_id, unsigned char tcont_num, unsigned char request_pq_num)
{
	//for GPON the schedule id is T-CONT
	int group=tcont_id/GPON_TCONT_ID_PER_GROUP;
	//todo 
	if( group >= CHIP_GPON_TCONT_GROUP) {
		dbprintf_bat(LOG_ERR, "group_id=%d should < %d\n", CHIP_GPON_TCONT_GROUP);
		return -1;
	}

	//max queue number in each group is GPON_PQ_ID_PER_GROUP
	if((tm->hwpq_allocmap[group].phy_pq_id_total + request_pq_num) > GPON_PQ_ID_PER_GROUP ) {
		dbprintf_bat(LOG_ERR, "alloc tcont_id=%d fail, (current pq count in this group=%d, but we request_pq_num=%d)\n", 
			tcont_id, tm->hwpq_allocmap[group].phy_pq_id_total, request_pq_num);
		return -1;	//resource alloc fail
	}

#if 0
	//todo: maybe no need to check
	if((tm->hwpq_allocmap[group].phy_schedule_id_total + tcont_num ) > GPON_TCONT_ID_PER_GROUP ) {
		dbprintf_bat(LOG_NOTICE, "resource alloc phy_schedule_id_total fail (%d, %d)\n", 
			tm->hwpq_allocmap[group].phy_schedule_id_total, tcont_num);
		return -2;	//resource alloc fail
	}
#endif

	return 0; //this group have enough queue to assign
}

static int 
tm_hwpq_allocmap_prepare(struct batchtab_cb_tm_t *tm)
{
	struct tm_pq_node_t *tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node;
	int pos, equiv_prio_pos, sum_of_equiv_weight_for_wrr;
	unsigned char pq_cnt_in_the_same_priority;
	int tcont_id, ret=0, gemport_cnt;
	char tm_pq_gem_qos_method;

	tm_hwpq_allocmap_set_default(tm);

	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {
		int allocid;

		equiv_prio_pos=0;	//0 is the lowest priority in tm_equiv_prio_node

		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;

		//util_fdprintf(fd, "\n\nt-cont:inst:%d, number_of_pq_in_tcont=%d\n", 
		//	tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->me_tcont->instance_num, 
		//	tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->number_of_pq_in_tcont);

		//Note: need { schedule id , queue id} to allocate physical queue id
		//we can get schedule id(tcont_id) by allocation id
		allocid=(unsigned short)meinfo_util_me_attr_data(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->me_tcont, 1);
		//dbprintf(LOG_ERR, "tm_tcont_get_by_allocid get(%d)\n", allocid);
		if (gpon_hw_g.tm_tcont_get_by_allocid== NULL || (ret=gpon_hw_g.tm_tcont_get_by_allocid(allocid, &tcont_id)) != 0) {
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_get_by_allocid get(%d), ret=%d (omci/ploam mismatch?)\n", allocid, ret);
			ret = -1;
			continue;
		}

		if(tcont_id >= GPON_TCONT_ID_TOTAL) {
			dbprintf_bat(LOG_ERR, "Error! tcont_id=%d should between 0-31\n", tcont_id);
			ret = -1;
			continue;
		}

		if( tm_hwpq_allocmap_check(tm, tcont_id, 1, tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->number_of_pq_in_tcont) < 0 ){
			dbprintf_bat(LOG_ERR, "Warning: resource alloc check fail tcont_id=%d\n", tcont_id);
			ret = -1;
			continue;
		}

		tm->multi_prio_rr_tree[pos].tcont_id=tcont_id;	//keep tcont_id

		dbprintf_bat(LOG_NOTICE, "After map to hardware tm->multi_prio_rr_tree:%d, allocid(0x%x)\n", pos, 
			(unsigned short)meinfo_util_me_attr_data(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->me_tcont, 1));

		if( omci_env_g.tm_pq_gem_qos_method == 9) { //auto analysis
			tm_pq_gem_qos_method=tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->tm_pq_gem_qos_method;
		} else {
			tm_pq_gem_qos_method=omci_env_g.tm_pq_gem_qos_method;
		}

		dbprintf_bat(LOG_ERR, "Notice: tcont_id=%d, tm_pq_gem_qos_method=%d\n", tcont_id, tm_pq_gem_qos_method);

		list_for_each_entry_reverse(tm_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {

			if(tm_pq_gem_qos_method==2) {
				pq_cnt_in_the_same_priority=0;
				sum_of_equiv_weight_for_wrr=0;
				list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
					gemport_cnt=me_pq_related_me_gem_sorted(tm_pq_node->me_pq, tm_pq_node->me_gem_sorted_by_cir_pir, TM_PER_PQ_RELATED_FLOW_MAX);
					if( equiv_prio_pos==0 ) {       //lowest priority in tm_equiv_prio_node
						pq_cnt_in_the_same_priority+=gemport_cnt;
						sum_of_equiv_weight_for_wrr+=(tm_pq_node->equiv_weight*gemport_cnt);
					}
				}
				tm_equiv_prio_node->sum_of_equiv_weight_for_wrr=sum_of_equiv_weight_for_wrr;
				dbprintf_bat(LOG_WARNING, "gemport_cnt:%d\n", gemport_cnt);

				list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
					int i;
					for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {
						dbprintf_bat(LOG_WARNING, "me_gem_sorted_by_cir_pir[%d]=[0x%x(%d)]\n", i, tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid, tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid);
					}
				}
			} else {
				pq_cnt_in_the_same_priority=0;
				sum_of_equiv_weight_for_wrr=0;
				//should treatment as WRR ?(should be lowest priority and at least 2 pq in the same priority)
				if( equiv_prio_pos==0 ) {	//lowest priority in tm_equiv_prio_node
					list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
						pq_cnt_in_the_same_priority++;
						sum_of_equiv_weight_for_wrr+=tm_pq_node->equiv_weight;
					}
				}
				tm_equiv_prio_node->sum_of_equiv_weight_for_wrr=sum_of_equiv_weight_for_wrr;
			}

			//only treatment as wrr need different physical queue in each pq, others use the same physical queue
			if( pq_cnt_in_the_same_priority <= 1) {	//wrr need at least 2 pq
				tm_equiv_prio_node->is_treatment_as_wrr=0;
			} else {
				tm_equiv_prio_node->is_treatment_as_wrr=1;
			}

			dbprintf_bat(LOG_NOTICE, "\tequiv_priority:%d is_treatment_as_wrr=%d, sum_of_equiv_weight_for_wrr=%d\n", 
				tm_equiv_prio_node->equiv_priority, tm_equiv_prio_node->is_treatment_as_wrr,
				tm_equiv_prio_node->sum_of_equiv_weight_for_wrr);

			list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				// alloc phy PQs for one logical pq
				tm_hwpq_allocmap_alloc_one_omci_pq(tm, tcont_id, tm_equiv_prio_node, tm_pq_node, tm_pq_gem_qos_method);
			}
			equiv_prio_pos++;
		}
	}
	return ret;
}

// routines for batchtab tm generator ///////////////////////////////////////////////////

int
tm_generator(struct batchtab_cb_tm_t *tm)
{
	if(tm == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, batchtab_cb_tm is NULL\n");
		return 0;
	}

	// omcitree && multi_prio_rr_tree has been released on tm_release()
	// we init omcitree and set default for alloc_map
	INIT_LIST_HEAD(&tm->omcitree);
	tm_hwpq_allocmap_set_default(tm);

#ifndef X86
	tm_omcitree_collect_by_tcont(tm, 0xffff, 1);	//1: collect active_only tcont
#else
	tm_omcitree_collect_by_tcont(tm, 0xffff, 0);	//0: collect all
#endif
	tm_omcitree_calc_pq_prio_weight(tm);

	tm_multi_prio_rr_tree_add_pq_from_omcitree(tm);

	return tm_hwpq_allocmap_prepare(tm);
}

// routines for batchtab tm sync_to_hardware ////////////////////////////////////////////

#if 0
static int 
tm_hwpq_hwsync_state_clear()
{
	int i;

	for(i=0; i < GPON_PQ_ID_TOTAL; i++) {
		memset(&tm->hwpq_hwsync_state[i], 0, sizeof(struct tm_hwpq_hwsync_state_t));
	}

	return 0;
}
#endif
#if 1
int 
tm_sync_to_hardware(struct batchtab_cb_tm_t *tm)
{
	struct tm_tcont_node_t *tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node;
	struct tm_pq_node_t *tm_pq_node;
	struct gpon_tm_tcont_config_t tm_config;
	struct gpon_tm_ts_config_t ts_config;
	struct gpon_tm_pq_config_t tq_config;
	unsigned short meid;
	int ret;

	list_for_each_entry(tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {

		dbprintf_bat(LOG_NOTICE, "t-cont:%d, policy:%d\n", tm_tcont_node->me_tcont->instance_num, tm_tcont_node->policy_tcont);
		
		tm_config.alloc_id=(unsigned short)meinfo_util_me_attr_data(tm_tcont_node->me_tcont, 1);
		tm_config.me_id=tm_tcont_node->me_tcont->meid;
		tm_config.policy=(unsigned short)meinfo_util_me_attr_data(tm_tcont_node->me_tcont, 2);
		
		if (gpon_hw_g.tm_tcont_config_set == NULL ) {
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_config_set NULL(%d-%d)\n", tm_config.me_id,tm_config.alloc_id);
			ret = -1;
			continue;
		}

		ret = gpon_hw_g.tm_tcont_config_set(&tm_config,1);

		if(ret !=0){
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_config_set fail(%d-%d-%d)\n", tm_config.me_id,tm_config.alloc_id,tm_config.policy);
			continue;	
		}

		ret = gpon_hw_g.tm_tcont_config_set(&tm_config,0);

		if(ret !=0){
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_config_set fail(%d-%d-%d)\n", tm_config.me_id,tm_config.alloc_id,tm_config.policy);
			continue;	
		}
		
		list_for_each_entry(tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			//unsigned int sum_of_weight=0;

			meid=tm_ts_node->me_ts->meid;
			ts_config.tcont_id = (unsigned short)meinfo_util_me_attr_data(tm_ts_node->me_ts, 1);
			ts_config.ts_ptr   = (unsigned short)meinfo_util_me_attr_data(tm_ts_node->me_ts, 2);
			ts_config.policy   = (unsigned char)meinfo_util_me_attr_data(tm_ts_node->me_ts, 3);
			ts_config.priority_weight   = (unsigned char)meinfo_util_me_attr_data(tm_ts_node->me_ts, 4);
			
			if(gpon_hw_g.tm_ts_config_set == NULL){
				dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set NULL(%d)\n", meid);
				ret = -1;
				continue;
			}
			//keep for dump
			ret = gpon_hw_g.tm_ts_config_set(meid,&ts_config,1);

			if(ret !=0){
				dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set fail(%d)\n", meid);
				continue;	
			}			

			
			list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {

				memset(&tq_config,0x0,sizeof(struct gpon_tm_pq_config_t));

				meid=tm_pq_node->me_pq->meid;
				tq_config.tcont_id  = (unsigned int)meinfo_util_me_attr_data(tm_pq_node->me_pq, 6);
				tq_config.ts_id  = (unsigned int)meinfo_util_me_attr_data(tm_pq_node->me_pq, 7);
				
				dbprintf_bat(LOG_NOTICE, "pq:%d, (W:%d,P:%d)\n", tm_pq_node->me_pq->classid, tm_pq_node->weight, tm_pq_node->priority);

				if(gpon_hw_g.tm_pq_config_set == NULL){
					dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set NULL(%d)\n", meid);
					ret = -1;
					continue;
				}

				ret = gpon_hw_g.tm_pq_config_set(meid,&tq_config,1);

				if(ret !=0){
					dbprintf_bat(LOG_ERR, "ERROR tm_pq_config_set fail(%d)\n", meid);
					continue;	
				}			
				
			}
		}
	}
	return ret;
}


int
tm_sync_to_hardware_del(struct batchtab_cb_tm_t *tm)
{
	struct tm_tcont_node_t *tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node;
	struct tm_pq_node_t *tm_pq_node;
	struct gpon_tm_tcont_config_t tm_config;
	struct gpon_tm_ts_config_t ts_config;
	struct gpon_tm_pq_config_t tq_config;
	unsigned short meid;
	int ret;

	list_for_each_entry(tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {

		list_for_each_entry(tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			//unsigned int sum_of_weight=0;

			list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {

				memset(&tq_config,0x0,sizeof(struct gpon_tm_pq_config_t));

				meid=tm_pq_node->me_pq->meid;
				tq_config.tcont_id  = (unsigned int)meinfo_util_me_attr_data(tm_pq_node->me_pq, 6);
				tq_config.ts_id  = (unsigned int)meinfo_util_me_attr_data(tm_pq_node->me_pq, 7);

				dbprintf_bat(LOG_NOTICE, "pq:%d, (W:%d,P:%d)\n", tm_pq_node->me_pq->classid, tm_pq_node->weight, tm_pq_node->priority);

				if(gpon_hw_g.tm_pq_config_del == NULL){
					dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set NULL(%d)\n", meid);
					ret = -1;
					continue;
				}

				ret = gpon_hw_g.tm_pq_config_del(meid,&tq_config);

				if(ret !=0){
					dbprintf_bat(LOG_ERR, "ERROR tm_pq_config_set fail(%d)\n", meid);
					continue;
				}

			}


			meid=tm_ts_node->me_ts->meid;
			ts_config.tcont_id = (unsigned short)meinfo_util_me_attr_data(tm_ts_node->me_ts, 1);
			ts_config.ts_ptr   = (unsigned short)meinfo_util_me_attr_data(tm_ts_node->me_ts, 2);
			ts_config.policy   = (unsigned char)meinfo_util_me_attr_data(tm_ts_node->me_ts, 3);
			ts_config.priority_weight   = (unsigned char)meinfo_util_me_attr_data(tm_ts_node->me_ts, 4);

			if(gpon_hw_g.tm_ts_config_del == NULL){
				dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set NULL(%d)\n", meid);
				ret = -1;
				continue;
			}
			//keep for dump
			ret = gpon_hw_g.tm_ts_config_del(meid,&ts_config);

			if(ret !=0){
				dbprintf_bat(LOG_ERR, "ERROR tm_ts_config_set fail(%d)\n", meid);
				continue;
			}


		}

		dbprintf_bat(LOG_NOTICE, "t-cont:%d, policy:%d\n", tm_tcont_node->me_tcont->instance_num, tm_tcont_node->policy_tcont);

		tm_config.alloc_id=(unsigned short)meinfo_util_me_attr_data(tm_tcont_node->me_tcont, 1);
		tm_config.me_id=tm_tcont_node->me_tcont->meid;
		tm_config.policy=(unsigned short)meinfo_util_me_attr_data(tm_tcont_node->me_tcont, 2);

		if (gpon_hw_g.tm_tcont_config_del == NULL ) {
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_config_set NULL(%d-%d)\n", tm_config.me_id,tm_config.alloc_id);
			ret = -1;
			continue;
		}

		ret = gpon_hw_g.tm_tcont_config_del(&tm_config);

		if(ret !=0){
			dbprintf_bat(LOG_ERR, "ERROR tm_tcont_config_set fail(%d-%d-%d)\n", tm_config.me_id,tm_config.alloc_id,tm_config.policy);
			continue;
		}
	}

	return ret;
}
#else
int 
tm_sync_to_hardware(struct batchtab_cb_tm_t *tm)
{
	struct tm_pq_node_t *tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node;
	struct gpon_tm_pq_config_t pq_config, pq_config_y;
	int pos, phy_queueid, ret=0, retvalue=0, i;
	char tm_pq_gem_qos_method;

	if(tm == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, tm is NULL\n");
		return 0;
	}

	/*
	Hope change disable the physical queue and make the minimal side effect
	STEP 1. Each time regenerator change STATE_CURRENT_USE to STATE_LAST_USE
	STEP 2. when "set" change the physical queue's state to STATE_CURRENT_USE
	STEP 3. find the state is STATE_LAST_USE(it means physical queue is used last time, but not used this time)
	   disable the physical queue and change state to STATE_UNUSED
	*/

	//STEP 1.
	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {

		if( tm->hwpq_hwsync_state[phy_queueid].state==STATE_LAST_USE) {
			dbprintf_bat(LOG_ERR, "Error, please check hwpq_hwsync_state because this is impossible to happen\n");
		}

		if( tm->hwpq_hwsync_state[phy_queueid].state==STATE_CURRENT_USE) {
			tm->hwpq_hwsync_state[phy_queueid].state=STATE_LAST_USE;
		}
	}

	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {

		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;

		if( omci_env_g.tm_pq_gem_qos_method == 9) { //auto analysis
			tm_pq_gem_qos_method=tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->tm_pq_gem_qos_method;
		} else {
			tm_pq_gem_qos_method=omci_env_g.tm_pq_gem_qos_method;
		}

		list_for_each_entry_reverse(tm_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {
			list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				if(tm_pq_gem_qos_method==2) {
					for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {
						if( tm_pq_node->gemport_q[i].schedule_id < 0) {
							dbprintf_bat(LOG_WARNING, "tm_pq_node->gemport_q[i].schedule_id < 0 ?\n");
							dbprintf_bat(LOG_WARNING, "{sch_id=%d, queue_id=%d}, phy_queueid=%d, gem meid: 0x%x, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->gemport_q[i].schedule_id, tm_pq_node->gemport_q[i].queue_id, tm_pq_node->gemport_q[i].phy_queueid, 
								tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid, tm_pq_node->me_pq->meid, 
								(pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							continue;
						}

						//get current value from hardware
						pq_config.tcont_id=tm_pq_node->gemport_q[i].schedule_id;
						gpon_hw_g.tm_pq_config_get(tm_pq_node->gemport_q[i].phy_queueid, &pq_config);

						//collect data
						pq_config.enable=1;
						pq_config.tcont_id=tm_pq_node->gemport_q[i].schedule_id;	//set value again

						if( tm_equiv_prio_node->is_treatment_as_wrr >= 1) {//lowest priority with wrr(have at least 2 pq)
							pq_config.wrr_weight=tm_pq_node->hw_weight;
							pq_config.policy=GPON_TM_POLICY_WRR;
						} else {//priority with sp
							pq_config.wrr_weight=0;
							pq_config.policy=GPON_TM_POLICY_SP;
						}

						//write back to hardware
						//workaround wrr with weight 0
						if( pq_config.policy == GPON_TM_POLICY_WRR && tm_pq_node->hw_weight == 0 ) {
							dbprintf_bat(LOG_ERR, "WARNING WRR with weight 0, workaround as 1 {sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->gemport_q[i].schedule_id, tm_pq_node->gemport_q[i].queue_id, tm_pq_node->gemport_q[i].phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							tm_pq_node->hw_weight=1;
							pq_config.wrr_weight=tm_pq_node->hw_weight;
						}

						if( (retvalue=gpon_hw_g.tm_pq_config_set(tm_pq_node->gemport_q[i].phy_queueid, &pq_config)) != 0 ) {
							dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_pq_config_set error return=%d\n", retvalue);
							dbprintf_bat(LOG_ERR, "{sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->gemport_q[i].schedule_id, tm_pq_node->gemport_q[i].queue_id, tm_pq_node->gemport_q[i].phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							ret=-1;
							continue;
						}
						//STEP 2.
						tm->hwpq_hwsync_state[tm_pq_node->gemport_q[i].phy_queueid].state=STATE_CURRENT_USE;
						tm->hwpq_hwsync_state[tm_pq_node->gemport_q[i].phy_queueid].tcont_id=pq_config.tcont_id;
					}//end for
				} else {
					if(tm_pq_gem_qos_method==1) {
						if( tm_pq_node->green_q.schedule_id < 0 || tm_pq_node->yellow_q.schedule_id < 0) {
							dbprintf_bat(LOG_WARNING, "tm_pq_node->green_q.schedule_id or tm_pq_node->yellow_q.schedule_id < 0 ?\n");
							dbprintf_bat(LOG_WARNING, "{sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							dbprintf_bat(LOG_WARNING, "yellow_q {sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->yellow_q.schedule_id, tm_pq_node->yellow_q.queue_id, tm_pq_node->yellow_q.phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							continue;
						}
					} else {
						if( tm_pq_node->green_q.schedule_id < 0) {
							dbprintf_bat(LOG_WARNING, "tm_pq_node->green_q.schedule_id < 0 ?\n");
							dbprintf_bat(LOG_WARNING, "{sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							continue;
						}
					}

					//get current value from hardware
					pq_config.tcont_id=tm_pq_node->green_q.schedule_id;
					gpon_hw_g.tm_pq_config_get(tm_pq_node->green_q.phy_queueid, &pq_config);

					if(tm_pq_gem_qos_method==1) {
						pq_config_y.tcont_id=tm_pq_node->yellow_q.schedule_id;
						gpon_hw_g.tm_pq_config_get(tm_pq_node->yellow_q.phy_queueid, &pq_config_y);
					}

					//collect data
					pq_config.enable=1;
					pq_config.tcont_id=tm_pq_node->green_q.schedule_id;	//set value again

					if(tm_pq_gem_qos_method==1) {
						pq_config_y.enable=1;
						pq_config_y.tcont_id=tm_pq_node->yellow_q.schedule_id;	//set value again
					}

					if( tm_equiv_prio_node->is_treatment_as_wrr >= 1) {//lowest priority with wrr(have at least 2 pq)
						pq_config.wrr_weight=tm_pq_node->hw_weight;
						pq_config.policy=GPON_TM_POLICY_WRR;

						if(tm_pq_gem_qos_method==1) {
							pq_config_y.wrr_weight=tm_pq_node->hw_weight;
							pq_config_y.policy=GPON_TM_POLICY_WRR;
						}
					} else {//priority with sp
						pq_config.wrr_weight=0;
						pq_config.policy=GPON_TM_POLICY_SP;

						if(tm_pq_gem_qos_method==1) {
							pq_config_y.wrr_weight=0;
							pq_config_y.policy=GPON_TM_POLICY_SP;
						}
					}

					//write back to hardware
					//workaround wrr with weight 0
					if( pq_config.policy == GPON_TM_POLICY_WRR && tm_pq_node->hw_weight == 0 ) {
						dbprintf_bat(LOG_ERR, "WARNING WRR with weight 0, workaround as 1 {sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
							tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid, 
							tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
						tm_pq_node->hw_weight=1;
						pq_config.wrr_weight=tm_pq_node->hw_weight;
					}

					if( (retvalue=gpon_hw_g.tm_pq_config_set(tm_pq_node->green_q.phy_queueid, &pq_config)) != 0 ) {
						dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_pq_config_set error return=%d\n", retvalue);
						dbprintf_bat(LOG_ERR, "{sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
							tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid, 
							tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
						ret=-1;
						continue;
					}
					//STEP 2.
					tm->hwpq_hwsync_state[tm_pq_node->green_q.phy_queueid].state=STATE_CURRENT_USE;
					tm->hwpq_hwsync_state[tm_pq_node->green_q.phy_queueid].tcont_id=pq_config.tcont_id;

					if(tm_pq_gem_qos_method==1) {
						if( (retvalue=gpon_hw_g.tm_pq_config_set(tm_pq_node->yellow_q.phy_queueid, &pq_config_y)) != 0 ) {
							dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_pq_config_set error return=%d\n", retvalue);
							dbprintf_bat(LOG_ERR, "yellow_q {sch_id=%d, queue_id=%d}, phy_queueid=%d, pq meid:0x%x, (policy=%s, hw_weight=%d)\n", 
								tm_pq_node->yellow_q.schedule_id, tm_pq_node->yellow_q.queue_id, tm_pq_node->yellow_q.phy_queueid, 
								tm_pq_node->me_pq->meid, (pq_config.policy==GPON_TM_POLICY_SP)?"sp":"wrr", tm_pq_node->hw_weight);
							ret=-1;
							continue;
						}
						//STEP 2_y.
						tm->hwpq_hwsync_state[tm_pq_node->yellow_q.phy_queueid].state=STATE_CURRENT_USE;
						tm->hwpq_hwsync_state[tm_pq_node->yellow_q.phy_queueid].tcont_id=pq_config_y.tcont_id;
					}
				}
			}//end list_for_each_entry
		}//end list_for_each_entry_reverse
	}//end for

	//STEP 3.
	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {

		if( tm->hwpq_hwsync_state[phy_queueid].state==STATE_LAST_USE) {
			tm->hwpq_hwsync_state[phy_queueid].state=STATE_UNUSED;

			pq_config.enable=0;
			pq_config.tcont_id=tm->hwpq_hwsync_state[phy_queueid].tcont_id;

			//disable hardware queue
			if((retvalue=gpon_hw_g.tm_pq_config_set(phy_queueid, &pq_config)) != 0) {
				dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_pq_config_set error return=%d\n", retvalue);
				ret=-1;
			}
		}
	}
	return ret;
}
#endif
// routines for batchtab tm dump ////////////////////////////////////////////////////////

static int
tm_dump_gem_related_td(int fd, struct me_t *me_gem)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me;

	if(me_gem == NULL)
		return 0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\t\t\t\t>[280]gem_td:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			if ((unsigned short)meinfo_util_me_attr_data(me_gem, 5)==me->meid)
				util_fdprintf(fd, ",US\n");
			else
				util_fdprintf(fd, ",DS\n");
		}
	}
	return 0;
}

static int
tm_dump_pq_related_td(int fd, struct me_t *me_pq)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me, *meptr=meinfo_me_get_by_meid(65323, me_pq->meid);

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meptr && me_related(meptr, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\t\t\t>[280]pq_td:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			util_fdprintf(fd, ",US\n");
		}
	}
	return 0;
}

static int 
tm_dump_phy_queue_by_me_tcont_pq(int fd, struct batchtab_cb_tm_t *tm, struct me_t *me_tcont, struct me_t *me_pq)
{
	struct tm_pq_node_t *tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node;
	int pos,i;
	char tm_pq_gem_qos_method;

	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {

		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;
		if (tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->me_tcont->meid != me_tcont->meid)
			continue;

		if( omci_env_g.tm_pq_gem_qos_method == 9) { //auto analysis
			tm_pq_gem_qos_method=tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->tm_pq_gem_qos_method;
		} else {
			tm_pq_gem_qos_method=omci_env_g.tm_pq_gem_qos_method;
		}

		list_for_each_entry_reverse(tm_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {
#if 0
			if(tm_equiv_prio_node->is_treatment_as_wrr) {
				util_fdprintf(fd, "\t\tmake decision use: (sum_of_equiv_weight_for_wrr:0x%x)\n", tm_equiv_prio_node->sum_of_equiv_weight_for_wrr);
			} else {
				util_fdprintf(fd, "\t\tmake decision use: (equiv_priority:0x%x)\n", tm_equiv_prio_node->equiv_priority);
			}
#endif
			list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				if (tm_pq_node->me_pq->meid != me_pq->meid)
					continue;
				if(tm_pq_gem_qos_method==2) {
					//util_fdprintf(fd, "\t\t(equiv_priority=0x%x, equiv_weight=%d)\n", tm_pq_node->equiv_priority, tm_pq_node->equiv_weight);
					for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {

						if( tm_equiv_prio_node->is_treatment_as_wrr >= 1) {//lowest priority with wrr(have at least 2 pq)
							//max weight in hardware is 128
							util_fdprintf(fd, "\t\tpq meid:0x%x, gem meid:0x%x --> wrr PQ weight=%d {sch_id=%d, queue_id=%d}, phy_queueid=%d\n", 
								tm_pq_node->me_pq->meid, tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid, tm_pq_node->hw_weight,
								tm_pq_node->gemport_q[i].schedule_id, tm_pq_node->gemport_q[i].queue_id, tm_pq_node->gemport_q[i].phy_queueid);

						} else {//priority with sp
							util_fdprintf(fd, "\t\tpq meid:0x%x, gem meid:0x%x --> sp PQ {sch_id=%d, queue_id=%d}, phy_queueid=%d\n", 
								tm_pq_node->me_pq->meid, tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid, tm_pq_node->gemport_q[i].schedule_id, 
								tm_pq_node->gemport_q[i].queue_id, tm_pq_node->gemport_q[i].phy_queueid); 
						}
						tm_dump_gem_related_td(fd, tm_pq_node->me_gem_sorted_by_cir_pir[i]);
					}

				} else {
					char *green_q_str = "green PQ";
					char *yellow_q_str = "yellow PQ";
					if(tm_pq_gem_qos_method==0)
						green_q_str = "PQ";

					//util_fdprintf(fd, "\t\t(equiv_priority=0x%x, equiv_weight=%d)", tm_pq_node->equiv_priority, tm_pq_node->equiv_weight);
					if( tm_equiv_prio_node->is_treatment_as_wrr >= 1) {//lowest priority with wrr(have at least 2 pq)
						//max weight in hardware is 128
						util_fdprintf(fd, "\t\tpq meid:0x%x --> wrr %s weight=%d {sch_id=%d, queue_id=%d}, phy_queueid=%d)\n", 
							tm_pq_node->me_pq->meid, green_q_str, tm_pq_node->hw_weight,
							tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid);
						if(tm_pq_gem_qos_method==1) {
							util_fdprintf(fd, "\t\tpq meid:0x%x --> wrr %s weight=%d {sch_id=%d, queue_id=%d}, phy_queueid=%d)\n", 
								tm_pq_node->me_pq->meid, yellow_q_str, tm_pq_node->hw_weight,
								tm_pq_node->yellow_q.schedule_id, tm_pq_node->yellow_q.queue_id, tm_pq_node->yellow_q.phy_queueid);
						}
					} else {//priority with sp
						util_fdprintf(fd, "\t\tpq meid:0x%x --> sp %s {sch_id=%d, queue_id=%d}, phy_queueid=%d)\n", 
							tm_pq_node->me_pq->meid, green_q_str, 
							tm_pq_node->green_q.schedule_id, tm_pq_node->green_q.queue_id, tm_pq_node->green_q.phy_queueid); 
						if(tm_pq_gem_qos_method==1) {
							util_fdprintf(fd, "\t\tpq meid:0x%x --> sp %s {sch_id=%d, queue_id=%d}, phy_queueid=%d)\n", 
								tm_pq_node->me_pq->meid, yellow_q_str, 
								tm_pq_node->yellow_q.schedule_id, tm_pq_node->yellow_q.queue_id, tm_pq_node->yellow_q.phy_queueid); 
						}
					}
				}
			}
		}
	}
	return 0;
}

static int 
tm_dump_by_me_tcont(int fd, struct batchtab_cb_tm_t *tm, struct me_t *me_tcont)
{
	struct tm_tcont_node_t *tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node;
	struct tm_pq_node_t *tm_pq_node;
	unsigned short allocid;
	int tcont_id;

	list_for_each_entry(tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {
		if(tm_tcont_node->me_tcont != me_tcont)
			continue;

		allocid=(unsigned short)meinfo_util_me_attr_data(me_tcont, 1);
		util_fdprintf(fd, "\nTCONT:meid=0x%x allocid=0x%x(%d), number_of_pq_in_tcont=%d\n", 
			tm_tcont_node->me_tcont->meid, allocid, allocid, tm_tcont_node->number_of_pq_in_tcont);

		if (gpon_hw_g.tm_tcont_get_by_allocid(allocid, &tcont_id) !=0 ) {
			util_fdprintf(fd, "Error, allocid=0x%x(%d) can't get tcont_id\n\n", allocid, allocid); 
		} else {
			util_fdprintf(fd, "allocid=0x%x(%d) tcont_id=%d\n", allocid, allocid, tcont_id); 
		}

		list_for_each_entry(tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			char *policy_merge_str="tcont:sp ts:sp";
			switch(tm_ts_node->policy_merge) {
				case TCONT_SP_TS_SP:	policy_merge_str="tcont:sp ts:sp"; break;
				case TCONT_SP_TS_WRR:	policy_merge_str="tcont:sp ts:wrr"; break;
				case TCONT_WRR_TS_SP:	policy_merge_str="tcont:wrr ts:sp"; break;
				case TCONT_WRR_TS_WRR:	policy_merge_str="tcont:wrr ts:wrr"; break;
			}
			util_fdprintf(fd, "\tTS meid=0x%x, %s, prio:%d, weight:%d\n", 
				tm_ts_node->me_ts->meid, policy_merge_str, tm_ts_node->priority, tm_ts_node->weight);
			list_for_each_entry(tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
				// print equiv prio/weight in omcitree
				util_fdprintf(fd, "\t\tpq meid=0x%x, prio:%d, weight:%d --> equiv: prio=%d<<16|%d, weight=%d*%d (Ph=%d,Pl=%d,Wh=%d,Wl=%d)\n", 
					tm_pq_node->me_pq->meid, tm_pq_node->priority, tm_pq_node->weight,
					tm_pq_node->priority_h, tm_pq_node->priority_l, tm_pq_node->weight_h, tm_pq_node->weight_l, 
					tm_pq_node->priority_h, tm_pq_node->priority_l, tm_pq_node->weight_h, tm_pq_node->weight_l);
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0)
					tm_dump_pq_related_td(fd, tm_pq_node->me_pq);
				// print pq-gem mapping to phy queue in multi_prio_rr_tree
				tm_dump_phy_queue_by_me_tcont_pq(fd, tm, tm_tcont_node->me_tcont, tm_pq_node->me_pq);
			}
		}
	}
	return 0;
}

int 
tm_dump(int fd, struct batchtab_cb_tm_t *tm)
{
	int pos;
	if(tm == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, tm is NULL\n");
		return 0;
	}
	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {
		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;
		tm_dump_by_me_tcont(fd, tm, tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->me_tcont);
	}
	return 0;
}

// routines for batchtab tm release /////////////////////////////////////////////////////

int 
tm_release(struct batchtab_cb_tm_t *tm)
{
	struct tm_tcont_node_t *tm_tcont_node, *tmp_tm_tcont_node;
	struct tm_ts_node_t *tm_ts_node, *tmp_tm_ts_node;
	struct tm_pq_node_t *tm_pq_node, *tmp_tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node, *tmp_tm_equiv_prio_node;
	unsigned char pos;

	if(tm == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, tm is NULL\n");
		return 0;
	}

	tm_sync_to_hardware_del(tm);

	// release per tcont multi_prio_rr_tree first
	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {

		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;
		//delete node on tm->multi_prio_rr_tree[i].tm_equiv_prio_list
		list_for_each_entry_safe(tm_equiv_prio_node, tmp_tm_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {

			list_for_each_entry_safe(tm_pq_node, tmp_tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				list_del(&tm_pq_node->tm_pq_entry_node2);
				//it will free in another list (omci similar data stru)
			}
			list_del(&tm_equiv_prio_node->tm_equiv_prio_entry_node);
			free_safe(tm_equiv_prio_node);
		}

		tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr=NULL;
	}

	// release omcitree
	list_for_each_entry_safe(tm_tcont_node, tmp_tm_tcont_node, &tm->omcitree, tm_tcont_entry_node) {
		list_for_each_entry_safe(tm_ts_node, tmp_tm_ts_node, &tm_tcont_node->tm_ts_list, tm_ts_entry_node) {
			list_for_each_entry_safe(tm_pq_node, tmp_tm_pq_node, &tm_ts_node->tm_pq_list, tm_pq_entry_node) {
				list_del(&tm_pq_node->tm_pq_entry_node);
				free_safe(tm_pq_node);
			}
			list_del(&tm_ts_node->tm_ts_entry_node);
			free_safe(tm_ts_node);
		}
		list_del(&tm_tcont_node->tm_tcont_entry_node);
		free_safe(tm_tcont_node);
	}
	return 0;
}

// tm routine called by external functions //////////////////////////////////////////////
// if me_pq is NULL, me_gem will be used to find related me_pq
int 
tm_get_tcont_id_phy_queue_info_by_me_pq_gem(struct me_t *me_pq, struct me_t *me_gem, int is_yellow,
				int *schedule_id, int *phy_queueid, int *is_policy_wrr, int *hw_weight)
{
	struct batchtab_cb_tm_t *tm;
	struct tm_pq_node_t *tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node;
	int pos, i;
	char tm_pq_gem_qos_method;

	tm=(struct batchtab_cb_tm_t *)batchtab_table_data_get("tm");
	if(tm == NULL) {
		return -1;	//error
	}

	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {

		if(tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;

		if( omci_env_g.tm_pq_gem_qos_method == 9) { //auto analysis
			tm_pq_gem_qos_method=tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->tm_pq_gem_qos_method;
		} else {
			tm_pq_gem_qos_method=omci_env_g.tm_pq_gem_qos_method;
		}

		list_for_each_entry_reverse(tm_equiv_prio_node, &tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {
			list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				if (me_pq) {
					if( me_pq != tm_pq_node->me_pq)
						continue;
				} else {
					if (!me_related(me_gem, tm_pq_node->me_pq))
						continue;
				}
				if(tm_pq_gem_qos_method==2) {
					for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {
						if(tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid == me_gem->meid) {
							*phy_queueid=tm_pq_node->gemport_q[i].phy_queueid;
							*schedule_id=tm_pq_node->gemport_q[i].schedule_id;
							*is_policy_wrr=tm_equiv_prio_node->is_treatment_as_wrr;
							*hw_weight=tm_pq_node->hw_weight;
							batchtab_table_data_put("tm");
							return 1;	//found
						}
					}
				} else {
					if(tm_pq_node->green_q.phy_queueid < 0 ) {
						dbprintf_bat(LOG_NOTICE, "Error, get physical pq info\n");
						batchtab_table_data_put("tm");
						return -1;	//fail
					}
					if(tm_pq_gem_qos_method==1 && is_yellow) {
						*schedule_id=tm_pq_node->yellow_q.schedule_id;
						*phy_queueid=tm_pq_node->yellow_q.phy_queueid;
					} else {
						*schedule_id=tm_pq_node->green_q.schedule_id;
						*phy_queueid=tm_pq_node->green_q.phy_queueid;
					}
					*is_policy_wrr=tm_equiv_prio_node->is_treatment_as_wrr;
					*hw_weight=tm_pq_node->hw_weight;
					batchtab_table_data_put("tm");
					return 1;	//ok
				}
			}
		}
	}
	batchtab_table_data_put("tm");
	return 0;	// not found
}

int
tm_get_meid_pq_gem_by_tcont_id_phy_queueid(int schedule_id, int phy_queueid, unsigned short *meid_pq, unsigned short *meid_gem, char *is_yellow)
{
	struct batchtab_cb_tm_t *batchtab_cb_tm;
	struct tm_pq_node_t *tm_pq_node;
	struct tm_equiv_prio_node_t *tm_equiv_prio_node;
	int pos, i;
	char tm_pq_gem_qos_method;

	batchtab_cb_tm=(struct batchtab_cb_tm_t *)batchtab_table_data_get("tm");
	if(batchtab_cb_tm == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, batchtab_cb_tm is NULL\n");
		return -1;
	}

	for(pos=0; pos < GPON_TCONT_ID_TOTAL; pos++) {
		if(batchtab_cb_tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr==NULL)
			continue;
		if( omci_env_g.tm_pq_gem_qos_method == 9) { //auto analysis
			tm_pq_gem_qos_method=batchtab_cb_tm->multi_prio_rr_tree[pos].tm_tcont_node_ptr->tm_pq_gem_qos_method;
		} else {
			tm_pq_gem_qos_method=omci_env_g.tm_pq_gem_qos_method;
		}
		list_for_each_entry_reverse(tm_equiv_prio_node, &batchtab_cb_tm->multi_prio_rr_tree[pos].tm_equiv_prio_list, tm_equiv_prio_entry_node) {
			list_for_each_entry(tm_pq_node, &tm_equiv_prio_node->tm_pq_list2, tm_pq_entry_node2) {
				if(tm_pq_gem_qos_method==2) {
					for(i=0; i < TM_PER_PQ_RELATED_FLOW_MAX && tm_pq_node->me_gem_sorted_by_cir_pir[i]!=NULL; i++) {
						if (phy_queueid == tm_pq_node->gemport_q[i].phy_queueid &&
						    schedule_id == tm_pq_node->gemport_q[i].schedule_id) {
						    	*meid_pq = tm_pq_node->me_pq->meid;
						    	*meid_gem = tm_pq_node->me_gem_sorted_by_cir_pir[i]->meid;
						    	*is_yellow = 0;
							batchtab_table_data_put("tm");
							return 1;	//found
						}
					}
				} else {
					if(tm_pq_node->green_q.phy_queueid < 0 ) {
						dbprintf_bat(LOG_WARNING, "Warning, tm_get_phy_queueid_and_schedule_id_by_me_pq_gem <0?\n");
						batchtab_table_data_put("tm");
						return -1;	//fail
					}
					if (phy_queueid == tm_pq_node->yellow_q.phy_queueid &&
					    schedule_id == tm_pq_node->yellow_q.schedule_id) {
					    	*meid_pq = tm_pq_node->me_pq->meid;
					    	*meid_gem = 0xffff;
					    	*is_yellow = 1;
						batchtab_table_data_put("tm");
						return 1;	//found
					}
					if (phy_queueid == tm_pq_node->green_q.phy_queueid &&
					    schedule_id == tm_pq_node->green_q.schedule_id) {
					    	*meid_pq = tm_pq_node->me_pq->meid;
					    	*meid_gem = 0xffff;
					    	*is_yellow = 0;
						batchtab_table_data_put("tm");
						return 1;	//found
					}
				}
			}
		}
	}
	batchtab_table_data_put("tm");
	return 0;	//not found
}

