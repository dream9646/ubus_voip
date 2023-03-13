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
 * Module  : meinfo_hw
 * File    : meinfo_hw_6.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"

// 9.1.6 Circuit_pack

/*
struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned char padding[31];
} __attribute__ ((packed));
*/
static int 
meinfo_hw_6_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	// get faked answer from software callback
	return miptr->callback.test_is_supported_cb(me, req);
}

/*
struct omcimsg_test_result_ont_circuit_selftest_t {
	unsigned char unused;
	unsigned char self_test_result;
	unsigned char padding[30];
} __attribute__ ((packed));
*/
static int 
meinfo_hw_6_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	// get faked answer from software callback
	return miptr->callback.test_cb(me, req, result);
}

struct meinfo_hardware_t meinfo_hw_6 = {
	.test_is_supported_hw	= meinfo_hw_6_test_is_supported,
	.test_hw		= meinfo_hw_6_test,
};

