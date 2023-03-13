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
#include <fcntl.h>
#include <errno.h>

#include "fwk_mutex.h"

#include "util.h"
#include "util_uci.h"
#include "intel_px126_util.h"
#include "intel_px126_mcc_util.h"

#include <omci/pon_adapter_omci.h>
#include "omci/me/pon_adapter_mac_bridge_service_profile.h"
#include "omci/me/pon_adapter_mac_bridge_port_config_data.h"
#include "omci/me/pon_adapter_pptp_lct_uni.h"

#define INTEL_OMCI_INIT_PARAMS_CNT 64 

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

struct list_module_table_t {
	int index;
	char module_name[INTEL_OMCI_INIT_PARAMS_CNT];	
};

static struct list_module_table_t module_list_table[]={
	{0, "libponimg"},
	{1, "libponnet"},
	{2, "libpon"}
};


struct intel_omci_context g_intel_omci_context;
static char const *g_intel_omci_init_params[INTEL_OMCI_INIT_PARAMS_CNT + 1];
static int g_intel_omci_init_params_cnt = 0;

static size_t num_lower_layers;
static struct intel_pa_ll_definition *lower_layers;


static int __inetl_omci_pa_config_get(void *ctx, const char *path, const char *sec,
			      const char *opt, size_t size, char *out)
{
	INTEL_UNUSED(ctx);
	return util_uci_config_get(path, sec, opt, out);
}

static struct pa_node *__intel_omci_pa_ll_find(struct intel_omci_context *context, const char *name)
{
	struct pa_node *np = NULL,*n = NULL;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
		if (strcmp(np->definition->name, name) == 0)
			return np;

	}

	return NULL;
}

int intel_omci_save_init_param(const char *name, const char *value)
{
	char *init_param;
	size_t param_len;

	param_len = strlen(name) + strlen(value) + 2; /* '=' and '\0' */
	init_param = malloc(param_len);
	if (!init_param)
		return INTEL_PON_ERROR;

	memset(init_param, 0x0, param_len);
	snprintf(init_param, param_len, "%s=%s",  name, value);
	g_intel_omci_init_params[g_intel_omci_init_params_cnt] = init_param;

	++g_intel_omci_init_params_cnt;
	if (g_intel_omci_init_params_cnt > INTEL_OMCI_INIT_PARAMS_CNT) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"Buffer for init parameters is too small. Please fix the code!\n");
		return INTEL_PON_ERROR;
	}
	return INTEL_PON_SUCCESS;
}

static int __intel_px126_read_list_count(void *ctx, const char *value)
{
	size_t *count = ctx;

	*count += 1;
	return 0;
}
static int __intel_read_lower_layers(void *ctx, const char *value)
{
	if (strlen(value) >= sizeof(lower_layers[0].name))
		return -1;

	snprintf(lower_layers[num_lower_layers].name,
		 sizeof(lower_layers[0].name),
		 "%s",
		 value);

	num_lower_layers++;

	return 0;
}

static int __intel_omci_pa_open_library(struct pa_node *np,
					    const char *name)
{
	char symbol_name[256] = "";
	char lib_name[256] = "";
	void *handle = NULL;
	ll_register_t *ll_reg = NULL;

	snprintf(lib_name, sizeof(lib_name), "%s.so", name);
	snprintf(symbol_name, sizeof(symbol_name), "%s_ll_register_ops", name);

	handle = dlopen(lib_name, RTLD_NOW);

	if (handle == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%s) dlopen: %s\n",
		    __func__, dlerror());
		return INTEL_PON_ERROR;
	}

	ll_reg = dlsym(handle, symbol_name);

	if (ll_reg == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%s) dlsym: %s\n",
			__func__, dlerror());
		return INTEL_PON_ERROR;
	}

	np->dl_handle = handle;
	np->ll_reg = ll_reg;

	return INTEL_PON_SUCCESS;
}

