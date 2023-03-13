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
 * Module  : meinfo_cb
 * File    : meinfo_cb_305.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_send.h"

//#define USE_MEP_ME 1

//classid 304 9.3.25 Dot1ag CFM stack

static unsigned short 
meinfo_cb_305_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me = meinfo_me_get_by_instance_num(45, instance_num);
	if (!parent_me || meinfo_me_is_exist(305, parent_me->meid)) {
		parent_me = meinfo_me_get_by_instance_num(130, instance_num);
		if (!parent_me) {
			dbprintf(LOG_ERR, "unable to find parent me, classid=%u or %u, instance=%u\n", 45, 130, instance_num);	
			return instance_num;
		}
	}
	return parent_me->meid;
}

static int 
meinfo_cb_305_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *parent_me=meinfo_me_get_by_meid(45, me->meid);
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if (!parent_me) {
		parent_me = meinfo_me_get_by_meid(130, me->meid);
		if (!parent_me) {
			dbprintf(LOG_ERR, "unable to find parent me, classid=%u or %u, meid=%u\n", 45, 130, me->meid);	
			return -1;
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 1)) { // Layer 2 type
		struct attr_value_t attr;
		if(parent_me->classid == 130) {
			attr.data = 1;
			meinfo_me_attr_set(me, 1, &attr, 0);
		} else {
			attr.data = 0;
			meinfo_me_attr_set(me, 1, &attr, 0);
		}
	}
#ifdef USE_MEP_ME
	struct meinfo_t *miptr_302 = meinfo_util_miptr(302); 
	if (miptr_302) {
		struct me_t *meptr = NULL;
		list_for_each_entry(meptr, &miptr_302->me_instance_list, instance_node) {
			struct me_t *ma_me=meinfo_me_get_by_meid(300, meinfo_util_me_attr_data(meptr, 3));
			struct me_t *bport_me=meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr, 1));
			if(((parent_me->classid == 45 && meinfo_util_me_attr_data(meptr, 2) == 0) && 
			   (bport_me && (parent_me->meid == meinfo_util_me_attr_data(bport_me, 1)))) ||
			   ((parent_me->classid == 130 && meinfo_util_me_attr_data(meptr, 2) == 1) && 
			   (parent_me->meid == meptr->meid))) {
				if (util_attr_mask_get_bit(attr_mask, 2)) { // MP status table
					unsigned char md_level = 0xFF;
					unsigned short mep_id = meinfo_util_me_attr_data(meptr, 4);
					struct me_t *ma = meinfo_me_get_by_meid(300, meinfo_util_me_attr_data(meptr, 3));
					if(ma) {
						struct me_t *md = meinfo_me_get_by_meid(299, meinfo_util_me_attr_data(ma, 1));
						if(md) md_level = meinfo_util_me_attr_data(md, 1);
					}
					cfm_config_t *cfm_config = NULL;
					if((md_level != 0xFF) && (cfm_config = cfm_mp_find(md_level, mep_id)) != NULL) {
						int ptr = 0;
						char *buf = NULL;
						unsigned short portid = (meinfo_util_me_attr_data(meptr, 2) == 0) ? htons(meinfo_util_me_attr_data(meptr, 1)) : 0;
						unsigned char md_level = cfm_config->md_level;
						unsigned char direction = (cfm_config->mep_control & 0x10) ? 2 : 1;
						unsigned short pri_vlan = (cfm_config->pri_vlan != 0) ? htons(cfm_config->pri_vlan) : htons(cfm_config->assoc_vlans[0]);
						unsigned short md_ptr = (ma_me != NULL) ? htons(meinfo_util_me_attr_data(ma_me, 1)) : 0;
						unsigned short ma_ptr = htons(meinfo_util_me_attr_data(meptr, 3));
						unsigned short mep_id = htons(cfm_config->mep_id);
						buf = malloc_safe(18);
						if(buf) {
							// Port id
							memcpy(buf+ptr, (char *) &portid, sizeof(unsigned short));
							ptr += sizeof(unsigned short);
							// Level
							memcpy(buf+ptr, (char *) &md_level, sizeof(unsigned char));
							ptr += sizeof(unsigned char);
							// Direction
							memcpy(buf+ptr, (char *) &direction, sizeof(unsigned char));
							ptr += sizeof(unsigned char);
							// VLAN ID
							memcpy(buf+ptr, (char *) &pri_vlan, sizeof(unsigned short));
							ptr += sizeof(unsigned short);
							// MD
							memcpy(buf+ptr, (char *) &md_ptr, sizeof(unsigned short));
							ptr += sizeof(unsigned short);
							// MA
							memcpy(buf+ptr, (char *) &ma_ptr, sizeof(unsigned short));
							ptr += sizeof(unsigned short);
							// MEP ID
							memcpy(buf+ptr, (char *) &mep_id, sizeof(unsigned short));
							ptr += sizeof(unsigned short);
							// MAC address
							memcpy(buf+ptr, (char *) cfm_config->macaddr, IFHWADDRLEN);
							
							meinfo_me_attr_set_table_entry(me, 2, buf, 0);
							//meinfo_me_attr_set_table_all_entries_by_mem(me, 2, 18, buf, 0);
							free_safe(buf);
						}
					}
				}
			}
		}
	}
