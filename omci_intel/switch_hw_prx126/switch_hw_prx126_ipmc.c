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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_ipmc.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include

#include "util.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "env.h"

#include "intel_px126_mcc_util.h"

int 
switch_hw_prx126_ipv4mcastaddr_add(struct switch_ipv4mcast_entry_t *mac_tab_entry)
{
	 struct switch_mac_tab_entry_t entry;
        
        if (mac_tab_entry->filter_mode == SWITCH_IPMC_MODE_SSM_INCLUDE)
        {                
                dbprintf(LOG_WARNING, "(x, G)\n");
                int i;
                for (i = 0; i < 8; i++)
                {
                        if (mac_tab_entry->src_list[i] == 0) break;
                        memset(&entry, 0, sizeof(struct switch_mac_tab_entry_t));
                        entry.vid = 0;
                        entry.ipmcast.dst_ip.ipv4.s_addr = mac_tab_entry->dst_ip.ipv4.s_addr;
                        entry.ipmcast.src_ip.ipv4.s_addr = mac_tab_entry->src_list[i];
                        entry.port_bitmap = mac_tab_entry->src_portmask[i];
                        switch_hw_prx126_ipmcastaddr_add(&entry);
                        dbprintf(LOG_WARNING, "[%d] includeOrExcludeIpList[%x].portmask 0x%x\n", i, mac_tab_entry->src_list[i], mac_tab_entry->src_portmask[i]);
                }              
        }
        else if (mac_tab_entry->filter_mode == SWITCH_IPMC_MODE_ASM)
        {
                dbprintf(LOG_WARNING, "(*, G)\n");
                memset(&entry, 0, sizeof(struct switch_mac_tab_entry_t));
                entry.vid = (omci_env_g.ivlan_enable) ? mac_tab_entry->vid : 0;
                entry.ipmcast.dst_ip.ipv4.s_addr = mac_tab_entry->dst_ip.ipv4.s_addr;
                entry.port_bitmap = mac_tab_entry->src_portmask[0];
                switch_hw_prx126_ipmcastaddr_add(&entry);
                dbprintf(LOG_WARNING, "dontCareSipModePortmask 0x%x\n", entry.port_bitmap);     
        }
        
        return 0;
        
	return 0;
}

int 
switch_hw_prx126_l2_impc_group_del(unsigned int gip)
{
	
	int ret = 0;
	unsigned char fid,p_idx=0;
	union px126_mcc_ip_addr mcc_ip;	
	
	fid = 0;	
	memset(&mcc_ip,0x0,sizeof(union px126_mcc_ip_addr));
	memcpy(&mcc_ip.ipv4,(unsigned char*)&gip,4);
	ret = intel_mcc_dev_port_remove(p_idx,fid,&mcc_ip);
	if(ret < 0){
		dbprintf(LOG_ERR, "mcc  gip remove error (0x%x)\n",gip);
		return -1;
	}
	
	return 0;
}

int
switch_hw_prx126_ipmcastaddr_add(struct switch_mac_tab_entry_t *mac_tab_entry)
{	
	int ret = 0;
	unsigned char fid,p_idx=0;
	union px126_mcc_ip_addr mcc_ip;

	if(IS_IPV6_ADDR(mac_tab_entry->ipmcast.dst_ip)) {
		ret = intel_mcc_dev_fid_get(mac_tab_entry->vid,&fid);
		if(ret < 0){
			dbprintf(LOG_WARNING, "mcc get fid error(vid=%d)\n",mac_tab_entry->vid);
			fid = 0;
		}
		memset(&mcc_ip,0x0,sizeof(union px126_mcc_ip_addr));
		memcpy(&mcc_ip.ipv6,(unsigned char*)&mac_tab_entry->ipmcast.dst_ip.ipv6.s6_addr,16);
		ret = intel_mcc_dev_port_add(px126_MCC_DIR_US,p_idx,fid,&mcc_ip);
		if(ret < 0){
			dbprintf(LOG_ERR, "mcc upstream gip ipv6 add error \n");
			return -1;
		}
		
		ret = intel_mcc_dev_port_add(px126_MCC_DIR_DS,p_idx,fid,&mcc_ip);
		if(ret < 0){
			dbprintf(LOG_ERR, "mcc Downstream gip ipv6 add error \n");
			return -1;
		}
	} else {
		if(mac_tab_entry->ipmcast.src_ip.ipv4.s_addr == 0){
			ret = intel_mcc_dev_fid_get(mac_tab_entry->vid,&fid);
			if(ret < 0){
				dbprintf(LOG_WARNING, "mcc get fid error(vid=%d)\n",mac_tab_entry->vid);
				fid = 0;
			}
			memset(&mcc_ip,0x0,sizeof(union px126_mcc_ip_addr));
			memcpy(&mcc_ip.ipv4,(unsigned char*)&mac_tab_entry->ipmcast.dst_ip.ipv4.s_addr,4);
			ret = intel_mcc_dev_port_add(px126_MCC_DIR_US,p_idx,fid,&mcc_ip);
			if(ret < 0){
				dbprintf(LOG_ERR, "mcc upstream gip add error (0x%x)\n",mac_tab_entry->ipmcast.dst_ip.ipv4.s_addr);
				return -1;
			}
			
			ret = intel_mcc_dev_port_add(px126_MCC_DIR_DS,p_idx,fid,&mcc_ip);
			if(ret < 0){
				dbprintf(LOG_ERR, "mcc Downstream gip add error (0x%x)\n",mac_tab_entry->ipmcast.dst_ip.ipv4.s_addr);
				return -1;
			}

		}else{
			dbprintf(LOG_WARNING, "don't support (x,g) for intel\n");   	
		}

		
	}
	
	return  ret;
}

