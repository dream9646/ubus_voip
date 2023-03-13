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
 * File    : me_related_425.c
 *
 ******************************************************************/

#include "me_related.h"
#include "util.h"

// relation check for 9.3.34_G.988 [425] Ethernet_frame_extended_PM_64

// relation to 9.5.1 [011] Physical_path_termination_point_Ethernet_UNI
// 425: 2.16) Clause 9.3.34, Ethernet frame extended PM 64-Bit
static int me_related_425_all(struct me_t *me1, struct me_t *me2)
{
        if (me1->classid != 425) return 0;

        if (me2->classid != 46 &&
            me2->classid != 47 &&
            me2->classid != 11 &&
            me2->classid != 98 &&
            me2->classid != 266 &&
            me2->classid != 281 &&
            me2->classid != 329 &&
            me2->classid != 162
            )
                return 0;
                        
        // parent classid of associated port
        int parent_classid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me1, 2))[1]);
        // parent meid of associated port
        int parent_meid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me1, 2))[2]);
        
        return (parent_classid == me2->classid && parent_meid == me2->meid);
}

int
me_related_425_046(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_047(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_011(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_098(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_266(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_281(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}

int
me_related_425_329(struct me_t *me1, struct me_t *me2)
{        
        return me_related_425_all(me1, me2);
}


//relation to 9.12.17 [426] Threshold_data_3
int
me_related_425_426(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

        int threshold_meid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me1, 2))[0]);
	
	if (threshold_meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}
