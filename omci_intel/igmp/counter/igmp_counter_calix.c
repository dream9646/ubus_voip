/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp counter module - calix
 * File    : igmp_counter_calix.c
 *
 ******************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
                     
#include "meinfo.h"
#include "util.h"
#include "hwresource.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "igmp_counter_calix.h"
#include "mcast_timer.h"
#include "igmp_mbship.h"
#include "util_inet.h"
#include "metacfg_adapter.h"

// count is to let the first retrive after a reset to hide currently
// joined membership
static int count=0;

#define IGMP_COUNTER_MODULE_NAME        "calix"

static char masterssid[128];
static int portreverse = 0;  // 823,814, 822, 819

extern struct igmp_mbship_db_t igmp_mbship_db_g;

static LIST_HEAD(igmp_counter_g);

static struct igmp_pkt_calix_counter_t *
igmp_counter_locate_by_port(unsigned short me47_id)
{
	struct igmp_pkt_calix_counter_t *pos;
	list_for_each_entry(pos, &igmp_counter_g, node) {
		if (pos->me47_id == me47_id)
			return pos;
	}
	return NULL;
}

static struct igmp_pkt_calix_counter_t *
igmp_counter_alloc_by_port(unsigned short me47_id)
{
        struct igmp_pkt_calix_counter_t *c = malloc_safe(sizeof(struct igmp_pkt_calix_counter_t));
        memset(c, 0, sizeof(struct igmp_pkt_calix_counter_t));
        c->me47_id = me47_id;
	list_add_tail(&(c->node), &igmp_counter_g);
        
	return c;
}

struct igmp_pkt_state_t {
	int state;
	int (*state_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
};

static int out_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{

        // lan to wan, a packet is sent only to WAN port
        // in wan to lan, a packet is sent to available all LAN port
        struct meinfo_t *me47_miptr = NULL;
        struct me_t *me47_tx = NULL, *me422 = NULL;
        struct igmp_group_t *g = NULL;
        short dir = 0;
        
        me47_miptr = meinfo_util_miptr(47);
        if (me47_miptr == NULL) return 0;
        
        me422 = hwresource_public2private(igmp_clientinfo->rx_desc.bridge_port_me);
        if (meinfo_util_me_attr_data(me422 , 4) <= 1)
                dir = 1;
        else if (2 != meinfo_util_me_attr_data(me422 , 4) && 4 != meinfo_util_me_attr_data(me422 , 4))
                return 0;       // RX neither LAN nor WAN
        
        list_for_each_entry(me47_tx, &me47_miptr->me_instance_list, instance_node) {
                me422 = hwresource_public2private(me47_tx);

                if (dir)
                {
                        if (meinfo_util_me_attr_data(me422, 4) != 2) //to wan
                                continue;
                        if (meinfo_util_me_attr_data(me422, 3) == 6) //skip multicast IWTP
                                continue;
                }
                else
                {
                        if (meinfo_util_me_attr_data(me422, 4) != 1 && 0 != meinfo_util_me_attr_data(me422, 4)) // to uni or veip(wifi)
                                continue;
                }
                
                struct igmp_pkt_calix_counter_t *c = igmp_counter_locate_by_port(me47_tx->meid);
                if (c == NULL) c = igmp_counter_alloc_by_port(me47_tx->meid);
                
                switch (igmp_clientinfo->igmp_msg_type) 
                {
                case IGMP_V1_MEMBERSHIP_REPORT:
                        c->txRpt1++;
			break;
		case IGMP_V2_MEMBERSHIP_REPORT:
                        c->txRpt2++;
			break;	
		case IGMP_V3_MEMBERSHIP_REPORT:
                        c->txRpt3++;
			break;
		case IGMP_V2_LEAVE_GROUP:
                        c->txLeave++;
			break;
		case IGMP_MEMBERSHIP_QUERY:                      
                        list_for_each_entry(g, &igmp_clientinfo->group_list, entry_node) {}
                        
                        if (igmp_clientinfo->igmp_version & IGMP_V3_BIT)
                        {
                                if (g->group_ip.ipv4.s_addr || g->src_ip.ipv4.s_addr)
                                        c->txGrpQry3++;
                                else
                                        c->txGenQry3++;               
                        }
                        else
                        {
                                if (igmp_clientinfo->igmp_version & IGMP_V2_BIT) // igmpv2 query
                                {
                                        if (g->group_ip.ipv4.s_addr)    // check if it's group query
                                                c->txGrpQry2++;
                                        else
                                                c->txGenQry2++;
                                }
                                else
                                        c->txGenQry1++;
                        }    
			break;	
		default:
			dbprintf_igmp(LOG_WARNING,"unhandled igmp msg (%d)\n", igmp_clientinfo->igmp_msg_type);
			return -1;
                }          
        }
                
        return 0;
}       

static int is_valid_uni_or_wan(struct me_t *me47)
{
        // verify RX is either WAN or LAN port
        struct me_t *me422 = hwresource_public2private(me47);
        
        if (me422 == NULL) return -1;
        
        if (meinfo_util_me_attr_data(me422 , 4) > 2) return -1;        // neither LAN nor WAN
        
        if (meinfo_util_me_attr_data(me422 , 4) == 2 && meinfo_util_me_attr_data(me422, 3) == 6) return -1;
        
        return 0;
}

static int in_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *g = NULL;
         
