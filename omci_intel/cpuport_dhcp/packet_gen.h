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
 * Module  : cpuport_dhcp
 * File    : parse_format.h
 *
 ******************************************************************/

#ifndef __PACKET_GEN_H__
#define __PACKET_GEN_H__

struct symbol_t {
	unsigned char type;
	unsigned char value;
};

struct format_list_node_t {
        struct list_head format_list;
        struct symbol_t symbol;
};

struct lineidformat_t {
	unsigned char slotid_S;
	unsigned short ponid_p;
	unsigned short onuid_authid_o;
	unsigned char symbol_format_type;	//0=USER,1=CTC,2=CNC
	//unsigned char user_define_format[256];
	//unsigned char reserve[32];
}__attribute__ ((packed));;

struct lineidparam_t {
	unsigned char slotid_S;
	unsigned short ponid_p;
	unsigned short onu_authid_o;
	unsigned char access_point_id_a[50];
	unsigned short shelf_id_r;
	unsigned short frame_id_f;
	unsigned char uplink_port_type_u[32];
	unsigned int olt_manage_vlanip_O;
	unsigned char traffic_board_type_L[16];
	unsigned char reserve[64];
}__attribute__ ((packed));;

int packet_gen_parse_format(char *input, int len);
void packet_gen_release_format_list(void);
void packet_gen_dump_format_list(void);
int packet_gen_generate_param(unsigned char **optstr, int *len, unsigned short cvlan);
#endif
