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
 * File    : meinfo_cb_256.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"
#include "metacfg_adapter.h"

// classid 256 9.1.1 ONT-G

// FIXME, kevin, why using this static variable instead of omci mib to store olt value?
static unsigned char loid_auth_state_256=0;	

static int 
meinfo_cb_256_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	char value[64];
	struct attr_value_t attr;

	if ((miptr=meinfo_util_miptr(256)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	//get sn from hw
	util_attr_mask_set_bit(attr_mask, 3);

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask);	//update attrs

	// onu_loid
	if (util_get_value_from_file(GPON_DAT_FILE, "onu_loid", value, 64)>0) {
		attr.ptr = value;
		meinfo_me_attr_set(me, 10, &attr, 0);
	}

	// onu_load_password
	if (util_get_value_from_file(GPON_DAT_FILE, "onu_loid_password", value, 64)>0) {
		attr.ptr = value;
		meinfo_me_attr_set(me, 11, &attr, 0);
	}

	// Auth State
	attr.data=loid_auth_state_256;
	meinfo_me_attr_set(me, 12, &attr, 0);
	
#ifdef OMCI_METAFILE_ENABLE
	// CHT workaround to switch "ont_mode" via ME256 attribute #6 "Battery_backup"
	if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
	    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
		unsigned char ont_mode = 1; // default is router mode
		struct metacfg_t kv_config;
		char *value_ptr;
		struct attr_value_t attr;
		
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		
		if(metacfg_adapter_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat") !=0)
		{
			dbprintf(LOG_ERR, "load kv_config error\n");
			metacfg_adapter_config_release(&kv_config);
			return -1;
		}
		
		if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, "ont_mode", 1)) > 0)
			ont_mode = util_atoi(value_ptr);
		
		metacfg_adapter_config_release(&kv_config);
		
		attr.data = ont_mode;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
#endif
	
	return 0;
}

static int
meinfo_cb_256_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	
	if (util_attr_mask_get_bit(attr_mask, 12)) {	// Auth State
		attr.data=loid_auth_state_256;
		meinfo_me_attr_set(me, 12, &attr, 0);
	}
	return 0;
}


static int 
meinfo_cb_256_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 12)) {	// Auth State
		struct attr_value_t attr;
		meinfo_me_attr_get(me, 12, &attr);
		loid_auth_state_256=(unsigned char)attr.data;
	}
	return 0;
}

static int
meinfo_cb_256_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 7, &attr, 0);
	
	meinfo_me_set_child_admin_lock(me, 6, lockval);	// circuit pack
	return 0;
}

static int 
meinfo_cb_256_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 7):0;
}

/*
struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned char padding[31];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_256_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *) req;

	if (test_req->select_test==7)		// selft test
		return 1;
	if (test_req->select_test>=8 && test_req->select_test<=15)	// vendor test
		return 0;
	
	return 0;
}

/*
struct omcimsg_test_result_ont_circuit_selftest_t {
	unsigned char unused;
	unsigned char self_test_result;
	unsigned char padding[30];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_256_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *)req;
	struct omcimsg_test_result_ont_circuit_selftest_t *test_result =(struct omcimsg_test_result_ont_circuit_selftest_t *)result;

	if (test_req->select_test==7)	// always return pass for selft test
		test_result->self_test_result=OMCIMSG_TEST_RESULT_SELFTEST_PASSED;
	else
		test_result->self_test_result=OMCIMSG_TEST_RESULT_SELFTEST_FAILED;
	return 0;
}

struct meinfo_callback_t meinfo_cb_256 = {
	.create_cb			= meinfo_cb_256_create,
	.get_cb				= meinfo_cb_256_get,
	.set_cb				= meinfo_cb_256_set,
	.set_admin_lock_cb	= meinfo_cb_256_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_256_is_admin_locked,
	.test_is_supported_cb	= meinfo_cb_256_test_is_supported,
	.test_cb		= meinfo_cb_256_test,
};

