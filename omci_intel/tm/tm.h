#ifndef __TM_H__
#define __TM_H__

#include "list.h"
#include "gpon_sw.h"
#include "switch.h"

#define STATE_LAST_USE		-1
#define STATE_UNUSED		0
#define STATE_CURRENT_USE	1
#define STATE_DATA_THE_SAME	2

// tm.c /////////////////////////////////////////////////////////////

#define TCONT_SP_TS_SP		1
#define TCONT_SP_TS_WRR		2
#define TCONT_WRR_TS_WRR	3
#define TCONT_WRR_TS_SP		4

/*
T-CONT 	TS
policy 	policy	prioH	prioL	WeightH WeightL Action
sp	sp	pri	pri_pq	1	1	All PQ put in SP list
sp	wrr	pri	OP1	1	wt_pq	Lowest prio PQ in WRR list Else PQ in SP list
wrr	wrr	0	0	wt_ts 	wt_pq	All PQ in WRR list
wrr	sp	0	0	wt_ts 	OP2	All PQ in WRR list

OP1: old history: 0x0fff + ((all-x)/all)*0xffff, all=weight1+weight2+weight3+.....
NOTE: now OP1 always use 0x0fff
OP2: sum of 255/n can't equal 255(ME value),so modify it as 
	sorting: sp1, sp2, sp3 ,sp4 => 1:2:4:8....
	highest= 255-(1+2+4+8...), others weight is 1,2,4,8
	OP2=1,2,4,8,highest
*/

// tm_pq_node is linked on 2 list, 
// a. omcitree (tm_tcont_node_t->tm_ts_list=>tm_ts_node_t->tm_pq_list) by tm_pq_entry_node
// b. multi_prio_rr_tree (tm_multi_prio_rr_t->tm_equiv_prio_list=>tm_equiv_prio_node_t->tm_pq_list2) by tm_pq_entry_node2, 
//    which group PQ under one tcont of same priority to same list
#define TM_PER_PQ_RELATED_FLOW_MAX	32

struct tm_phy_q_info_t {
	short schedule_id;	// schedule_id(0..31) cross all group
	short queue_id;		// queueid (0..31) within per hw group
	short phy_queueid;	// queueid (0..127) cross all group
};
struct tm_pq_node_t {
	struct me_t *me_pq;
	int priority, weight; 	//original value
	unsigned int priority_h, priority_l, weight_h, weight_l;
	unsigned int equiv_priority;		//equivalent priority
	unsigned int equiv_weight;		//equivalent weight
	int hw_weight;

	struct tm_phy_q_info_t green_q;
	struct tm_phy_q_info_t yellow_q;

	struct tm_phy_q_info_t gemport_q[TM_PER_PQ_RELATED_FLOW_MAX];
	int me_gem_sorted_by_cir_pir_cnt;
	struct me_t *me_gem_sorted_by_cir_pir[TM_PER_PQ_RELATED_FLOW_MAX];	

	struct list_head tm_pq_entry_node;	// a node of list tm_ts_node_t->tm_pq_list
	struct list_head tm_pq_entry_node2;	// a node of list tm_equiv_prio_node_t->tm_pq_list2
};

// datastru for tcont->ts->pq relation (omci similar)
struct tm_ts_node_t {
	struct me_t *me_ts;
	unsigned char policy_ts;
	unsigned char policy_merge;
	unsigned char number_of_pq_in_ts;	//used for sp translation to weight (TCONT_WRR_TS_SP)
	unsigned char priority, weight; 	//original value
	struct list_head tm_pq_list;		// a list of tm_pq_entry_node
	struct list_head tm_ts_entry_node;	// a entry_node of list tm_tcont_node_t->tm_ts_list
};
struct tm_tcont_node_t {	/* tm entry node */
	struct me_t *me_tcont;
	unsigned char policy_tcont;
	char tm_pq_gem_qos_method;		//used for tm_pq_gem_qos_method
	unsigned char number_of_pq_in_tcont;	//used for hardware assign
	struct list_head tm_ts_list;		// a list of tm_ts_entry_node
	struct list_head tm_tcont_entry_node;	// a entry_node of list omcitree
};

