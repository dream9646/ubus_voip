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
 * File    : meinfo_hw_anig.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <math.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "meinfo_hw_anig.h"

#include "m02098.h"
double
dbm_to_uw(double dbm)
{
	return pow(10, dbm/10)*1000;
}
double
uw_to_dbm(double uw)
{
	return 10*log10f(uw/1000.0);
}

double
uw_to_dbuw(double uw)
{
	return 10*log10f(uw);
}

// anig threshold/result from SFF-8472 compliance /////////////////////////////

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
// 0xA2, REG: 0x0~0x27
static int
meinfo_hw_anig_get_threshold_from_sff_8472(struct anig_threshold_t *anig_threshold)
{
	struct tranceiver_threshold_t tranceiver_threshold;
	unsigned short *values=(unsigned short*)&tranceiver_threshold;
	int i;
	unsigned int devaddr;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.eeprom_open==NULL || gpon_hw_g.eeprom_data_get==NULL)
		return -1;

	if (omci_env_g.anig_devaddr0)
		devaddr = omci_env_g.anig_devaddr0+1;
	else
		devaddr = BOSA_DEVICE_DIAG_REG_ADDR_DEF;

	gpon_hw_g.eeprom_open(devport, devaddr);

	// we read only 2 byte in one access for stability
	for (i=0; i<20; i++) {
		gpon_hw_g.eeprom_data_get(devport, devaddr, 0x0+i*2, &(values[i]), 2);
		// the values read from i2c bus are in big endian order
		// we convert them to host order to ease later processing
		values[i]=ntohs(values[i]);
	}

	anig_threshold->temperature_high = tranceiver_threshold.temperature_high_alarm / 256.0;
	anig_threshold->temperature_low = tranceiver_threshold.temperature_low_alarm / 256.0;
	anig_threshold->vcc_high = tranceiver_threshold.vcc_high_alarm / 10000.0;
	anig_threshold->vcc_low = tranceiver_threshold.vcc_low_alarm / 10000.0;
	anig_threshold->tx_bias_current_high = tranceiver_threshold.tx_bias_current_high_alarm / 500.0;
	anig_threshold->tx_bias_current_low = tranceiver_threshold.tx_bias_current_low_alarm / 500.0;
	anig_threshold->tx_power_high = tranceiver_threshold.tx_power_high_alarm / 10.0;
	anig_threshold->tx_power_low = tranceiver_threshold.tx_power_low_alarm / 10.0;
	anig_threshold->rx_power_high = tranceiver_threshold.rx_power_high_alarm / 10.0;
	anig_threshold->rx_power_low = tranceiver_threshold.rx_power_low_alarm / 10.0;
	
	return 0;
}

// 0xA2, REG: 0x60~0x69
static int
meinfo_hw_anig_get_result_from_sff_8472_internal(struct anig_result_t *anig_result)
{
	struct tranceiver_result_t tranceiver_result; 
	unsigned short *values=(unsigned short*)&tranceiver_result;
	int i;
	unsigned int devaddr;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.eeprom_open==NULL || gpon_hw_g.eeprom_data_get==NULL)
		return -1;

	if (omci_env_g.anig_devaddr0)
		devaddr = omci_env_g.anig_devaddr0+1;
	else
		devaddr = BOSA_DEVICE_DIAG_REG_ADDR_DEF;

	gpon_hw_g.eeprom_open(devport, devaddr);

	// we read only 2 byte in one access for stability 
	for (i=0; i<5; i++) {
		gpon_hw_g.eeprom_data_get(devport, devaddr, 0x60+i*2, &(values[i]), 2);
		values[i]=ntohs(values[i]);
	}

	anig_result->temperature = tranceiver_result.temperature / 256.0;
	anig_result->vcc = tranceiver_result.vcc / 10000.0;
	anig_result->tx_bias_current = tranceiver_result.tx_bias_current / 500.0;
	anig_result->tx_power = tranceiver_result.tx_power / 10.0;
	anig_result->rx_power = tranceiver_result.rx_power / 10.0;
	
	return 0;
}

