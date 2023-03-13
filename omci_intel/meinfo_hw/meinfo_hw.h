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
 * Module  : meinfo_hw
 * File    : meinfo_hw.h
 *
 ******************************************************************/

#ifndef __MEINFO_HW_H__
#define __MEINFO_HW_H__

#include <stdio.h>
#include "meinfo.h"
#include "omcimsg.h"

int meinfo_hw_init(void);
int meinfo_hw_register(unsigned short classid, struct meinfo_hardware_t *hw);

#endif //__MEINFO_HW_H__
