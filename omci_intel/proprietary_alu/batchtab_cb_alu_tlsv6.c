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
 * File    : batchtab_cb_alu_tlsv6.c
 *
 ******************************************************************/

/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>



#include "list.h"
#include "batchtab_cb.h"

#include "hwresource.h"

#include "switch.h"
#include "omciutil_misc.h"
#include "omcimsg.h"
#include "tm.h"
*/
#include "meinfo.h"
#include "util.h"
#include "util_run.h"
#include "batchtab.h"

static int alu_tlsv6_mode_g; //0: router, 1: bridge

////////////////////////////////////////////////////

static int
batchtab_cb_alu_tlsv6_init(void)
{
	alu_tlsv6_mode_g = 0;
	return 0;
}

static int
batchtab_cb_alu_tlsv6_finish(void)
{
	alu_tlsv6_mode_g = 0;
	return 0;
}

static int
batchtab_cb_alu_tlsv6_gen(void **table_data)
{
	//traverse bridge port filters of wan to find vlan entry 4095 and operation x00
	struct meinfo_t *miptr84 = meinfo_util_miptr(84);	// vlan tagging filter data
	struct me_t *filter_me;
	struct attr_value_t attr_filter_list={0, NULL};
	struct attr_value_t attr_filter_op={0, NULL};
	struct attr_value_t attr_filter_entry_total={0, NULL};
	unsigned char *bitmap;
	unsigned int i;
	unsigned short tci;	

	alu_tlsv6_mode_g = 0; //default, router mode
	
	list_for_each_entry(filter_me, &miptr84->me_instance_list, instance_node) 
	{
		//check vlan list and operation value
		//retrive tci from filter me
		meinfo_me_attr_get(filter_me, 1, &attr_filter_list);
		meinfo_me_attr_get(filter_me, 2, &attr_filter_op);
		meinfo_me_attr_get(filter_me, 3, &attr_filter_entry_total);

		if ((bitmap=(unsigned char *) attr_filter_list.ptr) &&
			attr_filter_op.data == 0)
		{
			for (i=0; i<attr_filter_entry_total.data; i++)
			{
				tci = util_bitmap_get_value(bitmap, 24*8, i*16, 16);

				if ((tci & 0xfff) == 4095)
				{
					alu_tlsv6_mode_g = 1; //set to bridge mode(TLS)
				}
			}
		}
	}

	*table_data = &alu_tlsv6_mode_g;

	return 0;
}

static int
batchtab_cb_alu_tlsv6_release(void *table_data)
{
	return 0;
}

// the dump info is generated from prev variables, as prev means the state that has been sync to hw
static int
batchtab_cb_alu_tlsv6_dump(int fd, void *table_data)
{
	int *mode = table_data;
	if (table_data)
	{
		util_fdprintf(fd, "[alu_tlsv6] mode = %s\n", *mode ? "1(bridge)" : "0(router)");
	}
	return 0;
}

static int
batchtab_cb_alu_tlsv6_hw_sync(void *table_data)
{
#ifdef OMCI_CMD_DISPATCHER_ENABLE
	int *mode = table_data;
	char cmd[256];

	if (table_data)
	{
		dbprintf_bat(LOG_ERR, "tlsv6 batch table: cmd_dispatcher.sh set ipv6_router_enable=%d\n", *mode ? 0 : 1);
		snprintf(cmd, 255, "/etc/init.d/cmd_dispatcher.sh set ipv6_router_enable=%d", *mode ? 0 : 1);
		util_run_by_thread(cmd, 0);
	}
#endif
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_alu_tlsv6_register(void)
{
	return batchtab_register("alu_tlsv6", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_alu_tlsv6_init,
			batchtab_cb_alu_tlsv6_finish,
			batchtab_cb_alu_tlsv6_gen,
			batchtab_cb_alu_tlsv6_release,
			batchtab_cb_alu_tlsv6_dump,
			batchtab_cb_alu_tlsv6_hw_sync);
}
