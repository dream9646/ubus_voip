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
 * File    : er_group_hw_241.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "omcimsg.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "metacfg_adapter.h"
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
#include "coretask.h"
#endif

#define NTP_CONFIG_OVERRIDE	"/var/run/ntp.config.override"

// 241 Information
static time_t tv_sec_previous = 0;

struct datetime_timezone_t {
	int time_offset;
	char *location_str;
	char *offset_str;
};

struct datetime_timezone_t datetime_timezone_list[]={
	{-43200, "International Date Line West", "MHT+12"},
	{-39600, "Coordinated Universal Time-11", "SST+11"},
	{-36000, "Hawaii", "HST+10"},
	{-32400, "Alaska", "AKST+9"},
	{-28800, "Pacific Time (US, Canada)", "PST+8"},
	{-28800, "Baja California", "PST+8"},
	{-25200, "Mountain Time (US, Canada)", "MST+7"},
	{-25200, "Arizona", "MST+7"},
	{-25200, "Chihuahua, La Paz, Mazatlan", "MST+7"},
	{-21600, "Central Time (US, Canada)", "CST+6"},
	{-21600, "Central America", "CST+6"},
	{-21600, "Guadalajara, Mexico City, Monterrey", "CST+6"},
	{-21600, "Saskatchewan", "CST+6"},
	{-18000, "Eastern Time (US, Canada)", "EST+5"},
	{-18000, "Bogota, Lima, Quito", "EST+5"},
	{-18000, "Indiana (East)", "EST+5"},
	{-16200, "Caracas", "VET+4:30"},
	{-14400, "Atlantic Time (Canada)", "AST+4"},
	{-14400, "Asuncion", "AST+4"},
	{-14400, "Cuiaba", "AST+4"},
	{-14400, "Georgetown, La Paz, Manaus, San Juan", "AST+4"},
	{-14400, "Santiago", "AST+4"},
	{-12600, "Newfoundland", "NST+3:30"},
	{-10800, "Brasilia", "BRT+3"},
	{-10800, "Buenos Aires", "BRT+3"},
	{-10800, "Cayenne, Fortaleza", "WGT+3"},
	{-10800, "Greenland", "WGT+3"},
	{-10800, "Montevideo", "WGT+3"},
	{-10800, "Salvador", "WGT+3"},
	{-7200, "Coordinated Universal Time-02", "GST+2"},
	{-7200, "Mid-Atlantic", "GST+2"},
	{-3600, "Azores", "AZOT+1"},
	{-3600, "Cape Verde Is.", "CVT+1"},
	{0, "Dublin, Edinburgh, Lisbon, London", "GMT+0"},
	{0, "Casablanca", "GMT+0"},
	{0, "Coordinated Universal Time", "GMT+0"},
	{0, "Monrovia, Reykjavik", "GMT+0"},
	{3600, "Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "CET-1"},
	{3600, "Belgrade, Bratislava, Budapest, Ljubljana, Prague", "CET-1"},
	{3600, "Brussels, Copenhagen, Madrid, Paris", "CET-1"},
	{3600, "Sarajevo, Skopje, Warsaw, Zagreb", "CET-1"},
	{3600, "West Central Africa", "WAT-1"},
	{3600, "Windhoek", "WAT-1"},
	{7200, "Amman", "IST-2"},
	{7200, "Athens, Bucharest", "IST-2"},
	{7200, "Beirut", "IST-2"},
	{7200, "Cairo", "CAT-2"},
	{7200, "Damascus", "CAT-2"},
	{7200, "Harare, Pretoria", "CAT-2"},
	{7200, "Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius", "CAT-2"},
	{7200, "Istanbul", "CAT-2"},
	{7200, "Jerusalem", "CAT-2"},
	{7200, "Nicosia", "CAT-2"},
	{10800, "Baghdad", "MSK-3"},
	{10800, "Kaliningrad, Minsk", "MSK-3"},
	{10800, "Kuwait, Riyadh", "MSK-3"},
	{10800, "Nairobi", "MSK-3"},
	{12600, "Tehran", "IRST-3:30"},
	{14400, "Abu Dhabi, Muscat", "GST-4"},
	{14400, "Baku", "GST-4"},
	{14400, "Moscow, St. Petersburg, Volgograd", "GST-4"},
	{14400, "Port Louis", "GST-4"},
	{14400, "Tbilisi", "GST-4"},
	{14400, "Yerevan", "GST-4"},
	{16200, "Kabul", "AFT-4:30"},
	{18000, "Islamabad, Karachi", "PKT-5"},
	{18000, "Tashkent",},
	{19800, "Chennai, Kolkata, Mumbai, New Delhi", "PKT-5"},
	{19800, "Sri Jayawardenepura", "SLT-5:30"},
	{20700, "Kathmandu", "NPT-5:45"},
	{21600, "Astana", "BDT-6"},
	{21600, "Dhaka", "BDT-6"},
	{21600, "Ekaterinburg", "BDT-6"},
	{23400, "Yangon (Rangoon)", "MMT-6:30"},
	{25200, "Bangkok, Hanoi, Jakarta", "ICT-7"},
	{25200, "Novosibirsk", "NOVST-7"},
	{28800, "Beijing, Chongqing, Hong Kong, Urumqi", "CST-8"},
	{28800, "Krasnoyarsk", "CST-8"},
	{28800, "Kuala Lumpur, Singapore", "CST-8"},
	{28800, "Perth", "CST-8"},
	{28800, "Taipei", "CST-8"},
	{28800, "Ulaanbaatar", "CST-8"},
	{32400, "Irkutsk", "JST-9"},
	{32400, "Osaka, Sapporo, Tokyo", "JST-9"},
	{32400, "Seoul", "KST-9"},
	{34200, "Adelaide", "ACST-9:30"},
	{34200, "Darwin", "ACST-9:30"},
	{36000, "Brisbane", "PGT-10"},
	{36000, "Canberra, Melbourne, Sydney", "EST-10"},
	{36000, "Guam, Port Moresby", "EST-10"},
	{36000, "Hobart", "EST-10"},
	{36000, "Yakutsk", "EST-10"},
	{39600, "Solomon Is., New Caledonia", "NCT-11"},
	{39600, "Vladivostok", "VLAST-11"},
	{43200, "Auckland, Wellington", "FJT-12"},
	{43200, "Coordinated Universal Time+12", "NZST-12"},
	{43200, "Fiji", "NZST-12"},
	{43200, "Magadan", "NZST-12"},
	{46800, "Nuku'alofa", "WST-13"},
	{46800, "Samoa", "WST-13"},
	{-1, NULL, NULL}
};

int get_tz_offset_location_str_by_seconds(int tz_seconds, char **offset_str, char **location_str) 
{
	int i;
	for(i=0; i < (sizeof(datetime_timezone_list)/sizeof(struct datetime_timezone_t)); i++) {
		if(tz_seconds == datetime_timezone_list[i].time_offset) {
			*offset_str = datetime_timezone_list[i].offset_str;
			*location_str = datetime_timezone_list[i].location_str;
			return 0;
		}
	}
	*offset_str ="GMT+0";
	*location_str  ="Dublin, Edinburgh, Lisbon, London";
	return -1;
}

//translation string to second in integer (support: 8, 0, 8:30, 08:45)
int tz_time_sub_string_to_int(char *str, int *tz_sec)
{
	int i, str_len;
	char tz_time_sub_str[12];	//max: XXXXX+11:45

	str_len=strlen(str);
	//printf("strlen=%d\n", str_len);

	for(i=0; i < str_len; i++) {
		if(str[i] == '+' || str[i] == '-') {
			struct tm tm;
			char minus_flag=0;

			if(str[i] == '+')	//{-43200, "International Date Line West", "MHT+12"},
				minus_flag=1;

			//(+|-)12:30
			snprintf(tz_time_sub_str, 12, "%s", str+i);

			memset(&tm, 0, sizeof(tm));
			if (strptime(str+i+1, "%H", &tm) != NULL) {
				//printf("hour: %d; minute: %d\n",  tm.tm_hour, tm.tm_min);
				//printf("seconds %d\n", tm.tm_hour*3600);
				if(minus_flag)
					*tz_sec= -(tm.tm_hour*3600);
				else
					*tz_sec= (tm.tm_hour*3600);
				return 0;
			} else if (strptime(str+i+1, "%H:%M", &tm) != NULL) {
				//printf("hour: %d; minute: %d\n",  tm.tm_hour, tm.tm_min);
				//printf("seconds %d\n",  tm.tm_hour*3600 + tm.tm_min*60);
				if(minus_flag)
					*tz_sec= -(tm.tm_hour*3600 + tm.tm_min*60);
				else
					*tz_sec= (tm.tm_hour*3600 + tm.tm_min*60);
				return 0;
			}
		}
	}
	return -1;
}

// 241@2
int 
er_group_hw_date_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	FILE *fp;
	char tmpfname[128];
	struct timeval tv;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		tv.tv_sec = attr_inst->er_attr_instance[0].attr_value.data;
		tv.tv_usec = 0;

