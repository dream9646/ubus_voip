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
 * Module  : igmp counter module - calix_15mins
 * File    : igmp_counter_calix_15mins.h
 *
 ******************************************************************/


#ifndef __IGMP_COUNTER_CALIX__15MINS_H__
#define __IGMP_COUNTER_CALIX__15MINS_H__

struct igmp_pkt_calix_15mins_t {
	int timer_g;
	long timestamp;
        unsigned int rxGenQry1;
        unsigned int rxGenQry2;
        unsigned int rxGenQry3;
        unsigned int rxGrpQry2;
        unsigned int rxGrpQry3;
        unsigned int rxRpt1;
        unsigned int rxRpt2;
        unsigned int rxRpt3;
        unsigned int rxLeave;
		
};

extern struct igmp_counter_module_t calix_15mins; 
#endif