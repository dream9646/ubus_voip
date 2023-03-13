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
 * Module  : proprietary_zte
 * File    : proprietary_zte.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_ZTE_H__
#define __PROPRIETARY_ZTE_H__

#ifndef X86
#include "er_group.h"
#endif

/* me_related_011.c */
int me_related_011_245(struct me_t *me1, struct me_t *me2);
int me_related_011_253(struct me_t *me1, struct me_t *me2);

/* me_related_047.c */
int me_related_047_65282(struct me_t *me1, struct me_t *me2);
int me_related_047_249(struct me_t *me1, struct me_t *me2);

/* me_related_134.c */
int me_related_134_250(struct me_t *me1, struct me_t *me2);
int me_related_134_65321(struct me_t *me1, struct me_t *me2);

/* me_related_245.c */
int me_related_245_011(struct me_t *me1, struct me_t *me2);

/* me_related_249.c */
int me_related_249_047(struct me_t *me1, struct me_t *me2);

/* me_related_250.c */
int me_related_250_134(struct me_t *me1, struct me_t *me2);

/* me_related_253.c */
int me_related_253_011(struct me_t *me1, struct me_t *me2);

/* me_related_65282.c */
int me_related_65282_047(struct me_t *me1, struct me_t *me2);

/* me_related_65321.c */
int me_related_65321_134(struct me_t *me1, struct me_t *me2);

#ifndef X86
/* er_group_hw_47_65282.c */
int er_group_hw_l2s_tags_create_add01_multicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_247.c */
int er_group_hw_loopdetect_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_onu_password(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_247.c */
int er_group_hw_extended_l2s_filt_configure(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_extended_l2s_filt_create_vid_tci_pri(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_11_245.c */
int er_group_hw_uni_traffic_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_134_250_65321.c */
int er_group_hw_pppoe_wan_setting(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_253.c */
int er_group_hw_broadcast_rate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_port_dhcp(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
#endif

int proprietary_zte(void);
int proprietary_zte_get_pppoe_mode(struct me_t *);

#endif
