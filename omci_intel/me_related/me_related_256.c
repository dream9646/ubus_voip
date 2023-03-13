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
 * File    : me_related_256.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "me_related.h"

//relation check section: 9.1.1 256 ONT_G

//relation to 9.1.3 ONT_DATA
int me_related_256_002(struct me_t *me1, struct me_t *me2)
{
	return me_related_002_256(me2, me1);
}

//relation to 9.1.6 Circuit
int me_related_256_006(struct me_t *me1, struct me_t *me2)
{
	return me_related_006_256(me2, me1);
}

//relation to 9.1.4 Software_image
int me_related_256_007(struct me_t *me1, struct me_t *me2)
{
	return me_related_007_256(me2, me1);
}

//relation to 9.12.2 OLT_G
int me_related_256_131(struct me_t *me1, struct me_t *me2)
{
	return me_related_131_256(me2, me1);
}

//relation to 9.1.7 ONT power shedding
int me_related_256_133(struct me_t *me1, struct me_t *me2)
{
	return me_related_133_256(me2, me1);
}

//relation to 9.1.9 Equipment extension package
int me_related_256_160(struct me_t *me1, struct me_t *me2)
{
	return me_related_160_256(me2, me1);
}

//relation to 9.1.2 ONT2_G
int me_related_256_257(struct me_t *me1, struct me_t *me2)
{
	return me1->meid==me2->meid;
}