#else
// 0xA2, REG: 0x0~0x27
static int
meinfo_hw_anig_get_threshold_from_sff_8472(struct anig_threshold_t *anig_threshold)
{
	struct tranceiver_threshold_t tranceiver_threshold;
	unsigned short *values=(unsigned short*)&tranceiver_threshold;
	int i;
	unsigned int devaddr;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	if (omci_env_g.anig_devaddr0)
		devaddr = omci_env_g.anig_devaddr0+1;
	else
		devaddr = BOSA_DEVICE_DIAG_REG_ADDR_DEF;

	gpon_hw_g.i2c_open(devport);

	// we read only 2 byte in one access for stability
	for (i=0; i<20; i++) {
		gpon_hw_g.i2c_read(devport, devaddr, 0x0+i*2, &(values[i]), 2);
		// the values read from i2c bus are in big endian order
		// we convert them to host order to ease later processing
		values[i]=ntohs(values[i]);
	}

	gpon_hw_g.i2c_close(devport);

	anig_threshold->temperature_high = tranceiver_threshold.temperature_high_alarm / 256.0;
	anig_threshold->temperature_low = tranceiver_threshold.temperature_low_alarm / 256.0;
	anig_threshold->vcc_high = tranceiver_threshold.vcc_high_alarm / 10000.0;
	anig_threshold->vcc_low = tranceiver_threshold.vcc_low_alarm / 10000.0;
	anig_threshold->tx_bias_current_high = tranceiver_threshold.tx_bias_current_high_alarm / 500.0;
	anig_threshold->tx_bias_current_low = tranceiver_threshold.tx_bias_current_low_alarm / 500.0;
	anig_threshold->tx_power_high = tranceiver_threshold.tx_power_high_alarm / 10.0;
	anig_threshold->tx_power_low = tranceiver_threshold.tx_power_low_alarm / 10.0;
	anig_threshold->rx_power_high = tranceiver_threshold.rx_power_high_alarm / 10.0;
	anig_threshold->rx_power_low = tranceiver_threshold.rx_power_low_alarm / 10.0;
	
	return 0;
}

// 0xA2, REG: 0x60~0x69
static int
meinfo_hw_anig_get_result_from_sff_8472_internal(struct anig_result_t *anig_result)
{
	struct tranceiver_result_t tranceiver_result; 
	unsigned short *values=(unsigned short*)&tranceiver_result;
	int i;
	unsigned int devaddr;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	if (omci_env_g.anig_devaddr0)
		devaddr = omci_env_g.anig_devaddr0+1;
	else
		devaddr = BOSA_DEVICE_DIAG_REG_ADDR_DEF;

	gpon_hw_g.i2c_open(devport);

	// we read only 2 byte in one access for stability 
	for (i=0; i<5; i++) {
		gpon_hw_g.i2c_read(devport, devaddr, 0x60+i*2, &(values[i]), 2);
		values[i]=ntohs(values[i]);
	}
	gpon_hw_g.i2c_close(devport);

	anig_result->temperature = tranceiver_result.temperature / 256.0;
	anig_result->vcc = tranceiver_result.vcc / 10000.0;
	anig_result->tx_bias_current = tranceiver_result.tx_bias_current / 500.0;
	anig_result->tx_power = tranceiver_result.tx_power / 10.0;
	anig_result->rx_power = tranceiver_result.rx_power / 10.0;
	
	return 0;
}
#endif

// anig threshold/result from tranceiver //////////////////////////////////////////////////////

