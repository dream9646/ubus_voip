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
 * File    : proprietary_ericsson.c
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
#include "proprietary_ericsson.h"

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_ericsson_250;
#endif

static int
proprietary_ericsson_me_related_init(void)
{
	return 0;
}

static int
proprietary_ericsson_meinfo_cb_init(void)
{
	return 0;
}

#ifndef X86
static int
proprietary_ericsson_meinfo_hw_init(void)
{
	meinfo_hw_register(250, &meinfo_hw_ericsson_250);
	return 0;
}

static int 
proprietary_ericsson_er_group_hw_init(void)
{
	return 0;
}
#endif

int 
proprietary_ericsson(void)
{
	proprietary_ericsson_me_related_init();	
	proprietary_ericsson_meinfo_cb_init();
#ifndef X86
	proprietary_ericsson_meinfo_hw_init();
	proprietary_ericsson_er_group_hw_init();
#endif
	return 0;
}
