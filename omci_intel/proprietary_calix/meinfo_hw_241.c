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
 * Module  : proprietary_calix
 * File    : meinfo_hw_241.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "metacfg_adapter.h"

//classid 241 Serial_Number
extern int tz_time_sub_string_to_int(char *str, int *tz_sec);

static int 
meinfo_hw_241_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}

	//todo: need update other attribute(especial attr 2)
	
	// 1: MfgSerialNumber
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		unsigned char *sn, mac[6];
		attr.data=0;
		attr.ptr= malloc_safe(12);
		memset(mac, 0, sizeof(mac));
		if ((sn=util_get_bootenv("calix_serial_number")) != NULL) {
			sprintf(attr.ptr, "%s", sn);
		} else {
			if (switch_hw_g.eth_address_get)
				switch_hw_g.eth_address_get(0, mac);
			sprintf(attr.ptr, "33%02X%02X%02X%02X%02X",mac[1],mac[2],mac[3],mac[4],mac[5]);
		}
		meinfo_me_attr_set(me, 1, &attr, 0);
		if (attr.ptr)
			free_safe(attr.ptr);
	}
	// 2: DateTime
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		struct attr_value_t attr;
		//struct timeval tv;
		//gettimeofday(&tv, NULL);	// get current time
		//attr.data = tv.tv_sec;
		attr.data = time(0);		// get current time in second, lower overhead
		meinfo_me_attr_set(me, 2, &attr, 0);
	}

#ifdef OMCI_METAFILE_ENABLE
	// 10: TZoffset or 11: DST
	if (util_attr_mask_get_bit(attr_mask, 10) || util_attr_mask_get_bit(attr_mask, 11)) {
		struct metacfg_t kv_config_metafile;
		int tz_sec=0, tz_dsttime=0;
		char *tmp_str;

		memset(&kv_config_metafile, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config_metafile);
		// load from line datetime_ntp_enable to datetime_server_retry block	
		if(metacfg_adapter_config_file_load_part(&kv_config_metafile, "/etc/wwwctrl/metafile.dat", "datetime_ntp_enable", "datetime_server_retry") !=0) 
		{
			metacfg_adapter_config_release(&kv_config_metafile);
			return -1;
		}

		if(strlen(tmp_str = metacfg_adapter_config_get_value(&kv_config_metafile, "datetime_timezone", 1)) > 0) {
			tz_time_sub_string_to_int(tmp_str, &tz_sec);
			//dbprintf(LOG_ERR, "datetime_timezone=%s, %d\n", tmp_str, tz_sec);
			attr.data = (unsigned int)tz_sec;		// cast to ensure only 4 byte are copied to attr.data, or value of tz_sec will be cast to attr.data datatype, which is 8 byte
			meinfo_me_attr_set(me, 10, &attr, 0);
		}

		if(strlen(tmp_str = metacfg_adapter_config_get_value(&kv_config_metafile, "datetime_dst_enable", 1)) > 0) {
			tz_dsttime=atoi(tmp_str);
			//dbprintf(LOG_ERR, "tz_dsttime=%d\n", tz_dsttime);
			attr.data = tz_dsttime;
			meinfo_me_attr_set(me, 11, &attr, 0);
		}

		metacfg_adapter_config_release(&kv_config_metafile);
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_241 = {
	.get_hw		= meinfo_hw_241_get,
};
