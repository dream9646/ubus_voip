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
 * File    : batchtab_cb_isolation.c
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
#include "meinfo.h"
#include "metacfg_adapter.h"
#include "switch.h"
#include "proprietary_alu.h"
#include "proprietary_calix.h"
#include "omciutil_misc.h"
#include "hwresource.h"
#include "me_related.h"

static struct batchtab_cb_isolation_set_t batchtab_cb_isolation_set_g;	

///////////////////////////////////////////////////////////////////////////////////////////////

struct me_t *
get_pptp_uni_by_logical_portid(int logical_portid)
{
	struct meinfo_t *miptr11=meinfo_util_miptr(11);
	struct me_t *pptp_me;

	if (miptr11==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=11\n", MEINFO_ERROR_CLASSID_UNDEF);
		return NULL;  //class_id out of range
	}
	list_for_each_entry(pptp_me, &miptr11->me_instance_list, instance_node) {
        		struct me_t *private_pptp_me = hwresource_public2private(pptp_me);
		if (private_pptp_me) {
			unsigned char pptp_logical_portid = (unsigned char)meinfo_util_me_attr_data(private_pptp_me, 4);
			if (pptp_logical_portid == logical_portid)
				return pptp_me;
		}
	}
	return NULL;
}

#if 0
static unsigned int
get_pptp_router_uni_portmask(void)
{
	struct meinfo_t *miptr11=meinfo_util_miptr(11);
	struct me_t *pptp_me;
	unsigned int pptp_router_uni_portmask = 0;

	if (miptr11==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=11\n", MEINFO_ERROR_CLASSID_UNDEF);
		return 0;  //class_id out of range
	}
	list_for_each_entry(pptp_me, &miptr11->me_instance_list, instance_node) {
		unsigned char ip_ind = (unsigned char)meinfo_util_me_attr_data(pptp_me, 11);
		if (ip_ind == 1) {	//0: bridge, 1: router, 2: both
	        	struct me_t *private_pptp_me = hwresource_public2private(pptp_me);
			if (private_pptp_me) {
		        	unsigned char logical_portid = (unsigned char)meinfo_util_me_attr_data(private_pptp_me, 4);
		        	pptp_router_uni_portmask |= (1<<logical_portid);
			}
		}
	}
	return pptp_router_uni_portmask;
}
#endif

static unsigned int
get_unig_portmask_by_veip_meid(unsigned short veip_meid)
{
	struct meinfo_t *miptr264=meinfo_util_miptr(264);
	struct me_t *unig_me, *pptp_me;
	unsigned int unig_portmask = 0;

	list_for_each_entry(unig_me, &miptr264->me_instance_list, instance_node) {
		if (veip_meid != meinfo_util_me_attr_data(unig_me, 4))
			continue;
		pptp_me = meinfo_me_get_by_meid(11, unig_me->meid); //pptp_me has same meid as uni-G											
		if (pptp_me) {
	        	struct me_t *private_pptp_me = hwresource_public2private(pptp_me);
			if (private_pptp_me) {
		        	unsigned char logical_portid = (unsigned char)meinfo_util_me_attr_data(private_pptp_me, 4);
		        	unig_portmask |= (1<<logical_portid);
			}
		}
	}
	return unig_portmask;
}

unsigned int
get_brwan_uni_portmask_from_meta ()
{
#if 1
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t  kv_config;
	char key[128];
	unsigned int brwan_uni_portmask = 0;
	int index;
	
	key[127] = 0;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "brwan0_name", "brwan7_portmask");	// load from line 0 to the end of cwmp block
	for (index=0; index < BRWAN_TOTAL; index++) {
		snprintf(key, 127, "brwan%d_enable", index);		
		if (strcmp(metacfg_adapter_config_get_value(&kv_config, key, 1), "1") == 0) {
			snprintf(key, 127, "brwan%d_portmask", index);
			brwan_uni_portmask |= util_atoi(metacfg_adapter_config_get_value(&kv_config, key, 1));
		}
	}					
	metacfg_adapter_config_release(&kv_config);
	
	return brwan_uni_portmask;
#else
	return 0;
#endif
#else
	struct kv_config_t kv_config;
	char key[128];
	unsigned int brwan_uni_portmask = 0;
	int index;
	
	key[127] = 0;

	kv_config_init(&kv_config);
	kv_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "brwan0_name", "brwan7_portmask");	// load from line 0 to the end of cwmp block
	for (index=0; index < BRWAN_TOTAL; index++) {
		snprintf(key, 127, "brwan%d_enable", index);		
		if (strcmp(kv_config_get_value(&kv_config, key, 1), "1") == 0) {
			snprintf(key, 127, "brwan%d_portmask", index);
			brwan_uni_portmask |= util_atoi(kv_config_get_value(&kv_config, key, 1));
		}
	}					
	kv_config_release(&kv_config);
	
	return brwan_uni_portmask;
#endif	
}

// ifsw srcport mask control //////////////////////////////////////////////////////////////////

static int
get_massproduct_mode(void)
{
	char *massproduct = util_get_bootenv("massproduct");
	if (!massproduct)
		return 0;
	return atoi(massproduct);
}

static int
read_ifsw_mask(char *name)
{
	char *data, *p;	
	util_readcmd("cat /proc/ifsw.srcmask", &data);
	if ((p=strstr(data, name)) != NULL) {
		p+=strlen(name);
		if (p[0] == '=' && p[1] == ' ') {
			int value = util_atoi(p+2);
			free_safe(data);
			return value;
		}
	}
	free_safe(data);
	return -1;
}

