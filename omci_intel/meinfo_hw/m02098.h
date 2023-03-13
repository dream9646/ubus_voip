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
 * File    : m02098.h
 *
 ******************************************************************/

/***
******************************************************************
** 								
** Project : GRT5-103						
** FileName: m02098.h						
** Version: 1.0							
** Date: Mar 22, 2011						
** 								
******************************************************************
** 								
** Copyright (c)2011 Browan Communications Inc.			
** ALL right reserved.						
** 								
******************************************************************
**Version History:
**------------
**Version: 1.0
**Date: Mar 22, 2011
**Revised By: Johnson.He
**Description: Origianl Version
******************************************************************
***/
#ifndef M02098_H_
#define M02098_H_
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*Config file elem 's struct*/
typedef struct {
	int TransceiveReg;
	/* Calibration Slope and Offset */
	float InitModCurrent;
	float InitBiasCurrent;
	float InitDAC;

	float VccSlope;
	float VccOffset;
	float TempSlope;
	float TempOffset;
	float ModSlope;
	float ModOffset;
	float BiasSlope;
	float BiasOffset;
	float TxPowerSlope;
	float TxPowerOffset;

	float RxPowerOffset0;
	float RxPowerOffset1;
	float RxPowerOffset2;
	float RxPowerOffset3;
	float RxPowerOffset4;

	float RxPowerSlope1;
	float RxPowerSlope2;
	float RxPowerSlope3;
	float RxPowerSlope4;

	float LowReceivedPower;
	float HighReceivedPower;
	float LowTransmitPower;
	float HighTransmitPower;
	float LaserBiasCurrent;
	float LaserSupplyVoltage;
	float LaserTemperature;
	int massproduct;	//0505
}Calibration;

/****************************************************************
 Function: MinspeedInitChip
 Description: init the Minspeed Chip
 Parameter:
			none
 Return :	
 			success return 0
 			fail return -1
*****************************************************************/
extern int MinspeedInitChip();

/****************************************************************
 Function: Minspeedbackupreg
 Description: set the Minspeed Chip
 Parameter:
			none
 Return :	
 			success return 0
 			fail return -1
*****************************************************************/
extern int Minspeedbackupreg();

/****************************************************************
 Function: MinspeedInit
 Description: init the config file to the link list
 Parameter:
			char *configFile: the config file
 Return :	
 			success return 0
 			fail return -1
*****************************************************************/
extern int MinspeedInit(char *configFile);

/****************************************************************
 Function: MinspeedDestroy
 Description: destroy the config file list
 Parameter:
			none
 Return :	
 			success return 0
 			fail return -1
*****************************************************************/
extern int MinspeedDestroy();

/****************************************************************
 Function: MinspeedGetVccddm
 Description:get the minspeed voltage ddmi
 Parameter:
			void
 Return :	
 			success return voltage ddmi value
*****************************************************************/
extern int MinspeedGetVccddm(void);

/****************************************************************
 Function: MinspeedGetTempddm
 Description:get the minspeed temperature ddmi
 Parameter:
			void
 Return :	
 			success return temperature ddmi value
*****************************************************************/
extern int MinspeedGetTempddm(void);

/****************************************************************
 Function: MinspeedGetBiasddm
 Description:get the minspeed biascurrent ddmi
 Parameter:
			void
 Return :	
 			success return biascurrent ddmi value
*****************************************************************/
extern int MinspeedGetBiasddm(void);

/****************************************************************
 Function: MinspeedGetModddm
 Description:get the minspeed modcurrent ddmi
 Parameter:
			void
 Return :	
 			success return modcurrent ddmi value
*****************************************************************/
extern int MinspeedGetModddm (void);

/****************************************************************
 Function: MinspeedGetRxPowerddm
 Description:get the minspeed RxPower ddmi
 Parameter:
			void
 Return :	
 			success return RxPower ddmi value
*****************************************************************/
extern int MinspeedGetRxPowerddm(void);

/****************************************************************
 Function: MinspeedGetTxPowerddm
 Description:get the minspeed TxPower ddmi
 Parameter:
			void
 Return :	
 			success return TxPower ddmi value
*****************************************************************/
extern int MinspeedGetTxPowerddm(void);

/****************************************************************
 Function: MinspeedGetData
 Description: get the minspeed Parameter value
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedGetData(char *str, char* msg);

/****************************************************************
 Function: MinspeedSetData
 Description: set the minspeed Parameter value
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedSetData(char *str, char* msg);

/****************************************************************
 Function: MinspeedSaveEEPROMInitData
 Description: Save Init Data to EEPROM
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedSaveEEPROMInitData(char* str, char* msg);

/****************************************************************
 Function: MinspeedCheckPRBS
 Description: prbs checker
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedCheckPRBS(char* str, char* msg);

/****************************************************************
 Function: MinspeedSetCalibrationValue
 Description: set minspeed calibration value
 Parameter:
			char *recvKey: the key string
			char *recvValue:the value
 Return :	
 			get success return 0
******************************************************************/
extern void MinspeedSetCalibrationValue(char *recvKey, char *recvValue);

/****************************************************************
 Function: MinspeedDumpEEPROM
 Description: Dump all EEPROM value
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedDumpEEPROM(char *str, char* msg);

/****************************************************************
 Function: MinspeedBackupReg
 Description: Backup register values to EEPROM
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedBackupReg(char *str, char* msg);

/****************************************************************
 Function: MinspeedGetMAC
 Description:get the board mac address
 Parameter:
			none
 Return :	
 			success return 0
 			fail return -1
*****************************************************************/
extern int MinspeedGetMAC();