int intel_omci_pa_ll_init(struct intel_omci_context *context,
				char const * const *init_params,
				const struct pa_config *pa_config,
				const struct pa_eh_ops *event_handlers)
{
		struct pa_node *np = NULL;
	int ret = INTEL_PON_SUCCESS;

	list_for_each_entry_reverse( np, &context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, system_ops, init)) {
			np->initialized = 1;

			int error = np->pa_ops->system_ops->init(
				init_params, pa_config, event_handlers,
				np->ll_ctx);

			if (error != INTEL_PON_SUCCESS) {
				ret = error;
				util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%d) %s init error\n", error,
					np->definition->name);
			} else {
				util_dbprintf(omci_env_g.debug_level, LOG_INFO, 0,"%s initialized\n",
					np->definition->name);
			}
		}
	}
	return ret;
}

int intel_omci_pa_ll_register(struct intel_omci_context *context,
				    const struct intel_pa_ll_definition *definition)
{
	struct pa_node *np = malloc_safe(sizeof(*np));
	int error;

	if (!np)
		return INTEL_PON_ERROR;

	memset(np, 0, sizeof(*np));

	np->ll_reg = definition->ll_reg;
	np->definition = definition;

	if (!np->ll_reg) {
		error = __intel_omci_pa_open_library(np, definition->name);

		if (error != INTEL_PON_SUCCESS) {
			free_safe(np);
			return INTEL_PON_ERROR;
		}
	}

	if (!np->ll_reg) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR %s: there is no register function\n",
			definition->name);
		free_safe(np);
		return INTEL_PON_ERROR;
	}

	error = np->ll_reg(context, &np->pa_ops, &np->ll_ctx);

	if (error != INTEL_PON_SUCCESS) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%d) %s registration error\n", error,
			definition->name);
		free_safe(np);
		return INTEL_PON_ERROR;
	}

	//init mutex
	fwk_mutex_create(&np->pa_lock);
	INIT_LIST_NODE(&np->entries);
	list_add_tail(&np->entries,&context->pa_list);

	return INTEL_PON_SUCCESS;
}

int intel_omci_pa_ll_register_array(
				struct intel_omci_context *context,
				const struct intel_pa_ll_definition *definition,
				unsigned int size)
{
	int ret = INTEL_PON_SUCCESS;
	unsigned int i;
	const struct intel_pa_ll_definition *def = definition;

	for (i = 0; i < size; i++) {
		ret = intel_omci_pa_ll_register(context, def);
		if (ret != INTEL_PON_SUCCESS)
			break;

		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"%s registered\n", def->name);
		def++;
	}
	return ret;
}

static void __alarm_cb(void *ctx, unsigned short class_id, unsigned short instance_id,
	unsigned int alarm, bool active)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"alarm_cb %p, %u, %u, %u, %s\n", ctx, class_id, instance_id,
		alarm, active ? "true" : "false");
}

static void 
__optic_alarm_cb(void *ctx, int alarm, bool active)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;
	
	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"optic_alarm_cb %p,  %u, %s\n", ctx,
		alarm, active ? "true" : "false");
#if 0
	error = single_me_get(context, OMCI_ME_ANI_G, &me);
	if (error != OMCI_SUCCESS) {
		mib_unlock(context);
		return;
	}

	(void)omci_me_alarm_set(context, OMCI_ME_ANI_G,
#endif
}

static void __omci_msg_cb(void *ctx, const unsigned char *msg,
			const unsigned short len)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"omci_msg_cb %p, %p, %u\n", ctx, msg, len);
}

static void __ploam_state_change_cb(void *ctx,
				  int prev_state,
				  int curr_state)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"ploam_state_change %p, %d -> %d\n", ctx, prev_state, curr_state);

	if (context->last_ploam_state != prev_state &&
		context->last_ploam_state != 0) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"Missed PLOAM state change from %d to %d\n",
			context->last_ploam_state, prev_state);
		prev_state = context->last_ploam_state;
	}
	context->last_ploam_state = curr_state;
}

