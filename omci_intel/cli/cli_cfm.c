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
 * Module  : cli
 * File    : cli_cfm.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <math.h>
#include "list.h"
#include "util.h"
#include "conv.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "cpuport.h"
#include "me_related.h"
#include "cli.h"
#include "hwresource.h"

#include "cfm_pkt.h"
#include "cfm_core.h"
#include "cfm_util.h"
#include "cfm_switch.h"
#include "cfm_print.h"
#include "cfm_send.h"

static int
cli_cfm_related_cfm_stack(int fd, struct me_t *me_br)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(305);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_br)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t-[305]cfm_stack:0x%x", me->meid);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_bridge(int fd, struct me_t *me_bp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(45);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_bp)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t>[45]bridge:0x%x", me->meid);
			cli_cfm_related_cfm_stack(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_bridge_port(int fd, struct me_t *me_mep)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mep)) {
			char *devname;
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t>[47]bport:0x%x,is_private=%d", me->meid, me->is_private);
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
			cli_cfm_related_bridge(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_mapper(int fd, struct me_t *me_mep)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(130);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mep)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t<[130]mapper:0x%x", me->meid);
			cli_cfm_related_bridge(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_tptype(int fd, struct me_t *me_mep, int tptype)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr;
	struct me_t *me;
	char type_str[16];
	int classid = 0;

	memset(type_str, 0, sizeof(type_str));

	switch(tptype) {
		case 1:
			strcpy(type_str, "uni");
			classid = 11;
			break;
		case 3:
			strcpy(type_str, "mapper");
			classid = 130;
			break;
		case 4:
			strcpy(type_str, "iphost");
			classid = 134;
			break;
		case 5:
			strcpy(type_str, "iwtp");
			classid = 266;
			break;
		case 6:
			strcpy(type_str, "mcast_iwtp");
			classid = 281;
			break;
		case 11:
			strcpy(type_str, "veip");
			classid = 329;
			break;
		default:
			return 0;
	}
	miptr=meinfo_util_miptr(classid);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mep)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t<[%d]%s:0x%x", classid, type_str, me->meid);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_mep_status(int fd, struct me_t *me_mep)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(303);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mep)) {
			int i;
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t-[303]mep_status:0x%x,ccm_tx=%d,lbr_tx=%d,next_lb_tid=%d,next_lt_tid=%d,",
				me->meid,
				(unsigned int)meinfo_util_me_attr_data(me, 8),
				(unsigned int)meinfo_util_me_attr_data(me, 10),
				(unsigned int)meinfo_util_me_attr_data(me, 11),
				(unsigned int)meinfo_util_me_attr_data(me, 12));
			util_fdprintf(fd, "mep_mac=");
			for (i=0; i<6; i++) {
				unsigned char mac = util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 1), 6*8, i*8, 8);
				util_fdprintf(fd, "%s%02x", (i>0)?":":"",mac);
			}
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_ccm_db(int fd, struct me_t *me)
{
	int i;

	for(i=1; i<=CFM_MAX_CCM_DB; i++) {
		struct attr_table_header_t *db = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me, i);
		struct attr_table_entry_t *table_entry_pos;

		list_for_each_entry(table_entry_pos, &db->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				util_fdprintf(fd, "\n\t\t\t[rmep_%02d]state=%d,failed-ok=%d,rdi=%d,",
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 0, 16),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 16, 8),
					(unsigned int) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 24, 32),
					(unsigned int) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 104, 8));
				util_fdprintf(fd, "mac=%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 56, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 64, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 72, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 80, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 88, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 32*8, 96, 8));
			}
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_mep_ccm_db(int fd, struct me_t *me_mep)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(304);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mep)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t-[304]ccm_db:0x%x", me->meid);
			cli_cfm_related_ccm_db(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_mep(int fd, struct me_t *me_ma)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(302);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_ma)) {
			int i;
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t<[302]mep:0x%x,mep_id=%d,mep_ctrl=0x%x,pri_vlan=%d,prio=%d,admin=%d,",
				me->meid,
				(unsigned short)meinfo_util_me_attr_data(me, 4),
				(unsigned char)util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 5), 1*8, 0, 8),
				(unsigned short)meinfo_util_me_attr_data(me, 6),
				(unsigned char)meinfo_util_me_attr_data(me, 8),
				(unsigned char)meinfo_util_me_attr_data(me, 7));
			util_fdprintf(fd, "egress_id=");
			for (i=0; i<6; i++) {
				unsigned char mac = util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 9), 6*8, i*8, 8);
				util_fdprintf(fd, "%s%02x", (i>0)?":":"",mac);
			}
			util_fdprintf(fd, ",%d", (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 9), 8*8, 48, 16));
			cli_cfm_related_mep_status(fd, me);
			cli_cfm_related_mep_ccm_db(fd, me);
			if(meinfo_util_me_attr_data(me, 2) == 0)
				cli_cfm_related_bridge_port(fd, me);
			else if(meinfo_util_me_attr_data(me, 2) == 1)
				cli_cfm_related_mapper(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_ma(int fd, struct me_t *me_md)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(300);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_md)) {
			int i;
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[300]ma:0x%x,name=(%s:%s),ccm_interval=%d,mhf_creation=%d,sender_id_permission=%d,",
				me->meid,
				(unsigned char *)meinfo_util_me_attr_ptr(me, 3),
				(unsigned char *)meinfo_util_me_attr_ptr(me, 4),
				(unsigned char)meinfo_util_me_attr_data(me, 5),
				(unsigned char)meinfo_util_me_attr_data(me, 7),
				(unsigned char)meinfo_util_me_attr_data(me, 8));
			util_fdprintf(fd, "assoc_vlan=");
			for (i=0; i<12; i++) {
				unsigned short vid = util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 6), 24*8, i*16+4, 12);
				util_fdprintf(fd, "%s%u", (i>0)?",":"", vid);
			}
			cli_cfm_related_mep(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_md(int fd)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(299);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_refresh(me, get_attr_mask);
		util_fdprintf(fd, "[299]md:0x%x,level=%d,name=(%s:%s),mhf_creation=%d,sender_id_permission=%d",
			me->meid,
			(unsigned char)meinfo_util_me_attr_data(me, 1),
			(unsigned char *)meinfo_util_me_attr_ptr(me, 3),
			(unsigned char *)meinfo_util_me_attr_ptr(me, 4),
			(unsigned char)meinfo_util_me_attr_data(me, 5),
			(unsigned char)meinfo_util_me_attr_data(me, 6));
		cli_cfm_related_ma(fd, me);
		util_fdprintf(fd, "\n");
	}
	return CLI_OK;
}

static int
cli_cfm_related_oam_mep(int fd, struct me_t *me_meg)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(65289);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_meg)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[65289]mep:0x%x,mep_id=%d,dir=%d,prio=%d,ccm=%d,admin=%d",
				me->meid,
				(unsigned short)meinfo_util_me_attr_data(me, 2),
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				(unsigned char)meinfo_util_me_attr_data(me, 7),
				(unsigned char)meinfo_util_me_attr_data(me, 8),
				(unsigned char)meinfo_util_me_attr_data(me, 6));
			cli_cfm_related_tptype(fd, me, (unsigned char)meinfo_util_me_attr_data(me, 4));
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_oam_mip(int fd, struct me_t *me_meg)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(65290);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_meg)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[65290]mip:0x%x,mep_id=%d",
				me->meid,
				(unsigned short)meinfo_util_me_attr_data(me, 2));
			cli_cfm_related_tptype(fd, me, (unsigned char)meinfo_util_me_attr_data(me, 3));
		}
	}
	return CLI_OK;
}

static int
cli_cfm_related_oam_meg(int fd)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(65288);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		unsigned char large_string[376];
		struct me_t *me_157;
		memset(large_string, 0, sizeof(large_string));
		meinfo_me_refresh(me, get_attr_mask);
		util_fdprintf(fd, "[65288]meg:0x%x,level=%d,vlan=%d,ccm_interval=%d,",
			me->meid,
			(unsigned char)meinfo_util_me_attr_data(me, 3),
			(unsigned short)meinfo_util_me_attr_data(me, 1),
			(unsigned char)meinfo_util_me_attr_data(me, 5));
		if ((me_157 = meinfo_me_get_by_meid(157, (unsigned short)meinfo_util_me_attr_data(me, 2))) != NULL) {
			meinfo_me_copy_from_large_string_me(me_157, large_string, sizeof(large_string));
			util_fdprintf(fd, "name=%s", large_string);
		} else
			util_fdprintf(fd, "name=NULL");
		cli_cfm_related_oam_mep(fd, me);
		cli_cfm_related_oam_mip(fd, me);
		util_fdprintf(fd, "\n");
	}
	return CLI_OK;
}

static cfm_config_t *
get_cfm_config_by_md_level_mp_id(int fd, int md_level, int mp_id)
{
	cfm_config_t *cfm_config;

	if (md_level < 0 || md_level > 7) {
		util_fdprintf(fd, "md level %d should between 0..7\n", md_level);
		return NULL;
	}
	if (mp_id < 1 || mp_id >8191) {
		util_fdprintf(fd, "mep id %d(0x%x) should between 1..8191\n", mp_id, mp_id);
		return NULL;
	}
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if (cfm_config->md_level == md_level && cfm_config->mep_id == mp_id)
			return cfm_config;
	}
	// check for server mep
	if (md_level == server_mep_cfm_config_g.md_level &&
	    mp_id == server_mep_cfm_config_g.mep_id)
	    	return &server_mep_cfm_config_g;

	util_fdprintf(fd, "md level %d, mep id %d(0x%x) not found\n", md_level, mp_id, mp_id);
	return NULL;
}

