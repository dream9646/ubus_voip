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
 * File    : meinfo_hw_65315.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "regexp.h"
#include "conv.h"
#include "iphost.h"
#include "metacfg_adapter.h"


static struct me_t *
find_other_related_to_me(unsigned short classid, struct me_t *me)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid);
	struct me_t *me_found;

	list_for_each_entry(me_found, &miptr->me_instance_list, instance_node) {
		if (me_related(me_found, me)) { // me_found -> me
			return me_found;
		}
	}
	return NULL;
}

static struct me_t *
find_me_related_to_other(struct me_t *me, unsigned short classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid);
	struct me_t *me_found;

	list_for_each_entry(me_found, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_found)) { // me_found -> me
			return me_found;
		}
	}
	return NULL;
}


// 9.99.18 65315 CalixRgConfig
static int
meinfo_hw_65315_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *me_157, *me_134, *me_136, *me_150, *me_153, *me_53;

	if ((me_157 = find_me_related_to_other(me, 157)) == NULL)	// large_string
		return 0;	// do nothing, the value in mib will be returned asis

	if ((me_134 = find_other_related_to_me(134, me)) == NULL)	// iphost
		return 0;	// do nothing, the value in mib will be returned asis
	if ((me_136 = find_other_related_to_me(136, me_134)) == NULL)	// tcpudp
		return 0;	// do nothing, the value in mib will be returned asis
	if ((me_150 = find_other_related_to_me(150, me_136)) == NULL)	// agent
		return 0;	// do nothing, the value in mib will be returned asis
	if ((me_153 = find_other_related_to_me(153, me_150)) == NULL)	// sip_user
		return 0;	// do nothing, the value in mib will be returned asis
	if ((me_53 = find_me_related_to_other(me_153, 53)) == NULL)	// pots
		return 0;	// do nothing, the value in mib will be returned asis

#ifdef OMCI_METAFILE_ENABLE
	{
		struct metacfg_t kv_config;
		char key[128], *value;
		snprintf(key, 127, "voip_ep%d_domain_name", me_53->instance_num);
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		if (metacfg_adapter_config_file_load_safe(&kv_config, "/etc/www/metafile.dat") <0) {
			metacfg_adapter_config_release(&kv_config);
			return 0;
		}
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		meinfo_me_copy_to_large_string_me(me_157, value);
		metacfg_adapter_config_release(&kv_config);
	}
#endif

	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65315 = {
	.get_hw		= meinfo_hw_65315_get,
};
