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
 * Module  : proprietary_zte
 * File    : meinfo_hw_247.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "gpon_sw.h"

//classid 247 ONT3-G

static int
meinfo_hw_247_get(struct me_t *me, unsigned char attr_mask[2])
{	
	char password[GPON_PASSWORD_LEN];

	if (util_attr_mask_get_bit(attr_mask, 9)) {	// ONU_password
		struct attr_value_t attr;

		if (gpon_hw_g.onu_password_get!=NULL)
			gpon_hw_g.onu_password_get(password);
		else
			memcpy(password, "0000000000", GPON_PASSWORD_LEN);	// shoudn't happen

		attr.ptr=malloc_safe(GPON_PASSWORD_LEN);
		memcpy(attr.ptr, password, GPON_PASSWORD_LEN);
		meinfo_me_attr_set(me, 9, &attr, 0);
		free_safe(attr.ptr);
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_zte_247 = {
	.get_hw		= meinfo_hw_247_get,
};