static int
get_mac_for_rmep(int fd, char *mac, cfm_config_t *cfm_config, char *str)
{
	if (strlen(str) == 17) {
		if (conv_str_mac_to_mem(mac, str) < 0) {
			util_fdprintf(fd, "Valid mac format: xx:xx:xx:xx:xx:xx\n");
			return -1;
		}
		return 0;
	} else {
		cfm_pkt_rmep_entry_t *entry;
		int rmep_id = util_atoi(str);

		if(rmep_id < 1 || rmep_id > 8191) {
			util_fdprintf(fd, "Valid rmep_id: 1~8191 (%d)\n", rmep_id);
			return -2;
		}
		list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
			if(entry->rmep_id == rmep_id) {
				memcpy(mac, &(entry->rmep_mac), IFHWADDRLEN);
				return 0;
			}
		}
		util_fdprintf(fd, "Cannot find rmep_id: %d\n", rmep_id);
		return -3;
	}
	return -3;
}

static int
get_opcode_from_str(char *opcode_str)
{
	if (strcmp(opcode_str, "ccm") == 0) return CFM_PDU_CCM;
	if (strcmp(opcode_str, "lbr") == 0) return CFM_PDU_LBR;
	if (strcmp(opcode_str, "lbm") == 0) return CFM_PDU_LBM;
	if (strcmp(opcode_str, "ltr") == 0) return CFM_PDU_LTR;
	if (strcmp(opcode_str, "ltm") == 0) return CFM_PDU_LTM;
	if (strcmp(opcode_str, "ais") == 0) return CFM_PDU_AIS;
	if (strcmp(opcode_str, "lck") == 0) return CFM_PDU_LCK;
	if (strcmp(opcode_str, "tst") == 0) return CFM_PDU_TST;
	if (strcmp(opcode_str, "lmr") == 0) return CFM_PDU_LMR;
	if (strcmp(opcode_str, "lmm") == 0) return CFM_PDU_LMM;
	if (strcmp(opcode_str, "1dm") == 0) return CFM_PDU_1DM;
	if (strcmp(opcode_str, "dmr") == 0) return CFM_PDU_DMR;
	if (strcmp(opcode_str, "dmm") == 0) return CFM_PDU_DMM;
	if (strcmp(opcode_str, "1sl") == 0) return CFM_PDU_1SL;
	if (strcmp(opcode_str, "slr") == 0) return CFM_PDU_SLR;
	if (strcmp(opcode_str, "slm") == 0) return CFM_PDU_SLM;
	return 0xff;
}

// list_type 0: lbr, 1:ltr, 2:ccm
static void
cfm_clear_lbr_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	int mep_total = 0;
	int rmep_total = 0;

	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
		    	mep_total++;
		    	rmep_total += cfm_mp_flush_lbr_list(cfm_config);
		}
	}
	if (mep_total)
		util_fdprintf(fd, "%d rmep entry cleared from %d mep(s)\n", rmep_total, mep_total);
	else
		util_fdprintf(fd, "no mep found\n");
	return;
}

static void
cfm_clear_ltm_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	int mep_total = 0;
	int rmep_total = 0;

	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
		    	mep_total++;
		    	rmep_total += cfm_mp_flush_ltm_list(cfm_config);
		}
	}
	if (mep_total)
		util_fdprintf(fd, "%d rmep entry cleared from %d mep(s)\n", rmep_total, mep_total);
	else
		util_fdprintf(fd, "no mep found\n");
	return;
}

static void
cfm_clear_ltr_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	int mep_total = 0;
	int rmep_total = 0;

	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
		    	mep_total++;
		    	rmep_total += cfm_mp_flush_ltr_list(cfm_config);
		}
	}
	if (mep_total)
		util_fdprintf(fd, "%d rmep entry cleared from %d mep(s)\n", rmep_total, mep_total);
	else
		util_fdprintf(fd, "no mep found\n");
	return;
}

static void
cfm_clear_rmep_list(int fd, int md_level, int mep_id)
{
	cfm_config_t *cfm_config;
	int mep_total = 0;
	int rmep_total = 0;

	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
		    	mep_total++;
		    	rmep_total += cfm_mp_flush_rmep_list(cfm_config);
			cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config);
		}
	}
	if (mep_total)
		util_fdprintf(fd, "%d rmep entry cleared from %d mep(s)\n", rmep_total, mep_total);
	else
		util_fdprintf(fd, "no mep found\n");
	return;
}

// cfm mp config update callback /////////////////////////////////////////////////////////////////////////////////////////////

static int
update_bp(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int port_type = util_atoi(argv[0]);
	if(port_type < 0 || port_type > 2)
		return -1;
	cfm_config->port_type = port_type;
	cfm_config->port_type_index = util_atoi(argv[1]);
	cfm_config->logical_port_id = util_atoi(argv[2]);
	return 0;
}

static int
update_dir(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	if (strcmp(argv[0], "up") == 0) {
		cfm_config->mep_control |= MEP_CONTROL_IS_UP_MEP;
	} else if (strcmp(argv[0], "down") == 0) {
		cfm_config->mep_control &= ~MEP_CONTROL_IS_UP_MEP;
	} else {
		int is_up_mep = util_atoi(argv[0]);
		if (is_up_mep)
			cfm_config->mep_control |= MEP_CONTROL_IS_UP_MEP;
		else
			cfm_config->mep_control &= ~MEP_CONTROL_IS_UP_MEP;
	}
	return 0;
}

static int
update_type(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	if (strcmp(argv[0], "mip") == 0) {
		cfm_config->ma_mhf_creation = MA_MHF_CREATION_DEFAULT;
	} else if (strcmp(argv[0], "mep") == 0) {
		cfm_config->ma_mhf_creation = MA_MHF_CREATION_NONE;
	} else {
		int mip_enable = util_atoi(argv[0])?1:0;
		cfm_config->ma_mhf_creation = mip_enable? MA_MHF_CREATION_DEFAULT : MA_MHF_CREATION_NONE;
	}
	return 0;
}

static int
update_macaddr(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	unsigned char macaddr[6];
	if (conv_str_mac_to_mem((char *)macaddr, argv[0])<0)
		return -1;
	memcpy(cfm_config->macaddr, macaddr, 6);
	return 0;
}

static char *
md_name_format_string(int type)
{
	char *format_str[] = { "?", "none", "dns", "macaddr_shortint", "string" };
	if (type>=1 && type <= 4)
		return format_str[type];
	else if (type == 32)
		return "icc";
	else
		return "unknown?";
}

static char *
ma_name_format_string(int type)
{
	char *format_str[] = { "?", "vid", "string", "shortint", "vpnid" };
	if (type>=1 && type <= 4)
		return format_str[type];
	else if (type == 32)
		return "icc";
	else
		return "unknown?";
}

static int
update_md_name(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int format;
	char *string, *string2 = NULL;

	if (argc == 0) {
		return -1;
	} else if (argc == 1) {
		if (strcmp(argv[0], "1") == 0) {	// format 0, null md name
			format=MD_NAME_FORMAT_NONE;
			string = NULL;
		} else {
			format=MD_NAME_FORMAT_STRING;
			string = argv[0];
			util_fdprintf(fd, "assume MD name format is %d(string)\n", MD_NAME_FORMAT_STRING);
		}
	} else if (argc >= 2) {
		format = util_atoi(argv[0]);
		string = argv[1];
		if (argc > 2)
			string2 = argv[2];
	}
	if (cfm_util_set_md_name(cfm_config, format, string, string2) <0) {
		util_fdprintf(fd, "md name format err, format=%d(%s), name=%s %s\n", 
			format, md_name_format_string(format), (string)?string:"", (string2)?string2:"");
		return -1;
	}
	cfm_config->md_format = format;
	return 0;
}

static int
update_ma_name(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int format;
	char *string, *string2 = NULL;

	if (argc == 0) {
		return -1;
	} else 	if (argc == 1) {
		format=MA_NAME_FORMAT_STRING;
		string = argv[0];
			util_fdprintf(fd, "assume MD name format is %d(string)\n", MA_NAME_FORMAT_STRING);
	} else if (argc >= 2) {
		format = util_atoi(argv[0]);
		string = argv[1];
		if (argc > 2)
			string2 = argv[2];
	}
	if (cfm_util_set_ma_name(cfm_config, format, string, string2) <0) {
		util_fdprintf(fd, "ma name format err, format=%d(%s), name=%s %s\n", 
			format, ma_name_format_string(format), string, (string2)?string2:"");
		return -1;
	}
	cfm_config->ma_format = format;
	return 0;
}

static int
update_sender_id_permission(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int permission =  util_atoi(argv[0]); 	//
	if (permission >= MA_SENDER_ID_PERMISSION_NONE &&
	    permission <= MA_SENDER_ID_PERMISSION_DEFER) {	// 1..5
	    	cfm_config->ma_sender_id_permission = permission;
	    	return 0;
	}
	return -1;
}

static int
update_admin(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int admin_state =  util_atoi(argv[0])?1:0; 	// admin state 1 means lock, port is disabled
	cfm_config->admin_state = admin_state;
	return 0;
}

static int
update_vlan(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int pri_vlan = util_atoi(argv[0]);
	if (pri_vlan < 0 || pri_vlan > 4095)
		return -1;
	cfm_config->pri_vlan = pri_vlan;
	return 0;
}

static int
update_priority(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int priority = util_atoi(argv[0]);
	if (priority < 0 || priority > 7)
		return -1;
	cfm_config->priority = priority;
	return 0;
}

static int
update_pbit_filter(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int pbit_filter = util_atoi(argv[0]);
	cfm_config->pbit_filter = pbit_filter;
	return 0;
}