static int
meinfo_hw_anig_get_threshold_from_tranceiver(struct anig_threshold_t *anig_threshold)
{
	struct tranceiver_threshold_t tranceiver_threshold;	
	unsigned short *values=(unsigned short*)&tranceiver_threshold;
	int i;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	gpon_hw_g.i2c_open(devport);

	// we read only 2 byte in one access for stability
	for (i=0; i<20; i++) {
		gpon_hw_g.i2c_read(devport, TRANCEIVER_SLAVE_ADDR, 0x0+i*2, &(values[i]), 2);
		// the values read from i2c bus are in big endian order
		// we convert them to host order to ease later processing
		values[i]=ntohs(values[i]);
	}
	gpon_hw_g.i2c_close(devport);

	anig_threshold->temperature_high = tranceiver_threshold.temperature_high_alarm / 256.0;
	anig_threshold->temperature_low = tranceiver_threshold.temperature_low_alarm / 256.0;	
	anig_threshold->vcc_high = tranceiver_threshold.vcc_high_alarm / 10000.0;
	anig_threshold->vcc_low = tranceiver_threshold.vcc_low_alarm / 10000.0;		
	anig_threshold->tx_bias_current_high = tranceiver_threshold.tx_bias_current_high_alarm / 500.0;
	anig_threshold->tx_bias_current_low = tranceiver_threshold.tx_bias_current_low_alarm / 500.0;	
	anig_threshold->tx_power_high = tranceiver_threshold.tx_power_high_alarm / 10.0;
	anig_threshold->tx_power_low = tranceiver_threshold.tx_power_low_alarm / 10.0;	
	anig_threshold->rx_power_high = tranceiver_threshold.rx_power_high_alarm / 10.0;
	anig_threshold->rx_power_low = tranceiver_threshold.rx_power_low_alarm / 10.0;
	
	return 0;
}

static int
meinfo_hw_anig_get_result_from_tranceiver(struct anig_result_t *anig_result)
{
	struct tranceiver_result_t tranceiver_result; 
	unsigned short *values=(unsigned short*)&tranceiver_result;
	int i;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	gpon_hw_g.i2c_open(devport);

	// we read only 2 byte in one access for stability 
	for (i=0; i<5; i++) {
		gpon_hw_g.i2c_read(devport, TRANCEIVER_SLAVE_ADDR, 0x60+i*2, &(values[i]), 2);
		values[i]=ntohs(values[i]);	
	}
	gpon_hw_g.i2c_close(devport);

	anig_result->temperature = tranceiver_result.temperature / 256.0;
	anig_result->vcc = tranceiver_result.vcc / 10000.0;
	anig_result->tx_bias_current = tranceiver_result.tx_bias_current / 500.0;
	anig_result->tx_power = tranceiver_result.tx_power / 10.0;
	anig_result->rx_power = tranceiver_result.rx_power / 10.0;
	
	return 0;
}

// anig threshold from static //////////////////////////////////////////////////////

static int
meinfo_hw_anig_get_threshold_static(struct anig_threshold_t *anig_threshold)
{
	// these values are referenced from APAC Opto transceiver (LSF2-C3M-TC-N3-G4) 
	anig_threshold->temperature_high = 85.0; 	// C degree
	anig_threshold->temperature_low = -14.9961;	// C degree	 	
	anig_threshold->vcc_high = 3.8; 		// v (but 3.6 max in datasheet)
	anig_threshold->vcc_low = 2.8;			// v		
	anig_threshold->tx_bias_current_high = 80.0;	// ma
	anig_threshold->tx_bias_current_low = 0.1;	// ma			

	// the tx/rx threshold are based on the measured values from tranceiver
	anig_threshold->tx_power_high = dbm_to_uw(omci_env_g.anig_txrx_threshold[0]);
	anig_threshold->tx_power_low = dbm_to_uw(omci_env_g.anig_txrx_threshold[1]);
	anig_threshold->rx_power_high = dbm_to_uw(omci_env_g.anig_txrx_threshold[2]);
	anig_threshold->rx_power_low = dbm_to_uw(omci_env_g.anig_txrx_threshold[3]);
	
	return 0;
}

