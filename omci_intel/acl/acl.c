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
 * File    : acl.c
 *
 ******************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
#include "acl.h"
#if 1
#ifndef	ACLTEST
#include "util.h"
#else
#include "vacltest_util.h"
#endif

#include "vacl.h"
#include "vacl_util.h"
#endif
#include "util_inet.h"
#include "conv.h"
#include "switch.h"

struct acl_info_t {
	int acl_count;
	struct list_head acl_table;
};

static struct acl_info_t aclinfo_mac[ACL_UNI_TOTAL];
static struct acl_info_t aclinfo_ip[ACL_UNI_TOTAL];
static struct acl_info_t aclinfo_vid[ACL_UNI_TOTAL];
static struct acl_info_t aclinfo_proto[ACL_UNI_TOTAL];

static char aclinfo_mode[ACL_UNI_TOTAL];

static unsigned char is_aclinfo_init;

static int 
aclinfo_init(void)
{
	int i;
	//init function
	for (i = 0; i < ACL_UNI_TOTAL; i++) {
		INIT_LIST_HEAD(&aclinfo_mac[i].acl_table);
		aclinfo_mac[i].acl_count=0;
		INIT_LIST_HEAD(&aclinfo_ip[i].acl_table);
		aclinfo_ip[i].acl_count=0;
		INIT_LIST_HEAD(&aclinfo_vid[i].acl_table);
		aclinfo_vid[i].acl_count=0;
		INIT_LIST_HEAD(&aclinfo_proto[i].acl_table);
		aclinfo_proto[i].acl_count=0;
		aclinfo_mode[i]=ACL_MODE_BLACK;
	}
	return 0;
}

