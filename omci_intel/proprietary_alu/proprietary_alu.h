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
 * Module  : proprietary_alu
 * File    : proprietary_alu.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_ALU_H__
#define __PROPRIETARY_ALU_H__

#ifndef X86
#include "er_group.h"
#endif

/* me_related_011.c */
int me_related_011_65297(struct me_t *me1, struct me_t *me2);
int me_related_011_65301(struct me_t *me1, struct me_t *me2);
int me_related_011_65305(struct me_t *me1, struct me_t *me2);
int me_related_011_65306(struct me_t *me1, struct me_t *me2);

/* me_related_047.c */
int me_related_047_65296(struct me_t *me1, struct me_t *me2);

/* me_related_053.c */
int me_related_053_65292(struct me_t *me1, struct me_t *me2);

/* me_related_277.c */
int me_related_277_65323(struct me_t *me1, struct me_t *me2);

/* me_related_277.c */
int me_related_280_65323(struct me_t *me1, struct me_t *me2);

/* me_related_310.c */
int me_related_310_65531(struct me_t *me1, struct me_t *me2);

/* me_related_329.c */
int me_related_329_65297(struct me_t *me1, struct me_t *me2);
int me_related_329_65301(struct me_t *me1, struct me_t *me2);
int me_related_329_65305(struct me_t *me1, struct me_t *me2);
int me_related_329_65306(struct me_t *me1, struct me_t *me2);

/* me_related_65292.c */
int me_related_65292_053(struct me_t *me1, struct me_t *me2);

/* me_related_65296.c */
int me_related_65296_047(struct me_t *me1, struct me_t *me2);

/* me_related_65297.c */
int me_related_65297_011(struct me_t *me1, struct me_t *me2);
int me_related_65297_329(struct me_t *me1, struct me_t *me2);

/* me_related_65301.c */
int me_related_65301_011(struct me_t *me1, struct me_t *me2);
int me_related_65301_329(struct me_t *me1, struct me_t *me2);

/* me_related_65305.c */
int me_related_65305_011(struct me_t *me1, struct me_t *me2);
int me_related_65305_329(struct me_t *me1, struct me_t *me2);

/* me_related_65306.c */
int me_related_65306_011(struct me_t *me1, struct me_t *me2);
int me_related_65306_329(struct me_t *me1, struct me_t *me2);

/* me_related_65323.c */
int me_related_65323_277(struct me_t *me1, struct me_t *me2);
int me_related_65323_280(struct me_t *me1, struct me_t *me2);

/* me_related_65531.c */
int me_related_65531_310(struct me_t *me1, struct me_t *me2);

/* batchtab_cb_alu_tls.c */
int batchtab_cb_alu_tls_register(void);

/* batchtab_cb_alu_sp.c */
int batchtab_cb_alu_sp_register(void);

/* batchtab_cb_alu_ds_mcastvlan.c */
int batchtab_cb_alu_ds_mcastvlan_register(void);

/* batchtab_cb_alu_ont_mode.c */
int batchtab_cb_alu_ont_mode_register(void);

/* batchtab_cb_alu_tlsv6.c */
int batchtab_cb_alu_tlsv6_register(void);

#ifndef X86
/* er_group_hw_remote_debug_session.c */
int er_group_hw_remote_debug_session(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65296.c */
int er_group_hw_port2port_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_slid_capability(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_invert_weight_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_reset_to_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65301.c */
int er_group_hw_anti_spoof_list(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65302.c */
int 
er_group_hw_rf_filter(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65304.c */
int er_group_hw_current_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_ntp_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_47_310_65531.c */
int er_group_hw_l2s_tags_create_add01_multicast_65531(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_11_47_65297.c */
int er_group_hw_l2s_tags_create_add012_ds_supplemental_1_old(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_11_47_65305.c */
int er_group_hw_l2s_tags_create_add012_ds_supplemental_1(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_47_329_65297.c */
int er_group_hw_l2s_tags_create_add012_veip_ds_supplemental_1_old(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_47_329_65305.c */
int er_group_hw_l2s_tags_create_add012_veip_ds_supplemental_1(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65297.c */
int er_group_hw_downstream_tag_handling_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_igmp_access_control_method(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_310.c */
int er_group_hw_vid_to_ethertype(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_256.c */
int er_group_hw_change_ont_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_171.c */
int er_group_hw_bundle_rate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_133.c */
int er_group_hw_mac_limit_learning(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_277_280_402_65323.c */
int er_group_hw_mac_traffic_control_td_us_pq(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

#endif

/* proprietary_alu.c */
unsigned int proprietary_alu_get_olt_version(void);
unsigned int proprietary_alu_get_olt_release(void);
unsigned int proprietary_alu_get_ont_mode(void);
int proprietary_alu(void);

#endif
