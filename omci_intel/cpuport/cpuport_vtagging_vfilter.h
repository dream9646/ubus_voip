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
 * Module  : cpuport
 * File    : cpuport_vtagging_vfilter.h
 *
 ******************************************************************/

#ifndef __CPUPORT_VTAGGING_VFILTER_H__
#define __CPUPORT_VTAGGING_VFILTER_H__

#define CPUPORT_VTAGGING_VFILTER_BLOCK		9
#define CPUPORT_VTAGGING_VFILTER_PASS		8
#define CPUPORT_VTAGGING_RULE_MATCH 		7
#define CPUPORT_VTAGGING_RULE_UNMATCH 		6
#define CPUPORT_VTAGGING_TREATMENT_4TAGS	5
#define CPUPORT_VTAGGING_TREATMENT_3TAGS	4
#define CPUPORT_VTAGGING_TREATMENT_SUCCESS	3
#define CPUPORT_VTAGGING_TREATMENT_FAIL		2
#define CPUPORT_VFILTER_PASS			1
#define CPUPORT_VFILTER_BLOCK			0

#include "omciutil_vlan.h"

/* cpuport_vtagging_vfilter.c */
int cpuport_vtagging_match(struct cpuport_tci_t *src_tci, struct omciutil_vlan_rule_fields_t *filter_rule);
int cpuport_vtagging_treatment(struct cpuport_tci_t *src_tci, struct omciutil_vlan_rule_fields_t *tag_rule, struct cpuport_tci_t *dst_tci);
int cpuport_vfilter(struct me_t *src_port, int is_ingress, struct cpuport_tci_t *src_tci);
int cpuport_vtagging_vfilter(struct me_t *src_port, struct me_t *dst_port, struct cpuport_tci_t *src_tci, struct cpuport_tci_t *dst_tci, struct cpuport_info_t *cpuport_info);

#endif
