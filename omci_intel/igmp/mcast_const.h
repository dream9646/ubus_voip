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
 * Module  : igmp
 * File    : mcast_const.h
 *
 ******************************************************************/

#ifndef __IGMP_CONSTANT_H__
#define __IGMP_CONSTANT_H__

#define VLAN_TAG_OFFSET		4
#define BUF_SIZE		2030

#define TRUE	1
#define FALSE	0

#define IGMP_FUNCTION_TRANSPARENT 			0
#define IGMP_FUNCTION_SNOOPING_WITH_PROXY_REPORTING 	1
#define IGMP_FUNCTION_PROXY 				2
#define IGMP_FUNCTION_DISABLE				3	

#define IGMP_V1_BIT	1
#define IGMP_V2_BIT	2
#define IGMP_V3_BIT	4
#define MLD_V1_BIT	8
#define MLD_V2_BIT	16

#define MAX_BRIDGE_NUM	2
#define IGMP_MAX_UNI_PORTS	8
#define IGMP_MAX_WAN_PORTS	8


#define MODE_IS_INCLUDE		1
#define MODE_IS_EXCLUDE 	2
#define CHANGE_TO_INCLUDE_MODE	3
#define CHANGE_TO_EXCLUDE_MODE	4
#define ALLOW_NEW_SOURCES	5
#define BLOCK_OLD_SOURCES	6


#define IGMP_SPECIAL			0xff

#ifndef IGMP_MEMBERSHIP_QUERY	// in netinet/igmp.h
#define IGMP_MEMBERSHIP_QUERY 		0x11	/* membership query */
#define IGMP_V1_MEMBERSHIP_REPORT	0x12	/* Ver. 1 membership report */
#define IGMP_V2_MEMBERSHIP_REPORT	0x16	/* Ver. 2 membership report */
#define IGMP_V2_LEAVE_GROUP		0x17	/* Leave-group message	 */
#endif
#define IGMP_V3_MEMBERSHIP_REPORT	0x22	/* Ver. 3 membership report */

#ifndef MLD_LISTENER_QUERY	// in netinet/icmp6.h
#define MLD_LISTENER_QUERY	0x82  /* MLD membership query*/
#define MLD_LISTENER_REPORT	0x83  /* Ver. 1 MLD membership report*/
#define MLD_LISTENER_REDUCTION	0x84  /* Ver. 1 MLD done message*/
#endif
#define MLD_V2_LISTENER_REPORT	0x8F  /* Ver. 2 MLD membership report*/

#define IGMP_JOIN_ACTIVE_GROUP_LIST_ENTRY_OTHER_CLIENT_FOUND	10	// active group entry found
#define IGMP_JOIN_ACTIVE_GROUP_LIST_ENTRY_CLIENT_FOUND		9	// active group entry found
#define IGMP_JOIN_STATIC_ACL_ENTRY_FOUND			8	// static acl entry found
#define IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND			7	// dyanmic acl preview entry found
#define IGMP_JOIN_APGT_ENTRY_FOUND				6	// allowed preview group table entry found
#define IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND			5	// dyanmic acl full auth entry found
#define IGMP_JOIN_ALREADY_BY_OTHER_CLIENT			4	// same group found in active list
#define IGMP_JOIN_ALREADY_BY_SAME_CLIENT			3	// same client+group found in active list
#define IGMP_JOIN_GRANTED_ALWAYS				2	// matched in static acl table
#define IGMP_JOIN_GRANTED					1	// matched in dynamic acl table
#define IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND			0	
#define IGMP_JOIN_DENIED_ACL_NOT_FOUND				-1
#define IGMP_JOIN_DENIED_ACL_FILTERED				-2
#define IGMP_JOIN_DENIED_CLIENT_ALREADY_EXIST			-3
#define IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND			-4
#define IGMP_JOIN_PREVIEW_ACL_ENTRY_NOT_FOUND			-5	// dyanmic acl preview entry found
#define IGMP_JOIN_APGT_ENTRY_NOT_FOUND				-6	// allowed preview group table entry found
#define IGMP_JOIN_FULL_AUTH_ACL_ENTRY_NOT_FOUND			-7	// dyanmic acl full auth entry found
#define IGMP_JOIN_ACTIVE_GROUP_ENTRY_LIST_CLIENT_NOT_FOUND	-8	// active group entry not found

#define IGMPV3_QUERY_TIMEOUT_INTERVAL igmp_config_g.max_respon_time*100 + igmp_config_g.robustness*igmp_config_g.query_interval*1000
#endif