        if (is_valid_uni_or_wan(igmp_clientinfo->pktinfo->rx_desc.bridge_port_me)) return 0;
        
        struct igmp_pkt_calix_counter_t *c = igmp_counter_locate_by_port(igmp_clientinfo->pktinfo->rx_desc.bridge_port_me->meid);
	if (c == NULL) c = igmp_counter_alloc_by_port(igmp_clientinfo->pktinfo->rx_desc.bridge_port_me->meid);
        
        switch (igmp_clientinfo->igmp_msg_type) 
	{
                case IGMP_V1_MEMBERSHIP_REPORT:
                        c->rxRpt1++;
			break;
		case IGMP_MEMBERSHIP_QUERY:
                        // determine if it's IGMPv3 query, IGMPv3 query packet length is longer                        
                        list_for_each_entry(g, &igmp_clientinfo->group_list, entry_node) {}
                        
                        if (igmp_clientinfo->igmp_version & IGMP_V3_BIT)
                        {
                                if (g->group_ip.ipv4.s_addr || g->src_ip.ipv4.s_addr)
                                        c->rxGrpQry3++;
                                else
                                        c->rxGenQry3++;               
                        }
                        else
                        {
                                if (igmp_clientinfo->igmp_version & IGMP_V2_BIT) // igmpv2 query
                                {
                                        if (g->group_ip.ipv4.s_addr)    // check if it's group query
                                                c->rxGrpQry2++;
                                        else
                                                c->rxGenQry2++;
                                }
                                else
                                        c->rxGenQry1++;
                        }    
			break;
		case IGMP_V2_MEMBERSHIP_REPORT:
                        c->rxRpt2++;
			break;	
		case IGMP_V3_MEMBERSHIP_REPORT:
                        c->rxRpt3++;
			break;
		case IGMP_V2_LEAVE_GROUP:
                        c->rxLeave++;
			break;	
		default:
			dbprintf_igmp(LOG_WARNING,"unhandled igmp msg (%d)\n", igmp_clientinfo->igmp_msg_type);
			return -1;
	}  
                
        return 0;
}

static struct igmp_pkt_state_t state_handler_table[]={
	{IGMP_PKT_PARSE_CLIENTINFO, in_handler},
	{IGMP_PKT_BRIDGE, out_handler}
};
       

// module functions
int
igmp_counter_calix_reset(void)
{
	struct igmp_pkt_calix_counter_t *pos, *n;
        
	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
		list_del(&pos->node);
		free_safe(pos);
	}
        
        count = 1;	// increment count
        
	return 0;
}

int
igmp_counter_calix_init(void)
{
	return 0;
}

const char *igmp_counter_calix_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

int igmp_counter_calix_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
	int i;
	
        int nHandlers = sizeof(state_handler_table)/sizeof(struct igmp_pkt_state_t);
        
        for (i=0; i < nHandlers; i++) {
                if (state_handler_table[i].state != igmp_clientinfo->state.exec_state) continue;
                // matching state, execute handler
                state_handler_table[i].state_handler(igmp_clientinfo);      
	}
        
	return 0;
}

#define PRINT_CLI_ENTRY(h, s, rx, tx) util_fdprintf(h, "%-25s%16d%16d\n", s, rx, tx);

