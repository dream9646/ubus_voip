/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
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
 * Module  : acl
 * File    : vacl.h
 *
 ******************************************************************/

#ifndef __VACL_H__
#define __VACL_H__
#include "chipdef.h"

#define VACL_DEFAULT_TEMPLATE_INDEX	(255)
#define VACL_DEFAULT_RULE_MODE		(0)
#define VACL_HW_ENTRY_ID_MIN		(0)
#define VACL_ACT_NONE			(0)
#define VACL_ENTRY_TOTAL		CHIP_MAX_NUM_OF_ACL_RULE_ENTRY
#define VACL_ORDER_TOTAL		(256)
#define VACL_RULE_FIELD_TOTAL		(8)
#define VACL_TEMPLATE_TOTAL		(4)
#define VACL_TEMPLATE_4EVAL		VACL_TEMPLATE_TOTAL
#define VACL_TEMPLATE_USER_MIN		(2)
#define VACL_TEMPLATE_FIELDS_IP		(2)
#define VACL_TEMPLATE_FIELDS_IPV6	(2)
#define VACL_TEMPLATE_FIELDS_MAC	(3)
#define VACL_FIELD_SELECTOR_TOTAL	(16)
#define VACL_METER_ENTRY_TOTAL		(CHIP_MAX_NUM_OF_METERING)
#define VACL_METER_ENTRY_INIT_RESERVE	(2) /* default reserved two, one for storm limit, the other for broadcast and multicast */
#define INGRESS_RANGE_VID_INDEX_TOTAL		(8)
#define INGRESS_RANGE_L4PORT_INDEX_TOTAL	(16)
#define INGRESS_RANGE_IP_INDEX_TOTAL		(8)
#define INGRESS_RANGE_IP_UNUSED			(0)
#define INGRESS_RANGE_IP_SIPV4			(1)
#define INGRESS_RANGE_IP_DIPV4			(2)
#define INGRESS_RANGE_IP_SIPV6			(3)
#define INGRESS_RANGE_IP_DIPV6			(4)
#define INGRESS_RANGE_PKTLEN_INDEX_TOTAL	(8)
#define INGRESS_RANGE_ETHER_TYPE_ARY_INDEX_TOTAL	(30)
#define ACT_METER_RATE_MIN		(1)
#define ACT_METER_RATE_UNIT		(8)
#define ACT_METER_RATE_MAX		(0x1ffff * ACT_METER_RATE_UNIT)
#define ACT_METER_RATE_DEFAULT		ACT_METER_RATE_MAX
#define ACT_METER_BUCKET_SIZE_MIN	(0)
#define ACT_METER_BUCKET_SIZE_MAX	(0xffff * ACT_METER_BUCKET_SIZE_UNIT)
#define ACT_METER_BUCKET_SIZE_DEFAULT	(0x3fff)
#define ACT_METER_BUCKET_SIZE_UNIT	(1)
#define ACT_METER_IFG_DEFAULT		(0)
#define ACT_METER_INDEX_AUTO_W_SHARE	(0xe0)
#define ACT_METER_INDEX_AUTO_WO_SHARE	(0xe1)
#define VACL_FLD_PROTOCOL_VALUE_MAX	(0xffff)

#define VACL_ADD_RET_FULL		(101)
#define VACL_ADD_RET_DUPLICATE		(102)
#define VACL_DEL_RET_EMPTY		(103)
#define VACL_DEL_RET_MISS		(104)

#define VACL_ACT_DROP_STR		"drop"
#define VACL_ACT_FWD_COPY_STR		"forward_copy"
#define VACL_ACT_FWD_REDIRECT_STR	"forward_redirect"
#define VACL_ACT_FWD_MIRROR_STR		"forward_mirror"
#define VACL_ACT_FWD_PORT_STR		"forward_port"
#define VACL_ACT_FWD_PORT_S1_STR	"port"
#define VACL_ACT_TRAP_CPU_STR		"trap_cpu"
#define VACL_ACT_TRAP_PS_STR		"trap_ps"
#define VACL_ACT_QOS_SHARE_METER_STR	"share_meter"
#define VACL_ACT_QOS_SHARE_METER_S1_STR	"meter"
#define VACL_ACT_LATCH_STR		"latch"
#define VACL_ACT_QOS_ACL_PRIORITY_STR	"acl_priority"
#define VACL_ACT_QOS_PRI_DSCP_STR	"assign_dscp"
#define VACL_ACT_QOS_PRI_1P_STR		"assign_dot1p"
#define VACL_ACT_QOS_STR		"qos"
#define VACL_ACT_STATISTIC_STR		"statistic"
#define VACL_ACT_CVLAN_STR		"cvlan"
#define VACL_ACT_SVLAN_STR		"svlan"
#define VACL_ACT_1P_REMARK_STR		"dot1p_remark"
#define VACL_ACT_DSCP_REMARK_STR	"dscp_remark"

#define VACL_ACT_DROP_ID		(0) // for compatible
#define VACL_ACT_FWD_COPY_ID		(1) // for compatible
#define VACL_ACT_FWD_REDIRECT_ID	(2) // for compatible
#define VACL_ACT_FWD_MIRROR_ID		(3) // for compatible
#define VACL_ACT_TRAP_CPU_ID		(4) // for compatible
#define VACL_ACT_SHARE_METER_ID		(5) // for compatible
#define VACL_ACT_LATCH_ID		(6) // for compatible
#define VACL_ACT_PRI_IPRI_ID		(730) // for compatible
#define VACL_ACT_PRI_DSCP_ID		(731) // for compatible
#define VACL_ACT_PRI_1P_ID		(732) // for compatible

