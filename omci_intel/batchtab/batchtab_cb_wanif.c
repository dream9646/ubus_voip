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
 * File    : batchtab_cb_wan.c
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
#include "batchtab.h"
#include "batchtab_cb.h"
#include "meinfo.h"
#include "me_related.h"
//#include "hwresource.h"
#include "metacfg_adapter.h"
#include "iphost.h"
#include "wanif.h"
#include "omciutil_misc.h"
#include "omcimsg.h"

// proto defined in proprietary_tellion.h
int proprietary_tellion_get_nat_mode(void);
// proto defined in proprietary_zte.h
int proprietary_zte_get_wan_mode(struct me_t *);
// proto defined in proprietary_huawei.h
int proprietary_huawei_get_voip_wan(struct me_t *);

// submit action
// 1. generate submit_kv_diff based on me current attrs
// 2. save submit_kv_diff to submit_kv_diff_filename
// 3. call 'cmd_dispatcher.sh diff submit_kv_diff_filename'
// 4. we relies on the existance of submit_kv_filename to know if the submit is complete or not
//    the next submit will be issued only if previous submit_kv_diff_filename is removed

static struct batchtab_cb_wanif_t batchtab_cb_wanif_g;

#ifdef OMCI_METAFILE_ENABLE
// this is var cleared in per batchtab_cb_wanif_submit()
// which is used to find out what wnaif is created in per batchtab_cb_wanif_submit()
// and set to t->wanif_configured_after_mibreset[]
static unsigned int configured_bitmap_in_wanif_submit = 0;
#endif

static unsigned int is_me137_delete = 0;
// support routines ///////////////////////////////////////////////////////////

#ifdef OMCI_METAFILE_ENABLE
static int
omci_wanif_vid_locate(char *devname)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int vid, i;	
	if (devname) {
		vid = wanif_devname_vlan(devname);
		if (vid<0) vid = 0;
	} else {
		vid = -1;
	}
	for (i=0; i<32; i++) {
		if (t->omci_wanif_vid[i] == vid)
			return i;
	}
	return -1;
}
static int
omci_wanif_vid_add(char *devname)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int i;
	if ((i = omci_wanif_vid_locate(devname)) >=0)
		return i;
	if ((i = omci_wanif_vid_locate(NULL)) >=0) {
		int vid = wanif_devname_vlan(devname);
		if (vid <0) vid = 0;
		t->omci_wanif_vid[i] = vid;
		return i;
	}
	return -1;
}
static int
omci_wanif_vid_del(char *devname)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int i;
	if ((i = omci_wanif_vid_locate(devname)) >=0) {
		t->omci_wanif_vid[i] = -1;
		return i;
	}
	return -1;
}
#endif

static void
omci_wanif_vid_del_all(void) 
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int i;
	for (i=0; i<32; i++)
		t->omci_wanif_vid[i] = -1;
}

static void
clear_access_info_for_mibreset()
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int i;

	memset(t->wanif_configured_after_mibreset, 0, sizeof(t->wanif_configured_after_mibreset));
	omci_wanif_vid_del_all();

	// clear change_mask
	for (i = 0; i < IPHOST_TOTAL; i++) {
		t->change_mask_me134[i][0] = t->change_mask_me134[i][1] = 0;
	}
	for (i = 0; i < VEIP_TOTAL; i++) {
		t->change_mask_me329[i][0] = t->change_mask_me329[i][1] = 0;
	}
	for (i = 0; i < TR069_TOTAL; i++) {
		t->change_mask_me340[i][0] = t->change_mask_me340[i][1] = 0;
	}
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		for (i = 0; i < CALIX_RG_CONFIG_TOTAL; i++) {
			t->change_mask_me65317[i][0] = t->change_mask_me65317[i][1] = 0;
		}
	}
	t->me309_me310_is_updated = 0;
}

static int
tr069_print_status(int fd)
{
#if 1
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char *ifname;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	//kv_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat");
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", NULL, "cwmp_udp_conn_req_notify_limit");	// load from line 0 to the end of cwmp block

	util_fdprintf(fd, "cwmp_enable=%s\n", metacfg_adapter_config_get_value(&kv_config, "cwmp_enable", 1));
	ifname = metacfg_adapter_config_get_value(&kv_config, "cwmp_interface", 1);
	if(ifname) util_fdprintf(fd, "cwmp_interface=%s\n", metacfg_adapter_config_get_value(&kv_config, ifname, 1));
	else util_fdprintf(fd, "cmwp_interface=%s\n", metacfg_adapter_config_get_value(&kv_config, "cwmp_interface", 1));
	util_fdprintf(fd, "cwmp_acs_url=%s\n", metacfg_adapter_config_get_value(&kv_config, "cwmp_acs_url", 1));
	util_fdprintf(fd, "cwmp_acs_username=%s\n", metacfg_adapter_config_get_value(&kv_config, "cwmp_acs_username", 1));
	util_fdprintf(fd, "cwmp_acs_password=%s\n", metacfg_adapter_config_get_value(&kv_config, "cwmp_acs_password", 1));

	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;

#else
	struct kv_config_t kv_config;
	char *ifname;

	kv_config_init(&kv_config);
	//kv_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat");
	kv_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", NULL, "cwmp_udp_conn_req_notify_limit");	// load from line 0 to the end of cwmp block

	util_fdprintf(fd, "cwmp_enable=%s\n", kv_config_get_value(&kv_config, "cwmp_enable", 1));
	ifname = kv_config_get_value(&kv_config, "cwmp_interface", 1);
	if(ifname) util_fdprintf(fd, "cwmp_interface=%s\n", kv_config_get_value(&kv_config, ifname, 1));
	else util_fdprintf(fd, "cmwp_interface=%s\n", kv_config_get_value(&kv_config, "cwmp_interface", 1));
	util_fdprintf(fd, "cwmp_acs_url=%s\n", kv_config_get_value(&kv_config, "cwmp_acs_url", 1));
	util_fdprintf(fd, "cwmp_acs_username=%s\n", kv_config_get_value(&kv_config, "cwmp_acs_username", 1));
	util_fdprintf(fd, "cwmp_acs_password=%s\n", kv_config_get_value(&kv_config, "cwmp_acs_password", 1));

	kv_config_release(&kv_config);
	return 0;
#endif	
}

// wanif delete routines ///////////////////////////////////////////////////////////

// there are 4 types auto created wanif 
// a. iphost related, devname from iphost devname
//    ignored if iphost has no related bport
// b. veip related, devbase from veip devname, vid comes from me 65317 (calix_rg_config)
//    ignored if veip has no related bport
//    disabled if veip is admin-state
// c. veip related, devbase from veip devname, vid comes from me 340 (bbf_tr069)
//    ignored if veip has no related bport
//    disabled if bbf_tr069 is admin-state, veip is admin-state
// d. rgvlan related, devbase differs from iphost/veip, vid comes from rg->ani vlan translation table
//    ignored if veip has no related bport
//    disabled if veip is admin-state

// the main flow of wanif_submit()
// 1. all wanif would be deleted when mgmt_mode is changed between 0 and 1
// 2. call delete_auto_created_wanif_not_match_rg_tag_list() to remove
//    i.  auto-created iphost wanif not matched in iphost_rgtag_list[]
//    ii. auto-created calix_rg_config wanif, bbf_tr069 wanif, rgvlan wanif not matched in veip_rgtag_list[]
// 3. before creating per wanif, call delete_other_rgvlan_wanif_of_the_same_vlan() 
//    in case b & c to remove other wanif of same vid created in case d or created by user
// 4. delete_other_rgvlan_wanif_of_the_same_vlan() deletes wanif no matter if
//    it is auto-created by omci to ensure wanif of case b/c uniquely uses the vlan
// 5. the wanif created by case a/b/c/d will have auto-created=1
//    if the wanif exists before it is auto created, the wanif auto-created attr will be set to 1 also
//    this ensures the wanif could be auto deleted in step 2

#ifdef OMCI_METAFILE_ENABLE
static int
delete_auto_created_wanif_not_match_rg_tag_list(struct metacfg_t *kv_config)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int index;
	int deleted = 0;
	char key[128], value[128];

	key[127] = value[127] = 0;

	// del all veip/vlan wanif whose vid/pbit is not found in rg_tag_list[]
	for (index=0; index<WANIF_INDEX_TOTAL; index++) {
		char devname[33];
		struct me_t *me134, *me329, *me340;
		char *wanif_type_str = NULL;
		int issue_del = 0, is_deleted = 0;

		devname[32] = 0;
		snprintf(key, 127, "dev_wan%d_interface", index);
		// copy the dev_wanX_interface value to buffer, or it would be unavailable when keconfig is cleared
		strncpy(devname, metacfg_adapter_config_get_value(kv_config, key, 1), 32);
		if (strlen(devname) == 0)
			continue;

		// we delete not only rgvlan wanif but also veip related wanif here
		// as veip wanif vid was selected by reverting 65317 ani vlan back to rg vlan
		// so if veip wanif vid could not be found in vlan translation table, it should be deleted
		// ps: 134 is checked before 329 intensionally,
		//     so if a wanif basename used by both 134 and 329, it wont be deleted
		if ((me134 = omciutil_misc_get_classid_me_by_devname_base(134, devname)) != NULL) {
			char *iphost_devname = meinfo_util_get_config_devname(me134);
			if (strcmp(iphost_devname, devname) == 0) {
				wanif_type_str = "iphost";
				if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_DEL) {
					if (omciutil_misc_find_me_related_bport(me134)) {	
						if (omciutil_misc_get_rg_tag_list_index_by_devname_vlan(t->iphost_rg_tag_list, t->iphost_rg_tag_total, devname)<0)
							issue_del = 1;
					} else {
						issue_del = 1;
					}
				}					
			}
		}
		if (wanif_type_str == NULL && (me329 = omciutil_misc_get_classid_me_by_devname_base(329, devname)) != NULL) {
			int is_tr069_wanif = 0, is_calixrg_wanif = 0;
			int wanif_feature_del = 0;
			
			if ((me340 = omciutil_misc_get_classid_me_by_devname_base(340, devname)) != NULL) {
				unsigned char tr069_admin_state = (unsigned char)meinfo_util_me_attr_data(me340, 1);
				if (tr069_admin_state == 0)
					is_tr069_wanif = 1;
			}
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
			    omciutil_misc_get_classid_me_by_devname_base(65317, devname))
			    	is_calixrg_wanif = 1;
			 
			if (is_tr069_wanif && is_calixrg_wanif) {
				wanif_type_str = "calixrg/tr069";
				wanif_feature_del = omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_DEL;
			} else if (is_tr069_wanif) {
				wanif_type_str = "tr069";
				wanif_feature_del = omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_DEL;
			} else if (is_calixrg_wanif) {
				wanif_type_str = "calixrg";
				wanif_feature_del = 1;
			}
			if (wanif_type_str && wanif_feature_del) {
				unsigned char veip_admin_state = (unsigned char)meinfo_util_me_attr_data(me329, 1);	
				if (veip_admin_state == 0 && omciutil_misc_find_me_related_bport(me329)) {
					if (omciutil_misc_get_rg_tag_list_index_by_devname_vlan(t->veip_rg_tag_list, t->veip_rg_tag_total, devname)<0)
						issue_del = 1;
				} else {
					issue_del = 1;
				}
			}
		}
		if (wanif_type_str == NULL) {
			wanif_type_str = "rgvlan";
			if ((omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_DEL) &&
			    omciutil_misc_get_rg_tag_list_index_by_devname_vlan(t->veip_rg_tag_list, t->veip_rg_tag_total, devname)<0)
				issue_del = 1;
		}		
		if (issue_del) {
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_DEL_NONAUTO) {
				is_deleted = wanif_delete_by_index(kv_config, index);
			} else { 
				is_deleted = wanif_delete_auto_by_index(kv_config, index);
			}
			if (is_deleted) {
				omci_wanif_vid_del(devname);
				deleted++;
			}
		}
		dbprintf_bat(LOG_ERR, "%s %s wanif %s=%s\n", is_deleted?"delete":"keep", wanif_type_str, key, devname);
	}

	// del all brwan whose vid/pbit is not found in rg_tag_list[]
	for (index=0; index<BRWAN_INDEX_TOTAL; index++) {
		char *vid_str, *pbit_str;

		snprintf(key, 127, "brwan%d_enable", index);
		if (util_atoi(metacfg_adapter_config_get_value(kv_config, key, 1)) == 0)
			continue;

		snprintf(key, 127, "brwan%d_vid", index);
		vid_str = metacfg_adapter_config_get_value(kv_config, key, 1);
		snprintf(key, 127, "brwan%d_pbit", index);
		pbit_str = metacfg_adapter_config_get_value(kv_config, key, 1);

		if (strlen(vid_str) >0 && strlen(pbit_str) >0) {
			unsigned short vid = util_atoi(vid_str);
			unsigned char pbit = util_atoi(pbit_str);
			unsigned short rg_tag = vid | (pbit <<13);
			int tag_index = omciutil_misc_tag_list_lookup(rg_tag, t->veip_rg_tag_total, t->veip_rg_tag_list);
			if (tag_index < 0) {
				int is_deleted = 0;
				if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_BRWAN_DEL) {
					is_deleted = wanif_delete_brwan_by_index(kv_config, index);
					if (is_deleted)
						deleted++;
				}
				dbprintf_bat(LOG_ERR, "%s brwan%d vid=%u, pbit=%u\n", is_deleted?"delete":"keep", index, vid, pbit);
			}
		}
	}
	return deleted;
}

int
delete_other_rgvlan_wanif_of_the_same_vlan(char *wanif_devname, struct metacfg_t *kv_config)
{
	int rg_vlan = wanif_devname_vlan(wanif_devname);
	unsigned int indexmap = wanif_get_indexmap_by_vlan(kv_config, rg_vlan);
	int indexmap_del = 0, i;
	char key[128];
	char *devname;

	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		if (indexmap & (1<<i)) {
			snprintf(key, 128,  "dev_wan%d_interface", i);
			devname = metacfg_adapter_config_get_value(kv_config, key, 1);

			// skip the wanif_devname itself
			if (strcmp(wanif_devname, devname) == 0)
				continue;

			// skip veip/iphost related wanif
			if (omciutil_misc_get_classid_me_by_devname_base(134, devname))
				continue;
			if (omciutil_misc_get_classid_me_by_devname_base(329, devname))
				continue;					

			// del the wanif if it has same vlan as calix_rg_config/tr069 wanif
			omci_wanif_vid_del(devname);
			wanif_delete_by_index(kv_config, i);

			indexmap_del |= (1<<i);
		}
	}
	return indexmap_del;
}

