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
 * File    : me_related_244.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for 9.99.4 [244] PotsUniExt

// relation to 9.9.1 [053] Pptp_POTS_UNI
int
me_related_244_053(struct me_t *me1, struct me_t *me2)
{
	return me_related_053_244(me2, me1);
}

