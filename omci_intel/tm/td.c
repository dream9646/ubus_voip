/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : map wan egress td, gem us/ds td, uni ingress/egress td to hw ratelimit facility
 * Module  : tm
 * File    : td.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>

#include "util.h"
#include "tm.h"
#include "meinfo.h"
#include "me_related.h"
#include "hwresource.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"
#include "omciutil_misc.h"
#include "proprietary_alu.h"
#include "proprietary_calix.h"

static inline int
min(int a, int b) {
	if(a > b)
		return b;
	else
		return a;
}

static inline int
non_zero_min(int a, int b) {
	if(a==0)
		return b;
	if(b==0)
		return a;
	if(a > b)
		return b;
	else
		return a;
}

static int
gemlist_to_unique_gemlist(int gemport_count, struct me_t *gem_list[8], struct me_t *unique_gem_list[8])
{
	int unique_gemport_count=0;
	int i, j;

	//collect unique_gem_list
	for (j=0; j<8; j++)
		unique_gem_list[j] = NULL;

	for(i=0; i < gemport_count; i++) {
		int is_gem_exist=0;
		for(j=0; j < unique_gemport_count; j++) {
			if(gem_list[i] == unique_gem_list[j]) {
				is_gem_exist=1;
				break;
			}
		}
		if(!is_gem_exist) {
			unique_gem_list[unique_gemport_count]=gem_list[i];
			unique_gemport_count++;
		}
	}
	return unique_gemport_count;
}

static int
find_gem_related_traffic_descriptor(struct me_t *me_gem, struct me_t **me_td_up, struct me_t **me_td_down)
{
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me_td;
	unsigned char found=0;

	list_for_each_entry(me_td, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me_td)) {
			found=1;
			if ((unsigned short)meinfo_util_me_attr_data(me_gem, 5)==me_td->meid) {
				*me_td_up=me_td;
			}
			if ((unsigned short)meinfo_util_me_attr_data(me_gem, 9)==me_td->meid) {
				*me_td_down=me_td;
			}
		}
	}
	return found;
}

static int
find_bport_related_traffic_descriptor(struct me_t *me_bport, struct me_t **me_td_out, struct me_t **me_td_in)
{
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me_td;
	unsigned char found=0;

	list_for_each_entry(me_td, &miptr->me_instance_list, instance_node) {
		if (me_related(me_bport, me_td)) {
			found=1;
			//dbprintf_bat(LOG_ERR, "\n[280]traffic_desc:0x%x,cir=%u,pir=%u\n", me_td->meid,
			//	(unsigned int)meinfo_util_me_attr_data(me_td, 1),
			//	(unsigned int)meinfo_util_me_attr_data(me_td, 2));
			if ((unsigned short)meinfo_util_me_attr_data(me_bport, 11)==me_td->meid) {
				*me_td_out=me_td;
			}
			if ((unsigned short)meinfo_util_me_attr_data(me_bport, 12)==me_td->meid) {
				*me_td_in=me_td;
			}
		}
	}
	return found;
}

// td_usgem_wanport_... is for upstream td on GEM, outbound td on WAN ////////////////////////////////////////////////////////////////

