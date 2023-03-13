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
 * Module  : igmp
 * File    : platform.h
 *
 ******************************************************************/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "hwresource.h"

#define THREAD_NAME	"omci-IGMP"
#define THREAD_ENV	omci_env_g

#define M_SET_SRC_PORTID(x, y)	x=y->rx_desc.bridge_port_me->meid;
#define M_HAS_SRC_PORTID(pkt)	(pkt->rx_desc.bridge_port_me != NULL)

// must be defined
int module_filter_specific_add();
int module_counter_specific_add();
void env_to_igmp_env();

#endif
