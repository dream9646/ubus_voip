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
#include <fcntl.h>
#include <dlfcn.h>

#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"

#include "intel_px126_util.h"
#include "pon_lib/fapi_pon.h"

int intel_px126_pon_get_state(int *state)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx  *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_gpon_status param = {0};

	
	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}
	fwk_mutex_lock(&libpob_pa->pa_lock);
	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	
	error = fapi_pon_gpon_status_get(pon_ctx, &param);

	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);


	switch (param.ploam_state) {
		case 10:
			//return "O1, Initial state";
		case 11:
			//return "O1.1, Initial off-sync state";
		case 12:
			//return "O1.2, Initial profile learning state";
			*state = 1;
			break;
		case 23:
			//return "O23, Serial number state";
			*state = 2;
			break;
		case 40:
			//return "O4, Ranging state";
			*state = 4;
			break;
		case 50:
			//return "O5, Operational state";
		case 51:
			//return "O5.1, Associated state";
		case 52:
			//return "O5.2, Pending state";
			*state = 5;
			break;
		case 60:
			//return "O6, Intermittent LOS state";
			*state = 6;
			break;
		case 70:
			//return "O7, Emergency stop state";
			*state = 7;
			break;
		case 80:
			//return "O8, Downstream tuning state";
		case 81:
			//return "O8.1, Downstream tuning off-sync state";
		case 82:
			//return "O8.2, Downstream tuning profile learning state";
			*state = 8;
			break;
		case 90:
			//return "O9, Upstream tuning state";
			*state = 9;
			break;
		default:
			return INTEL_PON_ERROR;
	
	}
	return INTEL_PON_SUCCESS;
}

int intel_prx126_pon_serial_number_get(char *vendor_id, unsigned int *serial_number)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_serial_number param = {0};

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_serial_number_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	UTIL_RETURN_IF_ERROR(error);
	vendor_id[0]= param.serial_no[0];
	vendor_id[1]= param.serial_no[1];
	vendor_id[2]= param.serial_no[2];
	vendor_id[3]= param.serial_no[3];

	*serial_number = 
		(param.serial_no[4]<<24) | (param.serial_no[5]<<16) |
			     (param.serial_no[6]<<8) | param.serial_no[7];
	
	return INTEL_PON_SUCCESS;

}
int intel_prx126_pon_password_get(char *password)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_password param = {0};

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_password_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	UTIL_RETURN_IF_ERROR(error);
	memcpy(password, param.password, 10);

	return 	INTEL_PON_SUCCESS;
}

int intel_prx126_pon_active(void)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_link_enable(pon_ctx);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return 	INTEL_PON_SUCCESS;
}

int intel_prx126_pon_deactive(void)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;

	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_link_disable(pon_ctx);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return 	INTEL_PON_SUCCESS;
}

#if 0
int intel_prx126_pon_optic_status_get(void)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx; 
	struct pon_optic_status optic_status;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_optic_status_get(pon_ctx, &optic_status);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}
#endif

int intel_prx126_pon_eeprom_open(const int ddmi_page, const char *filename)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_eeprom_open(pon_ctx, ddmi_page, filename);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);
	
	return INTEL_PON_SUCCESS;
}

int intel_prx126_pon_eeprom_data_get(const int ddmi_page,
				     unsigned char *data,
				     long offset,
				     size_t data_size)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_eeprom_data_get(pon_ctx, ddmi_page, data, offset, data_size);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_prx126_pon_eeprom_data_set(const int ddmi_page,
				     unsigned char *data,
				     long offset,
				     size_t data_size)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_eeprom_data_set(pon_ctx, ddmi_page, data, offset, data_size);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return PON_STATUS_OK;
}

int intel_prx126_gpon_alarm_status_get(unsigned int alarm_type, unsigned int *status)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_alarm_status param = { 0 };

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_alarm_status_get(pon_ctx, (unsigned short)alarm_type, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	*status = param.alarm_status;

	return INTEL_PON_SUCCESS;
}

int intel_prx126_gpon_tod_sync_mf_get(unsigned int *mf_cnt)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_gpon_tod_sync param;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	memset(&param, 0, sizeof(struct pon_gpon_tod_sync));
	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_gpon_tod_sync_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	*mf_cnt = param.multiframe_count;

	return INTEL_PON_SUCCESS;
}

