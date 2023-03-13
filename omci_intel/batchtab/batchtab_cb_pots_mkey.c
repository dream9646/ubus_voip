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
 * Module  : batchtab
 * File    : batchtab_cb_pots_mkey.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "util_run.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "metacfg_adapter.h"
#include "cli.h"
#ifdef  X86
#define voipclient_cmd_set(X)	0
#else
#include "voipclient_cmd.h"
#endif

#ifdef OMCI_METAFILE_ENABLE
struct pots_info_t {
	struct metacfg_t kv;
};

static struct pots_info_t pots_info_g;
#endif

static int is_hw_sync=0;

////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
batchtab_cb_pots_init(void)
{
#ifdef OMCI_METAFILE_ENABLE
	memset(&pots_info_g.kv, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&(pots_info_g.kv));
#endif
	is_hw_sync=BATCHTAB_POTS_MKEY_INIT;
	return 0;
}

static int
batchtab_cb_pots_finish(void)
{
	is_hw_sync=BATCHTAB_POTS_MKEY_FINISH;
	return 0;
}

static int
batchtab_cb_pots_gen(void **table_data)
{
	is_hw_sync=BATCHTAB_POTS_MKEY_GEN_TODO;
#ifdef OMCI_METAFILE_ENABLE
	metacfg_adapter_config_release(&(pots_info_g.kv));
	memset(&pots_info_g.kv, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&(pots_info_g.kv));
	pots_mkey(0xffff, &(pots_info_g.kv), 0);
#endif
	is_hw_sync=BATCHTAB_POTS_MKEY_GEN_DONE;
	return 0;
}

static int
batchtab_cb_pots_release(void *table_data)
{
	if(!table_data)
	{
		return 0;
	}

	is_hw_sync=BATCHTAB_POTS_MKEY_RELEASE;
#ifdef OMCI_METAFILE_ENABLE
	metacfg_adapter_config_release(&(pots_info_g.kv));
#endif
	return 0;
}

static int
batchtab_cb_pots_dump(int fd, void *table_data)
{
#ifdef OMCI_METAFILE_ENABLE
	if (!metacfg_adapter_config_is_empty(&(pots_info_g.kv))) {
		metacfg_adapter_config_dump(fd, &(pots_info_g.kv), NULL);
	}
#endif
	return 0;
}

#ifdef OMCI_METAFILE_ENABLE
static int
_codec_digitmap_mkey_find (void *kv_config, const char *key, const char *value, void *data)
{
	int *ep_bit_map, bitmap_digitmap, bitmap_codec;
	ep_bit_map=(int *)data;
	bitmap_digitmap = *ep_bit_map & 0x3;
	bitmap_codec = (*ep_bit_map >> 2) & 0x3;
	dbprintf_bat(LOG_DEBUG, "key[%s] value[%s] ep_bit_map(0x%x) bitmap_digitmap(0x%x) _codec(0x%x)\n", (char *)key, (char *)value, *ep_bit_map, bitmap_digitmap, bitmap_codec);
	if (bitmap_digitmap == 3) {
		return 0;
	} else if (bitmap_codec == 3) {
		return 0;
	} else if (0 == strncmp(key, "voip_ep0_digitmap_", 18)) {
		bitmap_digitmap |= 1;
	} else if (0 == strncmp(key, "voip_ep1_digitmap_", 18)) {
		bitmap_digitmap |= 2;
	} else if (0 == strncmp(key, "voip_ep0_codec_list_", 20)) {
		bitmap_codec |= 1;
	} else if (0 == strncmp(key, "voip_ep1_codec_list_", 20)) {
		bitmap_codec |= 2;
	}
	*ep_bit_map |= bitmap_digitmap;
	*ep_bit_map |= (bitmap_codec << 2);
	dbprintf_bat(LOG_DEBUG, "key[%s] value[%s] ep_bit_map(0x%x) bitmap_digitmap(0x%x) _codec(0x%x)\n", (char *)key, (char *)value, *ep_bit_map, bitmap_digitmap, bitmap_codec);
	return 0;
}
#endif

