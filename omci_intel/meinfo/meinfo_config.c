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
 * Module  : meinfo
 * File    : meinfo_config.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
					  
#include "meinfo.h"
#include "notify_chain.h"
#include "list.h"
#include "util.h"
#include "env.h"
#include "me_related.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

#define MEINFO_CONFIG_TCONT_TOTAL	32
#define MEINFO_CONFIG_TS_PER_TCONT	8
#define MEINFO_CONFIG_PQ_TOTAL		128

struct meinfo_config_tcont_t {
	int is_valid;
	int pq_start;
	int pq_end;
	int pq_per_ts;
	unsigned short meid;
	unsigned short ts_meid[MEINFO_CONFIG_TS_PER_TCONT];
};

struct meinfo_config_pq_t {
	int is_valid;
	int tcont;
	int ts;
	int priority;
};

static struct meinfo_config_tcont_t meinfo_config_tcont[MEINFO_CONFIG_TCONT_TOTAL];
static struct meinfo_config_pq_t meinfo_config_pq[MEINFO_CONFIG_PQ_TOTAL];

static int
meinfo_config_find_tcont_for_pq(int pq)
{
	int i;

	for (i=0; i< MEINFO_CONFIG_TCONT_TOTAL; i++) {
		if ( meinfo_config_tcont[i].is_valid &&
			pq >= meinfo_config_tcont[i].pq_start &&
			pq <= meinfo_config_tcont[i].pq_end)
		     	return i;
	}
	return -1;
}

static int
meinfo_config_find_ts_for_pq(int tcont, int pq)
{
	if (tcont<0 || !meinfo_config_tcont[tcont].is_valid)
		return -1;
	if (pq >meinfo_config_tcont[tcont].pq_end)
		return -2;
	return (pq-meinfo_config_tcont[tcont].pq_start) / meinfo_config_tcont[tcont].pq_per_ts;
}

static int
meinfo_config_find_priority_for_pq(int tcont, int pq)
{
	if (tcont<0 || !meinfo_config_tcont[tcont].is_valid)
		return -1;
	if (pq >meinfo_config_tcont[tcont].pq_end)
		return -2;
	return (pq-meinfo_config_tcont[tcont].pq_start) % meinfo_config_tcont[tcont].pq_per_ts;
}

static int
meinfo_config_tcont_map_load()
{
	int i;
	int pq_total=0;
	
	// calc meinfo_config_tcont for later use
	for (i=0; i< MEINFO_CONFIG_TCONT_TOTAL; i++) {
		if (omci_env_g.tcont_map_pq[i] <=0 || omci_env_g.tcont_map_ts[i] <= 0)
			continue;
		meinfo_config_tcont[i].is_valid=1;
		meinfo_config_tcont[i].pq_start=pq_total;
		meinfo_config_tcont[i].pq_end=meinfo_config_tcont[i].pq_start+omci_env_g.tcont_map_pq[i]-1;
		meinfo_config_tcont[i].pq_per_ts=(omci_env_g.tcont_map_pq[i]+omci_env_g.tcont_map_ts[i]-1)/omci_env_g.tcont_map_ts[i];
		pq_total+=omci_env_g.tcont_map_pq[i];
	}

	// find attr for each pq
	for (i=0; i< MEINFO_CONFIG_PQ_TOTAL; i++) {
		int tcont=meinfo_config_find_tcont_for_pq(i);
		if (tcont>=0) {
			meinfo_config_pq[i].is_valid=1;
			meinfo_config_pq[i].tcont=tcont;
			meinfo_config_pq[i].ts=meinfo_config_find_ts_for_pq(tcont, i);
			meinfo_config_pq[i].priority=meinfo_config_find_priority_for_pq(tcont, i);
		}			
	}
	
	return i;
}

static int
meinfo_config_tcont_meid_load()
{
	struct meinfo_t *miptr_tcont = meinfo_util_miptr(262); 
	struct meinfo_instance_def_t *instance_def_ptr;
	int i=0, total=0;

	list_for_each_entry(instance_def_ptr, &miptr_tcont->config.meinfo_instance_def_list, instance_node) {
		if (i < MEINFO_CONFIG_TCONT_TOTAL ) {
			meinfo_config_tcont[i].meid=instance_def_ptr->default_meid;
			total++;
		}
		i++;
	}
	return total;
}

