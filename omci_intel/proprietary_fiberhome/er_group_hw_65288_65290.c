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
 * Module  : proprietary_fiberhome
 * File    : er_group_hw_65288_65290.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "acl.h"
#include "switch.h"

#define MAX_FE_ACL_RULE_NUM 	8

//65288 omci_me_sw_port_acl_rule
//65288@meid, 	65288@1, 	65288@2, 	65288@3, 		65288@4, 	65288@5, 	65288@6
//meid, 	us_acl_action, 	us_acl_count, 	us_port_acl_entry, 	ds_acl_action, 	ds_acl_count,	ds_port_acl_entry

/*
us_acl_action:0:forward, 1:deny, default:1(04ONU only support 1) 
us_acl_count:0-4 (for table)
us_port_acl_entry:

ds_acl_action:0:forward, 1:deny, default:1(04ONU only support 1) 
ds_acl_count:0-4 (for table)
ds_port_acl_entry:
*/
struct field_t {
	unsigned char rule_operator;
	char smac[13], dmac[13];
	unsigned short ethertype;
	unsigned int srcip_start, srcip_end;
	unsigned int dstip_start, dstip_end; 
	unsigned int sport_start, sport_end;
	unsigned int dport_start, dport_end;
	unsigned short vid_start, vid_end;
	unsigned char pbit;
	unsigned char protocol;
	unsigned char tos;
};

static int current_us_rule_num_by_port[ACL_UNI_TOTAL];
static int current_ds_rule_num_by_port[ACL_UNI_TOTAL];
static int threshold_rule_num_by_port[ACL_UNI_TOTAL];