// anig result from mindspeed bosa //////////////////////////////////////////////////
static int
meinfo_hw_anig_get_result_from_bosa_mindspeed(struct anig_result_t *anig_result, struct anig_raw_t *anig_raw)
{
	unsigned char value1, value2;
	unsigned int value;
	//unsigned int tmp1, tmp2;
	int devport=omci_env_g.anig_devport;

	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	gpon_hw_g.i2c_open(devport);
	//util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "--------------------------\n");	

	// Temperature
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x58, &value1, 1);
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x57, &value2, 1);
	value=value1; value=(value<<2)|(value2&0x3);
	anig_raw->temperature= value;
	anig_result->temperature= value * BOSA_MINDSPEED_TEMP_SLOPE + BOSA_MINDSPEED_TEMP_OFFSET;
	//util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "Tem[%6d]:%f, %f\n", value, OMCI_BOSA_Get_Temperature(), anig_result->temperature);
	
	// Vcc
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x52, &value1, 1);
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x51, &value2, 1);
	value=value1; value=(value<<4)|(value2&0xf);
	anig_raw->vcc= value;
	anig_result->vcc= value * BOSA_MINDSPEED_VCC_SLOPE + BOSA_MINDSPEED_VCC_OFFSET;	
	//util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "Vcc[%6d]:%f, %f\n", value, OMCI_BOSA_Get_Vcc(),anig_result->vcc);

	// Tx Bias Current
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x59, &value1, 1);
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x5a, &value2, 1);
	value=value1&0xf; value=(value<<8)|value2;
	anig_raw->tx_bias_current = value;
	anig_result->tx_bias_current = value * BOSA_MINDSPEED_BIAS_SLOPE + BOSA_MINDSPEED_BIAS_OFFSET;
	//util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "Ibi[%6d]:%f, %f\n", value, OMCI_BOSA_Get_Biascurrent(),anig_result->tx_bias_current);

	// Tx Power
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x56, &value1, 1);
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x57, &value2, 1);
	value=value1; value=(value<<4)|(value2>>4);
	anig_raw->tx_power= value;
	anig_result->tx_power= value * BOSA_MINDSPEED_TXPWR_SLOPE + BOSA_MINDSPEED_TXPWR_OFFSET;
	//util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "Ptx[%6d]:%f, %f\n", value, OMCI_BOSA_Get_Txpower(),anig_result->tx_power);	

	// Rx Power (pin 28)
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x50, &value1, 1);
	gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x51, &value2, 1);
	value=value1; value=(value<<4)|(value2>>4);
	anig_raw->rx_power= value;
	anig_result->rx_power= value * BOSA_MINDSPEED_RXPWR_SLOPE + BOSA_MINDSPEED_RXPWR_OFFSET;
#if 0
	// Rx Power (pin 29)
	gpon_hw_g.i2c_read(0, BOSA_MINDSPEED_SLAVE_ADDR, 0x53, &value1, 1);
	gpon_hw_g.i2c_read(0, BOSA_MINDSPEED_SLAVE_ADDR, 0x54, &value2, 1);
	tmp1=value1; tmp1=(tmp1<<4)|(value2>>4);

	// Rx Power (internal)
	gpon_hw_g.i2c_read(0, BOSA_MINDSPEED_SLAVE_ADDR, 0x55, &value1, 1);
	gpon_hw_g.i2c_read(0, BOSA_MINDSPEED_SLAVE_ADDR, 0x54, &value2, 1);
	tmp2=value1; tmp2=(tmp2<<4)|(value2&0xf);

	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "Prx[%6d]:%f, %f <p29:%d, int:%d>\n", value, OMCI_BOSA_Get_Rxpower(),anig_result->rx_power, tmp1, tmp2);
#endif
	gpon_hw_g.i2c_close(devport);

	return 0;
}


// anig result from mindspeed bosa library///////////////////////////////////////////

