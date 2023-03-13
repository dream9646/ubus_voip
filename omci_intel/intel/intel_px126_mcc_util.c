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

#include "list.h"
#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"
#include "intel_px126_util.h"
#include "intel_px126_mcc_util.h"

#include "omci/pon_adapter_omci.h"
#include "omci/pon_adapter_mcc.h"
#include "omci/me/pon_adapter_multicast_operations_profile.h"

int  intel_mcc_dev_vlan_unaware_mode_enable( const bool enable)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;

	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->vlan_unaware_mode_set(mcc->ll_ctx ,enable);
	fwk_mutex_unlock(&mcc->mcc_lock);
					  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC ivlan_unaware_mode_set fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;
}

int intel_mcc_pkt_receive(struct mcc_pkt * mcc_pkt)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->pkt_receive(mcc->ll_ctx,
					  mcc_pkt->data, &mcc_pkt->len,
					  &mcc_pkt->llinfo);
	fwk_mutex_unlock(&mcc->mcc_lock);
					  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC pkt_receive fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;
}

int intel_mcc_pkt_send(struct mcc_pkt * mcc_pkt)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->pkt_send(mcc->ll_ctx,
					  mcc_pkt->data, mcc_pkt->len,
					  &mcc_pkt->llinfo);

	fwk_mutex_unlock(&mcc->mcc_lock);				  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC pkt_send fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;
}

int intel_mcc_dev_port_add(enum px126_mcc_dir dir ,unsigned char lan_port,unsigned char fid,union px126_mcc_ip_addr *ip)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->port_add(mcc->ll_ctx,dir
					  ,lan_port,fid,
					  (union pa_mcc_ip_addr*)ip);

	fwk_mutex_unlock(&mcc->mcc_lock);				  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC intel_mcc_dev_port_adds fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;

}
int intel_mcc_dev_port_remove(unsigned char lan_port,unsigned char fid,union px126_mcc_ip_addr *ip)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->port_remove(mcc->ll_ctx ,lan_port,fid, (union pa_mcc_ip_addr*)ip);

	fwk_mutex_unlock(&mcc->mcc_lock);				  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC intel_mcc_dev_port_remove fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;

}
int intel_mcc_dev_port_activity_get(unsigned char lport, unsigned char fid,union px126_mcc_ip_addr *ip,  unsigned char *is_active)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->port_activity_get(mcc->ll_ctx ,lport,fid, (union pa_mcc_ip_addr*)ip,(bool *)is_active);
	fwk_mutex_unlock(&mcc->mcc_lock);
					  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC intel_mcc_dev_port_activity_get fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;
}
int intel_mcc_dev_fid_get(unsigned short o_vid,	unsigned char *fid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct mcc_ctx *mcc;
	int error;
	if(!context->mcc_init_done){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"mcc_init_done not done\n");
		return INTEL_PON_ERROR;
	}
	mcc = (struct mcc_ctx *)context->mcc;
	fwk_mutex_lock(&mcc->mcc_lock);
	error =(int) mcc->mcc_ops->fid_get(mcc->ll_ctx ,o_vid,fid);
	fwk_mutex_unlock(&mcc->mcc_lock);

					  
	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"MCC intel_mcc_dev_fid_get fail\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;

}
int intel_mcc_me309_create(unsigned short meid,unsigned short igmp_ver)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_profile_ops *mc_profile_ops = NULL;

	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_profile))  {
			mc_profile_ops =
			    np->pa_ops->omci_me_ops->mc_profile;			
			break;
		}
	}	
	
	if (mc_profile_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mc profile fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = mc_profile_ops->create(np->ll_ctx,meid,igmp_ver);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;	
}
int intel_mcc_me309_del(unsigned short meid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_profile_ops *mc_profile_ops = NULL;

	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_profile))  {
			mc_profile_ops =
			    np->pa_ops->omci_me_ops->mc_profile;			
			break;
		}
	}	
	
	if (mc_profile_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mc profile fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = mc_profile_ops->destroy(np->ll_ctx,meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;	
}
int intel_mcc_me310_extvlan_set(struct switch_mc_profile_ext_vlan_update_data *up_data)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_profile_ops *mc_profile_ops = NULL;
	struct pa_mc_profile_ext_vlan_update_data update_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_profile))  {
			mc_profile_ops =
			    np->pa_ops->omci_me_ops->mc_profile;			
			break;
		}
	}	
	
	if (mc_profile_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mc profile fail\n");
		return INTEL_PON_ERROR;
	}

	/** LAN port index (0 to 3) */
	update_data.lan_idx = up_data->lan_idx;
	update_data.us_igmp_tag_ctrl = up_data->us_igmp_tag_ctrl;
	update_data.us_igmp_tci = up_data->us_igmp_tci;
	update_data.ds_igmp_mc_tag_ctrl = up_data->ds_igmp_mc_tag_ctrl;
	update_data.ds_igmp_mc_tci = up_data->ds_igmp_mc_tci;
	fwk_mutex_lock(&np->pa_lock);
	error = mc_profile_ops->mc_ext_vlan_update(np->ll_ctx,up_data->meid,&update_data);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;	
}

