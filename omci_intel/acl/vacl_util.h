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
 * File    : vacl_util.h
 *
 ******************************************************************/

#ifndef __VACL_UTIL_H__
#define __VACL_UTIL_H__

#define BITWISE_2BYTE           0x8000
#define STR_ENABLE              "Enable"
#define STR_DISABLE             "Disable"
#define STR_SUCCESS             "Success"
#define STR_FAILURE   		"Failure"
#define STR_VALID     		"Valid"
#define STR_INVALID   		"Invalid"
#define STR_DROP                "Drop"
#define STR_COPY                "Forward Copy"
#define STR_REDIRECT            "Forward Redirect"
#define STR_MIRROR              "Forward Mirror"
#define STR_PERMIT              "Permit"
#define STR_ACLTRAP             "Trap-to-CPU"
#define STR_METER               "Meter"
#define STR_LATCH               "Latch"
#define STR_IPRI                "Internal Priority"
#define STR_DOT1P               "dot1p Remark"
#define STR_DSCP                "dscp Remark"
#define STR_POLICING            "Policing"
#define STR_ACLMIB              "Acl MIB"
#define STR_REVISE              "Revise"
#define STR_NOTREVISE           "Not Revise"
#define STR_STATISTIC           "Statistic"
#define RET_OK			0
#define RET_YES			1
#define RET_NO			0
#define print_rg_rule_header(fd, index)		util_fdprintf(fd, "#################### acl/cf rg entry[%d] ###################\n", index);
#define print_acl_rule_header(fd, index)	util_fdprintf(fd, "==================== acl asic entry(%d) ====================\n", index)
#define print_cf_rule_header(fd, index)		util_fdprintf(fd, "-------------------- cf asic entry(%d) ---------------------\n", index);

extern const char *enable_ary_s[], *valid_ary_s[], *act_forward_ary_s[], *act_police_ary_s[], *rule_revise_ary_s[], *ip_ary_s[];
extern unsigned int   ingress_range_ether_type_size_ary_g;
extern unsigned short ingress_range_ether_type_hwid_ary_g[];
extern unsigned short ingress_range_ether_type_value_ary_g[];
extern unsigned short ingress_range_ether_type_mask_ary_g[];

int which_bigger(int a, int b);
int is2pow(int num);
int get_id_num(char *token);
int vacl_dump(int fd, int hw_order, int order); /* list user acl entry and dump hw registers */
char *vacl_util_inet_mactoa(const unsigned char *mac);
char *vacl_util_inet_ntoa(unsigned int ina);
char *vacl_util_inet6_ntoa_tail4(unsigned int ina);
char *vacl_util_inet6_ntoa(const unsigned char *ipv6);
int vacl_util_mcast_bw_to_meter(unsigned int mcast_bandwidth, unsigned int *meter_rate);
unsigned int vacl_util_portidx_mask_max(void);
int vacl_ingress_dmac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
int vacl_ingress_smac_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
int vacl_ingress_dipv4_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
int vacl_ingress_sipv4_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
int vacl_ingress_dipv4_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
int vacl_ingress_sipv4_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
int vacl_ingress_sipv6_addr_str_set(struct vacl_user_node_t *rule_p, char *addr, char *mask);
int vacl_ingress_dipv6_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
int vacl_ingress_sipv6_range_str_set(struct vacl_user_node_t *rule_p, char *lb, char *ub);
int vacl_ingress_dscp_ipv4_value_set(struct vacl_user_node_t *rule_p, int value);
int vacl_util_multicast_node_compose(struct vacl_user_node_t *rule_p, unsigned int portmask, unsigned int meter_rate, int flag_ipv6);
char *vacl_ingress_care_tag_str_get(enum vacl_ingress_care_tag_e tag);
void vacl_user_dump(int fd, struct vacl_user_node_t *rule_p);
int vacl_dump_portmask(int fd);
int vacl_dump_portidx(int fd);

#endif
