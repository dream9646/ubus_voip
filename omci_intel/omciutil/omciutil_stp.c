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
 * Module  : omciutil
 * File    : omciutil_stp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "util.h"
#include "env.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "me_related.h"
#include "switch.h"

int
omciutil_stp_state_by_porttype(int stp_conf, int porttype)
{
	switch (stp_conf) {		
	case ENV_STP_CONF_ALL_DISABLE:
		return STP_DISABLE;
		break;
	case ENV_STP_CONF_CPU_OFF_ELSE_DISABLE:
		if (porttype==ENV_BRIDGE_PORT_TYPE_CPU || porttype==ENV_BRIDGE_PORT_TYPE_IPHOST)			// cpu, iphost
			return STP_OFF;
		return STP_DISABLE;
		break;
	case ENV_STP_CONF_CPU_UNI_OFF_ELSE_DISABLE:
		if (porttype==ENV_BRIDGE_PORT_TYPE_CPU || porttype==ENV_BRIDGE_PORT_TYPE_IPHOST || // cpu, iphost
		    porttype==ENV_BRIDGE_PORT_TYPE_UNI)	// uni
			return STP_OFF;
		return STP_DISABLE;
		break;
	case ENV_STP_CONF_ALL_OFF:
	default:
		return STP_OFF;
	}
}

#if 0
// FIXME, KEVIN
// this was used to config mglc port state, which might not required for 2510
static int
omciutil_stp_set_internal_bridge_port(int stp_conf)
{
	unsigned short ibridge_port_classid=hwresource_is_related(47);
	struct meinfo_t *miptr=meinfo_util_miptr(ibridge_port_classid);
	struct me_t *me;

	if (stp_conf==ENV_STP_CONF_NO_CHANGE)
		return 0;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "classid 47 has no related hwresource?\n");
		return -1;
	}		
	if (switch_hw_g.stp_state_port_set==NULL) {
		dbprintf(LOG_ERR, "switch_hw_g.stp_state_port_set=NULL?\n");
		return -1;
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		unsigned char port_type=meinfo_util_me_attr_data(me, 4);
		unsigned char logical_port_id=meinfo_util_me_attr_data(me, 6);
		unsigned char stp_state=omciutil_stp_state_by_porttype(stp_conf, port_type);

		if (port_type!=ENV_BRIDGE_PORT_TYPE_DS_BCAST)
			switch_hw_g.stp_state_port_set(logical_port_id, stp_state);

		switch(port_type) {
		case ENV_BRIDGE_PORT_TYPE_CPU:	// cpu (veip)
			dbprintf(LOG_WARNING, "classid=%u, meid=%u, core port%u (veip), stp state %d\n", 
				me->classid, me->meid, logical_port_id, stp_state);
			break;
		case ENV_BRIDGE_PORT_TYPE_IPHOST: // iphost
			dbprintf(LOG_WARNING, "classid=%u, meid=%u, core port%u (iphost), stp state %d\n", 
				me->classid, me->meid, logical_port_id, stp_state);
			break;
		case ENV_BRIDGE_PORT_TYPE_UNI:	// uni
			dbprintf(LOG_WARNING, "classid=%u, meid=%u, core port%u (uni), stp state %d\n", 
				me->classid, me->meid, logical_port_id, stp_state);
			break;
		case ENV_BRIDGE_PORT_TYPE_WAN: // wan
			dbprintf(LOG_WARNING, "classid=%u, meid=%u, core port%u (wan), stp state %d\n", 
				me->classid, me->meid, logical_port_id, stp_state);
			break;
		case ENV_BRIDGE_PORT_TYPE_DS_BCAST: // ds_bcast
			dbprintf(LOG_WARNING, "classid=%u, meid=%u, core port%u (ds_bcast), stp state %d\n", 
				me->classid, me->meid, logical_port_id, stp_state);
			break;
		default:
			dbprintf(LOG_ERR, "classid=%u, meid=%u, core port%u, unknow type %d\n", 
				me->classid, me->meid, logical_port_id, port_type);
		}
	}

	return 0;
}
#endif

static int
omciutil_stp_set_internal_pptp_uni(int stp_conf)
{
	unsigned short ipptp_uni_classid=hwresource_is_related(11);
	struct meinfo_t *miptr=meinfo_util_miptr(ipptp_uni_classid);
	struct me_t *me;
	unsigned char stp_state_uni=omciutil_stp_state_by_porttype(stp_conf, ENV_BRIDGE_PORT_TYPE_UNI);

	if (stp_conf==ENV_STP_CONF_NO_CHANGE)
		return 0;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "classid 11 has no related hwresource?\n");
		return -1;
	}		
	if (switch_hw_g.stp_state_port_set==NULL) {
		dbprintf(LOG_ERR, "switch_hw_g.stp_state_port_set=NULL?\n");
		return -1;
	}
	if (switch_hw_g.stp_state_port_get==NULL) {
		dbprintf(LOG_ERR, "switch_hw_g.stp_state_port_set=NULL?\n");
		return -1;
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		unsigned char logical_port_id=meinfo_util_me_attr_data(me, 4);	
		
		switch_hw_g.stp_state_port_set(logical_port_id, stp_state_uni);
		dbprintf(LOG_INFO, "classid=%u, meid=%u, core port%u (uni), stp state %d\n", 
			me->classid, me->meid, logical_port_id, stp_state_uni);
	}

	return 0;
}

int
omciutil_stp_set_before_omci(void)
{
	int ret1=0, ret2=0;

	// note: internal bridge must be set earlier than pptp uni below
	//       as some bridge port should be stp_off if it is used by pptp uni on extsw

	// cpu to STP_OFF, uni/wan to STP_DISABLE
	//ret1=omciutil_stp_set_internal_bridge_port(omci_env_g.stp_conf_before_omci);
	ret2=omciutil_stp_set_internal_pptp_uni(omci_env_g.stp_conf_before_omci);

	if (ret1<0 || ret2<0)
		return -1;
	return 0;
}

int
omciutil_stp_set_after_omci(void)
{
	int ret1=0, ret2=0;

	// note: internal bridge must be set earlier than pptp uni below
	//       as some bridge port should be stp_off if it is used by pptp uni on extsw

	// cpu/uni ports to STP_OFF, wan to STP_DISABLE
	//ret1=omciutil_stp_set_internal_bridge_port(ENV_STP_CONF_CPU_UNI_OFF_ELSE_DISABLE);
	ret2=omciutil_stp_set_internal_pptp_uni(ENV_STP_CONF_CPU_UNI_OFF_ELSE_DISABLE);

	if (ret1<0 || ret2<0)
		return -1;
	return 0;
}

int
omciutil_stp_set_before_shutdown(void)
{
	int ret1=0, ret2=0;

	// note: internal bridge must be set later than pptpuni below
	//	 as we want to ensure all bridge port are turned stp_disabled
	
	// all extsw ports to STP_OFF as extsw may bot reset port stp state after reboot
	ret2=omciutil_stp_set_internal_pptp_uni(ENV_STP_CONF_ALL_OFF);
	// all core ports to STP_DISABLE to stop traffic
	//ret1=omciutil_stp_set_internal_bridge_port(ENV_STP_CONF_ALL_DISABLE);

	if (ret1<0 || ret2<0)
		return -1;
	return 0;
}