//delete all list head
int 
aclinfo_del_by_port(int uni_port)
{
	struct list_head *pos, *temp;

	if(!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	list_for_each_safe(pos, temp, &aclinfo_mac[uni_port].acl_table) {
		struct acl_mac_node_t *acl_mac_node = list_entry(pos, struct acl_mac_node_t, head);
		list_del(&acl_mac_node->head);
		free_safe(acl_mac_node);
	}
	aclinfo_mac[uni_port].acl_count=0;

	list_for_each_safe(pos, temp, &aclinfo_ip[uni_port].acl_table) {
		struct acl_ip_node_t *acl_ip_node = list_entry(pos, struct acl_ip_node_t, head);
		list_del(&acl_ip_node->head);
		free_safe(acl_ip_node);
	}
	aclinfo_ip[uni_port].acl_count=0;

	list_for_each_safe(pos, temp, &aclinfo_vid[uni_port].acl_table) {
		struct acl_vid_node_t *acl_vid_node = list_entry(pos, struct acl_vid_node_t, head);
		list_del(&acl_vid_node->head);
		free_safe(acl_vid_node);
	}
	aclinfo_vid[uni_port].acl_count=0;

	list_for_each_safe(pos, temp, &aclinfo_proto[uni_port].acl_table) {
		struct acl_proto_node_t *acl_proto_node = list_entry(pos, struct acl_proto_node_t, head);
		list_del(&acl_proto_node->head);
		free_safe(acl_proto_node);
	}
	aclinfo_proto[uni_port].acl_count=0;
	return 0;
}

void
aclinfo_flush_to_hw(void)
{
	int i;

	// FIXME, KEVIN, to be heavily implemented
	
	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	// 1. clear old entries in hw
	// switch_hw_g.acl_conf_clearall(1);

	// 2. flush current actinfo table to hw
	for(i=0; i < ACL_UNI_TOTAL; i++) {
		if(aclinfo_mode[i]==ACL_MODE_WHITE) {
			//switch_hw_g.acl_conf_white(i, &aclinfo_mac[i].acl_table, &aclinfo_ip[i].acl_table, 
			//				&aclinfo_vid[i].acl_table, &aclinfo_proto[i].acl_table);
		} else {
			//switch_hw_g.acl_conf_black(i, &aclinfo_mac[i].acl_table, &aclinfo_ip[i].acl_table, 
			//				&aclinfo_vid[i].acl_table, &aclinfo_proto[i].acl_table);
		}
	}
}

int 
acl_add_mac(int uni_port, char *dstmac, char *dstmac_mask, char *srcmac, char *srcmac_mask, unsigned short ethertype, unsigned char flush_hw)
{
	struct acl_mac_node_t *acl_mac_node;
	char	tmp[3], *ptr, zero[12]={0};
	int i;
	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	if (aclinfo_mac[uni_port].acl_count >= ACL_BLACK_WHITE_NUM) {
		dbprintf(LOG_ERR, "acl rule out of range total num >=%d\n", uni_port, ACL_BLACK_WHITE_NUM);
		return -1;
	}

	acl_mac_node = malloc_safe(sizeof (struct acl_mac_node_t));

	if (dstmac == NULL || (dstmac[0] == '0' && strlen(dstmac) != 12) || memcmp(dstmac, zero, 12) == 0 ) {
		//Don't care dstmac
		memset(acl_mac_node->dstmac, 0x0, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = dstmac[(i*2)];
			tmp[1] = dstmac[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			acl_mac_node->dstmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [dst mac] i=%d, len=%d[%s]\n", i, strlen(dstmac),dstmac);
			}
		}
	}

	if (dstmac_mask == NULL || (dstmac_mask[0] == '0' && strlen(dstmac_mask) != 12) || memcmp(dstmac_mask, zero, 12) == 0 ) {
		//match all dstmac_mask
		memset(acl_mac_node->dstmac_mask, 0xFF, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = dstmac_mask[(i*2)];
			tmp[1] = dstmac_mask[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			acl_mac_node->dstmac_mask[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [dst mac_mask] i=%d, len=%d[%s]\n", i, strlen(dstmac_mask),dstmac_mask);
			}
		}
	}

	if (srcmac == NULL || (srcmac[0] == '0' && strlen(srcmac) != 12) || memcmp(srcmac, zero, 12) == 0 ) {
		//Don't care srcmac
		memset(acl_mac_node->srcmac, 0x0, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = srcmac[(i*2)];
			tmp[1] = srcmac[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			acl_mac_node->srcmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [src mac] i=%d, len=%d[%s]\n", i, strlen(srcmac),srcmac);
			}
		}
	}

	if (srcmac_mask == NULL || (srcmac_mask[0] == '0' && strlen(srcmac_mask) != 12) || memcmp(srcmac_mask, zero, 12) == 0 ) {
		//match all dstmac_mask
		memset(acl_mac_node->srcmac_mask, 0xFF, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = srcmac_mask[(i*2)];
			tmp[1] = srcmac_mask[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			acl_mac_node->srcmac_mask[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [src mac_mask] i=%d, len=%d[%s]\n", i, strlen(srcmac_mask),srcmac_mask);
			}
		}
	}

	acl_mac_node->ethertype=ethertype;

	list_add_tail(&acl_mac_node->head, &aclinfo_mac[uni_port].acl_table);
	aclinfo_mac[uni_port].acl_count++;
	
	if( flush_hw )
		aclinfo_flush_to_hw();
	return 0;
}

int 
acl_add_ip(int uni_port, unsigned int dstip_start, unsigned int dstip_end, unsigned int srcip_start, unsigned int srcip_end, unsigned short tcpdport_start, unsigned short tcpdport_end , unsigned short tcpsport_start, unsigned short tcpsport_end, unsigned short udpdport_start, unsigned short udpdport_end, unsigned short udpsport_start, unsigned short udpsport_end, unsigned char flush_hw)
{
	struct acl_ip_node_t *acl_ip_node;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	if (aclinfo_ip[uni_port].acl_count >= ACL_BLACK_WHITE_NUM) {
		dbprintf(LOG_ERR, "acl rule out of range total num >=%d\n", uni_port, ACL_BLACK_WHITE_NUM);
		return -1;
	}

	acl_ip_node = malloc_safe(sizeof (struct acl_ip_node_t));

	acl_ip_node->dstip_start=dstip_start;
	acl_ip_node->dstip_end=dstip_end;
	acl_ip_node->srcip_start=srcip_start;
	acl_ip_node->srcip_end=srcip_end;
	acl_ip_node->tcpsport_start=tcpsport_start;
	acl_ip_node->tcpsport_end=tcpsport_end;
	acl_ip_node->tcpdport_start=tcpdport_start;
	acl_ip_node->tcpdport_end=tcpdport_end;
	acl_ip_node->udpsport_start=udpsport_start;
	acl_ip_node->udpsport_end=udpsport_end;
	acl_ip_node->udpdport_start=udpdport_start;
	acl_ip_node->udpdport_end=udpdport_end;

	list_add_tail(&acl_ip_node->head, &aclinfo_ip[uni_port].acl_table);
	aclinfo_ip[uni_port].acl_count++;
	
	if( flush_hw )
		aclinfo_flush_to_hw();
	return 0;
}

int 
acl_add_vid(int uni_port, unsigned short vid_start, unsigned short vid_end, unsigned char pbit, unsigned char flush_hw)
{
	struct acl_vid_node_t *acl_vid_node;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}
	
	if (aclinfo_vid[uni_port].acl_count >= ACL_BLACK_WHITE_NUM) {
		dbprintf(LOG_ERR, "acl rule out of range total num >=%d\n", uni_port, ACL_BLACK_WHITE_NUM);
		return -1;
	}

	acl_vid_node = malloc_safe(sizeof (struct acl_vid_node_t));

	acl_vid_node->vid_start=vid_start;
	acl_vid_node->vid_end=vid_end;
	acl_vid_node->pbit=pbit;

	list_add_tail(&acl_vid_node->head, &aclinfo_vid[uni_port].acl_table);
	aclinfo_vid[uni_port].acl_count++;
	
	if( flush_hw )
		aclinfo_flush_to_hw();
	return 0;
}

int 
acl_add_proto(int uni_port, unsigned char proto, unsigned char tos, unsigned char flush_hw)
{
	struct acl_proto_node_t *acl_proto_node;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}
	
	if (aclinfo_proto[uni_port].acl_count >= ACL_BLACK_WHITE_NUM) {
		dbprintf(LOG_ERR, "acl rule out of range total num >=%d\n", uni_port, ACL_BLACK_WHITE_NUM);
		return -1;
	}

	acl_proto_node = malloc_safe(sizeof (struct acl_proto_node_t));

	acl_proto_node->proto=proto;
	acl_proto_node->tos=tos;

	list_add_tail(&acl_proto_node->head, &aclinfo_proto[uni_port].acl_table);
	aclinfo_proto[uni_port].acl_count++;
	
	if( flush_hw )
		aclinfo_flush_to_hw();
	return 0;
}

int 
acl_del_mac(int uni_port, char *dstmac, char *dstmac_mask, char *srcmac, char *srcmac_mask, unsigned short ethertype)
{
	struct list_head *pos, *temp;
	char	tmp[3], *ptr, zero[12]={0};
	int i;
	unsigned char tmpdstmac[6], tmpdstmac_mask[6];
	unsigned char tmpsrcmac[6], tmpsrcmac_mask[6];

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	if (dstmac == NULL || (dstmac[0] == '0' && strlen(dstmac) != 12) || memcmp(dstmac, zero, 12) == 0 ) {
		memset(tmpdstmac, 0x0, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = dstmac[(i*2)];
			tmp[1] = dstmac[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			tmpdstmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [dst mac] i=%d, len=%d[%s]\n", i, strlen(dstmac),dstmac);
			}
		}
	}

	if (dstmac_mask == NULL || (dstmac_mask[0] == '0' && strlen(dstmac_mask) != 12) || memcmp(dstmac_mask, zero, 12) == 0 ) {
		memset(tmpdstmac_mask, 0xFF, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = dstmac_mask[(i*2)];
			tmp[1] = dstmac_mask[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			tmpdstmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [dst mac_mask] i=%d, len=%d[%s]\n", i, strlen(dstmac_mask),dstmac_mask);
			}
		}
	}

	if (srcmac == NULL || (srcmac[0] == '0' && strlen(srcmac) != 12) || memcmp(srcmac, zero, 12) == 0 ) {
		memset(tmpsrcmac, 0x0, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = srcmac[(i*2)];
			tmp[1] = srcmac[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			tmpsrcmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [src mac] i=%d, len=%d[%s]\n", i, strlen(srcmac),srcmac);
			}
		}
	}

	if (srcmac_mask == NULL || (srcmac_mask[0] == '0' && strlen(srcmac_mask) != 12) || memcmp(srcmac_mask, zero, 12) == 0 ) {
		memset(tmpsrcmac_mask, 0xFF, 6);
	} else {
		for(i=0; i < 6; i++) {
			tmp[0] = srcmac_mask[(i*2)];
			tmp[1] = srcmac_mask[(i*2)+1];
			tmp[2] = 0;
   			ptr = tmp;
			tmpsrcmac[i] = strtoul (tmp, &ptr, 16);
			if (ptr != &tmp[2]) {
				dbprintf(LOG_ERR, "Error parsing [src mac] i=%d, len=%d[%s]\n", i, strlen(srcmac_mask),srcmac_mask);
			}
		}
	}

	// find target to delete
	list_for_each_safe(pos, temp, &aclinfo_mac[uni_port].acl_table) {
		struct acl_mac_node_t *acl_mac_node = list_entry(pos, struct acl_mac_node_t, head);
		if (memcmp(acl_mac_node->dstmac, tmpdstmac, sizeof(acl_mac_node->dstmac) ) == 0 && 
			memcmp(acl_mac_node->dstmac_mask, tmpdstmac_mask, sizeof(acl_mac_node->dstmac_mask) ) == 0 && 
			memcmp(acl_mac_node->srcmac, tmpsrcmac, sizeof(acl_mac_node->srcmac) ) == 0 && 
			memcmp(acl_mac_node->srcmac_mask, tmpsrcmac_mask, sizeof(acl_mac_node->srcmac_mask) ) == 0 && 
			(acl_mac_node->ethertype == ethertype)) {
			dbprintf(LOG_ERR, "remove match MAC item\n");
			list_del(&acl_mac_node->head);
			free_safe(acl_mac_node);
			aclinfo_mac[uni_port].acl_count--;
		}
	}

	aclinfo_flush_to_hw();
	return 0;
}

int
acl_del_ip(int uni_port, unsigned int dstip_start, unsigned int dstip_end, unsigned int srcip_start, unsigned int srcip_end, unsigned short tcpdport_start, unsigned short tcpdport_end, unsigned short tcpsport_start, unsigned short tcpsport_end, unsigned short udpdport_start, unsigned short udpdport_end, unsigned short udpsport_start, unsigned short udpsport_end)
{
	struct list_head *pos, *temp;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	// find target to delete
	list_for_each_safe(pos, temp, &aclinfo_ip[uni_port].acl_table) {
		struct acl_ip_node_t *acl_ip_node = list_entry(pos, struct acl_ip_node_t, head);
		if ( (acl_ip_node->dstip_start == dstip_start) && 
			(acl_ip_node->dstip_end == dstip_end) && 
			(acl_ip_node->srcip_start == srcip_start) && 
			(acl_ip_node->srcip_end == srcip_end) && 
			(acl_ip_node->tcpdport_start == tcpdport_start) && 
			(acl_ip_node->tcpdport_end == tcpdport_end) && 
			(acl_ip_node->tcpsport_start == tcpsport_start) && 
			(acl_ip_node->tcpsport_end == tcpsport_end) && 
			(acl_ip_node->udpdport_start == udpdport_start) &&
			(acl_ip_node->udpdport_end == udpdport_end) &&
			(acl_ip_node->udpsport_start == udpsport_start) &&
			(acl_ip_node->udpsport_end == udpsport_end)) {
			dbprintf(LOG_ERR, "remove match IP item\n");
			list_del(&acl_ip_node->head);
			free_safe(acl_ip_node);
			aclinfo_ip[uni_port].acl_count--;
		}
	}

	aclinfo_flush_to_hw();
	return 0;
}

int 
acl_del_vid(int uni_port, unsigned short vid_start, unsigned short vid_end, unsigned char pbit)
{
	struct list_head *pos, *temp;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	// find target to delete
	list_for_each_safe(pos, temp, &aclinfo_vid[uni_port].acl_table) {
		struct acl_vid_node_t *acl_vid_node = list_entry(pos, struct acl_vid_node_t, head);
		if ( (acl_vid_node->vid_start == vid_start) &&
			(acl_vid_node->vid_end == vid_end) &&
			(acl_vid_node->pbit == pbit)) {
			dbprintf(LOG_ERR, "remove match vlan id, pbit item\n");
			list_del(&acl_vid_node->head);
			free_safe(acl_vid_node);
			aclinfo_vid[uni_port].acl_count--;
		}
	}

	aclinfo_flush_to_hw();
	return 0;
}

int 
acl_del_proto(int uni_port, unsigned char proto, unsigned char tos)
{
	struct list_head *pos, *temp;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	// find target to delete
	list_for_each_safe(pos, temp, &aclinfo_proto[uni_port].acl_table) {
		struct acl_proto_node_t *acl_proto_node = list_entry(pos, struct acl_proto_node_t, head);
		if ( (acl_proto_node->proto == proto) &&
			(acl_proto_node->tos== tos)) {
			dbprintf(LOG_ERR, "remove match vlan id, tos item\n");
			list_del(&acl_proto_node->head);
			free_safe(acl_proto_node);
			aclinfo_proto[uni_port].acl_count--;
		}
	}

	aclinfo_flush_to_hw();
	return 0;
}

int 
acl_del_by_port(int uni_port)
{
	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}
	
	aclinfo_del_by_port(uni_port);

	aclinfo_flush_to_hw();
	return 0;
}

