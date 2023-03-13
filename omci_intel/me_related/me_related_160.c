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
 * File    : me_related_160.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "me_related.h"

//relation check section: 9.1.9 160 Equipment extension package

//relation to 9.1.1 ONT_G
int me_related_160_256(struct me_t *me1, struct me_t *me2)
{
	return me1->meid==me2->meid;
}