/*clvan action ACL_IGR_CVLAN_ACT*/
#define VACL_ACT_CVLAN_IGR_CVLAN_BIT	(1)
#define VACL_ACT_CVLAN_EGR_CVLAN_BIT	(2)
#define VACL_ACT_CVLAN_DS_SVID_BIT	(3)
#define VACL_ACT_CVLAN_POLICING_BIT	(4)
#define VACL_ACT_CVLAN_MIB_BIT		(5)
#define VACL_ACT_CVLAN_1P_REMARK_BIT	(6)
#define VACL_ACT_CVLAN_BW_METER_BIT	(7)
/*svlan action ACL_IGR_SVLAN_ACT*/
#define VACL_ACT_SVLAN_IGR_SVLAN_BIT	(8)
#define VACL_ACT_SVLAN_EGR_SVLAN_BIT	(9)
#define VACL_ACT_SVLAN_US_CVID_BIT	(10)
#define VACL_ACT_SVLAN_POLICING_BIT	(11)
#define VACL_ACT_SVLAN_MIB_BIT		(12)
#define VACL_ACT_SVLAN_1P_REMARK_BIT	(13)
#define VACL_ACT_SVLAN_DSCP_REMARK_BIT	(14)
#define VACL_ACT_SVLAN_ROUTE_BIT	(15)
#define VACL_ACT_SVLAN_BW_METER_BIT	(16)
/*priority action ACL_IGR_PRI_ACT*/
#define VACL_ACT_PRI_ACL_PRI_ASSIGN_BIT	(17)
#define VACL_ACT_PRI_DSCP_REMARK_BIT	(18)
#define VACL_ACT_PRI_1P_REMARK_BIT	(19)
#define VACL_ACT_PRI_POLICING_BIT	(20)
#define VACL_ACT_PRI_MIB_BIT		(21)
#define VACL_ACT_PRI_ROUTE_BIT		(22)
#define VACL_ACT_PRI_BW_METER_BIT	(23)
/*policing action ACL_IGR_LOG_ACT*/
#define VACL_ACT_LOG_POLICING_BIT	(24)
#define VACL_ACT_LOG_MIB_BIT		(25)
#define VACL_ACT_LOG_BW_METER_BIT	(26)
#define VACL_ACT_LOG_1P_REMARK_BIT	(27)
/*forward action ACL_IGR_FORWARD_ACT*/
#define VACL_ACT_FWD_COPY_BIT		(28) /* 0x0: Copy frame with ACLPMSK */
#define VACL_ACT_FWD_REDIRECT_BIT	(29) /* 0x1: Redirect frame with ACLPMSK */
#define VACL_ACT_FWD_IGR_MIRROR_BIT	(30) /* 0x2: Ingress mirror to ACLPMSK */
#define VACL_ACT_FWD_TRAP_BIT		(31)
#define VACL_ACT_FWD_DROP_BIT		(32) /* 0x0: Copy frame with ACLPMSK */
#define VACL_ACT_FWD_EGRESSMASK_BIT	(33)
#define VACL_ACT_FWD_NOP_BIT		(34)
/*action ACL_IGR_INTR_ACT*/
#define VACL_ACT_INTR_LATCH_BIT		(35) /* latch rule index */
/*policy route action ACL_IGR_ROUTE_ACT*/
#define VACL_ACT_ROUTE_ROUTE_BIT	(36)
#define VACL_ACT_ROUTE_1P_REMARK_BIT	(37)

#define VACL_ACT_FWD_PERMIT_BIT		(38)
#define VACL_ACT_TOTAL_BIT		(39)

