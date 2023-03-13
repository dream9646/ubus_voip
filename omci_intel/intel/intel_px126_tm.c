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

#include "intel_px126_util.h"
#include "intel_px126_tm.h"

#include "omci/pon_adapter_omci.h"
#include "pon_lib/fapi_pon.h"

#include "omci/me/pon_adapter_traffic_scheduler.h"
#include "omci/me/pon_adapter_tcont.h"
#include "omci/me/pon_adapter_priority_queue.h"
#include "omci/me/pon_adapter_gem_port_network_ctp.h"
#include "omci/me/pon_adapter_dot1p_mapper.h"
#include "omci/me/pon_adapter_gem_interworking_tp.h"
#include "omci/me/pon_adapter_mac_bridge_port_config_data.h"
#include "omci/me/pon_adapter_multicast_gem_interworking_tp.h"


int intel_px126_tm_get_by_alloc_id(unsigned short allocid,int *tcont_id) 
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_allocation_index   out_param;
	
	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&libpob_pa->pa_lock);
	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	error = fapi_pon_alloc_id_get((struct pon_ctx*)pon_ctx,allocid,&out_param);

	if(error==PON_STATUS_OK)  // 221013 LEO add condition for tcont_id assignment
		*tcont_id = (int)out_param.alloc_index;

	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (error == PON_STATUS_ALLOC_ID_MISSING)
		return INTEL_PON_ERROR;
	if(error != 0){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"Get alloc id %d error=%d\n",allocid, error);
	}

	// 221013 LEO START suppress error message when no allocid
	return error;
	// 221013 LEO END
}

int intel_px126_tm_get_allocid_by_tmidex(unsigned short tmidex,unsigned short *allocid) 
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_allocation_id param_out = {0};
	
	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}
	fwk_mutex_lock(&libpob_pa->pa_lock);
	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	
	error = fapi_pon_alloc_index_get(pon_ctx,tmidex,&param_out);
	*allocid = (unsigned short)param_out.alloc_id;
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_tm_updtae_tcont(unsigned short meid,unsigned short policy,unsigned short alloc_id,unsigned short is_create) 
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_tcont_ops *p_tcont = NULL;
	int error;
		
	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			tcont))  {
			p_tcont =
			    np->pa_ops->omci_me_ops->tcont;
			
			if (PA_EXISTS(p_tcont, update) &&
			    PA_EXISTS(p_tcont, destroy) )
				break;
		}
	}	

	if (p_tcont == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	if(is_create)
		error = p_tcont->update(np->ll_ctx, meid,policy,alloc_id,1);
	else
		error = p_tcont->update(np->ll_ctx, meid,policy,alloc_id,0);
	fwk_mutex_unlock(&np->pa_lock);

	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	
}