static int
delete_all_wanif_brwan(struct metacfg_t *kv_config)
{
	int deleted = 0, index;

	for (index=0; index<WANIF_INDEX_TOTAL; index++)
		deleted += wanif_delete_by_index(kv_config, index);
	for (index=0; index<BRWAN_INDEX_TOTAL; index++)
		deleted += wanif_delete_brwan_by_index(kv_config, index);
	omci_wanif_vid_del_all();
	return deleted;
}

// wanif create routines : config_me, config_rgvlan... ///////////////////////////////////////////////////////////

#define IPOPT_DHCP 		(1<<0)
#define IPOPT_PING		(1<<1)
#define IPOPT_TRACEROUTE 	(1<<2)
#define IPOPT_IPSTACK 		(1<<3)

//*1: IP_options (1byte,unsigned integer,RW)
// 2: MAC_address (6byte,string-mac,R)
// 3: Ont_identifier (25byte,string,RW)
// 4: IP_address (4byte,unsigned integer-ipv4,RW)
// 5: Mask (4byte,unsigned integer-ipv4,RW)
// 6: Gateway (4byte,unsigned integer-ipv4,RW)
// 7: Primary_DNS (4byte,unsigned integer-ipv4,RW)
// 8: Secondary_DNS (4byte,unsigned integer-ipv4,RW)
// 9: Current_address (4byte,unsigned integer-ipv4,R)
//10: Current_mask (4byte,unsigned integer-ipv4,R)
//11: Current_gateway (4byte,unsigned integer-ipv4,R)
//12: Current_primary_DNS (4byte,unsigned integer-ipv4,R)
//13: Current_secondary_DNS (4byte,unsigned integer-ipv4,R)
//14: Domain_name (25byte,string,R)
//15: Host_name (25byte,string,R)
static int
config_me134(struct me_t *me, unsigned char attr_mask[2], struct metacfg_t *kv_config)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	char *me_devname = meinfo_util_get_config_devname(me);
	char found_devname[33];
	int wanif_index;
	char key[128], value[128];

	key[127] = value[127] = found_devname[32] = 0;

	if (me_devname == NULL)
		return -1;

	// ignore this me if the basedev of me_devname doesn't exist (because basedev could not be created by vconfig)
	if (iphost_interface_is_exist(wanif_devname_base(me_devname)) == 0) { // interface not exist
		dbprintf_bat(LOG_ERR, "iphost 0x%x(%s) is ignored because basedev %s doesn't exist\n", me->meid, me_devname, wanif_devname_base(me_devname));
		return 0;
	}

	wanif_index = wanif_locate_index_by_devname(kv_config, me_devname, found_devname, 32);
	if (!omciutil_misc_find_me_related_bport(me)) {	// disable iphost wanif if iphost me has no bport
		if (wanif_index >= 0) {
			snprintf(key, 127,  "wan%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			omci_wanif_vid_del(me_devname);
		}
		return 0;
	}

	if (wanif_index < 0) {
		// iphost interface will be always created no matter if it has been created then deleted before
		// ps: an iphost interface might be deleted becuase of me 134 attr1?
		//if (omci_wanif_vid_locate(me_devname)>=0)
		//	return 0;
		wanif_index = wanif_locate_index_by_devname_base(kv_config, me_devname, found_devname, 32);
		if (wanif_index >= 0) {
			dbprintf_bat(LOG_ERR, "iphost 0x%x devname %s found on wan%d(%s)\n", me->meid, me_devname, wanif_index, found_devname);
			omci_wanif_vid_del(found_devname);	// found_devname will be replaced which imples the found_devname deletion in cmd_dispatcher.conf
		} else {
			wanif_index = wanif_locate_index_unused(kv_config);
			if (wanif_index >= 0) {
				dbprintf_bat(LOG_ERR, "iphost 0x%x devname %s created on wan%d\n", me->meid, me_devname, wanif_index);
			} else {
				dbprintf_bat(LOG_ERR, "iphost 0x%x devname %s has no available wan port\n", me->meid, me_devname);
				return -1;
			}
		}
		t->wanif_configured_after_mibreset[wanif_index] = 0;
	}
	omci_wanif_vid_add(me_devname);

	// for attrs that need reset to default when wanif is created or reboot/mibreset happens
	// which means user/webui modification on these attr could be remained untill next boot/mibreset
	if (t->wanif_configured_after_mibreset[wanif_index] == 0) {
		configured_bitmap_in_wanif_submit |= (1<<wanif_index);

		snprintf(key, 127, "wan%d_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_ip_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_ipv6_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_TELLION) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_TELLION)) {
			int tellion_nat_mode = proprietary_tellion_get_nat_mode();
			snprintf(key, 127, "nat%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, (tellion_nat_mode) ? "1" : "0");
			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, (tellion_nat_mode) ? "1" : "0");
			snprintf(key, 127, "wan%d_pbit", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, (tellion_nat_mode) ? "7" : "0");
		} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
			int zte_wan_mode = proprietary_zte_get_wan_mode(me);
			if(zte_wan_mode) {
				struct me_t *meptr = meinfo_me_get_by_meid(250, me->meid);
				if(meptr != NULL) { // pppoe
					unsigned char nat_enable = (unsigned char)meinfo_util_me_attr_data(meptr, 1);
					unsigned char authtype = (unsigned char)meinfo_util_me_attr_data(meptr, 2);
					unsigned char connecttype = (unsigned char)meinfo_util_me_attr_data(meptr, 3);
					unsigned short maxidletime = (unsigned short)meinfo_util_me_attr_data(meptr, 4);
					char pppoe_username[26], pppoe_passwd[26], cmd[128], *output = NULL;
					struct attr_value_t attr;
					// NAT_enable
					snprintf(key, 127, "nat%d_enable", wanif_index);
					metacfg_adapter_config_entity_update(kv_config, key, (nat_enable) ? "1" : "0");
					// PPPOE_Mode
					snprintf(key, 127, "wan%d_pppoe_authtype", wanif_index);
					switch(authtype) {
						case 0: // auto
						default:
							metacfg_adapter_config_entity_update(kv_config, key, "0"); break;
						case 1: // chap
							metacfg_adapter_config_entity_update(kv_config, key, "2"); break;
						case 2: // pap
							metacfg_adapter_config_entity_update(kv_config, key, "1"); break;
					}
					// Connenction_Trigger
					snprintf(key, 127, "wan%d_pppoe_connecttype", wanif_index);
					metacfg_adapter_config_entity_update(kv_config, key, (connecttype) ? "1" : "0");
					// Release_timer
					if(maxidletime) {
						char release_timer[8];
						snprintf(release_timer, 8, "%d", maxidletime);
						snprintf(key, 127, "wan%d_pppoe_maxidletime", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, release_timer);
					}
					// PPPOE_username
					meinfo_me_attr_get(meptr, 5, &attr);
					memcpy(pppoe_username, attr.ptr, 25); pppoe_username[25] = 0;
					meinfo_me_attr_release(meptr->classid, 5, &attr);
					snprintf(key, 127, "wan%d_pppoe_username", wanif_index);
					metacfg_adapter_config_entity_update(kv_config, key, pppoe_username);
					// PPPOE_password
					meinfo_me_attr_get(meptr, 6, &attr);
					memcpy(pppoe_passwd, attr.ptr, 25); pppoe_passwd[25] = 0;
					meinfo_me_attr_release(meptr->classid, 6, &attr);
					snprintf(cmd, 128, "/usr/bin/rc4_base64 -e 27885vt8118 %s", pppoe_passwd);
					if (util_readcmd(cmd, &output) > 0) {	// output will be 'rc4_base64_encode=.....'
						char *eqptr = strchr(output, '=');
						if (eqptr) {
							snprintf(key, 127, "wan%d_pppoe_password", wanif_index);
							metacfg_adapter_config_entity_update(kv_config, key, util_trim(eqptr+1));
						}
					}
					free_safe(output);
				} else { // ipoe
					// NAT_enable
					snprintf(key, 127, "nat%d_enable", wanif_index);
					metacfg_adapter_config_entity_update(kv_config, key, "1");
				}
			} else {
				snprintf(key, 127, "nat%d_enable", wanif_index);
				metacfg_adapter_config_entity_update(kv_config, key, "0");
			}
			// reset mtu to default
			snprintf(key, 127, "wan%d_pppoe_mtu_select", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_pppoe_mtu", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1492");
			snprintf(key, 127, "wan%d_dhcp_mtu_select", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_dhcp_mtu", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1500");
			snprintf(key, 127, "wan%d_staticip_mtu_select", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_staticip_mtu", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1500");
			// disable cwmp iphost wanif
			snprintf(key, 127, "wan%d_service_cwmp", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			// disable igmp iphost wanif
			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_pbit", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
		} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) {
			int huawei_voip_wan = proprietary_huawei_get_voip_wan(me);
			snprintf(key, 127, "nat%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, (huawei_voip_wan) ? "0" : "1");
			// disable igmp iphost wanif
			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_pbit", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			if(!huawei_voip_wan) {
				snprintf(value, 127, "dev_wan%d_interface", wanif_index);
				metacfg_adapter_config_entity_update(kv_config, "system_default_route_interface", value);
			}
		} else {
			snprintf(key, 127, "nat%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			// disable igmp iphost wanif
			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			snprintf(key, 127, "wan%d_pbit", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
		}

		snprintf(key, 127,  "wan%d_if_name", wanif_index);
		snprintf(value, 127, "ipoe_iphost%d-%d", me->instance_num, wanif_devname_vlan(me_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

#if 0
		{ // if cwmp_interface or system_default_route_interface points to this newly created iphost wanif, show warning
			char *defwan_interface = kv_config_get_value(kv_config, "system_default_route_interface", 1);
			char *cwmp_interface = kv_config_get_value(kv_config, "cwmp_interface", 1);
			snprintf(value, 127,  "dev_wan%d_interface", wanif_index);
			if (strcmp(defwan_interface, value) == 0) {
				//kv_config_entity_update(kv_config, "system_default_route_interface", "");
				dbprintf_bat(LOG_ERR, "meta system_default_route_interface == newly created iphost!(%s)\n", value);
			}
			if (strcmp(cwmp_interface, value) == 0) {
				//kv_config_entity_update(kv_config, "cwmp_interface", "");
				dbprintf_bat(LOG_ERR, "meta cwmp_interface == newly created iphost!(%s)\n", value);
			}
		}		
#endif
	}

	// for attrs the will be always the same as default, not changable by user/webui
	{
		snprintf(key, 127,  "dev_wan%d_interface", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, me_devname);
		// always mark auto_created
		snprintf(key, 127, "wan%d_auto_created", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
	}
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_TELLION) == 0) &&
		(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_TELLION)) {
		int tellion_nat_mode = proprietary_tellion_get_nat_mode();
		snprintf(key, 127,  "wan%d_iphost_only", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, (tellion_nat_mode) ? "0" : "1");
	} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
		int zte_wan_mode = proprietary_zte_get_wan_mode(me);
		snprintf(key, 127,  "wan%d_iphost_only", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, (zte_wan_mode) ? "0" : "1");
	} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) {
		int huawei_voip_wan = proprietary_huawei_get_voip_wan(me);
		snprintf(key, 127,  "wan%d_iphost_only", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, (huawei_voip_wan) ? "1" : "0");
	} else {
		snprintf(key, 127,  "wan%d_iphost_only", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
	}

	{
		unsigned char ipopt = meinfo_util_me_attr_data(me, 1);

		// bit0: dhcp
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
			int zte_wan_mode = proprietary_zte_get_wan_mode(me);
			snprintf(key, 127,  "wan%d_ip_assignment", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key,  (zte_wan_mode==2) ? "2" : ((ipopt & IPOPT_DHCP) ? "1" : "0"));
		/*} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) {
			// pppoe wan workaround
			if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_HUAWEI) {
				int huawei_voip_wan = proprietary_huawei_get_voip_wan(me);
				snprintf(key, 127,  "wan%d_ip_assignment", wanif_index);
				kv_config_entity_update(kv_config, key,  (huawei_voip_wan) ? ((ipopt & IPOPT_DHCP) ? "1" : "0") : "2");
			}*/
		} else {
			snprintf(key, 127,  "wan%d_ip_assignment", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key,  (ipopt & IPOPT_DHCP)?"1":"0");
		}
		snprintf(key, 127,  "wan%d_ipv6_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key,  (ipopt & IPOPT_DHCP)?"1":"0");
		// bit1: response to ping
		snprintf(key, 127,  "fw_service_w%d_ping_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, (ipopt & IPOPT_PING)?"1":"0");
		// bit2: response to traceroute
		snprintf(key, 127,  "fw_service_w%d_traceroute_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, (ipopt & IPOPT_TRACEROUTE)?"1":"0");

		snprintf(key, 127,  "wan%d_enable", wanif_index);
#if 0	// most olt doesn't support this
		{
			static int iphost_was_up_at_least_once = 0;
			// bit3: ip stack enable
			if (ipopt & IPOPT_IPSTACK) {
				kv_config_entity_update(kv_config, key, "1");
				iphost_was_up_at_least_once = 1;
			} else { // support iphost down only if we are sure olt understand this flag
				if (iphost_was_up_at_least_once) {
					kv_config_entity_update(kv_config, key, "0");
					omci_wanif_vid_del(me_devname);
				}
			}
		}
#else
		metacfg_adapter_config_entity_update(kv_config, key, "1");
#endif

#if 0		// this part is replaced by filter.sh customer_rule_override_wan()
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// only the first instance (remote-diag) to enable telnet
			snprintf(key, 127,  "fw_service_w%d_telnet_enable", wanif_index);
			kv_config_entity_update(kv_config, key, (me->instance_num==0)?"1":"0");
		}
#endif

		if ((ipopt & IPOPT_DHCP) == 0) {	// static ip
			int ip_addr = htonl(meinfo_util_me_attr_data(me, 4));
			int netmask_addr = htonl(meinfo_util_me_attr_data(me, 5));
			int defgw_addr = htonl(meinfo_util_me_attr_data(me, 6));
			int dns1_addr = htonl(meinfo_util_me_attr_data(me, 7));
			int dns2_addr = htonl(meinfo_util_me_attr_data(me, 8));
			snprintf(key, 127,  "wan%d_ip", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(ip_addr));
			snprintf(key, 127,  "wan%d_netmask", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(netmask_addr));
			snprintf(key, 127,  "wan%d_gateway", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(defgw_addr));
			snprintf(key, 127,  "wan%d_dns1", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(dns1_addr));
			snprintf(key, 127,  "wan%d_dns2", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(dns2_addr));
		}
		
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
			// set wan mode (1:internet, 2:tr069, 4:voip, 8:other)
			int zte_wan_mode = proprietary_zte_get_wan_mode(me), igmp_proxy = 0;
			struct meinfo_t *miptr = meinfo_util_miptr(65321);
			struct me_t *meptr = NULL;
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
				unsigned char mode_mask = (unsigned char)meinfo_util_me_attr_data(meptr, 1);
				if(mode_mask&0x8) igmp_proxy = 1;
				if(me_related(meptr, me)) {
					unsigned short mtu = (unsigned short)meinfo_util_me_attr_data(meptr, 5);
					if(mtu) {
						if(zte_wan_mode==2) {
							snprintf(key, 127, "wan%d_pppoe_mtu_select", wanif_index);
							metacfg_adapter_config_entity_update(kv_config, key, "1");
							snprintf(key, 127, "wan%d_pppoe_mtu", wanif_index);
							snprintf(value, 127, "%d", mtu);
							metacfg_adapter_config_entity_update(kv_config, key, value);
						} else if(ipopt & IPOPT_DHCP) {
							snprintf(key, 127, "wan%d_dhcp_mtu_select", wanif_index);
							metacfg_adapter_config_entity_update(kv_config, key, "1");
							snprintf(key, 127, "wan%d_dhcp_mtu", wanif_index);
							snprintf(value, 127, "%d", mtu);
							metacfg_adapter_config_entity_update(kv_config, key, value);
						} else {
							snprintf(key, 127, "wan%d_staticip_mtu_select", wanif_index);
							metacfg_adapter_config_entity_update(kv_config, key, "1");
							snprintf(key, 127, "wan%d_staticip_mtu", wanif_index);
							snprintf(value, 127, "%d", mtu);
							metacfg_adapter_config_entity_update(kv_config, key, value);
						}
					} else {
						snprintf(key, 127, "wan%d_pppoe_mtu_select", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "0");
						snprintf(key, 127, "wan%d_pppoe_mtu", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "1492");
						snprintf(key, 127, "wan%d_dhcp_mtu_select", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "0");
						snprintf(key, 127, "wan%d_dhcp_mtu", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "1500");
						snprintf(key, 127, "wan%d_staticip_mtu_select", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "0");
						snprintf(key, 127, "wan%d_staticip_mtu", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "1500");
					}
#if 1
					// it may exist multiple wan with internet enabled
					if(mode_mask&0x1) {
						snprintf(value, 127, "dev_wan%d_interface", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, "system_default_route_interface", value);
					}
#endif
					if(mode_mask&0x2) {
						snprintf(key, 127, "wan%d_service_cwmp", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "1");
					} else {
						snprintf(key, 127, "wan%d_service_cwmp", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "0");
					}
					if(mode_mask&0x4) {
						snprintf(value, 127, "%s", me_devname);
						metacfg_adapter_config_entity_update(kv_config, "voip_ep0_interface_name", value);
						metacfg_adapter_config_entity_update(kv_config, "voip_ep1_interface_name", value);
					}
					if(mode_mask&0x8) {
						unsigned short mvlan = (unsigned short)meinfo_util_me_attr_data(meptr, 4);
						if(mvlan != 0xffff) {
							snprintf(value, 127, "echo vmap add %d 8 %d > /proc/net/vlan/config", mvlan, wanif_devname_vlan(me_devname));
							util_run_by_system(value);
						}
						// enable igmp iphost wanif
						snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "1");
					} else {
						unsigned short mvlan = (unsigned short)meinfo_util_me_attr_data(meptr, 4);
						if(mvlan != 0xffff) {
							snprintf(value, 127, "echo vmap del %d 8 %d > /proc/net/vlan/config", mvlan, wanif_devname_vlan(me_devname));
							util_run_by_system(value);
						}
						// disable igmp iphost wanif
						snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
						metacfg_adapter_config_entity_update(kv_config, key, "0");
					}
				}
			}
#if 0 // leave cwmp or default route to other place to check
			if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE) {
				miptr = meinfo_util_miptr(329);
				list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
					if(me_related(meptr, me)) {
#if 1
						// veip related iphost refers for tr069
						snprintf(key, 127, "wan%d_service_cwmp", wanif_index);
						kv_config_entity_update(kv_config, key, "1");
#else
						// set to default route if this iphost related to veip
						snprintf(value, 127, "dev_wan%d_interface", wanif_index);
						kv_config_entity_update(kv_config, "system_default_route_interface", value);
#endif
						break;
					}
				}
			}