#define VACL_ACT_CVLAN_IGR_CVLAN_MASK	(1ULL<<VACL_ACT_CVLAN_IGR_CVLAN_BIT)
#define VACL_ACT_CVLAN_EGR_CVLAN_MASK	(1ULL<<VACL_ACT_CVLAN_EGR_CVLAN_BIT)
#define VACL_ACT_CVLAN_DS_SVID_MASK	(1ULL<<VACL_ACT_CVLAN_DS_SVID_BIT)
#define VACL_ACT_CVLAN_POLICING_MASK	(1ULL<<VACL_ACT_CVLAN_POLICING_BIT)
#define VACL_ACT_CVLAN_MIB_MASK		(1ULL<<VACL_ACT_CVLAN_MIB_BIT)
#define VACL_ACT_CVLAN_1P_REMARK_MASK	(1ULL<<VACL_ACT_CVLAN_1P_REMARK_BIT)
#define VACL_ACT_CVLAN_BW_METER_MASK	(1ULL<<VACL_ACT_CVLAN_BW_METER_BIT)
#define VACL_ACT_SVLAN_IGR_SVLAN_MASK	(1ULL<<VACL_ACT_SVLAN_IGR_SVLAN_BIT)
#define VACL_ACT_SVLAN_EGR_SVLAN_MASK	(1ULL<<VACL_ACT_SVLAN_EGR_SVLAN_BIT)
#define VACL_ACT_SVLAN_US_CVID_MASK	(1ULL<<VACL_ACT_SVLAN_US_CVID_BIT)
#define VACL_ACT_SVLAN_POLICING_MASK	(1ULL<<VACL_ACT_SVLAN_POLICING_BIT)
#define VACL_ACT_SVLAN_MIB_MASK		(1ULL<<VACL_ACT_SVLAN_MIB_BIT)
#define VACL_ACT_SVLAN_1P_REMARK_MASK	(1ULL<<VACL_ACT_SVLAN_1P_REMARK_BIT)
#define VACL_ACT_SVLAN_DSCP_REMARK_MASK	(1ULL<<VACL_ACT_SVLAN_DSCP_REMARK_BIT)
#define VACL_ACT_SVLAN_ROUTE_MASK	(1ULL<<VACL_ACT_SVLAN_ROUTE_BIT)
#define VACL_ACT_SVLAN_BW_METER_MASK	(1ULL<<VACL_ACT_SVLAN_BW_METER_BIT)
#define VACL_ACT_PRI_ACL_PRI_ASSIGN_MASK	(1ULL<<VACL_ACT_PRI_ACL_PRI_ASSIGN_BIT)
#define VACL_ACT_PRI_DSCP_REMARK_MASK	(1ULL<<VACL_ACT_PRI_DSCP_REMARK_BIT)
#define VACL_ACT_PRI_1P_REMARK_MASK	(1ULL<<VACL_ACT_PRI_1P_REMARK_BIT)
#define VACL_ACT_PRI_POLICING_MASK	(1ULL<<VACL_ACT_PRI_POLICING_BIT)
#define VACL_ACT_PRI_MIB_MASK		(1ULL<<VACL_ACT_PRI_MIB_BIT)
#define VACL_ACT_PRI_ROUTE_MASK		(1ULL<<VACL_ACT_PRI_ROUTE_BIT)
#define VACL_ACT_PRI_BW_METER_MASK	(1ULL<<VACL_ACT_PRI_BW_METER_BIT)
#define VACL_ACT_LOG_POLICING_MASK	(1ULL<<VACL_ACT_LOG_POLICING_BIT)
#define VACL_ACT_LOG_MIB_MASK		(1ULL<<VACL_ACT_LOG_MIB_BIT)
#define VACL_ACT_LOG_BW_METER_MASK	(1ULL<<VACL_ACT_LOG_BW_METER_BIT)
#define VACL_ACT_LOG_1P_REMARK_MASK	(1ULL<<VACL_ACT_LOG_1P_REMARK_BIT)
#define VACL_ACT_FWD_COPY_MASK		(1ULL<<VACL_ACT_FWD_COPY_BIT)
#define VACL_ACT_FWD_REDIRECT_MASK	(1ULL<<VACL_ACT_FWD_REDIRECT_BIT)
#define VACL_ACT_FWD_IGR_MIRROR_MASK	(1ULL<<VACL_ACT_FWD_IGR_MIRROR_BIT)
#define VACL_ACT_FWD_TRAP_MASK		(1ULL<<VACL_ACT_FWD_TRAP_BIT)
#define VACL_ACT_FWD_DROP_MASK		(1ULL<<VACL_ACT_FWD_DROP_BIT)
#define VACL_ACT_FWD_EGRESSMASK_MASK	(1ULL<<VACL_ACT_FWD_EGRESSMASK_BIT)
#define VACL_ACT_FWD_NOP_MASK		(1ULL<<VACL_ACT_FWD_NOP_BIT)
#define VACL_ACT_FWD_PERMIT_MASK	(1ULL<<VACL_ACT_FWD_PERMIT_BIT)
#define VACL_ACT_INTR_LATCH_MASK	(1ULL<<VACL_ACT_INTR_LATCH_BIT)
#define VACL_ACT_ROUTE_ROUTE_MASK	(1ULL<<VACL_ACT_ROUTE_ROUTE_BIT)
#define VACL_ACT_ROUTE_1P_REMARK_MASK	(1ULL<<VACL_ACT_ROUTE_1P_REMARK_BIT)

#define VACL_CLI_METER_RESERVE "meter_reserve"
#define VACL_CLI_ORDER "order"
#define VACL_CLI_ORDER_S1 "or"
#define VACL2_CLI_ACL_WEIGHT "weight"
#define VACL2_CLI_ACL_WEIGHT_S1 "we"
#define VACL_CLI_HW_ORDER "hw_order"
#define VACL_CLI_HW_ORDER_S1 "ho"
#define VACL_CLI_CRC "crc"
#define VACL2_CLI_INTERNAL_PRI "internal_pri"
#define VACL2_CLI_ENTRY "entry"
#define VACL2_CLI_ENTRY_S1 "en"
#define VACL2_CLI_RG "rg"