static int __intel_omci_key_update(struct intel_omci_context *context,
				  const struct pa_omci_ik *omci_ik)
{
	fwk_mutex_lock(&context->omci_ik.lock);
	memcpy(context->omci_ik.key, omci_ik,sizeof(struct pa_omci_ik));
	
	util_dbprintf(omci_env_g.debug_level, LOG_ERR,0,"Got new integrity key from firmware: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		context->omci_ik.key[0], context->omci_ik.key[1],
		context->omci_ik.key[2], context->omci_ik.key[3],
		context->omci_ik.key[4], context->omci_ik.key[5],
		context->omci_ik.key[6], context->omci_ik.key[7],
		context->omci_ik.key[8], context->omci_ik.key[9],
		context->omci_ik.key[10], context->omci_ik.key[11],
		context->omci_ik.key[12], context->omci_ik.key[13],
		context->omci_ik.key[14], context->omci_ik.key[15]);
	fwk_mutex_unlock(&context->omci_ik.lock);
	return INTEL_PON_SUCCESS;
}

static void __omci_interval_end(void *context,
				  unsigned char interval_end_time)
{
	INTEL_UNUSED(context);
	INTEL_UNUSED(interval_end_time);
} 

static void __link_state_cb(void *ctx,
			  unsigned short instance_id,
			  bool state,
			  unsigned char config_ind)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;
	INTEL_UNUSED(context);
	
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"link_state_cb %p, %u, %s, %u\n", ctx, instance_id,
		state ? "up" : "down", config_ind);
	if(ctx || instance_id||state||config_ind){}
	///OMCI_ME_PPTP_ETHERNET_UNI,
}

static void __net_state_cb(void *ctx,
			 const char *iface_name,
			 const bool iface_up)
{
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "net_state_cb %p, %p, %s\n", ctx, (void *)iface_name,
		iface_up ? "true" : "false");

	if(ctx || iface_name||iface_up){}
}

static void __link_init_cb(void *ctx,
			 unsigned short instance_id,
			 bool is_initialized)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;
	unsigned char  oper_state = is_initialized ? 0 :  1;

	INTEL_UNUSED(context);
	INTEL_UNUSED(oper_state);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,  "link_init_cb %p, %u, %s\n", ctx, instance_id,
		is_initialized ? "true" : "false");
	if(ctx || instance_id ||is_initialized ){}
	/// OMCI_ME_PPTP_ETHERNET_UNI,
}

static void __loop_detect_cb(void *ctx,
			   unsigned short instance_id)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "__loop_detect_cb %p, %u\n", ctx, instance_id);

	if(ctx || instance_id){}
}

static void __auth_result_ready_cb(void *ctx,
				 const unsigned char  *onu_auth_result,
				 const unsigned short len)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"auth_result_ready_cb %p %p %u\n", ctx, onu_auth_result, len);
}

static void __auth_status_change_cb(void *ctx,
				  unsigned int status)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);

	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "auth_status_change_cb %p\n", ctx);
}

#ifdef INCLUDE_MIC_CHECK
static void __omci_ik_update_cb(void *ctx,
			      const struct pa_omci_ik *omci_ik)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;
	
	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"ik_update_cb %p %p\n", ctx, omci_ik);

	(void)__intel_omci_key_update(context, omci_ik);
}
#endif /* #ifdef INCLUDE_MIC_CHECK */

static void __omci_mib_reset_cb(void *ctx)
{
	struct intel_omci_context *context = (struct intel_omci_context *)ctx;

	INTEL_UNUSED(context);
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "mib_reset_cb %p\n", ctx);
}

static const struct pa_config pa_config = {
	.get = __inetl_omci_pa_config_get,
	/* By testing the .get_secure against NULL in the implementation
	 * one will learn whether sse_fapi is supported
	 */
#ifdef INCLUDE_SECURE_FAPI
	.get_secure = __intel_omci_secure_config_get,
#endif
};

