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
 * File    : meinfo_hw_53.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "voipclient_cmd.h"
#include "metacfg_adapter.h"
#include "batchtab_cb.h"
#include "meinfo_hw_util.h"

//9.9.1 Physical_path_termination_point_POTS_UNI
static int 
meinfo_hw_53_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
#ifdef OMCI_METAFILE_ENABLE
	int value, voipImp=0;
#endif
	unsigned char uc;
#ifdef OMCI_METAFILE_ENABLE
	char buf[64], *p;
#endif
	struct attr_value_t attr;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t *kv_p;
#endif

	if (me == NULL) {
		dbprintf(LOG_ERR, "Can't find this pots port\n");
		return -1;
	}
	if (BATCHTAB_POTS_MKEY_HW_SYNC_DONE != (ret=batchtab_cb_pots_is_hw_sync())) {
		dbprintf(LOG_NOTICE, "pots_mkey is not hw_sync (%d)\n", ret);
		return 0;
	}

	//port id
	port_id = me->instance_num;
	dbprintf_voip(LOG_INFO, "attr_mask[0](0x%02x) attr_mask[1](0x%02x) port(%d)\n", attr_mask[0], attr_mask[1], port_id);
	if (util_attr_mask_get_bit(attr_mask, 10)) {
		/* 10: Hook_state */
		if (0 == voipclient_cmd_get_hookstate(&uc, port_id)) {
			attr.data=0;
			uc = uc -'0';
			dbprintf_voip(LOG_INFO, "hookstate(%d) port(%d)\n", uc, port_id);
			attr.data = uc;
			meinfo_me_attr_set(me, 10, &attr, 0);
		}
	}

