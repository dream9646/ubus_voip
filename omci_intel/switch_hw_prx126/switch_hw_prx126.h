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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126.h
 *
 ******************************************************************/

#ifndef __SWITCH_HW_PRX126_H__
#define __SWITCH_HW_PRX126_H__

#include "chipdef.h"
#include "switch.h"

#define	DEFAULT_FID	0


#define FVT2510_PHY_REG0_LOOPBACK 14
#define FVT2510_PHY_REG0_POWERDOWN 11
#define FVT2510_PHY_REG0_ISOLATE 10
#define FVT2510_PHY_REG0_DUPLEX 8
#define FVT2510_QUEUE_RATE_METER_START_IDX 20

extern struct switch_hw_t switch_hw_prx126_g;
extern int acl_free_num_in_default;

#include "switch_hw_prx126_proto.h"
#endif	//__SWITCH_HW_PRX126_H__

