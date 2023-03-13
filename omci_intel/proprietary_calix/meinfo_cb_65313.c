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
 * Module  : proprietary_calix
 * File    : meinfo_cb_65313.c
 *
 ******************************************************************/

#include "meinfo_cb.h"

//classid 65313 CalixVlan

static unsigned short 
meinfo_cb_65313_meid_gen(unsigned short instance_num)
{
	return instance_num;
}

struct meinfo_callback_t meinfo_cb_calix_65313 = {
	.meid_gen_cb	= meinfo_cb_65313_meid_gen,
};
