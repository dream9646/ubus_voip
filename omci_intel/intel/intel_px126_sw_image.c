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
 * Module  : intel_prx126
 * File    : intel_prx126_sw_image.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>

#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"
#include "intel_px126_util.h"
#include "intel_px126_sw_image.h"

#include <omci/pon_adapter_omci.h>
#include "omci/me/pon_adapter_sw_image.h"


extern struct intel_omci_context g_intel_omci_context;


int intel_omci_sw_image_version_get(unsigned char id, unsigned char version_size,  char *version)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, version_get))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->version_get(np->ll_ctx, id, version_size, version);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}



int intel_omci_sw_image_valid_get(unsigned char id, unsigned char *valid)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, valid_get))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->valid_get(np->ll_ctx, id, valid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}



int intel_omci_sw_image_active_get(unsigned char id, unsigned char *active)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, active_get))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->active_get(np->ll_ctx, id, active);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

int intel_omci_sw_image_active_set(unsigned char id, unsigned int timeout)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, activate))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->activate(np->ll_ctx, id, timeout);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}



int intel_omci_sw_image_commit_get(unsigned char id, unsigned char *commit)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, commit_get))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->commit_get(np->ll_ctx, id, commit);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int intel_omci_sw_image_commit_set(unsigned char id)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, commit))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->commit(np->ll_ctx, id);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}



int intel_omci_sw_image_download_start(unsigned char id, unsigned int size)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, download_start))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->download_start(np->ll_ctx, id, size);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


int intel_omci_sw_image_download_stop(unsigned char id)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, download_stop))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	error = bsp->download_stop(np->ll_ctx, id);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


int intel_omci_sw_image_download_end(unsigned char id, unsigned int size, unsigned int crc, unsigned char filepath_size, char *filepath)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, download_end))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->download_end(np->ll_ctx, id, size, crc, filepath_size, filepath);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


int intel_omci_sw_image_download_handle_window(unsigned char id, unsigned short int window_id, unsigned char *data, unsigned int size)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, handle_window))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}


	fwk_mutex_lock(&np->pa_lock);
	error = bsp->handle_window(np->ll_ctx, id, window_id, data, size);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}


int intel_omci_sw_image_download_store(unsigned char id, unsigned char filepath_size, char *filepath)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_sw_image_ops * bsp;
	int error;


	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			sw_image))  {
			bsp =
			    np->pa_ops->omci_me_ops->sw_image;

			if (PA_EXISTS(bsp, store))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	error = bsp->store(np->ll_ctx, id, filepath_size, filepath);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;

}