static const struct pa_eh_ops event_handlers = {
	.alarm = __alarm_cb,
	.optic_alarm = __optic_alarm_cb,
	.omci_msg = __omci_msg_cb,
	.ploam_state = __ploam_state_change_cb,
	.interval_end = __omci_interval_end,
	.link_state = __link_state_cb,
	.net_state = __net_state_cb,
	.link_init = __link_init_cb,
	.loop_detect = __loop_detect_cb,
	.auth_result_rdy = __auth_result_ready_cb,
	.auth_status_chg = __auth_status_change_cb,
#ifdef INCLUDE_MIC_CHECK
	.omci_ik_update = __omci_ik_update_cb,
#endif
	.mib_reset = __omci_mib_reset_cb,
};

int intel_omci_pa_ll_start(struct intel_omci_context *context)
{
	struct pa_node *np = NULL;
	int ret = INTEL_PON_SUCCESS;

	list_for_each_entry_reverse( np, &context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, system_ops, start)) {
			ret = (int)
				np->pa_ops->system_ops->start(
					np->ll_ctx);

			if (ret != INTEL_PON_SUCCESS) {
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"ERROR(%d) %s start error\n", ret,
					np->definition->name);
			}
		}
	}

	return ret;
}

enum pa_pon_op_mode intel_omci_pa_get_pon_op_mode(struct intel_omci_context *context)
{
	struct pa_node *np = NULL,*n = NULL;
	//int ret = INTEL_PON_SUCCESS;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){
		if (PA_EXISTS(np->pa_ops, sys_sts_ops, get_pon_op_mode))
			return np->pa_ops->sys_sts_ops->get_pon_op_mode(
				np->ll_ctx);
	}

	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"ERROR(%d) PA_PON_MODE_UNKNOWN\n");

	return PA_PON_MODE_UNKNOWN;
}

void intel_omci_register_omcc_msg_cb(void (*omcc_raw_rcv)(void *msg,unsigned short len,unsigned int crc ))
{
	g_intel_omci_context.omcc_raw_rcv = omcc_raw_rcv;
}

void intel_omci_omcc_msg_send(const char *omci_msg,
			      const unsigned short len,
			      unsigned int crc_mic)
{
	intel_omcc_msg_send(&g_intel_omci_context,omci_msg,len,crc_mic);
}

int intel_omci_config_dbglvl(unsigned short is_set, unsigned short dbgmodule,unsigned short *dbglvl)
{
	struct pa_node *np = NULL;

	if(dbgmodule >=ARRAY_SIZE(module_list_table) )
		return INTEL_PON_ERROR;

	np = __intel_omci_pa_ll_find(&g_intel_omci_context,module_list_table[dbgmodule].module_name);

	if(np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "__intel_omci_pa_ll_find error\n");
		return INTEL_PON_ERROR;
	}

	if  (is_set) {
		if (PA_EXISTS(np->pa_ops->dbg_lvl_ops, set)) {
			np->pa_ops->dbg_lvl_ops->set(*dbglvl);
		}
	} else {
		if (PA_EXISTS(np->pa_ops->dbg_lvl_ops, get)) {
			*dbglvl =  np->pa_ops->dbg_lvl_ops->get();
		}
	}

	return INTEL_PON_SUCCESS;
}