static int
td_pq_cirpir_from_usgem_wanport(struct batchtab_cb_td_t *t, struct me_t *me_bport_wan, struct me_t *me_gem, struct me_t *me_pq, char is_yellow)
{
	int tcont_id, phy_queueid, is_policy_wrr, hw_weight;				
	struct me_t *me_td_up, *me_td_down;
	struct wan_td_node_t *wan_td_node, *tmp_wan_td_node;
	int found;

	if (tm_get_tcont_id_phy_queue_info_by_me_pq_gem(me_pq, me_gem, is_yellow, &tcont_id, &phy_queueid, &is_policy_wrr, &hw_weight)<=0) {
		dbprintf_bat(LOG_NOTICE, "Can't find phy_queueid, continue\n");
		return -1;
	}
	if( phy_queueid < 0)
		return -1;

	// sum up gem port for to specific pq //////////////////////////////////////////////

	me_td_up=NULL;
	me_td_down=NULL;
	//FIXME! maybe have quick way to get me_td
	if( find_gem_related_traffic_descriptor(me_gem, &me_td_up, &me_td_down)== 0) {
		dbprintf_bat(LOG_NOTICE, "Can't find related td\n");
		//even no limit still need to do
		//continue;
	}

	//collect cir/pir
	t->td_usgem_wanport_map[phy_queueid].is_used=1;
	t->td_usgem_wanport_map[phy_queueid].tcont_id=tcont_id;
	t->td_usgem_wanport_map[phy_queueid].wanport_meid=me_bport_wan->meid;	// only keep the last wanport mapped to this pq

	if(me_td_up != NULL) {	//upstream
		unsigned long long gem_td_cir_up=(unsigned int)meinfo_util_me_attr_data(me_td_up, 1); //cir
		unsigned long long gem_td_pir_up=(unsigned int)meinfo_util_me_attr_data(me_td_up, 2); //pir

		t->td_usgem_wanport_map[phy_queueid].is_gem_td_up_null = 0;

		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
			if (proprietary_alu_get_olt_version() == 0) {
				// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
				gem_td_cir_up=gem_td_cir_up*125;
			}
		}
		// this rate limitation is data rate, not ethernet line rate,
		// multiple a factor to meet cht requirement that under 10% inaccuracy
		gem_td_cir_up=gem_td_cir_up*omci_env_g.tm_gemus_rate_factor/100;

		t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up+=gem_td_cir_up;

		if( t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up > GPON_US_WIRESPEED_BYTE_PER_SEC )
			t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up = GPON_US_WIRESPEED_BYTE_PER_SEC;

		//end of cir

		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
			if (proprietary_alu_get_olt_version() == 0) {
				// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
				gem_td_pir_up=gem_td_pir_up*125;
			}
		}
		// this rate limitation is data rate, not ethernet line rate,
		// multiple a factor to meet cht requirement that under 10% inaccuracy
		gem_td_pir_up=gem_td_pir_up*omci_env_g.tm_gemus_rate_factor/100;

		t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up+=gem_td_pir_up;

		if( t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up > GPON_US_WIRESPEED_BYTE_PER_SEC )
			t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up = GPON_US_WIRESPEED_BYTE_PER_SEC;

	} else {//no upstream rate limit
		t->td_usgem_wanport_map[phy_queueid].is_gem_td_up_null = 1;
		t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up = 0;
		t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up = GPON_US_WIRESPEED_BYTE_PER_SEC;
	}

	if(is_yellow) {
		t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up = 0;
	}

	// sum up wan port td for specific pq //////////////////////////////////////////////

	//check if there is wan port rate limit keep in td_usgem_wanport_map[phy_queueid].wan_td_list
	found=0;
	list_for_each_entry(tmp_wan_td_node, &t->td_usgem_wanport_map[phy_queueid].wan_td_list, wan_td_entry_node) {
		if(tmp_wan_td_node->me_bport_wan == me_bport_wan) {
			found=1;	//rate limit in this wan bridge port already set to this phy_queueid
			break;
		}
	}
	if( found==0 ) {
		struct me_t *me_td_outbound, *me_td_inbound;

		me_td_outbound=NULL;
		me_td_inbound=NULL;

		find_bport_related_traffic_descriptor(me_bport_wan, &me_td_outbound, &me_td_inbound);

		wan_td_node=malloc_safe(sizeof(struct wan_td_node_t));
		wan_td_node->me_bport_wan=me_bport_wan;
		wan_td_node->me_td_outbound=me_td_outbound;
		wan_td_node->me_td_inbound=me_td_inbound;
		list_add_tail(&wan_td_node->wan_td_entry_node, &t->td_usgem_wanport_map[phy_queueid].wan_td_list);
		//collect cir/pir in/outbound
		if(me_td_outbound != NULL) {	//outbound
			unsigned long long wan_td_cir=(unsigned int)meinfo_util_me_attr_data(me_td_outbound, 1);//cir
			unsigned long long wan_td_pir=(unsigned int)meinfo_util_me_attr_data(me_td_outbound, 2);//pir
			t->td_usgem_wanport_map[phy_queueid].is_wan_td_outbound_null= 0;

			// CIR
			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
				// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
				if (proprietary_alu_get_olt_version() == 0)
					wan_td_cir=wan_td_cir*125;
			}
			wan_td_cir=wan_td_cir*omci_env_g.tm_gemus_rate_factor/100;
			t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound += wan_td_cir;
			if( t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound > GPON_US_WIRESPEED_BYTE_PER_SEC )
				t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound = GPON_US_WIRESPEED_BYTE_PER_SEC;

			// PIR
			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
				// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
				if (proprietary_alu_get_olt_version() == 0)
					wan_td_pir=wan_td_pir*125;
			}
			wan_td_pir=wan_td_pir*omci_env_g.tm_gemus_rate_factor/100;
			t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound += wan_td_pir;
			if( t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound > GPON_US_WIRESPEED_BYTE_PER_SEC )
				t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound = GPON_US_WIRESPEED_BYTE_PER_SEC;

		} else {//no outbound rate limit
			t->td_usgem_wanport_map[phy_queueid].is_wan_td_outbound_null= 1;
			t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound= 0;
			t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound= GPON_US_WIRESPEED_BYTE_PER_SEC;
		}
	}

	return 0;
}

