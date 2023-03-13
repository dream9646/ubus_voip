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
 * File    : me_related_150.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.9.3 [150] SIP_agent_config_data

// relation to 9.4.14 [136] TCP/UDP_config_data
int
me_related_150_136(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 5, &attr); //pointer to tcp/udp config data
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}


// relation to 9.9.14 [151] SIP_agent_performance_monitoring_history_data
int
me_related_150_151(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.15 [152] SIP_call_initiation_performance_monitoring_history_data
int
me_related_150_152(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

//relation to 9.12.6 [273] Threshold_data_1
int
me_related_150_273(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr); //threshold data 1/2 id
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

//relation to 9.12.7 [274] Threshold_data_2
int
me_related_150_274(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr); //threshold data 1/2 id
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

