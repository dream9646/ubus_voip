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
 * File    : me_related_045.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.1 [045] MAC bridge service profile

// relation to 9.1.5 [005] Cardholder
int
me_related_045_005(struct me_t *me1, struct me_t *me2)
{
	return me_related_005_045(me2, me1);
}

// relation to 9.1.6 [005] Circuit pack
int
me_related_045_006(struct me_t *me1, struct me_t *me2)
{
	return me_related_006_045(me2, me1);
}

// relation to 9.1.6 [047] MAC bridge port configuration data
int
me_related_045_047(struct me_t *me1, struct me_t *me2)
{
	return me_related_047_045(me2, me1);
}

// relation to 9.3.2 [046] MAC bridge configuration data
int
me_related_045_046(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.3 [051] MAC bridge performance monitoring history data
int
me_related_045_051(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.18 [298] Dot1_rate_limiter (after G.984.4 amd2)
int 
me_related_045_298(struct me_t *me1, struct me_t *me2)
{
	return me_related_298_045(me2, me1);
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int
me_related_045_130(struct me_t *me1, struct me_t *me2)
{
	return me_related_130_045(me2, me1);
}

// relation to 9.3.21 [301] Dot1ag default MD level
int
me_related_045_301(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.25 [305] Dot1ag CFM stack
int
me_related_045_305(struct me_t *me1, struct me_t *me2)
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