		if (tv.tv_sec && (tv.tv_sec != tv_sec_previous)) {// valid value if non-zero and tv_sec isn't previous timestamp
			settimeofday(&tv, NULL);
#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
			if (tv_sec_previous != 0) {
				coretask_omci_set_time_sync_state(1,&tv);
				dbprintf(LOG_ERR, "OMCI time sync. (%ld)\n",tv.tv_sec);
			}
#endif
		}

		snprintf(tmpfname, 127, "/var/run/ntp.me241.%u", (unsigned int)time(0));
		if((fp=fopen(tmpfname, "a+")) != NULL) {
			fprintf(fp, "datetime_ntp_enable=0\n");
			fclose(fp);
			rename(tmpfname, NTP_CONFIG_OVERRIDE);
		}
		tv_sec_previous = tv.tv_sec;

		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_date_time(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// 241@10,11
int 
er_group_hw_date_timezone(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
#ifdef OMCI_METAFILE_ENABLE
	char diff_filename[128], cmd[256];
	FILE *fp;
	int old_tz_seconds, tz_seconds;
	int old_tz_dsttime_enable, tz_dsttime_enable;	
#endif

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
#ifdef OMCI_METAFILE_ENABLE
		//if current datetime_timezone=ABCDE-8, we only check -8, and keep ABCDE(maybe set from tr069 or web)
		//if get "-9" from OLT, we update datetime_timezone and datetime_timezone_name by our datetime_timezone_list
		old_tz_seconds = -1;
		old_tz_dsttime_enable = -1;
		{ // init old value with an impossible value -1, so in case get failed, the cuttent and old values would be different
			struct metacfg_t kv_config_metafile;
			char *offset_str;
			
			memset(&kv_config_metafile, 0x0, sizeof(struct metacfg_t));
			metacfg_adapter_config_init(&kv_config_metafile);
			// load from line datetime_ntp_enable to datetime_server_retry block	
			if(metacfg_adapter_config_file_load_part(&kv_config_metafile, "/etc/wwwctrl/metafile.dat", "datetime_ntp_enable", "datetime_server_retry") !=0) 
			{
				metacfg_adapter_config_release(&kv_config_metafile);
				return -1;
			}
			if(strlen(offset_str = metacfg_adapter_config_get_value(&kv_config_metafile, "datetime_timezone", 1)) > 0) {
				//dbprintf(LOG_ERR, "datetime_timezone=%s\n", offset_str);
				tz_time_sub_string_to_int(offset_str, &old_tz_seconds);
			}
			old_tz_dsttime_enable = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, "datetime_dst_enable", 1)); 			
			metacfg_adapter_config_release(&kv_config_metafile);
		}

