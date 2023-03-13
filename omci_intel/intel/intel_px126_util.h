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
#ifndef __INTEL_PX126_UTIL_H__
#define __INTEL_PX126_UTIL_H__
#include <string.h>
#ifdef LINUX
#include <dlfcn.h>
#endif
#include "list.h"
#include "util_uci.h"
#include "pon_adapter.h"
#include "pon_adapter_system.h"
#include "pon_adapter_event_handlers.h"
#include "pon_adapter_config.h"

#include "fwk_mutex.h"
//////////////////////////////////////////////////////////////////////
#define INCLUDE_MIC_CHECK
#define OMCI_MIC_IK_LEN    16
#define INCLUDE_OMCI_ONU_UCI
#define PA_LOWER_LAYER_MAX_NAME_LEN 32
#define LOID_LEN 24 
#define LPWD_LEN 12
#define OMCI_MSG_DIR_DOWNSTREAM 0x01
#define OMCI_MSG_DIR_UPSTREAM 0x02

#define INTEL_UNUSED(x) (void)(x)
#define UTIL_RETURN_IF_ERROR(ERROR) \
	do { \
		if ((ERROR)) { \
			dbprintf(LOG_ERR,"%s error(%d)\n",__func__, ERROR); \
			return ERROR; \
		} \
	} while (0)

typedef enum pon_adapter_errno(ll_register_t)(void *hl_handle,
				     const struct pa_ops **pa_ops,
				     void **ll_handle);

typedef enum pon_adapter_errno (ethtool_t)(void *ctx,
				     const char *ifname,
				     unsigned int cmd,
				     void *data);

typedef enum pon_adapter_errno (ifname_get_t)(void *ctx,
				     unsigned short class_id, unsigned short me_id,
				     char *ifname, int size);

typedef unsigned char (ifname_to_lan_idx_t)(void *ctx, const char *ifname);

typedef enum pon_adapter_errno (mapper_dump_t)(void *ctx);


/* Support macros function to glue names into function pointers */
#define PA_GLUE1(arg1)\
	arg1
#define PA_GLUE2(arg1, arg2)\
	PA_GLUE1(arg1)->arg2
#define PA_GLUE3(arg1, arg2, arg3)\
	PA_GLUE2(arg1, arg2)->arg3
#define PA_GLUE4(arg1, arg2, arg3, arg4)\
	PA_GLUE3(arg1, arg2, arg3)->arg4
#define PA_GLUE5(arg1, arg2, arg3, arg4, arg5)\
	PA_GLUE4(arg1, arg2, arg3, arg4)->arg5

/** Support macro function to choose suitable glue function */
#define SELECT_PA_GLUE(arg1, arg2, arg3, arg4, arg5, N, ...) N

/** Support macro function to choose suitable PA_CALL_ALL macro */
#define SELECT_PA_ARGS(_ctx, _err, _func, _arg, N, ...) N

/** Macro function to generate function pointer based on provided arguments */
#define PA_GLUE(...) EVAL(SELECT_PA_GLUE(\
	__VA_ARGS__, PA_GLUE5, PA_GLUE4, PA_GLUE3,\
	PA_GLUE2, PA_GLUE1)(__VA_ARGS__))

/** Macro function to call PA_CALL_ALL_ARGS with args parameter
 * (optional: empty) in parenthesis
 */
#define PA_CALL_ALL(...) EVAL(SELECT_PA_ARGS(__VA_ARGS__,\
			 PA_CALL_ALL_ARGS, PA_CALL_ALL_NO_ARG)(__VA_ARGS__))

#define PA_CALL_ALL_NO_ARG(ctx, pa_err, func)\
	PA_CALL_ALL_ARGS(ctx, pa_err, func, ())

/** Support macros to shorten list of required function path */
#define PA_FUNC(...) _np->pa_ops, __VA_ARGS__
/** Support macros to shorten list of required function arguments */
#define PA_ARGS(...) (_np->ll_ctx, ## __VA_ARGS__)