int intel_px126_tm_destory_tcont(unsigned short meid,unsigned short alloc_id) 
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_tcont_ops *tcont = NULL;
	int error;
		
	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			tcont))  {
			tcont =
			    np->pa_ops->omci_me_ops->tcont;
			
			if (PA_EXISTS(tcont, update) &&
			    PA_EXISTS(tcont, destroy) )
				break;
		}
	}	

	if (tcont == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);

	error = tcont->destroy(np->ll_ctx, meid,alloc_id,0);
	
	fwk_mutex_unlock(&np->pa_lock);
	
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	
}
int intel_px126_tm_updtae_ts(unsigned short meid,struct px126_traffic_scheduler_update_data *update_data,unsigned short is_create)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_traffic_scheduler_ops *ts = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			traffic_scheduler))  {
			ts =
			    np->pa_ops->omci_me_ops->traffic_scheduler;			
			break;
		}
	}	
	
	if (ts == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET TS FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	if (is_create)
		error = ts->create(np->ll_ctx,meid,(struct pa_traffic_scheduler_update_data*)update_data);
	else
		error = ts->update(np->ll_ctx,meid,(struct pa_traffic_scheduler_update_data*)update_data);

	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int intel_px126_tm_destory_ts(unsigned short meid)
{
	
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_traffic_scheduler_ops *ts = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			traffic_scheduler))  {
			ts =
			    np->pa_ops->omci_me_ops->traffic_scheduler; 		
			break;
		}
	}	
	
	if (ts == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"del TS FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = ts->destroy(np->ll_ctx,meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int intel_px126_tm_updtae_queue(unsigned short meid,struct px126_priority_queue_update_data *update_data,unsigned short is_create)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_priority_queue_ops *tqueue = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			priority_queue))  {
			tqueue =
			    np->pa_ops->omci_me_ops->priority_queue;			
			break;
		}
	}	
	
	if (tqueue == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET priority_queue FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	if (is_create)
		error = tqueue->create(np->ll_ctx,meid,(struct pa_priority_queue_update_data*)update_data);
	else
		error = tqueue->update(np->ll_ctx,meid,(struct pa_priority_queue_update_data*)update_data);

	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_tm_destory_queue(unsigned short meid, struct px126_priority_queue_update_data *update_data)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_priority_queue_ops *tqueue = NULL;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			priority_queue))  {
			tqueue =
			    np->pa_ops->omci_me_ops->priority_queue;
			break;
		}
	}

	if (tqueue == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET priority_queue FAIL\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = tqueue->destroy(np->ll_ctx, meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int intel_px126_tm_del_queue(unsigned short meid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_priority_queue_ops *tqueue = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			priority_queue))  {
			tqueue =
			    np->pa_ops->omci_me_ops->priority_queue;			
			break;
		}
	}	
	
	if (tqueue == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET priority_queue FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	error = tqueue->destroy(np->ll_ctx,meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}
int intel_px126_tm_usflow_get(int usflow_id, struct gpon_tm_usflow_config_t *usflow_config)
{
	
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_gem_port param_out = {0};
	
	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_gem_port_index_get(pon_ctx,usflow_id,&param_out);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	
	if (error == PON_STATUS_GEM_PORT_ID_NOT_EXISTS_ERR) {
		///util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"gem_port_index_get=%d\n",usflow_id);
		//quite to return
		return error;
	}
	UTIL_RETURN_IF_ERROR(error);

	usflow_config->gemport = param_out.gem_port_id; 
	
	if(param_out.is_upstream)
		usflow_config->enable = 1;
	
	if(param_out.payload_type == 0)/*0 is ethernet,1 is OMCI*/
		usflow_config->flow_type = 1;
	else if(param_out.payload_type == 1)
		usflow_config->flow_type = 0;
	
	return INTEL_PON_SUCCESS;
}
int intel_px126_tm_dsflow_get(int dsflow_id, struct gpon_tm_dsflow_config_t *dsflow_config)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_gem_port param_out = {0};
	
	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_gem_port_index_get(pon_ctx,dsflow_id,&param_out);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (error == PON_STATUS_GEM_PORT_ID_NOT_EXISTS_ERR)
		return INTEL_PON_ERROR;

	UTIL_RETURN_IF_ERROR(error);

	dsflow_config->gemport = param_out.gem_port_id; 
	
	if(param_out.is_downstream)
		dsflow_config->enable = 1;
	
	if(param_out.payload_type == 0)/*0 is ethernet,1 is OMCI*/
		dsflow_config->flow_type = 1;
	else if(param_out.payload_type == 1)
		dsflow_config->flow_type = 0;

	/** Encryption key ring.
	 *  This value is used for XG-PON, XGS-PON, and NG-PON2 only
	 *  and otherwise ignored.
	 *  - 0: None, No encryption. The downstream key index is ignored,
	 *       and upstream traffic is transmitted with key index 0.
	 *  - 1: Unicast, Unicast payload encryption in both directions.
	 *       Keys are generated by the ONU and transmitted to the
	 *       OLT via the PLOAM channel.
	 *  - 2: Broadcast, Broadcast (multicast) encryption. Keys are
	 *       generated by the OLT and distributed via the OMCI.
	 *  - 3: Unicast downstream, Unicast encryption in downstream only.
	 *       Keys are generated by the ONU and transmitted to the OLT via
	 *       the PLOAM channel.
	 *  - Other: Reserved, Ignore and do not use.
	 */
	if(param_out.encryption_key_ring == 0)
		dsflow_config->aes_enable = 0;
	else if(param_out.encryption_key_ring == 1 ){
		dsflow_config->aes_enable = 1;
		dsflow_config->mcast_enable = 0;
	}else if(param_out.encryption_key_ring == 2 ){
		dsflow_config->aes_enable = 1;
		dsflow_config->mcast_enable = 1;
	}
	else if(param_out.encryption_key_ring == 3 ){
		dsflow_config->aes_enable = 1;
		dsflow_config->mcast_enable = 0;
	}
	return INTEL_PON_SUCCESS;
}
int intel_px126_tm_usflow_set(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_port_net_ctp_ops *ctp_ops = NULL;
	struct pa_gem_port_net_ctp_update_data *upd_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_port_net_ctp))  {
			ctp_ops =
			    np->pa_ops->omci_me_ops->gem_port_net_ctp;			
			break;
		}
	}	
	
	if (ctp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET ctp_ops fail\n");
		return INTEL_PON_ERROR;
	}

	upd_data = (struct pa_gem_port_net_ctp_update_data *)usflow_config;
	fwk_mutex_lock(&np->pa_lock);
	error = ctp_ops->update(np->ll_ctx,usflow_id,upd_data);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_tm_dsflow_set(int dsflow_id,  struct gem_port_net_ctp_update_data *dsflow_config)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_port_net_ctp_ops *ctp_ops = NULL;
	struct pa_gem_port_net_ctp_update_data *upd_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_port_net_ctp))  {
			ctp_ops =
			    np->pa_ops->omci_me_ops->gem_port_net_ctp;			
			break;
		}
	}	
	
	if (ctp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET ctp_ops fail\n");
		return INTEL_PON_ERROR;
	}

	upd_data = (struct pa_gem_port_net_ctp_update_data *)dsflow_config;

	fwk_mutex_lock(&np->pa_lock);
	error = ctp_ops->update(np->ll_ctx,dsflow_id,upd_data);
	fwk_mutex_unlock(&np->pa_lock);
	
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int intel_px126_tm_usflow_del(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_port_net_ctp_ops *ctp_ops = NULL;
	struct pa_gem_port_net_ctp_destroy_data upd_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_port_net_ctp))  {
			ctp_ops =
			    np->pa_ops->omci_me_ops->gem_port_net_ctp;			
			break;
		}
	}	
	
	if (ctp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET ctp_ops fail\n");
		return INTEL_PON_ERROR;
	}

	upd_data.gem_port_id = usflow_config->gem_port_id;
	upd_data.us_traffic_descriptor_profile_ptr = usflow_config->us_traffic_descriptor_profile_ptr;
	upd_data.ds_traffic_descriptor_profile_ptr = usflow_config->ds_traffic_descriptor_profile_ptr;
	fwk_mutex_lock(&np->pa_lock);
	error = ctp_ops->destroy(np->ll_ctx,usflow_id,&upd_data);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_px126_tm_dsflow_del(int dsflow_id,  struct gem_port_net_ctp_update_data *dsflow_config)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_port_net_ctp_ops *ctp_ops = NULL;
	struct pa_gem_port_net_ctp_destroy_data upd_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_port_net_ctp))  {
			ctp_ops =
			    np->pa_ops->omci_me_ops->gem_port_net_ctp;			
			break;
		}
	}	
	
	if (ctp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET ctp_ops fail\n");
		return INTEL_PON_ERROR;
	}

	upd_data.gem_port_id = dsflow_config->gem_port_id;
	upd_data.us_traffic_descriptor_profile_ptr = dsflow_config->us_traffic_descriptor_profile_ptr;
	upd_data.ds_traffic_descriptor_profile_ptr = dsflow_config->ds_traffic_descriptor_profile_ptr;
	fwk_mutex_lock(&np->pa_lock);
	error = ctp_ops->destroy(np->ll_ctx,dsflow_id,&upd_data);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int
