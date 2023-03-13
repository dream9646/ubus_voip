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
 * Module  : me_related
 * File    : me_related_272.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "me_related.h"

//relation check section: 9.2.7 272 GAL_Ethernet_profile

//relation to 9.2.8 276 GAL_Ethernet_performance_monitoring_history_data
int me_related_272_276(struct me_t *me1, struct me_t *me2)
{
	if( me1->meid == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}
