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
 * Module  : switch_hw_fvt2510
 * File    : switch_hw_fvt2510_hook.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "switch.h"
#include "env.h"

int
switch_null_hw_version_get(unsigned int *family, unsigned int *version, unsigned int *rev, unsigned int *subtype)
{
	*family = 0x2788;
	*version = 0x00;
	*rev = 0x81;
	*subtype = 0x18;
	return 0;
}

struct switch_hw_t switch_null_g = {  
	/* switch_null.c */
	.hw_version_get = switch_null_hw_version_get, 
};