static int
update_lbr_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int lbr_enable = util_atoi(argv[0]);
	if (lbr_enable)
		cfm_config->mep_control2 |= MEP_CONTROL2_LBR_ENABLE;
	else
		cfm_config->mep_control2 &= ~MEP_CONTROL2_LBR_ENABLE;
	return 0;
}

static int
update_ltr_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ltr_enable = util_atoi(argv[0]);
	if (ltr_enable)
		cfm_config->mep_control2 |= MEP_CONTROL2_LTR_ENABLE;
	else
		cfm_config->mep_control2 &= ~MEP_CONTROL2_LTR_ENABLE;
	return 0;
}

static int
update_dmm_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int dmm_enable = util_atoi(argv[0]);
	if (dmm_enable)
		cfm_config->mep_control2 |= MEP_CONTROL2_DMM_ENABLE;
	else
		cfm_config->mep_control2 &= ~MEP_CONTROL2_DMM_ENABLE;
	return 0;
}

static int
update_lmm_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int lmm_enable = util_atoi(argv[0]);
	if (lmm_enable)
		cfm_config->mep_control2 |= MEP_CONTROL2_LMM_ENABLE;
	else
		cfm_config->mep_control2 &= ~MEP_CONTROL2_LMM_ENABLE;
	return 0;
}

static int
update_ccm_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ccm_enable = util_atoi(argv[0]);
	if (ccm_enable)
		cfm_config->mep_control |= MEP_CONTROL_CCM_ENABLE;
	else
		cfm_config->mep_control &= ~MEP_CONTROL_CCM_ENABLE;
	return 0;
}

static int
update_ccm_interval(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ccm_interval = util_atoi(argv[0]);
	if (ccm_interval < 0 || ccm_interval > 7)
		return -1;
	cfm_config->ccm_interval = ccm_interval;

	{	// restart mep ccm timer, or change timer from long to short wont take effect immediately
		cfm_cmd_msg_t *cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
		cfm_cmd_msg->mdlevel= cfm_config->md_level;
		cfm_cmd_msg->usr_data = (void*)cfm_config;
		cfm_cmd_msg->cmd = CFM_CMD_CCM_INTERVAL_UPDATE;
		if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0) {
			dbprintf(LOG_ERR, "Cmd CFM_CMD_CCM_INTERVAL_UPDATE fail!\n");
			free_safe(cfm_cmd_msg);
			return -1;
		}
	}
	return 0;
}

static int
update_auto_discovery(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int auto_discovery = util_atoi(argv[0]);
	cfm_config->auto_discovery = auto_discovery;
	return 0;
}

static int
update_peer_mep_id(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int i, rmepid, total = argc;

	if (cfm_config == NULL || total > 12)
		return -1;

	// clear old peer list
	memset(cfm_config->peer_mep_ids, 0, sizeof(cfm_config->peer_mep_ids));
	// set new peer list
	for (i=0; i< total; i++) {
		if ((rmepid = util_atoi(argv[i])) > 0)
			cfm_config->peer_mep_ids[i] = rmepid;
	}
	cfm_mp_flush_rmep_list(cfm_config);
	cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config);

	return 0;
}

static int
update_rmep_state(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int rmep_id = util_atoi(argv[0]);
	int rmep_state = util_atoi(argv[1]);
	cfm_pkt_rmep_entry_t *entry, *entry_next;
	int total = 0;

	if (rmep_state<0 || rmep_state>7)
		return -1;
	if (strcmp(argv[0], "*") == 0)
		rmep_id = 0xffff;

	list_for_each_entry_safe(entry, entry_next, &cfm_config->cfm_recv_rmep_list, node) {
		if (rmep_id == 0xffff || rmep_id == entry->rmep_id) {
			if (rmep_state == CCM_RMEP_STATE_NONE || rmep_state == CCM_RMEP_STATE_REMOVE) {
				if (!cfm_mp_is_peer_mep_id(cfm_config, entry->rmep_id)) {	// rmep is removed only if not in peer_mep_id[]
					util_fdprintf(fd, "remove rMEP %d state %s(%d) from mdl=%d, mep=%d CCMdb\n",
						entry->rmep_id, cfm_print_get_rmep_state_str(entry->rmep_state), entry->rmep_state,
						cfm_config->md_level, cfm_config->mep_id);
					list_del(&entry->node);
					free_safe(entry);
					total++;
				}
			} else {
				entry->rmep_timestamp = cfm_util_get_time_in_us();
				entry->rmep_state = rmep_state;
				entry->rmep_ccm_consecutive = 0;
				util_fdprintf(fd, "set rMEP %d state to %s(%d) in mdl=%d, mep=%d CCMdb\n",
					entry->rmep_id, cfm_print_get_rmep_state_str(entry->rmep_state), entry->rmep_state,
					cfm_config->md_level, cfm_config->mep_id);
				total++;
			}
		}
	}
	if (total == 0)
		return -1;
	return 0;
}

static int
update_rmep_mac(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int rmep_id = util_atoi(argv[0]);
	unsigned char rmep_mac[6];
	cfm_pkt_rmep_entry_t *entry, *entry_next;
	int total = 0;

	if (conv_str_mac_to_mem((char *)rmep_mac, argv[1])<0)
		return -1;
	if (strcmp(argv[0], "*") == 0)
		rmep_id = 0xffff;

	list_for_each_entry_safe(entry, entry_next, &cfm_config->cfm_recv_rmep_list, node) {
		if (rmep_id == 0xffff || rmep_id == entry->rmep_id) {
			entry->rmep_timestamp = cfm_util_get_time_in_us();
			memcpy(entry->rmep_mac, rmep_mac, 6);
			entry->rmep_state = CCM_RMEP_STATE_DISCOVER; 	// mac is changed, as if we just get 1st ccm from this rmep, (DISCOVERY)
			entry->rmep_ccm_consecutive = 0;
			util_fdprintf(fd, "set rmep %d macaddr to %s in mdl=%d, mep=%d CCMdb\n",
				entry->rmep_id, util_str_macaddr(entry->rmep_mac), cfm_config->md_level, cfm_config->mep_id);
			total++;
		}
	}
	if (total == 0)
		return -1;
	return 0;
}

static int
update_ais_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ais_enable = util_atoi(argv[0]);
	if (ais_enable)
		cfm_config->mep_control |= MEP_CONTROL_AIS_ENABLE;
	else
		cfm_config->mep_control &= ~MEP_CONTROL_AIS_ENABLE;
	return 0;
}

static int
update_ais_interval(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ais_interval = util_atoi(argv[0]);
	if (ais_interval)
		cfm_config->eth_ais_control |= ETH_AIS_CONTROL_AIS_INTERVAL;
	else
		cfm_config->eth_ais_control &= (~ETH_AIS_CONTROL_AIS_INTERVAL);
	return 0;
}

static int
update_ais_pri(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ais_pri = util_atoi(argv[0]);
	if (ais_pri <0 || ais_pri >7)
		return -1;
	cfm_config->eth_ais_control &= (~ETH_AIS_CONTROL_AIS_PRIORITY);
	cfm_config->eth_ais_control |= (ais_pri<<1);
	return 0;
}

static int
update_ais_client_mdl(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ais_client_mdl = util_atoi(argv[0]);
	if (ais_client_mdl <0 || ais_client_mdl >7)
		return -1;
	cfm_config->eth_ais_control &= (~ETH_AIS_CONTROL_AIS_CLIENT_MDL);
	cfm_config->eth_ais_control |= (ais_client_mdl<<4);
	return 0;
}

static int
update_alarm_declaration_soak_time(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ms = util_atoi(argv[0]);
	if (ms < 0)
		return -1;
	cfm_config->alarm_declaration_soak_time = ms;
	return 0;
}

static int
update_alarm_clear_soak_time(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int ms = util_atoi(argv[0]);
	if (ms < 0)
		return -1;
	cfm_config->alarm_clear_soak_time = ms;
	return 0;
}

static int
update_fault_alarm_threshold(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int threshold = util_atoi(argv[0]);
	if (threshold < 0 || threshold >6)
		return -1;
	cfm_config->fault_alarm_threshold = threshold;
	return 0;
}

static int
update_lmm_dual_test_enable(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int lmm_dual_test_enable = util_atoi(argv[0]);
	cfm_config->lmm_dual_test_enable = lmm_dual_test_enable?1:0;
	return 0;
}

static int
update_payload_format(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int payload_format =  util_atoi(argv[0]);
	cfm_config->payload_format = payload_format;
	return 0;
}

static int
update_payload_pattern(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	unsigned int payload_pattern =  util_atoi(argv[0]);
	cfm_config->payload_pattern = payload_pattern;
	return 0;
}