static int
td_pq_cirpir_from_pq_supplemental(struct batchtab_cb_td_t *t, struct me_t *me_gem, struct me_t *me_pq, char is_yellow)
{
	unsigned short pq_supplemental_meid = me_pq->meid;
	int tcont_id, phy_queueid, is_policy_wrr, hw_weight;				
	struct me_t *me_td_up, *me_pq_supplemental;
	unsigned short td_up_meid;

	// pq_supplemental not found or it has no related td
	if ((me_pq_supplemental= meinfo_me_get_by_meid(65323, pq_supplemental_meid)) == NULL)
		return 0;
	if ((td_up_meid=meinfo_util_me_attr_data(me_pq_supplemental, 1)) == 0xffff)
		return 0;
	if ((me_td_up = meinfo_me_get_by_meid(280, td_up_meid)) == NULL)
		return 0;

	// locate phy_queueid
	if (tm_get_tcont_id_phy_queue_info_by_me_pq_gem(me_pq, me_gem, is_yellow, &tcont_id, &phy_queueid, &is_policy_wrr, &hw_weight)<=0) {
		dbprintf_bat(LOG_NOTICE, "Can't find phy_queueid, continue\n");
		return -1;
	}
	if( phy_queueid < 0)
		return -1;

	//do cir/pir configuration
	unsigned long long gem_td_cir_up=(unsigned int)meinfo_util_me_attr_data(me_td_up, 1); //cir
	unsigned long long gem_td_pir_up=(unsigned int)meinfo_util_me_attr_data(me_td_up, 2); //pir
	t->td_usgem_wanport_map[phy_queueid].is_gem_td_up_null = 0;

	// CIR
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
		if (proprietary_alu_get_olt_version() == 0)
			gem_td_cir_up=gem_td_cir_up*125;
	}
	gem_td_cir_up=gem_td_cir_up*omci_env_g.tm_gemus_rate_factor/100;
	if(is_yellow)
		gem_td_cir_up = 0;
	//overwrite cir
	t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up=gem_td_cir_up;
	if( t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up > GPON_US_WIRESPEED_BYTE_PER_SEC )
		t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up = GPON_US_WIRESPEED_BYTE_PER_SEC;

	// PIR
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
		if (proprietary_alu_get_olt_version() == 0)
			gem_td_pir_up=gem_td_pir_up*125;
	}
	gem_td_pir_up=gem_td_pir_up*omci_env_g.tm_gemus_rate_factor/100;
	//overwrite pir
	t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up=gem_td_pir_up;
	if( t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up > GPON_US_WIRESPEED_BYTE_PER_SEC )
		t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up = GPON_US_WIRESPEED_BYTE_PER_SEC;

	return 0;
}

int
td_usgem_wanport_generator(struct batchtab_cb_td_t *t)	//for GEM/WAN
{
	struct meinfo_t *miptr_bport=meinfo_util_miptr(47);
	struct me_t *me_bport_wan;
	int is_rate_ctrl_mode = omciutil_misc_traffic_management_option_is_rate_control();
	int i;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	if(batchtab_table_data_get("tm") == NULL) {
		dbprintf_bat(LOG_ERR, "batchtab_cb_tm resource still not ready for td, please try again\n");
		return -1;
	} else {
		batchtab_table_data_put("tm");
	}

	for(i=0; i < GPON_PQ_ID_TOTAL; i++) {
		memset(&t->td_usgem_wanport_map[i], 0, sizeof(struct td_usgem_wanport_map_t));
		INIT_LIST_HEAD(&t->td_usgem_wanport_map[i].wan_td_list);
	}

	list_for_each_entry(me_bport_wan, &miptr_bport->me_instance_list, instance_node) {
		struct me_t *gem_list[8], *unique_gem_list[8];
		int gemport_count, unique_gemport_count;

		{
			struct switch_port_info_t port_info;
			if( switch_get_port_info_by_me(me_bport_wan, &port_info) < 0) {
				dbprintf_bat(LOG_ERR, "switch_get_port_info_by_me error, continue\n");
				continue;
			}
			if (port_info.port_type != ENV_BRIDGE_PORT_TYPE_WAN &&
				port_info.port_type != ENV_BRIDGE_PORT_TYPE_DS_BCAST)
				continue;
		}

		// each wan might has in/outbound td, gem flow within a wan port might also has us/ds td
		// we try to find unique gem flows list for me_bport_wan, then sum up the wan/gem cir/pir to datastru t
		// which will be used in td_usgem_wanport_sync_to_hardware()
		if((gemport_count=omciutil_misc_get_gem_me_list_by_bport_me(me_bport_wan, gem_list)) <= 0) {
			dbprintf_bat(LOG_ERR, "This can't happen in normal\n");
			continue;
		}
		unique_gemport_count = gemlist_to_unique_gemlist(gemport_count, gem_list, unique_gem_list);

		//generator td to pq mapping according gem
		for(i=0; i < unique_gemport_count; i++) {
			struct me_t *me_gem = unique_gem_list[i];
			unsigned short pq_meid=omciutil_get_pq_pointer_for_gem_ctp(me_gem, is_rate_ctrl_mode);
			struct me_t *me_pq;

			if ((me_pq = meinfo_me_get_by_meid(277, pq_meid)) == NULL) {
				dbprintf_bat(LOG_ERR, "me_bport_wan=0x%x, unique_gemport_count[%d]:me_gem=0x%x, me_pq_meid=0x%x, me_pq not found! continue\n",
					me_bport_wan->meid, i, me_gem->meid, pq_meid);
				continue;
			}

			//green
			if (td_pq_cirpir_from_usgem_wanport(t, me_bport_wan, me_gem, me_pq, 0) <0)
				continue;
			if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) {
				td_pq_cirpir_from_pq_supplemental(t, me_gem, me_pq, 0);
			}
			if(omci_env_g.tm_pq_gem_qos_method==1) {
				//yellow
				if (td_pq_cirpir_from_usgem_wanport(t, me_bport_wan, me_gem, me_pq, 1) <0)
					continue;
				if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) {
					td_pq_cirpir_from_pq_supplemental(t, me_gem, me_pq, 1);
				}
			}
		}
	}
	return 0;
}

