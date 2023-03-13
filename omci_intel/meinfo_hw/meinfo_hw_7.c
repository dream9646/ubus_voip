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
 * File    : meinfo_hw_7.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"
#include "omcimsg.h"
#include "hwresource.h"
#include "switch.h"


#define PATH_LEN	256
#define TOKEN_LEN	14+1
#define STRING_LEN	256 

#define VENDOR_VOIP_CONFIG	"/etc/voip/voip_supplement.cfg"
#define VENDOR_RG_CONFIG		"/etc/wwwctrl/rg.cfg"

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
static int
meinfo_hw_parse_dump_env(struct omcimsg_sw_download_param_t *param)
{
	int ret;


	memset(param, 0x0, sizeof(struct omcimsg_sw_download_param_t));
	ret = switch_hw_g.sw_image_version_get(0, 14, param->version0);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_version_get 0 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_version_get(1, 14, param->version1);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_version_get 1 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_active_get(0, &param->active0);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_active_get 0 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_active_get(1, &param->active1);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_active_get 2 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_commit_get(0, &param->commit0);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_commit_get 0 error!\n");
		return -1;
	}


	ret = switch_hw_g.sw_image_commit_get(1, &param->commit1);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_commit_get 1 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_valid_get(0, &param->valid0);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_valid_get 0 error!\n");
		return -1;
	}

	ret = switch_hw_g.sw_image_valid_get(1, &param->valid1);
	if(ret < 0)
	{
		dbprintf(LOG_ERR, "sw_image_valid_get 1 error!\n");
		return -1;
	}

	return 0;
}

#else
static int
meinfo_hw_parse_dump_env(struct omcimsg_sw_download_param_t *param)
{
	FILE *fptr;
	char filename[]="/var/run/swimage/env_for_me7.log";
	char strbuf[STRING_LEN], strtmp[STRING_LEN], keybuf[TOKEN_LEN], valbuf[TOKEN_LEN], *ch, *next_start;
	char exec_cmd[PATH_LEN];
	int ret;

	//parse u-boot-env and version number(maybe need moutnt another image)
	//update firmware to flash (/usr/sbin/sw_download.sh dump_env )
	//
	memset(param, 0x0, sizeof(struct omcimsg_sw_download_param_t));
	snprintf(exec_cmd, PATH_LEN,"%s %s",omci_env_g.sw_download_script, "dump_env");

	ret=util_run_by_system((char *)exec_cmd);
	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "dump_env exec_cmd=[%s]\n", exec_cmd);
		dbprintf(LOG_ERR, "start_sw_update error ret=%d!\n", WEXITSTATUS(ret));
		return -1;
	}

	if ((fptr = fopen(filename, "r")) == NULL) {    /*which file you want to parser */
		dbprintf(LOG_ERR, "Can't open %s\n", filename);
		return -1;
	}

	while (util_fgets(strtmp, STRING_LEN, fptr, 1, 1)!=NULL) {
		strcpy(strbuf, util_trim(strtmp));
		if (strbuf[0] == '#' || strbuf[0] == 0)
			continue;

		if ((ch = strchr(strbuf, '=')) == NULL) {
			dbprintf(LOG_ERR, "%s: miss '='\n", filename);
			fclose(fptr);
			return -1;
		} else {
			next_start = ch + 1;
			*ch = 0x0;
			strncpy(keybuf, strbuf, TOKEN_LEN);
			strncpy(valbuf, next_start, TOKEN_LEN);
			if(valbuf == NULL){
				dbprintf(LOG_ERR, "%s: miss value\n", filename);
				fclose(fptr);
				return -1;
			}

			if( strcmp(keybuf, "sw_version0")==0) {
				if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) &&
					(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE)) {
					// for zte olt, force to add 'V' ahead of version string (for TI case)
					if(valbuf[0] != 'V') {
						char buf[TOKEN_LEN];
						buf[0] = 'V';
						strcpy(buf+1, valbuf);
						strcpy(valbuf, buf);
					}
					// for zte olt, force to add '_TED' in the tail of version string (for TE case)
					if(strstr(valbuf, "_TED") == 0)
						strncat(valbuf, "_TED", 4);
				}
				strncpy(param->version0, valbuf, TOKEN_LEN);
			}

			if( strcmp(keybuf, "sw_version1")==0) {
				if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) &&
					(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE)) {
					// for zte olt, force to add 'V' ahead of version string (for TI case)
					if(valbuf[0] != 'V') {
						char buf[TOKEN_LEN];
						buf[0] = 'V';
						strcpy(buf+1, valbuf);
						strcpy(valbuf, buf);
					}
					// for zte olt, force to add '_TED' in the tail of version string (for TE case)
					if(strstr(valbuf, "_TED") == 0)
						strncat(valbuf, "_TED", 4);
				}
				strncpy(param->version1, valbuf, TOKEN_LEN);
			}

			if( strcmp(keybuf, "sw_tryactive")==0) {
				param->active0=!atoi(valbuf);
				param->active1=atoi(valbuf);
			}

			if( strcmp(keybuf, "sw_commit")==0) {
				param->commit0=!atoi(valbuf);
				param->commit1=atoi(valbuf);
			}

			if( strcmp(keybuf, "sw_valid0")==0) {
				param->valid0=atoi(valbuf);
			}

			if( strcmp(keybuf, "sw_valid1")==0) {
				param->valid1=atoi(valbuf);
			}

			if( strcmp(keybuf, "sw_active0_ok")==0) {
				param->active0_ok=atoi(valbuf);
			}

			if( strcmp(keybuf, "sw_active1_ok")==0) {
				param->active1_ok=atoi(valbuf);
			}

		//dbprintf(LOG_ERR, "[%s][%s]\n", keybuf, valbuf);
		}
	}
	fclose(fptr);