#endif
			if(igmp_proxy) {
				// enable igmp proxy
				metacfg_adapter_config_entity_update(kv_config, "igmp_function", "proxy");
			} else {
				// enable igmp snoopy
				metacfg_adapter_config_entity_update(kv_config, "igmp_function", "snoopy");
			}
		}
	}

	return 0;
}

// 1: Administrative_state (1byte,unsigned integer,RW)
// 2: ACS_network_address (2byte,pointer,RW) [classid 137]
// 3: Associated_tag (2byte,bit field,RW)
// 3.0: TCI_pbit (3bit,unsigned integer)
// 3.1: TCI_cfi (1bit,enumeration)
// 3.2: TCI_vid (12bit,unsigned integer)
static int
config_me340_acs_info(struct me_t *me, struct metacfg_t *kv_config)
{
	struct me_t *me_137;
	char value[128];

	// ME137, get acs[1]
	if ((me_137 = meinfo_me_get_by_meid(137, (unsigned short)meinfo_util_me_attr_data(me, 2))) != NULL)
	{
		struct me_t *meptr;
		// ME148, get username1/password/username2
		if ((meptr = meinfo_me_get_by_meid(148, ((unsigned short)meinfo_util_me_attr_data(me_137, 1)))) != NULL)
		{
			struct attr_value_t attr1, attr2;
			unsigned char buf1[26], buf2[26];

			memset(buf1, 0x00, sizeof(buf1));
			attr1.ptr = buf1;
			meinfo_me_attr_get(meptr, 2, &attr1);
			memset(buf2, 0x00, sizeof(buf2));
			attr2.ptr = buf2;
			meinfo_me_attr_get(meptr, 5, &attr2);
			if(strlen(buf1)==25 && strlen(buf1) != 0)
				snprintf(value, 127,  "%s%s", buf1, buf2);
			else
				snprintf(value, 127,  "%s", buf1);
			if (value[0]) metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_username", value);
			// not clear cwmp_acs_username while value is null for compatible Calix V1

			memset(buf1, 0x00, sizeof(buf1));
			attr1.ptr = buf1;
			meinfo_me_attr_get(meptr, 3, &attr1);
			if (buf1[0]) metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_password", buf1);
			// not clear cwmp_acs_password while value is null for compatible Calix V1
		}

		// ME157, get parts/part_*
		if ((meptr = meinfo_me_get_by_meid(157, ((unsigned short)meinfo_util_me_attr_data(me_137,2)))) != NULL)
		{
			struct attr_value_t attr;
			int i = 0, parts = 0;
			unsigned char buf[26], acs[512];

			memset(acs, 0x00, sizeof(acs));
			parts = (unsigned char) meinfo_util_me_attr_data(meptr, 1);
			for(i=0; i<parts; i++) {
				memset(buf, 0x00, sizeof(buf));
				attr.ptr = buf;
				meinfo_me_attr_get(meptr, 2+i, &attr);
				strcat(acs, buf);
			}
			if (acs[0]) {
				dbprintf_bat(LOG_ERR, "To set metakey cwmp_enable=1, cwmp_acs_url=%s\n", acs);
				metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_url", acs);
				// acs url is not null, enable cwmp
				metacfg_adapter_config_entity_update(kv_config, "cwmp_enable", "1");
			}
			// not clear cwmp_acs_url and cwmp_enable while acs is null for compatible Calix V1
		}
		return 0;
	}
	return -1;
}
static int
config_me340_wanif(struct me_t *me, struct metacfg_t *kv_config)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	struct me_t *veip_me;
	unsigned char veip_admin_state = 0;
	unsigned char *tr069_tci_ptr;
	int tr069_is_standalone_vlan = 0;	// whether tr069 uses vlan other than the one used in calix rg config (me65317)

	char tr069_devname[33], found_devname[33];
	int i, tr069_index = -1, tr069_tci = -1, tr069_vlan = -1, tr069_pbit = -1, tr069_rg_pbit = 0;
	char key[128], value[128];

	tr069_devname[32] = found_devname[32] = key[127] = value[127] = 0;

	tr069_tci_ptr = (unsigned char*)meinfo_util_me_attr_ptr(me, 3);
	tr069_tci = (unsigned short) util_bitmap_get_value(tr069_tci_ptr, 16, 0, 16);
	tr069_vlan = (unsigned short) util_bitmap_get_value(tr069_tci_ptr, 16, 4, 12);
	tr069_pbit = (unsigned char) util_bitmap_get_value(tr069_tci_ptr, 16, 0, 3);

	// veip must be found for the tr069 devname
	if ((veip_me = meinfo_me_get_by_meid(329, (unsigned short)me->meid)) == NULL)
		return -1;
	veip_admin_state = meinfo_util_me_attr_data(veip_me, 1);

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		struct me_t *calix_rg_config_me = meinfo_me_get_by_meid(65317, me->meid);
		unsigned short wan_vlan = (unsigned short)meinfo_util_me_attr_data(calix_rg_config_me, 8);	// this is ani vlan
		if ((tr069_tci == 0xffff && (wan_vlan & 0xfff) != 0) ||
		    (tr069_tci != 0xffff && (wan_vlan & 0xfff) != tr069_vlan)) {
			dbprintf_bat(LOG_ERR, "tr069-bbf 0x%x is Out-of-Band because its vlan %d differs than the calix_rg_config 0x%x vlan %d\n",
				me->meid, tr069_tci& 0xfff, me->meid, wan_vlan&0xfff);
			tr069_is_standalone_vlan = 1;
		}
	}
	if (tr069_is_standalone_vlan) {		
		int rg_tag = omciutil_misc_get_veip_rg_tag_by_ani_tag(veip_me, tr069_tci);
		if (rg_tag < 0) {
			dbprintf_bat(LOG_ERR, "tr069-bbf 0x%x (Out-of-band) is ignored because vlan %d not found in ani side\n", me->meid, tr069_vlan);
 			return -1;
		}
		if (wanif_locate_index_by_vlan(kv_config, rg_tag&0xfff, tr069_devname, 33) < 0) {
			int devname_index = omciutil_misc_locate_wan_new_devname_index(kv_config, "pon");
			if (devname_index < 0) {
				dbprintf_bat(LOG_ERR, "tr069-bbf 0x%x (Out-of-band) can not alloc newdevname from pon[0-7]\n", me->meid);
				return -1;
			}
			if (tr069_tci == 0xffff) {
				snprintf(tr069_devname, 32, "pon%d", devname_index);
			} else {
				snprintf(tr069_devname, 32, "pon%d.%d", devname_index, tr069_tci&0xfff);
			}
		}
		tr069_rg_pbit =  (rg_tag>>13) & 0x7;
	} else {
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE)) {
			if (tr069_tci == 0xffff) {
				// veip->iphost
				struct me_t *veip_me = meinfo_me_get_by_instance_num(329, 0);
				if(veip_me) {
					struct meinfo_t *miptr = meinfo_util_miptr(134);
					struct me_t *meptr = NULL;
					list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
						if(me_related(veip_me, meptr)) {
							char *me_devname = meinfo_util_get_config_devname(meptr);
							tr069_index = wanif_locate_index_by_devname(kv_config, me_devname, tr069_devname, 32);
							break;
						}
					}
				}
			}
		} else {
			char *veip_devname = meinfo_util_get_config_devname(veip_me);
			if (veip_devname == NULL || strlen(veip_devname) == 0) {
				dbprintf_bat(LOG_ERR, "tr069-bbf 0x%x veip 0x%x devname %s invalid \n", me->meid, veip_me->meid, veip_devname?veip_devname:"?");
	 			return -1;
	 		}
			if (tr069_tci == 0xffff) {
				snprintf(tr069_devname, 32, "%s", wanif_devname_base(veip_devname));
			} else {
				tr069_rg_pbit = omciutil_misc_get_devname_by_veip_ani_tag(veip_me, tr069_vlan|(tr069_pbit<<13), tr069_devname, 33);
				if (tr069_rg_pbit < 0) {
					dbprintf_bat(LOG_ERR, "tr069-bbf 0x%x is ignored because vlan %d not found in ani side\n", me->meid, tr069_vlan);
					return -1;
				}
	 		}
	 	}
	}

	{ // remove auto created rg vlan wanif that has same vid as this tr069 wanif
		int indexmap_del = delete_other_rgvlan_wanif_of_the_same_vlan(tr069_devname, kv_config);
		if (indexmap_del) {
			dbprintf_bat(LOG_ERR, "tr069_bbf 0x%x devname %s del rgvlan wanif, indexmap = 0x%x\n",
				veip_me->meid, tr069_devname, indexmap_del);
		}
	}

	tr069_index = wanif_locate_index_by_devname(kv_config, tr069_devname, found_devname, 32);
	dbprintf_bat(LOG_ERR, "tr069_index=%d\n", tr069_index);
	if (!omciutil_misc_find_me_related_bport(veip_me)) {	// disable tr069 wanif if veip me has no bport
		if (tr069_index >= 0) {	// wont happen actually, the wanif should be deleted by delete_auto_created_wanif_not_match_rg_tag_list() already
			snprintf(key, 127,  "wan%d_enable", tr069_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			omci_wanif_vid_del(tr069_devname);
		}
		return 0;
	}

	if (tr069_index < 0) {
		if (omci_wanif_vid_locate(tr069_devname)>=0)
			return 0;
		tr069_index = wanif_locate_index_by_devname_base(kv_config, tr069_devname, found_devname, 32);
		dbprintf_bat(LOG_ERR, "tr069_index=%d\n", tr069_index);
		if (tr069_index >= 0) {
			dbprintf_bat(LOG_ERR, "tr069_bbf 0x%x devname %s found on wan%d(%s)\n", 
				me->meid, tr069_devname, tr069_index, found_devname);
			omci_wanif_vid_del(found_devname);	// found_devname will be replaced which imples the found_devname deletion in cmd_dispatcher.conf
		} else {
			tr069_index = wanif_locate_index_unused(kv_config);
			if (tr069_index >= 0) {
				// mark auto_created only if a wanif is newly created
				snprintf(key, 127,  "wan%d_auto_created", tr069_index);
				metacfg_adapter_config_entity_update(kv_config, key, "1");
				dbprintf_bat(LOG_ERR, "tr069_bbf 0x%x devname %s created on wan%d\n", me->meid, tr069_devname, tr069_index);
			} else {
				dbprintf_bat(LOG_ERR, "tr069_bbf 0x%x devname %s has no available wan slot\n", me->meid, tr069_devname);
				return -1;
			}
		}
		t->wanif_configured_after_mibreset[tr069_index] = 0;
	}
	omci_wanif_vid_add(tr069_devname);

	// for attrs that need reset to default when wanif is created or reboot/mibreset happens
	// which means user/webui modification on these attr could be remained untill next boot/mibreset
	if (t->wanif_configured_after_mibreset[tr069_index] == 0) {
		configured_bitmap_in_wanif_submit |= (1<<tr069_index);

		snprintf(key, 127,  "wan%d_if_name", tr069_index);
		if (veip_me)
			snprintf(value, 127, "ipoe_veip%d-%d", veip_me->instance_num, wanif_devname_vlan(tr069_devname));
		else
			snprintf(value, 127, "ipoe_veip?-%d", wanif_devname_vlan(tr069_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		// disable tr069 wanif if veip me is under admin
		snprintf(key, 127, "wan%d_enable", tr069_index);
		if (veip_admin_state == 0) {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			omci_wanif_vid_del(tr069_devname);
		}

		// nat for bbf-tr069 wanif
		snprintf(key, 127, "nat%d_enable", tr069_index);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_IS_IPHOST) {
			metacfg_adapter_config_entity_update(kv_config, key, "0");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		}

		snprintf(key, 127, "wan%d_igmp_enable", tr069_index);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
			// disable wanif igmp for calix when no us igmp tci matches rg ani vlan
			if (omciutil_misc_tag_list_lookup(tr069_vlan|(tr069_pbit<<13), t->us_tci_total, t->us_tci_list) >= 0)
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			else
				metacfg_adapter_config_entity_update(kv_config, key, "0");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		}

		snprintf(key, 127,  "wan%d_pbit", tr069_index);
		snprintf(value, 127,  "%d", tr069_rg_pbit);
		metacfg_adapter_config_entity_update(kv_config, key, value);
	}

	if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
		// enable wanif igmp if ani vlan matches igmp us tci
		if (t->me309_me310_is_updated) {
			snprintf(key, 127, "wan%d_igmp_enable", tr069_index);
			if (omciutil_misc_tag_list_lookup(tr069_vlan|(tr069_pbit<<13), t->us_tci_total, t->us_tci_list) >= 0)
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			else
				metacfg_adapter_config_entity_update(kv_config, key, "0");
		}
	}

	// for attrs the will be always the same as default, not changable by user/webui
	{
		snprintf(key, 127,  "dev_wan%d_interface", tr069_index);
		metacfg_adapter_config_entity_update(kv_config, key, tr069_devname);
		// always mark auto_created
		snprintf(key, 127, "wan%d_auto_created", tr069_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		// iphost only for tr069
		snprintf(key, 127, "wan%d_iphost_only", tr069_index);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_IS_IPHOST) {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "0");
		}
	}

	// for attrs that are tr069 specific and should be sync back to metafile
	{
		// set bbf-tr069 wanif as cwmp interface
		snprintf(value, 127,  "dev_wan%d_interface", tr069_index);
		metacfg_adapter_config_entity_update(kv_config, "cwmp_interface", value);
		// always set cwmp_enable to 1
		metacfg_adapter_config_entity_update(kv_config, "cwmp_enable", "1");
		// enable cwmp service on specific wanif
		for (i=0; i<WANIF_INDEX_TOTAL; i++) {
			snprintf(key, 127,  "wan%d_service_cwmp", i);
			metacfg_adapter_config_entity_update(kv_config, key, (i == tr069_index) ? "1" : "0");
		}
	}
	
	return 0;
}
static int
config_me340(struct me_t *me, struct metacfg_t *kv_config)
{
	unsigned char tr069_admin_state = 0;
	int mgmt_mode = 0;

	if (me == NULL)
		return -1;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		struct me_t *me65317_found = NULL;
		mgmt_mode = batchtab_cb_wanif_get_calix_mgmt_mode(&me65317_found);
		if (mgmt_mode <0)	// me 65317 not found
			return 0;
	}
	tr069_admin_state = (unsigned char)meinfo_util_me_attr_data(me, 1);
	// for zte case, check acs existence to decide admin_state
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) && 
		(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE))
		tr069_admin_state = (meinfo_me_get_by_meid(137, (unsigned short)meinfo_util_me_attr_data(me, 2)) != NULL) ? 0 : 1;
	dbprintf_bat(LOG_ERR, "mgmt_mode=%d, tr069_admin_state=%d\n", mgmt_mode, tr069_admin_state);
	if(tr069_admin_state) {	// the me is under admin
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_DEL) {
			// turn off cwmp_interface/cwmp_enable if all tr069 me are under admin
			struct meinfo_t *miptr340=meinfo_util_miptr(340);
			struct me_t *me340;
			int is_all_me340_under_admin = 1;
			list_for_each_entry(me340, &miptr340->me_instance_list, instance_node) {
				if ((unsigned char)meinfo_util_me_attr_data(me340, 1) == 0)
					is_all_me340_under_admin = 0;
			}
			dbprintf_bat(LOG_ERR, "is_all_me340_under_admin=%d\n", is_all_me340_under_admin);
			if (is_all_me340_under_admin) {
				// clear cwmp_interface only if in native mode
				// in external mode, cwmp_interface is assigned by gloden config, omci should not clear it
				if (mgmt_mode == 0) {	
					dbprintf_bat(LOG_ERR, "all tr069-bbf are under admin, clear cwmp_interface\n");
					metacfg_adapter_config_entity_update(kv_config, "cwmp_interface", "");
				}
				if (mgmt_mode == 0 ||
				    (mgmt_mode == 1 && is_me137_delete == 1)) {
					dbprintf_bat(LOG_ERR, "all tr069-bbf are under admin, mgmt_mode=%d is_me137_delete=%d: cwmp_enable=0, clear cwmp_acs_[url,username,password]\n", mgmt_mode, is_me137_delete);
					metacfg_adapter_config_entity_update(kv_config, "cwmp_enable", "0");
					metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_url", "");
					metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_username", "");
					metacfg_adapter_config_entity_update(kv_config, "cwmp_acs_password", "");
				}
			}
		}
		return 0;
	}

	// at least one me340 is not under admin
	// always config cwmp_enable, cwmp_acs_username, cwmp_acs_password, cwmp_acs_url 
	// no matter if in mgmt_mode or not, not matter if cwmp service/wanif is available or not
	config_me340_acs_info(me, kv_config);

	if (mgmt_mode == 0 && (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_ADD)) {
		config_me340_wanif(me, kv_config);
	}
	return 0;
}	