int
td_usgem_wanport_sync_to_hardware(struct batchtab_cb_td_t *t)
{
	struct gpon_tm_pq_config_t pq_config;
	int phy_queueid	;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {
		if( t->td_usgem_wanport_map[phy_queueid].is_used == 0 )
			continue;

		memset(&pq_config, 0, sizeof(pq_config));
		//get current value from hardware
		pq_config.tcont_id=t->td_usgem_wanport_map[phy_queueid].tcont_id;
		gpon_hw_g.tm_pq_config_get(phy_queueid, &pq_config);

		//collect data
		pq_config.enable=1;
		pq_config.tcont_id=t->td_usgem_wanport_map[phy_queueid].tcont_id;	//set value again, can remove?

		// choice non zero min(wan_td, pq_td)
		if (omci_env_g.tm_pir_zero_action == 0) {
			pq_config.cir=min(t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound, t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up);
			pq_config.pir=min(t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound, t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up);
		} else {
			pq_config.cir=non_zero_min(t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound, t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up);
			pq_config.pir=non_zero_min(t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound, t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up);
		}
		if (pq_config.cir > pq_config.pir) {
			if (omci_env_g.tm_pir_zero_action == 0 || (pq_config.cir > 0 && pq_config.pir >0)) {
				dbprintf_bat(LOG_ERR, "Warning! tcont_id=%d, phy_queueid=%d, CIR(%u) > PIR(%u)? force CIR = PIR(%u)\n",
					pq_config.tcont_id, phy_queueid, pq_config.cir, pq_config.pir, pq_config.pir);
				pq_config.cir = pq_config.pir;
			}
		}
		if (pq_config.pir == 0) {
			if (omci_env_g.tm_pir_zero_action == 1) {
				dbprintf_bat(LOG_ERR, "Warning! tcont_id=%d, phy_queueid=%d, PIR is 0, assume no rate limit\n",
					pq_config.tcont_id, phy_queueid);
				pq_config.pir=GPON_US_WIRESPEED_BYTE_PER_SEC;
			}
		}

		if( t->td_usgem_wanport_map[phy_queueid].is_wan_td_outbound_null==1 &&
		    t->td_usgem_wanport_map[phy_queueid].is_gem_td_up_null==1 ) {
			pq_config.cir=0;		// hareware behavior, no limit
			pq_config.pir=GPON_US_WIRESPEED_BYTE_PER_SEC;
		}

		//Warning!! hardware not support downstream cir/pir
		//write back to hardware
		//if( (ret=gpon_hw_g.tm_pq_config_set(phy_queueid, &pq_config)) != 0 ) {
		//	dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_pq_config_set error=0x%X\n", ret);
		//	return ret;
		//}
	}
	return 0;
}

