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
 * Module  : proprietary_zte
 * File    : proprietary_zte.c
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
#include "proprietary_zte.h"
#include "util.h"

extern struct meinfo_callback_t meinfo_cb_zte_65530;
extern struct meinfo_callback_t meinfo_cb_zte_249;
extern struct meinfo_callback_t meinfo_cb_zte_250;
extern struct meinfo_callback_t meinfo_cb_zte_253;

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_zte_247;
#endif

static int
proprietary_zte_me_related_init(void)
{
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 65282, me_related_047_65282);
	me_related_add(ME_RELATED_FUNC_SPEC, 65282, 47, me_related_65282_047);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 249, me_related_047_249);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 47, me_related_249_047);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 245, me_related_011_245);
	me_related_add(ME_RELATED_FUNC_SPEC, 245, 11, me_related_245_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 253, me_related_011_253);
	me_related_add(ME_RELATED_FUNC_SPEC, 253, 11, me_related_253_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 134, 250, me_related_134_250);
	me_related_add(ME_RELATED_FUNC_SPEC, 250, 134, me_related_250_134);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 134, 65321, me_related_134_65321);
	me_related_add(ME_RELATED_FUNC_SPEC, 65321, 134, me_related_65321_134);
	
	return 0;
}

static int
proprietary_zte_meinfo_cb_init(void)
{
	meinfo_cb_register(65530, &meinfo_cb_zte_65530);
	meinfo_cb_register(249, &meinfo_cb_zte_249);
	meinfo_cb_register(250, &meinfo_cb_zte_250);
	meinfo_cb_register(253, &meinfo_cb_zte_253);
	
	return 0;
}

#ifndef X86
static int
proprietary_zte_meinfo_hw_init(void)
{
	meinfo_hw_register(247, &meinfo_hw_zte_247);	

	return 0;
}

static int 
proprietary_zte_er_group_hw_init(void)
{
	/* er_group_hw_47_65282.c */
	er_attr_group_hw_add("l2s_tags_create_add01_multicast", er_group_hw_l2s_tags_create_add01_multicast);

	/* er_group_hw_247.c */
	er_attr_group_hw_add("loopdetect_time", er_group_hw_loopdetect_time);
	er_attr_group_hw_add("onu_password", er_group_hw_onu_password);
	
	/* er_group_hw_11_245.c */
	er_attr_group_hw_add("uni_traffic_limit", er_group_hw_uni_traffic_limit);

	/* er_group_hw_134_250_65321.c */
	er_attr_group_hw_add("pppoe_wan_setting", er_group_hw_pppoe_wan_setting);
	
	/* er_group_hw_253.c */
	er_attr_group_hw_add("broadcast_rate", er_group_hw_broadcast_rate);
	er_attr_group_hw_add("port_dhcp", er_group_hw_port_dhcp);

	/* er_group_hw_47_249.c */
//	er_attr_group_hw_add("extended_l2s_filt_configure", er_group_hw_extended_l2s_filt_configure);
//	er_attr_group_hw_add("extended_l2s_filt_create_vid_tci_pri", er_group_hw_extended_l2s_filt_create_vid_tci_pri);

	return 0;
}
#endif

int 
proprietary_zte(void)
{
	proprietary_zte_me_related_init();	
	proprietary_zte_meinfo_cb_init();
#ifndef X86
	proprietary_zte_meinfo_hw_init();
	proprietary_zte_er_group_hw_init();
#endif

	return 0;
}

// export global through function ////////////////////////////
// wan mode: 0-iphost-only, 1-non-pppoe, 2-pppoe
int
proprietary_zte_get_wan_mode(struct me_t *me)
{
	struct meinfo_t *miptr=meinfo_util_miptr(65321);
	struct me_t *meptr = NULL;
	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if(me_related(meptr, me)) {
			struct me_t *meptr = meinfo_me_get_by_meid(250, me->meid);
			return (meptr!=NULL) ? 2 : 1;
		}
	}
	return 0;
}