//*1: MgmtMode (1byte,enumeration,RW)
//*2: NativeWanProtocol (1byte,enumeration,RW)
// 3: NativeIpAddress (4byte,unsigned integer-ipv4,RW)
// 4: NativeMask (4byte,unsigned integer-ipv4,RW)
// 5: NativeGateway (4byte,unsigned integer-ipv4,RW)
//*6: NativePppAuthentication (2byte,pointer,RW) [classid 148]
// 7: RestoreDefaults (1byte,unsigned integer,W)
//*8: NativeWanVlan (2byte,unsigned integer,RW)
// 9: RemoteGuiAccessTime (4byte,unsigned integer,RW)
//10: NativePriDns (4byte,unsigned integer-ipv4,RW)
//11: NativeSecDns (4byte,unsigned integer-ipv4,RW)
//12: WanStatus (25byte,table,R)
//13: RegistrationId (10byte,string,RW)
static int
config_me65317(struct me_t *me, unsigned char attr_mask[2], struct metacfg_t *kv_config)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int wanif_index;
	int wan_rg_pbit = 0;
	char found_devname[33], calix_rg_devname[33];
	char key[128], value[128];

	unsigned short veip_meid = me->meid;   // veip has same meid as 65317
	unsigned char mgmt_mode = (unsigned char)meinfo_util_me_attr_data(me, 1);	// 0: native (omci), 1: external (tr069 or other)
	unsigned char wan_proto = (unsigned char)meinfo_util_me_attr_data(me, 2);	// 0: dhcp, 1:static, 2:pppoe, 3: bridged
	unsigned short wan_vlan = (unsigned short)meinfo_util_me_attr_data(me, 8);	// this is ani vlan

	struct me_t *veip_me;
	unsigned char veip_admin_state;

	if (mgmt_mode == 1)	// 0: native (omci), 1: external (other than omci)
		return 0;	// do nothing, the stale calix rg config wanif will be removed by calix_delete_all_wanif()

	found_devname[32] = calix_rg_devname[32] = key[127] = value[127] = 0;

	if ((veip_me=meinfo_me_get_by_meid(329, veip_meid)) == NULL)
		return 0;
	veip_admin_state = meinfo_util_me_attr_data(veip_me, 1);

	wan_rg_pbit = omciutil_misc_get_devname_by_veip_ani_tag(veip_me, wan_vlan, calix_rg_devname, 33);
	if (wan_rg_pbit <0) {
		char *veip_devname = meinfo_util_get_config_devname(veip_me);
		dbprintf_bat(LOG_ERR, "calix rg is ignored because veip 0x%x devname %s invalid or calix vlan %d not found in ani-side\n",
			veip_me->meid, veip_devname?veip_devname:"?", wan_vlan&0xfff);
		return 0;
	}
	{ // remove auto created rg vlan wanif that has same vid as this calix_rg_config wanif
		int indexmap_del = delete_other_rgvlan_wanif_of_the_same_vlan(calix_rg_devname, kv_config);
		if (indexmap_del) {
			dbprintf_bat(LOG_ERR, "calix_rg_config 0x%x devname %s del rgvlan wanif, indexmap = 0x%x\n",
				veip_me->meid, calix_rg_devname, indexmap_del);
		}
	}

	wanif_index = wanif_locate_index_by_devname(kv_config, calix_rg_devname, found_devname, 32);
	if (!omciutil_misc_find_me_related_bport(veip_me)) {	// disable calix_rg wanif if veip me has no bport
		if (wanif_index >= 0) {	// wont happen actually, the wanif should be deleted by delete_auto_created_wanif_not_match_rg_tag_list() already
			snprintf(key, 127,  "wan%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			omci_wanif_vid_del(calix_rg_devname);
		}
		return 0;
	}
	if (wanif_index < 0) {
		if (omci_wanif_vid_locate(calix_rg_devname)>=0)
			return 0;
		wanif_index = wanif_locate_index_by_devname_base(kv_config, calix_rg_devname, found_devname, 32);
		if (wanif_index >= 0) {
			dbprintf_bat(LOG_ERR, "calix_rg_config 0x%x devname %s found on wan%d(%s)\n", veip_me->meid, calix_rg_devname, wanif_index, found_devname);
			omci_wanif_vid_del(found_devname);	// found_devname will be replaced which imples the found_devname deletion in cmd_dispatcher.conf			
		} else {
			wanif_index = wanif_locate_index_unused(kv_config);
			if (wanif_index >= 0) {
				dbprintf_bat(LOG_ERR, "calix_rg_config 0x%x devname %s pbit%d created on wan%d\n", veip_me->meid, calix_rg_devname, wan_rg_pbit, wanif_index);
			} else {
				dbprintf_bat(LOG_ERR, "calix_rg_config 0x%x devname %s has no available wan port\n", veip_me->meid, calix_rg_devname);
				return -1;
			}
		}
		t->wanif_configured_after_mibreset[wanif_index] = 0;
	}
	omci_wanif_vid_add(calix_rg_devname);

	// for attrs that need reset to default when wanif is created or reboot/mibreset happens
	// which means user/webui modification on these attr could be remained untill next boot/mibreset
	if (t->wanif_configured_after_mibreset[wanif_index] == 0) {
		configured_bitmap_in_wanif_submit |= (1<<wanif_index);

		// set calix rg config wanif as system default route interface
		snprintf(value, 127, "dev_wan%d_interface", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, "system_default_route_interface", value);
		dbprintf_bat(LOG_ERR, "set meta system_default_route_interface = calix rg wanif!(%s)\n", value);

		// disable calix_rg wanif if veip me is under admin
		snprintf(key, 127, "wan%d_enable", wanif_index);
		if (veip_admin_state == 0) {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "0");
			omci_wanif_vid_del(calix_rg_devname);
		}
		snprintf(key, 127, "wan%d_ip_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_ipv6_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		// enable nat for calix rg config
		snprintf(key, 127, "nat%d_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
			// disable wanif igmp when no us igmp tci matches rg ani vlan
			if (omciutil_misc_tag_list_lookup(wan_vlan, t->us_tci_total, t->us_tci_list) >= 0)
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			else
				metacfg_adapter_config_entity_update(kv_config, key, "0");
		} else {
			metacfg_adapter_config_entity_update(kv_config, key, "1");
		}

		snprintf(key, 127, "wan%d_pbit", wanif_index);
		snprintf(value, 127, "%d", wan_rg_pbit);
		metacfg_adapter_config_entity_update(kv_config, key, value);
	}

	if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
		// enable wanif igmp if ani vlan matches igmp us tci
		if (t->me309_me310_is_updated) {
			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			if (omciutil_misc_tag_list_lookup(wan_vlan, t->us_tci_total, t->us_tci_list) >= 0)
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			else
				metacfg_adapter_config_entity_update(kv_config, key, "0");
		}
	}

	// for attrs the will be always the same as default, not changable by user/webui
	{
		snprintf(key, 127, "dev_wan%d_interface", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, calix_rg_devname);
		// always mark auto_created
		snprintf(key, 127, "wan%d_auto_created", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_iphost_only", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "0");
	}

	// set staticip/dhcp/pppoe common attrs
	if (wan_proto == 1) {   // static
		snprintf(key, 127, "wan%d_ip_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "0");
		snprintf(key, 127, "wan%d_ipv6_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "0");
		snprintf(key, 127, "wan%d_dnsauto_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "0");

		snprintf(key, 127,  "wan%d_if_name", wanif_index);
		snprintf(value, 127, "ipoe_veip%d-%d", veip_me->instance_num, wanif_devname_vlan(calix_rg_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		// delete branX_*
		wanif_delete_brwan_by_index(kv_config, wanif_index);

	} else if  (wan_proto == 0) {	// dhcp
		snprintf(key, 127, "wan%d_ip_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_ipv6_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");
		snprintf(key, 127, "wan%d_dnsauto_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		snprintf(key, 127,  "wan%d_if_name", wanif_index);
		snprintf(value, 127, "ipoe_veip%d-%d", veip_me->instance_num, wanif_devname_vlan(calix_rg_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		// delete branX_*
		wanif_delete_brwan_by_index(kv_config, wanif_index);

	} else if  (wan_proto == 2) {	// pppoe
		snprintf(key, 127, "wan%d_ip_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "2");
		snprintf(key, 127, "wan%d_ipv6_assignment", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "2");
		snprintf(key, 127, "wan%d_dnsauto_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		snprintf(key, 127,  "wan%d_if_name", wanif_index);
		snprintf(value, 127, "pppoe_veip%d-%d", veip_me->instance_num, wanif_devname_vlan(calix_rg_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		// delete branX_*
		wanif_delete_brwan_by_index(kv_config, wanif_index);

	} else {	// bridge mode, wan is deleted
		// delete dev_wanX_*
		wanif_delete_by_index(kv_config, wanif_index);

		// enable brwanX_*
		snprintf(key, 127, "brwan%d_name", wanif_index);
		snprintf(value, 127, "bridge_veip%d-%d", veip_me->instance_num, wanif_devname_vlan(calix_rg_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		snprintf(key, 127, "brwan%d_enable", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "1");

		snprintf(key, 127, "brwan%d_vid", wanif_index);
		snprintf(value, 127, "%d", wanif_devname_vlan(calix_rg_devname));
		metacfg_adapter_config_entity_update(kv_config, key, value);

		snprintf(key, 127, "brwan%d_pbit", wanif_index);
		snprintf(value, 127, "%d", wan_rg_pbit);
		metacfg_adapter_config_entity_update(kv_config, key, value);

		snprintf(key, 127, "brwan%d_portmask", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, "0xf");
	}

	// set static/pppoe specific attrs
	if (wan_proto == 1) {
		unsigned int ipaddress = (unsigned int)meinfo_util_me_attr_data(me, 3);
		unsigned int mask = (unsigned int)meinfo_util_me_attr_data(me, 4);
		unsigned int gateway = (unsigned int)meinfo_util_me_attr_data(me, 5);
		unsigned int dns1 = (unsigned int)meinfo_util_me_attr_data(me, 10);
		unsigned int dns2 = (unsigned int)meinfo_util_me_attr_data(me, 11);

		snprintf(key, 127, "wan%d_ip", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(ipaddress));
		snprintf(key, 127, "wan%d_netmask", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(mask));
		snprintf(key, 127, "wan%d_gateway", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(gateway));
		snprintf(key, 127, "wan%d_dns1", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(dns1));
		snprintf(key, 127, "wan%d_dns2", wanif_index);
		metacfg_adapter_config_entity_update(kv_config, key, iphost_inet_string(dns2));

	} else if (wan_proto == 2) {
		unsigned short pppauth_meid = (unsigned short)meinfo_util_me_attr_data(me, 6);
		struct me_t *pppauth_me = meinfo_me_get_by_meid(148, pppauth_meid);
		if (pppauth_me) {
			char pppoe_username[51], pppoe_passwd[26];
			struct attr_value_t attr_username1, attr_username2, attr_password;
			char cmd[128];
			char *output;

			attr_username1.ptr = attr_username2.ptr = attr_password.ptr = NULL;
			meinfo_me_attr_get(pppauth_me, 2, &attr_username1);
			meinfo_me_attr_get(pppauth_me, 4, &attr_username2);
			meinfo_me_attr_get(pppauth_me, 3, &attr_password);

			memcpy(pppoe_username, attr_username1.ptr, 25); pppoe_username[25]=0;
			if (strlen(pppoe_username)==25) {
				memcpy(pppoe_username+25, attr_username2.ptr, 25); pppoe_username[50]=0;
			}
			memcpy(pppoe_passwd, attr_password.ptr, 25); pppoe_username[25]=0;

			meinfo_me_attr_release(pppauth_me->classid, 2, &attr_username1);
			meinfo_me_attr_release(pppauth_me->classid, 4, &attr_username2);
			meinfo_me_attr_release(pppauth_me->classid, 3, &attr_password);

			snprintf(key, 127, "wan%d_pppoe_username", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, pppoe_username);

			snprintf(cmd, 128, "/usr/bin/rc4_base64 -e 27885vt8118 %s", pppoe_passwd);
			if (util_readcmd(cmd, &output) > 0) {	// output will be 'rc4_base64_encode=.....'
				char *eqptr = strchr(output, '=');
				if (eqptr) {
					snprintf(key, 127, "wan%d_pppoe_password", wanif_index);
					metacfg_adapter_config_entity_update(kv_config, key, util_trim(eqptr+1));
				}
			}
			free_safe(output);
		} else {
			snprintf(key, 127, "wan%d_pppoe_username", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "");
			snprintf(key, 127, "wan%d_pppoe_password", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "");
		}
	}

	return 0;
}

// auto create me329 related me171 wanif
static int
config_veip_rgvlan_wanif(struct metacfg_t *kv_config)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	int wanif_index, brwan_index, i;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		struct me_t *me65317_found = NULL;
		int mgmt_mode = batchtab_cb_wanif_get_calix_mgmt_mode(&me65317_found);
		if (mgmt_mode <0 || mgmt_mode == 1)	// 0: native (omci), 1: external (other than omci)
			return 0;		// do nothing, the stale rgvlan wanif will be removed by calix_delete_all_wanif()
	}

	// add rgvlan wanif if not found in dev_wan[0-7], brwan[0-7]
	for (i=0; i< t->veip_rg_tag_total; i++) {
		char key[128], value[128];
		char found_devname[33];
		int wanif_enable=0, brwan_enable=0;

		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IGNORE_UNTAG) {
			if ((t->veip_rg_tag_list[i] & 0xfff) == 0)
				continue;
		}

		struct me_t *veip_me = meinfo_me_get_by_meid(329, t->veip_rg_tag_meid[i]);
		unsigned char veip_admin_state = veip_me?meinfo_util_me_attr_data(veip_me, 1):0;

		key[127] = value[127] = found_devname[32] = 0;
		brwan_index = wanif_locate_brwan_index_by_vlan(kv_config, t->veip_rg_tag_list[i]&0xfff);
		wanif_index = wanif_locate_index_by_vlan(kv_config, t->veip_rg_tag_list[i]&0xfff, found_devname, 33);
		if (brwan_index >=0) {
			snprintf(key, 127, "brwan%d_enable", brwan_index);
			brwan_enable = util_atoi(metacfg_adapter_config_get_value(kv_config, key, 1));
		}
		if (wanif_index >= 0) {
			// exclude the veip/iphost wanif which is already handled in config_me65317()/config_me134()
			if (omciutil_misc_get_classid_me_by_devname_base(329, found_devname) ||
			    omciutil_misc_get_classid_me_by_devname_base(134, found_devname))
			    	continue;
			snprintf(key, 127, "wan%d_enable", brwan_index);
			wanif_enable = util_atoi(metacfg_adapter_config_get_value(kv_config, key, 1));
		}

		if (!omciutil_misc_find_me_related_bport(veip_me)) {	// disable rgvlan wanif if veip me has no bport
			if (brwan_index >=0) {	// wont happen actually, the wanif should be deleted by delete_auto_created_wanif_not_match_rg_tag_list() already
				snprintf(key, 127,  "brwan%d_enable", brwan_index);
				metacfg_adapter_config_entity_update(kv_config, key, "0");
			}
			if (wanif_index >=0) {	// wont happen actually, the wanif should be deleted by delete_auto_created_wanif_not_match_rg_tag_list() already
				snprintf(key, 127,  "wan%d_enable", wanif_index);
				metacfg_adapter_config_entity_update(kv_config, key, "0");
				omci_wanif_vid_del(found_devname);
			}
			continue;
		}

		if (brwan_enable) {
			if (wanif_enable) {	// the vid is used by brwan & wanif, delete the brwan
				wanif_delete_brwan_by_index(kv_config, brwan_index);
			} else {	// the vid used by brwan
				snprintf(key, 127,  "brwan%d_enable", brwan_index);
				metacfg_adapter_config_entity_update(kv_config, key, "1");
				continue;
			}
		}

		if (wanif_index<0) {	// create wanif if vid is not used by wanif or brwan
			int devname_index = omciutil_misc_locate_wan_new_devname_index(kv_config, "pon");
			if (devname_index < 0) {
				dbprintf_bat(LOG_ERR, "rg vlan %d can not find devname to use\n", t->veip_rg_tag_list[i]&0xfff);
				continue;
			}
			if ((t->veip_rg_tag_list[i]&0xfff) == 0) {
				snprintf(found_devname, 32, "pon%d", devname_index);
			} else {
				snprintf(found_devname, 32, "pon%d.%d", devname_index, t->veip_rg_tag_list[i]&0xfff);
			}
			if (omci_wanif_vid_locate(found_devname)>=0)
				continue;
			wanif_index = wanif_locate_index_unused(kv_config);
			if (wanif_index >=0) {
				dbprintf_bat(LOG_ERR, "rg vlan %d devname %s pbit%d created on wan%d\n", t->veip_rg_tag_list[i]&0xfff, found_devname, (t->veip_rg_tag_list[i]>>13)&0x7, wanif_index);
			} else {
				dbprintf_bat(LOG_ERR, "rg vlan %d devname %s has no available wan port\n", t->veip_rg_tag_list[i]&0xfff, found_devname);
				continue;
			}
			t->wanif_configured_after_mibreset[wanif_index] = 0;
		}
		omci_wanif_vid_add(found_devname);

		// for attrs that need reset to default when wanif is created or reboot/mibreset happens
		// which means user/webui modification on these attr could be remained untill next boot/mibreset
		if (t->wanif_configured_after_mibreset[wanif_index] == 0) {
			configured_bitmap_in_wanif_submit |= (1<<wanif_index);

			// disable calix_rg wanif if veip me is under admin
			snprintf(key, 127, "wan%d_enable", wanif_index);
			if (veip_admin_state == 0) {
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			} else {
				metacfg_adapter_config_entity_update(kv_config, key, "0");
				omci_wanif_vid_del(found_devname);
			}
			snprintf(key, 127, "wan%d_ip_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1");
			snprintf(key, 127, "wan%d_ipv6_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1");

			// enable nat for rgvlan wanif when it is crerated
			snprintf(key, 127, "nat%d_enable", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1");

			snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
				// disable wanif igmp when no us igmp tci matches rg ani vlan
				if (omciutil_misc_tag_list_lookup(t->veip_ani_tag_list[i]&0xfff, t->us_tci_total, t->us_tci_list) >= 0)
					metacfg_adapter_config_entity_update(kv_config, key, "1");
				else
					metacfg_adapter_config_entity_update(kv_config, key, "0");
			} else {
				metacfg_adapter_config_entity_update(kv_config, key, "1");
			}

			snprintf(key, 127, "wan%d_pbit", wanif_index);
			snprintf(value, 127, "%d", (t->veip_rg_tag_list[i]>>13)&0x7);
			metacfg_adapter_config_entity_update(kv_config, key, value);

			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IP_ASSIGN) {
				// ip assignment & if_name(desc) are possible to be overridden by user
				// so these 2 attr are assigned only when wanif is created, other attr are assigned every time
				snprintf(key, 127, "wan%d_ip_assignment", wanif_index);
				metacfg_adapter_config_entity_update(kv_config, key, "1");	// dhcp
				snprintf(key, 127, "wan%d_ipv6_assignment", wanif_index);
				metacfg_adapter_config_entity_update(kv_config, key, "1");	// dhcp
			}

			snprintf(key, 127,  "wan%d_if_name", wanif_index);
			if (veip_me)
				snprintf(value, 127, "ipoe_veip%d-%d", veip_me->instance_num, t->veip_rg_tag_list[i]&0xfff);
			else
				snprintf(value, 127, "ipoe_veip?-%d", t->veip_rg_tag_list[i]&0xfff);
			metacfg_adapter_config_entity_update(kv_config, key, value);
		}

		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE) {
			// enable wanif igmp if ani vlan matches igmp us tci
			if (t->me309_me310_is_updated) {
				snprintf(key, 127, "wan%d_igmp_enable", wanif_index);
				if (omciutil_misc_tag_list_lookup(t->veip_ani_tag_list[i]&0xfff, t->us_tci_total, t->us_tci_list) >= 0)
					metacfg_adapter_config_entity_update(kv_config, key, "1");
				else
					metacfg_adapter_config_entity_update(kv_config, key, "0");
			}
		}

		// for attrs the will be always the same as default, not changable by user/webui
		{
			snprintf(key, 127, "dev_wan%d_interface", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, found_devname);
			// always mark auto_created
			snprintf(key, 127, "wan%d_auto_created", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "1");
			snprintf(key, 127, "wan%d_iphost_only", wanif_index);
			metacfg_adapter_config_entity_update(kv_config, key, "0");
		}
	}
	return 0;
}
#endif

// main routine for the whole batchtab wanif //////////////////////////////////

static int
batchtab_cb_wanif_submit(struct batchtab_cb_wanif_t *t)
{
	//temp to escape exception then wait for fix

	return 0;
#if 1
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv1, kv2;
#endif
	struct meinfo_t *miptr;
	struct me_t *me;
#ifdef OMCI_METAFILE_ENABLE
	struct me_t *me65317_found = NULL;
	int mgmt_mode = 0;
#endif
	int ret, i;

	if (strlen(t->submit_kv_filename) > 0 &&
	    // util_file_is_available() means the diff file is not enqueued
	    // util_file_in_cmd_dispatcher_queue() means the diff is either enqueued or running
	    (util_file_is_available(t->submit_kv_filename) || util_file_in_cmd_dispatcher_queue(t->submit_kv_filename))) {
		long now = util_get_uptime_sec();
		if (now - t->submit_time < 120) {
			// previous submit kv file still exists, cancel this submit
			return -1;
		} else {
			dbprintf_bat(LOG_ERR, "previous submit %s exists for longer than 120 sec? process new submit anyway\n", t->submit_kv_filename);
		}
	}

	// clear access info (including wanif_configured_after_mibreset[], change_mask) if a new mibreset just happened
	// this flag(wanif_configured_after_mibreset[]) is used to determine whether to reset some wanif attr to default value or leave them asis
	if (t->mib_reset_timestamp_prev != omcimsg_mib_reset_finish_timestamp()) {
		t->mib_reset_timestamp_prev = omcimsg_mib_reset_finish_timestamp();
		clear_access_info_for_mibreset();
	}

	// collect rg->ani vlan translation on all iphost
	miptr=meinfo_util_miptr(134);
	t->iphost_rg_tag_total = 0;
	memset(t->iphost_rg_tag_meid, 0, sizeof(t->iphost_rg_tag_meid));
	memset(t->iphost_rg_tag_list, 0, sizeof(t->iphost_rg_tag_list));
	memset(t->iphost_ani_tag_list, 0, sizeof(t->iphost_ani_tag_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->iphost_rg_tag_total<RG2ANI_TAGLIST_TOTAL) {
			int found = omciutil_misc_me_rg_tag_ani_tag_collect(me, t->iphost_rg_tag_list+t->iphost_rg_tag_total, t->iphost_ani_tag_list+t->iphost_rg_tag_total, RG2ANI_TAGLIST_TOTAL-t->iphost_rg_tag_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->iphost_rg_tag_meid[t->iphost_rg_tag_total+i] = me->meid;
				t->iphost_rg_tag_total += found;
			}
		}
	}

	// collect rg->ani vlan translation on all veip
	miptr=meinfo_util_miptr(329);
	t->veip_rg_tag_total = 0;
	memset(t->veip_rg_tag_meid, 0, sizeof(t->veip_rg_tag_meid));
	memset(t->veip_rg_tag_list, 0, sizeof(t->veip_rg_tag_list));
	memset(t->veip_ani_tag_list, 0, sizeof(t->veip_ani_tag_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->veip_rg_tag_total<RG2ANI_TAGLIST_TOTAL) {
			int found = omciutil_misc_me_rg_tag_ani_tag_collect(me, t->veip_rg_tag_list+t->veip_rg_tag_total, t->veip_ani_tag_list+t->veip_rg_tag_total, RG2ANI_TAGLIST_TOTAL-t->veip_rg_tag_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->veip_rg_tag_meid[t->veip_rg_tag_total+i] = me->meid;
				t->veip_rg_tag_total += found;
			}
		}
	}

	// collect us igmp tci tag_control on all veip
	t->us_tci_total = 0;
	memset(t->us_tci_veip_meid, 0, sizeof(t->us_tci_veip_meid));
	memset(t->us_tci_list, 0, sizeof(t->us_tci_list));
	memset(t->us_tag_control_list, 0, sizeof(t->us_tag_control_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->us_tci_total<US_TCILIST_TOTAL) {
			int found = omciutil_misc_me_igmp_us_tci_tag_control_collect(me, t->us_tci_list+t->us_tci_total, t->us_tag_control_list+t->us_tci_total, US_TCILIST_TOTAL-t->us_tci_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->us_tci_veip_meid[t->us_tci_total+i] = me->meid;
				t->us_tci_total += found;
			}
		}
	}

	// mark start time of submit
	t->submit_time = util_get_uptime_sec();

#ifdef OMCI_METAFILE_ENABLE
	// load kv1, kv2 from metafile
	memset(&kv1, 0x0, sizeof(struct metacfg_t));
	memset(&kv2, 0x0, sizeof(struct metacfg_t));

	metacfg_adapter_config_init(&kv1);
	metacfg_adapter_config_init(&kv2);
	metacfg_adapter_config_file_load_safe(&kv1, "/etc/wwwctrl/metafile.dat");

	metacfg_adapter_config_merge(&kv2, &kv1, 1);	// clone kv1 to kv2

	// remove all replicated wanif, including original and replicated one
	{
		int indexmap_replicated = wanif_get_replicated_indexmap(&kv2);
		int i;
		for (i=0; i<WANIF_INDEX_TOTAL; i++) {
			if (indexmap_replicated & (1<<i)) {
				char key[32], *value;
				snprintf(key, 31, "dev_wan%d_interface", i);
				value = metacfg_adapter_config_get_value(&kv2, key, 1);
				dbprintf_bat(LOG_ERR, "delete wan%d %s because of replicated\n", i, value);
				wanif_delete_by_index(&kv2, i);
			}
		}
	}

	// init configured mask as 0 for this submit
	configured_bitmap_in_wanif_submit = 0;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		mgmt_mode = batchtab_cb_wanif_get_calix_mgmt_mode(&me65317_found);

		if (mgmt_mode >=0 && mgmt_mode != t->mgmt_mode_prev) {	// mgmt mode changed
			t->mgmt_mode_prev = mgmt_mode;
			dbprintf_bat(LOG_ERR, "calix_rg_config mgmt_mode is changed: %s, delete all wanif\n",
				mgmt_mode?"0(native) -> 1(external)":"1(external) -> 0(native)");

			// remove all wanif
			delete_all_wanif_brwan(&kv2);

			// save mgmt_mode to /storage/calix.mode, which would be used by
			// a. me 65317 hw_get to init me65317 attr1 in me create
			// b. batchtab_cb wanif to init the value of mgmt_mode_prev
			dbprintf_bat(LOG_ERR, "update mgmt_mode %d to /storage/calix.mode\n", mgmt_mode);
			batchtab_cb_wanif_save_mgmt_mode_to_storage(mgmt_mode);
		}
	}

	if (mgmt_mode == 0) { 	// native mode
		// clear stale veip/rgvlan wanif if it is not found in rg->ani vlan table
		delete_auto_created_wanif_not_match_rg_tag_list(&kv2);
	}

	if ((omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_ADD) &&
	    (miptr = meinfo_util_miptr(134)) != NULL) {
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me->instance_num >= IPHOST_TOTAL)
				continue;
//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
			config_me134(me, t->change_mask_me134[me->instance_num], &kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "config134 %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
		}
	}

	// config_me340 has different setting (nat=0, iphost=1) than config_me65317(nat=1, iphost=0) for wanif
	// if me340/me65317 are configuring the same wanif, since me65317 is executed later than me340,
	//  me65317 would override the setting configured by me340
	if ((miptr = meinfo_util_miptr(340)) != NULL) {
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			config_me340(me, &kv2);
		}
	}

	if (mgmt_mode == 0) { 	// native mode
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
		    (miptr = meinfo_util_miptr(65317)) != NULL) {
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				struct me_t *veip_me = meinfo_me_get_by_meid(329, me->meid);
				if (me->instance_num < CALIX_RG_CONFIG_TOTAL &&
				    veip_me && veip_me->instance_num < VEIP_TOTAL) {
//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
					config_me65317(me, t->change_mask_me65317[me->instance_num], &kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "config65317 %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
				}
			}
		}

//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_ADD)
			config_veip_rgvlan_wanif(&kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "configveip_rgvlan %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
	} 
	
	// copy configured_bitmap_in_wanif_submit bits to t->wanif_configured_after_mibreset[]
	for (i=0; i < WANIF_INDEX_TOTAL; i++) {
		if (configured_bitmap_in_wanif_submit & (1<<i))
			t->wanif_configured_after_mibreset[i] = 1;
	}
	configured_bitmap_in_wanif_submit = 0;

	// clear me309_me310_is_updated or igmp us_tci will be always referred and the wnif igmp_enable changed by webgui will be restored
	t->me309_me310_is_updated = 0;

	// clear entries of previous submit, (kv_config could be used after release without init as hash list head ptr are still valid)
	metacfg_adapter_config_release(&t->submit_kv_diff);
	memset(&t->submit_kv_diff, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&t->submit_kv_diff);
	// find out the kv_diff = kv2 -kv1
	ret = metacfg_adapter_config_diff(&kv1, &kv2, &t->submit_kv_diff, 0);
	dbprintf_bat(LOG_ERR, "submit %d changes\n", ret);
	metacfg_adapter_config_dump(util_dbprintf_get_console_fd(), &t->submit_kv_diff, "");
	metacfg_adapter_config_release(&kv1);
	metacfg_adapter_config_release(&kv2);

	if (ret > 0) {
		char cmd[256];
		snprintf(t->submit_kv_filename, 128, "/var/run/wanif.diff.%s", util_uptime_str());
		metacfg_adapter_config_file_save_safe(&t->submit_kv_diff, t->submit_kv_filename);

#ifdef OMCI_CMD_DISPATCHER_ENABLE
		snprintf(cmd, 256, "/etc/init.d/cmd_dispatcher.sh diff %s; rm %s", t->submit_kv_filename, t->submit_kv_filename);
		util_run_by_thread(cmd, 0);
#endif
	//	util_run_by_system(cmd);
	}
	return ret;
#else
	ret = 0;
	return ret;
#endif
#else
	struct kv_config_t kv1, kv2;
	struct meinfo_t *miptr;
	struct me_t *me;
	struct me_t *me65317_found = NULL;
	int mgmt_mode = 0;
	int ret, i;

	if (strlen(t->submit_kv_filename) > 0 &&
	    // util_file_is_available() means the diff file is not enqueued
	    // util_file_in_cmd_dispatcher_queue() means the diff is either enqueued or running
	    (util_file_is_available(t->submit_kv_filename) || util_file_in_cmd_dispatcher_queue(t->submit_kv_filename))) {
		long now = util_get_uptime_sec();
		if (now - t->submit_time < 120) {
			// previous submit kv file still exists, cancel this submit
			return -1;
		} else {
			dbprintf_bat(LOG_ERR, "previous submit %s exists for longer than 120 sec? process new submit anyway\n", t->submit_kv_filename);
		}
	}

	// clear access info (including wanif_configured_after_mibreset[], change_mask) if a new mibreset just happened
	// this flag(wanif_configured_after_mibreset[]) is used to determine whether to reset some wanif attr to default value or leave them asis
	if (t->mib_reset_timestamp_prev != omcimsg_mib_reset_finish_timestamp()) {
		t->mib_reset_timestamp_prev = omcimsg_mib_reset_finish_timestamp();
		clear_access_info_for_mibreset();
	}

	// collect rg->ani vlan translation on all iphost
	miptr=meinfo_util_miptr(134);
	t->iphost_rg_tag_total = 0;
	memset(t->iphost_rg_tag_meid, 0, sizeof(t->iphost_rg_tag_meid));
	memset(t->iphost_rg_tag_list, 0, sizeof(t->iphost_rg_tag_list));
	memset(t->iphost_ani_tag_list, 0, sizeof(t->iphost_ani_tag_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->iphost_rg_tag_total<RG2ANI_TAGLIST_TOTAL) {
			int found = omciutil_misc_me_rg_tag_ani_tag_collect(me, t->iphost_rg_tag_list+t->iphost_rg_tag_total, t->iphost_ani_tag_list+t->iphost_rg_tag_total, RG2ANI_TAGLIST_TOTAL-t->iphost_rg_tag_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->iphost_rg_tag_meid[t->iphost_rg_tag_total+i] = me->meid;
				t->iphost_rg_tag_total += found;
			}
		}
	}

	// collect rg->ani vlan translation on all veip
	miptr=meinfo_util_miptr(329);
	t->veip_rg_tag_total = 0;
	memset(t->veip_rg_tag_meid, 0, sizeof(t->veip_rg_tag_meid));
	memset(t->veip_rg_tag_list, 0, sizeof(t->veip_rg_tag_list));
	memset(t->veip_ani_tag_list, 0, sizeof(t->veip_ani_tag_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->veip_rg_tag_total<RG2ANI_TAGLIST_TOTAL) {
			int found = omciutil_misc_me_rg_tag_ani_tag_collect(me, t->veip_rg_tag_list+t->veip_rg_tag_total, t->veip_ani_tag_list+t->veip_rg_tag_total, RG2ANI_TAGLIST_TOTAL-t->veip_rg_tag_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->veip_rg_tag_meid[t->veip_rg_tag_total+i] = me->meid;
				t->veip_rg_tag_total += found;
			}
		}
	}

	// collect us igmp tci tag_control on all veip
	t->us_tci_total = 0;
	memset(t->us_tci_veip_meid, 0, sizeof(t->us_tci_veip_meid));
	memset(t->us_tci_list, 0, sizeof(t->us_tci_list));
	memset(t->us_tag_control_list, 0, sizeof(t->us_tag_control_list));
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (t->us_tci_total<US_TCILIST_TOTAL) {
			int found = omciutil_misc_me_igmp_us_tci_tag_control_collect(me, t->us_tci_list+t->us_tci_total, t->us_tag_control_list+t->us_tci_total, US_TCILIST_TOTAL-t->us_tci_total);
			if (found >0) {
				for (i=0; i<found; i++)	// mark those found rg->ani vtag rules owned by which veip
					t->us_tci_veip_meid[t->us_tci_total+i] = me->meid;
				t->us_tci_total += found;
			}
		}
	}

	// mark start time of submit
	t->submit_time = util_get_uptime_sec();

	// load kv1, kv2 from metafile
	kv_config_init(&kv1);
	kv_config_init(&kv2);
	kv_config_file_load_safe(&kv1, "/etc/wwwctrl/metafile.dat");

	kv_config_merge(&kv2, &kv1, 1);	// clone kv1 to kv2

	// remove all replicated wanif, including original and replicated one
	{
		int indexmap_replicated = wanif_get_replicated_indexmap(&kv2);
		int i;
		for (i=0; i<WANIF_INDEX_TOTAL; i++) {
			if (indexmap_replicated & (1<<i)) {
				char key[32], *value;
				snprintf(key, 31, "dev_wan%d_interface", i);
				value = kv_config_get_value(&kv2, key, 1);
				dbprintf_bat(LOG_ERR, "delete wan%d %s because of replicated\n", i, value);
				wanif_delete_by_index(&kv2, i);
			}
		}
	}

	// init configured mask as 0 for this submit
	configured_bitmap_in_wanif_submit = 0;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		mgmt_mode = batchtab_cb_wanif_get_calix_mgmt_mode(&me65317_found);

		if (mgmt_mode >=0 && mgmt_mode != t->mgmt_mode_prev) {	// mgmt mode changed
			t->mgmt_mode_prev = mgmt_mode;
			dbprintf_bat(LOG_ERR, "calix_rg_config mgmt_mode is changed: %s, delete all wanif\n",
				mgmt_mode?"0(native) -> 1(external)":"1(external) -> 0(native)");

			// remove all wanif
			delete_all_wanif_brwan(&kv2);

			// save mgmt_mode to /storage/calix.mode, which would be used by
			// a. me 65317 hw_get to init me65317 attr1 in me create
			// b. batchtab_cb wanif to init the value of mgmt_mode_prev
			dbprintf_bat(LOG_ERR, "update mgmt_mode %d to /storage/calix.mode\n", mgmt_mode);
			batchtab_cb_wanif_save_mgmt_mode_to_storage(mgmt_mode);
		}
	}

	if (mgmt_mode == 0) { 	// native mode
		// clear stale veip/rgvlan wanif if it is not found in rg->ani vlan table
		delete_auto_created_wanif_not_match_rg_tag_list(&kv2);
	}

	if ((omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_ADD) &&
	    (miptr = meinfo_util_miptr(134)) != NULL) {
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me->instance_num >= IPHOST_TOTAL)
				continue;
//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
			config_me134(me, t->change_mask_me134[me->instance_num], &kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "config134 %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
		}
	}

	// config_me340 has different setting (nat=0, iphost=1) than config_me65317(nat=1, iphost=0) for wanif
	// if me340/me65317 are configuring the same wanif, since me65317 is executed later than me340,
	//  me65317 would override the setting configured by me340
	if ((miptr = meinfo_util_miptr(340)) != NULL) {
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			config_me340(me, &kv2);
		}
	}

	if (mgmt_mode == 0) { 	// native mode
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
		    (miptr = meinfo_util_miptr(65317)) != NULL) {
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				struct me_t *veip_me = meinfo_me_get_by_meid(329, me->meid);
				if (me->instance_num < CALIX_RG_CONFIG_TOTAL &&
				    veip_me && veip_me->instance_num < VEIP_TOTAL) {
//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
					config_me65317(me, t->change_mask_me65317[me->instance_num], &kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "config65317 %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
				}
			}
		}

//struct kv_config_t kvorig, kvdiff; kv_config_init(&kvorig); kv_config_init(&kvdiff); kv_config_merge(&kvorig, &kv2, 1);
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_ADD)
			config_veip_rgvlan_wanif(&kv2);
//ret = kv_config_diff(&kvorig, &kv2, &kvdiff, 0); dbprintf_bat(LOG_ERR, "configveip_rgvlan %d changes\n", ret); kv_config_dump(util_dbprintf_get_console_fd(), &kvdiff, ""); kv_config_release(&kvorig); kv_config_release(&kvdiff);
	} 
	
	// copy configured_bitmap_in_wanif_submit bits to t->wanif_configured_after_mibreset[]
	for (i=0; i < WANIF_INDEX_TOTAL; i++) {
		if (configured_bitmap_in_wanif_submit & (1<<i))
			t->wanif_configured_after_mibreset[i] = 1;
	}
	configured_bitmap_in_wanif_submit = 0;

	// clear me309_me310_is_updated or igmp us_tci will be always referred and the wnif igmp_enable changed by webgui will be restored
	t->me309_me310_is_updated = 0;

	// clear entries of previous submit, (kv_config could be used after release without init as hash list head ptr are still valid)
	kv_config_release(&t->submit_kv_diff);
	kv_config_init(&t->submit_kv_diff);
	// find out the kv_diff = kv2 -kv1
	ret = kv_config_diff(&kv1, &kv2, &t->submit_kv_diff, 0);
	dbprintf_bat(LOG_ERR, "submit %d changes\n", ret);
	kv_config_dump(util_dbprintf_get_console_fd(), &t->submit_kv_diff, "");
	kv_config_release(&kv1);
	kv_config_release(&kv2);

	if (ret > 0) {
		char cmd[256];
		snprintf(t->submit_kv_filename, 128, "/var/run/wanif.diff.%s", util_uptime_str());
		kv_config_file_save_safe(&t->submit_kv_diff, t->submit_kv_filename);

		snprintf(cmd, 256, "/etc/init.d/cmd_dispatcher.sh diff %s; rm %s", t->submit_kv_filename, t->submit_kv_filename);
		util_run_by_thread(cmd, 0);
	//	util_run_by_system(cmd);
	}
	return ret;
#endif	
}

