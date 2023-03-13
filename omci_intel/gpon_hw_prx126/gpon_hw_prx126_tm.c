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
 * Module  : gpon_hw_prx126
 * File    : gpon_hw_prx126_tm.c
 *
 ******************************************************************/

#include "util.h"
#include "meinfo.h"  // 230116 LEO
#include "tm.h"  // 230116 LEO
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"

#include "intel_px126_tm.h"

int
gpon_hw_prx126_tm_pq_config_get(int pq_id, struct gpon_tm_pq_config_t *pq_config)
{
	return 0;
}

int
gpon_hw_prx126_tm_pq_config_set(int pq_id, struct gpon_tm_pq_config_t *pq_config,int create_only)
{
	struct px126_priority_queue_update_data updata;

	memset(&updata,0x0,sizeof(struct px126_priority_queue_update_data));

	updata.related_port = pq_config->tcont_id;
	updata.traffic_scheduler_ptr = pq_config->ts_id;

	return intel_px126_tm_updtae_queue((unsigned short)pq_id,&updata,(create_only)?1:0);

}

int
gpon_hw_prx126_tm_pq_config_del(int pq_id, struct gpon_tm_pq_config_t *pq_config)
{
	struct px126_priority_queue_update_data updata;

	memset(&updata,0x0,sizeof(struct px126_priority_queue_update_data));

	updata.related_port = pq_config->tcont_id;
	updata.traffic_scheduler_ptr = pq_config->ts_id;

	return intel_px126_tm_destory_queue((unsigned short)pq_id,&updata);
}

int
gpon_hw_prx126_tm_ts_config_get(int ts_id, struct gpon_tm_ts_config_t *ts_config)
{
	return 0;
}

int
gpon_hw_prx126_tm_ts_config_set(int ts_id, struct gpon_tm_ts_config_t *ts_config,int create_only)
{
	struct px126_traffic_scheduler_update_data updata;

	memset(&updata,0x0,sizeof(struct px126_traffic_scheduler_update_data));
	updata.tcont_ptr = ts_config->tcont_id;
	updata.traffic_scheduler_ptr = ts_config->ts_ptr;
	updata.policy = ts_config->policy;
	updata.priority_weight = ts_config->priority_weight;
	return intel_px126_tm_updtae_ts((unsigned short)ts_id,&updata,(create_only)?1:0);
}
int
gpon_hw_prx126_tm_ts_config_del(int ts_id, struct gpon_tm_ts_config_t *ts_config)
{
	return 0 ;
}

int
gpon_hw_prx126_tm_tcont_config_get( struct gpon_tm_tcont_config_t *tcont_config)
{
	return 0;
}

int
gpon_hw_prx126_tm_tcont_config_set(struct gpon_tm_tcont_config_t *tcont_config,int create_only)
{
	return intel_px126_tm_updtae_tcont(tcont_config->me_id,tcont_config->policy,tcont_config->alloc_id,(create_only)?1:0);
}

int
gpon_hw_prx126_tm_tcont_config_del(struct gpon_tm_tcont_config_t *tcont_config)
{
	return intel_px126_tm_destory_tcont(tcont_config->me_id,tcont_config->policy);
}


int
gpon_hw_prx126_tm_tcont_add_by_allocid(int allocid, int *tcont_id)
{

	return 0;
}

int
gpon_hw_prx126_tm_tcont_get_by_allocid(int allocid, int *tcont_id)
{
	return intel_px126_tm_get_by_alloc_id(allocid,tcont_id);
}

int
gpon_hw_prx126_tm_tcont_del_by_allocid(int allocid)
{

	return 0;
}

int
gpon_hw_prx126_tm_dsflow_get(int dsflow_id, struct gpon_tm_dsflow_config_t *dsflow_config)
{
	
	return intel_px126_tm_dsflow_get(dsflow_id,dsflow_config);
}

int
gpon_hw_prx126_tm_dsflow_set(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config)
{
	/*dsflow_id is gem meid*/
	return intel_px126_tm_dsflow_set(dsflow_id,dsflow_config);
}

int
gpon_hw_prx126_tm_dsflow_del(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config)
{
	/*dsflow_id is gem meid*/
	return intel_px126_tm_dsflow_del(dsflow_id,dsflow_config);
}

int
gpon_hw_prx126_tm_usflow_get(int usflow_id, struct gpon_tm_usflow_config_t *usflow_config)
{
	
	return intel_px126_tm_usflow_get(usflow_id,usflow_config);
}

int
gpon_hw_prx126_tm_usflow_set(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config)  
{
	/*usflow_id is gem meid*/
	return intel_px126_tm_usflow_set(usflow_id,usflow_config);
}
int
gpon_hw_prx126_tm_usflow_del(int usflow_id,  struct gem_port_net_ctp_update_data *usflow_config)  
{
	/*usflow_id is gem meid*/
	return intel_px126_tm_usflow_del(usflow_id,usflow_config);
}

int
gpon_hw_prx126_tm_usflow2pq_get(int usflow_id, int *pq_id)
{
	// 230113 LEO START 
	// 1. find gemport by usflow_id
	unsigned short gemport = 0;
	struct gpon_tm_usflow_config_t usflow_config;
	memset(&usflow_config, 0, sizeof(usflow_config));
	if (gpon_hw_g.tm_usflow_get(usflow_id, &usflow_config)==0 && usflow_config.enable)
		gemport = usflow_config.gemport;
	else
		return -1;

	// 2. prepare me info
	struct meinfo_t *miptr = meinfo_util_miptr(268);
	struct me_t* me=NULL;
	int tcont_id=0, queue_id=0, is_policy_wrr=0, hw_weight=0;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node)
		if(me->meid == gemport)
			tm_get_tcont_id_phy_queue_info_by_me_pq_gem(0, me, 0, &tcont_id, &queue_id, &is_policy_wrr, &hw_weight);

	// 3. get hw pq_id by me info
	*pq_id = queue_id;

	// 220113 LEO END
	return 0;
}

int
gpon_hw_prx126_tm_usflow2pq_set(int usflow_id, int pq_id)
{
	return 0;
}
int
gpon_hw_prx126_tm_flow_8021p_set(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg,int is_set)
{
	if(is_set)
		return intel_px126_tm_flow_8021p_set(meid,upd_cfg);
	else
		return intel_px126_tm_flow_8021p_del(meid,upd_cfg);
	return 0;
	
}
int
gpon_hw_prx126_tm_flow_iwtp_set(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg,int is_set)
{
	if(is_set)
		return intel_px126_tm_flow_iwtp_set(meid,upd_cfg);	
	else
		return intel_px126_tm_flow_iwtp_del(meid,upd_cfg);

	return 0;
}
int
gpon_hw_prx126_tm_flow_mcastiwtp_set(unsigned short meid, unsigned short ctp_ptr,int is_set)
{
	if(is_set)
		return intel_px126_tm_flow_mcastiwtp_set(meid,ctp_ptr);	
	else
		return intel_px126_tm_flow_mcastiwtp_destory(meid);	
	return 0;
}