// datastru for tcont->equiv_prio->pq relation (multiple level round-robin queues)
struct tm_equiv_prio_node_t {
	unsigned int equiv_priority;
	unsigned char is_treatment_as_wrr;
	unsigned int sum_of_equiv_weight_for_wrr;
	struct list_head tm_pq_list2;			// a list of tm_pq_node_t->tm_pq_entry_node2
	struct list_head tm_equiv_prio_entry_node;	// a node of list tm_equiv_prio_list
};
struct tm_multi_prio_rr_t {
	struct tm_tcont_node_t *tm_tcont_node_ptr;
	unsigned short tcont_id;
	struct list_head tm_equiv_prio_list;	/* a list of tm_equiv_prio_node_t->tm_equiv_prio_entry_node*/
};

// datastru to maintain hwpq alloc detail
struct phy_schedule_id_t {
	unsigned char value;
	unsigned char isused;
};
struct phy_pq_id_t {
	unsigned char value;
	unsigned char isused;
};
struct tm_hwpq_allocmap_t {
	struct phy_schedule_id_t phy_schedule_id[GPON_TCONT_ID_PER_GROUP];	//extend to 4 group 0-31
	unsigned char phy_schedule_id_total;					//max 8	
	struct phy_pq_id_t phy_pq_id[GPON_PQ_ID_PER_GROUP];			//map to pysical queue id
	unsigned char phy_pq_id_total;						//max 32
};

// datastru to maintain which hwpq has been hwsynced
struct tm_hwpq_hwsync_state_t {
	char state;
	unsigned short tcont_id;
};

struct batchtab_cb_tm_t {
	struct list_head omcitree;	/* a list of tm_tcont_node_t->tm_tcont_entry_node */
	struct tm_multi_prio_rr_t multi_prio_rr_tree[GPON_TCONT_ID_TOTAL];	//GPON_TCONT_ID_TOTAL 32
	struct tm_hwpq_allocmap_t hwpq_allocmap[CHIP_GPON_TCONT_GROUP];	//need reset when free resource
	struct tm_hwpq_hwsync_state_t hwpq_hwsync_state[GPON_PQ_ID_TOTAL];
};

// gemflow.c ///////////////////////////////////////////////////////

struct gemflow_map_t {
	unsigned short meid;
	unsigned short gem_portid;
	unsigned char phy_queueid;
	unsigned char is_used:1;
	unsigned char is_masked:1;
	unsigned char direction:2;
	unsigned char aes_enable:1;
	unsigned char mcast_enable:1;
	unsigned char is_yellow:1;
	unsigned char is_pre_alloc:1;
	struct gem_port_net_ctp_update_data gem_data;
	struct me_t *me_gem;
};

struct gemflow_8021p_map_t {
	unsigned short meid;
	unsigned char is_used;
	unsigned short gem_port_id[8];
	unsigned char dscp2pbit[24];
	struct gpon_dot1p_mapper_update_data update_data;
	struct gem_pmapper_data  pmapper;
	
};

struct gemflow_iwtp_map_t {
	unsigned short meid;
	unsigned short ctp_meid;
	unsigned char is_used;
	struct gpon_gem_interworking_tp_update_data  update_data;
};

struct gemflow_mcast_iwtp_map_t {
	unsigned short meid;
	unsigned short ctp_meid;
	unsigned char is_used;	
};

struct gemflow_hwsync_state_t {
	char us_state;
	char ds_state;
	struct gemflow_map_t us_fl;
	struct gemflow_map_t ds_fl;
};

struct batchtab_cb_gemflow_t {
	struct gemflow_map_t gemflow_map[GPON_FLOW_ID_TOTAL];
	struct gemflow_iwtp_map_t iwtp_map[GPON_FLOW_ID_TOTAL];
	struct gemflow_8021p_map_t gem_8021p_map[GPON_FLOW_8021P_MAP_TOTAL];
	struct gemflow_mcast_iwtp_map_t mcast_iwtp_map[GPON_FLOW_ID_TOTAL];
};

// td.c /////////////////////////////////////////////////////////////

// 1.2Gbps (1.24416*1000*1000*1000 bps = 1.24416*1000*1000*1000/8 Byte/sec = 155520000)
#define GPON_US_WIRESPEED_BYTE_PER_SEC	155520000 
#define GPON_DS_WIRESPEED_BYTE_PER_SEC	(GPON_US_WIRESPEED_BYTE_PER_SEC * 2)	// 2.4Gbps

struct wan_td_node_t {
	struct list_head wan_td_entry_node;	// a node of list td_usgem_wanport_map_t->wan_td_list
	struct me_t *me_bport_wan;
	struct me_t *me_td_outbound;
	struct me_t *me_td_inbound;
};
struct td_usgem_wanport_map_t {		// this data stru will be indexed by phy_queueid in an array
	struct list_head wan_td_list;	// a list of wan_td_node_t
	unsigned char is_used;
	unsigned short tcont_id;
	unsigned short wanport_meid; 	// assume each pq will be referred by only one wanport, this might not be always true

