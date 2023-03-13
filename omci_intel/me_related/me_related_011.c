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
 * File    : me_related_011.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.5.1 [11] Physical path termination point Ethernet UNI

// relation to 9.1.5 [5] Cardholder
int 
me_related_011_005(struct me_t *me1, struct me_t *me2)	
{
	return me_related_005_011(me2, me1);
}

// relation to 9.1.6 [6] Circuit pack
int 
me_related_011_006(struct me_t *me1, struct me_t *me2)	
{
	return me_related_006_011(me2, me1);
}

// relation to 9.5.2 [24] Ethernet performance monitoring history data
int 
me_related_011_024(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}

// relation to 9.3.4 [047] MAC bridge port configuration data
int 
me_related_011_047(struct me_t *me1, struct me_t *me2)	
{
	return me_related_047_011(me2, me1);
}

// relation to 9.12.1 [264] UNI-G
int 
me_related_011_264(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}

// relation to 9.5.4 [296] Ethernet performance monitoring history data 3
int 
me_related_011_296(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}

// relation to 9.3.14 [290] Dot1X port extension package
int 
me_related_011_290(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);

}

// relation to 9.3.34 [425] Ethernet_frame_extended_PM_64-Bit
int 
me_related_011_425(struct me_t *me1, struct me_t *me2)	
{
        return me_related_425_011(me2, me1);
}

// relation to 9.3.32 [334] Ethernet_frame_extended_PM
int 
me_related_011_334(struct me_t *me1, struct me_t *me2)	
{
          return me_related_334_011(me2, me1);
}

// relation to 9.5.6 [349] PoE Control
int 
me_related_011_349(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}