static int
meinfo_config_tcont_ts_meid_load(int tcont)
{
	struct meinfo_t *miptr_ts = meinfo_util_miptr(278); 
	struct meinfo_instance_def_t *instance_def_ptr;
	int i=0;

	list_for_each_entry(instance_def_ptr, &miptr_ts->config.meinfo_instance_def_list, instance_node) {
		int tcont_curr = i/MEINFO_CONFIG_TS_PER_TCONT;
		int ts = i%MEINFO_CONFIG_TS_PER_TCONT;
		if (tcont==tcont_curr) {
			meinfo_config_tcont[tcont].ts_meid[ts]=instance_def_ptr->default_meid;
		}
		i++;
	}
	return i;
}

int
meinfo_config_tcont_ts_pq_remap(void)
{
	struct meinfo_t *miptr_tcont = meinfo_util_miptr(262); 
	struct meinfo_t *miptr_ts = meinfo_util_miptr(278); 
	struct meinfo_t *miptr_pq = meinfo_util_miptr(277); 
	struct meinfo_instance_def_t *instance_def_ptr;
	int i, tcont, tcont_total;
	
	// load tcont_map
	meinfo_config_tcont_map_load();

	// load meid of tcont and ts
	tcont_total=meinfo_config_tcont_meid_load();
	for (tcont=0; tcont<tcont_total; tcont++) {
		meinfo_config_tcont_ts_meid_load(tcont);
	}

	// change attr3(policy) in tcont
	list_for_each_entry(instance_def_ptr, &miptr_tcont->config.meinfo_instance_def_list, instance_node) {
		instance_def_ptr->custom_default_value[3].data=omci_env_g.tcont_map_policy[0];
	}

	// change attr1(tcont pointer),attr3(policy), attr4(priority/weight) in all ts under per tcont
	for (tcont=0; tcont<tcont_total; tcont++) {
		if (meinfo_config_tcont[tcont].is_valid) {
			unsigned short tcont_meid=meinfo_config_tcont[tcont].meid;
			i=0;
			//dbprintf(LOG_WARNING, "tcont_meid=0x%x\n", tcont_meid);
			// find all ts belonging to this tcont
			list_for_each_entry(instance_def_ptr, &miptr_ts->config.meinfo_instance_def_list, instance_node) {
				if (instance_def_ptr->custom_default_value[1].data==tcont_meid) {
					//dbprintf(LOG_WARNING, "ts_meid=0x%x\n", instance_def_ptr->default_meid);
					instance_def_ptr->custom_default_value[1].data=meinfo_config_tcont[tcont].meid;
					instance_def_ptr->custom_default_value[3].data=omci_env_g.tcont_map_policy[1];
					if (omci_env_g.tcont_map_policy[0]==1) {	// SP
						instance_def_ptr->custom_default_value[4].data=i;	// priority
						i++;
					} else {	// WRR
						instance_def_ptr->custom_default_value[4].data=1;	// weight
					}
				}
			}
		}
	}	

	// change attr6(tcont pointer, priority) and attr7(ts pointer) in meinfo_config_t for all upstream pq
	i=0;
	list_for_each_entry(instance_def_ptr, &miptr_pq->config.meinfo_instance_def_list, instance_node) {
		if (instance_def_ptr->default_meid & 0x8000) { // for upstream pq, instance num msb==1
			if (i < MEINFO_CONFIG_PQ_TOTAL) {
				struct meinfo_config_pq_t *pq=&meinfo_config_pq[i];
				unsigned short tcont_meid=meinfo_config_tcont[pq->tcont].meid;
				unsigned short ts_meid=meinfo_config_tcont[pq->tcont].ts_meid[pq->ts];
				unsigned int *related_port=instance_def_ptr->custom_default_value[6].ptr;
			
				// attr6: 32bit bitfield, 0..15: tcont pointer, 16..31: priority
				*related_port=htonl(tcont_meid<<16|pq->priority);
				// attr7: 16bit, ts pointer
				instance_def_ptr->custom_default_value[7].data=ts_meid;							
				i++;
			}
		}			
	}

	return 0;
}


/*
struct meinfo_config_tcont_t {
	int is_valid;
	int pq_start;
	int pq_end;
	int pq_per_ts;
	unsigned short meid;
	unsigned short ts_meid[MEINFO_CONFIG_TS_PER_TCONT];
};

struct meinfo_config_pq_t {
	int is_valid;
	int tcont;
	int ts;
	int priority;
};

static struct meinfo_config_tcont_t meinfo_config_tcont[MEINFO_CONFIG_TCONT_TOTAL];
static struct meinfo_config_pq_t meinfo_config_pq[MEINFO_CONFIG_PQ_TOTAL];
*/

