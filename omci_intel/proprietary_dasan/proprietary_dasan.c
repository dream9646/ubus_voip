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
 * Module  : proprietary_dasan
 * File    : proprietary_dasan.c
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
#include "proprietary_dasan.h"

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_dasan_240;
extern struct meinfo_hardware_t meinfo_hw_dasan_244;
#endif

static int
proprietary_dasan_me_related_init(void)
{
	return 0;
}

static int
proprietary_dasan_meinfo_cb_init(void)
{
	return 0;
}

#ifndef X86
static int
proprietary_dasan_meinfo_hw_init(void)
{
	meinfo_hw_register(240, &meinfo_hw_dasan_240);
	meinfo_hw_register(244, &meinfo_hw_dasan_244);
	return 0;
}

static int 
proprietary_dasan_er_group_hw_init(void)
{
	/* er_group_hw_240.c */
	er_attr_group_hw_add("self_rogue_detect", er_group_hw_self_rogue_detect);
	/* er_group_hw_244.c */
	er_attr_group_hw_add("self_loop_detect", er_group_hw_self_loop_detect);

	return 0;
}
#endif

int 
proprietary_dasan(void)
{
	proprietary_dasan_me_related_init();	
	proprietary_dasan_meinfo_cb_init();
#ifndef X86
	proprietary_dasan_meinfo_hw_init();
	proprietary_dasan_er_group_hw_init();
#endif

	return 0;
}