static int
update_tlv_data_length(int fd, cfm_config_t *cfm_config, int argc, char *argv[])
{
	int tlv_data_length =  util_atoi(argv[0]);
	cfm_config->tlv_data_length = tlv_data_length;
	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_cfm_help(int fd)
{
	util_fdprintf(fd,
		"cfm help|[subcmd...]\n");
}

void
cli_cfm_help_long(int fd)
{
	util_fdprintf(fd,
		"cfm	start\n"
		"	stop\n"
		"	extract [0(disable)|1(enable)]\n"
		"	mp [md_level] [mepid]\n"
		"	mp add|del [md_level] [mepid] [port_type 0:cpu,1:uni,2:wan] [port_type_index]\n"
		"	   del ALL\n"
		"	mp config bp [md_level] [mepid] [port_type 0:cpu,1:uni,2:wan] [port_type_index] [logical_port_id]\n"
		"	          dir [md_level] [mepid] up|down\n"
		"	          type [md_level] [mepid] mep|mip\n"
		"	          macaddr [xx:xx:xx:xx:xx:xx]\n"
		"	          md_name|ma_name [md_level] [mepid] [string]\n"
		"	          md_name|ma_name [md_level] [mepid] [format] [name..]\n"
		"	          sender_id_permission [md_level] [mepid] [1:none,2:chassis,3:manage,4:chassis+manage,5:defer]\n"
		"	          admin [md_level] [mepid] [0(unlock)|1(lock)]\n"
		"	          vlan [md_level] [mepid] [vlan_id]\n"
		"	          pri [md_level] [mepid] [priority]\n"
		"	          ltr_enable|dmm_enable|lmm_enable|ccm_enable|ais_enable [md_level] [mepid] [0(disable)|1(enable)]\n"
		"	          ccm_interval [md_level] [mepid] [0..7]\n"
		"	          peer_mep_id [md_level] [mepid] [rmepid rmepid ...]\n"
		"	          rmep_state [md_level] [mepid] [rmepid] [state]\n"
		"	          rmep_mac [md_level] [mepid] [rmepid] [macaddr]\n"
		"	          ais_interval [md_level] [mepid] [0:1s,1:60s]\n"
		"	          ais_pri [md_level] [mepid] [priority]\n"
		"	          ais_client_mdl [md_level] [mepid] [0..7]\n"
		"	          alarm_declaration_soak_time [md_level] [mepid] [ms,default:2500]\n"
		"	          alarm_clear_soak_time [md_level] [mepid] [ms,default:10000]\n"
		"	          fault_alarm_threshold [md_level] [mepid] [0..6]\n"
		"	          lmm_dual_test_enable [md_level] [mepid] [0(disable)|1(enable)]\n"
		"	          auto_discovery [md_level] [mepid] [0(disable)|1(enable)]\n"
		"	          payload_format [md_level] [mepid] [0(skip)|1(data)|2(test)]\n"
		"	          payload_pattern [md_level] [mepid] [pattern, unsigned integer, default:0]\n"
		"	          tlv_data_length [md_level] [mepid] [length,default:64]\n"
		"	counter [ccm|lbm|lbr|ltm|ltr|ais|lck|dmm|dmr|1dm|lmm|lmr|slm|slr|1sl|*] [md_level] [mepid]\n"
		"	counter clear\n"
		"	show|clear lbr|ltm|ltr|ccm|test [md_level] [mepid]\n"
		);

	if (omci_env_g.cfm_y1731_enable) {
		util_fdprintf(fd,
		"	send ltm [md_level] [mepid] [remote_mepid|macaddr] [ttl] [fdbonly]\n"
		"	send lbm|dmm [md_level] [mepid] [remote_mepid|macaddr] [repeat] [interval_in_ms]\n"
		"	send 1dm [md_level] [mepid] [remote_mepid|macaddr]\n"
		"	send lmm_single|lmm_dual [md_level] [mepid] [remote_mepid] [repeat]\n"
		"	send slm_single|slm_dual [md_level] [mepid] [remote_mepid] [repeat] [interval_in_ms]\n"
		"	send tst [md_level] [mepid] [remote_mepid|macaddr] [length] [pattern_type]\n"
		"		ps: pattern_type: 0=null, 1=null+crc32, 2=prbs, 3=prbs+crc32\n"
		"	send ais|lck [md_level] [mepid] [remote_mepid|macaddr|NULL] [flags]\n"
		);
	} else {
		util_fdprintf(fd,
		"	send ltm [md_level] [mepid] [remote_mepid|macaddr] [ttl] [fdbonly]\n"
		"	send lbm [md_level] [mepid] [remote_mepid|macaddr] [repeat] [interval_in_ms]\n"
		);
	}

	if (omci_env_g.cfm_y1731_enable) {
		util_fdprintf(fd,
		"	result lbm|ltm|lmm_single|lmm_dual|slm_single|slm_dual [md_level] [mepid]\n"
		);
	} else {
		util_fdprintf(fd,
		"	result lbm|ltm [md_level] [mepid]\n"
		);
	}
}

static int
cli_cfm_mp_config(int fd, char *md_level_str, char *mep_id_str, int argc, char *argv[],
			int (*update_func)(int fd, cfm_config_t *cfm_config, int argc, char *argv[]))
{
	int md_level = util_atoi(md_level_str);
	int mep_id = util_atoi(mep_id_str);
	int mep_total = 0, update_total = 0;
	cfm_config_t *cfm_config;

	if (argc < 1)	// should be at leat one argv
		return -1;

	if (strcmp(md_level_str, "*") == 0)
		md_level = 0xff;
	if (strcmp(mep_id_str, "*") == 0)
		mep_id = 0xffff;

	// config normal mep
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if ((md_level == 0xff || md_level == cfm_config->md_level) &&
		    (mep_id == 0xffff || mep_id == cfm_config->mep_id)) {
		    	mep_total++;
		    	if (update_func(fd, cfm_config, argc, argv) == 0)
			    	update_total++;
		}
	}
	// config server mep
	if (md_level == server_mep_cfm_config_g.md_level &&
	    mep_id == server_mep_cfm_config_g.mep_id) {
	    	mep_total++;
		if (update_func(fd, &server_mep_cfm_config_g, argc, argv) == 0)
		   	update_total++;
	}

	if (mep_total == 0) {
		util_fdprintf(fd, "md level %s, mep id %s not found\n", md_level_str, mep_id_str);
		return -1;
	}
	util_fdprintf(fd, "%d meps have been updated\n", update_total);
	return update_total;
}