// as mindspeed init/destory may crash because of reentry (eg: anig alarm and anig test simtaneously)
// so we only call mindspeed init for once and never call mindspeed destroy 
static int
meinfo_hw_anig_get_result_from_bosa_mindspeed_lib(struct anig_result_t *anig_result)
{
#if 0
	static int is_mindspeed_inited=0;
	int fd;
	
	if (!is_mindspeed_inited) {
		if ((fd=open(MINDSPEED_DAT_FILE, O_RDONLY))<0) {
			util_run_by_system(MINDSPEED_DAT_GEN_SCRIPT);	// generate dat file from uboot env
		} else {
			close(fd);
		}
		if ( MinspeedInit(MINDSPEED_DAT_FILE) <0) {
			util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "MinspeedInit %s error\n", MINDSPEED_DAT_FILE);
			return -1;
		}
		is_mindspeed_inited=1;
	}

	anig_result->vcc = OMCI_BOSA_Get_Vcc();
	anig_result->temperature = OMCI_BOSA_Get_Temperature();
	anig_result->tx_bias_current = OMCI_BOSA_Get_Biascurrent();
	anig_result->tx_power = OMCI_BOSA_Get_Txpower();	
	anig_result->rx_power = OMCI_BOSA_Get_Rxpower();

#if 0
	if (MinspeedDestroy() <0) {
		util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "MindspeedDestroy error\n");
		return -1;	
	}
#endif
	return 0;
#else
	// FIXME, KEVIN, libm2098 is not available anymore, we use meinfo_hw_anig_get_result_from_bosa_mindspeed instead
	struct anig_raw_t anig_raw;
	return meinfo_hw_anig_get_result_from_bosa_mindspeed(anig_result, &anig_raw);
#endif
}

// anig result from bosa report binary ///////////////////////////////////////////
#define BOSA_REPORT_CMD_GEN	"/etc/init.d/bosa.sh report_cmd"

static int
meinfo_hw_anig_get_result_from_bosa_report(struct anig_result_t *anig_result)
{
	static char report_cmd[128]={0};
	char *output=NULL;
	int ret;
	
	static struct anig_result_t anig_result_prev={0};
	static int anig_result_prev_time = 0;
	int now = time(0);

	// as calling /etc/init.d/bosa.sh is slow
	// if request is within same second, reply with previous result directly
	if (now == anig_result_prev_time) {
		*anig_result = anig_result_prev;
		return 0;
	}	

	memset((void*)anig_result, 0, sizeof(struct anig_result_t));	

	if (strlen(report_cmd)==0) {
		char *s;
		if (util_readcmd(BOSA_REPORT_CMD_GEN, &output) <0 || output==NULL) {
			if (output)
				free_safe(output);
			dbprintf(LOG_ERR, "cmd %s error\n", BOSA_REPORT_CMD_GEN);
			return -1;
		}
		s=util_trim(output);
		strncpy(report_cmd, s, 128);
		free_safe(output);
	}

	if (util_readcmd(report_cmd, &output) <0 || output==NULL) {
		if (output)
			free_safe(output);
		dbprintf(LOG_ERR, "cmd %s error\n", report_cmd);
		return -2;
	}

	ret = sscanf(output, "%lf %lf %lf %lf %lf",
		&anig_result->temperature,
		&anig_result->vcc,
		&anig_result->tx_bias_current,
		&anig_result->tx_power,
		&anig_result->rx_power);
	
	if (ret<5) {
		dbprintf(LOG_ERR, "output %s error\n", output);
		free_safe(output);
		return -3;
	}

	// cache result for repeated access with same second
	anig_result_prev = *anig_result;
	anig_result_prev_time = time(0);	

	free_safe(output);
	return 0;
}

// get function wrapper //////////////////////////////////////////////////

