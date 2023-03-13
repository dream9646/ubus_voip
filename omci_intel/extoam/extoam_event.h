/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF EXTENDED OAM
 * Module  : extoam
 * File    : extoam_event.c
 *
 ******************************************************************/

#define EXTOAM_EVENT_PORT_0_LINK_DOWN	0x10
#define EXTOAM_EVENT_PORT_0_LINK_UP	0x11
#define EXTOAM_EVENT_PORT_1_LINK_DOWN	0x20
#define EXTOAM_EVENT_PORT_1_LINK_UP	0x21
#define EXTOAM_EVENT_PORT_2_LINK_DOWN	0x30
#define EXTOAM_EVENT_PORT_2_LINK_UP	0x31
#define EXTOAM_EVENT_PORT_3_LINK_DOWN	0x40
#define EXTOAM_EVENT_PORT_3_LINK_UP	0x41
#define EXTOAM_EVENT_PORT_4_LINK_DOWN	0x50
#define EXTOAM_EVENT_PORT_4_LINK_UP	0x51
#define EXTOAM_EVENT_GPON_LINK_DOWN	0x60
#define EXTOAM_EVENT_GPON_LINK_UP	0x61

#define EXTOAM_EVENT_TEMPERATURE_HIGH_CLEAR	0x100
#define EXTOAM_EVENT_TEMPERATURE_HIGH_WARN	0x101
#define EXTOAM_EVENT_TEMPERATURE_HIGH_ALARM	0x102
#define EXTOAM_EVENT_TEMPERATURE_LOW_CLEAR	0x110
#define EXTOAM_EVENT_TEMPERATURE_LOW_WARN	0x111
#define EXTOAM_EVENT_TEMPERATURE_LOW_ALARM	0x112
#define EXTOAM_EVENT_VOLTAGE_HIGH_CLEAR		0x120
#define EXTOAM_EVENT_VOLTAGE_HIGH_WARN		0x121
#define EXTOAM_EVENT_VOLTAGE_HIGH_ALARM		0x122
#define EXTOAM_EVENT_VOLTAGE_LOW_CLEAR		0x130
#define EXTOAM_EVENT_VOLTAGE_LOW_WARN		0x131
#define EXTOAM_EVENT_VOLTAGE_LOW_ALARM		0x132
#define EXTOAM_EVENT_TXPOWER_HIGH_CLEAR		0x140
#define EXTOAM_EVENT_TXPOWER_HIGH_WARN		0x141
#define EXTOAM_EVENT_TXPOWER_HIGH_ALARM		0x142
#define EXTOAM_EVENT_TXPOWER_LOW_CLEAR		0x150
#define EXTOAM_EVENT_TXPOWER_LOW_WARN		0x151
#define EXTOAM_EVENT_TXPOWER_LOW_ALARM		0x152
#define EXTOAM_EVENT_RXPOWER_HIGH_CLEAR		0x160
#define EXTOAM_EVENT_RXPOWER_HIGH_WARN		0x161
#define EXTOAM_EVENT_RXPOWER_HIGH_ALARM		0x162
#define EXTOAM_EVENT_RXPOWER_LOW_CLEAR		0x170
#define EXTOAM_EVENT_RXPOWER_LOW_WARN		0x171
#define EXTOAM_EVENT_RXPOWER_LOW_ALARM		0x172
#define EXTOAM_EVENT_BIASCURRENT_HIGH_CLEAR	0x180
#define EXTOAM_EVENT_BIASCURRENT_HIGH_WARN	0x181
#define EXTOAM_EVENT_BIASCURRENT_HIGH_ALARM	0x182
#define EXTOAM_EVENT_BIASCURRENT_LOW_CLEAR	0x190
#define EXTOAM_EVENT_BIASCURRENT_LOW_WARN	0x191
#define EXTOAM_EVENT_BIASCURRENT_LOW_ALARM	0x192

#define EXTOAM_EVENT_SF_CLEAR	0x200
#define EXTOAM_EVENT_SF_ALARM	0x201
#define EXTOAM_EVENT_SD_CLEAR	0x210
#define EXTOAM_EVENT_SD_ALARM	0x211

int extoam_event_send( struct extoam_link_status_t *cpe_link_status, unsigned short event_id );

