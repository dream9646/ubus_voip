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
 * Module  : switch
 * File    : switch.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "meinfo.h"
#include "hwresource.h"
#include "me_related.h"
#include "util.h"
#include "conv.h"
#include "gpon_sw.h"

struct gpon_hw_t gpon_hw_g = {0};

static int
gpon_hw_empty_func(void)
{
	return 0;
}
int
gpon_null_onu_serial_number_get(char *vendor_id, unsigned int *serial_number)
{
	memcpy(vendor_id, "FVTG", 4);
	*serial_number = 0x27888118;
	return 0;
}

int gpon_null_onu_id_get(unsigned short *id) 
{
	*id = 123;
	return 0;
}

int gpon_null_onu_state_get(unsigned int *state)
{
	*state = 5;
	return 0;
}

struct gpon_tm_pq_config_t null_pq_config[128];

int
gpon_null_tm_tcont_get_by_allocid(int allocid, int *tcont_id)
{
	if (allocid!=255) {
		int seq = allocid % 32;
		*tcont_id = (seq % 4) * 8  + seq / 4;
		return 0;
	}
	*tcont_id = -1;
	return -1;
}
int
gpon_null_tm_pq_config_get(int pq_id, struct gpon_tm_pq_config_t *pq_config)
{
	*pq_config = null_pq_config[pq_id%128];
	return 0;
}

int
gpon_null_tm_pq_config_set(int pq_id, struct gpon_tm_pq_config_t *pq_config,int create_only)
{
	null_pq_config[pq_id%128] = *pq_config;
	return 0;
}

int
gpon_hw_register(struct gpon_hw_t *gpon_hw)
{
	void **base = (void**)&gpon_hw_g;
	int i, total = sizeof(struct gpon_hw_t)/sizeof(void*);	

	if (gpon_hw) {
		gpon_hw_g = *gpon_hw;
		for (i=0; i<total; i++) {
			if (base[i] == NULL)
				base[i] = (void *)gpon_hw_empty_func;
		}
	} else {
		for (i=0; i<total; i++)
			base[i] = (void *)gpon_hw_empty_func;
		// null gpon specific stru & func
		for (i=0; i<128; i++) 
			memset(&(null_pq_config[i]), 0, sizeof(struct gpon_tm_pq_config_t));

		gpon_hw_g.onu_serial_number_get = gpon_null_onu_serial_number_get;
		gpon_hw_g.onu_id_get = gpon_null_onu_id_get;
		gpon_hw_g.onu_state_get = gpon_null_onu_state_get;
		gpon_hw_g.tm_tcont_get_by_allocid = gpon_null_tm_tcont_get_by_allocid;
		gpon_hw_g.tm_pq_config_get = gpon_null_tm_pq_config_get;
		gpon_hw_g.tm_pq_config_set = gpon_null_tm_pq_config_set;
	}
	return 0;
}

int
gpon_hw_unregister()
{
	return gpon_hw_register(NULL);
}
