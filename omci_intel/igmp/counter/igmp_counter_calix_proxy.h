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
 * Module  : igmp counter module - calix
 * File    : igmp_counter_calix.h
 *
 ******************************************************************/


#ifndef __IGMP_COUNTER_CALIX_PROXY_H__
#define __IGMP_COUNTER_CALIX_PROXY_H__

struct igmp_pkt_calix_proxy_counter_t {
	char vif[16];
        unsigned int rxGenQry1;
        unsigned int rxGenQry2;
        unsigned int rxGenQry3;
        unsigned int rxGrpQry2;
        unsigned int rxGrpQry3;
        unsigned int txGenQry1;
        unsigned int txGenQry2;
        unsigned int txGenQry3;
        unsigned int txGrpQry2;
        unsigned int txGrpQry3;
        unsigned int rxRpt1;
        unsigned int rxRpt2;
        unsigned int rxRpt3;
        unsigned int rxLeave;
        unsigned int txRpt1;
        unsigned int txRpt2;
        unsigned int txRpt3;
        unsigned int txLeave;        
};

extern struct igmp_counter_module_t calix_proxy; 
#endif