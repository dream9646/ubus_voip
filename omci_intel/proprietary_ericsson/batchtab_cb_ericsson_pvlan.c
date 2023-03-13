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
 * Module  : proprietary_ericsson
 * File    : batchtab_cb_ericsson_pvlan.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "util.h"
#include "switch.h"
#include "hwresource.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_ericsson.h"

unsigned int ericsson_pvlan_g[SWITCH_LOGICAL_PORT_TOTAL] = {0};

////////////////////////////////////////////////////

static int
batchtab_cb_ericsson_pvlan_init(void)
{
	memset(ericsson_pvlan_g, 0, sizeof(unsigned int)*SWITCH_LOGICAL_PORT_TOTAL);
	return 0;
}

static int
batchtab_cb_ericsson_pvlan_finish(void)
{
	memset(ericsson_pvlan_g, 0, sizeof(unsigned int)*SWITCH_LOGICAL_PORT_TOTAL);
	return 0;
}

static int
batchtab_cb_ericsson_pvlan_gen(void **table_data)
{
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) != 0)
		return 0;
	
	// for ericsson case, fill uni me78 vlan info from the first uni me84 vlan
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON)
	{
		struct meinfo_t *miptr47 = meinfo_util_miptr(47);
		struct me_t *me47 = NULL;
		list_for_each_entry(me47, &miptr47->me_instance_list, instance_node) {
			// 47 --> 84
			struct me_t *me84 = meinfo_me_get_by_meid(84, me47->meid);
			unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(me47, 3);
			if(me84 != NULL && tp_type == 1) { // uni only
				struct attr_value_t attr_filter_list = {0, NULL};
				unsigned char num = (unsigned char)meinfo_util_me_attr_data(me84, 3);
				meinfo_me_attr_get(me84, 1, &attr_filter_list);
				unsigned char *bitmap=(unsigned char *) attr_filter_list.ptr;
				if(num && bitmap) {
					unsigned short tci = util_bitmap_get_value(bitmap, 24*8, 0, 16);
					struct me_t *me11 = NULL;
					struct meinfo_t *miptr11 = meinfo_util_miptr(11);
					list_for_each_entry(me11, &miptr11->me_instance_list, instance_node) {
						// 47 --> 11
						if (me_related(me11, me47)) {
							// 11 --> 78
							struct me_t *me78 =  meinfo_me_get_by_meid(78, me11->meid);
							struct me_t *ipptp = hwresource_public2private(me11);
							unsigned char port_id = meinfo_util_me_attr_data(ipptp, 4);
							if(me78 != NULL) {
								struct attr_value_t attr = {0, NULL};
								attr.ptr = NULL;
								attr.data = 1; // upstream add tag
								meinfo_me_attr_set(me78, 1, &attr, 0);
								attr.ptr = malloc_safe(2);
								attr.data = 0;
								// fill the first vlan from me84 to me78
								util_bitmap_set_value(attr.ptr, 2*8, 0, 16, tci);
								meinfo_me_attr_set(me78, 2, &attr, 0);
								if (attr.ptr) free_safe(attr.ptr);
								attr.ptr = NULL;
								attr.data = 1; // downstream strip tag
								meinfo_me_attr_set(me78, 3, &attr, 0);
								ericsson_pvlan_g[port_id] = tci;
							}
							break;
						}
					}
					free_safe(bitmap);
				}
			}
		}
	}
	*table_data = &ericsson_pvlan_g;
	return 0;
}

static int
batchtab_cb_ericsson_pvlan_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_ericsson_pvlan_dump(int fd, void *table_data)
{
	int i;
	
	for(i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if(ericsson_pvlan_g[i])
			util_fdprintf(fd, "port=%d vlan=%d\n", i, ericsson_pvlan_g[i]);
	}
	return 0;
}

static int
batchtab_cb_ericsson_pvlan_hw_sync(void *table_data)
{
	batchtab_crosstab_update("classf");
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_ericsson_pvlan_register(void)
{
	return batchtab_register("ericsson_pvlan", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_ericsson_pvlan_init,
		batchtab_cb_ericsson_pvlan_finish,
		batchtab_cb_ericsson_pvlan_gen,
		batchtab_cb_ericsson_pvlan_release,
		batchtab_cb_ericsson_pvlan_dump,
		batchtab_cb_ericsson_pvlan_hw_sync);
}