int intel_omci_init(void)
{
	int error;
	size_t max_lower_layers = 0;
	
	memset(&g_intel_omci_context,0x0,sizeof(struct intel_omci_context));

	if (util_uci_config_get("gpon", "ponip", "pon_mode", g_intel_omci_context.op_mode_char))
		/* Store default operational mode */
		snprintf(g_intel_omci_context.op_mode_char, sizeof(g_intel_omci_context.op_mode_char), "%s", "gpon");

	if (util_uci_config_get("omci", "default", "loid", g_intel_omci_context.loid))
		/* Store default value */
		snprintf(g_intel_omci_context.loid, LOID_LEN, "%s",
			 "loid");

	if (util_uci_config_get("omci", "default", "lpwd", g_intel_omci_context.lpwd))
		/* Store default value */
		snprintf(g_intel_omci_context.lpwd, LPWD_LEN, "%s",
			"lpasswd");
	
	if (util_uci_config_generic_get("omci",
				    "pon_adapter",
				    "modules",
				    &max_lower_layers,
				    __intel_px126_read_list_count)) {
		max_lower_layers = 0;
	}

	if (!max_lower_layers) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "omci load lib info error\n");
		return INTEL_PON_ERROR;
	}
	
	lower_layers =
		malloc_safe(max_lower_layers *
			sizeof(lower_layers[0]));

	if (!lower_layers) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "omci Memory allocation error!\n");
		goto free_args;
	}

	memset(lower_layers,
		0,
		max_lower_layers * sizeof(lower_layers[0]));

	if (util_uci_config_generic_get("omci", "pon_adapter", "modules",
					&g_intel_omci_context,
					__intel_read_lower_layers))
	{
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "omci ERROR Reading lower layers failed!\n");
		goto free_args;
	}

	g_intel_omci_context.num_lower_layers = num_lower_layers;
	g_intel_omci_context.lower_layers = lower_layers;

	INIT_LIST_HEAD(&g_intel_omci_context.pa_list);
	error = intel_omci_pa_ll_register_array(&g_intel_omci_context, g_intel_omci_context.lower_layers,
					  g_intel_omci_context.num_lower_layers);     

	if (error != INTEL_PON_SUCCESS) {
		/**
		 * In case the registration of ll modules fails,
		 * skip cleanup and return directly.
		 */
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"%s error-(%d)\n",__func__, error);
		return error;
	}
	
	intel_omci_save_init_param("pon_if", omci_env_g.omcc_if);
	
	intel_omci_save_init_param("soc_mac", "00:e0:92:00:01:40");
	
	intel_omci_save_init_param("pon_mac", "00:e0:92:00:01:42");
	
	do {
		//omcc mutex
		fwk_mutex_create(&g_intel_omci_context.omcc_ctx.msg_stats.lock);

		//omcc ik mutex
		fwk_mutex_create(&g_intel_omci_context.omci_ik.lock);

		g_intel_omci_context.param_list = g_intel_omci_init_params;

		/* called before the initial MIB was created */
		error = intel_omci_pa_ll_init(&g_intel_omci_context, g_intel_omci_context.param_list,
						&pa_config,
						&event_handlers);
		if (error != INTEL_PON_SUCCESS){
			util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"omci_pa_ll_init fail %d\n", error);
			break;

		}

		error = intel_mcc_init(&g_intel_omci_context);
		if (error != INTEL_PON_SUCCESS) {
			util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%d) OMCI MCC init failed\n", error);
			break;
		}

		/* called after the initial MIB was created */
		error = intel_omci_pa_ll_start(&g_intel_omci_context);
		if (error != INTEL_PON_SUCCESS) {
			util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0,"ERROR(%d) OMCI API start error", error);
			break;
		}

		g_intel_omci_context.op_mode = intel_omci_pa_get_pon_op_mode(&g_intel_omci_context);

		///omcc soucket
		intel_omcc_init(&g_intel_omci_context);

	} while(0);

	return INTEL_PON_SUCCESS;

free_args:
	if(g_intel_omci_context.lower_layers)
		free_safe(g_intel_omci_context.lower_layers);

	return INTEL_PON_ERROR;
}

struct intel_omci_context* intel_omci_get_omci_context(void)
{
	return &g_intel_omci_context;
}

