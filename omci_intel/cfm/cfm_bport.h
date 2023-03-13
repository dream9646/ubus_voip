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
 * Module  : cfm
 * File    : cfm_bport.h
 *
 ******************************************************************/
 
#ifndef __CFM_BPORT_H__
#define __CFM_BPORT_H__

/* cfm_bport.c */
struct me_t *cfm_bport_find(cfm_config_t *cfm_config);
struct me_t *cfm_bport_find_peer(cfm_config_t *cfm_config);
unsigned int cfm_bport_find_peer_index(cfm_config_t *cfm_config);
unsigned int cfm_bport_find_peer_logical_portmask(cfm_config_t *cfm_config);

#endif