#define VACL_CLI_FLD_INGRESS "fld"
#define VACL_CLI_FLD_PORTMASK "port"
#define VACL_CLI_FLD_DMAC "dmac"
#define VACL_CLI_FLD_DMAC_MASK "dmac_m"
#define VACL_CLI_FLD_SMAC "smac"
#define VACL_CLI_FLD_SMAC_MASK "smac_m"
#define VACL_CLI_FLD_ETHERTYPE "ethertype"
#define VACL_CLI_FLD_ETHERTYPE_MASK "ethertype_m"
#define VACL_CLI_FLD_ETHERTYPE_S1 "et"
#define VACL_CLI_FLD_ETHERTYPE_MASK_S1 "et_m"
#define VACL_CLI_FLD_ETHERTYPE_LB "ethertype_lb"
#define VACL_CLI_FLD_ETHERTYPE_UB "ethertype_ub"
#define VACL_CLI_FLD_CTAG "ctag"
#define VACL_CLI_FLD_CTAG_MASK "ctag_m"
#define VACL_CLI_FLD_CVID "cvid"
#define VACL_CLI_FLD_CPRI "cpri"
#define VACL_CLI_FLD_CVID_LB "cvid_lb"
#define VACL_CLI_FLD_CVID_UB "cvid_ub"
#define VACL_CLI_FLD_STAG "stag"
#define VACL_CLI_FLD_STAG_MASK "stag_m"
#define VACL_CLI_FLD_SVID "svid"
#define VACL_CLI_FLD_SPRI "spri"
#define VACL_CLI_FLD_SVID_LB "svid_lb"
#define VACL_CLI_FLD_SVID_UB "svid_ub"
#define VACL_CLI_FLD_DSCP_IPV4 "dscp_ipv4"
#define VACL_CLI_FLD_DSCP_IPV6 "dscp_ipv6"
#define VACL_CLI_FLD_SIPV4 "sipv4"
#define VACL_CLI_FLD_SIPV4_MASK "sipv4_m"
#define VACL_CLI_FLD_DIPV4 "dipv4"
#define VACL_CLI_FLD_DIPV4_MASK "dipv4_m"
#define VACL_CLI_FLD_SIPV6 "sipv6"
#define VACL_CLI_FLD_SIPV6_MASK "sipv6_m"
#define VACL_CLI_FLD_DIPV6 "dipv6"
#define VACL_CLI_FLD_DIPV6_MASK "dipv6_m"
#define VACL_CLI_FLD_SIPV4_LB "sipv4_lb"
#define VACL_CLI_FLD_SIPV4_LB_S1 "sip_lb"
#define VACL_CLI_FLD_SIPV4_UB "sipv4_ub"
#define VACL_CLI_FLD_SIPV4_UB_S1 "sip_ub"
#define VACL_CLI_FLD_DIPV4_LB "dipv4_lb"
#define VACL_CLI_FLD_DIPV4_LB_S1 "dip_lb"
#define VACL_CLI_FLD_DIPV4_UB "dipv4_ub"
#define VACL_CLI_FLD_DIPV4_UB_S1 "dip_ub"
#define VACL_CLI_FLD_SIPV6_LB "sipv6_lb"
#define VACL_CLI_FLD_SIPV6_LB_S1 "sip6_lb"
#define VACL_CLI_FLD_SIPV6_UB "sipv6_ub"
#define VACL_CLI_FLD_SIPV6_UB_S1 "sip6_ub"
#define VACL_CLI_FLD_DIPV6_LB "dipv6_lb"
#define VACL_CLI_FLD_DIPV6_LB_S1 "dip6_lb"
#define VACL_CLI_FLD_DIPV6_UB "dipv6_ub"
#define VACL_CLI_FLD_DIPV6_UB_S1 "dip6_ub"
#define VACL_CLI_FLD_L4SPORT_LB "l4sport_lb"
#define VACL_CLI_FLD_L4SPORT_LB_S1 "sport_lb"
#define VACL_CLI_FLD_L4SPORT_UB "l4sport_ub"
#define VACL_CLI_FLD_L4SPORT_UB_S1 "sport_ub"
#define VACL_CLI_FLD_L4DPORT_LB "l4dport_lb"
#define VACL_CLI_FLD_L4DPORT_LB_S1 "dport_lb"
#define VACL_CLI_FLD_L4DPORT_UB "l4dport_ub"
#define VACL_CLI_FLD_L4DPORT_UB_S1 "dport_ub"
#define VACL_CLI_FLD_PKTLEN_LB "pktlen_lb"
#define VACL_CLI_FLD_PKTLEN_UB "pktlen_ub"
#define VACL_CLI_FLD_PKTLEN_INVERT "pktlen_invert"
#define VACL_CLI_FLD_GEMIDX "gem"
#define VACL_CLI_FLD_GEMIDX_MASK "gem_m"
#define VACL_CLI_FLD_PROTOCOL_VALUE "protocol_value"
#define VACL_CLI_INVERT "invert"
#define VACL_CLI_CARE_TAG "care_tag"
#define VACL_CLI_CARE_TAG_S1 "ct"
#define VACL_CLI_CARE_TAG_S2 "in-care_tag"
#define VACL_CLI_CARE_TAG_S3 "in-ct"

#define VACL2_CLI_FLD_STREAM_ID "stream_id"
#define VACL2_CLI_FLD_STREAM_ID_MASK "stream_id_m"
#define VACL2_CLI_FLD_INGRESS "in-fld"
#define VACL2_CLI_FLD_EGRESS "e-fld"
#define VACL2_CLI_FLD_INTF "intf_idx"
#define VACL2_CLI_FLD_PORTMASK "portmask"
#define VACL2_CLI_FLD_PORTMASK_S1 "in-port"
#define VACL2_CLI_FLD_IS_CTAG "is_ctag"
#define VACL2_CLI_FLD_IS_STAG "is_stag"
#define VACL2_CLI_FLD_CF_UNI_PORT_INDEX "port_idx"
#define VACL2_CLI_FLD_CF_UNI_PORT_INDEX_MASK "port_idx_m"

#define VACL2_CLI_FWD_TYPE_DIR_STR "fwd_type_dir"
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_ALL_PACKET_ID (RG_ACL_FWD_TYPE_DIR_INGRESS_ALL_PACKET)
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_ALL_PACKET_STR "in_all"
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_DROP_ID (RG_ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_DROP)
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_DROP_STR "l34_up_drop"
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_DROP_ID (RG_ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_DROP)
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_DROP_STR "l34_down_drop"
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN_ID (RG_ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN)
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN_STR "l34_up_gem_c_s"
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN_ID (RG_ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN)
#define VACL2_CLI_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN_STR "l34_down_c_s"