static int
cli_cfm_cmdline_mp(int fd, int argc, char *argv[])
{
	// for case argv[0] ="cfm", argv[1]="mp"

	// mep/mip add/del/detail /////////////////////////////////////////////////////////////////////////////////////////////

	if (argc==7 && strcmp("add", argv[2]) == 0) {
		cfm_mp_add(util_atoi(argv[3]), util_atoi(argv[4]), util_atoi(argv[5]), util_atoi(argv[6]));
		return 0;
	} else if (argc>=3 && strcmp("del", argv[2]) == 0) {
		if (argc == 7) {
			cfm_mp_del(util_atoi(argv[3]), util_atoi(argv[4]), util_atoi(argv[5]), util_atoi(argv[6]));
		} else if (argc==4 && strcmp("ALL", argv[3]) == 0) {
			cfm_mp_del_all();
		}
		return 0;

	// mep/mip attr update /////////////////////////////////////////////////////////////////////////////////////////////

	} else if (argc>=4 && strcmp("config", argv[2]) == 0) {
		if (argc==9 && strcmp("bp", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_bp) <0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("dir", argv[3]) == 0) {
			int dir = util_atoi(argv[6]); 
			if (strcmp(argv[6], "up")!=0 && strcmp(argv[6], "down")!=0 && dir !=0 && dir !=1) {
				util_fdprintf(fd, "dir should be up(1) or down(0)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_dir) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && (strcmp("type", argv[3]) == 0 || strcmp("mip", argv[3]) == 0)) {
			int type = util_atoi(argv[6]); 
			if (strcmp(argv[6], "mep")!=0 && strcmp(argv[6], "mip")!=0 && type !=0 && type !=1) {
				util_fdprintf(fd, "type should be mip(1) or mep(0)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_type) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("macaddr", argv[3]) == 0) {
			unsigned char macaddr[6];
			if (conv_str_mac_to_mem((char *)macaddr, argv[6])<0) {
				util_fdprintf(fd, "macaddr should be in format xx:xx:xx:xx:xx:xx\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_macaddr) < 0)? CLI_ERROR_SYNTAX:0;

		} else if ((argc==7||argc==8||argc==9) && strcmp("md_name", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_md_name) < 0)? CLI_ERROR_SYNTAX:0;

		} else if ((argc==7||argc==8) && strcmp("ma_name", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ma_name) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("sender_id_permission", argv[3]) == 0) {
			int sender_id_permission = util_atoi(argv[6]); 
			if (sender_id_permission <1 || sender_id_permission > 4) {
				util_fdprintf(fd, "sender_id_permission should be between 1..4\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_sender_id_permission) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("admin", argv[3]) == 0) {
			int admin = util_atoi(argv[6]); 
			if (admin !=0 && admin !=1) {
				util_fdprintf(fd, "admin should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_admin) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("vlan", argv[3]) == 0) {
			int vlan = util_atoi(argv[6]); 
			if (vlan <0 || vlan > 4094) {
				util_fdprintf(fd, "vlan should be between 0..4094\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_vlan) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("pri", argv[3]) == 0) {
			int priority = util_atoi(argv[6]);
			if (priority < 0 || priority > 7) {
				util_fdprintf(fd, "priority should be between 0..7\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_priority) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("pbit_filter", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "pbit_filter should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_pbit_filter) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("lbr_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "lbr_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_lbr_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ltr_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "ltr_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ltr_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("dmm_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "dmm_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_dmm_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("lmm_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "lmm_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_lmm_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ccm_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "ccm_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ccm_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ccm_interval", argv[3]) == 0) {
			int interval = util_atoi(argv[6]); 
			if (interval <0 && interval >7) {
				util_fdprintf(fd, "ccm_interval should be 0..7 (0:disable, 1:3.33ms, 2:10ms, 3:100ms, 4:1s, 5:10s, 6:1min, 7:10min)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ccm_interval) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("auto_discovery", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "auto_discovery should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_auto_discovery) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc>=7 && strcmp("peer_mep_id", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_peer_mep_id) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==8 && strcmp("rmep_state", argv[3]) == 0) {
			int rmep_state = util_atoi(argv[7]);
			if (rmep_state<0 || rmep_state>7) {
				util_fdprintf(fd, "rmep state should be between 0..7 (0,7:REMOVE, 1:EXPECT, 2:DISCOVER, 3:HOLD, 4:ACTIVE, 5:INACTIVE, 6:LOST)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_rmep_state) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==8 && strcmp("rmep_mac", argv[3]) == 0) {
			char rmep_mac[6];
			if (conv_str_mac_to_mem((char *)rmep_mac, argv[7])<0) {
				util_fdprintf(fd, "macaddr should be in format xx:xx:xx:xx:xx:xx\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_rmep_mac) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ais_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "ais_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ais_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ais_interval", argv[3]) == 0) {
			int interval = util_atoi(argv[6]); 
			if (interval <0 && interval >1) {
				util_fdprintf(fd, "ccm_interval should be 0(1sec)..1(60sec)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ais_interval) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ais_pri", argv[3]) == 0) {
			int ais_pri = util_atoi(argv[6]);
			if (ais_pri <0 || ais_pri >7) {
				util_fdprintf(fd, "ais priority should be between 0..7\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ais_pri) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("ais_client_mdl", argv[3]) == 0) {
			int ais_client_mdl = util_atoi(argv[6]);
			if (ais_client_mdl <0 || ais_client_mdl >7) {
				util_fdprintf(fd, "ais client md level should be between 0..7\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_ais_client_mdl) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("alarm_declaration_soak_time", argv[3]) == 0) {
			int time = util_atoi(argv[6]);
			if (time <2500 || time >10000) {
				util_fdprintf(fd, "alarm_declaration_soak_time should be between 2500..10000 (ms)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_alarm_declaration_soak_time) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("alarm_clear_soak_time", argv[3]) == 0) {
			int time = util_atoi(argv[6]);
			if (time <2500 || time >10000) {
				util_fdprintf(fd, "alarm_clear_soak_time should be between 2500..10000 (ms)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_alarm_clear_soak_time) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("fault_alarm_threshold", argv[3]) == 0) {
			int threshold = util_atoi(argv[6]);
			if (threshold <0 || threshold >6) {
				util_fdprintf(fd, "fault_alarm_threshold should be between 0(default, all without ais,rdi), 1(mask full)..6(mask none)\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_fault_alarm_threshold) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("lmm_dual_test_enable", argv[3]) == 0) {
			int enable = util_atoi(argv[6]); 
			if (enable !=0 && enable !=1) {
				util_fdprintf(fd, "lmm_dual_test_enable should be 0 or 1\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_lmm_dual_test_enable) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("payload_format", argv[3]) == 0) {
			int payload_format = util_atoi(argv[6]); 
			if (payload_format !=0 && payload_format !=1 && payload_format !=2) {
				util_fdprintf(fd, "payload_format should be 0 or 1 or 2\n");
				return CLI_ERROR_SYNTAX;
			}
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_payload_format) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("payload_pattern", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_payload_pattern) < 0)? CLI_ERROR_SYNTAX:0;

		} else if (argc==7 && strcmp("tlv_data_length", argv[3]) == 0) {
			return (cli_cfm_mp_config(fd, argv[4], argv[5], argc-6, argv+6, update_tlv_data_length) < 0)? CLI_ERROR_SYNTAX:0;
		}

	// mep/mip status/detail /////////////////////////////////////////////////////////////////////////////////////////////

	} else if (argc == 2) {
		// cfm mp
		cfm_print_all_mp_status(fd, 0xff, 0xffff);
		return 0;
	} else if (argc == 3 && (argv[2][0]>='0' && argv[2][0]<='7')) {
		// cfm mp mdl
		cfm_print_all_mp_status(fd, util_atoi(argv[2]), 0xffff);
		return 0;
	} else if (argc == 4 && (argv[2][0]>='0' && argv[2][0]<='7')) {
		// cfm mp mdl mepid
		cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[2]), util_atoi(argv[3]));
		if (cfm_config == NULL)
			return CLI_ERROR_SYNTAX;
		cfm_print_mp_detail(fd, cfm_config);
		return 0;
	}

	util_fdprintf(fd, "Invalid arguments for 'cfm mp'\n");
	return CLI_ERROR_SYNTAX;
}

int
cli_cfm_cmdline(int fd, int argc, char *argv[])
{
	unsigned char null_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if ((cfm_task_loop_g == 1) && strcmp(argv[0], "cfm") == 0) {
		if (argc==1) {
			cli_cfm_related_md(fd);
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
				cli_cfm_related_oam_meg(fd);
			return 0;
		} else if (argc==2 && strcmp(argv[1], "start") == 0) {
			util_fdprintf(fd, "cfm task had already started\n");
			return 0;
		} else if (argc==2 && strcmp(argv[1], "stop") == 0) {
			cfm_stop();
			cfm_shutdown();
			return 0;
		} else if ((argc==2||argc==3) && strcmp("extract", argv[1]) == 0) {
			int enable = 0;
			if(argc==3)
				cfm_switch_extract_set(util_atoi(argv[2])?1:0);
			cfm_switch_extract_get(&enable);
			util_fdprintf(fd, "Extraction: %s\n", (enable==1) ? "Enable" : "Disable");
			return 0;

		} else if (argc>=3 && strcmp("config", argv[1]) == 0) {
			if (strcmp(argv[2], "chassis_info") == 0) {
				if (argc == 3) {
					char *id = "NULL", *domain = "NULL", *addr = "NULL";
					if (cfm_chassis_g.chassis_id_length)
						id = cfm_chassis_g.chassis_id;
					if (cfm_chassis_g.mgmtaddr_domain_length)
						domain =  cfm_chassis_g.mgmtaddr_domain;
					if (cfm_chassis_g.mgmtaddr_length)
						addr = cfm_chassis_g.mgmtaddr;
					util_fdprintf(fd, "chassis_id=%s, mgmtaddr_domain=%s, mgmtaddr=%s\n",
						id, domain, addr);
					return 0;
				} else {
					int chassis_id_length = cfm_chassis_g.chassis_id_length;
					int mgmtaddr_domain_length = cfm_chassis_g.mgmtaddr_domain_length;
					int mgmtaddr_length = cfm_chassis_g.mgmtaddr_length;
					if (argc >=4)
						chassis_id_length = (strcmp(argv[3], "NULL")==0)?0:strlen(argv[3]);
					if (argc >=5)
						mgmtaddr_domain_length = (strcmp(argv[4], "NULL")==0)?0:strlen(argv[4]);
					if (argc >=6)
						mgmtaddr_length = (strcmp(argv[5], "NULL")==0)?0:strlen(argv[5]);
					if (chassis_id_length + mgmtaddr_domain_length + mgmtaddr_length > 54) {
						util_fdprintf(fd, "chassis_id(%d)+mgmtaddr_domain(%d)+mgmtaddr(%d) should <= 54\n",
							chassis_id_length, mgmtaddr_domain_length, mgmtaddr_length);
						return CLI_ERROR_SYNTAX;
					}
					if (argc >=4) {
						cfm_chassis_g.chassis_id_length = chassis_id_length;
						if (chassis_id_length == 0) {
							cfm_chassis_g.chassis_id[0] = 0;
						} else {
							if (cfm_chassis_g.chassis_id_length > 50)
								cfm_chassis_g.chassis_id_length = 50;
							strncpy(cfm_chassis_g.chassis_id, argv[3], cfm_chassis_g.chassis_id_length);
						}
					}
					if (argc >=5) {
						cfm_chassis_g.mgmtaddr_domain_length = mgmtaddr_domain_length;
						if (mgmtaddr_domain_length == 0) {
							cfm_chassis_g.mgmtaddr_domain[0] = 0;
						} else {
							if (cfm_chassis_g.mgmtaddr_domain_length > 50)
								cfm_chassis_g.mgmtaddr_domain_length = 50;
							strncpy(cfm_chassis_g.mgmtaddr_domain, argv[4], cfm_chassis_g.mgmtaddr_domain_length);
						}
					}
					if (argc >=6) {
						cfm_chassis_g.mgmtaddr_length = mgmtaddr_length;
						if (mgmtaddr_length == 0) {
							cfm_chassis_g.mgmtaddr[0] = 0;
						} else {
							if (cfm_chassis_g.mgmtaddr_length > 50)
								cfm_chassis_g.mgmtaddr_length = 50;
							strncpy(cfm_chassis_g.mgmtaddr, argv[5], cfm_chassis_g.mgmtaddr_length);
						}
					}
					return 0;
				}
			}

		} else if (argc==2 && strcmp("help", argv[1])==0) {
			cli_cfm_help_long(fd);
			return 0;

		} else if (argc>=2 && strcmp("counter", argv[1])==0) {
			int opcode=0xff, md_level = 0xff, mep_id = 0xffff;
			if (argc == 3) {
				if (strcmp(argv[2], "clear") == 0) {
					opcode = -1;
					md_level = -1;
					mep_id = -1;
				} else {
					if (strcmp(argv[2], "*") != 0)
						opcode = get_opcode_from_str(argv[2]);
				}
			} else if (argc == 4) {
				if (strcmp(argv[2], "*") != 0)
					opcode = get_opcode_from_str(argv[2]);
				if (strcmp(argv[3], "*") != 0)
					md_level = util_atoi(argv[3]);
			} else if (argc >= 5) {
				if (strcmp(argv[2], "*") != 0)
					opcode = get_opcode_from_str(argv[2]);
				if (strcmp(argv[3], "*") != 0)
					md_level = util_atoi(argv[3]);
				if (strcmp(argv[4], "*") != 0)
					mep_id = util_atoi(argv[4]);
			}
			cfm_print_mp_pkt_counter(fd, opcode, md_level, mep_id);
			return 0;

		} else if (argc>=3 && strcmp("show", argv[1]) == 0) {
			int md_level = 0xff, mep_id = 0xffff;
			if (argc == 4) {
				if (strcmp(argv[3], "*") != 0)
					md_level = util_atoi(argv[3]);
			} else if (argc >= 5) {
				if (strcmp(argv[3], "*") != 0)
					md_level = util_atoi(argv[3]);
				if (strcmp(argv[4], "*") != 0)
					mep_id = util_atoi(argv[4]);
			}
			if (strcmp("lbr", argv[2]) == 0) {
				cfm_print_lbr_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ltm", argv[2]) == 0) {
				cfm_print_ltm_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ltr", argv[2]) == 0) {
				cfm_print_ltr_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ccm", argv[2]) == 0) {
				cfm_print_rmep_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("test", argv[2]) == 0) {
				cfm_print_lm_test_list(fd, md_level, mep_id);
				return 0;
			}

		} else if (argc>=3 && strcmp("clear", argv[1]) == 0) {
			int md_level = 0xff, mep_id = 0xffff;
			if (argc == 4) {
				md_level = util_atoi(argv[3]);
			} else if (argc >= 5) {
				md_level = util_atoi(argv[3]);
				mep_id = util_atoi(argv[4]);
			}
			if (strcmp("lbr", argv[2]) == 0) {
				cfm_clear_lbr_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ltm", argv[2]) == 0) {
				cfm_clear_ltm_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ltr", argv[2]) == 0) {
				cfm_clear_ltr_list(fd, md_level, mep_id);
				return 0;
			} else if (strcmp("ccm", argv[2]) == 0) {
				cfm_clear_rmep_list(fd, md_level, mep_id);
				return 0;
			}

		// mep/mip add/del/config /////////////////////////////////////////////////////////////////////////////////////////////
		} else if (argc >=2 &&  strcmp("mp", argv[1]) == 0) {
			return cli_cfm_cmdline_mp(fd, argc, argv);

		// cfm message send & recv result /////////////////////////////////////////////////////////////////////////////////////////////

		} else if (argc>=3 && strcmp("send", argv[1]) == 0) {

			if (argc==6 && strcmp("lbm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				int i;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				cfm_send_lbm(cfm_config, mac, -1, 0);

				// try to wait reply, so server prompt wont show earlier than result on cli client
				for (i=0; i<100; i++) {
					if (!list_empty(&cfm_config->cfm_recv_lbr_list))
						break;
					usleep(10*1000);	// 10ms
				}
				return 0;

			} else if (argc>=7 && strcmp("lbm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				int repeat = util_atoi(argv[6]);
				int interval = 1000;	// unit:ms
				unsigned int lbr_lost = 0, lbr_recv = 0;
				unsigned int resptime_max = 0, resptime_min = 10000, resptime_avg = 0, resptime_mdev = 0;
				unsigned long resptime_total = 0, resptime_total2 = 0;
				cfm_pkt_lbr_entry_t *entry;
				int i, t;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				if (repeat < 1)
					repeat = 1;
				if (repeat > 1000)
					repeat = 1000;
				if (argc>=8) {
					interval = util_atoi(argv[7]);
					if (interval < 100)
						interval = 100;
					if (interval > 10000)
						interval = 10000;
				}

				for (i=0; i< repeat; i++) {
					unsigned long long start = cfm_util_get_time_in_us();
					cfm_send_lbm(cfm_config, mac, -1, 0);
					dbprintf(LOG_WARNING, "LBM to %s: xid=0x%x, sent_time=%u.%06us\n",
						util_str_macaddr(mac), entry->u.lbr.lbr_pdu.xid, (unsigned int)start/1000000, (unsigned int)start%1000000);

					for (t=0; t<10; t++) {	// wait reply up to 100ms
						usleep(10*1000);	// 10ms
						if (!list_empty(&cfm_config->cfm_recv_lbr_list))
							break;
					}
					if (list_empty(&cfm_config->cfm_recv_lbr_list)) {
						lbr_lost++;
					} else {
						list_for_each_entry(entry, &cfm_config->cfm_recv_lbr_list, node) {
							unsigned int resptime = entry->lbr_timestamp - cfm_config->cfm_send_lbm_timestamp;
							if (resptime < resptime_min)
								resptime_min = resptime;
							if (resptime > resptime_max)
								resptime_max = resptime;
							resptime_total += resptime;
							resptime_total2 += resptime * resptime;
							lbr_recv++;
						}
					}
					if (i<repeat-1) {
						unsigned long long time_passed = cfm_util_get_time_in_us() - start;
						if (interval*1000ULL > time_passed)
							usleep(interval*1000ULL - time_passed);
					}
				}

				if( lbr_recv != 0 ) {
					resptime_avg = resptime_total/lbr_recv;
					// definition: mdev = SQRT(SUM(RTT*RTT) / N – (SUM(RTT)/N)^2)
					resptime_mdev = sqrt(resptime_total2/lbr_recv - resptime_avg*resptime_avg);

					util_fdprintf(fd, "%d LBM sent, %u LBR received, %u LBM loss, time %u.%03ums\n",
						repeat, lbr_recv, lbr_lost, (unsigned int)(resptime_total/1000), (unsigned int)(resptime_total%1000));
					util_fdprintf(fd, "rtt min/avg/max/mdev = %u.%03u/%u.%03u/%u.%03u/%u.%03u ms\n",
						resptime_min/1000, resptime_min%1000,
						resptime_avg/1000, resptime_avg%1000,
						resptime_max/1000, resptime_max%1000,
						resptime_mdev/1000, resptime_mdev%1000);
				} else {
					util_fdprintf(fd, "%d LBM sent, %u LBR received, %u LBM loss\n", repeat, lbr_recv, lbr_lost);
				}
				return 0;

			} else if (argc>=6 && strcmp("ltm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				int ttl = 64;
				int is_fdbonly = 1;
				int i;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				if (argc>=7) {
					ttl = util_atoi(argv[6]);
					if (ttl <= 0)
						ttl = 64;
					if (ttl > 255)
						ttl = 255;
				}
				if (argc>=8) {
					is_fdbonly = util_atoi(argv[7])?1:0;
				}
				cfm_send_ltm(cfm_config, mac, -1, 0, ttl, is_fdbonly);

				// try to wait reply, so server prompt wont show earlier than result on cli client
				for (i=0; i<100; i++) {
					if (!list_empty(&cfm_config->cfm_recv_ltr_list))
						break;
					usleep(10*1000);	// 10ms
				}
				return 0;

			} else if (argc==7 && strcmp("ais", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				char flags = util_atoi(argv[6]);

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (strcmp(argv[5], "NULL") !=0 && get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				if(flags < 0 || flags > 7) {
					util_fdprintf(fd, "Valid flags: 0~7 (%d)\n",util_atoi(argv[6]));
					return CLI_ERROR_SYNTAX;
				}
				if (strcmp(argv[5], "NULL") == 0) {
					cfm_send_ais(cfm_config, "NULL", -1, 0, flags);
				} else {
					cfm_send_ais(cfm_config, mac, -1, 0, flags);
				}
				util_fdprintf(fd, "AIS Sent: flags:%x\n", flags);
				return 0;

			} else if (argc==7 && strcmp("lck", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				char flags = util_atoi(argv[6]);

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (strcmp(argv[5], "NULL") !=0 && get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				if(flags < 0 || flags > 7) {
					util_fdprintf(fd, "Valid flags: 0~7 (%d)\n",util_atoi(argv[6]));
					return CLI_ERROR_SYNTAX;
				}
				if (strcmp(argv[5], "NULL") == 0) {
					cfm_send_lck(cfm_config, "NULL", -1, 0, flags);
				} else {
					cfm_send_lck(cfm_config, mac, -1, 0, flags);
				}
				util_fdprintf(fd, "LCK Sent: flags:%x\n", flags);
				return 0;

			} else if (argc==6 && strcmp("dmm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				int i;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				cfm_send_dmm(cfm_config, mac, -1, 0);

				// try to wait reply, so server prompt wont show earlier than result on cli client
				for (i=0; i<100; i++) {
					if (memcmp(null_mac, cfm_config->cfm_recv_dmr_mac, IFHWADDRLEN)!=0)
						break;
					usleep(10*1000);	// 10ms
				}
				return 0;

			} else if (argc>=7 && strcmp("dmm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				int repeat = util_atoi(argv[6]);
				int interval = 1000;	// unit:ms
				unsigned int dmr_lost = 0, dmr_recv = 0;
				unsigned int resptime_max = 0, resptime_min = 10000, resptime_avg = 0, resptime_mdev = 0;
				unsigned int resptime_total = 0, resptime_total2 = 0;
				int i, t;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				if (repeat < 1)
					repeat = 1;
				if (repeat > 1000)
					repeat = 1000;
				if (argc>=8) {
					interval = util_atoi(argv[7]);
					if (interval < 100)
						interval = 100;
					if (interval > 10000)
						interval = 10000;
				}

				for (i=0; i< repeat; i++) {
					unsigned long long start = cfm_util_get_time_in_us();

					cfm_send_dmm(cfm_config, mac, -1, 0);

					dbprintf(LOG_WARNING, "DMM to %s: sent_time=%u.%06us\n",
						util_str_macaddr(mac), (unsigned int)start/1000000, (unsigned int)start%1000000);

					for (t=0; t<10; t++) {	// wait reply up to 100ms
						usleep(10*1000);	// 10ms
						if(!memcmp(null_mac, cfm_config->cfm_recv_dmr_mac, IFHWADDRLEN)==0)
							break;
					}

					if( memcmp(null_mac, cfm_config->cfm_recv_dmr_mac, IFHWADDRLEN)==0) {
						dmr_lost++;
					} else {
						unsigned int resptime = cfm_config->frame_delay;

						if (resptime < resptime_min)
							resptime_min = resptime;
						if (resptime > resptime_max)
							resptime_max = resptime;

						resptime_total += resptime;
						resptime_total2 += resptime * resptime;
						dmr_recv++;
					}

					if (i<repeat-1) {
						unsigned long long time_passed = cfm_util_get_time_in_us() - start;
						if (interval*1000ULL > time_passed)
							usleep(interval*1000ULL - time_passed);
					}
				}

				if( dmr_recv != 0 ) {
					resptime_avg = resptime_total/dmr_recv;

					// definition: mdev = SQRT(SUM(RTT*RTT) / N – (SUM(RTT)/N)^2)
					resptime_mdev = sqrt(resptime_total2/dmr_recv - resptime_avg*resptime_avg);

					util_fdprintf(fd, "%d DMM sent, %u DMR received, %u DMM loss, time %u.%03ums\n",
						repeat, dmr_recv, dmr_lost, (unsigned int)(resptime_total/1000), (unsigned int)(resptime_total%1000));

					util_fdprintf(fd, "rtt min/avg/max/mdev = %u.%03u/%u.%03u/%u.%03u/%u.%03u ms\n",
						resptime_min/1000, resptime_min%1000,
						resptime_avg/1000, resptime_avg%1000,
						resptime_max/1000, resptime_max%1000,
						resptime_mdev/1000, resptime_mdev%1000);
				} else {
					util_fdprintf(fd, "%d DMM sent, %u DMR received, %u DMM loss\n", repeat, dmr_recv, dmr_lost);
				}
				return 0;

			} else if (argc==6 && strcmp("1dm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				cfm_send_1dm(cfm_config, mac, -1, 0);
				//util_fdprintf(fd, "1DM Sent: TxTimestampf:%llusec,%lluus\n", cfm_config->TxTimestampf/1000000, cfm_config->TxTimestampf%1000000);
				return 0;
			} else if (argc==6 && strcmp("tst", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				cfm_send_tst(cfm_config, mac, -1, 0);
				return 0;
			} else if (argc==8 && strcmp("tst", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				unsigned char mac[6];
				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				cfm_config->tlv_tst_length=util_atoi(argv[6]);

				if(cfm_config->tlv_tst_length < 34 || cfm_config->tlv_tst_length > 1480) {
					util_fdprintf(fd, "Suggest 34 <= TST TLV length <= 1480\n");
					return CLI_ERROR_SYNTAX;
				}

				cfm_config->tlv_tst_pattern_type=util_atoi(argv[7]);
				if(cfm_config->tlv_tst_pattern_type > 3) {
					util_fdprintf(fd, "Suggest 0 <= TST pattern type<= 3\n");
					return CLI_ERROR_SYNTAX;
				}

				cfm_send_tst(cfm_config, mac, -1, 0);
				return 0;
			} else if (argc>=7 && strcmp("lmm_single", argv[2]) == 0) {
				int md_level = util_atoi(argv[3]);
				int mep_id = util_atoi(argv[4]);
				unsigned short rmep_id = util_atoi(argv[5]);
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, md_level, mep_id);
				unsigned char mac[6];
				cfm_cmd_msg_t *cfm_cmd_msg;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
				INIT_LIST_NODE(&cfm_cmd_msg->q_entry);
				cfm_cmd_msg->mdlevel= md_level;
				cfm_cmd_msg->rmepid = rmep_id;
				cfm_cmd_msg->repeat = util_atoi(argv[6]);
				if (argc == 8 )
					cfm_cmd_msg->interval = util_atoi(argv[7]);
				else 
					cfm_cmd_msg->interval = cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval);
				cfm_cmd_msg->usr_data = (void*)cfm_config;
				cfm_cmd_msg->cmd = CFM_CMD_ETH_LM_SINGLE_START;
				if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0) {
					dbprintf(LOG_ERR, "Cmd CFM_CMD_ETH_LM_SINGLE_START fail!\n");
					free_safe(cfm_cmd_msg);
				}
				return 0;

			} else if (argc>=7 && strcmp("lmm_dual", argv[2]) == 0) {
				int md_level = util_atoi(argv[3]);
				int mep_id = util_atoi(argv[4]);
				unsigned short rmep_id = util_atoi(argv[5]);
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, md_level, mep_id);
				unsigned char mac[6];
				cfm_cmd_msg_t *cfm_cmd_msg;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;
				if (cfm_config->lmm_dual_test_enable == 0) {
					util_fdprintf(fd, "please do 'cfm mp config lmm_dual_test_enable %d %d 1' first\n",
						md_level, mep_id);
					return CLI_ERROR_SYNTAX;
				}

				cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
				INIT_LIST_NODE(&cfm_cmd_msg->q_entry);
				cfm_cmd_msg->mdlevel= md_level; //use classid field to store md_level value
				cfm_cmd_msg->rmepid = rmep_id; 	//use rmeid field to store mep_id value
				cfm_cmd_msg->usr_data = (void*)cfm_config;
				cfm_cmd_msg->repeat = util_atoi(argv[6]);
				if (argc == 8 )
					cfm_cmd_msg->interval = util_atoi(argv[7]);
				else 
					cfm_cmd_msg->interval = cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval);
				cfm_cmd_msg->cmd = CFM_CMD_ETH_LM_DUAL_START;
				if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0) {
					dbprintf(LOG_ERR, "Cmd CFM_CMD_ETH_LM_DUAL_START fail!\n");
					free_safe(cfm_cmd_msg);
				}
				return 0;

			} else if (argc==8 && strcmp("slm_single", argv[2]) == 0) {
				int md_level = util_atoi(argv[3]);
				int mep_id = util_atoi(argv[4]);
				unsigned short rmep_id = util_atoi(argv[5]);
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, md_level, mep_id);
				unsigned char mac[6];
				cfm_cmd_msg_t *cfm_cmd_msg;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
				INIT_LIST_NODE(&cfm_cmd_msg->q_entry);
				cfm_cmd_msg->mdlevel= md_level; //use classid field to store md_level value
				cfm_cmd_msg->rmepid = rmep_id; 	//use rmeid field to store mep_id value
				cfm_cmd_msg->usr_data = (void*)cfm_config;
				cfm_cmd_msg->repeat = util_atoi(argv[6]);
				cfm_cmd_msg->interval = util_atoi(argv[7]);
				cfm_cmd_msg->cmd = CFM_CMD_ETH_SLM_SINGLE_START;
				if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0) {
					dbprintf(LOG_ERR, "Cmd CFM_CMD_ETH_SLM_SINGLE_START fail!\n");
					free_safe(cfm_cmd_msg);
				}
				return 0;

			} else if (argc==8 && strcmp("slm_dual", argv[2]) == 0) {
				int md_level = util_atoi(argv[3]);
				int mep_id = util_atoi(argv[4]);
				unsigned short rmep_id = util_atoi(argv[5]);
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, md_level, mep_id);
				unsigned char mac[6];
				cfm_cmd_msg_t *cfm_cmd_msg;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if (get_mac_for_rmep(fd, mac, cfm_config, argv[5]) <0)
					return CLI_ERROR_SYNTAX;

				cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
				INIT_LIST_NODE(&cfm_cmd_msg->q_entry);
				cfm_cmd_msg->mdlevel= md_level; //use classid field to store md_level value
				cfm_cmd_msg->rmepid = rmep_id; 	//use rmeid field to store mep_id value
				cfm_cmd_msg->usr_data = (void*)cfm_config;
				cfm_cmd_msg->repeat = util_atoi(argv[6]);
				cfm_cmd_msg->interval = util_atoi(argv[7]);
				cfm_cmd_msg->cmd = CFM_CMD_ETH_SLM_DUAL_START;
				if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0) {
					dbprintf(LOG_ERR, "Cmd CFM_CMD_ETH_SLM_DUAL_START fail!\n");
					free_safe(cfm_cmd_msg);
				}
				return 0;
			}

		} else if (argc>=3 && strcmp("result", argv[1]) == 0) {

			if (argc==5 && strcmp("lbm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				int total = 0;
				cfm_pkt_lbr_entry_t *entry;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				list_for_each_entry(entry, &cfm_config->cfm_recv_lbr_list, node) {
					unsigned int resptime = entry->lbr_timestamp - cfm_config->cfm_send_lbm_timestamp;
					util_fdprintf(fd, "LBR from %s: xid=0x%x, time=%u.%03ums\n",
						util_str_macaddr(entry->lbr_mac), entry->u.lbr.lbr_pdu.xid,
						resptime/1000, resptime%1000);
					total++;
				}
				if (total == 0)
					util_fdprintf(fd, "No result\n");
				else
					util_fdprintf(fd, "Total %d LBRs\n", total);
				return 0;

			} else if (argc==5 && strcmp("ltm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				int total = 0;
				cfm_pkt_ltr_entry_t *entry = NULL;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;

				list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
					unsigned int resptime = entry->ltr_timestamp - cfm_config->cfm_send_ltm_timestamp;
					util_fdprintf(fd, "LTR from %s: xid=0x%x, ttl=%d, time=%u.%03ums\n",
						util_str_macaddr(entry->ltr_mac), entry->u.ltr.ltr_pdu.xid, entry->u.ltr.ltr_pdu.ttl,
						resptime/1000, resptime%1000);
					total++;
				}

				if (total == 0)
					util_fdprintf(fd, "No result\n");
				else
					util_fdprintf(fd, "Total %d LTRs\n", total);
				return 0;

			} else if (argc==5 && strcmp("dmm", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				if(memcmp(null_mac, cfm_config->cfm_recv_dmr_mac, IFHWADDRLEN)==0) {
					util_fdprintf(fd, "No result\n");
					return 0;
				}
				util_fdprintf(fd, "DMR from %s: two way frame delay: time=%llu.%03llums\n",
					util_str_macaddr(cfm_config->cfm_recv_dmr_mac),
					cfm_config->frame_delay/1000, cfm_config->frame_delay%1000);
				return 0;

			} else if (argc>=4 && strcmp("lmm_single", argv[2]) == 0) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				cfm_pkt_rmep_entry_t *entry;
				unsigned char sleep_en = 0;
				int total = 0;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if (entry->eth_lm_single_stop_timer > 0) {
						fwk_timer_delete( entry->eth_lm_single_stop_timer );
						entry->eth_lm_single_repeat = 0;
						entry->eth_lm_single_force_stop = 1;
						entry->eth_lm_single_stop_timer =
							fwk_timer_create(cfm_timer_qid_g,
									CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP,
									1000,
									(void *)cfm_config);
						sleep_en = 1;
					}
				}
				if ( sleep_en )
					sleep(2);

				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					int floss = entry->eth_lm_single_tx_floss - entry->eth_lm_single_rx_floss;
					int nloss = entry->eth_lm_single_tx_nloss - entry->eth_lm_single_rx_nloss;

					if (	entry->eth_lm_single_tx_floss == 0 &&
						entry->eth_lm_single_rx_floss == 0 &&
						entry->eth_lm_single_tx_nloss == 0 &&
						entry->eth_lm_single_rx_nloss == 0)
						continue;
					if (total == 0)
						util_fdprintf( fd, "Loss Mesaurement (single):\n");
					total++;

					if (floss > 0 && entry->eth_lm_single_tx_floss > 0)
						entry->eth_lm_single_fratio = floss*10000/entry->eth_lm_single_tx_floss;
					if (nloss > 0 && entry->eth_lm_single_tx_nloss > 0)
						entry->eth_lm_single_nratio = nloss*10000/entry->eth_lm_single_tx_nloss;
					util_fdprintf( fd,
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, entry->rmep_id,
						floss, entry->eth_lm_single_tx_floss, entry->eth_lm_single_rx_floss, entry->eth_lm_single_fratio/100, entry->eth_lm_single_fratio%100,
						nloss, entry->eth_lm_single_tx_nloss, entry->eth_lm_single_rx_nloss, entry->eth_lm_single_nratio/100, entry->eth_lm_single_nratio%100);
						entry->eth_lm_single_force_stop = 0;
				}
				if (!total)
					util_fdprintf( fd, "No result\n");

				return 0;

			} else if (argc >= 4 && strcmp("lmm_dual", argv[2]) == 0 ) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				cfm_pkt_rmep_entry_t *entry;
				unsigned char sleep_en = 0;
				int total = 0;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if ( entry->eth_lm_dual_stop_timer > 0) {
						entry->eth_lm_dual_force_stop = 1;
						fwk_timer_delete( entry->eth_lm_dual_stop_timer );
						entry->eth_lm_dual_stop_timer =
							fwk_timer_create(cfm_timer_qid_g,
									CFM_TIMER_EVENT_ETH_LM_DUAL_STOP,
									1000,
									(void *)cfm_config);
						sleep_en = 1;
					}
				}
				if ( sleep_en )
					sleep(2);

				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					int floss = entry->eth_lm_dual_tx_floss - entry->eth_lm_dual_rx_floss;
					int nloss = entry->eth_lm_dual_tx_nloss - entry->eth_lm_dual_rx_nloss;

					if (	entry->eth_lm_dual_tx_floss == 0 &&
						entry->eth_lm_dual_rx_floss == 0 &&
						entry->eth_lm_dual_tx_nloss == 0 &&
						entry->eth_lm_dual_rx_nloss == 0)
						continue;
					if (total == 0)
						util_fdprintf( fd, "Loss Mesaurement (dual):\n");
					total++;

					if (floss > 0 && entry->eth_lm_dual_tx_floss > 0)
						entry->eth_lm_dual_fratio = floss*10000/entry->eth_lm_dual_tx_floss;
					if (nloss > 0 && entry->eth_lm_dual_tx_nloss > 0)
						entry->eth_lm_dual_nratio = nloss*10000/entry->eth_lm_dual_tx_nloss;
					util_fdprintf( fd,
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, entry->rmep_id,
						floss, entry->eth_lm_dual_tx_floss, entry->eth_lm_dual_rx_floss, entry->eth_lm_dual_fratio/100, entry->eth_lm_dual_fratio%100,
						nloss, entry->eth_lm_dual_tx_nloss, entry->eth_lm_dual_rx_nloss, entry->eth_lm_dual_nratio/100, entry->eth_lm_dual_nratio%100);
					entry->eth_lm_dual_force_stop = 0;
				}
				if (!total)
					util_fdprintf( fd, "No result\n");
				return 0;

			} else if (argc == 5 && strcmp("slm_single", argv[2]) == 0 ) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				cfm_pkt_rmep_entry_t *entry;
				unsigned char sleep_en = 0;
				int total = 0;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if ( entry->eth_slm_single_sender_loop_timer > 0) {
						fwk_timer_delete( entry->eth_slm_single_sender_stop_timer );
						entry->eth_slm_single_sender_stop_timer = 0;
						entry->eth_slm_single_repeat = 0;
						entry->eth_slm_single_force_stop = 1;
						sleep_en = 1;
					}
				}
				if ( sleep_en )
					sleep(2);

				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					int floss = entry->eth_slm_single_tx_floss - entry->eth_slm_single_rx_floss;
					int nloss = entry->eth_slm_single_tx_nloss - entry->eth_slm_single_rx_nloss;

					if (	entry->eth_slm_single_tx_floss == 0 &&
						entry->eth_slm_single_rx_floss == 0 &&
						entry->eth_slm_single_tx_nloss == 0 &&
						entry->eth_slm_single_rx_nloss == 0)
						continue;
					if (total == 0)
						util_fdprintf( fd, "Synthetic Loss (single):\n");
					total++;

					if (floss > 0 && entry->eth_slm_single_tx_floss > 0)
						entry->eth_slm_single_fratio = floss*10000/entry->eth_slm_single_tx_floss;
					if (nloss > 0 && entry->eth_slm_single_tx_nloss > 0)
						entry->eth_slm_single_nratio = nloss*10000/entry->eth_slm_single_tx_nloss;
					util_fdprintf( fd,
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, entry->rmep_id,
						floss, entry->eth_slm_single_tx_floss, entry->eth_slm_single_rx_floss, entry->eth_slm_single_fratio/100, entry->eth_slm_single_fratio%100,
						nloss, entry->eth_slm_single_tx_nloss, entry->eth_slm_single_rx_nloss, entry->eth_slm_single_nratio/100, entry->eth_slm_single_nratio%100);
					entry->eth_slm_single_force_stop = 0;
				}
				if (!total)
					util_fdprintf( fd, "No result\n");
				return 0;


			} else if (argc == 5 && strcmp("slm_dual", argv[2]) == 0 ) {
				cfm_config_t *cfm_config = get_cfm_config_by_md_level_mp_id(fd, util_atoi(argv[3]), util_atoi(argv[4]));
				cfm_pkt_rmep_entry_t *entry;
				unsigned char sleep_en = 0;
				int total = 0;

				if (cfm_config == NULL)
					return CLI_ERROR_SYNTAX;
				//as a 1SL PDU sender
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if (entry->eth_slm_dual_repeat > 0 ) {
						entry->eth_slm_dual_repeat = 0;
					}
				}
				//as a 1SL PDU receiver
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if ( entry->eth_slm_dual_recv_timer > 0 ) {
						fwk_timer_delete( entry->eth_slm_dual_recv_timer );
						entry->eth_slm_dual_force_stop = 1;
						entry->eth_slm_dual_recv_timer =
							fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK, 500 , (void *)cfm_config);
						sleep_en = 1;
					}
				}
				if ( sleep_en )
					sleep(1);

				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					int nloss = entry->eth_slm_dual_tx_nloss - entry->eth_slm_dual_rx_nloss;
					unsigned int MAYBE_UNUSED nratio = 0;

					if (	entry->eth_slm_dual_tx_nloss == 0 &&
						entry->eth_slm_dual_rx_nloss == 0)
						continue;
					if (total == 0)
						util_fdprintf( fd, "Synthetic Loss Mesaurement (dual):\n");
					total++;

					if (nloss > 0 && entry->eth_slm_dual_tx_nloss > 0)
						nratio = nloss*10000/entry->eth_slm_dual_tx_nloss;

					util_fdprintf( fd,
						"md_level %d, mep %d, rmep %d: testid %u\n"
						"loss nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, entry->rmep_id, entry->test_id_1sl,
						nloss, entry->eth_slm_dual_tx_nloss, entry->eth_slm_dual_rx_nloss, entry->eth_slm_dual_nratio/100, entry->eth_slm_dual_nratio%100);
					entry->eth_slm_dual_force_stop = 0;
				}
				if (!total)
					util_fdprintf( fd, "No result\n");
				return 0;
			}
		}
		util_fdprintf(fd, "Invalid command. Valid commands:\n");
		cli_cfm_help_long(fd);
		return CLI_ERROR_SYNTAX;
	} else if ((cfm_task_loop_g == 0) && strcmp(argv[0], "cfm") == 0) {
		cfm_init();
		cfm_start();
		return 0;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}