		tz_seconds = attr_inst->er_attr_instance[0].attr_value.data;
		tz_dsttime_enable = attr_inst->er_attr_instance[1].attr_value.data;

		if (tz_seconds != old_tz_seconds || tz_dsttime_enable != old_tz_dsttime_enable) {
			snprintf(diff_filename, 127, "/var/run/datetime.diff.%lu", random()%1000000);
			if((fp = fopen(diff_filename, "w+")) != NULL) {
				//only when tz time offset is change, we modify tz offset and tz location name
				if(tz_seconds != old_tz_seconds) {
					char *offset_str, *location_str;
					get_tz_offset_location_str_by_seconds(tz_seconds, &offset_str, &location_str);
					//dbprintf(LOG_ERR, "ME241 TZ_INFO=%s, location=%s\n", offset_str, location_str);
					//dbprintf(LOG_ERR, "old_tz_seconds=%d, tz_seconds=%d\n", old_tz_seconds, tz_seconds);
					fprintf(fp, "datetime_timezone=%s\n", offset_str);
					fprintf(fp, "datetime_timezone_name=%s\n", location_str);
				}
				if (tz_dsttime_enable != old_tz_dsttime_enable) {
					fprintf(fp, "datetime_dst_enable=%d\n", tz_dsttime_enable);
				}
				fclose(fp);
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 255, "/etc/init.d/cmd_dispatcher.sh diff %s", diff_filename);
				util_run_by_thread(cmd, 1);
#endif
				unlink(diff_filename);
			}
		}
#endif
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_date_timezone(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// 241@3
int 
er_group_hw_telnet_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char *massproduct = NULL, enable = attr_inst->er_attr_instance[0].attr_value.data;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((massproduct=util_get_bootenv("massproduct")) != NULL) {
			if(!atoi(massproduct)) { // for shipping mode only
				if (enable) { // open port 2001
					util_run_by_system("killall -9 telnetd");
					util_run_by_system("telnetd -p 2001");
				} else { // disable telnetd
					util_run_by_system("killall -9 telnetd");
				}
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
		if (er_group_hw_telnet_enable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// 241@7
int 
er_group_hw_ookla_disable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char disable = (attr_inst->er_attr_instance[0].attr_value.data==1)?1:0;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (disable) {

		} else {

		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_ookla_disable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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
