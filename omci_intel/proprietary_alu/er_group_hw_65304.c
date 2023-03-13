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
 * File    : er_group_hw_65304.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "omcimsg.h"
#include "er_group.h"
#include "er_group_hw.h"
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
#include "coretask.h"
#endif

#define NTP_CONFIG_OVERRIDE	"/var/run/ntp.config.override"

// 65304 NTP_Configuration_V2_ME

// 65304@1,9,10
int 
er_group_hw_current_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	extern struct omcimsg_type_attr_t omcimsg_type_attr[32];

	// ALU uses message type 29 as "proprietary set without MIB data sync updated"
	omcimsg_type_attr[OMCIMSG_SET_TABLE].inc_mib_data_sync = 0;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(attr_inst->er_attr_instance[0].attr_value.data == 0) {	// NTP: OFF
			FILE *fp;
			char tmpfname[128];
			struct timeval tv;
			struct timezone tz;

			tv.tv_sec = attr_inst->er_attr_instance[1].attr_value.data;
			tv.tv_usec = 0;
			tz.tz_minuteswest = attr_inst->er_attr_instance[2].attr_value.data;
			tz.tz_dsttime = 0;
			if (tv.tv_sec) {// valid value if non-zero
				settimeofday(&tv, &tz);
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
				coretask_omci_set_time_sync_state(1,&tv);
				dbprintf(LOG_ERR, "OMCI time sync. (%ld)\n",tv.tv_sec);
#endif
			}

			snprintf(tmpfname, 127, "/var/run/ntp.me65304.%u", (unsigned int)time(0));
			if ((fp=fopen(tmpfname, "a+")) != NULL) {
				fprintf(fp, "datetime_ntp_enable=0\n");
				fclose(fp);
				rename(tmpfname, NTP_CONFIG_OVERRIDE);
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
		if (er_group_hw_current_time(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// hour offset to uclibc TZ name, 
// the first 3 byte is localtime timezone name, the sign/number is the offset from GMT to localtime
char *
hour_offset_to_tz(int hour_offset)
{
	switch(hour_offset) {
		case -12:	return "MHT+12";	// GMT-12:00  Eniwetok, Kwajalein </option>
		case -11:	return "SST+11";	// GMT-11:00  Midway Island, Samoa </option>
		case -10:	return "HST+10";	// GMT-10:00  Hawaii </option>
		case -9:	return "YST+9";		// GMT-09:00  Alaska </option>
		case -8:	return "PST+8";		// GMT-08:00  Pacific Time (US &amp; Canada), Tijuana </option>
		case -7:	return "MST+7";		// GMT-07:00  Arizona; Mountain Time (US &amp; Canada) </option>
		case -6:	return "CST+6";		// GMT-06:00  Central Time (US &amp; Canada) </option>
		case -5:	return "EST+5";		// GMT-05:00  Eastern Time (US &amp; Canada) </option>
		case -4:	return "AST+4";		// GMT-04:00  Atlantic Time (Canada) </option>
		case -3:	return "EBT+3";		// GMT-03:00  Buenos Aires, Georgetown </option>
		case -2:	return "GST+2";		// GMT-02:00  Mid-Atlantic </option>
		case -1:	return "CVT+1";		// GMT-01:00  Azores; Cape Verde Is. </option>
		case -0:	return "GMT+0";		// GMT  Greenwich Mean Time:  Dublin, Edinburgh, London </option>
		case 1:		return "CET-1";		// GMT+01:00  Berlin, Rome, Vienna, Paris, Sarajevo </option>
		case 2:		return "IST-2";		// GMT+02:00  Israel </option>
		case 3:		return "MSK-3";		// GMT+03:00  St. Petersburg, Volgograd, Nairobi </option>
		case 4:		return "GST-4";		// GMT+04:00  Abu Dhabi, Muscat, Yerevan </option>
		case 5:		return "PKT-5";		// GMT+05:00  Islamabad, Karachi, Tashkent </option>
		case 6:		return "BDT-6";		// GMT+06:00  Almaty, Dhaka </option>
		case 7:		return "ICT-7";		// GMT+07:00  Bangkok, Hanoi, Jakarta </option>
		case 8:		return "CST-8";		// GMT+08:00  Taipei, Beijing, Chongqing, Hong Kong </option>
		case 9:		return "JST-9";		// GMT+09:00  Osaka, Sapporo, Toyko, Seoul, Yakutsk </option>
		case 10:	return "PGT-10";	// GMT+10:00  Guam, Port Moresby </option>
		case 11:	return "NCT-11";	// GMT+11:00  Magadan, Solamon, New Caledonia </option>
		case 12:	return "FJT-12";	// GMT+12:00  Auckland, Wellington, Fiji </option>
	}
	return NULL;
}

// 65304@1,3,4,5,10
int 
er_group_hw_ntp_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(((unsigned int) attr_inst->er_attr_instance[0].attr_value.data) == 1) {	// NTP: ON
			FILE *fp;
			char tmpfname[128];

			snprintf(tmpfname, 127, "/var/run/ntp.me65304.%u", (unsigned int)time(0));
			if ((fp=fopen(tmpfname, "a+")) != NULL) {
				struct in_addr ipaddr1, ipaddr2, ipaddr3;
				short hour_offset = ((short) attr_inst->er_attr_instance[4].attr_value.data) / 60;
				char *tz_str = hour_offset_to_tz(hour_offset);

				ipaddr1.s_addr=htonl(attr_inst->er_attr_instance[1].attr_value.data);
				ipaddr2.s_addr=htonl(attr_inst->er_attr_instance[2].attr_value.data);
				ipaddr3.s_addr=htonl(attr_inst->er_attr_instance[3].attr_value.data);
				if (ipaddr1.s_addr)
					fprintf(fp, "datetime_ntp_server1=%s\n", inet_ntoa(ipaddr1));
				if (ipaddr2.s_addr)
					fprintf(fp, "datetime_ntp_server2=%s\n", inet_ntoa(ipaddr2));
				if (ipaddr3.s_addr)
					fprintf(fp, "datetime_ntp_server3=%s\n", inet_ntoa(ipaddr3));

				// follow timezone only if at least one ntp server is assigned
				if (tz_str && (ipaddr1.s_addr || ipaddr2.s_addr || ipaddr3.s_addr))
					fprintf(fp, "datetime_timezone=%s\n", tz_str);

				fprintf(fp, "datetime_ntp_enable=1\n");
				fclose(fp);
			}

			// ntp.sh will use NTP_CONFIG_OVERRIDE to override the config in metafile
			if (util_file_size(tmpfname) > 0) {
				rename(tmpfname, NTP_CONFIG_OVERRIDE);
				// while ntp is already executed in crontab, its updateinterval might be large, eg: 24hour
				// in order to have change take effect ASAP, we execute ntp immediately after override file is generated
				// execute this command in background to avoid delay MIB reset process
				util_run_by_thread("/etc/init.d/ntp.sh config", 0);
			} else {
				unlink(tmpfname);
				unlink(NTP_CONFIG_OVERRIDE);
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
		if (er_group_hw_ntp_time(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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
