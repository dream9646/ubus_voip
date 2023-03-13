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
 * File    : meinfo_hw_util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>


#include "util.h"
#include "iphost.h"
#include "metacfg_adapter.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "voip_hw.h"
#include "gpon_sw.h"
#include "meinfo_hw_util.h"
#include "voip_hw.h"
#include "voipclient_cmd.h"

#ifdef OMCI_METAFILE_ENABLE
static time_t metafile_ts_voip_g=0;
static struct metacfg_t kv_config_voip_g;
#endif
extern int er_group_hw_voip_wrapper_wri_mg_port_register_control(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm);
extern int er_group_hw_voip_wrapper_wri_fax_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm);
extern int er_group_hw_voip_wrapper_wri_ngn_port_statics(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm);
extern int gponmacFd0;

int
meinfo_hw_util_get_ontstate(void)
{
	unsigned int ontstate = 0;
	if (gpon_hw_g.onu_state_get)
		gpon_hw_g.onu_state_get(&ontstate);
	return ontstate;
}

int
meinfo_hw_util_get_omci_by_fvcli_epregstate(unsigned short port_id)
{
	int temp_int;
	if (0 == voipclient_cmd_get_int(&temp_int, "epregstate %d", port_id)) {
		switch (temp_int) {
		case PORT_STATUS__INITIAL:	temp_int = SIP_STATUS__INITIAL;
			break;
		case PORT_STATUS__REGISTERED:	temp_int = SIP_STATUS__REGISTERED;
			break;
		case PORT_STATUS__IN_SESSION:	temp_int = SIP_STATUS__IN_SESSION;
			break;
		case PORT_STATUS__FAILED_ICMP:	temp_int = SIP_STATUS__FAILED_ICMP;
			break;
		case PORT_STATUS__FAILED_TCP:	temp_int = SIP_STATUS__FAILED_TCP;
			break;
		case PORT_STATUS__FAILED_AUTH:	temp_int = SIP_STATUS__FAILED_AUTH;
			break;
		case PORT_STATUS__FAILED_TIMEOUT: temp_int = SIP_STATUS__FAILED_TIMEOUT;
			break;
		case PORT_STATUS__FAILED_SERVER: temp_int = SIP_STATUS__FAILED_SERVER;
			break;
		case PORT_STATUS__FAILED_OTHER: temp_int = SIP_STATUS__FAILED_OTHER;
			break;
		case PORT_STATUS__REGISTERING: temp_int = SIP_STATUS__REGISTERING;
			break;
		case PORT_STATUS__UNREGISTERING: temp_int = SIP_STATUS__UNREGISTERING;
			break;
		default:
			temp_int = SIP_STATUS__FAILED_AUTH;
		}
	} else {
		temp_int = -1;
	}
	return temp_int;
}

int
meinfo_hw_util_get_voip_param_by_port(struct me_t *me, void *param, unsigned char attr_mask[2], unsigned char omci_attr_mask[2])
{
	unsigned short port_id;
	int ret = 0;
	struct me_t *port_me;

	//find Physical_path_termination_point_POTS_UNI for port number
	if (me == NULL || attr_mask == NULL || omci_attr_mask == NULL || (port_me = er_attr_group_util_find_related_me(me, 53)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related pots port\n");
		return -1;
	}

	//port id
	port_id = port_me->instance_num;

	switch (me->classid)
	{
	//9.9.2
	case 153:
		ret = er_group_hw_voip_wrapper_sip_user_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_sip_user_data_t *)param);
		break;
	//9.9.3
	case 150:
		ret = er_group_hw_voip_wrapper_sip_agent_config_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_sip_agent_config_data_t *)param);
		break;
	//9.9.4
	case 139:
		ret = er_group_hw_voip_wrapper_voip_voice_ctp(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voip_voice_ctp_t *)param);
		break;
	//9.9.5
	case 142:
		ret = er_group_hw_voip_wrapper_voip_media_profile(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voip_media_profile_t *)param);
		break;
	//9.9.6
	case 58:
		ret = er_group_hw_voip_wrapper_voice_service_profile(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voice_service_profile_t *)param);
		break;
	//9.9.7
	case 143:
		ret = er_group_hw_voip_wrapper_rtp_profile_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_rtp_profile_data_t *)param);
		break;
	//9.9.8
	case 146:
		ret = er_group_hw_voip_wrapper_voip_application_service_profile(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voip_application_service_profile_t *)param);
		break;
	//9.9.9
	case 147:
		ret = er_group_hw_voip_wrapper_voip_feature_access_codes(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voip_feature_access_codes_t *)param);
		break;
	//9.9.10
	case 145:
		ret = er_group_hw_voip_wrapper_network_dial_plan_table(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_network_dial_plan_table_t *)param);
		break;
	//9.9.11
	case 141:
		ret = er_group_hw_voip_wrapper_voip_line_status(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_voip_line_status_t *)param);
		break;
	//9.9.12
	case 140:
		ret = er_group_hw_voip_wrapper_call_control_performance_monitoring_history_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_call_control_performance_monitoring_history_data_t *)param);
		break;
	//9.9.13
	case 144:
		ret = er_group_hw_voip_wrapper_rtp_performance_monitoring_history_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_rtp_performance_monitoring_history_data_t *)param);
		break;
	//9.9.14
	case 151:
		ret = er_group_hw_voip_wrapper_sip_agent_performance_monitoring_history_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_sip_agent_performance_monitoring_history_data_t *)param);
		break;
	//9.9.15
	case 152:
		ret = er_group_hw_voip_wrapper_sip_call_initiation_performance_monitoring_history_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t *)param);
		break;
	//9.99.14
	//proprietary_fiberhome
	case 65416:
		ret = er_group_hw_voip_wrapper_wri_mg_port_register_control(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_wri_mg_port_register_control_t *)param);
		break;
	//9.99.16
	case 65418:
	//proprietary_fiberhome
		ret = er_group_hw_voip_wrapper_wri_fax_config_data(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_wri_fax_config_data_t *)param);
		break;
	//9.99.28
	case 65426:
		ret = er_group_hw_voip_wrapper_wri_ngn_port_statics(VOIP_HW_ACTION_GET, port_id, attr_mask, (struct voip_hw_wri_ngn_port_statics_t *)param);
		break;
	default:
		dbprintf(LOG_ERR, "This is not a voip me, classid=%u, meid=%u\n", me->classid, me->meid);
		return -1;
	}

	omci_attr_mask[0] = attr_mask[0];
	omci_attr_mask[1] = attr_mask[1];

	return ret;
}

