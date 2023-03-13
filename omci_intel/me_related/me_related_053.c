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
 * File    : me_related_053.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.9.1 [53] Physical_path_termination_point_POTS_UNI

// relation to 9.9.12 [140] Call_control_performance_monitoring_history_data
int
me_related_053_140(struct me_t *me1, struct me_t *me2)
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

// relation to 9.9.11 [141] VoIP_line_status
int
me_related_053_141(struct me_t *me1, struct me_t *me2)
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

// relation to 9.9.13 [144] RTP_performance_monitoring_history_data
int
me_related_053_144(struct me_t *me1, struct me_t *me2)
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

// relation to 9.2.4 [266] GEM_interworking_termination_point
int
me_related_053_266(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr1); //tp pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

//relation to 9.12.6 [273] Threshold_data_1
int
me_related_053_273(struct me_t *me1, struct me_t *me2)
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
me_related_053_274(struct me_t *me1, struct me_t *me2)
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

