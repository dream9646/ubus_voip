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
 * Module  : meinfo_hw
 * File    : meinfo_hw_util.h
 *
 ******************************************************************/

#ifndef __MEINFO_HW_UTIL_H__
#define __MEINFO_HW_UTIL_H__

#include "meinfo.h"
#include "metacfg_adapter.h"

/* meinfo_hw_util.c */
int meinfo_hw_util_get_ontstate(void);
int meinfo_hw_util_get_omci_by_fvcli_epregstate(unsigned short port_id);
int meinfo_hw_util_get_voip_param_by_port(struct me_t *me, void *param, unsigned char attr_mask[2], unsigned char omci_attr_mask[2]);
int meinfo_hw_util_me_delete(unsigned short classid, unsigned short meid);
int meinfo_hw_util_set_attr_meid(unsigned short classid, struct me_t *me, unsigned char attr_order, unsigned short meid, int check_avc);
int meinfo_hw_util_me_new(unsigned short classid, int is_private, struct me_t **me);
#ifdef OMCI_METAFILE_ENABLE
int meinfo_hw_util_attr_sync_class_157(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey);
int meinfo_hw_util_attr_sync_class_137(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey);
int meinfo_hw_util_attr_sync_class_148(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey1, char *fmt_mkey2);
int meinfo_hw_util_is_mkey_voip_changed(struct metacfg_t **kv_pp);
void meinfo_hw_util_kv_voip_release(void);
#endif

#endif