int
meinfo_hw_util_me_delete(unsigned short classid, unsigned short meid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;

	if (miptr == NULL) {
		dbprintf(LOG_ERR, "classid=%u, miptr null?\n", classid);
		return -1;
	}
	if (NULL == (me=meinfo_me_get_by_meid(classid, meid))) {
		dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), me not exist?\n", classid, meid, meid);
		return -2;
	}
	if (meinfo_me_delete_by_instance_num(classid, me->instance_num) < 0) {
		return -3;
	}
	return 0;
}

int
meinfo_hw_util_set_attr_meid(unsigned short classid, struct me_t *me, unsigned char attr_order, unsigned short meid, int check_avc)
{
	struct attr_value_t attr;
	char buf[16];
	unsigned char attr_mask[2]={0, 0};

	attr.ptr=NULL;
	snprintf(buf, 15, "0x%x", (unsigned short)meid);
	meinfo_conv_string_to_attr(classid, attr_order, &attr, buf);
	if (meinfo_me_attr_is_valid(me, attr_order, &attr)) {
		meinfo_me_attr_set(me, attr_order, &attr, check_avc);
		util_attr_mask_set_bit(attr_mask, attr_order);
		// call cb and hw after mib write
		meinfo_me_flush(me, attr_mask);
	} else {
		dbprintf(LOG_ERR, "meinfo_me_attr_is_valid() failed\n");
	}
	return 0;
}

int
meinfo_hw_util_me_new(unsigned short classid, int is_private, struct me_t **me)
{
	short instance_num;
	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0) {
		dbprintf(LOG_ERR, "meinfo_instance_alloc_free_num(%u) failed\n", classid);
		return -1;
	}
	if (NULL == (*me=meinfo_me_alloc(classid, instance_num))) {
		dbprintf(LOG_ERR, "meinfo_me_alloc() failed\n");
		return -1;
	}
	// set default from config
	meinfo_me_load_config(*me);
	// assign meid
	if (meinfo_me_allocate_rare_meid(classid, &((*me)->meid)) < 0) {
		meinfo_me_release(*me);
		dbprintf(LOG_ERR, "meinfo_me_allocate_rare_meid() failed classid(%u)\n", classid);
		return -1;
	}
	//create by olt
	(*me)->is_cbo = 0;
	if (is_private) {
		(*me)->is_private = 1;
	} else {
		(*me)->is_private = 0;
	}
	if (meinfo_me_create(*me) < 0) {
		meinfo_me_release(*me);
		dbprintf(LOG_ERR, "me classid(%u) create error\n", classid);
		return -1;
	} else {
		dbprintf(LOG_DEBUG, "meinfo_me_create() succ meid(%u)(0x%x) instance_num(%d)\n", (*me)->meid, (*me)->meid, instance_num);
	}
	return 0;
}