int 
acl_change_mode(int uni_port, unsigned char mode)
{
	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	if (aclinfo_mode[uni_port] != mode) {
		aclinfo_mode[uni_port]=mode;
		dbprintf(LOG_EMERG, "Method:%s\n",aclinfo_mode[uni_port]?"WHITE":"BLACK");
		aclinfo_flush_to_hw();
	}
	return 0;
}

//reset: clear all rule in aclinfo and hw
int 
acl_reset(void)
{
	int i;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	for(i=0; i < ACL_UNI_TOTAL; i++) {
		aclinfo_del_by_port(i);
		//default mode is black list
		aclinfo_mode[i]=ACL_MODE_BLACK;
	}

	// FIXME, KEVIN, to be heavily reimplemented
	// switch_hw_g.acl_conf_clearall(0);
	return 0;
}

unsigned int
acl_dump(int fd, int uni_port)
{
	struct list_head *pos, *temp;
	struct in_addr addr;
	int i;

	if (!is_aclinfo_init) {
		is_aclinfo_init=1;
		aclinfo_init();
	}

	util_fdprintf(fd, "Method:%s\n",aclinfo_mode[uni_port]?"WHITE":"BLACK");
	list_for_each_safe(pos, temp, &aclinfo_mac[uni_port].acl_table) {
		struct acl_mac_node_t *acl_mac_node = list_entry(pos, struct acl_mac_node_t, head);
		//uniport dstmac srcmac ethertype
		util_fdprintf(fd, "uniport\tdstmac\t\t\tsrcmac\t\tethertype\n");
		util_fdprintf(fd, "%d\t", (uni_port));

		for(i=0; i < 6; i++) {
			if (i==5 )
				util_fdprintf(fd, "%02X\t", (unsigned char)acl_mac_node->dstmac[i]);
			else
				util_fdprintf(fd, "%02X:", (unsigned char)acl_mac_node->dstmac[i]);
		}

		for(i=0; i < 6; i++) {
			if (i==5 )
				util_fdprintf(fd, "%02X ", (unsigned char)acl_mac_node->srcmac[i]);
			else
				util_fdprintf(fd, "%02X:", (unsigned char)acl_mac_node->srcmac[i]);
		}

		util_fdprintf(fd, "0x%04x\n", (acl_mac_node->ethertype));
	}

	list_for_each_safe(pos, temp, &aclinfo_ip[uni_port].acl_table) {
		struct acl_ip_node_t *acl_ip_node = list_entry(pos, struct acl_ip_node_t, head);
		//uniport dstip srcip tcpdport tcpsport udpdport udpsport
		util_fdprintf(fd, "uniport\tdstip_start\tdstip_end\tsrcip_start\tsrcip_end\ttcpdport\ttcpsport\tudpdport\tudpsport\n");
		util_fdprintf(fd, "%d\t", (uni_port));

		addr.s_addr=acl_ip_node->dstip_start;
		util_fdprintf(fd, "%s \t", inet_ntoa(addr));
		addr.s_addr=acl_ip_node->dstip_end;
		util_fdprintf(fd, "%s \t", inet_ntoa(addr));

		addr.s_addr=acl_ip_node->srcip_start;
		util_fdprintf(fd, "%s \t", inet_ntoa(addr));
		addr.s_addr=acl_ip_node->srcip_end;
		util_fdprintf(fd, "%s \t", inet_ntoa(addr));

		util_fdprintf(fd, "%d ~ %d \t\t", (acl_ip_node->tcpdport_start), (acl_ip_node->tcpdport_end));
		util_fdprintf(fd, "%d ~ %d \t\t", (acl_ip_node->tcpsport_start), (acl_ip_node->tcpsport_end));
		util_fdprintf(fd, "%d ~ %d \t\t", (acl_ip_node->udpdport_start), (acl_ip_node->udpdport_end));
		util_fdprintf(fd, "%d ~ %d\n", (acl_ip_node->udpsport_start), (acl_ip_node->udpsport_end));
	}

	list_for_each_safe(pos, temp, &aclinfo_vid[uni_port].acl_table) {
		struct acl_vid_node_t *acl_vid_node = list_entry(pos, struct acl_vid_node_t, head);
		//uniport vlanid
		util_fdprintf(fd, "uniport\tvid\t\tpbit\n");
		util_fdprintf(fd, "%d\t", (uni_port));
		util_fdprintf(fd, "%d ~ %d \t", (acl_vid_node->vid_start), (acl_vid_node->vid_end));
		util_fdprintf(fd, "%d\n", (acl_vid_node->pbit));
	}

	list_for_each_safe(pos, temp, &aclinfo_proto[uni_port].acl_table) {
		struct acl_proto_node_t *acl_proto_node = list_entry(pos, struct acl_proto_node_t, head);
		//uniport vlanid
		util_fdprintf(fd, "uniport\tproto\ttos\n");
		util_fdprintf(fd, "%d\t", (uni_port));
		util_fdprintf(fd, "%d\t", (acl_proto_node->proto));
		util_fdprintf(fd, "%d\n", (acl_proto_node->tos));
	}

	return 0;
}