// utility function used by caller outisde this module /////////////////////////////////////
#ifdef OMCI_METAFILE_ENABLE

int
batchtab_cb_wanif_save_mgmt_mode_to_storage(int mgmt_mode)
{
	struct metacfg_t kv;

	memset(&kv, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv);
	metacfg_adapter_config_entity_add(&kv, "calix_mgmt_mode", mgmt_mode?"1":"0", 1);
	metacfg_adapter_config_file_save_safe(&kv, "/storage/calix.mode");
	metacfg_adapter_config_release(&kv);
	return 0;
}

int
batchtab_cb_wanif_load_mgmt_mode_from_storage(void)
{
	struct metacfg_t kv;
	int mgmt_mode = 0;
	
	memset(&kv, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv);
	if (util_file_is_available("/storage/calix.mode") &&
	    metacfg_adapter_config_file_load_safe(&kv, "/storage/calix.mode") >=0 ) {
		// if not found, "" will be returned by kv get, so 0 will be used
		mgmt_mode = util_atoi(metacfg_adapter_config_get_value(&kv, "calix_mgmt_mode", 1));
	}
	metacfg_adapter_config_release(&kv);

	return mgmt_mode;
}
#endif

int
batchtab_cb_wanif_get_calix_mgmt_mode(struct me_t **me65317_found)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	struct meinfo_t *miptr=meinfo_util_miptr(65317);
	struct me_t *me;

	if (!miptr)
		return -1;
	// find the instance with mgmt mode changed
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num <CALIX_RG_CONFIG_TOTAL) {
			if (util_attr_mask_get_bit(t->change_mask_me65317[me->instance_num], 1)) {	// attr1 (mgmt_mode) is modified
				*me65317_found = me;
				return (unsigned char)meinfo_util_me_attr_data(me, 1);
			}
		}
	}
	// find the instance with mgmt mode set as 1
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if ((unsigned char)meinfo_util_me_attr_data(me, 1) != 0) {
			*me65317_found = me;
			return 1;
		}
	}
	// else, just return the 1st instance
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		*me65317_found = me;
		return (unsigned char)meinfo_util_me_attr_data(me, 1);
	}
	return -1;
}

