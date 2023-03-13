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
 * File    : me_related_142.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.9.5 [142] VoIP_media_profile

// relation to 9.9.6 [058] Voice_service_profile
int
me_related_142_058(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr1); //voice service profile pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation for 9.9.7 [143] RTP_profile_data
int
me_related_142_143(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 16, &attr1); //rtp profile data
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}
