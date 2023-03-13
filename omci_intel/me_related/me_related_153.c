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
 * File    : me_related_153.c
 *
 ******************************************************************/

#include "me_related.h"
#include <stdio.h>

// relation check for 9.9.2 [153] SIP_user_data

// relation to 9.9.1 [53] Physical_path_termination_point_POTS_UNI
int
me_related_153_053(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 10, &attr1); //pptp pointer

	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.10 [145] Network_dial_plan_table
int
me_related_153_145(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 7, &attr1); //dial plan table pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.8 [146] VoIP_application_service_profile
int
me_related_153_146(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 8, &attr1); //application service profile pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.9 [147] VoIP_feature_access_codes
int
me_related_153_147(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 9, &attr1); //feature code
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.3 [150] SIP_agent_config_data
int
me_related_153_150(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //sip agent config data
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