int
batchtab_cb_wanif_me_update(struct me_t *me, unsigned char change_mask[2])
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;

	if (me == NULL)
		return -1;

	// clear access info if new mibreset happened
	if (t->mib_reset_timestamp_prev != omcimsg_mib_reset_finish_timestamp()) {
		t->mib_reset_timestamp_prev = omcimsg_mib_reset_finish_timestamp();
		clear_access_info_for_mibreset();
	}

	if (me->classid == 134 && me->instance_num < IPHOST_TOTAL) {
		t->change_mask_me134[me->instance_num][0] |= change_mask[0];
		t->change_mask_me134[me->instance_num][1] |= change_mask[1];
		dbprintf_bat(LOG_INFO, "me%d 0x%x: change mask: update=%02x%02x, result=%02x%02x\n",
			me->classid, me->meid, change_mask[0], change_mask[1],
			t->change_mask_me134[me->instance_num][0], t->change_mask_me134[me->instance_num][1]);
		return 0;
	}
	if (me->classid == 329 && me->instance_num < VEIP_TOTAL) {
		t->change_mask_me329[me->instance_num][0] |= change_mask[0];
		t->change_mask_me329[me->instance_num][1] |= change_mask[1];
		dbprintf_bat(LOG_INFO, "me%d 0x%x: change mask: update=%02x%02x, result=%02x%02x\n",
			me->classid, me->meid, change_mask[0], change_mask[1],
			t->change_mask_me329[me->instance_num][0], t->change_mask_me329[me->instance_num][1]);
		return 0;
	}
	if (me->classid == 340 && me->instance_num < TR069_TOTAL) {
		t->change_mask_me340[me->instance_num][0] |= change_mask[0];
		t->change_mask_me340[me->instance_num][1] |= change_mask[1];
		dbprintf_bat(LOG_INFO, "me%d 0x%x: change mask: update=%02x%02x, result=%02x%02x\n",
			me->classid, me->meid, change_mask[0], change_mask[1],
			t->change_mask_me340[me->instance_num][0], t->change_mask_me340[me->instance_num][1]);
		return 0;
	}
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		if (me->classid == 65317 && me->instance_num < CALIX_RG_CONFIG_TOTAL) {
			t->change_mask_me65317[me->instance_num][0] |= change_mask[0];
			t->change_mask_me65317[me->instance_num][1] |= change_mask[1];
			dbprintf_bat(LOG_INFO, "me%d 0x%x: change mask: update=%02x%02x, result=%02x%02x\n",
				me->classid, me->meid, change_mask[0], change_mask[1],
				t->change_mask_me65317[me->instance_num][0], t->change_mask_me65317[me->instance_num][1]);
			return 0;
		}
	}
	if (me->classid == 309 || me->classid == 310) {
		t->me309_me310_is_updated = 1;
		return 0;
	}
	return -1;
}

