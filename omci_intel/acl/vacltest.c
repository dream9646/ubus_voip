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
 * File    : vacltest.c
 *
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rtk/init.h>

#include "list.h"
#include "util.h"
#include "env.h"
#include "switch.h"
#include "vacl.h"
#include "vacl_util.h"


int
main(void)
{
	extern struct switch_hw_t switch_hw_fvt_g;
	int count = 0, ret, fd;
	struct vacl_user_node_t acl_rule;
	int sub_order;

	env_set_to_default();
	switch_hw_register(&switch_hw_fvt_g);
	fd = util_dbprintf_get_console_fd();
	rtk_core_init();
	vacl_init();
	switch_hw_fvt_g.init(0);
	switch_hw_fvt_g.vacl_init(0);

#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	vacl_smac_str_set(&acl_rule, "00:04:80:e4:53:90","ff:ff:ff:ff:ff:ff");
	acl_rule.act_type = VACL_ACT_FWD_DROP_MASK;
	acl_rule.active_portmask = 0x08;
	acl_rule.order = 132;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 1
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	vacl_ingress_dipv4_str_set(&acl_rule, "192.168.1.1", "255.255.255.255");
	acl_rule.act_type = VACL_ACT_LOG_POLICING_MASK;
	acl_rule.act_meter_bucket_size = 2048;
	acl_rule.act_meter_rate = 309600;
	acl_rule.act_meter_ifg = 0;
	acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE;
	acl_rule.ingress_active_portmask = 0x02;
	acl_rule.order = 132;
	ret = vacl_add(&acl_rule, &sub_order);
	if (ret != 0) printf("vacl_add() fail\n");
	else printf("sub_order=%d\n", sub_order);
#endif
#if 1
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
	vacl_user_node_init(&acl_rule);
	vacl_ingress_dipv4_str_set(&acl_rule, "192.168.1.2", "255.255.255.255");
	acl_rule.act_type = VACL_ACT_LOG_POLICING_MASK;
	acl_rule.act_meter_bucket_size = 1024;
	acl_rule.act_meter_rate = 102400;
	acl_rule.act_meter_ifg = 0;
	acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE;
	acl_rule.ingress_active_portmask = 0x02;
	acl_rule.order = 132;
	vacl_add(&acl_rule, &sub_order);
	if (ret != 0) printf("vacl_add() fail\n");
	else printf("sub_order=%d\n", sub_order);

	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
	vacl_user_node_init(&acl_rule);
	vacl_ingress_dipv4_str_set(&acl_rule, "192.168.1.3", "255.255.255.255");
	acl_rule.act_type = VACL_ACT_LOG_POLICING_MASK;
	acl_rule.act_meter_bucket_size = 2048;
	acl_rule.act_meter_rate = 309600;
	acl_rule.act_meter_ifg = 0;
	acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE;
	acl_rule.ingress_active_portmask = 0x02;
	acl_rule.order = 132;
	vacl_add(&acl_rule, &sub_order);
	if (ret != 0) printf("vacl_add() fail\n");
	else printf("sub_order=%d\n", sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.stag_u.vid.value = 47104;
	acl_rule.stag_u.vid.mask = 65535;
	acl_rule.care_u.bit.care_stag_val = 1;
	acl_rule.care_u.bit.care_stag_mask = 1;
	acl_rule.act_type = VACL_ACT_FWD_COPY_MASK;
	acl_rule.act_forward_portmask = 0xf;
	acl_rule.active_portmask = 0x04;
	acl_rule.order = 32;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.ctag_u.bound.lower = 111;
	acl_rule.ctag_u.bound.upper = 222;
	acl_rule.stag_u.bound.lower = 333;
	acl_rule.stag_u.bound.upper = 444;
	acl_rule.care_u.bit.care_ctag_vid_lb = 1;
	acl_rule.care_u.bit.care_ctag_vid_ub = 1;
	acl_rule.care_u.bit.care_stag_vid_lb = 1;
	acl_rule.care_u.bit.care_stag_vid_ub = 1;
	acl_rule.act_type = VACL_ACT_LOG_POLICING_MASK;
	acl_rule.act_meter_bucket_size = 1023;
	acl_rule.act_meter_rate = 204800;
	acl_rule.act_meter_ifg = 0;
	acl_rule.active_portmask = 0x08;
	acl_rule.order = 35;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.sport_u.bound.lower = 5555;
	acl_rule.sport_u.bound.upper = 6666;
	acl_rule.dport_u.bound.lower = 7777;
	acl_rule.dport_u.bound.upper = 8888;
	acl_rule.care_u.bit.care_sport_lb = 1;
	acl_rule.care_u.bit.care_sport_ub = 1;
	acl_rule.care_u.bit.care_dport_lb = 1;
	acl_rule.care_u.bit.care_dport_ub = 1;
	acl_rule.act_type = VACL_ACT_FWD_REDIRECT_MASK;
	acl_rule.act_forward_portmask = 0xf;
	acl_rule.active_portmask = 0x10;
	acl_rule.order = 36;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	vacl_dip6_bound_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "fe80::224:7eff:fede:ffff");
	vacl_sip6_bound_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "fe80::224:7eff:fede:ffff");
	vacl_dip_bound_str_set(&acl_rule, "192.168.1.1", "192.168.1.255");
	vacl_sip_bound_str_set(&acl_rule, "10.20.32.1", "10.20.32.255");
	acl_rule.act_type = VACL_ACT_FWD_IGR_MIRROR_MASK;
	acl_rule.act_forward_portmask = 0xf;
	acl_rule.active_portmask = 0x03;
	acl_rule.order = 37;
	vacl_add(&acl_rule, &sub_order);

	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.pktlen_bound.lower = 512;
	acl_rule.pktlen_bound.upper = 1518;
	acl_rule.pktlen_invert = 1;
	acl_rule.care_u.bit.care_pktlen_lb = 1;
	acl_rule.care_u.bit.care_pktlen_ub = 1;
	acl_rule.care_u.bit.care_pktlen_invert = 1;
	acl_rule.gem_llid_val = 0x1234;
	acl_rule.gem_llid_mask = 0xffff;
	acl_rule.care_u.bit.care_gem_llid_val = 1;
	acl_rule.care_u.bit.care_gem_llid_mask = 1;
	acl_rule.act_type = VACL_ACT_FWD_IGR_MIRROR_MASK;
	acl_rule.act_forward_portmask = 0xf;
	acl_rule.active_portmask = 0x03;
	acl_rule.order = 37;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	vacl_sip6_addr_str_set(&acl_rule, "fe80::224:7eff:fede:1d7b", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
	acl_rule.act_type = VACL_ACT_INTR_LATCH_MASK;
	acl_rule.active_portmask = (1<<2);
	acl_rule.order = 139;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.ether_type_u.etype.value = 0x0800;
	acl_rule.ether_type_u.etype.mask = 0xffff;
	acl_rule.care_u.bit.care_ether_type_val = 1;
	acl_rule.care_u.bit.care_ether_type_mask = 1;
	acl_rule.act_type = VACL_ACT_INTR_LATCH_MASK;
	acl_rule.active_portmask = 0x07;
	acl_rule.order = 100;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.pktlen_bound.lower = 512;
	acl_rule.pktlen_bound.upper = 1518;
	acl_rule.pktlen_invert = 1;
	acl_rule.care_u.bit.care_pktlen_lb = 1;
	acl_rule.care_u.bit.care_pktlen_ub = 1;
	acl_rule.care_u.bit.care_pktlen_invert = 1;
	acl_rule.gem_llid_val = 0x1234;
	acl_rule.gem_llid_mask = 0xffff;
	acl_rule.care_u.bit.care_gem_llid_val = 1;
	acl_rule.care_u.bit.care_gem_llid_mask = 1;
	acl_rule.act_type = VACL_ACT_FWD_TRAP_MASK;
	acl_rule.active_portmask = 0x0f;
	acl_rule.order = 38;
	vacl_add(&acl_rule, &sub_order);
#endif
#if 0
	memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
 	vacl_user_node_init(&acl_rule);
	acl_rule.ctag_u.vid.value = 60440;
	acl_rule.ctag_u.vid.mask = 65535;
	acl_rule.care_u.bit.care_ctag_val = 1;
	acl_rule.care_u.bit.care_ctag_mask = 1;
	acl_rule.act_type = VACL_ACT_FWD_DROP_MASK;
	acl_rule.active_portmask = (1<<3);
	acl_rule.order = 37;
	vacl_add(&acl_rule, &sub_order);
#endif
	//	vacl_commit();
	//	vacl_dump(1);

#ifndef	X86
	switch_hw_fvt_g.vacl_commit(0, &count);
	printf("%d rules hw_commit-ed.\n", count);
	switch_hw_fvt_g.vacl_dump(fd, 0);
#endif
	vacl_del_order(132, &count);
	printf("order 132, %d rules deleted.\n", count);
	switch_hw_fvt_g.vacl_dump(fd, 0);
	return 0;
}