int
td_usgem_wanport_dump(int fd, struct batchtab_cb_td_t *t)
{
	//struct wan_td_node_t *wan_td_node;
	int phy_queueid;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	util_fdprintf(fd, "gem us td, wan outbound td\n");
	util_fdprintf(fd, "---------------------------\n");
	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {
		if( t->td_usgem_wanport_map[phy_queueid].is_used == 0 )
			continue;

		util_fdprintf(fd, "gpon pq {sch_id=%d,qid=%d} phy_queueid=%d",
			t->td_usgem_wanport_map[phy_queueid].tcont_id, phy_queueid%32, phy_queueid);

		util_fdprintf(fd, " <-- me ");
		{
			unsigned short meid_pq, meid_gem;
			char is_yellow;			
			if (tm_get_meid_pq_gem_by_tcont_id_phy_queueid(t->td_usgem_wanport_map[phy_queueid].tcont_id, phy_queueid, &meid_pq, &meid_gem, &is_yellow)) {
				if (omci_env_g.tm_pq_gem_qos_method == 2) {
					util_fdprintf(fd, "pq:0x%x gem:0x%x", meid_pq, meid_gem);
				} else if (omci_env_g.tm_pq_gem_qos_method == 1) {
					util_fdprintf(fd, "pq:0x%x(%s)", meid_pq, is_yellow?"yellow":"green");
				} else {
					util_fdprintf(fd, "pq:0x%x", meid_pq);
				}
			}
		}
		util_fdprintf(fd, " bport:0x%x", t->td_usgem_wanport_map[phy_queueid].wanport_meid);
		{
			struct me_t *me=meinfo_me_get_by_meid(47, t->td_usgem_wanport_map[phy_queueid].wanport_meid);
			char *devname = NULL;
			if (me && (devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, "(%s)", devname);
		}
		util_fdprintf(fd, "\n");
		util_fdprintf(fd, "\tgem_us_td:       cir=%dB/s(%s), pir=%dB/s(%s)\n",
			t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up,
			util_bps_to_ratestr(t->td_usgem_wanport_map[phy_queueid].gem_td_cir_up*8ULL),
			t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up,
			util_bps_to_ratestr(t->td_usgem_wanport_map[phy_queueid].gem_td_pir_up*8ULL));

		util_fdprintf(fd, "\twan_outbound_td: cir=%dB/s(%s), pir=%dB/s(%s)\n",
			t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound,
			util_bps_to_ratestr(t->td_usgem_wanport_map[phy_queueid].wan_td_cir_outbound*8ULL),
			t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound,
			util_bps_to_ratestr(t->td_usgem_wanport_map[phy_queueid].wan_td_pir_outbound*8ULL));

		//util_fdprintf(fd, "wan_td_list=0x%X\n", t->td_usgem_wanport_map[phy_queueid].wan_td_list);
	}

	util_fdprintf(fd, "\nps: minimal(pq_related_gem_td_sum, pq_related_wan_td_sum) will be set to pq cir/pir respectively\n\n");
	return 0;
}

int
td_usgem_wanport_release(struct batchtab_cb_td_t *t)
{
	struct wan_td_node_t *wan_td_node, *tmp_wan_td_node;
	int phy_queueid;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {
		if( t->td_usgem_wanport_map[phy_queueid].is_used == 0 )
			continue;
		list_for_each_entry_safe(wan_td_node, tmp_wan_td_node,
			&t->td_usgem_wanport_map[phy_queueid].wan_td_list, wan_td_entry_node) {

			list_del(&wan_td_node->wan_td_entry_node);
			free_safe(wan_td_node);
		}
	}

	for(phy_queueid=0; phy_queueid < GPON_PQ_ID_TOTAL; phy_queueid++) {
		memset(&t->td_usgem_wanport_map[phy_queueid], 0, sizeof(struct td_usgem_wanport_map_t));
		INIT_LIST_HEAD(&t->td_usgem_wanport_map[phy_queueid].wan_td_list);
	}

	return 0;
}

// td_dsgem_... is for downstream td on GEM ////////////////////////////////////////////////////////////////////

int
td_dsgem_generator(struct batchtab_cb_td_t *t)
{
	struct meinfo_t *miptr268=meinfo_util_miptr(268);
	struct me_t *meptr268, *me_td_up, *me_td_down;
	int ret = 0, pos;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}
	if (miptr268==NULL) {
		dbprintf_bat(LOG_ERR, "err=%d, classid=268\n", MEINFO_ERROR_CLASSID_UNDEF);
		return MEINFO_ERROR_CLASSID_UNDEF;  //class_id out of range
	}

	pos=0;
        list_for_each_entry(meptr268, &miptr268->me_instance_list, instance_node) {
		me_td_up=NULL;
		me_td_down=NULL;

		if( find_gem_related_traffic_descriptor(meptr268, &me_td_up, &me_td_down)== 0) {
			dbprintf_bat(LOG_NOTICE, "Can't find related td\n");
			continue;
			//even no limit still need to do
		}

		if(pos >= TD_DSGEM_MAX) {
			dbprintf_bat(LOG_ERR, "numbers of GEM port > TD_DSGEM_MAX=%d\n", TD_DSGEM_MAX);
			ret=-1;
			continue;
		}

		t->td_dsgem_map[pos].is_used=1;
		t->td_dsgem_map[pos].me_gem = meptr268;
		t->td_dsgem_map[pos].me_td = me_td_down;

		if(me_td_down != NULL) {//downstream, byte/s
			unsigned long long td_pir_down=(unsigned int)meinfo_util_me_attr_data(me_td_down, 2); //pir

			t->td_dsgem_map[pos].td_cir_down=
				(unsigned int)meinfo_util_me_attr_data(me_td_down, 1); //cir

			//boundary check
			if(t->td_dsgem_map[pos].td_cir_down > METER_WIRESPEED_BYTE_PER_SEC)
				t->td_dsgem_map[pos].td_cir_down = METER_WIRESPEED_BYTE_PER_SEC;

			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
				if (proprietary_alu_get_olt_version() == 0) {
					// omci uses byte/sec but alu old olt uses kbit/sec (125=1000/8)
					td_pir_down=td_pir_down*125; 	//byts/s
				}
			}
			// this rate limitation is data rate, not ethernet line rate,
			// multiple a factor to meet cht requirement that under 10% inaccuracy
			td_pir_down=td_pir_down*omci_env_g.tm_gemds_rate_factor/100;

			t->td_dsgem_map[pos].td_pir_down=td_pir_down;

			//boundary check
			if (t->td_dsgem_map[pos].td_pir_down > METER_WIRESPEED_BYTE_PER_SEC)
				t->td_dsgem_map[pos].td_pir_down = METER_WIRESPEED_BYTE_PER_SEC;

			t->td_dsgem_map[pos].td_pbs_down=
				(unsigned int)meinfo_util_me_attr_data(me_td_down, 4); //pbs

			//boundary check
			if(t->td_dsgem_map[pos].td_pbs_down > METER_MAX_BURST_BYTES)
				t->td_dsgem_map[pos].td_pbs_down = METER_MAX_BURST_BYTES;

		} else {//no downstream rate limit
			t->td_dsgem_map[pos].td_cir_down = 0;
			t->td_dsgem_map[pos].td_pir_down = METER_WIRESPEED_BYTE_PER_SEC;
		}
		pos++;
	}

	return ret;
}

