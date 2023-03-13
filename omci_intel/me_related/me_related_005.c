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
 * File    : me_related_005.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "util.h"
#include "me_related.h"

//relation check section: 9.1.5 005 Cardholder

//relation to 9.1.6 006 Circuit_pack
int me_related_005_006(struct me_t *me1, struct me_t *me2)
{
	if( me1->meid == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.1.4 007 Software_image
int me_related_005_007(struct me_t *me1, struct me_t *me2)
{
	//005's 2nd meid == 007's 1st meid
	if((me1->meid & 0x00FF) == (me2->meid >> 8))
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.5.1 11 Physical path termination point Ethernet UNI
int 
me_related_005_011(struct me_t *me1, struct me_t *me2)	
{
	unsigned char cardholder_slot=(me1->meid)&0xff;
	unsigned char uni_slot=me2->meid>>8;
	return (uni_slot==cardholder_slot);
}

//relation to 9.3.1 45 MAC bridge service profile
int
me_related_005_045(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
		return 0;

	dbprintf(LOG_WARNING, "Unsure relation, classid1=5, meid1=%d, classid2=45, meid2=%d\n", me1->meid, me2->meid);
	return 1;
}

//relation to 9.2.2 262 T_CONT
int me_related_005_262(struct me_t *me1, struct me_t *me2)
{
	//005's 2nd byte is slot number
	//262's 1st byte is slot id

	if((me1->meid & 0x00FF) == (me2->meid >> 8))
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.2.1 263 ANI_G
int me_related_005_263(struct me_t *me1, struct me_t *me2)
{
	//005's 2nd byte is slot number
	//263's 1st byte is slot id

	if((me1->meid & 0x00FF) == (me2->meid >> 8))
		return 1;	//related
	else
		return 0;	//not related
}