static int
batchtab_cb_pots_hw_sync_handler(void *table_data)
{
#ifdef OMCI_METAFILE_ENABLE
	int i, ret, ep_bit_map=0;
	char filename[128], cmd[256];
	struct metacfg_t kv_metafile_now, kv_diff, kv_tmp, kv_tmp2;

	is_hw_sync=BATCHTAB_POTS_MKEY_HW_SYNC_TODO;
	if (!metacfg_adapter_config_is_empty(&(pots_info_g.kv))) {
		memset(&kv_metafile_now, 0x0, sizeof(struct metacfg_t));
		memset(&kv_diff, 0x0, sizeof(struct metacfg_t));
		memset(&kv_tmp, 0x0, sizeof(struct metacfg_t));
		memset(&kv_tmp2, 0x0, sizeof(struct metacfg_t));

		metacfg_adapter_config_init(&kv_metafile_now);
		metacfg_adapter_config_init(&kv_diff);
		metacfg_adapter_config_init(&kv_tmp);
		metacfg_adapter_config_init(&kv_tmp2);
		metacfg_adapter_config_file_load_safe(&kv_metafile_now, "/etc/wwwctrl/metafile.dat");

		// find out the kv_diff = &(pots_info_g.kv) - kv_metafile_now
		if (0 < (ret=metacfg_adapter_config_diff(&kv_metafile_now, &(pots_info_g.kv), &kv_diff, 0))) {
			metacfg_adapter_config_iterate(&kv_diff, _codec_digitmap_mkey_find, &ep_bit_map);
			dbprintf_bat(LOG_DEBUG, "kv_diff own (%d)mkeys ep_bit_map(0x%x)\n", ret, ep_bit_map);
			if (ep_bit_map & 0x3) {
				for (i=1; i<=100; i++) {
					snprintf(filename, 127, "voip_ep0_digitmap_%d", i);
					metacfg_adapter_config_entity_add(&kv_tmp, filename, metacfg_adapter_config_get_value(&kv_metafile_now, filename, 1), 1);
					if ((ep_bit_map & 1) && (NULL != metacfg_adapter_config_entity_locate(&kv_diff, filename))) {
						metacfg_adapter_config_entity_del(&kv_diff, filename);
					}
					snprintf(filename, 127, "voip_ep1_digitmap_%d", i);
					metacfg_adapter_config_entity_add(&kv_tmp, filename, metacfg_adapter_config_get_value(&kv_metafile_now, filename, 1), 1);
					if ((ep_bit_map & 2) && (NULL != metacfg_adapter_config_entity_locate(&kv_diff, filename))) {
						metacfg_adapter_config_entity_del(&kv_diff, filename);
					}
				}
				metacfg_adapter_config_merge(&kv_tmp, &(pots_info_g.kv), 0);
				if (ep_bit_map & 1) {
					/* /usr/bin/fvcli set digitmapdelall 0 */
					if (0 != voipclient_cmd_set("digitmapdelall 0")) {
						dbprintf_bat(LOG_ERR, "voipclient digitmapdelall 0 fail\n");
					} else {
						dbprintf_bat(LOG_DEBUG, "voipclient digitmapdelall 0 succ\n");
					}
				}
				if (ep_bit_map & 2) {
					/* /usr/bin/fvcli set digitmapdelall 1 */
					if (0 != voipclient_cmd_set("digitmapdelall 1")) {
						dbprintf_bat(LOG_ERR, "voipclient digitmapdelall 1 fail\n");
					} else {
						dbprintf_bat(LOG_DEBUG, "voipclient digitmapdelall 1 succ\n");
					}
				}
			}
			if (ep_bit_map & (0x3<<2)) {
				for (i=1; i<=12; i++) {
					snprintf(filename, 127, "voip_ep0_codec_list_%d", i);
					metacfg_adapter_config_entity_add(&kv_tmp2, filename, metacfg_adapter_config_get_value(&kv_metafile_now, filename, 1), 1);
					if ((ep_bit_map & (1<<2)) && (NULL != metacfg_adapter_config_entity_locate(&kv_diff, filename))) {
						metacfg_adapter_config_entity_del(&kv_diff, filename);
					}
					snprintf(filename, 127, "voip_ep1_codec_list_%d", i);
					metacfg_adapter_config_entity_add(&kv_tmp2, filename, metacfg_adapter_config_get_value(&kv_metafile_now, filename, 1), 1);
					if ((ep_bit_map & (2<<2)) && (NULL != metacfg_adapter_config_entity_locate(&kv_diff, filename))) {
						metacfg_adapter_config_entity_del(&kv_diff, filename);
					}
				}
				metacfg_adapter_config_merge(&kv_tmp2, &(pots_info_g.kv), 0);
				if (ep_bit_map & (0x1 << 2)) {
					/* /usr/bin/fvcli set codecremoveall 0 */
					if (0 != voipclient_cmd_set("codecremoveall 0")) {
						dbprintf_bat(LOG_ERR, "voipclient codecremoveall 0 fail\n");
					} else {
						dbprintf_bat(LOG_ERR, "voipclient codecremoveall 0 succ\n");
					}
				}
				if (ep_bit_map & (0x2 << 2)) {
					/* /usr/bin/fvcli set codecremoveall 1 */
					if (0 != voipclient_cmd_set("codecremoveall 1")) {
						dbprintf_bat(LOG_ERR, "voipclient codecremoveall 1 fail\n");
					} else {
						dbprintf_bat(LOG_ERR, "voipclient codecremoveall 1 succ\n");
					}
				}
			}
			i=metacfg_adapter_config_count(&kv_diff);
			dbprintf_bat(LOG_NOTICE, "kv_diff process mkeys (%d)=>(%d), ep_bit_map(0x%x)\n", ret, i, ep_bit_map);
			if (i > 0) {
				metacfg_adapter_config_dump(util_dbprintf_get_console_fd(), &kv_diff, NULL);
				snprintf(filename, 128, "/var/run/pots_mkey.diff.%s", util_uptime_str());
				metacfg_adapter_config_file_save_safe(&kv_diff, filename);
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 256, "/etc/init.d/cmd_dispatcher.sh diff %s; rm -f %s", filename, filename);
				util_run_by_thread(cmd, 0);
#endif
			}
			if (ep_bit_map & 0x3) {
				metacfg_adapter_config_dump_value(util_dbprintf_get_console_fd(), &kv_tmp, NULL, 1);
				snprintf(filename, 128, "/var/run/pots_mkey_digitmap.diff.%s", util_uptime_str());
				metacfg_adapter_config_file_save_safe(&kv_tmp, filename);
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 256, "/etc/init.d/cmd_dispatcher.sh diff %s; rm -f %s", filename, filename);
				util_run_by_thread(cmd, 0);
#endif
			}
			if (ep_bit_map & (0x3<<2)) {
				metacfg_adapter_config_dump_value(util_dbprintf_get_console_fd(), &kv_tmp2, NULL, 1);
				snprintf(filename, 128, "/var/run/pots_mkey_codeclist.diff.%s", util_uptime_str());
				metacfg_adapter_config_file_save_safe(&kv_tmp2, filename);
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 256, "/etc/init.d/cmd_dispatcher.sh diff %s; rm -f %s", filename, filename);
				util_run_by_thread(cmd, 0);
#endif
			}
		}
		metacfg_adapter_config_release(&kv_tmp);
		metacfg_adapter_config_release(&kv_tmp2);
		metacfg_adapter_config_release(&kv_metafile_now);
		metacfg_adapter_config_release(&kv_diff);
		metacfg_adapter_config_release(&(pots_info_g.kv));
		memset(&(pots_info_g.kv), 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&(pots_info_g.kv));
	} else {
		dbprintf_bat(LOG_DEBUG, "Nothing to apply cmd_dispatcher\n");
	}