///
int intel_omci_bridge_inf_create(unsigned int brd_meid)
{
	struct intel_omci_context* context = &g_intel_omci_context;
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mac_bridge_service_profile_ops *bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mac_bridge_service_profile))  {
			bsp =
			    np->pa_ops->omci_me_ops->mac_bridge_service_profile;
			
			if (PA_EXISTS(bsp, init) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy) &&
			    PA_EXISTS(bsp, port_count_get))
				break;
		}
	}

	if (bsp == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET BRIDGE FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	bsp = np->pa_ops->omci_me_ops->mac_bridge_service_profile;
	/*create netdev*/
	fwk_mutex_lock(&np->pa_lock);
	error = bsp->init(np->ll_ctx, brd_meid);
	fwk_mutex_unlock(&np->pa_lock);

	UTIL_RETURN_IF_ERROR(error);

	return error;
}

///
int intel_omci_bridge_inf_del(unsigned int brd_meid)
{
	struct intel_omci_context* context = &g_intel_omci_context;
	struct pa_node *np = NULL,*n = NULL;
	const struct pa_mac_bridge_service_profile_ops *bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mac_bridge_service_profile))  {
			bsp =
			    np->pa_ops->omci_me_ops->mac_bridge_service_profile;
			
			if (PA_EXISTS(bsp, init) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy) &&
			    PA_EXISTS(bsp, port_count_get))
				break;
		}
	}

	if (bsp == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET BRIDGE FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	bsp = np->pa_ops->omci_me_ops->mac_bridge_service_profile;
	/*create netdev*/
	fwk_mutex_lock(&np->pa_lock);
	error = bsp->destroy(np->ll_ctx, brd_meid);
	fwk_mutex_unlock(&np->pa_lock);
	
	UTIL_RETURN_IF_ERROR(error);

	return error;
}

int intel_omci_bridge_port_cfg_update(unsigned short meid,void* upd_data,int is_del)
{
	struct pa_mac_bp_config_data_upd_data * ll_upd_data;
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	struct pa_mac_bp_config_data_destroy_data destroy_data;
	const struct pa_mac_bp_config_data_ops * bsp;
	int error;
	
	if(upd_data ==NULL)
		return -1;
	ll_upd_data = (struct pa_mac_bp_config_data_upd_data *)upd_data;
	
	ll_upd_data->mcc_ipv4_enabled = 1;
	ll_upd_data->mcc_ipv6_enabled = 1;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mac_bp_config_data))  {
			bsp =
			    np->pa_ops->omci_me_ops->mac_bp_config_data;
			
			if (PA_EXISTS(bsp, connect) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy))
				break;
		}
	}
	
	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	if (!is_del) {
		fwk_mutex_lock(&np->pa_lock);
		error = bsp->update(np->ll_ctx, meid,ll_upd_data);	
		fwk_mutex_unlock(&np->pa_lock);
		
		UTIL_RETURN_IF_ERROR(error);
	} else {
		memset(&destroy_data,0x0,sizeof(struct pa_mac_bp_config_data_destroy_data));
		destroy_data.tp_type = ll_upd_data->tp_type;
		destroy_data.tp_ptr  = ll_upd_data->tp_ptr;
		destroy_data.outbound_td_ptr = ll_upd_data->outbound_td_ptr;
		destroy_data.inbound_td_ptr  = ll_upd_data->inbound_td_ptr;
		fwk_mutex_lock(&np->pa_lock);
		error = bsp->destroy(np->ll_ctx, meid,&destroy_data);
		fwk_mutex_unlock(&np->pa_lock);
		UTIL_RETURN_IF_ERROR(error);
	}

	return INTEL_PON_SUCCESS;
}

struct pa_node *intel_omci_pa_lib_get_byname(const char *name)
{
	return __intel_omci_pa_ll_find(&g_intel_omci_context,name);
}