#define VACL2_ACT_DROP_STR		"drop"
#define VACL2_ACT_PERMIT_STR		"permit"
#define VACL2_ACT_TRAP_CPU_STR		"trap_cpu"
#define VACL2_ACT_QOS_STR		"qos"
#define VACL2_ACT_TRAP_PS_STR		"trap_ps"
#define VACL2_ACT_POLICY_ROUTE_STR	"policy_route"
#define VACL2_ACT_QOS_1P_REMARKING_PRI_STR	"dot1p_remarking_pri"
#define VACL2_ACT_QOS_IP_PRECEDENCE_REMARKING_PRI_STR	"ip_precedence_remarking_pri"
#define VACL2_ACT_QOS_DSCP_REMARKING_PRI_STR	"dscp_remarking_pri"
#define VACL2_ACT_QOS_QUEUE_ID_STR		"queue_id"
#define VACL2_ACT_QOS_SHARE_METER_STR		"share_meter"
#define VACL2_ACT_QOS_STREAM_ID_OR_LLID_STR	"stream_id"
#define VACL2_ACT_QOS_ACL_PRIORITY_STR		"acl_priority"
#define VACL2_ACT_QOS_CVLAN_STR			"cvlan"
#define VACL2_ACT_QOS_CVLAN_TAG_WITH_C2S_STR	"tag_with_c2s"
#define VACL2_ACT_QOS_CVLAN_TAG_WITH_SP2C_STR	"tag_with_sp2c"
#define VACL2_ACT_QOS_CVLAN_TRANSPARENT_STR	"transparent"
#define VACL2_ACT_QOS_CVLAN_UNTAG_STR		"untag"
#define VACL2_ACT_QOS_CVLAN_TAGGING_STR		"tagging"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_ASSIGN_STR		"cvid_assign"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_1ST_TAG_STR	"cvid_copy_from_1st_tag"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_2ND_TAG_STR	"cvid_copy_from_2nd_tag"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_INTERNAL_VID_STR	"cvid_copy_from_internal_vid"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_CPOY_FROM_DMAC2CVID_STR	"cvid_cpoy_from_dmac2cvid"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_ASSIGN_STR		"cpri_assign"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_1ST_TAG_STR	"cpri_copy_from_1st_tag"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_2ND_TAG_STR	"cpri_copy_from_2nd_tag"
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_INTERNAL_PRI_STR	"cpri_copy_from_internal_pri"
#define VACL2_ACT_QOS_SVLAN_STR			"svlan"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SP2C_STR	"tag_with_sp2c"
#define VACL2_ACT_QOS_SVLAN_TRANSPARENT_STR	"transparent"
#define VACL2_ACT_QOS_SVLAN_UNTAG_STR		"untag"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_8100_STR	"tag_with_8100"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_TPID_STR	"tag_with_tpid"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_ASSIGN_STR			"svid_assign"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_COPY_FROM_1ST_TAG_STR		"svid_copy_from_1st_tag"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_COPY_FROM_2ND_TAG_STR		"svid_copy_from_2nd_tag"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_ASSIGN_STR			"spri_assign"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_1ST_TAG_STR		"spri_copy_from_1st_tag"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_2ND_TAG_STR		"spri_copy_from_2nd_tag"
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_INTERNAL_PRI_STR	"spri_copy_from_internal_pri"
#define VACL2_ACT_QOS_ACL_INGRESS_VID_STR	"acl_ingress_vid"
#define VACL2_ACT_QOS_DS_UNIMASK_STR		"ds_unimask"
#define VACL2_ACT_DROP				(10)
#define VACL2_ACT_PERMIT			(11)
#define VACL2_ACT_TRAP_CPU			(12)
#define VACL2_ACT_QOS				(13)
#define VACL2_ACT_TRAP_PS			(14)
#define VACL2_ACT_POLICY_ROUTE			(15)
#define VACL2_ACT_QOS_NOP			(130)
#define VACL2_ACT_QOS_1P_REMARKING_PRI	(131)
#define VACL2_ACT_QOS_IP_PRECEDENCE_REMARKING_PRI	(132)
#define VACL2_ACT_QOS_DSCP_REMARKING_PRI	(133)
#define VACL2_ACT_QOS_QUEUE_ID			(134)
#define VACL2_ACT_QOS_SHARE_METER		(135)
#define VACL2_ACT_QOS_STREAM_ID_OR_LLID		(136)
#define VACL2_ACT_QOS_ACL_PRIORITY		(137)
#define VACL2_ACT_QOS_CVLAN			(138)
#define VACL2_ACT_QOS_CVLAN_TAG_WITH_C2S	(1381)
#define VACL2_ACT_QOS_CVLAN_TAG_WITH_SP2C	(1382)
#define VACL2_ACT_QOS_CVLAN_TRANSPARENT		(1383)
#define VACL2_ACT_QOS_CVLAN_UNTAG		(1384)
#define VACL2_ACT_QOS_CVLAN_TAGGING		(1385)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_ASSIGN			(0)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_1ST_TAG	(1)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_2ND_TAG	(2)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_COPY_FROM_INTERNAL_VID	(3)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CVID_CPOY_FROM_DMAC2CVID	(4)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_ASSIGN			(0)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_1ST_TAG	(1)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_2ND_TAG	(2)
#define VACL2_ACT_QOS_CVLAN_TAGGING_CPRI_COPY_FROM_INTERNAL_PRI	(3)
#define VACL2_ACT_QOS_SVLAN			(139)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SP2C	(1391)
#define VACL2_ACT_QOS_SVLAN_TRANSPARENT		(1392)
#define VACL2_ACT_QOS_SVLAN_UNTAG		(1393)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_8100	(1394)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_TPID	(1395)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_ASSIGN			(0)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_COPY_FROM_1ST_TAG		(1)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SVID_COPY_FROM_2ND_TAG		(2)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_ASSIGN			(0)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_1ST_TAG		(1)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_2ND_TAG		(2)
#define VACL2_ACT_QOS_SVLAN_TAG_WITH_SPRI_COPY_FROM_INTERNAL_PRI	(3)
#define VACL2_ACT_DUMMY			(65500)
#define VACL2_ACT_LATCH			(VACL2_ACT_DUMMY+1)

