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
 * File    : meinfo_hw_anig.h
 *
 ******************************************************************/

#ifndef __MEINFO_HW_UTIL_ANIG_H__
#define __MEINFO_HW_UTIL_ANIG_H__

#define TRANCEIVER_SLAVE_ADDR0	0x50	// for identidifer
#define TRANCEIVER_SLAVE_ADDR	0x51	// for diagnostic

#define BOSA_MINDSPEED_SLAVE_ADDR	0x4e	// mind speed m02098

#define BOSA_DEVICE_SFP_REG_ADDR_DEF	TRANCEIVER_SLAVE_ADDR0
#define BOSA_DEVICE_DIAG_REG_ADDR_DEF	TRANCEIVER_SLAVE_ADDR

#define BOSA_DEVICE_SFP_PORT_DEF	0

// 1. rx equation: register_reading_value = rx_power /slope + offset
//    register_reading_value = offset, when rx input power = 0
//    register_reading_value = 50uw / slope + offset
// 2. tx equation: register_reading_value = tx_power /slope
//    register_reading_value = 1000uw / slope 

// the following value nees to be derive from real calibration
#if 0
#define BOSA_MINDSPEED_RX_OFFSET	7
#define BOSA_MINDSPEED_RX_SLOPE		((float)50/(711-BOSA_MINDSPEED_RX_OFFSET))
#define BOSA_MINDSPEED_TX_SLOPE		((float)1000/22247)
//#else // Fine tune the Mindspeed BoSA parameters
#define BOSA_MINDSPEED_RX_OFFSET	0
#define BOSA_MINDSPEED_RX_SLOPE		0.086559257
#define BOSA_MINDSPEED_TX_SLOPE		2.1551724137931034482758620689655
#endif

// Factors for linear equations (Val_Reading = Val_Eeprom * SLOPE + OFFSET)
#define BOSA_MINDSPEED_TEMP_SLOPE	0.247
#define BOSA_MINDSPEED_TEMP_OFFSET	-77.9
#define BOSA_MINDSPEED_VCC_SLOPE	0.0014
#define BOSA_MINDSPEED_VCC_OFFSET	0.0
#define BOSA_MINDSPEED_BIAS_SLOPE	0.0163
#define BOSA_MINDSPEED_BIAS_OFFSET	0.0
#define BOSA_MINDSPEED_TXPWR_SLOPE	4.6284
#define BOSA_MINDSPEED_TXPWR_OFFSET	-1280.63208
#define BOSA_MINDSPEED_RXPWR_SLOPE	0.0478
#define BOSA_MINDSPEED_RXPWR_OFFSET	-0.006

#define MINDSPEED_DAT_FILE		"/tmp/m02098config.dat"
#define MINDSPEED_DAT_GEN_SCRIPT	"/etc/init.d/env2dat.sh"

// tranceiver eeprom: slaveaddr 0x51, regaddr 0x0..0x27, value in little endian
struct anig_threshold_t {
	double temperature_high;	// c degree
	double temperature_low;		// c degree
	double vcc_high;		// v
	double vcc_low;			// v
	double tx_bias_current_high;	// ma
	double tx_bias_current_low;	// ma
	double tx_power_high;		// uw
	double tx_power_low;		// uw
	double rx_power_high;		// uw
	double rx_power_low;		// uw	
};

struct anig_raw_t {
	unsigned int temperature;
	unsigned int vcc;	
	unsigned int tx_bias_current;
	unsigned int tx_power;
	unsigned int rx_power;
};

struct anig_result_t {
	double temperature;		// c degree
	double vcc;			// v
	double tx_bias_current;		// ma
	double tx_power;		// uw
	double rx_power;		// uw
};


struct tranceiver_threshold_t {
	short temperature_high_alarm;
	short temperature_low_alarm;
	short temperature_high_warn;
	short temperature_low_warn;
	unsigned short vcc_high_alarm;
	unsigned short vcc_low_alarm;
	unsigned short vcc_high_warn;
	unsigned short vcc_low_warn;
	unsigned short tx_bias_current_high_alarm;
	unsigned short tx_bias_current_low_alarm;
	unsigned short tx_bias_current_high_warn;
	unsigned short tx_bias_current_low_warn;
	unsigned short tx_power_high_alarm;
	unsigned short tx_power_low_alarm;
	unsigned short tx_power_high_warn;
	unsigned short tx_power_low_warn;
	unsigned short rx_power_high_alarm;
	unsigned short rx_power_low_alarm;
	unsigned short rx_power_high_warn;
	unsigned short rx_power_low_warn;
};

// tranceiver eeprom: slaveaddr 0x51, regaddr 0x60..0x69, value in little endian
struct tranceiver_result_t {
	unsigned short temperature;	// -128..127 in lowbyte
	unsigned short vcc;		// 0..65535 *100uv
	unsigned short tx_bias_current;	// 0..65535 *2ua
	unsigned short tx_power;	// 0..65535 *0.1uw (-40..8.2dbm)
	unsigned short rx_power;	// 0..65535 *0.1uw (-40..8.2dbm)
};

/* meinfo_hw_anig.c */
double dbm_to_uw(double dbm);
double uw_to_dbm(double uw);
double uw_to_dbuw(double uw);
int meinfo_hw_anig_get_threshold_result(int anig_type, struct anig_threshold_t *t, struct anig_result_t *r, struct anig_raw_t *raw);
int meinfo_hw_anig_print_threshold_result(int fd, int anig_type, struct anig_threshold_t *t, struct anig_result_t *r, struct anig_raw_t *raw);
int meinfo_hw_anig_detect_type(void);

#endif