#endif
	is_hw_sync=BATCHTAB_POTS_MKEY_HW_SYNC_DONE;
	return 0;
}

static int
batchtab_cb_pots_hw_sync(void *table_data)
{
	static int defer_count=0;

	/* table_data no used here */

	// dataasync ==0, system booting up, do nothing
	// so pots_mkey auto created before wont be deleted in every system bootup.
	// thus the user configuration on those pots_mkey could be preserved
	if (meinfo_util_get_data_sync() == 0) {
		dbprintf_bat(LOG_ERR, "meinfo_util_get_data_sync()=0\n");
		return 0;
	}
	// skip pots_mkey auto gen if mibreset till now < 20sec
	if (omcimsg_mib_reset_finish_time_till_now() < 20 || batchtab_cb_tagging_is_ready() == 0) {
		// keep the trigger for future and do nothing for now
		defer_count++;
		dbprintf_bat(LOG_WARNING, "DEFER pots_mkey hw_sync defer_count(%d)\n", defer_count);
		// do DEFER in hw_sync, call omci update to trigger future table_gen
		batchtab_omci_update("pots_mkey");
		return 0;
	} else {
		dbprintf_bat(LOG_ERR, "resume pots_mkey hw_sync defer_count(%d)\n", defer_count);
	}
	if (defer_count) {
		defer_count = 0;
	}
	return batchtab_cb_pots_hw_sync_handler(table_data);
}

int
batchtab_cb_pots_is_hw_sync(void)
{
	return is_hw_sync;
}

// public register function /////////////////////////////////////
int
batchtab_cb_pots_register(void)
{
	return batchtab_register("pots_mkey", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_pots_init,
			batchtab_cb_pots_finish,
			batchtab_cb_pots_gen,
			batchtab_cb_pots_release,
			batchtab_cb_pots_dump,
			batchtab_cb_pots_hw_sync);
}
