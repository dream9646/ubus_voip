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
#ifndef __INTEL_PX126_PON_H__
#define __INTEL_PX126_PON_H__

#include "pon_lib/fapi_pon.h"

#define PON_ALARM_LOS PON_ALARM_STATIC_LOS
#define PON_ALARM_LOF PON_ALARM_STATIC_LOF
#define PON_ALARM_LOL PON_ALARM_STATIC_LOL

int intel_px126_pon_get_state(int *state);
int intel_prx126_pon_serial_number_get(char *vendor_id, unsigned int *serial_number);
int intel_prx126_pon_password_get(char *password);
int intel_prx126_pon_active(void);
int intel_prx126_pon_deactive(void);
int intel_prx126_pon_eeprom_data_get(const int ddmi_page, unsigned char * data, long offset, size_t data_size);
int intel_prx126_pon_eeprom_data_set(const int ddmi_page, unsigned char * data, long offset, size_t data_size);
int intel_prx126_pon_eeprom_open(const int ddmi_page, const char *filename);
int intel_prx126_gpon_alarm_status_get(unsigned int alarm_type, unsigned int *status);
int intel_prx126_gpon_tod_sync_mf_get(unsigned int *mf_cnt);

#endif