#ifdef OMCI_METAFILE_ENABLE
	if (0 == meinfo_hw_util_is_mkey_voip_changed(&kv_p)) {
		dbprintf_voip(LOG_INFO, "No diff VOIP metakeys, no need to refresh me(%d)\n", me->classid);
		return 0;
	}
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		/* 1: Administrative_state */
		/* administrative_state 0  <-> accountenable 1 */
		/* administrative_state 1  <-> accountenable 0 */
		snprintf(buf, 63, "voip_ep%u_account_enable", port_id);
		if (0 == util_atoi(metacfg_adapter_config_get_value(kv_p, buf, 1))) {
			attr.data = 1;
		} else {
			attr.data = 0;
		}
		meinfo_me_attr_set(me, 1, &attr, 0);
		dbprintf_voip(LOG_INFO, "administrative_state(%d) port(%d) mkey[%s]\n", attr.data, port_id, buf);
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: Impedance */
		snprintf(buf, 63, "voip_ep%u_tdi_impedance_index", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			value = util_atoi(p);
			if (value == 0) {
				voipImp = 0;
			} else if (value == 9) {
				voipImp = 1;
			} else if (value == 1) {
				voipImp = 2;
			} else if (value == 3) {
				voipImp = 3;
			} else if (value == 10) {
				voipImp = 4;
			} else {
				voipImp = -1;
			}
			if (0 <= voipImp) {
				attr.data = (unsigned char)voipImp;
				meinfo_me_attr_set(me, 5, &attr, 0);
				dbprintf_voip(LOG_INFO, "Impedance(%d) port(%d)\n", voipImp, port_id);
			} else {
				dbprintf_voip(LOG_ERR, "Impedance metakey[%s] value(%d) out of range! port(%d)\n", buf, value, port_id);
			}
		} else {
			dbprintf_voip(LOG_ERR, "Impedance no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {
		/* 7: Rx_gain */ /* -120 to 60 */
		snprintf(buf, 63, "voip_ep%u_sysrx_gain_cb", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			value = util_atoi(p);
			if(value < -120) {
				value = -120;
			} else if(value > 60) {
				value = 60;
			}
			attr.data = (char)value;
			meinfo_me_attr_set(me, 7, &attr, 0);
			dbprintf_voip(LOG_INFO, "rx_gain(%d) port(%d)\n", value, port_id);
		} else {
			dbprintf_voip(LOG_ERR, "Rx_gain no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) {
		/* 8: Tx_gain */ /* -120 to 60 */
		snprintf(buf, 63, "voip_ep%u_systx_gain_cb", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			if(value < -120) {
				value = -120;
			} else if(value > 60) {
				value = 60;
			}
			attr.data = (char)value;
			meinfo_me_attr_set(me, 8, &attr, 0);
			dbprintf_voip(LOG_INFO, "tx_gain(%d) port(%d)\n", value, port_id);
		} else {
			dbprintf_voip(LOG_ERR, "Tx_gain no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
#endif
	return 0;
}

static int 
meinfo_hw_53_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	unsigned short port_id;
	struct omcimsg_test_pots_isdn_t *test_req_msg = (struct omcimsg_test_pots_isdn_t *)req;
	struct omcimsg_test_result_pots_isdn_t *test_result_msg =(struct omcimsg_test_result_pots_isdn_t *)result;
	struct omcimsg_test_pots_isdn_t test_req;
	struct omcimsg_test_result_pots_isdn_t test_result;
	int test_class = (test_req_msg->select_test>>6) & 0x3;

	memset(&test_req, 0x00, sizeof(struct omcimsg_test_pots_isdn_t));
	memset(&test_result, 0x00, sizeof(struct omcimsg_test_result_pots_isdn_t));

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "Can't find this pots port\n");
		return -1;
	}

	//port id
	port_id = me->instance_num;
	test_req.select_test = test_req_msg->select_test;
	
	if (test_class == 0) {	
		test_req.u.class0.dbdt_timer_t1 = test_req_msg->u.class0.dbdt_timer_t1;
		test_req.u.class0.dbdt_timer_t2 = test_req_msg->u.class0.dbdt_timer_t2;
		test_req.u.class0.dbdt_timer_t3 = test_req_msg->u.class0.dbdt_timer_t3;
		test_req.u.class0.dbdt_timer_t4 = test_req_msg->u.class0.dbdt_timer_t4;
		test_req.u.class0.dbdt_control_byte = test_req_msg->u.class0.dbdt_control_byte;
		test_req.u.class0.digit_to_be_dialled = test_req_msg->u.class0.digit_to_be_dialled;
		test_req.u.class0.dial_tone_frequency_1 = ntohs(test_req_msg->u.class0.dial_tone_frequency_1);
		test_req.u.class0.dial_tone_frequency_2 = ntohs(test_req_msg->u.class0.dial_tone_frequency_2);
		test_req.u.class0.dial_tone_frequency_3 = ntohs(test_req_msg->u.class0.dial_tone_frequency_3);
		test_req.u.class0.dial_tone_power_threshold = test_req_msg->u.class0.dial_tone_power_threshold;
		test_req.u.class0.idle_channel_power_threshold = test_req_msg->u.class0.idle_channel_power_threshold;
		test_req.u.class0.dc_hazardous_voltage_threshold = test_req_msg->u.class0.dc_hazardous_voltage_threshold;
		test_req.u.class0.ac_hazardous_voltage_threshold = test_req_msg->u.class0.ac_hazardous_voltage_threshold;
		test_req.u.class0.dc_foreign_voltage_threshold = test_req_msg->u.class0.dc_foreign_voltage_threshold;
		test_req.u.class0.ac_foreign_voltage_threshold = test_req_msg->u.class0.ac_foreign_voltage_threshold;
		test_req.u.class0.tip_ground_and_ring_ground_resistance_threshold = test_req_msg->u.class0.tip_ground_and_ring_ground_resistance_threshold;
		test_req.u.class0.tip_ring_resistance_threshold = test_req_msg->u.class0.tip_ring_resistance_threshold;
		test_req.u.class0.ringer_equivalence_min_threshold = ntohs(test_req_msg->u.class0.ringer_equivalence_min_threshold);
		test_req.u.class0.ringer_equivalence_max_threshold = ntohs(test_req_msg->u.class0.ringer_equivalence_max_threshold);
		test_req.u.class0.pointer_to_a_general_purpose_buffer_me = ntohs(test_req_msg->u.class0.pointer_to_a_general_purpose_buffer_me);
		test_req.u.class0.pointer_to_an_octet_string_me = ntohs(test_req_msg->u.class0.pointer_to_an_octet_string_me);
	} else if (test_class == 1) {
		strncpy(test_req.u.class1.number_to_dial, test_req_msg->u.class1.number_to_dial, 16);
	} else {
		dbprintf(LOG_ERR, "POTS test class %d not supported, meid=%\n", test_class, me->meid);
		return -1;
	}

	//call voip test api
	if (voip_hw_physical_path_termination_point_pots_uni_test(port_id, &test_req, &test_result) == 0)
	{
		//success
		if (test_class == 0) {	
			test_result_msg->select_test_and_mlt_drop_test_result = test_result.select_test_and_mlt_drop_test_result;
			test_result_msg->self_or_vendor_test_result = test_result.self_or_vendor_test_result;
			test_result_msg->dial_tone_make_break_flags = test_result.dial_tone_make_break_flags;
			test_result_msg->dial_tone_power_flags = test_result.dial_tone_power_flags;
			test_result_msg->loop_test_dc_voltage_flags = test_result.loop_test_dc_voltage_flags;
			test_result_msg->loop_test_ac_voltage_flags = test_result.loop_test_ac_voltage_flags;
			test_result_msg->loop_test_resistance_flags_1 = test_result.loop_test_resistance_flags_1;
			test_result_msg->loop_test_resistance_flags_2 = test_result.loop_test_resistance_flags_2;
			test_result_msg->time_to_draw_dial_tone = test_result.time_to_draw_dial_tone;
			test_result_msg->time_to_break_dial_tone = test_result.time_to_break_dial_tone;
			test_result_msg->total_dial_tone_power_measurement = test_result.total_dial_tone_power_measurement;
			test_result_msg->quiet_channel_power_measurement = test_result.quiet_channel_power_measurement;
			test_result_msg->tip_ground_dc_voltage = htons(test_result.tip_ground_dc_voltage);
			test_result_msg->ring_ground_dc_voltage = htons(test_result.ring_ground_dc_voltage);
			test_result_msg->tip_ground_ac_voltage = test_result.tip_ground_ac_voltage;
			test_result_msg->ring_ground_ac_voltage = test_result.ring_ground_ac_voltage;
			test_result_msg->tip_ground_dc_resistance = htons(test_result.tip_ground_dc_resistance);
			test_result_msg->ring_ground_dc_resistance = htons(test_result.ring_ground_dc_resistance);
			test_result_msg->tip_ring_dc_resistance = htons(test_result.tip_ring_dc_resistance);
			test_result_msg->ringer_equivalence = test_result.ringer_equivalence;
			test_result_msg->pointer_to_a_general_purpose_buffer_me = test_result.pointer_to_a_general_purpose_buffer_me;
			test_result_msg->loop_tip_ring_test_ac_dc_voltage_flags = test_result.loop_tip_ring_test_ac_dc_voltage_flags;
			test_result_msg->tip_ring_ac_voltage = test_result.tip_ring_ac_voltage;
			test_result_msg->tip_ring_dc_voltage = htons(test_result.tip_ring_dc_voltage);
		} else if (test_class == 1) {
			test_result_msg->select_test_and_mlt_drop_test_result = test_result.select_test_and_mlt_drop_test_result;		
		}
	} else {
		//fail
		dbprintf(LOG_ERR, "POTS test error, meid=%\n", me->meid);
		return -1;
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_53 = {
	.get_hw		= meinfo_hw_53_get,
	.test_hw	= meinfo_hw_53_test
};

