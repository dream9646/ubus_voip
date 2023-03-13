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
 * Module  : gpon_hw_prx126
 * File    : intel_px126_pm.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>

#include "fwk_mutex.h"
#include "util.h"

#include "intel_px126_util.h"
#include "intel_px126_pm.h"
#include "pon_lib/fapi_pon.h"
#include <linux/ethtool.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
int intel_px126_pm_ifname_to_lan_index_get(const char *ifname)
{
	struct pa_node *libpob_pa;
	unsigned char lan_index;
	void *pon_net_ctx;
	ifname_to_lan_idx_t *ifname_to_lan_idx = NULL;
	char symbol_name[256] = "";

	if (ifname == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libponnet");

	if(libpob_pa == NULL || libpob_pa->dl_handle == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	snprintf(symbol_name, sizeof(symbol_name), "ifname_to_lan_idx");

	pon_net_ctx = (void *)libpob_pa->ll_ctx;
	ifname_to_lan_idx = dlsym(libpob_pa->dl_handle, symbol_name);

	if (pon_net_ctx==NULL || ifname_to_lan_idx==NULL)
		return INTEL_PON_ERROR;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	lan_index = ifname_to_lan_idx(pon_net_ctx, ifname); // classid: 11
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (lan_index == 0xFF)
		return INTEL_PON_ERROR;

	return (int)lan_index;
}

int intel_px126_pm_ifname_get( unsigned short class_id, unsigned short me_id, char *ifname, int size)
{
	struct pa_node *libpob_pa;
	enum fapi_pon_errorcode error;
	void *pon_net_ctx;
	ifname_get_t *ifname_get = NULL;
	char symbol_name[256] = "";

	if (ifname == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libponnet");

	if(libpob_pa == NULL || libpob_pa->dl_handle == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	snprintf(symbol_name, sizeof(symbol_name), "pon_net_ifname_get");

	pon_net_ctx = (void *)libpob_pa->ll_ctx;
	ifname_get = dlsym(libpob_pa->dl_handle, symbol_name);

	if (pon_net_ctx==NULL || ifname_get==NULL)
		return INTEL_PON_ERROR;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = ifname_get(pon_net_ctx, class_id, me_id, ifname, size);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	// 220914 LEO START suppress error message when no ifname info
	return error;
	// 220914 LEO END
}

int intel_px126_pm_gem_port_counters_get( unsigned int gem_port_id, 
					struct intel_px126_pon_gem_port_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error=-1,gem_ret=-1;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_gem_port_counters param = {0};
	struct pon_gem_port gem_param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_gem_port_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	if ((gem_ret = fapi_pon_gem_port_index_get(pon_ctx, (unsigned char)gem_port_id, &gem_param)) == PON_STATUS_OK) {
		if (gem_param.alloc_valid == PON_ALLOC_INVALID &&
			gem_param.alloc_id == 0xFFFF) {
			gem_ret = -1;
		} else {
		error = fapi_pon_gem_port_counters_get(pon_ctx, gem_param.gem_port_id, &param);
	}
	}
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (gem_ret)
		return INTEL_PON_SUCCESS;

	if (error) {
		dbprintf(LOG_ERR,"error=%d, gem_error=%d, gem_port_id=%d, alloc_valid=%d, alloc_id=%d, gem_port_id=%d\n", 
				error, gem_ret, gem_port_id, gem_param.alloc_valid, gem_param.alloc_id, gem_param.gem_port_id);
	}

	UTIL_RETURN_IF_ERROR(error);

	counter->gem_port_id = param.gem_port_id;
	counter->tx_frames = param.tx_frames;
	counter->tx_fragments = param.tx_fragments;
	counter->tx_bytes = param.tx_bytes;
	counter->rx_frames = param.rx_frames;
	counter->rx_fragments = param.rx_fragments;
	counter->rx_bytes = param.rx_bytes;
	counter->key_errors = param.key_errors;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_gem_all_counters_get(struct intel_px126_pon_gem_port_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_gem_port_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_gem_port_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_gem_all_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->gem_port_id = param.gem_port_id;
	counter->tx_frames = param.tx_frames;
	counter->tx_fragments = param.tx_fragments;
	counter->tx_bytes = param.tx_bytes;
	counter->rx_frames = param.rx_frames;
	counter->rx_fragments = param.rx_fragments;
	counter->rx_bytes = param.rx_bytes;
	counter->key_errors = param.key_errors;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_gtc_counters_get( struct intel_px126_pon_gtc_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_gtc_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_gtc_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_gtc_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->bip_errors = param.bip_errors;
	counter->disc_gem_frames = param.disc_gem_frames;
	counter->gem_hec_errors_corr = param.gem_hec_errors_corr;
	counter->gem_hec_errors_uncorr = param.gem_hec_errors_uncorr;
	counter->bwmap_hec_errors_corr = param.bwmap_hec_errors_corr;
	counter->bytes_corr = param.bytes_corr;
	counter->fec_codewords_corr = param.fec_codewords_corr;
	counter->fec_codewords_uncorr = param.fec_codewords_uncorr;
	counter->total_frames = param.total_frames;
	counter->fec_sec = param.fec_sec;
	counter->gem_idle = param.gem_idle;
	counter->lods_events = param.lods_events;
	counter->dg_time = param.dg_time;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_xgtc_counters_get( struct intel_px126_pon_xgtc_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_xgtc_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_xgtc_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_xgtc_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->psbd_hec_err_uncorr = param.psbd_hec_err_uncorr;
	counter->psbd_hec_err_corr = param.psbd_hec_err_corr;
	counter->fs_hec_err_uncorr = param.fs_hec_err_uncorr;
	counter->fs_hec_err_corr = param.fs_hec_err_corr;
	counter->lost_words = param.lost_words;
	counter->ploam_mic_err = param.ploam_mic_err;
	counter->xgem_hec_err_corr = param.xgem_hec_err_corr;
	counter->xgem_hec_err_uncorr = param.xgem_hec_err_uncorr;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_fec_counters_get( struct intel_px126_pon_fec_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_fec_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_fec_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_fec_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->bytes_corr = param.bytes_corr;
	counter->words_corr = param.words_corr;
	counter->words_uncorr = param.words_uncorr;
	counter->words = param.words;
	counter->seconds = param.seconds;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_alloc_counters_get( unsigned int alloc_index, 
				       struct intel_px126_pon_alloc_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_alloc_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_alloc_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_alloc_counters_get(pon_ctx, (unsigned char)alloc_index, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->allocations = param.allocations;
	counter->idle = param.idle;
	counter->us_bw = param.us_bw;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_ploam_ds_counters_get(struct intel_px126_pon_ploam_ds_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_ploam_ds_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_ploam_ds_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_ploam_ds_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->us_overhead = param.us_overhead;
	counter->assign_onu_id = param.assign_onu_id;
	counter->ranging_time = param.ranging_time;
	counter->deact_onu = param.deact_onu;
	counter->disable_ser_no = param.disable_ser_no;
	counter->enc_port_id = param.enc_port_id;
	counter->req_passwd = param.req_passwd;
	counter->assign_alloc_id = param.assign_alloc_id;
	counter->no_message = param.no_message;
	counter->popup = param.popup;
	counter->req_key = param.req_key;
	counter->config_port_id = param.config_port_id;
	counter->pee = param.pee;
	counter->cpl = param.cpl;
	counter->pst = param.pst;
	counter->ber_interval = param.ber_interval;
	counter->key_switching = param.key_switching;
	counter->ext_burst = param.ext_burst;
	counter->pon_id = param.pon_id;
	counter->swift_popup = param.swift_popup;
	counter->ranging_adj = param.ranging_adj;
	counter->sleep_allow = param.sleep_allow;
	counter->req_reg = param.req_reg;
	counter->key_control = param.key_control;
	counter->burst_profile = param.burst_profile;
	counter->cal_req = param.cal_req;
	counter->tx_wavelength = param.tx_wavelength;
	counter->tuning_request = param.tuning_request;
	counter->tuning_complete = param.tuning_complete;
	counter->system_profile = param.system_profile;
	counter->channel_profile = param.channel_profile;
	counter->protection = param.protection;
	counter->power = param.power;
	counter->rate = param.rate;
	counter->reset = param.reset;
	counter->unknown = param.unknown;
	counter->all = param.all;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_ploam_us_counters_get( struct intel_px126_pon_ploam_us_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_ploam_us_counters param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_ploam_us_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = fapi_pon_ploam_us_counters_get(pon_ctx, &param);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	counter->ser_no = param.ser_no;
	counter->passwd = param.passwd;
	counter->dying_gasp = param.dying_gasp;
	counter->no_message = param.no_message;
	counter->enc_key = param.enc_key;
	counter->pee = param.pee;
	counter->pst = param.pst;
	counter->rei = param.rei;
	counter->ack = param.ack;
	counter->sleep_req = param.sleep_req;
	counter->reg = param.reg;
	counter->key_rep = param.key_rep;
	counter->tuning_resp = param.tuning_resp;
	counter->power_rep = param.power_rep;
	counter->rate_resp = param.rate_resp;
	counter->all = param.all;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_eth_rx_counters_get( unsigned int gem_port_id, 
					struct intel_px126_pon_eth_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error=-1,gem_ret=-1;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_eth_counters param = {0};
	struct pon_gem_port gem_param = {0};
	
	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_eth_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	if ((gem_ret = fapi_pon_gem_port_index_get(pon_ctx, (unsigned char)gem_port_id, &gem_param)) == PON_STATUS_OK) {
		if (gem_param.alloc_valid == PON_ALLOC_INVALID &&
			gem_param.alloc_id == 0xFFFF) {
			gem_ret = -1;
		} else {
		error = fapi_pon_eth_rx_counters_get(pon_ctx, gem_param.gem_port_id, &param);
	}
	}
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (gem_ret)
		return INTEL_PON_SUCCESS;

	if (error) {
		dbprintf(LOG_ERR,"error=%d, gem_error=%d, gem_port_id=%d, alloc_valid=%d, alloc_id=%d, gem_port_id=%d\n", 
				error, gem_ret, gem_port_id, gem_param.alloc_valid, gem_param.alloc_id, gem_param.gem_port_id);
	}

	UTIL_RETURN_IF_ERROR(error);

	counter->bytes = param.bytes;
	counter->frames_lt_64 = param.frames_lt_64;
	counter->frames_64 = param.frames_64;
	counter->frames_65_127 = param.frames_65_127;
	counter->frames_128_255 = param.frames_128_255;
	counter->frames_256_511 = param.frames_256_511;
	counter->frames_512_1023 = param.frames_512_1023;
	counter->frames_1024_1518 = param.frames_1024_1518;
	counter->frames_gt_1518 = param.frames_gt_1518;
	counter->frames_fcs_err = param.frames_fcs_err;
	counter->bytes_fcs_err = param.bytes_fcs_err;
	counter->frames_too_long = param.frames_too_long;

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_eth_tx_counters_get( unsigned int gem_port_id, 
					struct intel_px126_pon_eth_counters *counter)
{
	struct pa_node * libpob_pa;
	enum fapi_pon_errorcode error=-1,gem_ret=-1;
	struct px126_fapi_pon_wrapper_ctx *fapi_ctx;
	struct pon_ctx *pon_ctx;
	struct pon_eth_counters param = {0};
	struct pon_gem_port gem_param = {0};

	if (counter == NULL)
		return INTEL_PON_ERROR;

	libpob_pa = intel_omci_pa_lib_get_byname("libpon");

	if(libpob_pa == NULL || libpob_pa->ll_ctx == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	fapi_ctx = (struct px126_fapi_pon_wrapper_ctx  *)libpob_pa->ll_ctx;
	pon_ctx  = (struct pon_ctx *)fapi_ctx->pon_ctx;

	memset(counter, 0, sizeof(struct intel_px126_pon_eth_counters));
	
	fwk_mutex_lock(&libpob_pa->pa_lock);
	if ((gem_ret = fapi_pon_gem_port_index_get(pon_ctx, (unsigned char)gem_port_id, &gem_param)) == PON_STATUS_OK) {
		if (gem_param.alloc_valid == PON_ALLOC_INVALID &&
			gem_param.alloc_id == 0xFFFF) {
			gem_ret = -1;
		} else {
		error = fapi_pon_eth_tx_counters_get(pon_ctx, gem_param.gem_port_id, &param);
	}
	}
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (gem_ret)
		return INTEL_PON_SUCCESS;
	
	if (error) {
		dbprintf(LOG_ERR,"error=%d, gem_error=%d, gem_port_id=%d, alloc_valid=%d, alloc_id=%d, gem_port_id=%d\n", 
				error, gem_ret, gem_port_id, gem_param.alloc_valid, gem_param.alloc_id, gem_param.gem_port_id);
	}
	
	UTIL_RETURN_IF_ERROR(error);

	counter->bytes = param.bytes;
	counter->frames_lt_64 = param.frames_lt_64;
	counter->frames_64 = param.frames_64;
	counter->frames_65_127 = param.frames_65_127;
	counter->frames_128_255 = param.frames_128_255;
	counter->frames_256_511 = param.frames_256_511;
	counter->frames_512_1023 = param.frames_512_1023;
	counter->frames_1024_1518 = param.frames_1024_1518;
	counter->frames_gt_1518 = param.frames_gt_1518;
	counter->frames_fcs_err = param.frames_fcs_err;
	counter->bytes_fcs_err = param.bytes_fcs_err;
	counter->frames_too_long = param.frames_too_long;

	return INTEL_PON_SUCCESS;
}

/////// Ethernet port
#if 0
#define STRINGS_LEN_MAX 100000
static int
intel_px126_pm_ethtool_find_string_idx(const struct ethtool_gstrings *strings, const char *str)
{
	int i;
	int len_max;

	len_max = (strings->len > STRINGS_LEN_MAX) ? STRINGS_LEN_MAX : (int)strings->len;

	for (i = 0; i < len_max; i++) {
		if (strncmp(str,
			    ((const char *)strings->data + i * ETH_GSTRING_LEN),
			    ETH_GSTRING_LEN) == 0)
			return i;
	}

	return -1;
}
#endif

static struct ethtool_gstrings *
intel_px126_pm_ethtool_strings_get(void *ctx,
		    const char *ifname,
		    enum ethtool_stringset set_id,
		    ethtool_t *ethtool)
{
	int ret;
	struct {
		struct ethtool_sset_info hdr;
		unsigned int buf; /* will store size of sset */
	} sset_info = { 0 };
	uint32_t len = 0;
	struct ethtool_gstrings *strings;

	sset_info.hdr.sset_mask = 1ULL << set_id;
	ret = ethtool(ctx, ifname, ETHTOOL_GSSET_INFO, &sset_info);
	if (!ret && sset_info.hdr.sset_mask)
		len = sset_info.hdr.data[0];

	if (!len)
		return NULL;

	strings = malloc_safe(sizeof(*strings) + len * ETH_GSTRING_LEN);
	if (!strings)
		return NULL;

	memset(strings, 0, sizeof(*strings) + len * ETH_GSTRING_LEN);

	strings->string_set = set_id;
	strings->len = len;
	ret = ethtool(ctx, ifname, ETHTOOL_GSTRINGS, strings);

	if (ret) {
		free_safe(strings);
		return NULL;
	}

	return strings;
}

int intel_px126_pm_eth_port_counter_get( unsigned int me_id, 
					struct intel_px126_switch_eth_port_counters *counter)
{
	struct pa_node *libpob_pa;
	enum fapi_pon_errorcode error;
	void *pon_net_ctx;
	ethtool_t *ethtool = NULL;
	struct ethtool_gstrings *strings;
	struct ethtool_stats *stats;
	unsigned int n_stats, sz_stats;
	unsigned int i;
	char symbol_name[256] = "";
	char ifname[IF_NAMESIZE];
	// 220809 LEO START disable unused variable lan_id
	//int lan_id=-1;
	// 220809 LEO END
	
	unsigned long long *get_stat=(unsigned long long *)counter;

	if (counter == NULL)
		return INTEL_PON_ERROR;

	// 220809 LEO START to retrieve ifname, adopt classid 47 instead of classid 11, for not only eth0_0 but also pmapper*
	if (intel_px126_pm_ifname_get(11,me_id,ifname,sizeof(ifname)) != INTEL_PON_SUCCESS)
	{
		if (intel_px126_pm_ifname_get(47,me_id,ifname,sizeof(ifname)) != INTEL_PON_SUCCESS)
		{
			// 220914 LEO START suppress error message when no ifname info
			dbprintf(LOG_NOTICE,"Cannot get ifname, me_id=0x%x\n",me_id);
			// 220914 LEO END
			return INTEL_PON_ERROR;
		}
	}
	// 220809 LEO END

	// 220809 LEO START disable unused variable lan_id
	/*
	if ((lan_id=intel_px126_pm_ifname_to_lan_index_get(ifname)) < 0) {
		dbprintf(LOG_ERR,"Cannot get lan_id, me_id=0x%x, ifname=%s\n",me_id,ifname);
		return INTEL_PON_ERROR;
	}
	*/
	// 220809 LEO END

	libpob_pa = intel_omci_pa_lib_get_byname("libponnet");

	if(libpob_pa == NULL || libpob_pa->dl_handle == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	snprintf(symbol_name, sizeof(symbol_name), "pon_net_ethtool");

	pon_net_ctx = (void *)libpob_pa->ll_ctx;
	ethtool = dlsym(libpob_pa->dl_handle, symbol_name);

	if (pon_net_ctx==NULL || ethtool==NULL)
		return INTEL_PON_ERROR;

	memset(counter, 0, sizeof(struct intel_px126_switch_eth_port_counters));

	// dbprintf(LOG_ERR,"me_id=%u, ifname=%s, ethtool=%p\n", me_id, ifname, ethtool);

	fwk_mutex_lock(&libpob_pa->pa_lock);
	strings = intel_px126_pm_ethtool_strings_get(pon_net_ctx, ifname, ETH_SS_STATS, ethtool);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (!strings) {
		dbprintf(LOG_ERR,"Cannot get stats strings information\n");
		return INTEL_PON_ERROR;
	}

	n_stats = strings->len;
	if (n_stats < 1) {
		dbprintf(LOG_ERR,"No stats available\n");
		free_safe(strings);
		return INTEL_PON_ERROR;
	}

	sz_stats = n_stats * sizeof(unsigned long long);

	//dbprintf(LOG_ERR,"n_stats=%d, sz_stats=%d\n", n_stats, sz_stats);

	stats = malloc_safe(sz_stats + sizeof(struct ethtool_stats));
	if (!stats) {
		dbprintf(LOG_ERR,"No memory available\n");
		free_safe(strings);
		return INTEL_PON_ERROR;
	}

	stats->cmd = ETHTOOL_GSTATS;
	stats->n_stats = n_stats;

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = ethtool(pon_net_ctx, ifname, ETHTOOL_GSTATS, stats);
	fwk_mutex_unlock(&libpob_pa->pa_lock);

	if (error < 0) {
		dbprintf(LOG_ERR,"Cannot get stats information\n");
		free_safe(strings);
		free_safe(stats);
		return INTEL_PON_ERROR;
	}

#if 0 // For debug
	dbprintf(LOG_ERR,"Get from Intel %s,%d statistics:\n", ifname, lan_id);
	for (i = 0; i < n_stats; i++) {
		dbprintf(LOG_ERR,"     %.*s: %llu\n",
			ETH_GSTRING_LEN,
			&strings->data[i * ETH_GSTRING_LEN],
			stats->data[i]);
	}
#endif

	for (i = 0; i < n_stats; i++) {
		*(get_stat+i) = stats->data[i];
	}

#if 0 // For debug
	dbprintf(LOG_ERR,"Examine %s statistics:\n", ifname);
	for (i = 0; i < n_stats; i++) {
		dbprintf(LOG_ERR,"     %.*s: %llu\n",
			ETH_GSTRING_LEN,
			&strings->data[i * ETH_GSTRING_LEN],
			*(get_stat+i));
	}
#endif

	free_safe(strings);
	free_safe(stats);

	return INTEL_PON_SUCCESS;
}

int intel_px126_pm_eth_port_refresh( unsigned int me_id)
{
	char cmd[128]={0};
	char ifname[IF_NAMESIZE];
	int lan_id=-1;
	
	if (intel_px126_pm_ifname_get(11,me_id,ifname,sizeof(ifname)) != INTEL_PON_SUCCESS) {
		dbprintf(LOG_ERR,"Cannot get ifname, me_id=0x%x\n",me_id);
		return INTEL_PON_ERROR;
	}

	if ((lan_id=intel_px126_pm_ifname_to_lan_index_get(ifname)) < 0) {
		dbprintf(LOG_ERR,"Cannot get lan_id, me_id=0x%x, ifname=%s\n",me_id,ifname);
		return INTEL_PON_ERROR;
	}

	if (lan_id != 0) // Force a port, maybe extend in the feature.
		return INTEL_PON_ERROR;

	sprintf(cmd, "/usr/bin/switch_cli GSW_RMON_CLEAR dev=0 nRmonId=4 eRmonType=2");

	//dbprintf(LOG_ERR,"me_id=0x%x, ifname:%s, lan_id:%d\n cmd:%s\n",me_id,ifname,lan_id,cmd);

	system(cmd);

	return INTEL_PON_SUCCESS;
}