#define VACL2_ACT_MASK_QOS_NOP				(1<<(VACL2_ACT_QOS_NOP-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_1P_REMARKING_PRI		(1<<(VACL2_ACT_QOS_1P_REMARKING_PRI-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_IP_PRECEDENCE_REMARKING_PRI	(1<<(VACL2_ACT_QOS_IP_PRECEDENCE_REMARKING_PRI-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_DSCP_REMARKING_PRI		(1<<(VACL2_ACT_QOS_DSCP_REMARKING_PRI-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_QUEUE_ID		(1<<(VACL2_ACT_QOS_QUEUE_ID-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_SHARE_METER		(1<<(VACL2_ACT_QOS_SHARE_METER-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_STREAM_ID_OR_LLID	(1<<(VACL2_ACT_QOS_STREAM_ID_OR_LLID-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_ACL_PRIORITY		(1<<(VACL2_ACT_QOS_ACL_PRIORITY-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_CVLAN		(1<<(VACL2_ACT_QOS_CVLAN-VACL2_ACT_QOS_NOP))
#define VACL2_ACT_MASK_QOS_SVLAN		(1<<(VACL2_ACT_QOS_SVLAN-VACL2_ACT_QOS_NOP))

#define VACL_EXTRACT_ORDER_CFM		(34)
#define VACL_EXTRACT_ORDER_EXTOAM	(40)
#define VACL_EXTRACT_ORDER_PPPOE	(41)
#define VACL_ETHER_TYPE_CFM		(0x8902)
#define VACL_ETHER_TYPE_EAPOL		(0x888e)
#define VACL_ETHER_TYPE_EXTOAM		(0x8809)
#define VACL_ETHER_TYPE_PPPOE		(0x8863)

enum vacl_ingress_care_tag_e
{
	VACL_INGRESS_CARE_TAG_PPPOE = 0,
	VACL_INGRESS_CARE_TAG_CTAG,
	VACL_INGRESS_CARE_TAG_STAG,
	VACL_INGRESS_CARE_TAG_IPV4,
	VACL_INGRESS_CARE_TAG_IPV6,
	VACL_INGRESS_CARE_TAG_TCP,
	VACL_INGRESS_CARE_TAG_UDP,
	VACL_INGRESS_CARE_TAG_NUMBER_TOTAL,
};

struct acl_uint16_range_t
{
	unsigned short lower, upper;
}__attribute__ ((packed));

struct acl_uint16_value_t
{
	unsigned short value, mask;
}__attribute__ ((packed));

struct acl_ipv4addr_range_t
{
	struct in_addr lower, upper;
}__attribute__ ((packed));

struct acl_ipv4addr_value_t
{
	struct in_addr value, mask;
}__attribute__ ((packed));

struct acl_ipv6addr_range_t
{
	struct in6_addr lower, upper;
}__attribute__ ((packed));

struct acl_ipv6addr_value_t
{
	struct in6_addr value, mask;
}__attribute__ ((packed));

#define TEMPLATE0_RELATIVES_MASK	((unsigned long long)0xff00000000000000ULL)
#define TEMPLATE1_RELATIVES_MASK	((unsigned long long)0x03fff00000000000ULL)
#define TEMPLATE2_3_RELATIVES_MASK	((unsigned long long)0x00000fffff800000ULL)
#define VACL_USER_NODE_INTERNAL_VAR_SIZE (sizeof(int) + sizeof(struct list_head) + sizeof(short)*11 + sizeof(union ingress_care_u) + 1)

struct vacl_ingress_field_care_t
{
//>>> acl pre-define template 0
	unsigned char dstmac_value:1;
	unsigned char dstmac_mask:1;
	unsigned char stag_value:1;
	unsigned char stag_mask:1;
	unsigned char srcmac_value:1;
	unsigned char srcmac_mask:1;
//>>> acl pre-define template 1
	unsigned char gem_llid_value:1;
	unsigned char gem_llid_mask:1;
//<<< byte MSB 1st
	unsigned char ctag_value:1;
	unsigned char ctag_mask:1;
	unsigned char sipv4_value:1;
	unsigned char sipv4_mask:1;
	unsigned char ether_type_value:1;
	unsigned char ether_type_mask:1;
	unsigned char l4sport_lower:1;
	unsigned char l4sport_upper:1;
//<<< byte MSB 2nd
	unsigned char l4dport_lower:1;
	unsigned char l4dport_upper:1;
	unsigned char dipv4_value:1;
	unsigned char dipv4_mask:1;
//>>> acl user define template
	unsigned char pktlen_lower:1;
	unsigned char pktlen_upper:1;
	unsigned char dipv6_value:1;
	unsigned char dipv6_mask:1;
//<<< byte MSB 3rd
	unsigned char sipv6_value:1;
	unsigned char sipv6_mask:1;
	unsigned char pktlen_invert:1;
	unsigned char ether_type_lower:1;
	unsigned char ether_type_upper:1;
	unsigned char ctag_vid_lower:1;
	unsigned char ctag_vid_upper:1;
	unsigned char stag_vid_lower:1;
//<<< byte MSB 4th
	unsigned char stag_vid_upper:1;
	unsigned char sipv4_lower:1;
	unsigned char sipv4_upper:1;
	unsigned char dipv4_lower:1;
	unsigned char dipv4_upper:1;
	unsigned char sipv6_lower:1;
	unsigned char sipv6_upper:1;
	unsigned char dipv6_lower:1;
//<<< byte MSB 5th
	unsigned char dipv6_upper:1;
	unsigned char protocol_value:1;
	unsigned char protocol_value_mask:1;
	unsigned char ipv4_dscp_value:1;
//<<< byte MSB 6th
}__attribute__ ((packed));