/** This macro calls all PA functions.
 *
 *  \param[in]  ctx	OMCI context
 *  \param[out] pa_err	Returned OMCI error code
 *  \param[in]  func	Function pointer path
 *  \param[in]  args	Called PA function arguments - in parenthesis
 *			parenthesis are needed for empty argument list, too
 *
 *  \remark It will stop looping over PA modules on first encountered error
 *
 */
#define PA_CALL_ALL_ARGS(ctx, pa_err, func, args) \
do { \
	struct pa_node *_np = NULL; \
	char module_idx; \
	unsigned long long int rmask_curr = UINT64_MAX; \
	unsigned long long int rmask_last; \
	bool exit = false; \
	pa_err = 0; \
	do { \
		module_idx = -1; \
		rmask_last = rmask_curr; \
		rmask_curr = 0; \
		list_for_each_entry( _np, &(ctx)->pa_list ,entries){ \
			if (PA_EXISTS(PA_FUNC func)) { \
				++module_idx; \
				if (!(rmask_last & \
						((unsigned long long int)1 << module_idx))) \
					continue; \
				pa_err = (PA_GLUE(PA_FUNC func)) PA_ARGS args; \
				if (pa_err != 0) { \
					if (pa_err == PON_ADAPTER_EAGAIN) { \
						rmask_curr |= ((unsigned long long int)1 << \
							module_idx); \
						continue; \
					} \
					dbprintf(LOG_ERR, "PON Adapter module %s " \
						"failed on function %s.\n", \
						_np->definition->name, #func); \
					exit = true; \
					break; \
				} \
			} \
		} \
	} while (!exit && rmask_curr && rmask_curr != rmask_last); \
\
	if (!rmask_curr) \
		break; \
	module_idx = -1; \
	list_for_each_entry( _np, &(ctx)->pa_list ,entries) { \
		if (PA_EXISTS(PA_FUNC func)) { \
			++module_idx; \
			if (rmask_curr & ((unsigned long long int)1 << module_idx)) \
				dbprintf(LOG_ERR, "PON Adapter module %s " \
					"failed on function %s.", \
					_np->definition->name, #func); \
		} \
	} \
} while (0)


/** OMCI_IK key context*/
struct intel_omci_ik_context {
	/** OMCI_IK key context lock */
	struct fwk_mutex_t lock;

	/** Array containing OMCI integrity key */
	unsigned char key[OMCI_MIC_IK_LEN];
};

struct intel_omci_msg_stats {
	/** Lock for this structure */
	struct fwk_mutex_t lock;

	/** Number of valid messages */
	unsigned long rx_valid;

	/** Number of invalid messages */
	unsigned long rx_invalid;
};

/** PON adapter error numbering */
enum intel_omci_errno {
	/** Try again */
	INTEL_PON_EAGAIN					= 1,
	/** Success */
	INTEL_PON_SUCCESS					= 0,
	/** Unspecific error */
	INTEL_PON_ERROR					= -1,
	/** Resource was not found */
};


struct intel_pa_ll_definition {
	/** Name of the lower layer module. The name of the dynamic library and
	 * the name of the register function loaded from the library will be
	 * deduced from this name. The name of the dynamic library is "name.so".
	 * The name of the init function will be "name_ll_register_ops".
	 */
	char name[PA_LOWER_LAYER_MAX_NAME_LEN];
	/** Temporary pointer to the register function. If it is set to NULL,
	 * then the register function will be loaded from the dynamic library.
	 * If it is set to a function pointer, the dynamic library won't be
	 * loaded.
	 */
	ll_register_t *const ll_reg;
};
/**
 *	PON Adapter list node
 */
struct pa_node {
	/** Definition used to create pa_node */
	const struct intel_pa_ll_definition *definition;
	/** Pointer to lower layer operations structure */
	const struct pa_ops *pa_ops;
	/** Pointer to lower layer context */
	void *ll_ctx;
	/** Temporary pointer to register function */
	ll_register_t *ll_reg;

	/** Handle to dynamic library */
	void *dl_handle;

	/** Initialization status of lower layer module */
	bool initialized;
	/** */
	struct fwk_mutex_t pa_lock;
	
	struct list_head entries;
};

struct intel_omcc_context {
	/** Counters for valid and invalid messages */
	struct intel_omci_msg_stats msg_stats;

	/** Function to check validity of received messages */
	enum intel_omci_errno (*msg_mic_check)(void *context,
					 const void *omci_msg, int len,
					 unsigned int crc_mic);

	/** Function to calculate crc or mic */
	enum intel_omci_errno (*msg_mic_calc)(void *context,
					const void *omci_msg, int len,
					unsigned int *crc_mic);
};


struct intel_omci_context {
	/** OLT Vendor ID. Updated within OLT-G ME. */
	char olt_id[4];

	char op_mode_char[UTIL_UCI_MAX_LEN];
	
	int num_lower_layers;
	struct intel_pa_ll_definition *lower_layers;
	
	/** OMCC related context */
	struct intel_omcc_context omcc_ctx;

	/** LOID authentication status */
	unsigned char loid_auth_status;
	
	char const *const *param_list;
	/** MCC context */
	void *mcc;
	int mcc_init_done;
#ifdef INCLUDE_MIC_CHECK
	/** Context for OMCI_IK */
	struct intel_omci_ik_context omci_ik;
#endif /* ifdef INCLUDE_MIC_CHECK */


	/** List of lower layer modules */
	struct list_head pa_list;

	/** Operation mode */
	enum pa_pon_op_mode op_mode;

	void (*omcc_raw_rcv)(void *msg,unsigned short len,unsigned int crc );
	//
	/** last reported PLOAM state */
	int last_ploam_state;
	
	/** Logical ONU ID (24 bytes) */
	char loid[LOID_LEN];

	/** Logical ONU password (12 bytes) */
	char lpwd[LPWD_LEN];
};
/*this partial fapi_pon_wrapper_ctx */
struct px126_fapi_pon_wrapper_ctx 
{
	/** FAPI PON handle structure */
	void *pon_ctx;
	/** FAPI PON event context handle structure */
	void *ponevt_ctx;
	/** Pointer to context of higher layer */
	void *hl_ctx;
};

int intel_omci_save_init_param(const char *name, const char *value);
void intel_omci_register_omcc_msg_cb(void (*omcc_raw_rcv)(void *msg,unsigned short len,unsigned int crc ));
void intel_omci_omcc_msg_send(const char *omci_msg,
			      const unsigned short len,
			      unsigned int crc_mic);

int intel_omci_init(void);
int intel_omcc_init(struct intel_omci_context *context);
int intel_omcc_msg_send(struct intel_omci_context *context,
			      const char *omci_msg,
			      const unsigned short len,
			      unsigned int crc_mic);
int 
intel_omci_config_dbglvl(unsigned short is_set, unsigned short dbgmodule,unsigned short *dbglvl);
struct intel_omci_context* intel_omci_get_omci_context(void);
int intel_omci_bridge_inf_create(unsigned int brd_meid);
int intel_omci_bridge_inf_del(unsigned int brd_meid);
int intel_omci_bridge_port_cfg_update(unsigned short meid,void* upd_data,int is_del);
struct pa_node *intel_omci_pa_lib_get_byname(const char *name);
int intel_omci_bridge_port_connect(unsigned short br_meid,unsigned short br_port_meid,unsigned char tp_type,unsigned short tp_ptr);

int intel_omci_pptp_lct_uni_create(unsigned short meid);
int intel_omci_pptp_lct_uni_update(unsigned short meid, unsigned char state_admin);
int intel_omci_pptp_lct_uni_destory(unsigned short meid);

enum pa_pon_op_mode intel_omci_pa_get_pon_op_mode(struct intel_omci_context *context);

int intel_omci_pa_mapper_dump(void);

#endif
