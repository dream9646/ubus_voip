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
 * Module  : proprietary_huawei
 * File    : proprietary_huawei.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "meinfo_cb.h"
#include "er_group.h"
#ifndef X86
#include "meinfo_hw.h"
#include "er_group_hw.h"
#endif
#include "proprietary_huawei.h"

extern struct meinfo_callback_t meinfo_cb_huawei_351;
extern struct meinfo_callback_t meinfo_cb_huawei_65530;

static int
proprietary_huawei_me_related_init(void)
{
	return 0;
}

static int
proprietary_huawei_meinfo_cb_init(void)
{
	meinfo_cb_register(351, &meinfo_cb_huawei_351);
	meinfo_cb_register(65530, &meinfo_cb_huawei_65530);
	
	return 0;
}

#ifndef X86
static int
proprietary_huawei_meinfo_hw_init(void)
{
	return 0;
}

static int 
proprietary_huawei_er_group_hw_init(void)
{
	/* er_group_hw_171.c */
	er_attr_group_hw_add("ignore_vlan_drop_rule", er_group_hw_ignore_vlan_drop_rule);
	er_attr_group_hw_add("l2s_tags_create_add01_multicast", er_group_hw_l2s_tags_create_add01_multicast);

	return 0;
}
#endif

int 
proprietary_huawei(void)
{
	proprietary_huawei_me_related_init();	
	proprietary_huawei_meinfo_cb_init();
#ifndef X86
	proprietary_huawei_meinfo_hw_init();
	proprietary_huawei_er_group_hw_init();
#endif

	return 0;
}

// export global through function ////////////////////////////
// voip wan: 0-iphost for hsi, 1-iphost-only for voip
int
proprietary_huawei_get_voip_wan(struct me_t *me)
{
	struct meinfo_t *miptr=meinfo_util_miptr(136);
	struct me_t *meptr = NULL;
	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if(me_related(meptr, me)) {
			return 1;
		}
	}
	return 0;
}