static int
config_ifsw_mask_for_prefix(int is_config, unsigned int router_mode_uni_portmask, unsigned int bridge_mode_uni_portmask, char *prefix)
{
	char rt_mask_name[64], br_mask_name[64];
	int old_rt_mask, old_br_mask, rt_mask=0xfff, br_mask=0xfff;
	unsigned int all_uni_portmask = switch_get_uni_logical_portmask();
	int is_changed = 0;
	
	snprintf(rt_mask_name, 63, "%srt_allow_src_mask", prefix);
	snprintf(br_mask_name, 63, "%sbr_allow_src_mask", prefix);
	
	old_rt_mask = read_ifsw_mask(rt_mask_name);
	old_br_mask = read_ifsw_mask(br_mask_name);	
	if (old_rt_mask == -1 || old_br_mask == -1)
		return 0;
		
	if (is_config) {	// disable routing path for bridge mode uni ports
		//only modify uni port related bit
		rt_mask = (old_rt_mask & ~all_uni_portmask) | router_mode_uni_portmask;
		br_mask = (old_br_mask & ~all_uni_portmask) | bridge_mode_uni_portmask;
		// for arp/nd, always enable uni's br_mask for lan-slave wlan communication
		// for lldp, always enable uni's br_mask for omci-lldp task handling
		// once enabling br_mask, to wan port packet will be discarded by vlan tagging filter if it's denied to pass
		if ((strcmp(prefix, "arp_") == 0) || (strcmp(prefix, "nd_") == 0) || (strcmp(prefix, "lldp_") == 0))
			br_mask |= all_uni_portmask;

		// for rsra/dhcp6 bridge path, if any uni bridge port was enabled, also enable ds pon port simultaneously.
		// ds bridge path to omci will filter unnecessary port by vlan rules.
		if ((strcmp(prefix, "rsra_") == 0) || (strcmp(prefix, "dhcp6_") == 0))
		{
			if (bridge_mode_uni_portmask) //have any bridge port
			{
				br_mask = (1<<CHIP_PON_PORTID) | bridge_mode_uni_portmask;
			} else {
				br_mask = 0x0;
			}
		}
	} else {		// retore to enable br/rt path for all uni port
		rt_mask = old_rt_mask | all_uni_portmask;
		br_mask = old_br_mask | all_uni_portmask;

		if ((strcmp(prefix, "rsra_") == 0) || (strcmp(prefix, "dhcp6_") == 0))
		{
			br_mask = 0x0;
		}
	}

	if (rt_mask != old_rt_mask) {
		util_write_file("/proc/ifsw", "%s 0x%x\n", rt_mask_name, rt_mask);
		is_changed = 1;
	}
	if (br_mask != old_br_mask) {
		util_write_file("/proc/ifsw", "%s 0x%x\n", br_mask_name, br_mask);
		is_changed = 1;
	}
	return is_changed;
}

static int
config_ifsw_mask_for_all(int is_config, unsigned int router_mode_uni_portmask, unsigned int bridge_mode_uni_portmask)
{
	char *prefix[] = {
		"", 
		"arp_", "eapol_", "dhcp_tag_", "dhcp_untag_", "igmp_", 
		"nd_", "rsra_", "dhcp6_", "mld_", "lldp_", 
		NULL
	};
	int is_changed = 0, i;
	for (i=0; prefix[i] != NULL; i++)
		is_changed += config_ifsw_mask_for_prefix(is_config, router_mode_uni_portmask, bridge_mode_uni_portmask, prefix[i]);
	return is_changed?1:0;
}

static void
config_brext_block_ra_from_local(int is_config, unsigned int bridge_mode_uni_portmask)
{
	int i, is_set = 0;
    char *intf[] = {
            "eth0.2", "eth0.3", "eth0.4", "eth0.5", 
            NULL
        };

	//clear frist
	for (i = 0; intf[i] != NULL; i++)
	{
		util_write_file("/proc/brext", "del bk_mdeliver_ra %s\n", intf[i]);
	}
	util_write_file("/proc/brext", "del bk_mdeliver_ra eth0\n");

	//set by mask
	if (is_config)
	{
		unsigned int all_uni_portmask = switch_get_uni_logical_portmask();
		
		for (i = 0; intf[i] != NULL; i++)
		{
			if (((1 << i) & bridge_mode_uni_portmask) && ((1 << i) & all_uni_portmask))
			{
				util_write_file("/proc/brext", "add bk_mdeliver_ra %s\n", intf[i]);
				is_set = 1;
			}
		}
		if (is_set)
		{
			util_write_file("/proc/brext", "add bk_mdeliver_ra eth0\n");
		}
	}

	return;
}

// lanif member portmask control ///////////////////////////////////////////////////////////////
static int
config_lanif_uniportmask_for_vlanid(int is_config, unsigned int router_mode_uni_portmask, unsigned short vlanid)
{
	int is_changed = 0;

	return is_changed;
}
	
static int
config_lanif_uniportmask_for_all(int is_config, unsigned int router_mode_uni_portmask)
{
	unsigned short vlanid;
	int is_changed = 0;
	
	for (vlanid = omci_env_g.classf_hw_intf_lan_vid; vlanid < (omci_env_g.classf_hw_intf_lan_vid + 4); vlanid++)
		is_changed += config_lanif_uniportmask_for_vlanid(is_config, router_mode_uni_portmask, vlanid);
	return is_changed?1:0;
}

// rg sw path bridge_mode_uni_portmask control ///////////////////////////////////////////////////////////////

