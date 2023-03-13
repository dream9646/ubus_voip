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
 * File    : meinfo_hw_330.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"

struct generic_portal_status_env_t {
	char *ve_interdomain_name;
	unsigned short ve_iana_assigned_port;
	unsigned char tcpudp_protocol;
	unsigned short tcpudp_port;
	unsigned char tcpudp_tosdscp;
	unsigned int iphost_ipaddr;
	char xmlfile[128];;
};	

static void
portal_env_init(struct generic_portal_status_env_t *e)
{
	e->ve_interdomain_name=NULL;
	e->ve_iana_assigned_port=0xffff;
	e->tcpudp_protocol=6;	// tcp:6, udp:11
	e->tcpudp_port=0xffff;
	e->tcpudp_tosdscp=0;
	e->iphost_ipaddr=0;
	snprintf(e->xmlfile, 128, "/var/run/generic_status_portal.%x04.xml", rand()%0xffff);
}

static int
system_cmd(char *script, char *argstr, struct generic_portal_status_env_t *e)
{
	char cmd[1024];
	int ret;

	util_setenv_str("ve_interdomain_name", e->ve_interdomain_name);
	util_setenv_number("ve_iana_assigned_port", e->ve_iana_assigned_port);
	util_setenv_number("tcpudp_protocol", e->tcpudp_protocol);
	util_setenv_number("tcpudp_port", e->tcpudp_port);
	util_setenv_number("tcpudp_tosdscp", e->tcpudp_tosdscp);
	util_setenv_ipv4("iphost_ipaddr", e->iphost_ipaddr);
	util_setenv_str("xmlfile", e->xmlfile);

	snprintf(cmd, 1024, "%s %s", script, argstr);
	ret=util_run_by_system(cmd);

	unsetenv("ve_interdomain_name");
	unsetenv("ve_iana_assigned_port");
	unsetenv("tcpudp_protocol");
	unsetenv("tcpudp_port");
	unsetenv("tcpudp_tosdscp");
	unsetenv("iphost_ipaddr");
	unsetenv("xmlfile");

	return ret;
}

// 9.12.14 Generic_status_portal
static int 
meinfo_hw_330_get(struct me_t *me, unsigned char attr_mask[2])
{
	char generic_status_portal_script[256];
	struct me_t *virtual_ethernet_me=NULL, *tcpudp_me=NULL, *iphost_me=NULL;
	unsigned short tcpudp_meid;
	unsigned short iphost_meid;
	struct generic_portal_status_env_t portal_env;
	unsigned short attr_order;

	// find ptr to me...
	virtual_ethernet_me=meinfo_me_get_by_meid(329, me->meid);
	if (virtual_ethernet_me==NULL) {
		dbprintf(LOG_ERR, "generic status portal %u:%u(0x%x) has not related virtual ethernet me?\n", 
			me->classid, me->meid, me->meid);
		return -1;
	}
	tcpudp_meid=meinfo_util_me_attr_data(virtual_ethernet_me, 4);		
	tcpudp_me=meinfo_me_get_by_meid(136, tcpudp_meid);
	if (tcpudp_me) {
		iphost_meid=meinfo_util_me_attr_data(tcpudp_me, 4);
		iphost_me=meinfo_me_get_by_meid(134, iphost_meid);
	}
	
	portal_env_init(&portal_env);
	// get attrs from me and set to portal_env
	portal_env.ve_interdomain_name=meinfo_util_me_attr_ptr(virtual_ethernet_me, 3);
	portal_env.ve_iana_assigned_port=meinfo_util_me_attr_data(virtual_ethernet_me, 5);
	if (tcpudp_me) {
		portal_env.tcpudp_port=meinfo_util_me_attr_data(tcpudp_me, 1);
		portal_env.tcpudp_protocol=meinfo_util_me_attr_data(tcpudp_me, 2);
		portal_env.tcpudp_tosdscp=meinfo_util_me_attr_data(tcpudp_me, 3);
	}
	if (iphost_me) {
		unsigned char ipopt=meinfo_util_me_attr_data(iphost_me, 1);
		unsigned int static_ipaddr=meinfo_util_me_attr_data(iphost_me, 4);
		unsigned int dhcp_ipaddr=meinfo_util_me_attr_data(iphost_me, 9);

		if ((ipopt & 1) && static_ipaddr==0) {	// dhcp enabled && no static ip
			portal_env.iphost_ipaddr=dhcp_ipaddr;
		} else {
		    	portal_env.iphost_ipaddr=static_ipaddr;
		}
	}

	util_omcipath_filename(generic_status_portal_script, 256, "script/generic_status_portal.sh");	
	for (attr_order=1; attr_order<=2; attr_order++) {
		if (util_attr_mask_get_bit(attr_mask, attr_order)) {	
			int xmlsize;
			char *xmlptr=NULL;

			if (attr_order==1) {	// attr1: status_document_table
				snprintf(portal_env.xmlfile, 128, "/var/run/portal_status.%u.xml", me->meid);
				unlink(portal_env.xmlfile);
				system_cmd(generic_status_portal_script, "get_status", &portal_env);
			} else {		// attr2: configuration_document_table
				snprintf(portal_env.xmlfile, 128, "/var/run/portal_config.%u.xml", me->meid);
				unlink(portal_env.xmlfile);
				system_cmd(generic_status_portal_script, "get_config", &portal_env);
			}
			xmlsize=util_readfile(portal_env.xmlfile, &xmlptr);
			unlink(portal_env.xmlfile);
			if (xmlsize>0) {
				meinfo_me_attr_set_table_all_entries_by_mem(me, attr_order, xmlsize, xmlptr, 1);
			} else {
				dbprintf(LOG_ERR, "Generic status portal %u:%u(0x%x) attr %u result xml error?\n",
					me->classid, me->meid, me->meid, attr_order);
			}
			if (xmlptr)
				free_safe(xmlptr);
		}				
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_330 = {
	.get_hw		= meinfo_hw_330_get,
};