int
meinfo_hw_anig_get_threshold_result(int anig_type, struct anig_threshold_t *t, struct anig_result_t *r, struct anig_raw_t *raw)
{
	int ret=0;

	// init threshold/result to 0
	memset(t, 0, sizeof(struct anig_threshold_t));
	memset(r, 0, sizeof(struct anig_result_t));
	memset(raw, 0, sizeof(struct anig_raw_t));

	util_create_lockfile("/var/run/anig.lock", 30);
	
	if (anig_type==ENV_ANIG_TYPE_TRANCEIVER) {
		if (meinfo_hw_anig_get_result_from_tranceiver(r)<0)
			ret = -1;
		if (meinfo_hw_anig_get_threshold_from_tranceiver(t)<0)
			ret = -2;
	} else if (anig_type==ENV_ANIG_TYPE_BOSA_MINDSPEED) {
		if (meinfo_hw_anig_get_result_from_bosa_mindspeed(r, raw)<0)
			ret = -1;
		if (meinfo_hw_anig_get_threshold_static(t)<0)
			ret = -2;
	} else if (anig_type==ENV_ANIG_TYPE_BOSA_MINDSPEED_LIB) {
		if (meinfo_hw_anig_get_result_from_bosa_mindspeed_lib(r)<0)
			ret = -1;
		if (meinfo_hw_anig_get_threshold_static(t)<0)
			ret = -2;
	} else if (anig_type==ENV_ANIG_TYPE_BOSA_REPORT) {
		if (meinfo_hw_anig_get_result_from_bosa_report(r)<0)
			ret = -1;
		if (meinfo_hw_anig_get_threshold_static(t)<0)
			ret = -2;
	} else if (anig_type==ENV_ANIG_TYPE_SFF8472) {
		if (omci_env_g.anig_mon_mode==ENV_ANIG_MON_MODE_CAL_INT) {
			if (meinfo_hw_anig_get_result_from_sff_8472_internal(r)<0)
				ret = -1;
			if (meinfo_hw_anig_get_threshold_from_sff_8472(t)<0)
				ret = -2;
		} else {
			// External calibration is not support yet.
			ret = -3;
		}
	} else {
		ret = -3;
	}

	util_release_lockfile("/var/run/anig.lock");

	if (ret <0)
		return ret;

	if (r->rx_power<0)
		r->rx_power=0;
	if (r->tx_power<0)
		r->tx_power=0;

	return 0;			
}

// print function /////////////////////////////////////////////////////////////////////////////

int
meinfo_hw_anig_print_threshold_result(int fd, int anig_type, struct anig_threshold_t *t, struct anig_result_t *r, struct anig_raw_t *raw)
{
	if (anig_type==ENV_ANIG_TYPE_BOSA_MINDSPEED) {
		util_fdprintf(fd, "\n");
		util_fdprintf(fd, "degree(%u)\t%.3f C  \t[%.1f ~ %.1f C]\n", raw->temperature, r->temperature, t->temperature_low, t->temperature_high);
		util_fdprintf(fd, "vcc(%u)   \t%.3f V  \t[%.1f ~ %.1f V]\n", raw->vcc, r->vcc, t->vcc_low, t->vcc_high);
		util_fdprintf(fd, "bias(%u)  \t%.3f mA \t[%.1f ~ %.1f mA]\n", raw->tx_bias_current, r->tx_bias_current, t->tx_bias_current_low, t->tx_bias_current_high);
		util_fdprintf(fd, "tx(%u)    \t%.3f uW(%.3f dBm)\t[%.1f uW (%.3f dBm) ~ %.1f uW (%.3f dBm)]\n", 
			raw->tx_power, 
			r->tx_power,      uw_to_dbm(r->tx_power), 
			t->tx_power_low,  uw_to_dbm(t->tx_power_low), 
			t->tx_power_high, uw_to_dbm(t->tx_power_high));
		util_fdprintf(fd, "rx(%u)    \t%.3f uW(%.3f dBm)\t[%.1f uW (%.3f dBm) ~ %.1f uW (%.3f dBm)]\n", 
			raw->rx_power, 
			r->rx_power,      uw_to_dbm(r->rx_power), 
			t->rx_power_low,  uw_to_dbm(t->rx_power_low), 
			t->rx_power_high, uw_to_dbm(t->rx_power_high) );
		util_fdprintf(fd, "\n");
	} else {
		util_fdprintf(fd, "\n");
		util_fdprintf(fd, "degree\t%.3f C  \t[%.1f ~ %.1f C]\n", r->temperature, t->temperature_low, t->temperature_high);
		util_fdprintf(fd, "vcc   \t%.3f V  \t[%.1f ~ %.1f V]\n", r->vcc, t->vcc_low, t->vcc_high);
		util_fdprintf(fd, "bias  \t%.3f mA \t[%.1f ~ %.1f mA]\n", r->tx_bias_current, t->tx_bias_current_low, t->tx_bias_current_high);
		util_fdprintf(fd, "tx    \t%.3f uW(%.3f dBm)\t[%.1f uW (%.3f dBm) ~ %.1f uW (%.3f dBm)]\n", 
			r->tx_power,      uw_to_dbm(r->tx_power), 
			t->tx_power_low,  uw_to_dbm(t->tx_power_low), 
			t->tx_power_high, uw_to_dbm(t->tx_power_high) );
		util_fdprintf(fd, "rx    \t%.3f uW(%.3f dBm)\t[%.1f uW (%.3f dBm) ~ %.1f uW (%.3f dBm)]\n", 
			r->rx_power,      uw_to_dbm(r->rx_power), 
			t->rx_power_low,  uw_to_dbm(t->rx_power_low), 
			t->rx_power_high, uw_to_dbm(t->rx_power_high));
		util_fdprintf(fd, "\n");
	}
	return 0;
}