// callback function registered in batchtab_cb /////////////////////////////////////

static int
batchtab_cb_wanif_init(void)
{
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;

	memset(&batchtab_cb_wanif_g, 0x0, sizeof(struct batchtab_cb_wanif_t));
	t->mib_reset_timestamp_prev = 0;
	clear_access_info_for_mibreset();

	t->iphost_rg_tag_total = 0;
	memset(t->iphost_rg_tag_meid, 0, sizeof(t->iphost_rg_tag_meid));
	memset(t->iphost_rg_tag_list, 0, sizeof(t->iphost_rg_tag_list));
	memset(t->iphost_ani_tag_list, 0, sizeof(t->iphost_ani_tag_list));

	t->veip_rg_tag_total = 0;
	memset(t->veip_rg_tag_meid, 0, sizeof(t->veip_rg_tag_meid));
	memset(t->veip_rg_tag_list, 0, sizeof(t->veip_rg_tag_list));
	memset(t->veip_ani_tag_list, 0, sizeof(t->veip_ani_tag_list));

	t->us_tci_total = 0;
	memset(t->us_tci_veip_meid, 0, sizeof(t->us_tci_veip_meid));
	memset(t->us_tci_list, 0, sizeof(t->us_tci_list));
	memset(t->us_tag_control_list, 0, sizeof(t->us_tag_control_list));

#ifdef OMCI_METAFILE_ENABLE
	metacfg_adapter_config_init(&t->submit_kv_diff);
#endif
	t->submit_kv_filename[0] = 0;
	t->submit_time = util_get_uptime_sec();

#ifdef OMCI_METAFILE_ENABLE
	// load mgmt_mode from /storage/calix.mode
	t->mgmt_mode_prev = batchtab_cb_wanif_load_mgmt_mode_from_storage();
	dbprintf_bat(LOG_ERR, "set mgmt_mode_prev to %d\n", t->mgmt_mode_prev);
#endif
	return 0;
}

static int
batchtab_cb_wanif_finish(void)
{
#ifdef OMCI_METAFILE_ENABLE
	struct batchtab_cb_wanif_t *t = &batchtab_cb_wanif_g;
	metacfg_adapter_config_release(&t->submit_kv_diff);
#endif
	return 0;
}
static int
batchtab_cb_wanif_gen(void **table_data)
{
	*table_data = &batchtab_cb_wanif_g;
	return 0;
}