int
meinfo_config_tcont_ts_pq_dump(void)
{
	struct meinfo_t *miptr_pq = meinfo_util_miptr(277); 
	struct meinfo_instance_def_t *instance_def_ptr;
	int i, j;

	for (i=0; i<MEINFO_CONFIG_TCONT_TOTAL; i++) {
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%stcont[%d]: meid=0x%x", 
			meinfo_config_tcont[i].is_valid?"+":"-", i, 
			meinfo_config_tcont[i].meid);
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, ", pq=%d..%d, per_ts_pq=%d", 
			meinfo_config_tcont[i].pq_start,
			meinfo_config_tcont[i].pq_end,
			meinfo_config_tcont[i].pq_per_ts);
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, ", ts_meid="); 
		for (j=0; j<MEINFO_CONFIG_TS_PER_TCONT; j++) {
			if (j <omci_env_g.tcont_map_ts[i])
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "0x%x ", meinfo_config_tcont[i].ts_meid[j]); 
		}
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n"); 
	}

	i=0;
	list_for_each_entry(instance_def_ptr, &miptr_pq->config.meinfo_instance_def_list, instance_node) {
		if (instance_def_ptr->default_meid & 0x8000) { // for upstream pq, instance num msb==1
			if (i < MEINFO_CONFIG_PQ_TOTAL) {
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%spq[%d]: meid=0x%x, tcont=%d, ts=%d, prio=%d", 
					meinfo_config_pq[i].is_valid?"+":"-", i, 
					instance_def_ptr->default_meid,
					meinfo_config_pq[i].tcont,
					meinfo_config_pq[i].ts,
					meinfo_config_pq[i].priority);
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\n"); 
				i++;
			}
		}			
	}

	return 0;
}

int
meinfo_config_pptp_uni(int admin_lock)
{
	struct meinfo_t *miptr_pptpuni = meinfo_util_miptr(11); 
	struct meinfo_instance_def_t *instance_def_ptr;

	if (miptr_pptpuni==NULL)
		return -1;
	if (admin_lock==0 || admin_lock==1) {
		list_for_each_entry(instance_def_ptr, &miptr_pptpuni->config.meinfo_instance_def_list, instance_node) {
			// attr 5: Administrative_state
			instance_def_ptr->custom_default_value[5].data=admin_lock;
		}
	} // else value is ignored
	return 0;
}

int
meinfo_config_card_holder(void)
{
	struct meinfo_t *miptr_cardholder = meinfo_util_miptr(5); 
	struct meinfo_instance_def_t *instance_def_ptr_cardholder;
	struct meinfo_t *miptr; 
	struct meinfo_instance_def_t *instance_def_ptr;

	if (miptr_cardholder==NULL)
		return -1;

	list_for_each_entry(instance_def_ptr_cardholder, &miptr_cardholder->config.meinfo_instance_def_list, instance_node) {
		unsigned unit_type=instance_def_ptr_cardholder->custom_default_value[2].data;
		int port_count=0;
		
		if (unit_type==48) {	// veip
			unsigned short private_classid=hwresource_is_related(329);
			miptr= meinfo_util_miptr(private_classid?private_classid:329);
			list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
				port_count++;					

		} else if (unit_type==24 || unit_type==34 || unit_type==47) { // pptp uni: 10/100baseT, Giga Optical, 10/100/100BaseT
			unsigned int port_id;
			for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
				if ((1<<port_id) & (switch_get_uni_logical_portmask()|switch_get_wifi_logical_portmask()))
					port_count++;
			}
		} else if (unit_type==32) {	// pots
			if(omci_env_g.voip_enable!=ENV_VOIP_DISABLE) {
				unsigned short private_classid=hwresource_is_related(53);
				miptr= meinfo_util_miptr(private_classid?private_classid:53);
				list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
					port_count++;
			}
		}
		if (port_count>0) {
			// attr3: expected port_count
			instance_def_ptr_cardholder->custom_default_value[3].data=port_count;
		}
	}
	return 0;
}

