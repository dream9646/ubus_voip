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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_acl.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"
#include "switch.h"

#include "intel_px126_util.h"
#include <omci/pon_adapter_omci.h>
#include "omci/me/pon_adapter_pptp_ethernet_uni.h"

int intel_px126_port_inf_set(unsigned short meid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = uni_ops->create(np->ll_ctx,meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_port_inf_del(unsigned short meid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	
	fwk_mutex_lock(&np->pa_lock);

	error = uni_ops->destroy(np->ll_ctx,meid);

	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int intel_px126_port_inf_update(unsigned short meid,struct switch_pptp_eth_uni_data *updata)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = uni_ops->update(np->ll_ctx,meid,(struct pa_pptp_eth_uni_data*)updata);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_port_unlock(unsigned short meid)
{


	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = uni_ops->unlock(np->ll_ctx,meid );
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	

}
int intel_px126_port_lock(unsigned short meid)
{


	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = uni_ops->lock(np->ll_ctx,meid );
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	

}

int intel_px126_op_status_get(unsigned short meid,unsigned char * status)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_pptp_eth_uni_ops *uni_ops = NULL;
	int error;
	unsigned char oper_state = 0;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_eth_uni))  {
			uni_ops =
			    np->pa_ops->omci_me_ops->pptp_eth_uni;			
			break;
		}
	}	
	
	if (uni_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET uni_ops fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = uni_ops->oper_state_get(np->ll_ctx,meid ,&oper_state);
	fwk_mutex_unlock(&np->pa_lock);
	*status = oper_state;
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	
}