struct vacl_user_node_t
{
	//////// internal use variables ////////////////////////////////////////////////////
	unsigned int	crc;
	struct list_head list;
	unsigned short	ary_index; /* internal arrays stored position id */
	unsigned short	hw_order; /* used to commit into hw and further query via cli */
	unsigned short	hw_template_field_bit_range_vid; /* range_vid table entry position, points to range_vid_[lb,ub,type]_g arrays. It is template field value. */
	unsigned short	hw_template_field_bit_range_l4port; /* range_l4port table entry position, points to range_l4port_[lb,ub,type]_g arrays.  It is template field value. */
	unsigned short	hw_template_field_bit_range_ip; /* range_ip table entry position, points to range_ip_[lb,ub,type]_g arrays. It is template field value. */
	unsigned short	hw_template_field_bit_range_pktlen; /* range_pktlen table entry position, points to range_pktlen_[lb,ub,type]_g arrays. It is template field value. */
	unsigned short	hw_template_field_bit_field_selector; /* field selector, points to field_selector_[format|offset]_ary[] array. */
	unsigned short	template_index;
	unsigned short	template_field_use;
	unsigned short	ingress_range_ether_type_index;
	unsigned short	ingress_range_ether_type_number;
	union ingress_care_u {
		unsigned long long value; //fields relative to tempalte decisoin
		struct vacl_ingress_field_care_t bit;
	} ingress_care_u;

	//////// external use variables ////////////////////////////////////////////////////
	/******* each rule sw relative variable *******************************************/
	unsigned short	order; /* user defined rule order; same order entries which hw_order decided by added time sequence */
	// reverse mapping to rg weight; order=0 has the hightest priority
	unsigned char	valid:1;
	//////// CRC checksum include variables ////////////////////////////////////////////
	short		ingress_interface_index;
	unsigned int	ingress_active_portmask; /* rule source port, logical port map */
	/******* each rule action variables ***********************************************/
	unsigned long long	act_type;
	unsigned int	act_forward_portmask; /* for Action redirect port, logical port map */
	unsigned int	act_meter_bucket_size; /* bucket size(unit byte), <0~65535> */
	unsigned int	act_meter_rate; /* This value range is <8~1048568>. The granularity of rate is 8 Kbps. */
	short		hw_meter_table_entry; /* record hw meter table entry id. Its point to meter_[rate, burst, ifg]_g arrays */
	unsigned short	act_pri_data;
	unsigned int	act_statistic_index;
	unsigned short  act_cvlan_1p_remark;
	unsigned short  act_svlan_dscp_remark;

	/******* each rule field variables ************************************************/
	unsigned char ingress_dstmac_value[6], ingress_dstmac_mask[6];
	unsigned char ingress_srcmac_value[6], ingress_srcmac_mask[6];
	union {
		struct acl_uint16_range_t range;
		struct acl_uint16_value_t etype;
	} ingress_ether_type_u;
	union {
		struct acl_uint16_range_t vid_range;
		struct acl_uint16_value_t vid;
	} ingress_ctag_u;
	unsigned short	ingress_ctag_pri_value;
	unsigned short	ingress_ctag_pri_mask;
	unsigned short	ingress_ctag_cfi_value;
	unsigned short	ingress_ctag_cfi_mask;
	union {
		struct acl_uint16_range_t vid_range;
		struct acl_uint16_value_t vid;
	} ingress_stag_u;
	unsigned short	ingress_stag_pri_value;
	unsigned short	ingress_stag_pri_mask;
	unsigned short	ingress_stag_dei_value;
	unsigned short	ingress_stag_dei_mask;
	union {
		unsigned short	value;
		struct acl_uint16_range_t range; // reserved for future
	} ingress_ipv4_dscp_u;
	union {
		unsigned short	value;
		struct acl_uint16_range_t range; // reserved for future
	} ingress_ipv6_dscp_u;
	union {
		struct acl_ipv4addr_range_t range;
		struct acl_ipv4addr_value_t addr;
	} ingress_dipv4_u;
	union {
		struct acl_ipv4addr_range_t range;
		struct acl_ipv4addr_value_t addr;
	} ingress_sipv4_u;
	union {
		struct acl_ipv6addr_range_t range;
		struct acl_ipv6addr_value_t addr;
	} ingress_dipv6_u; /* for IPv6 the address only specify IPv6[31:0] */
	union {
		struct acl_ipv6addr_range_t range;
		struct acl_ipv6addr_value_t addr;
	} ingress_sipv6_u; /* for IPv6 the address only specify IPv6[31:0] */
	union {
		unsigned short	value; // reserved for future
		struct acl_uint16_range_t range;
	} ingress_l4dport_u;
	union {
		unsigned short	value; // reserved for future
		struct acl_uint16_range_t range;
	} ingress_l4sport_u;
	struct acl_uint16_range_t ingress_pktlen_range;
	unsigned short	ingress_gem_llid_value;
	unsigned short	ingress_gem_llid_mask;
	unsigned short  ingress_protocol_value;
	unsigned short  ingress_protocol_value_mask;
	unsigned char	invert:1;
	unsigned char	act_meter_ifg:1;
	unsigned char	ingress_pktlen_invert:1;
	unsigned char	ingress_care_tag_udp_value:1;
	unsigned char	ingress_care_tag_udp_mask:1;
	unsigned char	ingress_care_tag_tcp_value:1;
	unsigned char	ingress_care_tag_tcp_mask:1;
	unsigned char	ingress_care_tag_ipv6_value:1;
	unsigned char	ingress_care_tag_ipv6_mask:1;
	unsigned char	ingress_care_tag_ipv4_value:1;
	unsigned char	ingress_care_tag_ipv4_mask:1;
	unsigned char	ingress_care_tag_stag_value:1;
	unsigned char	ingress_care_tag_stag_mask:1;
	unsigned char	ingress_care_tag_ctag_value:1;
	unsigned char	ingress_care_tag_ctag_mask:1;
	unsigned char	ingress_care_tag_pppoe_value:1;
	unsigned char	ingress_care_tag_pppoe_mask:1;
}__attribute__ ((packed));