int
meinfo_config_circuit_pack(void)
{
	struct meinfo_t *miptr_circuitpack = meinfo_util_miptr(6); 
	struct meinfo_instance_def_t *instance_def_ptr_circuitpack;
	struct meinfo_t *miptr; 
	struct meinfo_instance_def_t *instance_def_ptr;

	if (miptr_circuitpack==NULL)
		return -1;

	list_for_each_entry(instance_def_ptr_circuitpack, &miptr_circuitpack->config.meinfo_instance_def_list, instance_node) {
		unsigned unit_type=instance_def_ptr_circuitpack->custom_default_value[1].data;
		int port_count=0;
		int pq_count=0;
		
		if (unit_type==48) {	// veip
			unsigned short private_classid=hwresource_is_related(329);
			miptr= meinfo_util_miptr(private_classid?private_classid:329);
			list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
				port_count++;
			pq_count=port_count*8;
		} else if (unit_type==24 || unit_type==34 || unit_type==47) { // pptp uni: 10/100baseT, Giga Optical, 10/100/100BaseT
			unsigned int port_id;
			for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
				if ((1<<port_id) & (switch_get_uni_logical_portmask()|switch_get_wifi_logical_portmask()))
					port_count++;
			}
			pq_count=port_count*8;
		} else if (unit_type==32) { // pots
			if(omci_env_g.voip_enable != ENV_VOIP_DISABLE) {
				unsigned short private_classid=hwresource_is_related(53);
				miptr= meinfo_util_miptr(private_classid?private_classid:53);
				list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
					port_count++;
			}
			pq_count=0;
		}
		if (port_count>0) {
			// attr2: number of ports
			instance_def_ptr_circuitpack->custom_default_value[2].data=port_count;
			// attr12: number of pq
			instance_def_ptr_circuitpack->custom_default_value[12].data=pq_count;
		}
	}
	return 0;
}
#if 0 // Remove VEIP related workaround
// config devname for veip/iphost to pon0.vid 
// where vid is defined in omcienv classf_veip_orig_tci_map[]
int
meinfo_config_iphost(void)
{
	struct meinfo_t *miptr_iphost = meinfo_util_miptr(134); 
	struct meinfo_instance_def_t *instance_def_ptr_iphost;

	if (miptr_iphost==NULL)
		return -1;
	list_for_each_entry(instance_def_ptr_iphost, &miptr_iphost->config.meinfo_instance_def_list, instance_node) {
		if (instance_def_ptr_iphost->instance_num < DEVICE_NAME_MAX_LEN &&
		    omci_env_g.classf_veip_orig_tci_map[instance_def_ptr_iphost->instance_num] >0) {
			snprintf(instance_def_ptr_iphost->devname, DEVICE_NAME_MAX_LEN, "pon0.%d",
				omci_env_g.classf_veip_orig_tci_map[instance_def_ptr_iphost->instance_num]);
		}
	}
	return 0;
}

