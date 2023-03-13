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
 * Module  : meinfo_cb
 * File    : meinfo_cb_263.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo.h"
#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"

//classid 263 9.2.1 ANI_G

static int 
meinfo_cb_263_set(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_me_set_arc_timestamp(me, attr_mask, 8, 9);
}

static int 
meinfo_cb_263_is_arc_enabled(struct me_t *me)
{
	return meinfo_me_check_arc_interval(me, 8, 9);
}

/*
struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned char padding[31];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_263_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *) req;

	if (test_req->select_test==7)		// selft test
		return 1;
	if (test_req->select_test>=8 && test_req->select_test<=15)	// vendor test
		return 0;
	
	return 0;
}

/*
struct omcimsg_test_result_ani_linetest_t {
	unsigned char type1;
	unsigned short power_feed_voltage;
	unsigned char type3;
	unsigned short received_optical_power;
	unsigned char type5;
	unsigned short transmitted_optical_power;
	unsigned char type9;
	unsigned short laser_bias_current;
	unsigned char type12;
	unsigned short temperature_degrees;
	unsigned char padding[17];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_263_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *)req;
	struct omcimsg_test_result_ani_linetest_t *test_result =(struct omcimsg_test_result_ani_linetest_t *)result;

	if (test_req->select_test==7) {
		// fill all fields with 0, which means all test type are not supported
		memset(test_result, 0, sizeof(struct omcimsg_test_result_ani_linetest_t));
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_263 = {
	.set_cb				= meinfo_cb_263_set,
	.is_arc_enabled_cb		= meinfo_cb_263_is_arc_enabled,
	.test_is_supported_cb	= meinfo_cb_263_test_is_supported,
	.test_cb				= meinfo_cb_263_test,
};