int 
meinfo_hw_anig_detect_type(void)
{
	int dmesg_level = util_get_dmesg_level();
	struct anig_result_t anig_result;
	unsigned short value;
	int i;
	int devport=omci_env_g.anig_devport;

	util_set_dmesg_level(1);

	// try tranceiver first
	if (gpon_hw_g.i2c_open==NULL || gpon_hw_g.i2c_read==NULL || gpon_hw_g.i2c_close==NULL)
		return -1;

	gpon_hw_g.i2c_open(devport);
	for (i=0; i<3; i++) {
		if (gpon_hw_g.i2c_read(devport, TRANCEIVER_SLAVE_ADDR, 0x0+i*2, &value, 2)>0) {
			util_set_dmesg_level(dmesg_level);
			gpon_hw_g.i2c_close(devport);
			return ENV_ANIG_TYPE_TRANCEIVER;
		}
	}
	gpon_hw_g.i2c_close(devport);

	// try bosa report 2nd
	if (meinfo_hw_anig_get_result_from_bosa_report(&anig_result)==0) {
		util_set_dmesg_level(dmesg_level);
		return ENV_ANIG_TYPE_BOSA_REPORT;
	}

	// try bosa mindspeed 3rd
	gpon_hw_g.i2c_open(devport);
	for (i=0; i<3; i++) {
		if (gpon_hw_g.i2c_read(devport, BOSA_MINDSPEED_SLAVE_ADDR, 0x57+i, &value, 1)>0) {
			util_set_dmesg_level(dmesg_level);
			gpon_hw_g.i2c_close(devport);
			//return ENV_ANIG_TYPE_BOSA_MINDSPEED;
			return ENV_ANIG_TYPE_BOSA_MINDSPEED_LIB;
		}
	}
	gpon_hw_g.i2c_close(devport);

	// try sff-8472 specific device address last
	if (omci_env_g.anig_devaddr0) {
		if (omci_env_g.anig_mon_mode==ENV_ANIG_MON_MODE_CAL_INT) {
			if (meinfo_hw_anig_get_result_from_sff_8472_internal(&anig_result)==0) {
				util_set_dmesg_level(dmesg_level);
				return ENV_ANIG_TYPE_SFF8472;
			}
		}
	}

	util_set_dmesg_level(dmesg_level);
	return ENV_ANIG_TYPE_NONE;
}
