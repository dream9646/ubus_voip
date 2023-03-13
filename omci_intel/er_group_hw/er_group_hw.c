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
 * Module  : er_group_hw
 * File    : er_group_hw.c
 *
 ******************************************************************/

#include <stdio.h>
#include "meinfo.h"
#include "util.h"
#include "hwresource.h"
#include "omciutil_vlan.h"
#include "er_group.h"
#include "er_group_hw.h"

int 
er_group_hw_init(void)
{
	/* er_group_hw_11_47_171.c */
	er_attr_group_hw_add("l2s_dscp_vlan_tagging_rxtagging_map_pptp_uni", er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_pptp_uni);
	er_attr_group_hw_add("l2s_tags_create_add012_pptp_uni", er_group_hw_l2s_tags_create_add012_pptp_uni);

	/* er_group_hw_11_47.c */
	er_attr_group_hw_add("ethernet_macaddr", er_group_hw_ethernet_macaddr);

	/* er_group_hw_11_47_78.c */
	er_attr_group_hw_add("l2s_tags_create_add01_pptp_uni", er_group_hw_l2s_tags_create_add01_pptp_uni);

	/* er_group_hw_11.c */
	er_attr_group_hw_add("ethernet_autoneg_mode", er_group_hw_ethernet_autoneg_mode);
	er_attr_group_hw_add("phy_loopback", er_group_hw_phy_loopback);
	er_attr_group_hw_add("ethernet_enable", er_group_hw_ethernet_enable);
	er_attr_group_hw_add("ethernet_mtu", er_group_hw_ethernet_mtu);
	er_attr_group_hw_add("ethernet_setpause", er_group_hw_ethernet_setpause);
	er_attr_group_hw_add("l2s_bridging_pppoe_filt", er_group_hw_l2s_bridging_pppoe_filt);
	er_attr_group_hw_add("loopdetect_enable", er_group_hw_loopdetect_enable);
	er_attr_group_hw_add("bridge_router_mode", er_group_hw_bridge_router_mode);
	er_attr_group_hw_add("poe_control", er_group_hw_poe_control);

	/* er_group_hw_11_130_266_268_277_171_401.c */
	er_attr_group_hw_add("l2s_unipqmap_802_1p_exttagop", er_group_hw_l2s_unipqmap_802_1p_exttagop);

	/* er_group_hw_11_130_266_268_277_401.c */
	er_attr_group_hw_add("l2s_unipqmap_802_1p", er_group_hw_l2s_unipqmap_802_1p);

	/* er_group_hw_11_130_266_268_277_78_401.c */
	er_attr_group_hw_add("l2s_unipqmap_802_1p_tagop", er_group_hw_l2s_unipqmap_802_1p_tagop);

	/* er_group_hw_11_290.c */
	er_attr_group_hw_add("l2s_bridging_dot1x", er_group_hw_dot1x);
	er_attr_group_hw_add("l2s_bridging_dot1x_dir", er_group_hw_dot1x_dir);

	/* er_group_hw_290_291.c */
	er_attr_group_hw_add("dot1x_config", er_group_hw_dot1x_config);
	
	/* er_me_group_134.c */
	er_attr_group_hw_add("ip_host_config_data", er_group_hw_ip_host_config_data);

	/* er_group_hw_137_148_157_340.c */
	er_attr_group_hw_add("tr069_config", er_group_hw_tr069_config);

	/* er_me_group_138.c */
	er_attr_group_hw_add("voip_config_data", er_group_hw_voip_config_data);

	/* er_me_group_149.c */
	er_attr_group_hw_add("sip_config_portal", er_group_hw_sip_config_portal);

	/* er_group_hw_256.c */
	er_attr_group_hw_add("mac_traffic_control_mng_option", er_group_hw_mac_traffic_control_mng_option);

	/* er_group_hw_262_277_278_402.c */
	er_attr_group_hw_add("mac_traffic_control_pq_ts_tcont", er_group_hw_mac_traffic_control_pq_ts_tcont);

	/* er_group_hw_262_278.c */
	er_attr_group_hw_add("mac_traffic_control_first_ts", er_group_hw_mac_traffic_control_first_ts);

	/* er_group_hw_262.c */
	er_attr_group_hw_add("mac_traffic_control_second_ts", er_group_hw_mac_traffic_control_second_ts);
	er_attr_group_hw_add("mac_tcont_allocate", er_group_hw_mac_tcont_allocate);

	/* er_group_hw_263.c */
	er_attr_group_hw_add("mac_gem_block", er_group_hw_mac_gem_block);
	er_attr_group_hw_add("mac_bip_setsdsf", er_group_hw_mac_bip_setsdsf);

	/* er_group_hw_268_277.c */
	er_attr_group_hw_add("generator_pq_ts_tcont", er_group_hw_generator_pq_ts_tcont);

	/* er_group_hw_268_280_401.c */
	er_attr_group_hw_add("mac_traffic_control_td_ds_gem", er_group_hw_mac_traffic_control_td_ds_gem);
	/* er_group_hw_47_266_268_277_280_402.c */
	er_attr_group_hw_add("mac_traffic_control_td_us_gem_iwtp", er_group_hw_mac_traffic_control_td_us_gem_iwtp);
	/* er_group_hw_47_262_266_268_277_280_402.c */
	er_attr_group_hw_add("mac_traffic_control_td_us_gem_iwtp_tm_rate_controlled", er_group_hw_mac_traffic_control_td_us_gem_iwtp_tm_rate_controlled);
	/* er_group_hw_47_130_266_268_277_280_402.c */
	er_attr_group_hw_add("mac_traffic_control_td_us_gem_802_1p", er_group_hw_mac_traffic_control_td_us_gem_802_1p);
	/* er_group_hw_47_130_262_266_268_277_280_402.c */
	er_attr_group_hw_add("mac_traffic_control_td_us_gem_802_1p_tm_rate_controlled", er_group_hw_mac_traffic_control_td_us_gem_802_1p_tm_rate_controlled);

	/* er_group_hw_277_402.c */
	er_attr_group_hw_add("mac_traffic_control_red", er_group_hw_mac_traffic_control_red);

	/* er_group_hw_300.c */
	er_attr_group_hw_add("l2s_tags_create_add012_cfm", er_group_hw_l2s_tags_create_add012_cfm);

	/* er_group_hw_45_47_49.c */
	er_attr_group_hw_add("l2s_mac_table_create_delete", er_group_hw_l2s_mac_table_create_del);

	/* er_group_hw_45_47_79.c */
	er_attr_group_hw_add("l2s_preassign_filter", er_group_hw_l2s_preassign_filt);

	/* er_group_hw_45_47.c */
	er_attr_group_hw_add("l2s_bridging_localbridging", er_group_hw_l2s_bridging_localbridging);
	er_attr_group_hw_add("l2s_mac_table_assign", er_group_hw_l2s_mac_table_assign);
	er_attr_group_hw_add("l2s_mac_table_flood", er_group_hw_l2s_mac_table_flood);
	er_attr_group_hw_add("mac_learning_depth_by_port", er_group_mac_learning_depth_by_port);
	er_attr_group_hw_add("l2s_tags_create_add012_bridge_port_add", er_group_hw_l2s_tags_create_add012_bridge_port_add);

	/* er_group_hw_45.c */
	er_attr_group_hw_add("l2s_mac_aging_time", er_group_hw_l2s_mac_aging_time);
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321	
	er_attr_group_hw_add("bridge_meid_create_inf",er_group_hw_l2s_bridge_inf);
	er_attr_group_hw_add("er_group_hw_47_entity_set",er_group_hw_47_entity_set);

#endif
	/* er_group_hw_45_280_298.c */
	er_attr_group_hw_add("port_storm_control_unknow_unicast", er_group_hw_port_storm_control_unknow_unicast);
	er_attr_group_hw_add("port_storm_control_broadcast", er_group_hw_port_storm_control_broadcast);
	er_attr_group_hw_add("port_storm_control_unknow_multicast", er_group_hw_port_storm_control_unknow_multicast);

	/* er_group_hw_130_280_298.c */
	er_attr_group_hw_add("storm_control_dup_unknow_unicast", er_group_hw_storm_control_dup_unknow_unicast);
	er_attr_group_hw_add("storm_control_dup_broadcast", er_group_hw_storm_control_dup_broadcast);
	er_attr_group_hw_add("storm_control_dup_unknow_multicast", er_group_hw_storm_control_dup_unknow_multicast);

	/* er_group_hw_47_130_171.c */
	er_attr_group_hw_add("l2s_dscp_vlan_tagging_rxtagging_map_802_1p", er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_802_1p);
	er_attr_group_hw_add("l2s_tags_create_add012_802_1p", er_group_hw_l2s_tags_create_add012_802_1p);

	/* er_group_hw_47_134_171.c */
	er_attr_group_hw_add("l2s_dscp_vlan_tagging_rxtagging_map_ip_host", er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_ip_host);
	er_attr_group_hw_add("l2s_tags_create_add012_ip_host", er_group_hw_l2s_tags_create_add012_ip_host);

	/* er_group_hw_47_171_329.c */
	er_attr_group_hw_add("l2s_dscp_vlan_tagging_rxtagging_map_veip", er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_veip);
	er_attr_group_hw_add("l2s_tags_create_add012_veip", er_group_hw_l2s_tags_create_add012_veip);

	/* er_group_hw_47_78_329.c */
	er_attr_group_hw_add("l2s_tags_create_add01_veip", er_group_hw_l2s_tags_create_add01_veip);

	/* er_group_hw_47_130_262_266_268_277_402.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_802_1p_tm_rate_controlled", er_group_hw_mac_gem_bridge_allocate_802_1p_tm_rate_controlled);

	/* er_group_hw_47_130_266_268_277_402.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_802_1p", er_group_hw_mac_gem_bridge_allocate_802_1p);

	/* er_group_hw_47_130.c */
	er_attr_group_hw_add("l2s_dscp_port_enable", er_group_hw_l2s_dscp_port_enable);
	er_attr_group_hw_add("l2s_dscp_port_map", er_group_hw_l2s_dscp_port_map);
	er_attr_group_hw_add("l2s_dscp_port_default", er_group_hw_l2s_dscp_port_default);

	/* er_group_hw_47_171.c */
	er_attr_group_hw_add("l2s_dscp_vlan_tagging_rxtagging_map_bridge", er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_bridge);
	er_attr_group_hw_add("l2s_tags_create_add012_bridge", er_group_hw_l2s_tags_create_add012_bridge);

	/* er_group_hw_47_266_268_401.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_broadcast_gem_iw_tp", er_group_hw_mac_gem_bridge_allocate_broadcast_gem_iw_tp);

	/* er_group_hw_47_266_268_277_402.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_gem_iw_tp", er_group_hw_mac_gem_bridge_allocate_gem_iw_tp);

	/* er_group_hw_47_262_266_268_277_402.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_gem_iw_tp_tm_rate_controlled", er_group_hw_mac_gem_bridge_allocate_gem_iw_tp_tm_rate_controlled);

	/* er_group_hw_47_281_268.c */
	er_attr_group_hw_add("mac_gem_bridge_allocate_multicast_gem_iw_tp", er_group_hw_mac_gem_bridge_allocate_multicast_gem_iw_tp);

	/* er_group_hw_47_280_403.c */
	er_attr_group_hw_add("mac_traffic_control_td_inbound", er_group_hw_mac_traffic_control_td_inbound);
	/* er_group_hw_47_280_404.c */
	er_attr_group_hw_add("mac_traffic_control_td_outbound", er_group_hw_mac_traffic_control_td_outbound);

	/* er_group_hw_47_48.c */
	er_attr_group_hw_add("l2s_bridging_stp_portstate", er_group_hw_l2s_bridging_stp_portstate);

	/* er_group_hw_47_78_130.c */
	er_attr_group_hw_add("l2s_tags_create_add01_802_1p", er_group_hw_l2s_tags_create_add01_802_1p);

	/* er_group_hw_47_78_134.c */
	er_attr_group_hw_add("l2s_tags_create_add01_ip_host", er_group_hw_l2s_tags_create_add01_ip_host);

	/* er_group_hw_47_78.c */
	er_attr_group_hw_add("l2s_tags_create_add01_bridge", er_group_hw_l2s_tags_create_add01_bridge);

	/* er_group_hw_47_84.c */
	er_attr_group_hw_add("l2s_filt_configure", er_group_hw_l2s_filt_configure);
	er_attr_group_hw_add("l2s_filt_create_vid_tci_pri", er_group_hw_l2s_filt_create_vid_tci_pri);

	/* er_group_hw_47_309_310.c */
	er_attr_group_hw_add("l2s_tags_create_add01_multicast_309", er_group_hw_l2s_tags_create_add01_multicast_309);
	er_attr_group_hw_add("igmp_upstream_rate", er_group_hw_igmp_upstream_rate_set);
	er_attr_group_hw_add("igmp_max_memberships", er_group_hw_igmp_max_membership);
	er_attr_group_hw_add("max_multicast_bandwidth", er_group_hw_max_multicast_bandwidth_set);
	er_attr_group_hw_add("static_acl_table", er_group_hw_static_acl_table);
	er_attr_group_hw_add("mcast_conf", er_group_hw_mcast_conf_set);

	/* er_group_hw_47_329.c */
	er_attr_group_hw_add("sync_veip_autouni_admin_state", er_group_hw_sync_veip_autouni_admin_state);

	/* er_group_hw_53.c */
	er_attr_group_hw_add("physical_path_termination_point_pots_uni", er_group_hw_physical_path_termination_point_pots_uni);

	/* er_group_hw_53_134_136_150_153.c */
	er_attr_group_hw_add("sip_agent_config_data", er_group_hw_sip_agent_config_data);

	/* er_group_hw_53_139.c */
	er_attr_group_hw_add("voip_voice_ctp", er_group_hw_voip_voice_ctp);

	/* er_group_hw_53_139_142.c */
	er_attr_group_hw_add("voip_media_profile", er_group_hw_voip_media_profile);

	/* er_group_hw_53_139_142_143.c */
	er_attr_group_hw_add("rtp_profile_data", er_group_hw_rtp_profile_data);

	/* er_me_group_53_140.c */
	er_attr_group_hw_add("call_control_performance_monitoring_history_data", er_group_hw_call_control_performance_monitoring_history_data);

	/* er_group_hw_53_141.c */
	er_attr_group_hw_add("voip_line_status", er_group_hw_voip_line_status);

	/* er_me_group_53_144.c */
	er_attr_group_hw_add("rtp_performance_monitoring_history_data", er_group_hw_rtp_performance_monitoring_history_data);

	/* er_group_hw_53_145_153.c */
	er_attr_group_hw_add("network_dial_plan_table", er_group_hw_network_dial_plan_table);

	/* er_group_hw_53_146_153.c */
	er_attr_group_hw_add("voip_application_service_profile", er_group_hw_voip_application_service_profile);

	/* er_group_hw_53_147_153.c */
	er_attr_group_hw_add("voip_feature_access_codes", er_group_hw_voip_feature_access_codes);

	/* er_me_group_53_150_151_153.c */
	er_attr_group_hw_add("sip_agent_performance_monitoring_history_data", er_group_hw_sip_agent_performance_monitoring_history_data);

	/* er_me_group_53_150_152_153.c */
	er_attr_group_hw_add("sip_call_initiation_performance_monitoring_history_data", er_group_hw_sip_call_initiation_performance_monitoring_history_data);

	/* er_group_hw_53_153.c */
	er_attr_group_hw_add("sip_user_data", er_group_hw_sip_user_data);

	/* er_group_hw_53_58_139_142.c */
	er_attr_group_hw_add("voice_service_profile", er_group_hw_voice_service_profile);
	
	/* er_group_hw_7.c */
	er_attr_group_hw_add("update_commit_env", er_group_hw_update_commit_env);
	er_attr_group_hw_add("update_active_env", er_group_hw_update_active_env);
	er_attr_group_hw_add("update_valid_env", er_group_hw_update_valid_env);

	/* er_group_hw_131.c */
	er_attr_group_hw_add("olt_vendor_check", er_group_hw_olt_vendor_check);
	er_attr_group_hw_add("timeofday_info", er_group_hw_timeofday_info);

	/* er_group_hw_47_299_300_302.c */
	er_attr_group_hw_add("cfm_bridge_info", er_group_hw_cfm_bridge_info);

	/* er_group_hw_47_130_299_300_302.c */
	er_attr_group_hw_add("cfm_mapper_info", er_group_hw_cfm_mapper_info);

	/* er_group_hw_47_84_299_300.c */
	er_attr_group_hw_add("cfm_mip_info", er_group_hw_cfm_mip_info);

	/* er_group_hw_309.c */
	//er_attr_group_hw_add("igmp_upstream_rate", er_group_hw_igmp_upstream_rate_set);

	/* er_group_hw_158.c */
	er_attr_group_hw_add("remote_debug", er_group_hw_remote_debug);

	/* er_group_hw_334.c */
	er_attr_group_hw_add("extended_pm_334_accum", er_group_hw_334_accum_disable); 
                
	/* er_group_hw_425.c */
	er_attr_group_hw_add("extended_pm_425_accum", er_group_hw_425_accum_disable);        

	/* er_group_hw_82.c */
	er_attr_group_hw_add("video_uni_enable", er_group_hw_video_uni_enable);
	
	/* er_group_hw_83.c */
	er_attr_group_hw_add("physical_path_termination_point_lct_uni", er_group_hw_physical_path_termination_point_lct_uni);

	/* er_group_hw_90.c */
	er_attr_group_hw_add("video_ani_enable", er_group_hw_video_ani_enable);
	er_attr_group_hw_add("agc_setting", er_group_hw_agc_setting);
	
	/* er_group_hw_349.c */
	er_attr_group_hw_add("poe_power_priority", er_group_hw_poe_power_priority);
	er_attr_group_hw_add("poe_class_control", er_group_hw_poe_class_control);

	er_attr_group_hw_add("voip_pots_config", er_group_hw_pots_config);
	return 0;
}

