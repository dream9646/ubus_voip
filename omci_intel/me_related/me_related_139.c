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
 * File    : me_related_139.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.9.4 [139] VoIP_voice_CTP

// relation to 9.9.2 [153] SIP_user_data
int
me_related_139_153(struct me_t *me1, struct me_t *me2)
{
	struct me_t *voip_config_data; //class 138
	struct attr_value_t attr1, attr2;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if ((voip_config_data = meinfo_me_get_by_instance_num(138, 0)) == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(voip_config_data, 2, &attr1); //user protocol used
	meinfo_me_attr_get(me1, 1, &attr2); //user protocol pointer
	
	if ((unsigned short)(attr1.data) == 1 && //SIP
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.1 [053] Physical_path_termination_point_POTS_UNI
int
me_related_139_053(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr1); //pptp pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.9.5 [142] VoIP_media_profile
int
me_related_139_142(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //voip media profile pointer
	
	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}
