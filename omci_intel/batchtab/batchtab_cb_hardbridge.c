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
 * File    : batchtab_cb_hardbridge.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "util_run.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_misc.h"
#include "hwresource.h"
#include "switch.h"

// this batchtab is a workaround 
// that trys enables hw mactable aging function on RG platform
//  when mac limiting is enabled and there is no rg service assigned from olt

static struct batchtab_cb_hardbridge_t batchtab_cb_hardbridge_g;

static int
batchtab_cb_hardbridge_init(void)
{
	memset(&batchtab_cb_hardbridge_g, 0x00, sizeof(batchtab_cb_hardbridge_g));
	return 0;
}

static int
batchtab_cb_hardbridge_finish(void)
{
	// turn off hardbridge if it has been enabled
	if (batchtab_cb_hardbridge_g.hardbridge_enable_old) {
		util_run_by_system("/etc/init.d/switch.sh hardbridge_off");
	}	
	return 0;
}

static int
batchtab_cb_hardbridge_gen(void **table_data)
{
	struct batchtab_cb_hardbridge_t *t = &batchtab_cb_hardbridge_g;

	if (!omci_env_g.batchtab_hardbridge_enable) {
		if (batchtab_cb_hardbridge_g.hardbridge_enable_old) {
			batchtab_cb_hardbridge_g.hardbridge_enable = batchtab_cb_hardbridge_g.hardbridge_enable_old = 0;
			util_run_by_system("/etc/init.d/switch.sh hardbridge_off");
		}			
	  	*table_data =t;
		return 0;
	}
#ifdef RG_ENABLE
	struct meinfo_t *miptr47=meinfo_util_miptr(47);
	struct meinfo_t *miptr329=meinfo_util_miptr(329);
	struct me_t *me47, *me329;

	// check maclimit_enable
	t->maclimit_enable = 0;
	list_for_each_entry(me47, &miptr47->me_instance_list, instance_node) {
		if (!me47->is_private) {
			unsigned char bport_type = (unsigned char)meinfo_util_me_attr_data(me47, 3);  
			if (bport_type == 1 || bport_type == 11) {	// 1:uni, 11:veip
				struct me_t *me45 = meinfo_me_get_by_meid(45, (unsigned short)meinfo_util_me_attr_data(me47, 1));
				if (me45) {
					if ((unsigned char)meinfo_util_me_attr_data(me45, 9) > 0 ||	// bridge learning depth > 0
					    (unsigned char)meinfo_util_me_attr_data(me47, 13) > 0) {	// bport learning depth > 0
						t->maclimit_enable = 1;
						break;
					}
				}
			}
		}
	}

	// check has_veip_service
	t->has_veip_service = 0;
	list_for_each_entry(me329, &miptr329->me_instance_list, instance_node) {
		if (!me329->is_private) {
			unsigned char veip_admin_state = (unsigned char)meinfo_util_me_attr_data(me329, 1);	
			if (veip_admin_state == 0 && omciutil_misc_find_me_related_bport(me329)) {
				t->has_veip_service = 1;
				break;
			}
		}
	}
	
	if (t->has_veip_service == 0 && t->maclimit_enable == 1) {
		t->hardbridge_enable = 1;
	} else {
		t->hardbridge_enable = 0;
	}
#endif
	// update pointer for batchtab
	*table_data =t;
	return 0;
}

static int
batchtab_cb_hardbridge_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_hardbridge_dump(int fd, void *table_data)
{
	util_fdprintf(fd, "hardbridge is always off because hwnat is not enabled\n");	

	return 0;
}

static int
batchtab_cb_hardbridge_hw_sync(void *table_data)
{
	struct batchtab_cb_hardbridge_t *t = (struct batchtab_cb_hardbridge_t *)table_data;

	if(t == NULL)
		return 0;
	if (!omci_env_g.batchtab_hardbridge_enable) {
		if (batchtab_cb_hardbridge_g.hardbridge_enable_old) {
			batchtab_cb_hardbridge_g.hardbridge_enable = batchtab_cb_hardbridge_g.hardbridge_enable_old = 0;
			util_run_by_system("/etc/init.d/switch.sh hardbridge_off");
		}			
		return 0;
	}
	
	// call switch.sh to change hardbridge mode if the result differs from the previous one
	if (t->hardbridge_enable != t->hardbridge_enable_old) {
		if (t->hardbridge_enable) {
			util_run_by_system("/etc/init.d/switch.sh hardbridge_on");
		} else {
			util_run_by_system("/etc/init.d/switch.sh hardbridge_off");
		}
		dbprintf_bat(LOG_ERR, "hardbridge is %s\n", t->hardbridge_enable?"on":"off");
		t->hardbridge_enable_old = t->hardbridge_enable;
	}
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_hardbridge_register(void)
{
	return batchtab_register("hardbridge", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_hardbridge_init,
			batchtab_cb_hardbridge_finish,
			batchtab_cb_hardbridge_gen,
			batchtab_cb_hardbridge_release,
			batchtab_cb_hardbridge_dump,
			batchtab_cb_hardbridge_hw_sync);
}

