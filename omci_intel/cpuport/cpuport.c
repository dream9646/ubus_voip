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
 * Module  : cpuport
 * File    : cpuport.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "list.h"
#include "util.h"
#include "cpuport.h"

struct cpuport_hw_t cpuport_hw_g;

static int
cpuport_hw_empty_func(void)
{
	return 0;
}

static int
cpuport_hw_empty_func_frame_recv_is_available(struct timeval *timeout)
{
	usleep(1000000);
	return 0;
}

int
cpuport_hw_register(struct cpuport_hw_t *cpuport_hw)
{
	void **base = (void**)&cpuport_hw_g;
	int i, total = sizeof(struct cpuport_hw_t)/sizeof(void*);	

	if (cpuport_hw == NULL)
		cpuport_hw = &cpuport_null_g;

	if (cpuport_hw) {
		cpuport_hw_g = *cpuport_hw;
		for (i=0; i<total; i++) {
			if (base[i] == NULL)
				base[i] = (void *)cpuport_hw_empty_func;
		}
	} else {	// wont happen becuase of cpuport_null_g
		for (i=0; i<total; i++)
			base[i] = (void *)cpuport_hw_empty_func;
		cpuport_hw_g.frame_recv_is_available = cpuport_hw_empty_func_frame_recv_is_available;
	}
	return 0;
}

int
cpuport_hw_unregister()
{
	return cpuport_hw_register(NULL);
}