#ifdef OMCI_METAFILE_ENABLE
int
meinfo_hw_util_attr_sync_class_157(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey)
{
	unsigned short meid157_me;
	char *p, buf[128];
	struct me_t *me157=NULL;

	snprintf(buf, 127, fmt_mkey, port_id);
	metacfg_adapter_config_file_load_part(kv_config, "/etc/wwwctrl/metafile.dat", buf, buf);
	if (0xffff > (meid157_me=(unsigned short)meinfo_util_me_attr_data(me, attr_order))) {
		me157 = meinfo_me_get_by_meid(157, meid157_me);
	} else {
		me157 = NULL;
	}
	dbprintf(LOG_DEBUG, "me attr(%d) meid157(%d 0x%x) port(%d)\n", attr_order, meid157_me, meid157_me, port_id);
	if (NULL != (p=metacfg_adapter_config_get_value(kv_config, buf, 0)) &&
	    strlen(p) > 0) {
		if (!me157) {
			dbprintf(LOG_DEBUG, "To new a me157 for class(%d) attr(%d)\n", classid, attr_order);
			if (0 != meinfo_hw_util_me_new(157, 0, &me157)) {
				return -1;
			} else {
				meinfo_hw_util_set_attr_meid(classid, me, attr_order, (unsigned short)me157->meid, 0);
			}
		}
		meinfo_me_copy_to_large_string_me(me157, p);
		dbprintf(LOG_INFO, "mkey[%s]=[%s] to class 157 meid(%u 0x%x)\n", buf, p, me157->meid, me157->meid);
	} else if (meid157_me < 0xffff && me157 != NULL) {
		dbprintf(LOG_NOTICE, "TO remove class 157 meid(%d)\n", meid157_me);
		meinfo_hw_util_set_attr_meid(classid, me, attr_order, 0xffff, 0);
		meinfo_hw_util_me_delete(157, meid157_me);
	}
	return 0;
}

static int
meinfo_hw_util__me_delete_137(unsigned short meid)
{
	unsigned short meid2;
	struct me_t *me;

	if (NULL != (me=meinfo_me_get_by_meid(137, meid))) {
		if (0xffff > (meid2=(unsigned short)meinfo_util_me_attr_data(me, 1))) {
			meinfo_hw_util_me_delete(148, meid2);
		}
		if (0xffff > (meid2=(unsigned short)meinfo_util_me_attr_data(me, 2))) {
			meinfo_hw_util_me_delete(157, meid2);
		}
		meinfo_hw_util_me_delete(137, meid);
	}
	return 0;
}

int
meinfo_hw_util_attr_sync_class_137(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey)
{
	unsigned short meid137_me;
	char *p, buf[128];
	struct me_t *me137=NULL, *me157=NULL;

	snprintf(buf, 127, fmt_mkey, port_id);
	metacfg_adapter_config_file_load_part(kv_config, "/etc/wwwctrl/metafile.dat", buf, buf);
	if (0xffff > (meid137_me=(unsigned short)meinfo_util_me_attr_data(me, attr_order))) {
		me137 = meinfo_me_get_by_meid(137, meid137_me);
	} else {
		me137 = NULL;
	}
	dbprintf(LOG_DEBUG, "class(%d) attr(%d) (%d 0x%x) port(%d)\n", classid, attr_order, meid137_me, meid137_me, port_id);
	if (NULL != (p=metacfg_adapter_config_get_value(kv_config, buf, 0)) &&
	    strlen(p) > 0) {
		if (!me137) {
			dbprintf(LOG_DEBUG, "To new a me 137 for attr(%d)\n", attr_order);
			if (0 != meinfo_hw_util_me_new(137, 0, &me137)) {
				return -1;
			} else {
				meinfo_hw_util_set_attr_meid(classid, me, attr_order, (unsigned short)me137->meid, 0);
			}
			dbprintf(LOG_DEBUG, "To new a me137 for class(%d) attr(%d)\n", classid, attr_order);
			if (0 != meinfo_hw_util_me_new(157, 0, &me157)) {
				return -1;
			} else {
				meinfo_hw_util_set_attr_meid(137, me137, 1, 0xffff, 0);
				meinfo_me_copy_to_large_string_me(me157, p);
				meinfo_hw_util_set_attr_meid(137, me137, 2, (unsigned short)me157->meid, 0);
			}
			dbprintf(LOG_INFO, "Done class(%d) attr(%d)=[%s] mkey[%s] to class(157) meid(%u 0x%x) via class(137) meid (%u 0x%x)\n", classid, attr_order, p, buf, me157->meid, me157->meid, me137->meid, me137->meid);
		}
	} else if (meid137_me < 0xffff && me137 != NULL) {
		dbprintf(LOG_NOTICE, "TO remove class 137 meid(%d)\n", meid137_me);
		meinfo_hw_util_set_attr_meid(classid, me, attr_order, 0xffff, 0);
		meinfo_hw_util__me_delete_137(meid137_me);
	}
	return 0;
}