/****************************************************************
 Function: MinspeedPrbsEnableTX
 Description:Enable Prbs TX
 Parameter:
			none
 Return :	
 			void
*****************************************************************/
extern void MinspeedPrbsEnableTX();

/****************************************************************
 Function: MinspeedPrbsDisableTX
 Description:Disable Prbs TX
 Parameter:
			none
 Return :	
 			void
*****************************************************************/
extern void MinspeedPrbsDisableTX();

/****************************************************************
 Function: MinspeedPrbsRX
 Description: prbs checker
 Parameter:
			none
 Return :	
 			void
*****************************************************************/
extern int MinspeedPrbsRX();

/****************************************************************
 Function: MinspeedRead
 Description:read minspeed register
 Parameter:
			int reg_addr: register address
 Return :	
 			success return register value
*****************************************************************/
extern char MinspeedRead(int reg_addr);

/****************************************************************
 Function: MinspeedWrite
 Description:write minspeed register
 Parameter:
			int reg_addr: register address
			int val:write value
 Return :	
 			void
*****************************************************************/
extern void MinspeedWrite(int reg_addr,int val);

/****************************************************************
 Function: OMCI_BOSA_Get_Vcc
 Description:get the minspeed voltage value
 Parameter:
			void
 Return :	
 			success return voltage  value
*****************************************************************/
extern float OMCI_BOSA_Get_Vcc();

/****************************************************************
 Function: OMCI_BOSA_Get_Temperature
 Description:get the minspeed temperature value
 Parameter:
			void
 Return :	
 			success return temperature  value
*****************************************************************/
extern float OMCI_BOSA_Get_Temperature();

/****************************************************************
 Function: OMCI_BOSA_Get_Biascurrent
 Description:get the minspeed biascurrent value
 Parameter:
			void
 Return :	
 			success return biascurrent value
*****************************************************************/
extern float OMCI_BOSA_Get_Biascurrent();

/****************************************************************
 Function: OMCI_BOSA_Get_Modcurrent
 Description:get the minspeed modcurrent value
 Parameter:
			void
 Return :	
 			success return modcurrent value
*****************************************************************/
extern float OMCI_BOSA_Get_Modcurrent();

/****************************************************************
 Function: OMCI_BOSA_Get_Txpower
 Description:get the minspeed TxPower value
 Parameter:
			void
 Return :	
 			success return TxPower value
*****************************************************************/
extern float OMCI_BOSA_Get_Txpower();

/****************************************************************
 Function: OMCI_BOSA_Get_Rxpower
 Description:get the minspeed RxPower value
 Parameter:
			void
 Return :	
 			success return RxPower value
*****************************************************************/
extern float OMCI_BOSA_Get_Rxpower();

/****************************************************************
 Function: OMCI_BOSA_Get_Alarm
 Description:get the minspeed alarm status
 Parameter:
			void
 Return :	
 			success return alarm status value,binary 0 nomal, 1 abnormity
 			bit0: LowReceivedPower status
 			bit1: HighReceivedPower status
 			bit2: LowTransmitPower status
 			bit3: HighTransmitPower status
 			bit4: LaserBiasCurrent status
 			bit5: LaserTemperature status
 			bit6: LaserSupplyVoltage status
*****************************************************************/
extern char OMCI_BOSA_Get_Alarm();

/****************************************************************
 Function: MinspeedRWMAC
 Description: Read and Write MAC address
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedRWMAC(char *str,  char *msg);

/****************************************************************
 Function: MinspeedRWSN
 Description: Read and Write Serial Number
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedRWSN(char *str, char* msg);

/****************************************************************
 Function: MinspeedGetVersion
 Description: get firmware version
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedGetVersion(char *str, char* msg);

/****************************************************************
 Function: MinspeedRestoreDefault
 Description: restore default
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedRestoreDefault(char *str, char* msg);

/****************************************************************
 Function: MinspeedSetEnv
 Description: Set Env
 Parameter:
			char *env: env parameter 
			char* value:the parameter value
 Return :	
 			NULL
******************************************************************/
extern void MinspeedSetEnv(char *env, char *value);

/****************************************************************
 Function: MinspeedReadEnv
 Description: Get Env
 Parameter:
			char *msg: the value buffer 
			char* env: env parameter 
 Return :	
 			get success return 0
******************************************************************/
extern int  MinspeedReadEnv(char *msg, char *env);

/****************************************************************
 Function: MinspeedReadEnvToFile
 Description: read env to file
 Parameter:
			char *char *configFile: configfile file 
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedReadEnvToFile(char *configFile);
/*****************************************************************
Function:	MinspeedGetRxSensitivity
Description: get rx sensitivity from gpio pin 6 (start with 0)
	 		 #define KM_TRIPLEXER_SD km_GPIO_6  //input (start with km_GPIO_0)
Parameter:	 NULL
			 
Return:	error -1, 0 fail 1 true
*****************************************************************/
extern int MinspeedGetRxSensitivity();
/****************************************************************
 Function: MinspeedGetSensitivity
 Description: gps command
 Parameter:
			char *str: command 
			char* msg:the value buffer
 Return :	
 			get success return 0
******************************************************************/
extern int MinspeedGetSensitivity(char *str, char* msg);
#endif