struct vacl_conf_t {
	unsigned short count_rules_max;
	unsigned short count_rules;
	unsigned short count_hw_rules;
	unsigned short count_hw_meter_rules;
	unsigned short mode128;
	unsigned short portmask_enable; /* logical port map */
	unsigned short portmask_permit; /* logical port map */
};

struct vacl_ll_head_t {
	unsigned short count;
	struct list_head list;
};

struct vacl_hw_node_t {
	unsigned short	ary_index; /* record index of rule_fvt2510_ary_p[] */
	unsigned short	hw_order; /* used to commit into hw and further query via hw interface */
	struct list_head list;
};

extern unsigned char is_acl_init_g;
extern struct vacl_conf_t vacl_conf_g;
extern unsigned short meter_entry_reserve_g;
extern struct vacl_ll_head_t rule_ll_head_user_ary_g[VACL_ORDER_TOTAL];
extern unsigned int meter_rate_ary_g[VACL_METER_ENTRY_TOTAL];
extern unsigned int meter_burst_ary_g[VACL_METER_ENTRY_TOTAL];
extern unsigned int meter_ifg_g;
extern unsigned short meter_ref_count_ary_g[VACL_METER_ENTRY_TOTAL];
extern unsigned short ingress_range_vid_lower_ary_g[INGRESS_RANGE_VID_INDEX_TOTAL];
extern unsigned short ingress_range_vid_upper_ary_g[INGRESS_RANGE_VID_INDEX_TOTAL];
extern unsigned int   ingress_range_vid_type_g;
extern unsigned short ingress_range_l4port_lower_ary_g[INGRESS_RANGE_L4PORT_INDEX_TOTAL]; //l4 port, 0~65535
extern unsigned short ingress_range_l4port_upper_ary_g[INGRESS_RANGE_L4PORT_INDEX_TOTAL]; //l4 port, 0~65535
extern unsigned int   ingress_range_l4port_type_g;  //0:dst-port 1:src-port; default 0
extern unsigned int   ingress_range_ip_lower_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL];
extern unsigned int   ingress_range_ip_upper_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL];
extern unsigned short ingress_range_ip_type_ary_g[INGRESS_RANGE_IP_INDEX_TOTAL];
extern unsigned short ingress_range_pktlen_lower_ary_g[INGRESS_RANGE_PKTLEN_INDEX_TOTAL];
extern unsigned short ingress_range_pktlen_upper_ary_g[INGRESS_RANGE_PKTLEN_INDEX_TOTAL];
extern unsigned short ingress_range_pktlen_invert_g;
extern unsigned short field_selector_commit_ary_g[VACL_FIELD_SELECTOR_TOTAL];
extern unsigned short field_selector_format_ary_g[VACL_FIELD_SELECTOR_TOTAL];
extern unsigned short field_selector_offset_ary_g[VACL_FIELD_SELECTOR_TOTAL];
extern unsigned short ingress_field_selector_ref_count_ary_g[VACL_FIELD_SELECTOR_TOTAL];
extern struct fwk_mutex_t vacl_nonrg_api_mutex;
extern struct fwk_mutex_t vacl_rg_api_mutex;

#define get_vacl_meter_entry_total()	(VACL_METER_ENTRY_TOTAL - meter_entry_reserve_g)

int meter_table_find_index(unsigned int bucket_size, unsigned int rate, unsigned char ifg, unsigned char action_share_meter, int *find_index, int *find_index_available);
void meter_check_fix_input(unsigned int *bucket_size, unsigned int *rate);
int action_set_meter(unsigned int bucket_size, unsigned int rate, unsigned char ifg, unsigned short *meter_index);

int vacl_init_check(void);
int vacl_user_node_init(struct vacl_user_node_t *rule_p);
/* below with mutex protection */
int vacl_add(struct vacl_user_node_t *rule_p, int *sub_order);
int vacl_del_hworder(int hw_order);
int vacl_del_order(int order, int *count);
int vacl_del(struct vacl_user_node_t *rule_p, int *hw_order);
int vacl_valid_crc(unsigned int crc, unsigned int valid);
int vacl_valid_hworder(int hw_order, unsigned int valid);
int vacl_is_enable(struct vacl_user_node_t *rule_p, int *disable_code);
void vacl_valid_all(unsigned int valid);
int _vacl_init(void);
int vacl_init(void);
int vacl_shutdown(void);
int vacl_del_all(void);
int vacl_del_crc(unsigned int crc, int *hw_order);
int _vacl_commit(void);
int vacl_commit(void); /* decide hw entry id, then commit to hw */

/* struct vacl_user_node_t helpers */
int vacl_port_enable_set(unsigned int portmask);
int vacl_port_permit_set(unsigned int portmask);

#endif