int
switch_hw_prx126_ipmcastaddr_del(struct switch_mac_tab_entry_t *mac_tab_entry)
{
	int ret = 0;
	unsigned char fid,p_idx=0;
	union px126_mcc_ip_addr mcc_ip;
	
	if(IS_IPV6_ADDR(mac_tab_entry->ipmcast.dst_ip)) {
		ret = intel_mcc_dev_fid_get(mac_tab_entry->vid,&fid);
		if(ret < 0){
			dbprintf(LOG_WARNING, "mcc get fid error(vid=%d)\n",mac_tab_entry->vid);
			fid = 0;
		}
		memset(&mcc_ip,0x0,sizeof(union px126_mcc_ip_addr));
		memcpy(&mcc_ip.ipv6,(unsigned char*)&mac_tab_entry->ipmcast.dst_ip.ipv6.s6_addr,16);
		ret = intel_mcc_dev_port_remove(p_idx,fid,&mcc_ip);
		if(ret < 0){
			dbprintf(LOG_ERR, "mcc  gip ipv6 remove error \n");
			return -1;
		}
		
	} else {
		if(mac_tab_entry->ipmcast.src_ip.ipv4.s_addr == 0){
			ret = intel_mcc_dev_fid_get(mac_tab_entry->vid,&fid);
			if(ret < 0){
				dbprintf(LOG_WARNING, "mcc get fid error(vid=%d)\n",mac_tab_entry->vid);
				fid = 0;
			}
			memset(&mcc_ip,0x0,sizeof(union px126_mcc_ip_addr));
			memcpy(&mcc_ip.ipv4,(unsigned char*)&mac_tab_entry->ipmcast.dst_ip.ipv4.s_addr,4);
			ret = intel_mcc_dev_port_remove(p_idx,fid,&mcc_ip);
			if(ret < 0){
				dbprintf(LOG_ERR, "mcc  gip remove error (0x%x)\n",mac_tab_entry->ipmcast.dst_ip.ipv4.s_addr);
				return -1;
			}
	
		}else{
			dbprintf(LOG_WARNING, "don't support (x,g) for intel\n");	
		}
		
	}
	
	return  ret;
}


int
switch_hw_prx126_ipmcastaddr_get(struct switch_mac_tab_entry_t *mac_tab_entry)
{
	return 0;
}

int
switch_hw_prx126_ipmcastaddr_get_next(struct switch_mac_tab_entry_t *mac_tab_entry, int *num)
{
	return 0;
}

int
switch_hw_prx126_ipmcastaddr_flush(void)
{
	return 0;
}

int
switch_hw_prx126_ipmcast_mode_clear(void)
{
	return 0;
}

int
switch_hw_prx126_ipmcast_mode_set(void)
{
	return 0;
}
int switch_hw_prx126_ipmcast_me309_mode_set(unsigned short meid,unsigned short igmp_ver,int is_create)
{
	int ret;
	if(is_create)
		ret = intel_mcc_me309_create(meid,igmp_ver);
	else
		ret = intel_mcc_me309_del(meid);
	return ret;
}
int switch_hw_prx126_ipmcast_me310_extvlan_set(struct switch_mc_profile_ext_vlan_update_data *up_data)
{
	return intel_mcc_me310_extvlan_set(up_data);		
}