static int get_parameter(struct me_t *me_ptr, unsigned char attr_order, struct field_t *field, unsigned char rule_limit)
{

	struct get_attr_table_context_t table_context;
	unsigned char entry_data[10], local_rule_number=0;
	unsigned short rule_field;
	unsigned char rule_field_value[6];
	unsigned char index;	
	unsigned char *m;
	//struct in_addr in;


	if (meinfo_me_attr_get_table_entry(me_ptr, attr_order, &table_context, entry_data) == MEINFO_RW_OK) {
		do {
			index=util_bitmap_get_value(entry_data, 10*8, 0, 8);
			rule_field=util_bitmap_get_value(entry_data, 10*8, 8, 16);
			field->rule_operator=util_bitmap_get_value(entry_data, 10*8, 72, 8);

			//dbprintf(LOG_ERR, "index=%d\n", index);	
			//dbprintf(LOG_ERR, "rule_field=%x\n", rule_field);		//type
			//dbprintf(LOG_ERR, "rule_operator=%d\n", field->rule_operator);	//should == 0

			switch(rule_field) {
			case 0x00:	//Source MAC ADDR(6)
				m=entry_data+3;
				snprintf(field->smac,13,"%02x%02x%02x%02x%02x%02x",m[0], m[1], m[2], m[3], m[4], m[5]);
			break;
			case 0x01:	//Destination MAC ADDR(6)
				m=entry_data+3;
				snprintf(field->dmac,13,"%02x%02x%02x%02x%02x%02x",m[0], m[1], m[2], m[3], m[4], m[5]);
			break;
			case 0x02:	//Source IP ADDR(4bytes)
				if(field->rule_operator==2) { // "<="
					//field->srcip_end=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->srcip_end=util_bitmap_get_value(entry_data, 10*8, 24, 32);
					//in.s_addr=htonl(field->srcip_end);
				} else {
					//field->srcip_start=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->srcip_start=util_bitmap_get_value(entry_data, 10*8, 24, 32);
					//in.s_addr=htonl(field->srcip_start);	// conv udata to network order for inet_ntoa
				}
			break;
			case 0x03:	//Destination IP ADDR(4bytes)
				if(field->rule_operator==2) { // "<="
					//field->dstip_end=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->dstip_end=util_bitmap_get_value(entry_data, 10*8, 24, 32);
					//in.s_addr=htonl(field->dstip_end);	// conv udata to network order for inet_ntoa
				} else {
					//field->dstip_start=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->dstip_start=util_bitmap_get_value(entry_data, 10*8, 24, 32);
					//in.s_addr=htonl(field->dstip_start);	// conv udata to network order for inet_ntoa
				}
			break;
			case 0x04:	//VLAN ID(2bytes,0-4085)
				if(field->rule_operator==2) { // "<="
					//field->vid_end=util_bitmap_get_value(entry_data, 10*8, 24+32, 16);
					field->vid_end=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				} else {
					//field->vid_start=util_bitmap_get_value(entry_data, 10*8, 24+32, 16);
					field->vid_start=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				}
			break;
			case 0x05:	//Ethernet Type(2bytes)
				//field->ethertype=util_bitmap_get_value(entry_data, 10*8, 24+32, 16);
				field->ethertype=util_bitmap_get_value(entry_data, 10*8, 24, 16);
			break;
			case 0x06:	//IP protocol(1bytes)
				//field->protocol=util_bitmap_get_value(entry_data, 10*8, 24+40, 8);
				field->protocol=util_bitmap_get_value(entry_data, 10*8, 24, 8);
			break;
			case 0x07:	//Ethernet Priority(1bytes, 0-7, 0xff don't care)
				//field->pbit=util_bitmap_get_value(entry_data, 10*8, 24+40, 8);
				field->pbit=util_bitmap_get_value(entry_data, 10*8, 24, 8);
			break;
			case 0x08:	//IP TOS(1bytes)
				//field->tos=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
				field->tos=util_bitmap_get_value(entry_data, 10*8, 24, 8);
			break;
			case 0x09:	//Source Port(2bytes)
				if(field->rule_operator==2) { // "<="
					//field->sport_end=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->sport_end=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				} else {
					//field->sport_start=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->sport_start=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				}
			break;
			case 0x0A:	//Destination Port(2bytes)
				if(field->rule_operator==2) { // "<="
					//field->dport_end=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->dport_end=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				} else {
					//field->dport_start=util_bitmap_get_value(entry_data, 10*8, 24+16, 32);
					field->dport_start=util_bitmap_get_value(entry_data, 10*8, 24, 16);
				}
			break;
			default:
				dbprintf(LOG_ERR, "error rule_field=%x\n", rule_field);		//type
				dbprintf(LOG_ERR, "rule_field_value=%d\n", rule_field_value);
				dbprintf(LOG_ERR, "rule_operator=%d\n", field->rule_operator);	//should == 0
				dbprintf(LOG_ERR, "index=%d\n", index);	
			}
			memset(entry_data,0,10);
			local_rule_number++;

			if(local_rule_number >= rule_limit) {
				//dbprintf(LOG_ERR,"Rule on port:%d already >= acl_count %d\n", port, rule_limit);
				return 0;
			}
		} while (meinfo_me_attr_get_table_entry_next(me_ptr, attr_order, &table_context, entry_data) == MEINFO_RW_OK);
	}
	return 0;
}

static int 
update_rule_list(struct me_t *me_ptr)
{
	unsigned char port, uplink_port; 
	unsigned char us_acl_action, us_acl_count, ds_acl_action, ds_acl_count;
	//unsigned char *m;
	unsigned char mac_zero[13]={0};
	struct field_t us_field, ds_field;
	struct me_t *acl_me_ptr;

	//init us_field	
	memset(&us_field, 0, sizeof(struct field_t)); 
	us_field.pbit=0xff;
	us_field.tos=0xff;

	//init ds_field	
	memset(&ds_field, 0, sizeof(struct field_t)); 
	ds_field.pbit=0xff;
	ds_field.tos=0xff;

	/*
		port 0: meid 0x00 ~ 0x07
		port 1: meid 0x08 ~ 0x0F
		port 2: meid 0x10 ~ 0x17
		port 3: meid 0x20 ~ 0x27
	*/
	
	port=(me_ptr->meid)/MAX_FE_ACL_RULE_NUM;	//assume meid start from 0
	//dbprintf(LOG_ERR, "port=%d, meid=%d\n", port, me_ptr->meid);

	//65290's meid=0x0 means port 0, 0x02 means port 2
	acl_me_ptr=meinfo_me_get_by_meid(65290, port);
	if(acl_me_ptr == NULL) {
		dbprintf(LOG_ERR, "class 65290 for port %d is not exist(65288's meid=%d)\n", port, me_ptr->meid);
		threshold_rule_num_by_port[port]=1;
	} else {
		threshold_rule_num_by_port[port]=meinfo_util_me_attr_data(acl_me_ptr, 1);
	}

	// check if rule number is correct
	us_acl_count=meinfo_util_me_attr_data(me_ptr, 2);

	/*=== for upstream ===*/
	if( us_acl_count > 4) {
		dbprintf(LOG_ERR, "According to spec. us_acl_action should between 0-4\n");
		return -1;
	}

	if (meinfo_me_attr_get_table_entry_count(me_ptr, 3) > us_acl_count) {	//us_port_acl_entry
		dbprintf(LOG_WARNING, "table rule number > us_acl_count\n");
		//return 0;
	}

	us_acl_action=meinfo_util_me_attr_data(me_ptr, 1);

	/*=== for downstream ===*/
	// check if rule number is correct
	ds_acl_count=meinfo_util_me_attr_data(me_ptr, 5);
	if( ds_acl_count > 4) {
		dbprintf(LOG_ERR, "According to spec. ds_acl_count should between 0-4\n");
		return -1;
	}

	if (meinfo_me_attr_get_table_entry_count(me_ptr, 6) > ds_acl_count) {	//ds_port_acl_entry
		dbprintf(LOG_WARNING, "table rule number > ds_acl_count\n");
		//return 0;
	}

	ds_acl_action=meinfo_util_me_attr_data(me_ptr, 4);

	/* get parameter form me and filled to field struct*/
	if (us_acl_count > 0 )
		get_parameter(me_ptr, 3, &us_field, us_acl_count);

	if (ds_acl_count > 0 )
		get_parameter(me_ptr, 6, &ds_field, ds_acl_count);


	//update upstream ACL rule by port
	if (us_acl_count > 0 ) {
		//Don't need to update upstream ACL rule
		if(threshold_rule_num_by_port[port] ==0 ){
			dbprintf(LOG_WARNING,"Accroding to class 65290: acl_rule_num=0\n", port,threshold_rule_num_by_port[port]);
			return 0;
		}
		if( us_acl_action == 1) {	//deny,configure as black list 
			acl_change_mode(port, ACL_MODE_BLACK);
		} else if( us_acl_action == 0) {//forward ,configure as white list 
			acl_change_mode(port, ACL_MODE_WHITE);
		}
	
		if( memcmp(us_field.smac, mac_zero, 12)!=0 || memcmp(us_field.dmac, mac_zero, 12)!=0 || us_field.ethertype!=0) {
			//m=us_field.dmac;
			//m=us_field.smac;
			//dbprintf(LOG_ERR,"DMAC:%s\n",us_field.dmac);
			//dbprintf(LOG_ERR,"SMAC:%s\n",us_field.smac);

			if(current_us_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}

			if( acl_add_mac(port, us_field.dmac, 0, us_field.smac, 0, us_field.ethertype, 0) != 0 ){
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
				return -1;
			} else {
				current_us_rule_num_by_port[port]++;
			}
		}

		if( us_field.srcip_start != 0 || us_field.dstip_start != 0 || us_field.sport_start != 0 || us_field.dport_start != 0) {
			if((us_field.rule_operator == 2 || us_field.rule_operator == 3) &&
			   !((us_field.srcip_start != 0 && us_field.srcip_end != 0) ||
			   (us_field.dstip_start != 0 && us_field.dstip_end != 0) ||
			   (us_field.sport_start != 0 && us_field.sport_end != 0) ||
			   (us_field.dport_start != 0 && us_field.dport_end != 0))) {
				dbprintf(LOG_ERR,"Rule on port:%d, lack '<=' or '>=' operator\n", port);
				return 0;
			}
			
			if(current_us_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}

			if(us_field.dstip_start != 0 || us_field.srcip_start != 0) {
				if( acl_add_ip(port, ntohl(us_field.dstip_start), ntohl(us_field.dstip_end), ntohl(us_field.srcip_start), ntohl(us_field.srcip_end), 0, 0, 0, 0, 0, 0, 0, 0, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
					return -1;
				} else {
					current_us_rule_num_by_port[port]++;
				}
			}
			
			if(us_field.dport_start != 0 || us_field.sport_start != 0) {
				//user view is only one rule, port export to tcp and udp port
				if( acl_add_ip(port, ntohl(us_field.dstip_start), ntohl(us_field.dstip_end), ntohl(us_field.srcip_start), ntohl(us_field.srcip_end), us_field.dport_start, us_field.dport_end, us_field.sport_start, us_field.sport_end, 0, 0, 0, 0, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
					return -1;
				} else {
					//current_us_rule_num_by_port[port]++;
				}
				
				if( acl_add_ip(port, ntohl(us_field.dstip_start), ntohl(us_field.dstip_end), ntohl(us_field.srcip_start), ntohl(us_field.srcip_end), 0, 0, 0, 0, us_field.dport_start, us_field.dport_end, us_field.sport_start, us_field.sport_end, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
					return -1;
				} else {
					//user view is only one rule
					current_us_rule_num_by_port[port]++;
				}
			}
		}

		if( us_field.vid_start != 0 || us_field.pbit != 0xff) {
			if((us_field.rule_operator == 2 || us_field.rule_operator == 3) &&
			  !(us_field.vid_start != 0 && us_field.vid_end != 0)) {
				dbprintf(LOG_ERR,"Rule on port:%d, lack '<=' or '>='\n", port);
				return 0;
			}
			
			if(current_us_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if( acl_add_vid(port, us_field.vid_start, us_field.vid_end, us_field.pbit, 0) != 0 ) {
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
				return -1;
			} else {
				current_us_rule_num_by_port[port]++;
			}
		}

		if( us_field.protocol != 0 || us_field.tos != 0xff) {
			if(current_us_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if( acl_add_proto(port, us_field.protocol, us_field.tos, 0) != 0) {
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", port);
				return -1;
			} else {
				current_us_rule_num_by_port[port]++;
			}
		}
	} else if (us_acl_count == 0 && current_us_rule_num_by_port[port] ==0 ) {
			//deny,configure as black list for default
			acl_change_mode(port, ACL_MODE_BLACK);
	}

	//update downstream ACL rule by port
	if (ds_acl_count > 0 ) {
		//Don't need to update upstream ACL rule
		if(threshold_rule_num_by_port[port] ==0 ){
			dbprintf(LOG_WARNING,"Accroding to class 65290: acl_rule_num=0\n", port,threshold_rule_num_by_port[port]);
			return 0;
		}
		//set uplink_port= uplink cpu port;
		uplink_port = switch_get_wan_logical_portid();
		//dbprintf(LOG_ERR,"uplink_port:%d\n",uplink_port);

		if( ds_acl_action == 1) {	//deny,configure as black list 
			acl_change_mode(uplink_port, ACL_MODE_BLACK);
		} else if( ds_acl_action == 0) {//forward ,configure as white list 
			acl_change_mode(uplink_port, ACL_MODE_WHITE);
		}
	
		if( memcmp(ds_field.smac, mac_zero, 12)!=0 || memcmp(ds_field.dmac, mac_zero, 12)!=0 || ds_field.ethertype!=0) {
			//m=ds_field.dmac;
			//m=ds_field.smac;
			//dbprintf(LOG_ERR,"DMAC:%s\n",ds_field.dmac);
			//dbprintf(LOG_ERR,"SMAC:%s\n",ds_field.smac);
			if(current_ds_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if( acl_add_mac(uplink_port, ds_field.dmac, 0, ds_field.smac, 0, ds_field.ethertype, 0) != 0) {
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
				return -1;
			} else {
				current_ds_rule_num_by_port[port]++;
			}
		}

		if( ds_field.srcip_start != 0 || ds_field.dstip_start != 0 || ds_field.sport_start != 0 || ds_field.dport_start != 0) {
			if((ds_field.rule_operator == 2 || ds_field.rule_operator == 3) &&
			   !((ds_field.srcip_start != 0 && ds_field.srcip_end != 0) ||
			   (ds_field.dstip_start != 0 && ds_field.dstip_end != 0) ||
			   (ds_field.sport_start != 0 && ds_field.sport_end != 0) ||
			   (ds_field.dport_start != 0 && ds_field.dport_end != 0))) {
				dbprintf(LOG_ERR,"Rule on port:%d, lack '<=' or '>='\n", port);
				return 0;
			}
			
			if(current_ds_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if(ds_field.dstip_start != 0 || ds_field.srcip_start != 0) {
				if( acl_add_ip(uplink_port, ntohl(ds_field.dstip_start), ntohl(ds_field.dstip_end), ntohl(ds_field.srcip_start), ntohl(ds_field.srcip_end), 0, 0, 0, 0, 0, 0, 0, 0, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
					return -1;
				} else {
					current_ds_rule_num_by_port[port]++;
				}
			}
			
			if(ds_field.dport_start != 0 || ds_field.sport_start != 0) {
				//user view is only one rule, port export to tcp and udp port
				if( acl_add_ip(uplink_port, ntohl(ds_field.dstip_start), ntohl(ds_field.dstip_end), ntohl(ds_field.srcip_start), ntohl(ds_field.srcip_end), ds_field.dport_start, ds_field.dport_end, ds_field.sport_start, ds_field.sport_end, 0, 0, 0, 0, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
					return -1;
				} else {
					//current_ds_rule_num_by_port[port]++;
				}
				
				if( acl_add_ip(uplink_port, ntohl(ds_field.dstip_start), ntohl(ds_field.dstip_end), ntohl(ds_field.srcip_start), ntohl(ds_field.srcip_end), 0, 0, 0, 0, ds_field.dport_start, ds_field.dport_end, ds_field.sport_start, ds_field.sport_end, 0) !=0) {
					dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
					return -1;
				} else {
					//user view is only one rule
					current_ds_rule_num_by_port[port]++;
				}
			}
		}

		if( ds_field.vid_start != 0 || ds_field.pbit != 0xff) {
			if((ds_field.rule_operator == 2 || ds_field.rule_operator == 3) &&
			  !(ds_field.vid_start != 0 && ds_field.vid_end != 0)) {
				dbprintf(LOG_ERR,"Rule on port:%d, lack '<=' or '>='\n", port);
				return 0;
			}
			
			if(current_ds_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if( acl_add_vid(uplink_port, ds_field.vid_start, ds_field.vid_end, ds_field.pbit, 0) !=0) {
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
				return -1;
			} else {
				//user view is only one rule, port export to tcp and udp port
				current_ds_rule_num_by_port[port]++;
			}
		}

		if( ds_field.protocol != 0 || ds_field.tos != 0xff) {
			if(current_ds_rule_num_by_port[port] >= threshold_rule_num_by_port[port]){
				dbprintf(LOG_ERR,"Rule on port:%d >= %d(by class 65290)\n", port,threshold_rule_num_by_port[port]);
				return 0;
			}
			
			if( acl_add_proto(uplink_port, ds_field.protocol, ds_field.tos, 0) !=0) {
				dbprintf(LOG_ERR,"Rule on port:%d switch set error\n", uplink_port);
				return -1;
			} else {
				current_ds_rule_num_by_port[port]++;
			}
		}
	} else if (ds_acl_count == 0 && current_ds_rule_num_by_port[port] ==0 ) {
		//set uplink_port= uplink cpu port;
		uplink_port = switch_get_wan_logical_portid();
		//deny,configure as black list for default
		acl_change_mode(uplink_port, ACL_MODE_BLACK);
	}
	return 0;
}

int 
er_group_hw_omci_me_sw_port_acl_rule(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *acl_miptr;
        struct me_t *acl_meptr, *meptr;
#if 1
	struct er_attr_group_instance_t *current_attr_inst;
	struct attr_table_entry_t *entry_old, *entry_new;
#endif
	int i;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		//already create by ONT
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		//get meid
		if ((meptr=er_attr_group_util_attrinst2me(attr_inst, 0))==NULL) {
			dbprintf(LOG_ERR, "get me error\n");
			return -1;
		}
		if( meptr->instance_num == 31) {	//means last me (0-31)
			//clean all acl rule list
			for(i=0; i < ACL_UNI_TOTAL; i++) {
				aclinfo_del_by_port(i);
				current_us_rule_num_by_port[i]=0;
				current_ds_rule_num_by_port[i]=0;
			}
			//clean hw
			dbprintf(LOG_INFO, "last instance flush_to_hw\n");
			aclinfo_flush_to_hw();
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//check if value had been change
#if 1
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//every thing the same: do nothing.
		if( 	me->classid == 65288 &&
			current_attr_inst->er_attr_instance[1].attr_value.data == attr_inst->er_attr_instance[1].attr_value.data &&
			current_attr_inst->er_attr_instance[2].attr_value.data == attr_inst->er_attr_instance[2].attr_value.data &&
			current_attr_inst->er_attr_instance[4].attr_value.data == attr_inst->er_attr_instance[4].attr_value.data &&
			current_attr_inst->er_attr_instance[5].attr_value.data == attr_inst->er_attr_instance[5].attr_value.data &&
			er_attr_group_util_table_op_detection(attr_inst, 3, &entry_old, &entry_new) == ER_ATTR_GROUP_HW_OP_NONE &&
			er_attr_group_util_table_op_detection(attr_inst, 6, &entry_old, &entry_new) == ER_ATTR_GROUP_HW_OP_NONE ) {

			dbprintf(LOG_ERR,"Field the same do nothing\n");
			er_attr_group_util_release_attrinst(current_attr_inst);
			return 0;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);
#endif
		// traverse all classid 65288's me
		// 65288 omci_me_sw_port_acl_rule

		acl_miptr= meinfo_util_miptr(65288);
		if (acl_miptr==NULL) {
			dbprintf(LOG_ERR, "class=%d, null miptr?\n", 65288);
			return -1;
		}

		//clean all acl rule list
		for(i=0; i < ACL_UNI_TOTAL; i++) {
			aclinfo_del_by_port(i);
			current_us_rule_num_by_port[i]=0;
		}

		//collect all acl rule list
		list_for_each_entry(acl_meptr, &acl_miptr->me_instance_list, instance_node) {
			if( update_rule_list(acl_meptr) != 0 ) {
				dbprintf(LOG_ERR,"Unknow operator\n");
				return -1;
			}
		}

		//check another me for per port acl rule number
		aclinfo_flush_to_hw();
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