static int
config_rg_bridge_mode_uni_port_mask(int is_config, unsigned int bridge_mode_uni_portmask)
{
	unsigned int bridge_mode_uni_portmask_prev;
	char *data;
	int is_changed = 0;
	
	util_readcmd("cat /proc/rg/bridge_mode_uni_portmask", &data);
	bridge_mode_uni_portmask_prev = util_atoi(data);
	free_safe(data);
	
	if (is_config) {
		if (bridge_mode_uni_portmask != bridge_mode_uni_portmask_prev) {
			util_write_file("/proc/rg/bridge_mode_uni_portmask", "0x%x\n", bridge_mode_uni_portmask);
			is_changed = 1;
		}
	} else {
		if (0 != bridge_mode_uni_portmask_prev) {
			util_write_file("/proc/rg/bridge_mode_uni_portmask", "0x%x\n", 0);
			is_changed = 1;
		}
	}
	return is_changed;
}

// cpuport isolation control ///////////////////////////////////////////////////////////////
// ps: the trap to cpu function will always work no matter if the src uni port is isolated from cpuport or not
#if 0
static int
config_cpuport_isolation_override(int is_config, unsigned int bridge_mode_uni_portmask)
{
	unsigned int cpu_portid = switch_get_cpu_logical_portid(0);
	unsigned int old_dst_bitmap, dst_bitmap;
	int is_changed = 0;
	int i, ret;
	
	// the isolation set default is determined by the main part of hw_sync
	// this routine is called at the end of hw sync, which is used to override the setting of default 
	// so when is_config ==0, just do nothing
	if (!is_config)
		return 0;
	
	for (i=0; i < SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if ( ((1<<i) & bridge_mode_uni_portmask) == 0)
			continue;

		// isolation block from cpu to bridge mode uni
		old_dst_bitmap = dst_bitmap = 0;
		if ( (ret=switch_hw_g.port_isolation_get(cpu_portid, &old_dst_bitmap)) == 0) {
			dst_bitmap = old_dst_bitmap & (~bridge_mode_uni_portmask);
			if (old_dst_bitmap != dst_bitmap) {
				if ( (ret=switch_hw_g.port_isolation_set(cpu_portid, dst_bitmap)) == 0) {
					dbprintf_bat(LOG_WARNING, "port %d dst portmask is changed 0x%x to 0x%x\n", cpu_portid, old_dst_bitmap, dst_bitmap);
					is_changed = 1;
				} else {
					dbprintf_bat(LOG_ERR, "isolation_set error (0x%x) - port=%d, dst_bitmap=0x%x\n", ret, cpu_portid, dst_bitmap);
				}
			}
		} else {
			dbprintf_bat(LOG_ERR, "isolation_get error (0x%x) - port=%d\n", ret, cpu_portid);
		}
		// isolation block from bridge mode uni to cpuport
		old_dst_bitmap = dst_bitmap = 0;
		if ( (ret=switch_hw_g.port_isolation_get(i, &old_dst_bitmap)) == 0) {
			dst_bitmap = old_dst_bitmap & (~(1<<cpu_portid));
			if (old_dst_bitmap != dst_bitmap) {
				if ( (ret=switch_hw_g.port_isolation_set(i, dst_bitmap)) == 0) {
					dbprintf_bat(LOG_WARNING, "port %d dst portmask is changed 0x%x to 0x%x\n", i, old_dst_bitmap, dst_bitmap);
					is_changed = 1;
				} else {
					dbprintf_bat(LOG_ERR, "isolation_set error (0x%x) - port=%d, dst_bitmap=0x%x\n", ret, i, dst_bitmap);
				}
			}
		} else {
			dbprintf_bat(LOG_ERR, "isolation_get error (0x%x) - port=%d\n", ret, i);
		}
	}
	
	return is_changed;
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////

static int
batchtab_cb_isolation_set_init(void)
{
	return 0;
}

static int
batchtab_cb_isolation_set_finish(void)
{
	return 0;
}

static int
batchtab_cb_isolation_set_gen(void **table_data)
{
	struct batchtab_cb_isolation_set_t *t = &batchtab_cb_isolation_set_g;
	struct collect_bridge_t *b;
	struct meinfo_t *miptr45=meinfo_util_miptr(45);
	struct meinfo_t *miptr47=meinfo_util_miptr(47);
	struct me_t *meptr45, *meptr47;
	int ret = 0, pos, portid;
	unsigned int uni_portmask = switch_get_uni_logical_portmask();

	if (miptr45==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=45\n", MEINFO_ERROR_CLASSID_UNDEF);
		return MEINFO_ERROR_CLASSID_UNDEF;  //class_id out of range
	}

	if (miptr47==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=47\n", MEINFO_ERROR_CLASSID_UNDEF);
		return MEINFO_ERROR_CLASSID_UNDEF;  //class_id out of range
	}
	// clear everything
	memset(t, 0x0, sizeof(struct batchtab_cb_isolation_set_t));
	
	t->null_bridge_portmask = switch_get_uni_logical_portmask();
	pos=0;
	// per omci bridge
	list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
		if (pos >= BRIDGE_NUM_MAX) {
			dbprintf_bat(LOG_CRIT, "bridge service number should < %d\n", BRIDGE_NUM_MAX);
			ret = -1;
			break;
		}
		b = &t->collect_bridge[pos];

		b->is_used=1;
		b->bridge_meid = meptr45->meid;
		if (omci_env_g.localbridging_default==2) 
			b->local_bridge_enable=1;	//force onu local bridge=1
		else
			b->local_bridge_enable=meptr45->attr[3].value.data;

		list_for_each_entry(meptr47, &miptr47->me_instance_list, instance_node) {
			unsigned short tp_classid;
			struct me_t *tp_me;
			struct switch_port_info_t port_info;

			if (meptr47->is_private)
				continue;
			if (meptr47->attr[1].value.data != meptr45->meid)	//47's Bridge id pointer != 45's meid
				continue;
			tp_classid = miptr47->callback.get_pointer_classid_cb(meptr47, 4);
			tp_me = meinfo_me_get_by_meid(tp_classid, (unsigned short)meinfo_util_me_attr_data(meptr47,4));
			if (tp_me == NULL)
				continue;
			if (switch_get_port_info_by_me(meptr47, &port_info) < 0) {
				if(meptr47 != NULL)
					dbprintf_bat(LOG_ERR, "get port info error:classid=%u, meid=%u\n", meptr47->classid, meptr47->meid);
				continue;
			}
			// possible port type ENV_BRIDGE_PORT_TYPE_(CPU|UNI|WAN|IPHOST|DS_BCAST)
			// we collect pptp-uni, veip, iphost for display, but only pptp will be used for isolation_set control
			if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI ||		// bridge -> bport -> pptp_uni
			    port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ||		// bridge -> bport -> veip -> uni-g -> pptp_uni
			    port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST) {	// bridge -> bport -> iphost			    	
				if (b->port_total >= BRIDGE_PORT_MAX) {
					dbprintf_bat(LOG_CRIT, "Error bridge %d port total should <= %d\n", pos, BRIDGE_PORT_MAX);
					ret = -1;
					break;	// goto next bridge
				}			
				b->port_id[b->port_total] = port_info.logical_port_id;
				b->port_meid[b->port_total] = meptr47->meid;
				b->tp_classid[b->port_total] = tp_classid;
				b->tp_meid[b->port_total] = tp_me->meid;
				b->port_total++;
				if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
					b->portmask |= (1 << port_info.logical_port_id);
					t->null_bridge_portmask &= ~(1 << port_info.logical_port_id);

					if (omci_env_g.ctc_internet_bridge_enabled_srcmask) {
						if (omci_env_g.ctc_internet_bridge_enabled_srcmask &  (1 << port_info.logical_port_id))
							t->router_mode_uni_portmask |= (1 << port_info.logical_port_id) ;
					}
				}
			}
		}
		pos++;
	}	

	// null bridge	
	for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if (t->null_bridge_portmask & (1<<portid)) {
			struct me_t *pptp_me, *unig_me = NULL;
			if (t->null_bridge_port_total >= BRIDGE_PORT_MAX) {
				dbprintf_bat(LOG_CRIT, "Error bridge null port total should <= %d\n", BRIDGE_PORT_MAX);
				ret = -1;
				break;
			}			
			if ((pptp_me = get_pptp_uni_by_logical_portid(portid)) != NULL)
				unig_me =  meinfo_me_get_by_meid(264, pptp_me->meid);
			t->null_bridge_port_id[t->null_bridge_port_total] = portid;
			if (pptp_me)
				t->null_bridge_pptp_meid[t->null_bridge_port_total] = pptp_me->meid;
			if (unig_me)
				t->null_bridge_unig_meid[t->null_bridge_port_total] = unig_me->meid;
			t->null_bridge_port_total++;

			if (omci_env_g.ctc_internet_bridge_enabled_srcmask) {
				if (omci_env_g.ctc_internet_bridge_enabled_srcmask &  (1 << portid))
					t->router_mode_uni_portmask |= (1 << portid) ;
			} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
#if 0	// router_mode uni judged by pptp-uni attr 11
				unsigned char ip_ind = (unsigned char)meinfo_util_me_attr_data(pptp_me, 11);
				if (ip_ind == 1) {	//0: bridge, 1: router, 2: both
					t->router_mode_uni_portmask |= (1 << portid);
#else	// router_mode uni judged by unig-g attr 4
				unsigned short veip_meid = (unsigned short)meinfo_util_me_attr_data(unig_me, 4);
				struct me_t *veip_me = NULL;
				if (veip_meid)
					veip_me = meinfo_me_get_by_meid(329, veip_meid);
				// per claix request: a uni port is treated as route uni if it is related to veip through unig, no matter if the veip has bport or not
				//if (veip_me && omciutil_misc_find_me_related_bport(veip_me)) 
				if (veip_me)
					t->router_mode_uni_portmask |= (1 << portid);
#endif
			}
		}
	}
	dbprintf_bat(LOG_WARNING, "me relation check: router_mode_uni_portmask=0x%x\n", t->router_mode_uni_portmask);
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
			if(batchtab_cb_calix_apas_is_enabled()) {
				// in apas mode, override both rt and br mask wih all uni_portmask
				t->bridge_mode_uni_portmask = t->router_mode_uni_portmask = uni_portmask;
			} else {
				// in non-apas mode, find bridge mode uni port from omci
				t->bridge_mode_uni_portmask = uni_portmask & (~t->router_mode_uni_portmask);
			}
		} else {
			// find bridge mode uni port from omci
			t->bridge_mode_uni_portmask = uni_portmask & (~t->router_mode_uni_portmask);
		}
		dbprintf_bat(LOG_WARNING, "calix olt (apas=%d): bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
			batchtab_cb_calix_apas_is_enabled(), t->bridge_mode_uni_portmask, t->router_mode_uni_portmask);

	} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
		for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
			struct me_t *pptp_me = NULL;
			if (((1<<portid) & switch_get_uni_logical_portmask()) && ((pptp_me = get_pptp_uni_by_logical_portid(portid)) != NULL)) {
				struct me_t *ext_uni_me = meinfo_me_get_by_meid(253, pptp_me->meid);
				if(ext_uni_me) {
					unsigned char dhcp_mode = (unsigned char)meinfo_util_me_attr_data(ext_uni_me, 2);
					switch (dhcp_mode) {
						case 0: // no ctrl
						default:
							t->router_mode_uni_portmask |= (1 << portid);
							t->bridge_mode_uni_portmask |= (1 << portid);
							break;
						case 1: // router
							t->router_mode_uni_portmask |= (1 << portid);
							t->bridge_mode_uni_portmask &= ~(1 << portid);
							break;
						case 2: // bridge
							t->router_mode_uni_portmask &= ~(1 << portid);
							t->bridge_mode_uni_portmask |= (1 << portid);
							break;
						case 3: // disabled
							t->router_mode_uni_portmask &= ~(1 << portid);
							t->bridge_mode_uni_portmask &= ~(1 << portid);
							break;
					}
				}
			}
		}
		dbprintf_bat(LOG_WARNING, "zte olt: bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
			t->bridge_mode_uni_portmask, t->router_mode_uni_portmask);

	} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) {
		// use me171 to check bridge|router mode
		for (portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
			struct me_t *pptp_me = NULL;
			t->router_mode_uni_portmask |= (1 << portid);
			t->bridge_mode_uni_portmask |= (1 << portid);
			if (((1<<portid) & switch_get_uni_logical_portmask()) && ((pptp_me = get_pptp_uni_by_logical_portid(portid)) != NULL)) {
				struct meinfo_t *miptr171=meinfo_util_miptr(171);
				struct me_t *ext_vlan_me = NULL;
				list_for_each_entry(ext_vlan_me, &miptr171->me_instance_list, instance_node) {
					if(me_related(ext_vlan_me, pptp_me)) {
						int has_vlan_rule = 0;
						struct attr_table_header_t *table_header = NULL;
						struct attr_table_entry_t *table_entry_pos = NULL;
						if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(ext_vlan_me, 6)) == NULL)
							continue;
						list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
						{
							unsigned short treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*4, 112, 13);
							if(treatment_inner_vid>1 && treatment_inner_vid<4096) {
								has_vlan_rule = 1;
								break;
							}
						}
						if(has_vlan_rule) { // bridge
							if (omci_env_g.olt_workaround_enable != ENV_OLT_WORKAROUND_HUAWEI)
								t->router_mode_uni_portmask &= ~(1 << portid); // TODO: workaround to reach ont
							t->bridge_mode_uni_portmask |= (1 << portid);
						} else { // router
							t->router_mode_uni_portmask |= (1 << portid);
							t->bridge_mode_uni_portmask &= ~(1 << portid);
						}
						break;
					}
				}
			}
		}
		dbprintf_bat(LOG_WARNING, "huawei olt: bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
			t->bridge_mode_uni_portmask, t->router_mode_uni_portmask);

	} else {
		t->bridge_mode_uni_portmask = uni_portmask;

		dbprintf_bat(LOG_WARNING, "generic olt: bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
			t->bridge_mode_uni_portmask, t->router_mode_uni_portmask);
	}
	
	// for lan-to-lan intra-multicast, bridge mask needs to include all uni|wifi ports
	if(omci_env_g.cpuport_im_lan2lan_enable) {
		t->bridge_mode_uni_portmask |= (switch_get_uni_logical_portmask()|switch_get_wifi_logical_portmask());
		dbprintf_bat(LOG_WARNING, "env cpuport_im_lan2lan_enable: bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
			t->bridge_mode_uni_portmask, t->router_mode_uni_portmask);
	}
	
	// find bridge mode uni port from metafile
	t->brwan_uni_portmask = get_brwan_uni_portmask_from_meta();
	
	*table_data = t;
	return ret;
}