static void generate_lan_ether(FILE *fp)
{
	int i = 0;
	char tmp[256];
	struct igmp_pkt_calix_counter_t tmpobj;
	
	memset(&tmpobj, 0, sizeof(struct igmp_pkt_calix_counter_t));
	for (i = 0; i < 4; i++)
	{
		struct igmp_pkt_calix_counter_t *pos, *n, *counter=NULL;
        	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
                	struct me_t *me47 = meinfo_me_get_by_meid(47, pos->me47_id);
               	 	char *devname = hwresource_devname(me47);
                	if (devname == NULL) devname="unknown";
                	struct me_t *me422 = hwresource_public2private(me47);
				
                	 if (meinfo_util_me_attr_data(me422, 6) == i)
                	 {
                	 	counter = pos;
                	 	break;
			 }
			 	
        	}
        	if (counter == NULL) counter = &tmpobj;
		int ge = (portreverse)?(5-(i + 1)):(i + 1);  
        	snprintf(tmp, 256, "{\"PortName\":\"GE%d\"", ge);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V1RepIn\":%d", counter->rxRpt1);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V2RepIn\":%d", counter->rxRpt2);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V3RepIn\":%d", counter->rxRpt3);
        	fputs(tmp, fp);
        	snprintf(tmp, 256,",\"V2LeaveIn\":%d}", counter->rxLeave);
        	fputs(tmp, fp);
		if (i < 3) fputs(",", fp); 
	}
}

static void generate_lan_ether_clients(FILE *fp)
{
	struct mcast_mbship_t *pos=NULL, *n=NULL;
        
	char str[1024];
	char tmp[256];
	memset(str,0,1024);
	char group_ip[INET6_ADDRSTRLEN];;
	char client_ip[INET6_ADDRSTRLEN];
	int i = 0;
        long current = igmp_timer_get_cur_time();
        
	if (!list_empty(&igmp_mbship_db_g.mbship_list))
	{
		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if (pos->logical_port_id > 3) continue;
			memset(group_ip,0,INET6_ADDRSTRLEN);
			memset(client_ip,0,INET6_ADDRSTRLEN); 
			
			util_inet_ntop(AF_INET, &pos->group_ip, (char *)group_ip, sizeof(group_ip));
			util_inet_ntop(AF_INET, &pos->client_ip, (char *)client_ip, sizeof(client_ip));

			if (i != 0) fputs(",", fp);
			int ge = (portreverse)?(5-(pos->logical_port_id + 1)):(pos->logical_port_id + 1);    
        		snprintf(tmp, 256, "{\"LanPort\":\"GE%d\"", ge);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"GroupAddr\":\"%s\"", group_ip);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"RepAddr\":\"%s\"", client_ip);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"Timeout\":%ld}", (current - pos->access_time)/1000);
        		fputs(tmp, fp);
        		i++;
		}
	}
}

static void generate_lan_wifi(FILE *fp)
{
	int i = 0;
	char tmp[256];
	struct igmp_pkt_calix_counter_t tmpobj;
	int j = 0;
	
	memset(&tmpobj, 0, sizeof(struct igmp_pkt_calix_counter_t));
	for (i = 8; i < 9; i++)
	{
		struct igmp_pkt_calix_counter_t *pos, *n, *counter=NULL;
        	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
                	struct me_t *me47 = meinfo_me_get_by_meid(47, pos->me47_id);
               	 	char *devname = hwresource_devname(me47);
                	if (devname == NULL) devname="unknown";
                	
                	struct me_t *me422 = hwresource_public2private(me47);
			if (meinfo_util_me_attr_data(me422, 6) == i)
                	{
                	 	counter = pos;
                	 	break;
			}
			 	
        	}
        	if (counter == NULL) counter = &tmpobj;
		
		if (j != 0) fputs(",", fp);
        	snprintf(tmp, 256, "{\"SSID\":\"%s\"", masterssid);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V1RepIn\":%d", counter->rxRpt1);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V2RepIn\":%d", counter->rxRpt2);
        	fputs(tmp, fp);
        	snprintf(tmp, 256, ",\"V3RepIn\":%d", counter->rxRpt3);
        	fputs(tmp, fp);
        	snprintf(tmp, 256,",\"V2LeaveIn\":%d}", counter->rxLeave);
        	fputs(tmp, fp);
        	j++;
	}
}