#if 0
	dbprintf(LOG_ERR, "active0=%d, active1=%d\n", param->active0, param->active1);
	dbprintf(LOG_ERR, "commit0=%d, commit1=%d\n", param->commit0, param->commit1);
	dbprintf(LOG_ERR, "valid0=%d, valid1=%d\n", param->valid0, param->valid1);
	dbprintf(LOG_ERR, "active0_ok=%d, active1_ok=%d\n", param->active0_ok, param->active1_ok);
#endif
	return 0;
}
#endif


static int
meinfo_hw_parse_vendor_config(struct omcimsg_sw_download_param_t *param)
{
	FILE *fptr;
	char strbuf[STRING_LEN], strtmp[STRING_LEN], keybuf[TOKEN_LEN], valbuf[TOKEN_LEN], *ch, *next_start;

	memset(param, 0x0, sizeof(struct omcimsg_sw_download_param_t));
	param->version0[0] = 0x30; // "0"
	param->version1[0] = 0x30; // "0"

	if ((fptr = fopen(VENDOR_VOIP_CONFIG, "r")) != NULL) {    /*which file you want to parser */

		while (util_fgets(strtmp, STRING_LEN, fptr, 1, 1)!=NULL) {
			strcpy(strbuf, util_trim(strtmp));
			if (strstr(strbuf, ";Version") || strstr(strbuf, ";CalixVersion"))
				;
			else if (strbuf[0] == '#' || strbuf[0] == ';' || strbuf[0] == 0)
				continue;

			if ((ch = strchr(strbuf, ':')) == NULL) {
				if((ch = strchr(strbuf, '=')) == NULL)
					continue;
			}
			next_start = ch + 1;
			*ch = 0x0;
			strncpy(keybuf, strbuf, TOKEN_LEN);
			strncpy(valbuf, next_start, TOKEN_LEN);
			if(valbuf == NULL){
				dbprintf(LOG_ERR, "%s: miss value\n", VENDOR_VOIP_CONFIG);
				fclose(fptr);
				break;
			}

			if( strcmp(keybuf, "Version")==0 || strcmp(keybuf, "CalixVersion")==0 || strcmp(keybuf, ";Version")==0 || strcmp(keybuf, ";CalixVersion")==0) {
				int i=0, j=0;
				memset(keybuf, 0, sizeof(keybuf));
				 for(i=0; i<sizeof(valbuf); i++) {
					if(valbuf[i] != '"')
						keybuf[j++] = valbuf[i];
				}
				strncpy(param->version0, keybuf, TOKEN_LEN);
				param->valid0=1;
				param->active0_ok=1;
				param->active0=1;
				param->commit0=1;
				break;
			}
		}
		fclose(fptr);
	}
	
	if ((fptr = fopen(VENDOR_RG_CONFIG, "r")) != NULL) {    /*which file you want to parser */

		while (util_fgets(strtmp, STRING_LEN, fptr, 1, 1)!=NULL) {
			strcpy(strbuf, util_trim(strtmp));
			if (strstr(strbuf, ";Version") || strstr(strbuf, ";CalixVersion"))
				;
			else if (strbuf[0] == '#' || strbuf[0] == ';' || strbuf[0] == 0)
				continue;
			if ((ch = strchr(strbuf, ':')) == NULL) {
				if((ch = strchr(strbuf, '=')) == NULL)
					continue;
			}
			next_start = ch + 1;
			*ch = 0x0;
			strncpy(keybuf, strbuf, TOKEN_LEN);
			strncpy(valbuf, next_start, TOKEN_LEN);
			if(valbuf == NULL){
				dbprintf(LOG_ERR, "%s: miss value\n", VENDOR_RG_CONFIG);
				fclose(fptr);
				break;
			}

			if( strcmp(keybuf, "Version")==0 || strcmp(keybuf, "CalixVersion")==0 || strcmp(keybuf, ";Version")==0 || strcmp(keybuf, ";CalixVersion")==0) {
				int i=0, j=0;
				memset(keybuf, 0, sizeof(keybuf));
				for(i=0; i<sizeof(valbuf); i++) {
					if(valbuf[i] != '"')
						keybuf[j++] = valbuf[i];
				}
				strncpy(param->version1, keybuf, TOKEN_LEN);
				param->valid1=1;
				param->active1_ok=1;
				param->active1=1;
				param->commit1=1;
				break;
			}
		}
		fclose(fptr);
	}
#if 0
	dbprintf(LOG_ERR, "active0=%d, active1=%d\n", param->active0, param->active1);
	dbprintf(LOG_ERR, "commit0=%d, commit1=%d\n", param->commit0, param->commit1);
	dbprintf(LOG_ERR, "valid0=%d, valid1=%d\n", param->valid0, param->valid1);
	dbprintf(LOG_ERR, "active0_ok=%d, active1_ok=%d\n", param->active0_ok, param->active1_ok);
#endif
	return 0;
}