int
td_dsgem_sync_to_hardware(struct batchtab_cb_td_t *t) 
{
	int pos, ret=0, flowid=-1;
	unsigned int burst_size;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	for(pos=0; pos < TD_DSGEM_MAX; pos++) {
		int down_rate=0;
		unsigned short gem_portid;
		
		if (t->td_dsgem_map[pos].is_used==0 || t->td_dsgem_map[pos].me_gem == NULL)
			continue;

		// remove old acl memter in this entry first
		if (t->td_dsgem_map[pos].hw_rule_entry >= 0) {
			int count;

			dbprintf_bat(LOG_ERR, "To vacl_del_order() for t->td_dsgem_map[%d].hw_rule_entry=(%d)\n", pos, t->td_dsgem_map[pos].hw_rule_entry);
			vacl_del_order(GEM_TD_METER_ACL_ORDER, &count);
			t->td_dsgem_map[pos].hw_rule_entry = -1;
		}

		//acl meter use flowid, not gem port id
		gem_portid = meinfo_util_me_attr_data(t->td_dsgem_map[pos].me_gem, 1);
		if( (flowid=gem_flowid_get_by_gem_portid(gem_portid, 0)) < 0 ) {	//care green only
			dbprintf_bat(LOG_ERR, "gem:0x%x(portid=%d) get flowid error? \n", t->td_dsgem_map[pos].me_gem->meid, gem_portid);
			continue;
		} else {
			dbprintf_bat(LOG_WARNING, "gem:0x%x(portid=%d) get flowid=%d\n", t->td_dsgem_map[pos].me_gem->meid, gem_portid, flowid);
		}

		down_rate=t->td_dsgem_map[pos].td_pir_down;	// ratelimit always use PIR, CIR is for coloring
		if (omci_env_g.tm_pir_zero_action == 1) {
			if( down_rate == 0)
				down_rate = METER_WIRESPEED_BYTE_PER_SEC;	// when tm_pir_zero_action == 1, pir 0 means no limit
		}		
		// if ratelimit >= 1000*1000*1000bps, do not set ratelimit, so the downstream throughput could be maxinum
		if (down_rate >= METER_WIRESPEED_BYTE_PER_SEC*RATELIMIT_PERCENT_THRESHOLD) 
			continue;

		if(t->td_dsgem_map[pos].td_pbs_down != 0) {
			burst_size = t->td_dsgem_map[pos].td_pbs_down; // set token bucket size according to omci pbs
		} else {
			burst_size = down_rate; // set token bucket size = token for rate * 1 second
			if (burst_size < 1000)
				burst_size = 1000;
			else if (burst_size > METER_MAX_BURST_BYTES)
				burst_size = METER_MAX_BURST_BYTES;
			// while METER_WIRESPEED_BYTE_PER_SEC(0xF000) was suggested, we found max 0xFA00 still works
			// so we enlarge burst size from 0xF000 to 0xFA00 for better tcp throughput if ratelimit is over 650M
			if (down_rate > METER_WIRESPEED_BYTE_PER_SEC*0.65) 
				burst_size = 0xFA00;	
		}

		{
			struct vacl_user_node_t acl_rule;
			int sub_order=0;
			memset(&acl_rule, 0, sizeof(struct vacl_user_node_t));
			vacl_user_node_init(&acl_rule);

			acl_rule.ingress_active_portmask = 0x20;
			acl_rule.ingress_gem_llid_value = flowid;
			acl_rule.ingress_gem_llid_mask = 0xffff;
			acl_rule.ingress_care_u.bit.gem_llid_value = 1;
			acl_rule.ingress_care_u.bit.gem_llid_mask = 1;
	
			// while gpon pq cir/pir and uni ratelimit unit is 8kbps, meter unit is kbps!
			acl_rule.act_meter_rate = down_rate*8/1000;	// byte/s->kbps			
			if (acl_rule.act_meter_rate >0) {
				if (acl_rule.act_meter_rate <8)
					acl_rule.act_meter_rate = 8;
				acl_rule.act_meter_bucket_size = burst_size;
				acl_rule.act_meter_ifg = 0;
				//acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_W_SHARE; //means meter will auto allocate(share the same meter)
				acl_rule.hw_meter_table_entry = ACT_METER_INDEX_AUTO_WO_SHARE; //not share the same meter
				acl_rule.act_type |= VACL_ACT_LOG_POLICING_MASK;
			} else if (acl_rule.act_meter_rate == 0) { // rate==0, always drop
				acl_rule.act_type |= VACL_ACT_FWD_DROP_MASK;
			}
			acl_rule.order = GEM_TD_METER_ACL_ORDER;

			vacl_add(&acl_rule, &sub_order);
			switch_hw_g.vacl_commit(&ret);
			t->td_dsgem_map[pos].hw_rule_entry=sub_order;
			dbprintf_bat(LOG_ERR, "t->td_dsgem_map[%d].hw_rule_entry=%d\n", pos, sub_order);
		}
	}

	return ret;
}

int
td_dsgem_dump(int fd, struct batchtab_cb_td_t *t)
{
	int pos;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	util_fdprintf(fd, "gem ds td\n");
	util_fdprintf(fd, "---------------------------\n");
	for(pos=0; pos < TD_DSGEM_MAX; pos++) {
		unsigned short gem_portid;
		if(t->td_dsgem_map[pos].is_used==0 || t->td_dsgem_map[pos].me_gem == NULL) 
			continue;
		gem_portid = meinfo_util_me_attr_data(t->td_dsgem_map[pos].me_gem, 1);
		util_fdprintf(fd, "gem:0x%x portid=%d(0x%x)  ds_td:0x%x cir=%dB/s(%s), pir=%dB/s(%s), pbs=%dBytes  ", 
			t->td_dsgem_map[pos].me_gem->meid, gem_portid, gem_portid, 
			t->td_dsgem_map[pos].me_td?t->td_dsgem_map[pos].me_td->meid: 0xffff, 
			t->td_dsgem_map[pos].td_cir_down, util_bps_to_ratestr(t->td_dsgem_map[pos].td_cir_down*8ULL), 
			t->td_dsgem_map[pos].td_pir_down, util_bps_to_ratestr(t->td_dsgem_map[pos].td_pir_down*8ULL),
			t->td_dsgem_map[pos].td_pbs_down);

		util_fdprintf(fd, "vacl order=%d:%d\n", GEM_TD_METER_ACL_ORDER, t->td_dsgem_map[pos].hw_rule_entry);
	}

	util_fdprintf(fd, "\nps: pir is implemented by hw acl meter, cir is ignored\n\n");
	return 0;
}