	unsigned char is_gem_td_up_null;
	unsigned int gem_td_cir_up;
	unsigned int gem_td_pir_up;

	unsigned char is_wan_td_outbound_null;
	unsigned int wan_td_cir_outbound;
	unsigned int wan_td_pir_outbound;
};

// hw meter unit is kbps, max is 1048576kbps (131072000Bps)
// 1Gbps = 1000*1024*1024 bps = 1000*1024*1024 bps =  1000*1024*1024/8 byte/s = 131072000 Bps
#define	METER_WIRESPEED_BYTE_PER_SEC	131072000	
#define	METER_MAX_BURST_BYTES		0xF000		// hw meter unit is bytes, max is 0xF000bytes, meter might not work if value larger than this
#define	ETHER_WIRESPEED_BYTE_PER_SEC	METER_WIRESPEED_BYTE_PER_SEC
// since OLT might issue 1Gbps as 1000*1000*1000 bps, which is 95.3% of 1000*1024*1024
#define RATELIMIT_PERCENT_THRESHOLD	0.95

#define	GEM_TD_METER_ACL_ORDER	30
#define TD_DSGEM_MAX	32		// number of ratelimite-enabled ds gemflow

struct td_dsgem_map_t {
	unsigned char is_used;
	struct me_t *me_gem;
	struct me_t *me_td;
	unsigned int td_cir_down;      
	unsigned int td_pir_down;      
	unsigned int td_pbs_down;      
	short hw_rule_entry;      
};

struct td_uniport_map_t {		// this data stru will be indexed by uni logical_port_id in an array  
	unsigned char is_used;
	struct me_t *me_td_inbound;
	struct me_t *me_td_outbound;
};

struct batchtab_cb_td_t {
	struct td_usgem_wanport_map_t td_usgem_wanport_map[GPON_PQ_ID_TOTAL]; 
	struct td_uniport_map_t td_uniport_map[SWITCH_LOGICAL_PORT_TOTAL];
	struct td_dsgem_map_t td_dsgem_map[TD_DSGEM_MAX];
};

/* gemflow.c */
int gemflow_generator(struct batchtab_cb_gemflow_t *g);
int gemflow_sync_to_hardware(struct batchtab_cb_gemflow_t *g);
int gemflow_dump(int fd, struct batchtab_cb_gemflow_t *g);
int gemflow_release(struct batchtab_cb_gemflow_t *g);
int gem_flowid_get_by_gem_me(struct me_t *me_gem, char is_yellow);
int gem_flowid_get_by_gem_portid(unsigned short gem_portid, char is_yellow);
/* td.c */
int td_usgem_wanport_generator(struct batchtab_cb_td_t *t);
int td_usgem_wanport_sync_to_hardware(struct batchtab_cb_td_t *t);
int td_usgem_wanport_dump(int fd, struct batchtab_cb_td_t *t);
int td_usgem_wanport_release(struct batchtab_cb_td_t *t);
int td_dsgem_generator(struct batchtab_cb_td_t *t);
int td_dsgem_sync_to_hardware(struct batchtab_cb_td_t *t);
int td_dsgem_dump(int fd, struct batchtab_cb_td_t *t);
int td_dsgem_release(struct batchtab_cb_td_t *t);
int td_uniport_generator(struct batchtab_cb_td_t *t);
int td_uniport_sync_to_hardware(struct batchtab_cb_td_t *t);
int td_uniport_dump(int fd, struct batchtab_cb_td_t *t);
int td_uniport_release(struct batchtab_cb_td_t *t);
/* tm.c */
int tm_generator(struct batchtab_cb_tm_t *tm);
int tm_sync_to_hardware(struct batchtab_cb_tm_t *tm);
int tm_dump(int fd, struct batchtab_cb_tm_t *tm);
int tm_release(struct batchtab_cb_tm_t *tm);
int tm_get_tcont_id_phy_queue_info_by_me_pq_gem(struct me_t *me_pq, struct me_t *me_gem, int is_yellow, int *schedule_id, int *phy_queueid, int *is_policy_wrr, int *hw_weight);
int tm_get_meid_pq_gem_by_tcont_id_phy_queueid(int schedule_id, int phy_queueid, unsigned short *meid_pq, unsigned short *meid_gem, char *is_yellow);

#endif
