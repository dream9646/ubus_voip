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
 * Module  : er_group_hw
 * File    : er_group_hw_131.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "switch.h"
#include "omcimain.h"
#include "er_group.h"
#include "er_group_hw.h"
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
#include "coretask.h"
#endif

#define OMCI_ENV_FILE_PATH "/etc/omci/omcienv.xml"
#define OMCI_CUSTOM_ENV_PATH "/etc/omci/custom_env"
#define OMCI_VENDOR_FILE ".olt_vendor"

// 131 OLT-G

void olt_vendor_save(char *vendor)
{
	char cmd[64];
	FILE *fp = popen("mount | grep storage", "r");
	
	if(fp) {
		char buf[256];
		if (fgets(buf, sizeof(buf), fp) && strstr(buf, "/storage")) {
			// use /storage to save olt_vendor file
			sprintf(cmd, "echo %s > /storage/%s", vendor, OMCI_VENDOR_FILE);
		} else {
			// use /nvram to save olt_vendor file
			sprintf(cmd, "echo %s > /nvram/%s", vendor, OMCI_VENDOR_FILE);
		}
		pclose(fp);
	}
	util_run_by_system(cmd);
}

void omcienv_xml_change(char *vendor)
{
	char *sw_tryactive = util_get_bootenv("sw_tryactive");
	
	// only proceed when sw_tryactive==2 (active&commit)
	if ( NULL!=sw_tryactive && atoi(sw_tryactive)!=2 ) {
		return;
	} else {
		FILE *fp;
		char config[64];

		memset(config, 0, sizeof(config));
#if 0
		sprintf(config, "%s.%s", OMCI_ENV_FILE_PATH, vendor);
		if ((fp = fopen(config, "r")) != NULL) {
			unlink(OMCI_ENV_FILE_PATH);
			symlink(config, OMCI_ENV_FILE_PATH);
			fclose(fp);
			util_run_by_system("reboot");
		}
	}
#else
		sprintf(config, "%s.%s", OMCI_CUSTOM_ENV_PATH, vendor);
		if ((fp = fopen(config, "r")) != NULL) {
			char cmd[128];
			fclose(fp);
			sprintf(cmd, "custom_omcienv.sh %s %s.%s", OMCI_ENV_FILE_PATH, OMCI_CUSTOM_ENV_PATH, vendor);
			util_run_by_system(cmd);
			util_run_by_system("reboot");
			omci_stop_shutdown();
		}
	}
#endif
}

// 131@1
int 
er_group_hw_olt_vendor_check(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(omci_env_g.olt_vendor_check_enable) {
			int i;
			if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_HUAWEI) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ZTE) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_DASAN) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_DASAN) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ERICSSON) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ADTRAN) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ADTRAN) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_FIBERHOME) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME) != 0)) {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 1); // enable phyiso
				}
				util_run_by_system("echo 3 > /tmp/led_sw_upgrade"); // alarm on
			} else {
				for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ((1<<i) & switch_get_uni_logical_portmask())
						switch_hw_g.port_phyiso_set(i, 0); // disable phyiso
				}
				util_run_by_system("echo 0 > /tmp/led_sw_upgrade"); // alarm off
			}
		}
		if(omci_env_g.olt_vendor_config_change) {
			if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU)
				olt_vendor_save(ENV_OLT_PROPRIETARY_ALU);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_HUAWEI)
				olt_vendor_save(ENV_OLT_PROPRIETARY_HUAWEI);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ZTE)
				olt_vendor_save(ENV_OLT_PROPRIETARY_ZTE);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_DASAN)
				olt_vendor_save(ENV_OLT_PROPRIETARY_DASAN);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX)
				olt_vendor_save(ENV_OLT_PROPRIETARY_CALIX);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ERICSSON)
				olt_vendor_save(ENV_OLT_PROPRIETARY_ERICSSON);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ADTRAN)
				olt_vendor_save(ENV_OLT_PROPRIETARY_ADTRAN);
			else if (omci_get_olt_vendor() == ENV_OLT_WORKAROUND_FIBERHOME)
				olt_vendor_save(ENV_OLT_PROPRIETARY_FIBERHOME);
			if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_ALU);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_HUAWEI) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_HUAWEI);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ZTE) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_ZTE);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_DASAN) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_DASAN) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_DASAN);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_CALIX);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ERICSSON) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_ERICSSON);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ADTRAN) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ADTRAN) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_ADTRAN);
			} else if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_FIBERHOME) && 
			    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME) != 0)) {
				omcienv_xml_change(ENV_OLT_PROPRIETARY_FIBERHOME);
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_olt_vendor_check(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}
	
	return 0;
}

// 131@4 
int 
er_group_hw_timeofday_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (util_attr_mask_get_bit(attr_mask, 4)) {	// 4: Time of day information
			struct timeval tv;
			// TStampN - seconds field (6 bytes)  // TStampN is defined IEEE 1588-2008
			tv.tv_sec = (unsigned long)util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 112, 32, 48);
			// TStampN - nanoseconds field (4 bytes)		
			tv.tv_usec = (unsigned long)(util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 112, 80, 32)/1000);
			
			//dbprintf(LOG_ERR, "TStampN: Sec(%dB)=0x%012x, NanoSec(%dB)=0x%08x\n",sizeof(tv.tv_sec), tv.tv_sec, sizeof(tv.tv_usec), tv.tv_usec);
			
			if (tv.tv_sec) {// valid value if non-zero
				settimeofday(&tv, NULL);
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
				coretask_omci_set_time_sync_state(1,&tv);
				dbprintf(LOG_ERR, "OMCI time sync. (%ld)\n",tv.tv_sec);
#endif
			}
		}
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_timeofday_info(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;		
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	
	return 0;
}
