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
 * File    : batchtab_cb_alu_tls.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_alu.h"
#ifndef X86
#include "switch.h"
#endif

int proprietary_alu_tls = 0;

////////////////////////////////////////////////////

void
proprietary_alu_tls_mode(unsigned int enable)
{
#ifndef X86
	if(enable) {
		util_run_by_system("switch.sh tls");

		// Flush IPMC table
		if(switch_hw_g.ipmcastaddr_flush != NULL) 
			switch_hw_g.ipmcastaddr_flush();
	} else {
		if(proprietary_alu_get_ont_mode() == 0) { // bridge mode
			util_run_by_system("switch.sh bridge");
		} else { // router mode
			util_run_by_system("switch.sh router");
		}
	}
#endif
}

static int
batchtab_cb_alu_tls_init(void)
{
	proprietary_alu_tls = 0;
	return 0;
}

static int
batchtab_cb_alu_tls_finish(void)
{
	proprietary_alu_tls = 0;
	return 0;
}

static int
batchtab_cb_alu_tls_gen(void **table_data)
{
	struct meinfo_t *miptr = meinfo_util_miptr(171);
	struct attr_table_header_t *table_header = NULL;	
	struct attr_table_entry_t *table_entry_pos;
	unsigned short filter_inner_vid = 0xFFFF;
	//unsigned short treatment_inner_vid = 0xFFFF;
	//unsigned char filter_inner_pbit = 0xFF, treatment_inner_pbit = 0xFF;
	struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);

	if(meptr && meinfo_util_me_attr_data(meptr, 1) != 1) // Skip this workaround when ME65302 RF_Filter!=1
		return 0;

	proprietary_alu_tls = 0;

	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr, 6)) == NULL)
			continue;
		
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				filter_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 36, 13);
				//filter_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 32, 4);
				//treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 112, 13);
				//treatment_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 108, 4);
				
				if(filter_inner_vid == 4085) { // TLS VID
					proprietary_alu_tls = 1;
					break;
				}
			}
		}
		if(proprietary_alu_tls) break;
	}

	*table_data = &proprietary_alu_tls;
	
	return 0;
}

static int
batchtab_cb_alu_tls_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_alu_tls_dump(int fd, void *table_data)
{
	int *total = table_data;
	
	util_fdprintf(fd, "[alu_tls] found = %d\n", *total);
	return 0;
}

static int
batchtab_cb_alu_tls_hw_sync(void *table_data)
{
	batchtab_omci_update("mcastmode");
	proprietary_alu_tls_mode(proprietary_alu_tls);
	
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_alu_tls_register(void)
{
	return batchtab_register("alu_tls", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_alu_tls_init,
		batchtab_cb_alu_tls_finish,
		batchtab_cb_alu_tls_gen,
		batchtab_cb_alu_tls_release,
		batchtab_cb_alu_tls_dump,
		batchtab_cb_alu_tls_hw_sync);
}