static void generate_lan_wifi_clients(FILE *fp)
{
	struct mcast_mbship_t *pos=NULL, *n=NULL;
        
	char str[1024];
	char tmp[256];
	memset(str,0,1024);
	char group_ip[INET6_ADDRSTRLEN];;
	char client_ip[INET6_ADDRSTRLEN];
	int i = 0;
        long current = igmp_timer_get_cur_time();
        
	if (!list_empty(&igmp_mbship_db_g.mbship_list))
	{
		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if (pos->logical_port_id != 8) continue; 
			memset(group_ip,0,INET6_ADDRSTRLEN);
			memset(client_ip,0,INET6_ADDRSTRLEN);

			util_inet_ntop(AF_INET, &pos->group_ip, (char *)group_ip, sizeof(group_ip));
			util_inet_ntop(AF_INET, &pos->client_ip, (char *)client_ip, sizeof(client_ip));

			if (i != 0) fputs(",", fp);  
        		snprintf(tmp, 256, "{\"SSID\":\"%s\"", masterssid);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"GroupAddr\":\"%s\"", group_ip);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"RepAddr\":\"%s\"", client_ip);
        		fputs(tmp, fp);
        		snprintf(tmp, 256, ",\"Timeout\":%ld}", (current - pos->access_time)/1000);
        		fputs(tmp, fp);
        		i++;
		}
	}
}

static void generate_wan(FILE *fp)
{
        char tmp[256];
	struct igmp_pkt_calix_counter_t tmpobj, *counter=NULL;
        struct me_t *txport_me=NULL;
        struct meinfo_t *port_miptr=NULL;
        
        
        port_miptr = meinfo_util_miptr(47);
        
	memset(&tmpobj, 0, sizeof(struct igmp_pkt_calix_counter_t));
        list_for_each_entry(txport_me, &port_miptr->me_instance_list, instance_node) {
		
		struct me_t *itxport_me = hwresource_public2private(txport_me);
		
		if (meinfo_util_me_attr_data(itxport_me, 4) != 2) //to wan
			continue;
		if (meinfo_util_me_attr_data(txport_me, 3) == 6) //skip multicast IWTP
			continue;
			
		struct igmp_pkt_calix_counter_t *pos, *n;
		list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
			struct me_t *me47 = meinfo_me_get_by_meid(47, pos->me47_id);
			if (me47 == txport_me)
			{
				counter = pos;
				break;
			}
		}
		if (counter != NULL) break;
        }
        if (counter == NULL) counter = &tmpobj;
        
        snprintf(tmp, 256, "\"GnQueryInV1\":%d", counter->rxGenQry1);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GnQueryInV2\":%d", counter->rxGenQry2);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GpQueryInV2\":%d", counter->rxGrpQry2);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GnQueryInV3\":%d", counter->rxGenQry3);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GpQueryInV3\":%d", counter->rxGrpQry3);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"V3ReportOut\":%d", counter->txRpt1);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"V2ReportOut\":%d", counter->txRpt2);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"V1ReportOut\":%d", counter->txRpt3);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"V2LeaveOut\":%d", counter->txLeave);
        fputs(tmp, fp);
}

static void generate_lan_tx(FILE *fp)
{
	int i = 0;
	char tmp[256];
	struct igmp_pkt_calix_counter_t tmpobj, *counter=NULL;
	
	memset(&tmpobj, 0, sizeof(struct igmp_pkt_calix_counter_t));
	for (i = 0; i < 4; i++)
	{
		struct igmp_pkt_calix_counter_t *pos, *n;
        	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
                	struct me_t *me47 = meinfo_me_get_by_meid(47, pos->me47_id);
               	 	char *devname = hwresource_devname(me47);
                	if (devname == NULL) devname="unknown";
                	
                	struct me_t *me422 = hwresource_public2private(me47);
			if (meinfo_util_me_attr_data(me422, 6) == i)
                	{
                	 	counter = pos;
                	 	break;
			}
			 	
        	}
		if (counter != NULL) break;		 
	}
        if (counter == NULL) counter = &tmpobj;
		
        snprintf(tmp, 256, "\"GnQueryV1\":%d", counter->txGenQry1);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GnQueryV2\":%d", counter->txGenQry2);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GnQueryV3\":%d", counter->txGenQry3);
        fputs(tmp, fp);
        snprintf(tmp, 256, ",\"GpQueryV2\":%d", counter->txGrpQry2);
        fputs(tmp, fp);
        snprintf(tmp, 256,",\"GpQueryV3\":%d", counter->txGrpQry3);
        fputs(tmp, fp);
}