static int
batchtab_cb_isolation_set_hw_sync(void *table_data)
{
	struct batchtab_cb_isolation_set_t *t = (struct batchtab_cb_isolation_set_t *)table_data;
	struct collect_bridge_t *b;
	// CTC internet bridge port: support nat only, else CTC other bridge port: support nat + bridge
	unsigned int ctc_inetbr_uni_portmask = (omci_env_g.ctc_internet_bridge_enabled_srcmask & switch_get_uni_logical_portmask());	
	int pos, num, ret;

	if (t == NULL)
		return 0;
		
	// bridging all uni if alu olt and port2port mode enabled
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		if (omci_env_g.port2port_enable || proprietary_alu_get_ont_mode()==1) {
			if (switch_hw_g.local_bridging)
				switch_hw_g.local_bridging(1);
			dbprintf_bat(LOG_WARNING, "local bridging is forced for ALU port2port or router mode\n");
		}
		return 0;
	}

	for (pos=0; pos < BRIDGE_NUM_MAX; pos++) {
		unsigned int bridge_portmask = 0;
		b = &t->collect_bridge[pos];

		if (b->is_used==0) 
			continue;
			
		if (b->portmask == 0)
			continue;

		dbprintf_bat(LOG_WARNING, "bridge 0x%x, bridge_portmask=0x%x\n", b->bridge_meid, bridge_portmask);
		for (num=0; num < b->port_total; num++) {
			unsigned int self_portbit= (1 << b->port_id[num]);
			unsigned int dst_portmask = self_portbit;

			if (b->tp_classid[num] != 11)
				continue;
				
			if (b->local_bridge_enable) {
				dst_portmask = b->portmask;
				
				if (omci_env_g.ctc_internet_bridge_enabled_srcmask > 0) {
					unsigned int ctc_inetbr_portmask = bridge_portmask & ctc_inetbr_uni_portmask;
					unsigned int ctc_otherbr_portmask = bridge_portmask & (~ctc_inetbr_uni_portmask);
					if (self_portbit & ctc_inetbr_portmask) {
						dbprintf_bat(LOG_WARNING, "bridge 0x%x, srcport %d, ctc_inetbr_portmask=0x%x\n",
							b->bridge_meid, b->port_id[num], ctc_inetbr_portmask);
						dst_portmask = ctc_inetbr_portmask;
					} else if (self_portbit & ctc_otherbr_portmask) {
						dbprintf_bat(LOG_WARNING, "bridge 0x%x, srcport %d, ctc_otherbr_portmask=0x%x\n",
							b->bridge_meid, b->port_id[num], ctc_otherbr_portmask);
						dst_portmask = ctc_otherbr_portmask;
					}
				}

			}
			switch_hw_g.port_allowed(b->port_id[num], dst_portmask);
		}
	}

	// null_bridge_portmask means the ports not owned by any bridge
	dbprintf_bat(LOG_WARNING, "NULL bridge, null_bridge_portmask=0x%x\n", t->null_bridge_portmask);
	for (num=0; num < t->null_bridge_port_total; num++) {
		unsigned int self_portbit = (1 << t->null_bridge_port_id[num]);
		unsigned int dst_portmask = self_portbit;
		
		if (omci_env_g.localbridging_default) {
			dst_portmask = t->null_bridge_portmask;

			if (omci_env_g.ctc_internet_bridge_enabled_srcmask > 0) {
				unsigned int ctc_inetbr_portmask = t->null_bridge_portmask & ctc_inetbr_uni_portmask;
				unsigned int ctc_otherbr_portmask = t->null_bridge_portmask & (~ctc_inetbr_uni_portmask);
				if (self_portbit & ctc_inetbr_portmask) {
					dbprintf_bat(LOG_WARNING, "NULL bridge, srcport %d, ctc_inetbr_portmask=0x%x\n", t->null_bridge_port_id[num], ctc_inetbr_portmask);
					dst_portmask = ctc_inetbr_portmask;
				} else if (self_portbit & ctc_otherbr_portmask) {
					dbprintf_bat(LOG_WARNING, "NULL bridge, srcport %d, ctc_otherbr_portmask=0x%x\n", t->null_bridge_port_id[num], ctc_otherbr_portmask);
					dst_portmask = ctc_otherbr_portmask;
				}
			} else if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
#if 0	// partition null bridge by pptp-uni attr 11 bridged_or_ip_ind
				unsigned int pptp_router_uni_portmask = get_pptp_router_uni_portmask();	
				unsigned int br_portmask = t->null_bridge_portmask & (~pptp_router_uni_portmask);	// bridge
				unsigned int rt_portmask = t->null_bridge_portmask & pptp_router_uni_portmask;		// router
				if (self_portbit & br_portmask) {
					dbprintf_bat(LOG_WARNING, "NULL bridge, srcport %d, pptp br_portmask=0x%x\n", t->null_bridge_port_id[num], br_portmask);
					dst_portmask = br_portmask;
				} else if (self_portbit & rt_portmask) {
					dbprintf_bat(LOG_WARNING, "NULL bridge, srcport %d, pptp rt_portmask=0x%x\n", t->null_bridge_port_id[num], rt_portmask);
					dst_portmask = rt_portmask;
				}
#else	// partition null bridge by uni-g attr 4 non-omci_mgmt_id (veip ptr)
				struct me_t *unig_me = meinfo_me_get_by_meid(264, t->null_bridge_unig_meid[num]);
				if (unig_me) {
					unsigned short veip_meid = (unsigned short)meinfo_util_me_attr_data(unig_me, 4);
					if (meinfo_me_get_by_meid(329, veip_meid)) { // collect veip related uni only if the veip exist
						unsigned int unig_portmask = get_unig_portmask_by_veip_meid(veip_meid);
						dbprintf_bat(LOG_WARNING, "NULL bridge, srcport %d, veip_me:0x%x related unig_portmask=0x%x\n", 
							t->null_bridge_port_id[num], veip_meid, unig_portmask);
						dst_portmask = unig_portmask;
					}
				}
#endif
			}
		}
		switch_hw_g.port_allowed(t->null_bridge_port_id[num], dst_portmask);
	}

	//force cpu and pon to all other ports	
	{
		int pon_port = switch_get_wan_logical_portid(); 
		int cpu_port = switch_get_cpu_logical_portid(0);
		// pon -> all ports: uni+cpu_ext, pon, cpu
		if ( (ret=switch_hw_g.port_isolation_set(pon_port, 0xFFF)) != 0) {
			dbprintf_bat(LOG_ERR, "error (0x%x) - port=%d, dst_bitmap=0x%x\n", ret, pon_port, 0xFFF);
			return -1;
		}	
		// CPU -> all ports: uni+cpu_ext, pon, cpu
		if ( (ret=switch_hw_g.port_isolation_set(cpu_port, 0xFFF)) != 0) {
			dbprintf_bat(LOG_ERR, "error (0x%x) - port=%d, dst_bitmap=0x%x\n", ret, cpu_port, 0xFFF);
			return -1;
		}
	}

	// calix/zte/huawei special behavior:
	// bridge mode uni: allow briging pkt only + brwan
	// router mode uni: allow routing pkt only - brwan
	// apas mode uni: allow bridging/routing pkt, bridging pkt is dropped by the vtagging/vfiltering
	// implemented behavior:
	// a. disable ifsw routing path for bridge mode uni, for sw path
	// b. remove bridge mode uni from lanif membership, for hw path
	// c. override isolation set to block traffic between cpuport & bridge mode uni, for hw path
	// d. block local router advertisement to bridge mode unis in br0 deliver multicast path
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) ||
	    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) ||
	    (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0)) {
		unsigned int bridge_mode_uni_portmask, router_mode_uni_portmask;
		unsigned int uni_portmask = switch_get_uni_logical_portmask();
		
		// only do this in non massproduction, since the factory needs hybird mode uni to do hw test
		if (get_massproduct_mode() == 0) {
			int onu_state;
			int is_changed = 0;
			// for ifsw br/rt srcmask control
			bridge_mode_uni_portmask  = t->bridge_mode_uni_portmask | t->brwan_uni_portmask;
			router_mode_uni_portmask = t->router_mode_uni_portmask & ~t->brwan_uni_portmask;

			dbprintf_bat(LOG_WARNING, "combine brwan_uni_portmask(0x%x): bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
				t->brwan_uni_portmask, bridge_mode_uni_portmask, router_mode_uni_portmask);

			gpon_hw_g.onu_state_get(&onu_state);
			if (onu_state == GPON_ONU_STATE_O5 && meinfo_util_get_data_sync() > 0) {	// omci provisioning happeded
				// disable ifsw routing path for bridge mode uni port
				is_changed += config_ifsw_mask_for_all(1, router_mode_uni_portmask, bridge_mode_uni_portmask);
				dbprintf_bat(LOG_WARNING, "config ifsw: bridge_mode_uni_portmask=0x%x, router_mode_uni_portmask=0x%x\n", 
					bridge_mode_uni_portmask, router_mode_uni_portmask);
	
				// rmove bridge mode uni portmask from lan netif membership
				is_changed += config_lanif_uniportmask_for_all(1, router_mode_uni_portmask);
				dbprintf_bat(LOG_WARNING, "config rg lanif uni portmask: router_mode_uni_portmask=0x%x\n", router_mode_uni_portmask);
	
				// config rg slowpath for bridge_mode_uni_portmask
				is_changed += config_rg_bridge_mode_uni_port_mask(1, (~router_mode_uni_portmask)&uni_portmask);
				dbprintf_bat(LOG_WARNING, "config rg slowpath for bridge uni: ~router_mode_uni_portmask&uni_portmask=0x%x\n",  (~router_mode_uni_portmask)&uni_portmask);
	
				// override isolation set to block traffic between bridge mode uni & cpuport
				//is_changed += config_cpuport_isolation_override(1, bridge_mode_uni_portmask);
				// we use (~router_mode_uni_portmask)&uni_portmask) instead of bridge_mode_uni_portmask
				// since in calix apas mode, both bridge_mode_uni_portmask/router_mode_uni_portmask are 0xffff
				// so (~router_mode_uni_portmask)&uni_portmask) can fit for normal mode and apas mode
				config_brext_block_ra_from_local(1, (~router_mode_uni_portmask)&uni_portmask);
				dbprintf_bat(LOG_WARNING, "brext block localout ra: ~router_mode_uni_portmask&uni_portmask=0x%x\n", (~router_mode_uni_portmask)&uni_portmask);

			} else {
				is_changed += config_ifsw_mask_for_all(0, router_mode_uni_portmask, bridge_mode_uni_portmask);
				is_changed += config_lanif_uniportmask_for_all(0, router_mode_uni_portmask);
				is_changed += config_rg_bridge_mode_uni_port_mask(0, (~router_mode_uni_portmask)&uni_portmask);
				//is_changed += config_cpuport_isolation_override(0, bridge_mode_uni_portmask);
				config_brext_block_ra_from_local(0, (~router_mode_uni_portmask)&uni_portmask);
			}
	
			if (is_changed) {

			}
		} else {
			// for ifsw br/rt srcmask control
			bridge_mode_uni_portmask  = switch_get_uni_logical_portmask();
			router_mode_uni_portmask = switch_get_uni_logical_portmask();
			config_ifsw_mask_for_all(0, router_mode_uni_portmask, bridge_mode_uni_portmask);
			config_lanif_uniportmask_for_all(0, router_mode_uni_portmask);
			config_rg_bridge_mode_uni_port_mask(0, (~router_mode_uni_portmask)&uni_portmask);
			dbprintf_bat(LOG_ERR, "MASSPRODUCT mode! bridge mode uni remains ACCESS to router path!\n", get_massproduct_mode());
			config_brext_block_ra_from_local(0, 0x0);
		}
	}
	return 0;
}