int 
intel_omci_bridge_port_connect(unsigned short br_meid,unsigned short br_port_meid,unsigned char tp_type,unsigned short tp_ptr)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_mac_bp_config_data_ops * bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			mac_bp_config_data))  {
			bsp =
			    np->pa_ops->omci_me_ops->mac_bp_config_data;
			
			if (PA_EXISTS(bsp, connect) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"br_meid=%d-%d-%d-%d\n", br_meid,br_port_meid,tp_type,tp_ptr);
	error = bsp->connect(np->ll_ctx, br_meid,br_port_meid,tp_type,tp_ptr);	
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int
intel_omci_pptp_lct_uni_create(unsigned short meid)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_pptp_lct_uni_ops * bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_lct_uni))  {
			bsp =
			    np->pa_ops->omci_me_ops->pptp_lct_uni;

			if (PA_EXISTS(bsp, create) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"br_meid=%d-%d-%d-%d\n", br_meid,br_port_meid,tp_type,tp_ptr);
	error = bsp->create(np->ll_ctx, meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int
intel_omci_pptp_lct_uni_update(unsigned short meid, unsigned char state_admin)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_pptp_lct_uni_ops * bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_lct_uni))  {
			bsp =
			    np->pa_ops->omci_me_ops->pptp_lct_uni;

			if (PA_EXISTS(bsp, create) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"br_meid=%d-%d-%d-%d\n", br_meid,br_port_meid,tp_type,tp_ptr);
	error = bsp->update(np->ll_ctx, meid, state_admin);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int
intel_omci_pptp_lct_uni_destory(unsigned short meid)
{
	struct pa_node *np = NULL,*n = NULL;
	struct intel_omci_context* context = &g_intel_omci_context;
	const struct pa_pptp_lct_uni_ops * bsp;
	int error;

	list_for_each_entry_safe( np, n ,&context->pa_list ,entries){

		if (PA_EXISTS(np->pa_ops, omci_me_ops,
			pptp_lct_uni))  {
			bsp =
			    np->pa_ops->omci_me_ops->pptp_lct_uni;

			if (PA_EXISTS(bsp, create) &&
			    PA_EXISTS(bsp, update) &&
			    PA_EXISTS(bsp, destroy))
				break;
		}
	}

	if (np == NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB ME FUNC FAIL\n");
		return INTEL_PON_ERROR;
	}

	fwk_mutex_lock(&np->pa_lock);
	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"br_meid=%d-%d-%d-%d\n", br_meid,br_port_meid,tp_type,tp_ptr);
	error = bsp->destroy(np->ll_ctx, meid);
	fwk_mutex_unlock(&np->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}


int intel_omci_pa_mapper_dump(void)
{
	struct pa_node *libpob_pa;
	int error;
	void *pon_net_ctx;
	mapper_dump_t *mapper_dump = NULL;
	char symbol_name[256] = "";
	static int check_flag=0;

	libpob_pa = intel_omci_pa_lib_get_byname("libponnet");

	if(libpob_pa == NULL || libpob_pa->dl_handle == NULL){
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,"GET LIB PON FAIL\n");
		return INTEL_PON_ERROR;
	}

	snprintf(symbol_name, sizeof(symbol_name), "pon_net_pa_mapper_dump");

	pon_net_ctx = (void *)libpob_pa->ll_ctx;
	mapper_dump = dlsym(libpob_pa->dl_handle, symbol_name);

	if (pon_net_ctx==NULL || mapper_dump==NULL)
		return INTEL_PON_ERROR;

	if( 0 == check_flag ) {
		int std_fd = -1;

		std_fd = open("/dev/console", O_RDWR);
		if (std_fd < 0) {
			dbprintf(LOG_ERR, "errno=%d (%s)\n", errno, strerror(errno));
			return INTEL_PON_ERROR;
		}
		dup2(std_fd, 1);	// standrad ouput set
		dup2(std_fd, 2);	// standrad error set
		close(std_fd);
		check_flag = 1;
	}

	fwk_mutex_lock(&libpob_pa->pa_lock);
	error = mapper_dump(pon_net_ctx);
	fwk_mutex_unlock(&libpob_pa->pa_lock);
	UTIL_RETURN_IF_ERROR(error);

	return INTEL_PON_SUCCESS;
}

