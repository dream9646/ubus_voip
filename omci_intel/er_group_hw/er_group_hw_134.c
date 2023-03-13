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
 * Module  : er_group_hw
 * File    : er_group_hw_134.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "util.h"
#include "batchtab.h"
#include "batchtab_cb.h"

// 134 Ip_host_config_data

//134@1,134@4,134@5,134@6,134@7,134@8,134@14,134@15
int
er_group_hw_ip_host_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	char *me_devname = meinfo_util_get_config_devname(me);

	if (me_devname == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		batchtab_cb_wanif_me_update(me, attr_mask);
		batchtab_omci_update("wanif");		
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		batchtab_cb_wanif_me_update(me, attr_mask);
		batchtab_omci_update("wanif");		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}

	return 0;
}
