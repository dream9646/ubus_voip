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
 * Module  : hwresource
 * File    : hwresource_cb.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
 
#include "list.h"
#include "util.h"
#include "conv.h"
#include "meinfo.h"
#include "notify_chain.h"
#include "meinfo_cb.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "me_related.h"

// 9.5.1 [11] Physical_path_termination_point_Ethernet_UNI
static int
hwresource_cb_match_pptp_ethernet_uni(struct me_t *public_me, struct me_t *private_me)
{
	if (!public_me || !private_me)
		return 0;
	if (public_me->instance_num==private_me->instance_num)
		return 1;
	return 0;
}


// 9.1.4 [7] Software_image
static int
hwresource_cb_match_software_image(struct me_t *public_me, struct me_t *private_me)
{
	if (!public_me || !private_me)
		return 0;
	if (public_me->instance_num==private_me->instance_num)
		return 1;
	return 0;
}

// 9.2.3 [268] GEM_port_network_CTP 
static int
hwresource_cb_match_gemport(struct me_t *public_me, struct me_t *private_me)
{
	if (!public_me || !private_me)
		return 0;
	return 1;
}
// 9.2.3 [268] GEM_port_network_CTP 
static int
hwresource_cb_dismiss_gemport(struct me_t *public_me, struct me_t *private_me)
{
	if (!public_me || !private_me)
		return -1;
	
	// clear pq pointer in igem
	if(private_me) {
		struct attr_value_t attr;
		attr.data=0xffff;
		meinfo_me_attr_set(private_me, 5, &attr, 0);
	}
	
	return 0;
}

// 9.3.4 [47] MAC_bridge_port_configuration_data 
static int
hwresource_cb_match_bridgeport(struct me_t *public_me, struct me_t *private_me)
{
	struct attr_value_t attr_public_tptype;
	struct attr_value_t attr_public_tppointer;
	struct attr_value_t attr_private_porttype;
	struct attr_value_t attr_private_porttypeindex;
	struct me_t *uni_me, *veip_me, *iphost_me, *gem_iw_tp_me;

	if (!public_me || !private_me)
		return 0;
	
	// get public me attr3 (TP type)
	meinfo_me_attr_get(public_me, 3, &attr_public_tptype);
	// get public me attr4 (TP pointer)
	meinfo_me_attr_get(public_me, 4, &attr_public_tppointer);
	
	// get private me attr4 (bridge port type), 0:cpu, 1:uni, 2:wan
	meinfo_me_attr_get(private_me, 4, &attr_private_porttype);	
	// get private me attr5 (index within bridge port type)
	meinfo_me_attr_get(private_me, 5, &attr_private_porttypeindex);	

	switch (attr_public_tptype.data) {
	case 1:	// Physical path termination point Ethernet UNI
	case 2:	// Interworking VCC termination point
	case 7:	// Physical path termination point xDSL UNI part 1
	case 8:	// Physical path termination point VDSL UNI
	case 9:	// Ethernet flow termination point
	case 10:// Physical path termination point 802.11 UNI
		// get the meptr of the 'Physical path termination point Ethernet UNI'
		uni_me = meinfo_me_get_by_meid(11, attr_public_tppointer.data);
		if (uni_me) {
			struct me_t * veip_me=omciutil_misc_find_pptp_related_veip(uni_me);
			if (veip_me) {	// if the ppup uni has same devname as specific veip, try to alloc ibport of cpu type 
				if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_CPU) {     // veip
					if (veip_me->instance_num==attr_private_porttypeindex.data)
						return 1;
				}					
			} else {
				if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_UNI) {	// uni
					// choose the private me based on the inst num of the uni me pointed by the bport me
					if (uni_me->instance_num==attr_private_porttypeindex.data) {
						char *devname = meinfo_util_get_config_devname(private_me);
						if (public_me->is_private) {	// find ibport whose devname is autouni0..5, or autouni_wlan0/1
							if (strncmp(devname, "autouni", 7) == 0)
								return 1;
						} else {	// find ibport whose devname is not started with autouni
							if (strncmp(devname, "autouni", 7) != 0)
								return 1;
						}
					}
				}
			}
		}
		break;
	case 5:	// GEM interworking termination point
		gem_iw_tp_me = me_related_find_parent_me(public_me, 266);
		if (gem_iw_tp_me != NULL) {
			if (meinfo_util_me_attr_data(gem_iw_tp_me, 2) == 6) {	// attr2: iw option == downstream broadcast
				if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_DS_BCAST) // ds_bcast
					return 1;
			} else {						// 0..5: iwtp, 8021p mapper, ....
				if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_WAN) // wan
					return 1;
			}
		}
		break;
	case 3:	// 802.1p mapper service profile
	case 6:	// Multicast GEM interworking termination point
		if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_WAN)	// wan
			return 1;
		break;
	case 4:	// IP host config data
		iphost_me = meinfo_me_get_by_meid(134, attr_public_tppointer.data);
		if (iphost_me) {
			if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_IPHOST) {	// iphost
				if (iphost_me->instance_num==attr_private_porttypeindex.data)
					return 1;
			}
		}
		break;
	case 11:// VEIP
		veip_me = meinfo_me_get_by_meid(329, attr_public_tppointer.data);
		if (veip_me) {
			if (attr_private_porttype.data==ENV_BRIDGE_PORT_TYPE_CPU) {	// cpu(veip)
				if (veip_me->instance_num==attr_private_porttypeindex.data) {
					char *devname = meinfo_util_get_config_devname(private_me);
					if (public_me->is_private) {	// find ibport whose devname is autoveip0..1
						if (strncmp(devname, "autoveip", 8) == 0)
							return 1;
					} else {	// find ibport whose devname is not started with autoveip
						if (strncmp(devname, "autoveip", 8) != 0)
							return 1;
					}
				}
			}
		}
		break;
	}
	return 0;
}

int
hwresource_cb_init(void)
{
	// register hwresource match func for onu created me
	hwresource_register(11, "Internal_pptp_ethernet_uni", hwresource_cb_match_pptp_ethernet_uni, NULL);
	hwresource_register(7, "Internal_software_image", hwresource_cb_match_software_image, NULL);

	// register hwresource match func for olt created me
	hwresource_register(268, "Internal_gemport", hwresource_cb_match_gemport, hwresource_cb_dismiss_gemport);
	hwresource_register(47, "Internal_bridge_port", hwresource_cb_match_bridgeport, NULL);

	return 0;
}