static int
batchtab_cb_isolation_set_dump(int fd, void *table_data)
{
	struct batchtab_cb_isolation_set_t *t = (struct batchtab_cb_isolation_set_t *)table_data;
	struct collect_bridge_t *b;
	struct me_t *bridge_me;
	int pos, num;

	if (t == NULL)
		return 0;

	for (pos=0; pos < BRIDGE_NUM_MAX; pos++) {
		b = &t->collect_bridge[pos];
		bridge_me = meinfo_me_get_by_meid(45, b->bridge_meid);		
		if (b->is_used && bridge_me)  {
			util_fdprintf(fd, "bridge %d: bridge_me:0x%x,local_bridging=%d\n", 
				pos, b->bridge_meid, (unsigned char)meinfo_util_me_attr_data(bridge_me, 3));
			for (num=0; num < b->port_total; num++) {
				struct me_t *bport_me = meinfo_me_get_by_meid(47, b->port_meid[num]);
				char *devname = hwresource_devname(bport_me);
				
				util_fdprintf(fd, "\tport %d: bport_me:0x%x (%s)", b->port_id[num], b->port_meid[num], devname?devname:"?");
				if (b->tp_classid[num] == 11) {	// pptp-uni				
					struct me_t *pptp_me = meinfo_me_get_by_meid(11, b->tp_meid[num]);
					struct me_t *unig_me = meinfo_me_get_by_meid(264, pptp_me->meid);
					if (pptp_me) {
						util_fdprintf(fd, " pptp_me:0x%x,admin=%d,op=%d,ip_ind=%d", 
							pptp_me->meid,					
							(unsigned char)meinfo_util_me_attr_data(pptp_me, 5),
							(unsigned char)meinfo_util_me_attr_data(pptp_me, 6),
							(unsigned char)meinfo_util_me_attr_data(pptp_me, 11));
					}
					if (unig_me) {
						util_fdprintf(fd, " unig_me:0x%x,admin=%d,veip_meid=0x%x", 
							unig_me->meid,					
							(unsigned char)meinfo_util_me_attr_data(unig_me, 2),
							(unsigned short)meinfo_util_me_attr_data(unig_me, 4));
					}
					if (omci_env_g.ctc_internet_bridge_enabled_srcmask > 0) {
						if (omci_env_g.ctc_internet_bridge_enabled_srcmask & (1<<b->port_id[num])) {
							util_fdprintf(fd, " ctc_inetbr");
						} else {
							util_fdprintf(fd, " ctc_otherbr");
						}
					}
					if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
						struct me_t *ext_uni_me = meinfo_me_get_by_meid(253, b->tp_meid[num]);
						if (ext_uni_me) {
							util_fdprintf(fd, " ext_uni_me:0x%x,dhcp_mode=%d", 
								(unsigned short) ext_uni_me->meid,
								(unsigned char) meinfo_util_me_attr_data(ext_uni_me, 2));
						}
					}
				} else if (b->tp_classid[num] == 329) { // veip
					struct me_t *veip_me = meinfo_me_get_by_meid(329, b->tp_meid[num]);
					if (veip_me) {	
						char *devname = meinfo_util_get_config_devname(veip_me);
						util_fdprintf(fd, " veip_me:0x%x,dev=%s", veip_me->meid, devname?devname:"?");
					}
				} else if (b->tp_classid[num] == 134) { // iphost
					struct me_t *iphost_me = meinfo_me_get_by_meid(134, b->tp_meid[num]);
					if (iphost_me) {
						char *devname = meinfo_util_get_config_devname(iphost_me);
						util_fdprintf(fd, " iphost_me:0x%x,dev=%s", iphost_me->meid, devname?devname:"?");
					}
				}
				util_fdprintf(fd, "\n");
			}
		}
	}

	if (t->null_bridge_portmask) {
		util_fdprintf(fd, "bridge null:\n");
		for (num=0; num < t->null_bridge_port_total; num++) {
			struct me_t *pptp_me = meinfo_me_get_by_meid(11, t->null_bridge_pptp_meid[num]);
			struct me_t *unig_me = meinfo_me_get_by_meid(264, t->null_bridge_unig_meid[num]);
			
			util_fdprintf(fd, "\tport %d:", t->null_bridge_port_id[num]);
			if (pptp_me) {
				char *devname = hwresource_devname(pptp_me);
				util_fdprintf(fd, " (%s) pptp_me:0x%x,admin=%d,op=%d,ip_ind=%d", 
					devname, t->null_bridge_pptp_meid[num],
					(unsigned char)meinfo_util_me_attr_data(pptp_me, 5),
					(unsigned char)meinfo_util_me_attr_data(pptp_me, 6),
					(unsigned char)meinfo_util_me_attr_data(pptp_me, 11));
			}
			if (unig_me) {
				util_fdprintf(fd, " unig_me:0x%x,admin=%d,veip_meid=0x%x", 
					t->null_bridge_unig_meid[num],
					(unsigned char)meinfo_util_me_attr_data(unig_me, 2),
					(unsigned short)meinfo_util_me_attr_data(unig_me, 4));
			}
			if (omci_env_g.ctc_internet_bridge_enabled_srcmask > 0) {
				if (omci_env_g.ctc_internet_bridge_enabled_srcmask & (1<<t->null_bridge_port_id[num])) {
					util_fdprintf(fd, " ctc_inetbr");
				} else {
					util_fdprintf(fd, " ctc_otherbr");
				}
			}
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) {
				struct me_t *ext_uni_me = meinfo_me_get_by_meid(253, t->null_bridge_pptp_meid[num]);
				if (ext_uni_me) {
					util_fdprintf(fd, " ext_uni_me:0x%x,dhcp_mode=%d", 
						(unsigned short) ext_uni_me->meid,
						(unsigned char) meinfo_util_me_attr_data(ext_uni_me, 2));
				}
			}
			util_fdprintf(fd, "\n");
		}
	}
	
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "omci router_mode_uni_portmask: 0x%x\n", t->router_mode_uni_portmask);
	// find bridge mode uni port from omci
	util_fdprintf(fd, "bridge mode uni portmask: 0x%x%s\n", t->bridge_mode_uni_portmask,
		get_massproduct_mode()?" (ignored, massproduct==1)":" (massproduct==0)");
	// find bridge mode uni port from metafile
	util_fdprintf(fd, "meta brwan_uni_portmask: 0x%x\n", t->brwan_uni_portmask);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
	   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX)) {
		util_fdprintf(fd, "calix apas mode: %d\n", batchtab_cb_calix_apas_is_enabled());
	}
	util_fdprintf(fd, "\n");
	switch_hw_g.port_isolation_print(fd, 0);

	// bridging all uni if alu olt and port2port mode enabled
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		if (omci_env_g.port2port_enable || proprietary_alu_get_ont_mode()==1)
			util_fdprintf(fd, "\nlocal bridging is forced for ALU port2port or router mode\n");
	}

	return 0;
}

static int
batchtab_cb_isolation_set_release(void *table_data)
{
	//struct batchtab_cb_streamid_to_wan_t *t= (struct batchtab_cb_streamid_to_wan_t *)table_data;
	//int i;
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_isolation_set_register(void)
{
	return batchtab_register("isolation_set", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_isolation_set_init,
			batchtab_cb_isolation_set_finish,
			batchtab_cb_isolation_set_gen,
			batchtab_cb_isolation_set_release,
			batchtab_cb_isolation_set_dump,
			batchtab_cb_isolation_set_hw_sync);
}