intel_px126_tm_flow_8021p_set(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_dot1p_mapper_ops *dot1p_ops = NULL;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			dot1p_mapper))  {
			dot1p_ops =
			    np->pa_ops->omci_me_ops->dot1p_mapper;			
			break;
		}
	}	
	
	if (dot1p_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET dot1p_mapper fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = dot1p_ops->update(np->ll_ctx,meid,(struct pa_dot1p_mapper_update_data*)upd_cfg);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
	
}

int
intel_px126_tm_flow_8021p_del(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_dot1p_mapper_ops *dot1p_ops = NULL;
	struct pa_dot1p_mapper_destroy_data dot1p_mapper_data;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			dot1p_mapper))  {
			dot1p_ops =
			    np->pa_ops->omci_me_ops->dot1p_mapper;
			break;
		}
	}

	if (dot1p_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET dot1p_mapper fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	memset(&dot1p_mapper_data, 0x0, sizeof(struct pa_dot1p_mapper_destroy_data));
	dot1p_mapper_data.tp_pointer = 0;
	dot1p_mapper_data.tp_pointer_type = upd_cfg->tp_pointer_type;
	error = dot1p_ops->destroy(np->ll_ctx, meid, &dot1p_mapper_data);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


int
intel_px126_tm_flow_iwtp_set(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg)
{

	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_interworking_tp_ops *iwtp_ops = NULL;
	struct pa_gem_interworking_tp_update_data  tp_update_data;
	struct pa_bridge_data bridge_data;
	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_itp))  {
			iwtp_ops =
			    np->pa_ops->omci_me_ops->gem_itp;			
			break;
		}
	}	
	
	if (iwtp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET iwtp fail\n");
		return INTEL_PON_ERROR;
	}

	tp_update_data.interworking_option = upd_cfg->interworking_option;
	/** Service profile pointer */
	tp_update_data.service_profile_pointer = upd_cfg->service_profile_pointer ;
	/** Interworking termination point pointer */
	tp_update_data.interworking_tp_pointer = upd_cfg->interworking_tp_pointer;
	/** GAL loopback configuration */
	tp_update_data.gal_loopback_configuration = upd_cfg->gal_loopback_configuration;

	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"meid=%d,gemid=%d,GET iwtp IWTP=%d sp=%d tp=%d gal=%d\n",meid,upd_cfg->gem_port_id,tp_update_data.interworking_option
	//	,tp_update_data.service_profile_pointer,tp_update_data.interworking_tp_pointer,tp_update_data.gal_loopback_configuration);
	
	memcpy(&bridge_data,&upd_cfg->bridge_data,sizeof(struct pa_bridge_data));
	//{
	//	int i;
	//	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"bridge counter=%d,meid=%d\n",bridge_data.count,bridge_data.me_id);
	//	
	//	for(i=0;i<bridge_data.count;i++){
	//		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"port[%d].me_id=%d,tp_type=%d,tp_ptr=%d\n",i
	//			     ,bridge_data.port[i].me_id,bridge_data.port[i].tp_type,bridge_data.port[i].tp_ptr);
	//	}
	//}
	fwk_mutex_lock(&np->pa_lock);
	error = iwtp_ops->update(np->ll_ctx,meid,upd_cfg->gem_port_id,&bridge_data
		,upd_cfg->gal_payload_len,&tp_update_data);
	fwk_mutex_unlock(&np->pa_lock);
	
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int
intel_px126_tm_flow_iwtp_del(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg)
{

	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_gem_interworking_tp_ops *iwtp_ops = NULL;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			gem_itp))  {
			iwtp_ops =
			    np->pa_ops->omci_me_ops->gem_itp;
			break;
		}
	}

	if (iwtp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET iwtp fail\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	error = iwtp_ops->destroy(np->ll_ctx, meid, upd_cfg->gem_port_id, upd_cfg->interworking_option, upd_cfg->service_profile_pointer);
	fwk_mutex_unlock(&np->pa_lock);

	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int
intel_px126_tm_flow_mcastiwtp_set(unsigned short meid, unsigned short ctp_ptr)
{

	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_gem_itp_ops *mcastiwtp_ops = NULL;

	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_gem_itp))  {
			mcastiwtp_ops =
			    np->pa_ops->omci_me_ops->mc_gem_itp;			
			break;
		}
	}	
	
	if (mcastiwtp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mciwtp fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = mcastiwtp_ops->update(np->ll_ctx,meid,ctp_ptr);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
int
intel_px126_tm_flow_mcastiwtp_destory(unsigned short meid)
{
	struct intel_omci_context *context = intel_omci_get_omci_context();
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mc_gem_itp_ops *mcastiwtp_ops = NULL;

	int error;

	
	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
	
		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mc_gem_itp))  {
			mcastiwtp_ops =
			    np->pa_ops->omci_me_ops->mc_gem_itp;			
			break;
		}
	}	
	
	if (mcastiwtp_ops == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET mciwtp fail\n");
		return INTEL_PON_ERROR;
	}
	fwk_mutex_lock(&np->pa_lock);
	error = mcastiwtp_ops->destroy(np->ll_ctx,meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