static int
batchtab_cb_wanif_release(void *table_data)
{
	// do nothing as no dynamic malloc in batchtab_cb_wanif_gen
	return 0;
}
static int
batchtab_cb_wanif_dump(int fd, void *table_data)
{
	struct batchtab_cb_wanif_t *t= (struct batchtab_cb_wanif_t *)table_data;
	struct meinfo_t *miptr;
	struct me_t *me;
	char *ifname;
	long  i, now;

	if(t == NULL)
		return 0;
	{
		char buff[256], *s=buff;
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_ADD) {
			s += sprintf(s, (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_DEL)?"iphost+-, ":"iphost+, ");
		} else {
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_IPHOST_DEL)
				s += sprintf(s, "iphost-, ");
		}
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_ADD) {
			s += sprintf(s, (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_DEL)?"tr069+-, ":"tr069+, ");
		} else {
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_TR069_DEL)
				s += sprintf(s, "tr069-, ");
		}
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_ADD) {
			s += sprintf(s, (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_DEL)?"rgvlan+-, ":"rgvlan+, ");
		} else {
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_DEL)
				s += sprintf(s, "rgvlan-, ");
		}
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_BRWAN_ADD) {
			s += sprintf(s, (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_BRWAN_DEL)?"brwan+-, ":"brwan+, ");
		} else {
			if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_BRWAN_DEL)
				s += sprintf(s, "brwan-, ");
		}
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_AUTO_IGMP_ENABLE)
			s += sprintf(s, "auto_igmp, ");
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IGNORE_UNTAG)
			s += sprintf(s, "rgvlan_ignore_untag, ");
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_DEL_NONAUTO)
			s += sprintf(s, "rgvlan_del_nonauto, ");
		if (strlen(buff))	// remove tail ,
			buff[strlen(buff)-1] = buff[strlen(buff)-2] = 0;
		util_fdprintf(fd, "feature: 0x%x (%s)\n", omci_env_g.batchtab_wanif_feature, buff);
	}

	now = util_get_uptime_sec();
	if (strlen(t->submit_kv_filename) == 0) {
		util_fdprintf(fd, "last submit: %ds ago, none\n", now - t->submit_time);
	} else {
#ifdef OMCI_METAFILE_ENABLE
		util_fdprintf(fd, "last submit: %ds ago, %s, %s\n",
			now - t->submit_time, t->submit_kv_filename, util_file_is_available(t->submit_kv_filename)?"on going":"finished");
		metacfg_adapter_config_dump(fd, &t->submit_kv_diff, "");
#endif
	}
	util_fdprintf(fd, "\n");

	if ((miptr = meinfo_util_miptr(134)) != NULL) {
		util_fdprintf(fd, "iphost (classid 134):\n");
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me->instance_num >= IPHOST_TOTAL)
				continue;
			ifname = meinfo_util_get_config_devname(me);
			util_fdprintf(fd, "\t0x%04x (%s): change_mask=0x%02x%02x%s\n",
				me->meid, ifname,
				t->change_mask_me134[me->instance_num][0], t->change_mask_me134[me->instance_num][1],
				omciutil_misc_find_me_related_bport(me)?"":" (unused)");
		}
	}
	if ((miptr = meinfo_util_miptr(329)) != NULL) {
		unsigned char veip_admin_state;
		util_fdprintf(fd, "veip (classid 329):\n");
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me->instance_num >= VEIP_TOTAL)
				continue;
			veip_admin_state = (unsigned char)meinfo_util_me_attr_data(me, 1);
			ifname = meinfo_util_get_config_devname(me);
			util_fdprintf(fd, "\t0x%04x (%s): change_mask=0x%02x%02x, admin=%d%s\n",
				me->meid, ifname,
				t->change_mask_me329[me->instance_num][0], t->change_mask_me329[me->instance_num][1],
				veip_admin_state,
				(veip_admin_state == 0 && omciutil_misc_find_me_related_bport(me))?"":" (unused)");
		}
	}
	if ((miptr = meinfo_util_miptr(340)) != NULL) {
		struct me_t *veip_me;
		char *veip_devname;
		unsigned char tr069_admin_state;
		unsigned char *tr069_tci_ptr;
		unsigned short tr069_tci;
		int tr069_is_standalone_vlan = 0;
		char tr069_devname[33];

		tr069_devname[32] = 0;

		util_fdprintf(fd, "bbf_tr069 (classid 340):\n");
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			char *misc_str = "";
		
			if (me->instance_num >= TR069_TOTAL)
				continue;
			if ((veip_me=meinfo_me_get_by_meid(329, me->meid)) == NULL)
				continue;

			tr069_admin_state = (unsigned char)meinfo_util_me_attr_data(me, 1);
			tr069_tci_ptr = (unsigned char*)meinfo_util_me_attr_ptr(me, 3);
			tr069_tci = (unsigned short) util_bitmap_get_value(tr069_tci_ptr, 16, 0, 16);
			tr069_is_standalone_vlan = 0;
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
				struct me_t *calix_rg_config_me = meinfo_me_get_by_meid(65317, me->meid);
				unsigned short wan_vlan = (unsigned short)meinfo_util_me_attr_data(calix_rg_config_me, 8);	// this is ani vlan
				if ((tr069_tci == 0xffff && (wan_vlan & 0xfff) != 0) ||
				    (tr069_tci != 0xffff && (wan_vlan & 0xfff) != (tr069_tci&0xfff))) {
					tr069_is_standalone_vlan = 1;
				}
			}		
			if (tr069_is_standalone_vlan) {
#ifdef OMCI_METAFILE_ENABLE
				struct metacfg_t kv_config;
				memset(&kv_config, 0x0, sizeof(struct metacfg_t));
				metacfg_adapter_config_init(&kv_config);
				metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "datetime_ntp_enable");
				if (wanif_locate_index_by_vlan(&kv_config, tr069_tci&0xfff, tr069_devname, 33) <0)
					snprintf(tr069_devname, 32, "?");
				metacfg_adapter_config_release(&kv_config);
#endif
			} else {
				if ((veip_devname = meinfo_util_get_config_devname(veip_me)) == NULL || strlen(veip_devname) == 0)
					continue;
				if (tr069_tci == 0xffff) {
					snprintf(tr069_devname, 32, "%s", wanif_devname_base(veip_devname));
				} else {
					if (omciutil_misc_get_devname_by_veip_ani_tag(veip_me, tr069_tci, tr069_devname, 33) <0)
						snprintf(tr069_devname, 32, "%s.?", wanif_devname_base(veip_devname));
				}
			}
			if (tr069_admin_state == 0 && omciutil_misc_find_me_related_bport(veip_me)) {
				misc_str = tr069_is_standalone_vlan? "(standalone vlan)":"";
			} else {
				misc_str = "(unused)";
			}
			util_fdprintf(fd, "\t0x%04x (%s): change_mask=0x%02x%02x, admin=%d, associate_tag=0x%x%s\n",
				me->meid, tr069_devname,
				t->change_mask_me329[me->instance_num][0], t->change_mask_me329[me->instance_num][1],
				tr069_admin_state, tr069_tci, misc_str);
		}
	}
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		if ((miptr = meinfo_util_miptr(65317)) != NULL) {
			struct me_t *veip_me;
			char *veip_devname;
			unsigned char veip_admin_state;
			unsigned short wan_vlan;
			unsigned char mgmt_mode;
			char calix_rg_devname[33];
			char *misc_str = "";

			calix_rg_devname[32] = 0;

			util_fdprintf(fd, "calix_rg_config (classid 65317):\n");
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				if (me->instance_num >= CALIX_RG_CONFIG_TOTAL)
					continue;
				if ((veip_me=meinfo_me_get_by_meid(329, me->meid)) == NULL)
					continue;
				if ((veip_devname = meinfo_util_get_config_devname(veip_me)) == NULL || strlen(veip_devname) == 0)
					continue;
				veip_admin_state = (unsigned char)meinfo_util_me_attr_data(veip_me, 1);
				wan_vlan = (unsigned short)meinfo_util_me_attr_data(me, 8);	// ani vlan
				if (omciutil_misc_get_devname_by_veip_ani_tag(veip_me, wan_vlan, calix_rg_devname, 33) <0) {
					snprintf(calix_rg_devname, 32, "%s.?", wanif_devname_base(veip_devname));
				}
				mgmt_mode = (unsigned short)meinfo_util_me_attr_data(me, 1);	// 0:native, 1:external
				if (veip_admin_state == 0 && omciutil_misc_find_me_related_bport(veip_me)) {
					if (mgmt_mode==0 && wan_vlan == 0)
						misc_str="(RG service 1 not set on OLT)";
				} else {
					misc_str="(unused)";
				}
				util_fdprintf(fd, "\t0x%04x (%s): change_mask=0x%02x%02x, mgmt_mode=%d(%s,prev=%d), ani wan_vlan=0x%x %s\n",
					me->meid, calix_rg_devname,
					t->change_mask_me65317[me->instance_num][0], t->change_mask_me65317[me->instance_num][1],
					mgmt_mode, (mgmt_mode==0)?"native":"external", t->mgmt_mode_prev, 
					wan_vlan, misc_str);
			}
		}
	}
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "iphost bport tag translation:\n");
	for (i=0; i< t->iphost_rg_tag_total; i++) {
		if ((t->iphost_rg_tag_list[i]>>12)&1) { // in omciutil_misc_me_rg_tag_ani_tag_collect(), we mark de bit to represent pbit don't care term
			util_fdprintf(fd, "\t[%d] rg vlan %d pbit %s -> ani vlan %d pbit %d (iphost 0x%x)\n",
				i, t->iphost_rg_tag_list[i] & 0xfff, "*", t->iphost_ani_tag_list[i] & 0xfff, (t->iphost_ani_tag_list[i]>>13)&7, t->iphost_rg_tag_meid[i]);
		} else {
			util_fdprintf(fd, "\t[%d] rg vlan %d pbit %d -> ani vlan %d pbit %d (iphost 0x%x)\n",
				i, t->iphost_rg_tag_list[i] & 0xfff, (t->iphost_rg_tag_list[i]>>13)&7, t->iphost_ani_tag_list[i] & 0xfff, (t->iphost_ani_tag_list[i]>>13)&7, t->iphost_rg_tag_meid[i]);
		}
	}
	util_fdprintf(fd, "veip bport tag translation:\n");
	for (i=0; i< t->veip_rg_tag_total; i++) {
		if (omci_env_g.batchtab_wanif_feature & WANIF_FEATURE_RGVLAN_IGNORE_UNTAG) {
			if ((t->veip_rg_tag_list[i] & 0xfff) == 0)
				continue;
		}
		if ((t->veip_rg_tag_list[i]>>12)&1) { // in omciutil_misc_me_rg_tag_ani_tag_collect(), we mark de bit to represent pbit don't care term
			util_fdprintf(fd, "\t[%d] rg vlan %d pbit %s -> ani vlan %d pbit %d (veip 0x%x)\n",
				i, t->veip_rg_tag_list[i] & 0xfff, "*", t->veip_ani_tag_list[i] & 0xfff, (t->veip_ani_tag_list[i]>>13)&7, t->veip_rg_tag_meid[i]);
		} else {
			util_fdprintf(fd, "\t[%d] rg vlan %d pbit %d -> ani vlan %d pbit %d (veip 0x%x)\n",
				i, t->veip_rg_tag_list[i] & 0xfff, (t->veip_rg_tag_list[i]>>13)&7, t->veip_ani_tag_list[i] & 0xfff, (t->veip_ani_tag_list[i]>>13)&7, t->veip_rg_tag_meid[i]);
		}
	}
	util_fdprintf(fd, "veip bport igmp us tci:\n");
	for (i=0; i< t->us_tci_total; i++) {
		if (t->us_tag_control_list[i] == 0) {
			util_fdprintf(fd, "\t[%d] -1tag,+1tag: pri=pri,vid=vid (veip 0x%x)\n",
				i, t->us_tci_veip_meid[i]);
		} else if (t->us_tag_control_list[i] == 1) {
			util_fdprintf(fd, "\t[%d] +1tag: pri=%d,vid=%d (veip 0x%x)\n",
				i, ((t->us_tci_list[i]>>13)&0x7), (t->us_tci_list[i]&0xfff), t->us_tci_veip_meid[i]);
		} else if (t->us_tag_control_list[i] == 2) {
			util_fdprintf(fd, "\t[%d] -1tag,+1tag: pri=%d,vid=%d (veip 0x%x)\n",
				i, ((t->us_tci_list[i]>>13)&0x7), (t->us_tci_list[i]&0xfff), t->us_tci_veip_meid[i]);
		} else if (t->us_tag_control_list[i] == 3) {
			util_fdprintf(fd, "\t[%d] -1tag,+1tag: pri=pri,vid=%d (veip 0x%x)\n",
				i, (t->us_tci_list[i]&0xfff), t->us_tci_veip_meid[i]);
		}
	}

	util_fdprintf(fd, "\ntr069 status:\n");
	tr069_print_status(fd);
	util_fdprintf(fd, "\nwanif status:\n");
	wanif_print_status(fd);

	{
		char buff[256]={0}, *s=buff;
		for (i=0; i<32; i++) {
			if (t->omci_wanif_vid[i] >=0)
				s +=snprintf(s, 255-strlen(buff), "%d, ", t->omci_wanif_vid[i]);
		}
		if (strlen(buff) >0) {
			buff[strlen(buff)-2] = 0;
		} else {
			sprintf(buff, "none");
		}
		util_fdprintf(fd, "\nwanif vid created by omci: %s\n", buff);
	}

	return 0;
}
static int
batchtab_cb_wanif_hw_sync(void *table_data)
{
	struct batchtab_cb_wanif_t *t = (struct batchtab_cb_wanif_t *)table_data;
	static int defer_count;

	if (t == NULL)
		return 0;

	// dataasync ==0, system booting up, do nothing
	// so wanif auto created before wont be deleted in every system bootup.
	// thus the user configuration on those wanif could be preserved
	if (meinfo_util_get_data_sync() == 0)
		return 0;

	// skip wanif auto gen if mibreset till now < 20sec
	if (omcimsg_mib_reset_finish_time_till_now() < 20 || batchtab_cb_tagging_is_ready() == 0) {
		// keep the trigger for future and do nothing for now
		if (defer_count == 0)
			dbprintf_bat(LOG_ERR, "DEFER wanif hwsync\n");
		defer_count++;
		// do DEFER in hw_sync, call omci update to trigger future table_gen
		batchtab_omci_update("wanif");
		return 0;
	}
	if (defer_count) {
		defer_count = 0;
		dbprintf_bat(LOG_ERR, "resume DEFERRED wanif hwsync\n");
	}

	{
		static int is_submitting =0;
		int ret;
		if (!is_submitting) {
			is_submitting = 1;
			ret = batchtab_cb_wanif_submit(t);
			is_submitting = 0;
			if (ret <0)
				return -1;
		}
	}
	return 0;
}

// public function /////////////////////////////////////
int
batchtab_cb_wanif_me137_create(void)
{
	is_me137_delete = 0;
	return 0;
}

int
batchtab_cb_wanif_me137_delete(void)
{
	is_me137_delete = 1;
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_wanif_register(void)
{
	return batchtab_register("wanif", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_wanif_init,
			batchtab_cb_wanif_finish,
			batchtab_cb_wanif_gen,
			batchtab_cb_wanif_release,
			batchtab_cb_wanif_dump,
			batchtab_cb_wanif_hw_sync);
}