int intel_mcc_me310_extvlan_clear(unsigned short meid,unsigned short lan_port_idx)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_profile_ops *mc_profile_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_profile))  {
			mc_profile_ops =
			    np->pa_ops->omci_me_ops->mc_profile;			
			break;
		}
	}	
	
	if (mc_profile_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mc profile fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);

	error = mc_profile_ops->mc_ext_vlan_clear(np->ll_ctx,meid,lan_port_idx);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;	
}

int intel_mcc_init(struct intel_omci_context *context)
{
	int error = INTEL_PON_SUCCESS;
	struct mcc_ctx *mcc;
	struct pa_node *np = NULL,*n = NULL;
	int i;


	/* allocate mcc context */
	mcc = malloc_safe(sizeof(*mcc));
	if(mcc == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR MCC no memory\n");
		return INTEL_PON_ERROR;
	}

	/* init mcc context */
	memset(mcc, 0, sizeof(*mcc));
	context->mcc = mcc;

	mcc->ctx_core = context;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){	
		if (PA_EXISTS(np->pa_ops, omci_mcc_ops)) {
			struct pa_omci_mcc_ops const *mcc_ops =
				np->pa_ops->omci_mcc_ops;
			/* Check if all required functions are available */
			if (PA_EXISTS(mcc_ops, init) &&
			    PA_EXISTS(mcc_ops, shutdown) &&
			    PA_EXISTS(mcc_ops, pkt_receive) &&
			    PA_EXISTS(mcc_ops, pkt_receive_cancel) &&
			    PA_EXISTS(mcc_ops, pkt_send) &&
			    PA_EXISTS(mcc_ops, fid_get) &&
			    PA_EXISTS(mcc_ops, vlan_unaware_mode_set) &&
			    PA_EXISTS(mcc_ops, fwd_update) &&
			    PA_EXISTS(mcc_ops, port_add) &&
			    PA_EXISTS(mcc_ops, port_remove) &&
			    PA_EXISTS(mcc_ops, port_activity_get))
				break;
		}
	}
	
	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"NO  MCC operation point\n");
		return INTEL_PON_ERROR;
	}
	mcc->mcc_ops = np->pa_ops->omci_mcc_ops;
	mcc->ll_ctx = np->ll_ctx;
	
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"MCC: Using ops from %s", np->definition->name);

	/* mcc lower layer init */
	error = mcc->mcc_ops->init(mcc->ll_ctx, &mcc->max_ports);
	if(error!=INTEL_PON_SUCCESS){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR MCC init\n");
		return INTEL_PON_ERROR;
	}

	mcc->port = malloc_safe(sizeof(struct mcc_port) * mcc->max_ports);

	if(mcc->port == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR MCC port no memory\n");
		return INTEL_PON_ERROR;
	}
	
	memset(mcc->port, 0, sizeof(struct mcc_port) * mcc->max_ports);
	fwk_mutex_create(&mcc->mcc_lock);
	
	for (i = 0; i < mcc->max_ports; i++) {
		fwk_mutex_create(&mcc->port[i].lock);

		/* init Flow list */
		INIT_LIST_HEAD(&mcc->port[i].flw_list.head);
		mcc->port[i].flw_list.num = 0;

		/* init Rate Limiter */
		INIT_LIST_HEAD(&mcc->port[i].rl.rl_list.head);
		mcc->port[i].rl.rl_list.num = 0;
		mcc->port[i].rl.type = MCC_RL_TYPE_NA;
	}
	context->mcc_init_done = 1;
	/* set default VLAN mode */
	error = intel_mcc_dev_vlan_unaware_mode_enable(MCC_VLAN_MODE_AWARE);
	if(error!=INTEL_PON_SUCCESS){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR SET VLAN mode\n");
		return INTEL_PON_ERROR;
	}
	
	return error;
}
int  intel_mcc_exit(struct intel_omci_context *context)
{
	int error;
	struct mcc_ctx *mcc = (struct mcc_ctx *)context->mcc;
	int i;

	if(mcc == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"MCC NULL POINT\n");
		return INTEL_PON_ERROR;
	}


	if (mcc->mcc_ops) {
		error = mcc->mcc_ops->shutdown(mcc->ll_ctx);
		if(error!=INTEL_PON_SUCCESS){
			util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"MCC SHUTDOWN ERROR\n");
			return INTEL_PON_ERROR;
		}
	}
	fwk_mutex_destroy(&mcc->mcc_lock);
	if (mcc->port) {
		for (i = 0; i < mcc->max_ports; i++)
			fwk_mutex_destroy(&mcc->port[i].lock);

		free_safe(mcc->port);
	}
	free_safe(mcc);
	context->mcc = NULL;

	return error;
}

