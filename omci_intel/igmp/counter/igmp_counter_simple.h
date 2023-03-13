/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp counter module - simple
 * File    : igmp_counter_simple.h
 *
 ******************************************************************/

/*
        This module is the port of original before counter modulelisation.
        MLD related are removed for symmetry e.g. no IGMP equivalent and
        simplicity. This module does basically:
        1. total trapped IGMP msg from LAN
        2. total trapped join msgs
        3. total trapped leave msgs
        4. total accepted join after all filters i.e. omci, traf, cutstom 
*/


#ifndef __IGMP_COUNTER_SIMPLE_H__
#define __IGMP_COUNTER_SIMPLE_H__

struct igmp_pkt_vid_msg_counter_t {
	unsigned short bridge_port_meid;
	unsigned int vid;
	unsigned int received_igmp;    // total igmp coming from WAN
	unsigned int general_query;    // general query
	unsigned int specific_query;   // group guery & group source specific query
	unsigned int total_join;       // total joins i.e. igmp v1/v2/v3 & mldv1 mldv2
	unsigned int successful_join;  // total accepted join type i.e. after filter
	unsigned int total_leave;      // totoal leve type i.e. igmp v2 leave & mld reduction
	struct list_head node;
};

extern struct igmp_counter_module_t simple;        
#endif