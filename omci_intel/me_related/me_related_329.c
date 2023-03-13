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
 * Module  : me_related
 * File    : me_related_329.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.5.5 [329] Virtual Ethernet interface point

// relation to 9.3.4 [47] MAC bridge port configuration data
int 
me_related_329_047(struct me_t *me1, struct me_t *me2)	
{
	return me_related_047_329(me2, me1);
}

// relation to 9.4.14 [136] TCP/UDP_config_data
int 
me_related_329_136(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
		return 0;

	meinfo_me_attr_get(me1, 4, &attr); //pointer to tcpudp
	
	if (attr.data == me2->meid) {
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.4.12 [134] IP_host_config_data
int 
me_related_329_134(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr=meinfo_util_miptr(136);
	struct me_t *meptr = NULL;

	if (me1 == NULL || me2 == NULL)
		return 0;

	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if(me_related(me1, meptr)) {
			if(me_related(meptr, me2)) {
				return 1;
			}
			break;
		}
	}
	return 0;
}

// relation to 9.12.1 [264] UNI_G
int 
me_related_329_264(struct me_t *me1, struct me_t *me2)	
{
	return me_related_264_329(me2, me1);
}

// relation to 9.2.3 [268] GEM_port_network_CTP
int 
me_related_329_268(struct me_t *me1, struct me_t *me2)	
{
	// 329 -> 47 -> 45 -> 47 -> 130 -> 266 -> 268
	struct meinfo_t *miptr47 = meinfo_util_miptr(47);
	struct me_t *meptr47_uni = NULL;
	list_for_each_entry(meptr47_uni, &miptr47->me_instance_list, instance_node) {
		if (me_related(me1, meptr47_uni)) { // 329 -> 47
			struct meinfo_t *miptr45 = meinfo_util_miptr(45);
			struct me_t *meptr45 = NULL;
			list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
				if (me_related(meptr47_uni, meptr45)) { // 47 (UNI) -> 45
					struct me_t *meptr47_ani = NULL;
					list_for_each_entry(meptr47_ani, &miptr47->me_instance_list, instance_node) {
						unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(meptr47_ani, 3);
						if (me_related(meptr45, meptr47_ani) && (tp_type == 3 || tp_type == 5)) { // 45 -> 47*N (ANI)
							unsigned short tp_ptr = (unsigned short) meinfo_util_me_attr_data(meptr47_ani, 4);
							if(tp_type == 3) { // 802.1p mapper
								struct me_t *mapper_me = meinfo_me_get_by_meid(130, tp_ptr);
								struct meinfo_t *miptr266 = meinfo_util_miptr(266);
								struct me_t *meptr266 = NULL;
								list_for_each_entry(meptr266, &miptr266->me_instance_list, instance_node) {
									if (me_related(mapper_me, meptr266)) { // 130 -> 266*N
										struct meinfo_t *miptr268 = meinfo_util_miptr(268);
										struct me_t *meptr268 = NULL;
										list_for_each_entry(meptr268, &miptr268->me_instance_list, instance_node) {
											if (me_related(meptr266, meptr268)) { // 266 -> 268
												if(meptr268->meid == me2->meid)
													return 1;
											}
										}
										// break; // may exist N gem itwp
									}
								}
							} else { // gem iwtp
								struct me_t *gem_iwtp_me = meinfo_me_get_by_meid(266, tp_ptr);
								struct meinfo_t *miptr268 = meinfo_util_miptr(268);
								struct me_t *meptr268 = NULL;
								list_for_each_entry(meptr268, &miptr268->me_instance_list, instance_node) {
									if (me_related(gem_iwtp_me, meptr268)) { // 266 -> 268
										if(meptr268->meid == me2->meid)
											return 1;
									}
								}
							}
							//break; // may exist N bridge ports
						}
					}
					break;
				}
			}
		}
	}
	return 0;
}

// relation to 9.12.14 [330] Generic status portal
int 
me_related_329_330(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}

// relation to 9.12.16 [340] BBF TR-069 management server
int 
me_related_329_340(struct me_t *me1, struct me_t *me2)	
{
	return (me1->meid==me2->meid);
}