int
td_dsgem_release(struct batchtab_cb_td_t *t) 
{
	int pos;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	for(pos=0; pos < TD_DSGEM_MAX; pos++) {
		if(t->td_dsgem_map[pos].is_used != 0) {
			int count;
			vacl_del_order(GEM_TD_METER_ACL_ORDER, &count);
		}
		memset( &t->td_dsgem_map[pos], 0x0, sizeof(struct td_dsgem_map_t));
		t->td_dsgem_map[pos].hw_rule_entry = -1; // hw_rule_entry has to be -1 because 0 is a valid entry
	}

	return 0;
}

// td_uniport_... is for inbound/outbound td on UNI ////////////////////////////////////////////////////////////////////

int
td_uniport_generator(struct batchtab_cb_td_t *t)
{
	struct meinfo_t *miptr_bport=meinfo_util_miptr(47);
	struct me_t *me_bport_uni;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}
	if(batchtab_table_data_get("tm") == NULL) {
		dbprintf_bat(LOG_ERR, "batchtab_cb_tm resource still not ready for td, please try again\n");
		return -1;
	} else {
		batchtab_table_data_put("tm");
	}

	list_for_each_entry(me_bport_uni, &miptr_bport->me_instance_list, instance_node) {
		struct me_t *me_td_inbound = NULL, *me_td_outbound = NULL;
		int portid;

		{
			struct switch_port_info_t port_info;
			if(switch_get_port_info_by_me(me_bport_uni, &port_info) < 0) {
				dbprintf_bat(LOG_ERR, "switch_get_port_info_by_me error, continue\n");
				continue;
			}
			if (port_info.port_type != ENV_BRIDGE_PORT_TYPE_UNI)
				continue;
			portid=port_info.logical_port_id;
		}

		//collect data
		find_bport_related_traffic_descriptor(me_bport_uni, &me_td_outbound, &me_td_inbound);
		if(me_td_inbound) {
			t->td_uniport_map[portid].is_used = 1;
			t->td_uniport_map[portid].me_td_inbound = me_td_inbound;
		} else {
			t->td_uniport_map[portid].me_td_inbound = NULL;
			dbprintf_bat(LOG_WARNING, "me_td_inbound is NULL\n");
		}
		if(me_td_outbound) {
			t->td_uniport_map[portid].is_used = 1;
			t->td_uniport_map[portid].me_td_outbound = me_td_outbound;
		} else {
			t->td_uniport_map[portid].me_td_outbound = NULL;
			dbprintf_bat(LOG_WARNING, "me_td_outbound is NULL\n");
		}
	}
	return 0;
}

