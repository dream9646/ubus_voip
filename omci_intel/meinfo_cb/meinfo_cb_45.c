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
 * File    : meinfo_cb_45.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 45 9.3.2 MAC bridge configuration data

static int 
meinfo_cb_45_create(struct me_t *me, unsigned char attr_mask[2])
{
	batchtab_omci_update("autouni");
	batchtab_omci_update("autoveip");
	return 0;
}

static int 
meinfo_cb_45_delete(struct me_t *me)
{
	batchtab_omci_update("autouni");
	batchtab_omci_update("autoveip");
	return 0;
}

struct meinfo_callback_t meinfo_cb_45 = {
	.create_cb		= meinfo_cb_45_create,
	.delete_cb		= meinfo_cb_45_delete,
};
