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
 * File    : meinfo_hw_65503.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/stat.h>
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"
#include "metacfg_adapter.h"

#define RG_HARD_RESTORE		0x1
#define VOIP_HARD_RESTORE	0x2
#define RG_SOFT_RESTORE		0x4

#define VOIP_CONF_PATH	"/var/run/voip/fv_voip.cfg"
#define METAFILE_PATH	"/etc/wwwctrl/metafile.dat"
#define METAFILE_DEFAULT	"/rom/etc/wwwctrl/metafile.dat"

static struct stat voipconf_timestamp_current, voipconf_timestamp_previous;
static struct stat metafile_timestamp_current, metafile_timestamp_previous;
#ifdef OMCI_METAFILE_ENABLE
static int is_loaded_config_rom = 0;
static struct metacfg_t kv_config_rom;
#endif

static int isSoftResetChanged_RG()
{
#ifdef OMCI_METAFILE_ENABLE
	// software reset, metafile has to be indentical except few preserved keys
	int ret = 0, i = 0, count = 0;
	struct metacfg_t kv_config_overlay, kv_meta_diff;
	char *exempt[] = { // TBD, excluding list
		"system_modelname",
		"system_hostname",
		"upnp_model_name",
		"cwmp_cpe_device_id",
		"dev_wan0_interface",
		"dev_wlan0_interface",
		"dev_wlan1_interface",
		"dev_wlan2_interface",
		"dev_wlan3_interface",
		"wlanac_enable",
		"dev_wlanac0_interface",
		"dev_wlanac1_interface",
		"dev_wlanac2_interface",
		"dev_wlanac3_interface",
		"wlanac_enable"};
	
	count = sizeof(exempt) / sizeof(char *);
	
	//load from file to hash list
	if(!is_loaded_config_rom) {
		is_loaded_config_rom = 1;
		memset(&kv_config_rom, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config_rom);
		if(metacfg_adapter_config_file_load_safe(&kv_config_rom, METAFILE_DEFAULT) !=0) {
			dbprintf(LOG_ERR, "load rom kv_config error\n");
			metacfg_adapter_config_release(&kv_config_rom);
			is_loaded_config_rom = 0;
			return 0;
		}
	}
	
	memset(&kv_config_overlay, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config_overlay);
	if(metacfg_adapter_config_file_load_safe(&kv_config_overlay, METAFILE_PATH) !=0) {
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config_overlay);
		return 0;
	}
	
	// generate a kv_diff
	memset(&kv_meta_diff, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_meta_diff);
	metacfg_adapter_config_diff(&kv_config_rom, &kv_config_overlay, &kv_meta_diff, KV_CONFIG_UPDATE_ONLY);
	//kv_config_dump(util_dbprintf_get_console_fd(), &kv_meta_diff, "");
	
	// remove keys in our exempt list
	for (i = 0; i < count;i++)
	{
		if (metacfg_adapter_config_entity_locate(&kv_meta_diff, exempt[i]))
			metacfg_adapter_config_entity_del(&kv_meta_diff, exempt[i]);
	}
	
	// check if metafile is still dirty
	if (!metacfg_adapter_config_is_empty(&kv_meta_diff)) ret = 1;
	dbprintf(LOG_DEBUG, "exempt size - %d, metafile dirty - %d\n", count, ret);
	
	//kv_config_release(&kv_config_rom); // always keep in ram to speed up comparison
	metacfg_adapter_config_release(&kv_meta_diff);
	metacfg_adapter_config_release(&kv_config_overlay);
	
	return ret;
#else
	return 0;
#endif
}

static int isHardResetChanged_RG()
{
	// Do not check whole metafile since it takes too much time, check timestamp instead
	// If modification time is at 1970/1/1 (tolerance 100 seconds), treat as default value
	return (metafile_timestamp_current.st_mtime < 100) ? 0 : 1;
}

static int isHardResetChanged_VOIP()
{
	int ret = 0;
	FILE *f;
	
	unlink("/tmp/voip_reset_status");
	util_run_by_system("/opt/wwwctrl/voip_config.sh status");
	f = fopen("/tmp/voip_reset_status", "r");
	if(f != NULL) {
		fscanf(f, "%d", &ret);
		fclose(f);
	}
	
	dbprintf(LOG_DEBUG, "voip differs to default - %d\n", ret);
	
	return ret;
}

static int isVoIPModel()
{
	struct meinfo_t *miptr= meinfo_util_miptr(53);
	if (miptr == NULL) return 0;
	
	return 1;
}

static int 
meinfo_hw_65503_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	int isVoIP = 0, avc = 0;
	FILE *fp = NULL;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}

	isVoIP = isVoIPModel();
	
	// Check current config timestamp
	fp = fopen(VOIP_CONF_PATH, "r");
	if(fp != NULL) {
		fclose(fp);
		stat(VOIP_CONF_PATH, &voipconf_timestamp_current);
	}
	fp = fopen(METAFILE_PATH, "r");
	if(fp != NULL) {
		fclose(fp);
		stat(METAFILE_PATH, &metafile_timestamp_current);
	}

	// 2: Status
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		meinfo_me_attr_get(me, 2, &attr);

		// check if ME changed, change status bit to 0 if it does
		if (isHardResetChanged_RG()) {
			avc = 1;
			attr.data &= (~RG_HARD_RESTORE);
		} else {
			avc = 0;
			attr.data |= RG_HARD_RESTORE;
		}

		// Check only when voipconf is changed, do not traverse voipconf too often
		if(isVoIP && voipconf_timestamp_current.st_mtime != voipconf_timestamp_previous.st_mtime) {
			if (isHardResetChanged_VOIP()) {
				avc = 1;
				attr.data &= (~VOIP_HARD_RESTORE);
			} else {
				avc = 0;
				attr.data |= VOIP_HARD_RESTORE;
			}
		}

		// Check only when metafile is changed, do not traverse metafile too often
		if(metafile_timestamp_current.st_mtime != metafile_timestamp_previous.st_mtime) {
			if (isSoftResetChanged_RG()) {
				avc =1;
				attr.data &= (~RG_SOFT_RESTORE);
			} else {
				avc = 0;
				attr.data |= RG_SOFT_RESTORE;
			}
		}

		if (avc)
		{
			avc = 1;
			dbprintf(LOG_DEBUG, "status changed 0x%llx\n", attr.data);
		}
		
		meinfo_me_attr_set(me, 2, &attr, avc);
	}

	// 3: Option
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		attr.ptr = NULL;
		attr.data = RG_HARD_RESTORE | RG_SOFT_RESTORE;
		if (isVoIP) attr.data |= VOIP_HARD_RESTORE;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	
	// Save current config timestamp to previous
	memcpy(&voipconf_timestamp_previous, &voipconf_timestamp_current, sizeof(struct stat));
	memcpy(&metafile_timestamp_previous, &metafile_timestamp_current, sizeof(struct stat));
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65503 = {
	.get_hw		= meinfo_hw_65503_get,
};