/*
Hope to change/disable the port rate control with the minimal side effect
step 1. each time regenerator change STATE_CURRENT_USE to STATE_LAST_USE
step 2. when "set" change the port rate control's state to STATE_CURRENT_USE
step 3. find the state is STATE_LAST_USE(it means port rate conrtol is used last time, but not used this time)
	disable the port rate conrtol and change state to STATE_UNUSED
*/
int
td_uniport_sync_to_hardware(struct batchtab_cb_td_t *t)
{
	static char uni_inbound_ratelimit_state[SWITCH_LOGICAL_PORT_TOTAL] = {0};
	static char uni_outbound_ratelimit_state[SWITCH_LOGICAL_PORT_TOTAL] = {0};

	int uni_portmask = switch_get_uni_logical_portmask();
	int portid;
	struct switch_rate_control_config_t rate_control_cfg;

	if(t == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, t is NULL\n");
		return -1;
	}

	//step 1.
	for(portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if (((1<<portid) & uni_portmask) == 0)
			continue;
		if( uni_inbound_ratelimit_state[portid] == STATE_LAST_USE ||
			uni_outbound_ratelimit_state[portid] == STATE_LAST_USE) {
			dbprintf_bat(LOG_ERR, "Error, check td_uniport_hw_private_map because this is impossible to happen\n");
		}
		if( uni_inbound_ratelimit_state[portid] == STATE_CURRENT_USE) {
			uni_inbound_ratelimit_state[portid]=STATE_LAST_USE;
		}
		if( uni_outbound_ratelimit_state[portid] == STATE_CURRENT_USE) {
			uni_outbound_ratelimit_state[portid]=STATE_LAST_USE;
		}
	}

	for(portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if (((1<<portid) & uni_portmask) == 0)
			continue;
		if( t->td_uniport_map[portid].is_used == 0)
			continue;

		if( t->td_uniport_map[portid].me_td_inbound) {
			unsigned long long pir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_inbound, 2);
			pir = pir * omci_env_g.tm_unius_rate_factor/100;

			rate_control_cfg.enable=1;
			rate_control_cfg.dir=0;		// inbound/ingress;						
			rate_control_cfg.rate = pir;	// always use pir for port ratelimit
			if (omci_env_g.tm_pir_zero_action == 1) {
				if(rate_control_cfg.rate==0)
					rate_control_cfg.enable=0; //disable
			}
			//FIXME justin, maybe need add option in omcienv
			rate_control_cfg.has_ifg=0;
			rate_control_cfg.fc_en=1;
			if(switch_hw_g.rate_control_set)
				switch_hw_g.rate_control_set(portid, rate_control_cfg);

			//step 2.
			uni_inbound_ratelimit_state[portid]=STATE_CURRENT_USE;
		}

		// apply uni outbound td only if calix apas mode is not enabled
		if( t->td_uniport_map[portid].me_td_outbound && batchtab_cb_calix_apas_is_enabled() == 0) {
			unsigned long long pir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_outbound, 2);
			pir = pir * omci_env_g.tm_unids_rate_factor/100;

			rate_control_cfg.enable=1;
			rate_control_cfg.dir=1;		// outbound/egress;
			rate_control_cfg.rate = pir;	// always use pir for port ratelimit
			if (omci_env_g.tm_pir_zero_action == 1) {
				if(rate_control_cfg.rate==0)
					rate_control_cfg.enable=0; //disable
			}
			// if ratelimit >= 1000*1000*1000bps, do not set ratelimit, so the downstream throughput could be maxinum
			if (pir > ETHER_WIRESPEED_BYTE_PER_SEC*RATELIMIT_PERCENT_THRESHOLD)
				rate_control_cfg.enable=0;
			
			//FIXME justin, maybe need add option in omcienv
			rate_control_cfg.has_ifg=0;
			rate_control_cfg.fc_en=1;
			if(switch_hw_g.rate_control_set)
				switch_hw_g.rate_control_set(portid, rate_control_cfg);

			//step 2.
			uni_outbound_ratelimit_state[portid]=STATE_CURRENT_USE;
		}
	}

	//step 3.
	for(portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if (((1<<portid) & uni_portmask) == 0)
			continue;

		if( uni_inbound_ratelimit_state[portid] == STATE_LAST_USE) {
			memset(&rate_control_cfg, 0, sizeof(rate_control_cfg));
			rate_control_cfg.enable=0;	//disable hardware queue
			rate_control_cfg.dir=0;		//inbound/ingress;			
			if(switch_hw_g.rate_control_set)
				switch_hw_g.rate_control_set(portid, rate_control_cfg);
			uni_inbound_ratelimit_state[portid]=STATE_UNUSED;
		}

		if( uni_outbound_ratelimit_state[portid] == STATE_LAST_USE) {
			memset(&rate_control_cfg, 0, sizeof(rate_control_cfg));
			rate_control_cfg.enable=0;	//disable hardware queue
			rate_control_cfg.dir=1;		//outbound/egress;
			if(switch_hw_g.rate_control_set)
				switch_hw_g.rate_control_set(portid, rate_control_cfg);
			uni_outbound_ratelimit_state[portid]=STATE_UNUSED;
		}
	}

	return 0;
}

int
td_uniport_dump(int fd, struct batchtab_cb_td_t *t)
{
	int uni_portmask = switch_get_uni_logical_portmask();
	int portid, uni_index = 0;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}

	util_fdprintf(fd, "uni inbound/outbound td\n");
	util_fdprintf(fd, "---------------------------\n");
	for(portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		if (((1<<portid) & uni_portmask) == 0)
			continue;
		if( t->td_uniport_map[portid].is_used==0)
			continue;

		if( t->td_uniport_map[portid].me_td_inbound) {
			int cir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_inbound, 1);
			int pir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_inbound, 2);
			util_fdprintf(fd, "uni%d\tinbound td:0x%x cir=%dB/s(%s) pir=%dB/s(%s)\n", 
				uni_index, t->td_uniport_map[portid].me_td_inbound->meid,
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
		}

		// apply uni outbound td only if calix apas mode is not enabled
		if( t->td_uniport_map[portid].me_td_outbound && batchtab_cb_calix_apas_is_enabled() == 0) {
			int cir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_outbound, 1);
			int pir= (unsigned int)meinfo_util_me_attr_data(t->td_uniport_map[portid].me_td_outbound, 2);
			if (t->td_uniport_map[portid].me_td_inbound == NULL) {
				util_fdprintf(fd, "uni%d\toutbound td:0x%x cir=%dB/s(%s) pir=%dB/s(%s)\n", 
					uni_index, t->td_uniport_map[portid].me_td_outbound->meid,
					cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			} else {
				util_fdprintf(fd, "\toutbound td:0x%x cir=%dB/s(%s) pir=%dB/s(%s)\n",
					t->td_uniport_map[portid].me_td_outbound->meid,
					cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			}
		}
		uni_index++;
	}
	util_fdprintf(fd, "\n");
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
		if (batchtab_cb_calix_apas_is_enabled())
			util_fdprintf(fd, "ps: calix apas mode enabled, all uni outbound are disabled\n");
	}
	util_fdprintf(fd, "ps: pir is implemented by uni ingrss/egress ratelimit, cir is ignored\n\n");
	return 0;
}

int
td_uniport_release(struct batchtab_cb_td_t *t)
{
	int portid;

	if(t == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, t is NULL\n");
		return -1;
	}
	for(portid=0; portid < SWITCH_LOGICAL_PORT_TOTAL; portid++) {
		memset(&t->td_uniport_map[portid], 0, sizeof(struct td_uniport_map_t));
	}
	return 0;
}
