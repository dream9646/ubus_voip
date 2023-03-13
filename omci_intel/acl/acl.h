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
 * Module  : switch
 * File    : switch_ext_acl.h
 *
 ******************************************************************/

#ifndef __ACL_H__
#define __ACL_H__

#define ACL_UNI_TOTAL 12
#define ACL_BLACK_WHITE_NUM 25	//the same as rtl8367 RTL8367_ACL_BLACK_WHITE_NUM
#define ACL_MODE_BLACK 0
#define ACL_MODE_WHITE 1

struct acl_mac_node_t {
	struct list_head head;
	unsigned char dstmac[6];
	unsigned char srcmac[6];
	unsigned char dstmac_mask[6];
	unsigned char srcmac_mask[6];
	unsigned short ethertype;
};

struct acl_ip_node_t {
        struct list_head head;
	unsigned int dstip_start, dstip_end;
	unsigned int srcip_start, srcip_end;
	unsigned short	tcpsport_start, tcpsport_end;
	unsigned short	tcpdport_start, tcpdport_end;
	unsigned short	udpsport_start, udpsport_end;
	unsigned short	udpdport_start, udpdport_end;
};

struct acl_vid_node_t {
        struct list_head head;
	unsigned short	vid_start;
	unsigned short	vid_end;
	unsigned char pbit;	//0xff means no use
};

struct acl_proto_node_t {
        struct list_head head;
	unsigned int proto;
	unsigned char tos;
};
 
/* acl.c */
int aclinfo_del_by_port(int uni_port);
void aclinfo_flush_to_hw(void);
int acl_add_mac(int uni_port, char *dstmac, char *dstmac_mask, char *srcmac, char *srcmac_mask, unsigned short ethertype, unsigned char flush_hw);
int acl_add_ip(int uni_port, unsigned int dstip_start, unsigned int dstip_end, unsigned int srcip_start, unsigned int srcip_end, unsigned short tcpdport_start, unsigned short tcpdport_end, unsigned short tcpsport_start, unsigned short tcpsport_end, unsigned short udpdport_start, unsigned short udpdport_end, unsigned short udpsport_start, unsigned short udpsport_end, unsigned char flush_hw);
int acl_add_vid(int uni_port, unsigned short vid_start, unsigned short vid_end, unsigned char pbit, unsigned char flush_hw);
int acl_add_proto(int uni_port, unsigned char proto, unsigned char tos, unsigned char flush_hw);
int acl_del_mac(int uni_port, char *dstmac, char *dstmac_mask, char *srcmac, char *srcmac_mask, unsigned short ethertype);
int acl_del_ip(int uni_port, unsigned int dstip_start, unsigned int dstip_end, unsigned int srcip_start, unsigned int srcip_end, unsigned short tcpdport_start, unsigned short tcpdport_end, unsigned short tcpsport_start, unsigned short tcpsport_end, unsigned short udpdport_start, unsigned short udpdport_end, unsigned short udpsport_start, unsigned short udpsport_end);
int acl_del_vid(int uni_port, unsigned short vid_start, unsigned short vid_end, unsigned char pbit);
int acl_del_proto(int uni_port, unsigned char proto, unsigned char tos);
int acl_del_by_port(int uni_port);
int acl_change_mode(int uni_port, unsigned char mode);
int acl_reset(void);
unsigned int acl_dump(int fd, int uni_port);


#endif