#else // USE_MEP_ME
	if (util_attr_mask_get_bit(attr_mask, 2)) { // MP status table
		cfm_config_t *cfm_config = NULL;
		list_for_each_entry(cfm_config, &cfm_config_list, node) {
			struct me_t *meptr_422 = NULL;
			struct meinfo_t *miptr = meinfo_util_miptr(422);
			int is_found = 0, ptr = 0;
			char *buf = NULL;
			unsigned short portid = 0;
			unsigned char md_level = cfm_config->md_level;
			unsigned char direction = (cfm_config->mep_control & 0x10) ? 2 : 1;
			unsigned short pri_vlan = (cfm_config->pri_vlan != 0) ? htons(cfm_config->pri_vlan) : htons(cfm_config->assoc_vlans[0]);
			unsigned short md_ptr = htons(cfm_config->md_meid);
			unsigned short ma_ptr = htons(cfm_config->ma_meid);
			unsigned short mep_id = (cfm_config->ma_mhf_creation != MA_MHF_CREATION_NONE) ? 0 : htons(cfm_config->mep_id);
			unsigned char local_mac[IFHWADDRLEN];
				
			list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
				if((meinfo_util_me_attr_data(meptr_422, 4) == cfm_config->port_type) &&
				   (meinfo_util_me_attr_data(meptr_422, 5) == cfm_config->port_type_index) &&
				   (meinfo_util_me_attr_data(meptr_422, 6) == cfm_config->logical_port_id)) {
					is_found = 1;
					break;
				}
				if(is_found) break;
			}
			if(is_found && (meinfo_util_me_attr_data(meptr_422, 1) == 1)) {
				struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr_422, 2));
				if(!me_related(meptr_47, parent_me)) continue;
				if(cfm_config->mep_meid) { // MEP, check if Layer2_Type is matched
					struct me_t *meptr_302 = meinfo_me_get_by_meid(302, cfm_config->mep_meid);
					if((meinfo_util_me_attr_data(meptr_302, 2) == 0) && parent_me->classid != 45) continue;
					if((meinfo_util_me_attr_data(meptr_302, 2) == 1) && parent_me->classid != 130) continue;
				} else { // MIP, only associates to MAC bridge, no 802.1p mapper
					if(parent_me->classid == 130) continue;
				}
			} else {
				continue;
			}
			
			if(parent_me->classid != 130) {
				struct me_t *meptr_302 = meinfo_me_get_by_meid(302, cfm_config->mep_meid);
				if(meptr_302) { // MEP
					portid = htons(meinfo_util_me_attr_data(meptr_302, 1));
				} else { // MIP
					is_found = 0;
					list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
						if((meinfo_util_me_attr_data(meptr_422, 4) == cfm_config->port_type) &&
						   (meinfo_util_me_attr_data(meptr_422, 5) == cfm_config->port_type_index) &&
						   (meinfo_util_me_attr_data(meptr_422, 6) == cfm_config->logical_port_id)) {
							is_found = 1;
							break;
						}
						if(is_found) break;
					}
					if(is_found && (meinfo_util_me_attr_data(meptr_422, 1) == 1))
						portid = htons(meinfo_util_me_attr_data(meptr_422, 2));
		}
	}
			cfm_util_get_mac_addr(cfm_config, local_mac, "eth0");
			buf = malloc_safe(18);
			if(buf) {
				// Port id
				memcpy(buf+ptr, (char *) &portid, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// Level
				memcpy(buf+ptr, (char *) &md_level, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// Direction
				memcpy(buf+ptr, (char *) &direction, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// VLAN ID
				memcpy(buf+ptr, (char *) &pri_vlan, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// MD
				memcpy(buf+ptr, (char *) &md_ptr, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// MA
				memcpy(buf+ptr, (char *) &ma_ptr, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// MEP ID
				memcpy(buf+ptr, (char *) &mep_id, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// MAC address
				memcpy(buf+ptr, (char *) &local_mac, IFHWADDRLEN);
	
				meinfo_me_attr_set_table_entry(me, 2, buf, 0);
				//meinfo_me_attr_set_table_all_entries_by_mem(me, 2, 18, buf, 0);
				free_safe(buf);
			}
		}
	}
#endif // USE_MEP_ME
	return 0;
}

struct meinfo_callback_t meinfo_cb_305 = {
	.meid_gen_cb	= meinfo_cb_305_meid_gen,
	.get_cb		= meinfo_cb_305_get,
};