int
meinfo_hw_util_attr_sync_class_148(unsigned short classid, unsigned char attr_order, struct me_t *me, int port_id, struct metacfg_t *kv_config, char *fmt_mkey1, char *fmt_mkey2)
{
	unsigned short meid148_me, flag1=0, flag2=0;
	char *p1, *p2, buf1[64], buf2[64];
	struct me_t *me148=NULL;
	struct attr_value_t attr;

	snprintf(buf1, 63, fmt_mkey1, port_id);
	snprintf(buf2, 63, fmt_mkey2, port_id);
	metacfg_adapter_config_file_load_part(kv_config, "/etc/wwwctrl/metafile.dat", buf1, buf1);
	metacfg_adapter_config_file_load_part(kv_config, "/etc/wwwctrl/metafile.dat", buf2, buf2);
	if (0xffff > (meid148_me=(unsigned short)meinfo_util_me_attr_data(me, attr_order))) {
		me148 = meinfo_me_get_by_meid(148, meid148_me);
	} else {
		me148 = NULL;
	}
	dbprintf(LOG_DEBUG, "me attr(%d) meid157(%d 0x%x) port(%d)\n", attr_order, meid148_me, meid148_me, port_id);
	if (NULL != (p1=metacfg_adapter_config_get_value(kv_config, buf1, 0)) && strlen(p1) > 0) {
		flag1=1;
	}
	if (NULL != (p2=metacfg_adapter_config_get_value(kv_config, buf2, 0)) && strlen(p2) > 0) {
		flag2=1;
	}
	if (flag1 || flag2) {
		if (!me148) {
			dbprintf(LOG_DEBUG, "To new a me148 for class(%d) attr(%d)\n", classid, attr_order);
			if (0 != meinfo_hw_util_me_new(148, 0, &me148)) {
				return -1;
			} else {
				meinfo_hw_util_set_attr_meid(classid, me, attr_order, (unsigned short)me148->meid, 0);
			}
		}
		/* set class 148 attr 2 & 3 */
		if (flag1) {
			attr.ptr = strdup(p1);
			dbprintf(LOG_INFO, "2:Username_1 set to [%s] mkey[%s]\n", p1, buf1);
			meinfo_me_attr_set(me148, 2, &attr, 0);
		} else {
			dbprintf(LOG_NOTICE, "2:Username_1 no set because mkey[%s] is null value\n", buf1);
		}
		if (flag2) {
			attr.ptr = strdup(p2);
			dbprintf(LOG_INFO, "3:Password set to [%s] mkey[%s]\n", p2, buf2);
			meinfo_me_attr_set(me148, 3, &attr, 0);
		} else {
			dbprintf(LOG_NOTICE, "3:Password no set because mkey[%s] is null value\n", buf2);
		}
	} else if (meid148_me < 0xffff && me148 != NULL) {
		dbprintf(LOG_NOTICE, "TO remove class 148 meid(%d)\n", meid148_me);
		meinfo_hw_util_set_attr_meid(classid, me, attr_order, 0xffff, 0);
		meinfo_hw_util_me_delete(148, meid148_me);
	}
	return 0;
}

/* return value 1:changed 0:no_change */
int
meinfo_hw_util_is_mkey_voip_changed(struct metacfg_t **kv_pp)
{
	int ret=0;
	struct metacfg_t kv_config;

	*kv_pp = &kv_config_voip_g;
	if (0 != metafile_ts_voip_g && 0 == util_is_file_changed(METAFILE_PATH, &metafile_ts_voip_g)) {
		dbprintf_voip(LOG_DEBUG, "metafile.dat access timestamp no changed\n");
		return 0;
	}
	if (0 == metafile_ts_voip_g) {
		util_is_file_changed(METAFILE_PATH, &metafile_ts_voip_g);
		metacfg_adapter_config_init(*kv_pp);
		util_kv_load_voip(*kv_pp, METAFILE_PATH);
		dbprintf_voip(LOG_ERR, "The first time to load metafile.dat voip_* and system_voip_*\n");
		ret=1;
	} else {
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		util_kv_load_voip(&kv_config, METAFILE_PATH);
		ret=metacfg_adapter_config_merge(*kv_pp, &kv_config, 1);
		metacfg_adapter_config_release(&kv_config);
		if (0 == ret) {
			dbprintf_voip(LOG_DEBUG, "No diff VOIP metakeys of metafile.dat\n");
			ret=0;
		} else {
			dbprintf_voip(LOG_ERR, "There are different VOIP metakeys(%d) values\n", ret);
			ret=1;
		}
	}
	return ret;
}

void
meinfo_hw_util_kv_voip_release(void)
{
	metafile_ts_voip_g=0;
	metacfg_adapter_config_release(&kv_config_voip_g);
	return;
}
#endif

