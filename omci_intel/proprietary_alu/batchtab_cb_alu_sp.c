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
 * Module  : proprietary_alu
 * File    : batchtab_cb_alu_sp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_alu.h"

static int alu_sp_total_g = 0;

////////////////////////////////////////////////////

static int
batchtab_cb_alu_sp_init(void)
{
	alu_sp_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_sp_finish(void)
{
	alu_sp_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_sp_gen(void **table_data)
{
	struct meinfo_t *miptr = meinfo_util_miptr(278);
	struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);
	
	if(proprietary_alu_get_olt_version() != 0) // Skip this workaround when ALU OLT isn't FTTU (7342)
		return 0;
	
	if(meptr && meinfo_util_me_attr_data(meptr, 1) != 0) // Skip this workaround when ME65302 RF_Filter!=0
		return 0;

	alu_sp_total_g = 0;
	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		struct meinfo_t *miptr_pq = meinfo_util_miptr(277);
		struct me_t *meptr_pq = NULL;
		list_for_each_entry(meptr_pq, &miptr_pq->me_instance_list, instance_node) {
			if (me_related(meptr_pq, meptr)) {
				struct attr_value_t attr6 = {0, NULL}, attr8 = {0, NULL};
				unsigned char attr_mask[2]={0, 0};
				int priority = 0;
				meinfo_me_attr_get(meptr_pq, 6, &attr6);
				meinfo_me_attr_get(meptr_pq, 8, &attr8); // expect 1~8 as priority
				if(((unsigned char) attr8.data)==0) priority=7;
				else if(((unsigned char) attr8.data)>8) priority=0;
				else priority=(8-((unsigned char) attr8.data));
				util_bitmap_set_value(attr6.ptr, 32, 16, 16, priority);
				meinfo_me_attr_set(meptr_pq, 6, &attr6, 0);
				util_attr_mask_set_bit(attr_mask, 6);
				meinfo_me_flush(meptr_pq, attr_mask);
				alu_sp_total_g++;
			}
		}
	}
	*table_data = &alu_sp_total_g;
	
	return 0;
}

static int
batchtab_cb_alu_sp_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_alu_sp_dump(int fd, void *table_data)
{
	int *total = table_data;
	
	util_fdprintf(fd, "[alu_sp] found = %d\n", (table_data) ? *total : 0);
	return 0;
}

static int
batchtab_cb_alu_sp_hw_sync(void *table_data)
{
	batchtab_omci_update("tm");
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_alu_sp_register(void)
{
	return batchtab_register("alu_sp", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_alu_sp_init,
		batchtab_cb_alu_sp_finish,
		batchtab_cb_alu_sp_gen,
		batchtab_cb_alu_sp_release,
		batchtab_cb_alu_sp_dump,
		batchtab_cb_alu_sp_hw_sync);
}