int
meinfo_config_veip(void)
{
	struct meinfo_t *miptr_veip = meinfo_util_miptr(329); 
	struct meinfo_instance_def_t *instance_def_ptr_veip;

	if (miptr_veip==NULL)
		return -1;
	list_for_each_entry(instance_def_ptr_veip, &miptr_veip->config.meinfo_instance_def_list, instance_node) {
		if (instance_def_ptr_veip->instance_num < DEVICE_NAME_MAX_LEN &&
		    omci_env_g.classf_veip_orig_tci_map[instance_def_ptr_veip->instance_num] >0) {
			snprintf(instance_def_ptr_veip->devname, DEVICE_NAME_MAX_LEN, "pon0.%d",
				omci_env_g.classf_veip_orig_tci_map[instance_def_ptr_veip->instance_num]);
		}
	}
	return 0;
}
#endif
// eg: me256_inst0_attr9=1234
static int
parse_me_inst_attr_str(char *str, unsigned short *classid, unsigned short *inst_num, unsigned short *attr_order)
{
	char *s, *endptr;

	// parse classid
	if (strncmp(str, "me", 2) != 0)
		return -1;
	s = str+2;	
	*classid = strtol(s, &endptr, 10);

	// parse inst num
	if (endptr && strncmp(endptr, "_inst", 5) != 0)
		return -2;
	s = endptr + 5;
	*inst_num = strtol(s, &endptr, 10);

	// parse attr_order
	if (endptr && strncmp(endptr, "_attr", 5) != 0)
		return -3;
	s = endptr + 5;
	*attr_order = strtol(s, &endptr, 10);

	// key is not terminated after attr order
	if (endptr != NULL && strlen(endptr)>0)
		return -4;

	return 0;
}	
int
meinfo_config_set_attr_by_str(unsigned short classid, unsigned short inst_num, unsigned char attr_order, char *str)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	struct meinfo_instance_def_t *instance_def_ptr;

	if (miptr == NULL) {
		dbprintf(LOG_ERR, "invalid classid %d\n", classid);
		return -1;
	}
	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "invalid attr_order %d\n", attr_order);
		return -2;
	}
	
	list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node) {
		if (instance_def_ptr->instance_num == inst_num) {
			struct attrinfo_t *aiptr = meinfo_util_aiptr(classid, attr_order);
			struct attr_value_t *attr = &(instance_def_ptr->custom_default_value[attr_order]);

			if (aiptr->format == ATTR_FORMAT_TABLE) {
				dbprintf(LOG_ERR, "classid %d attr %d is table, not supported\n", classid, attr_order);
				return -3;
			}
			if (meinfo_conv_string_to_attr(classid, attr_order, attr, str) < 0) {
				dbprintf(LOG_ERR, "classid %d attr %d inst %d, set to '%s' failed\n", 
					classid, attr_order, inst_num, str);
				return -4;
			}
			return 0;
		}
	} 
	return -5;
}
int
meinfo_config_load_custom_mib(void)
{
	char filename[1024], line[1024];
	char *key, *value, *s;
	unsigned short classid, inst_num, attr_order;
	FILE *fp;
	int count = 0;
	
	snprintf(filename, 1024, "%s/%s", omci_env_g.etc_omci_path, "custom_mib");
	if ((fp = fopen(filename, "r")) == NULL) {
		snprintf(filename, 1024, "%s/%s", omci_env_g.etc_omci_path2, "custom_mib");
		if ((fp = fopen(filename, "r")) == NULL) {
			dbprintf(LOG_ERR, "custom_mib not found in %s & %s\n", 
				omci_env_g.etc_omci_path, omci_env_g.etc_omci_path2);
			return -1;
		}
	}
					
	while( fgets(line, 1024, fp) != NULL) {
		if (line[0]=='#')	// comment, ignore
			continue;
		if ((s = strchr(line, '=')) == NULL)
			continue;	// '=' not found in this line, ignore
		*s = 0;			// sep line into 2 part: key and value
		key = util_trim(line);
		value = util_trim(s+1);
		
		if (parse_me_inst_attr_str(key, &classid, &inst_num, &attr_order) == 0) {
			dbprintf(LOG_WARNING, "custom_mib: me%d, inst%d, attr%d, value=%s\n", 
				classid, inst_num, attr_order, value);		
			meinfo_config_set_attr_by_str(classid, inst_num, attr_order, value);
			count++;
		}
	}
	fclose(fp);
	return count;
}

char *
meinfo_config_get_str_from_custom_mib(unsigned short classid, unsigned short inst_num, unsigned short attr_order)
{
	char filename[1024], line[1024];
	char *key, *value, *s;
	unsigned short classid2, inst_num2, attr_order2;
	FILE *fp;

	snprintf(filename, 1024, "%s/%s", omci_env_g.etc_omci_path, "custom_mib");
	if ((fp = fopen(filename, "r")) == NULL) {
		snprintf(filename, 1024, "%s/%s", omci_env_g.etc_omci_path2, "custom_mib");
		if ((fp = fopen(filename, "r")) == NULL) {
			dbprintf(LOG_WARNING, "custom_mib not found in %s & %s\n", 
				omci_env_g.etc_omci_path, omci_env_g.etc_omci_path2);
			return NULL;
		}
	}

	while( fgets(line, 1024, fp) != NULL) {
		if (line[0]=='#')	// comment, ignore
			continue;
		if ((s = strchr(line, '=')) == NULL)
			continue;	// '=' not found in this line, ignore
		*s = 0;			// sep line into 2 part: key and value
		key = util_trim(line);
		value = util_trim(s+1);

		if (parse_me_inst_attr_str(key, &classid2, &inst_num2, &attr_order2) == 0) {
			dbprintf(LOG_WARNING, "custom_mib: me%d, inst%d, attr%d, value=%s\n", 
				classid2, inst_num2, attr_order2, value);
			if (classid == classid2 && inst_num == inst_num2 && attr_order == attr_order2) {
				static char attr_str[64];
				attr_str[63] = 0;
				strncpy(attr_str, value, 63);
				fclose(fp);
				return attr_str;
			}
		}
	}
	fclose(fp);
	return NULL;
}