static void generate_calix_ui_dump()
{
	FILE *slog = NULL;
	struct stat fstat={0};
	int skip_membership = 0;
	
	if (count == 1)
	{
		skip_membership = 1;
		count = 0;
	}
	
        stat("/tmp/calixcounter.log", &fstat);
	slog = fopen("/tmp/calixcounter.log", "w+");
	if (slog)
	{
		fputs("{", slog);
		fputs("\"lan\":{", slog);
			fputs("\"ethernet\":[", slog);
			generate_lan_ether(slog);
			fputs("],", slog);
			fputs("\"wifi\":[", slog);
			generate_lan_wifi(slog);
			fputs("],", slog);
			fputs("\"EthEntries\":[", slog);
			if (!skip_membership) generate_lan_ether_clients(slog);
			fputs("],", slog);
			fputs("\"WifiEntries\":[", slog);
			if (!skip_membership) generate_lan_wifi_clients(slog);
			fputs("],", slog);
			generate_lan_tx(slog);
		fputs("},", slog);
		fputs("\"wan\":{", slog);
	        generate_wan(slog);
		fputs("}", slog);
		fputs("}", slog);
	}
	fclose(slog);	
}

int igmp_cli_counter_calix(int fd)
{
	struct igmp_pkt_calix_counter_t *pos, *n;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "system_modelname", "wlan_autorate");

	char modelname[256];
	memset(modelname, 0, sizeof(modelname));
	strcpy(modelname, metacfg_adapter_config_get_value(&kv_config, "system_modelname", 1));
	if (strstr(modelname, "823") ||
	    //strstr(modelname, "822") ||
	    //strstr(modelname, "819") ||
	    strstr(modelname, "814")
	    )
		portreverse = 1;
			
	memset(masterssid, 0, sizeof(masterssid));
	strcpy(masterssid, metacfg_adapter_config_get_value(&kv_config, "wlan_ssid", 1));
			
	metacfg_adapter_config_release(&kv_config);
#endif
        list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
                struct me_t *me47 = meinfo_me_get_by_meid(47, pos->me47_id);
                char *devname = hwresource_devname(me47);
                if (devname == NULL) devname="unknown";
                
                util_fdprintf(fd, "%-25s%16s%16s\n", devname, "RX", "TX");
                PRINT_CLI_ENTRY(fd, "General Query v1:", pos->rxGenQry1, pos->txGenQry1)
                PRINT_CLI_ENTRY(fd, "General Query v2:", pos->rxGenQry2, pos->txGenQry2)
                PRINT_CLI_ENTRY(fd, "General Query v3:", pos->rxGenQry3, pos->rxGenQry3)
                PRINT_CLI_ENTRY(fd, "Group Query v2:", pos->rxGrpQry2, pos->txGrpQry2)
                PRINT_CLI_ENTRY(fd, "Group/Source Query v3:", pos->rxGrpQry3, pos->txGrpQry3)
                PRINT_CLI_ENTRY(fd, "Report v1:", pos->rxRpt1, pos->txRpt1)
                PRINT_CLI_ENTRY(fd, "Report v2:", pos->rxRpt2, pos->txRpt2)
                PRINT_CLI_ENTRY(fd, "Report v3:", pos->rxRpt3, pos->txRpt3)
                PRINT_CLI_ENTRY(fd, "Leave v2:", pos->rxLeave, pos->txLeave)
                util_fdprintf(fd, "\n");
        }
        
        generate_calix_ui_dump();
        
	return 0;
}

int igmp_counter_iterator_calix(int (*iterator_function)(struct igmp_pkt_calix_counter_t *pos, void *data), void *iterator_data)
{
        struct igmp_pkt_calix_counter_t *pos_counter=NULL;
        
        list_for_each_entry(pos_counter,  &igmp_counter_g, node) {
                (*iterator_function)(pos_counter, iterator_data);               
        }
        
        return 0;
}


struct igmp_counter_module_t calix =
{
        .module_name = igmp_counter_calix_name,
        .init = igmp_counter_calix_init,
        .reset = igmp_counter_calix_reset,
        .igmp_counter_handler = igmp_counter_calix_handler,
        .igmp_cli_handler = igmp_cli_counter_calix,
        .igmp_counter_iterator = igmp_counter_iterator_calix
};


