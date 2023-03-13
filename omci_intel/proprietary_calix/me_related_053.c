/******************************************************************
 *
 * Copyright (C) 2053 5V Technologies Ltd.
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
 * Module  : proprietary_calix
 * File    : me_related_053.c
 *
 ******************************************************************/

#include "proprietary_calix.h"
#include "me_related.h"

// relation check for 9.9.1 [53] Pptp_POTS_UNI

// relation to 9.99.4 [244] PotsUniExt
int
me_related_053_244(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.4 [65280] PotsUniExt
int
me_related_053_65283(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