//9.1.4 Software image

static int 
meinfo_hw_7_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct omcimsg_sw_download_param_t param;
	struct me_t *private_me=0;

	meinfo_hw_parse_dump_env(&param);

	private_me=hwresource_public2private(me);
	if (private_me==NULL) {
		dbprintf(LOG_ERR, "Can't found related private me?\n");
		return -1;
	}

	//for(i=1; i<5; i++)
	//	dbprintf(LOG_ERR,"[%d]\n", util_attr_mask_get_bit(attr_mask, i));

	//init for image0
	if( me->instance_num == 0) {
		if (util_attr_mask_get_bit(attr_mask, 1)) {
			// get image version from image env only if the image version is not overridden in /etc/omci/custom_mib
			if (meinfo_config_get_str_from_custom_mib(me->classid, me->instance_num, 1) == NULL) {
				attr.ptr=malloc_safe(TOKEN_LEN);
				attr.data=0;
				if (meinfo_conv_string_to_attr(7, 1, &attr, param.version0) < 0 ) {
					dbprintf(LOG_ERR, "meinfo_conv_string_to_attr error!\n");
					return -1;
				}
				meinfo_me_attr_set(me, 1, &attr, 0);
				if (attr.ptr)
					free_safe(attr.ptr);
			}
		}

		if (util_attr_mask_get_bit(attr_mask, 2)) {
			attr.data=param.commit0; attr.ptr=NULL;
			meinfo_me_attr_set(me, 2, &attr, 0);
		}

		if (util_attr_mask_get_bit(attr_mask, 3)) {
			attr.data=param.active0; attr.ptr=NULL;
			meinfo_me_attr_set(me, 3, &attr, 0);
		}

		if (util_attr_mask_get_bit(attr_mask, 4)) {
			attr.data=param.valid0; attr.ptr=NULL;
			meinfo_me_attr_set(me, 4, &attr, 0);
		}

		attr.data=param.active0_ok; attr.ptr=NULL;
		meinfo_me_attr_set(private_me, 4, &attr, 0);       // attr 4: active0_ok
	}

	//init for image1
	if( me->instance_num == 1) {
		if (util_attr_mask_get_bit(attr_mask, 1)) {
			// get image version from image env only if the image version is not overridden in /etc/omci/custom_mib
			if (meinfo_config_get_str_from_custom_mib(me->classid, me->instance_num, 1) == NULL) {
				attr.ptr=malloc_safe(TOKEN_LEN);
				attr.data=0;
				if (meinfo_conv_string_to_attr(7, 1, &attr, param.version1) < 0 ) {
					dbprintf(LOG_ERR, "meinfo_conv_string_to_attr error!\n");
					return -1;
				}
				meinfo_me_attr_set(me, 1, &attr, 0);
				if (attr.ptr)
					free_safe(attr.ptr);
			}
		}

		if (util_attr_mask_get_bit(attr_mask, 2)) {
			attr.data=param.commit1; attr.ptr=NULL;
			meinfo_me_attr_set(me, 2, &attr, 0);
		}

		if (util_attr_mask_get_bit(attr_mask, 3)) {
			attr.data=param.active1; attr.ptr=NULL;
			meinfo_me_attr_set(me, 3, &attr, 0);
		}

		if (util_attr_mask_get_bit(attr_mask, 4)) {
			attr.data=param.valid1; attr.ptr=NULL;
			meinfo_me_attr_set(me, 4, &attr, 0);
		}

		attr.data=param.active1_ok; attr.ptr=NULL;
		meinfo_me_attr_set(private_me, 4, &attr, 0);       // attr 4: active1_ok
	}
	
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
	strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
		if( me->instance_num == 2 || me->instance_num == 3)
			meinfo_hw_parse_vendor_config(&param);
		
		if( me->instance_num == 2) {
			if (util_attr_mask_get_bit(attr_mask, 1)) {
				attr.ptr=malloc_safe(TOKEN_LEN);
				attr.data=0;
				if (meinfo_conv_string_to_attr(7, 1, &attr, param.version0) < 0 ) {
					dbprintf(LOG_ERR, "meinfo_conv_string_to_attr error!\n");
					return -1;
				}
				meinfo_me_attr_set(me, 1, &attr, 0);
				if (attr.ptr)
					free_safe(attr.ptr);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 2)) {
				attr.data=param.commit0; attr.ptr=NULL;
				meinfo_me_attr_set(me, 2, &attr, 0);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 3)) {
				attr.data=param.active0; attr.ptr=NULL;
				meinfo_me_attr_set(me, 3, &attr, 0);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 4)) {
				attr.data=param.valid0; attr.ptr=NULL;
				meinfo_me_attr_set(me, 4, &attr, 0);
			}
	
			attr.data=param.active0_ok; attr.ptr=NULL;
			meinfo_me_attr_set(private_me, 4, &attr, 0);       // attr 4: active0_ok
		}

		if( me->instance_num == 3) {
			if (util_attr_mask_get_bit(attr_mask, 1)) {
				attr.ptr=malloc_safe(TOKEN_LEN);
				attr.data=0;
				if (meinfo_conv_string_to_attr(7, 1, &attr, param.version1) < 0 ) {
					dbprintf(LOG_ERR, "meinfo_conv_string_to_attr error!\n");
					return -1;
				}
				meinfo_me_attr_set(me, 1, &attr, 0);
				if (attr.ptr)
					free_safe(attr.ptr);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 2)) {
				attr.data=param.commit1; attr.ptr=NULL;
				meinfo_me_attr_set(me, 2, &attr, 0);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 3)) {
				attr.data=param.active1; attr.ptr=NULL;
				meinfo_me_attr_set(me, 3, &attr, 0);
			}
	
			if (util_attr_mask_get_bit(attr_mask, 4)) {
				attr.data=param.valid1; attr.ptr=NULL;
				meinfo_me_attr_set(me, 4, &attr, 0);
			}
	
			attr.data=param.active1_ok; attr.ptr=NULL;
			meinfo_me_attr_set(private_me, 4, &attr, 0);       // attr 4: active1_ok
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_7 = {
	.get_hw = meinfo_hw_7_get
};

