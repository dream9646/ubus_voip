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
 * Module  : igmp counter module - calix proxy
 * File    : igmp_counter_calix_proxy.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "igmp_counter_calix_proxy.h"
#include "mcast_timer.h"
#include "igmp_mbship.h"
#include "util_inet.h"
#include "metacfg_adapter.h"

#define IGMP_COUNTER_MODULE_NAME        "calix_proxy"

// count is to let the first retrive after a reset to hide currently
// joined membership
static int count=0;

static char masterssid[128];
static int portreverse = 0;  // 823,814, 822, 819

static void generate_lan_ether(FILE *log)
{
	FILE *fp = popen("cat /proc/igmpproxy_counter", "r");
        char tmp[256];
        
	if(fp) {
		char buf[1024];
		struct igmp_pkt_calix_proxy_counter_t counter, *c;
		c = &counter;
		
		int i = 0;
		memset(buf, 0, sizeof(buf));
		memset(&counter, 0, sizeof(struct igmp_pkt_calix_proxy_counter_t));
		while (fgets(buf, sizeof(buf), fp)) {
			//util_fdprintf(fd,"%s", buf);
			sscanf(buf, "%16[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				c->vif,
				&c->rxGenQry1,
        			&c->rxGenQry2,
        			&c->rxGenQry3,
        			&c->rxGrpQry2,
        			&c->rxGrpQry3,
        			&c->txGenQry1,
        			&c->txGenQry2,
        			&c->txGenQry3,
        			&c->txGrpQry2,
        			&c->txGrpQry3,
        			&c->rxRpt1,
        			&c->rxRpt2,
        			&c->rxRpt3,
        			&c->rxLeave,
        			&c->txRpt1,
        			&c->txRpt2,
        			&c->txRpt3,
        			&c->txLeave);
        			
        		if (strlen(c->vif) < 3) continue;
        		if (strncmp(c->vif, "eth", 3) == 0)
        		{
				if (strcmp(c->vif, "eth0.2") == 0)
					snprintf(tmp, 256, (portreverse)?"{\"PortName\":\"GE4\"":"{\"PortName\":\"GE1\"");
				else if (strcmp(c->vif, "eth0.3") == 0)
					snprintf(tmp, 256, (portreverse)?"{\"PortName\":\"GE3\"":"{\"PortName\":\"GE2\"");
				else if (strcmp(c->vif, "eth0.4") == 0)
					snprintf(tmp, 256, (portreverse)?"{\"PortName\":\"GE2\"":"{\"PortName\":\"GE3\"");
				else if (strcmp(c->vif, "eth0.5") == 0)
					snprintf(tmp, 256, (portreverse)?"{\"PortName\":\"GE1\"":"{\"PortName\":\"GE4\"");
				else
					continue;

                                fputs(tmp, log);
                                snprintf(tmp, 256, ",\"V1RepIn\":%d", c->rxRpt1);
        			fputs(tmp, log);
        			snprintf(tmp, 256, ",\"V2RepIn\":%d", c->rxRpt2);
        			fputs(tmp, log);
        			snprintf(tmp, 256, ",\"V3RepIn\":%d", c->rxRpt3);
        			fputs(tmp, log);
        			snprintf(tmp, 256,",\"V2LeaveIn\":%d}", c->rxLeave);
        			fputs(tmp, log);
	
				if (i < 3) fputs(",", log); 
				i++;	
			}
			
		}
		pclose(fp);
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

static void generate_lan_wifi(FILE *log)
{
	FILE *fp = popen("cat /proc/igmpproxy_counter", "r");
        char tmp[256];
        int j = 0;
        
	if(fp) {
		char buf[1024];
		struct igmp_pkt_calix_proxy_counter_t counter, *c;
		c = &counter;
		
		memset(buf, 0, sizeof(buf));
		memset(&counter, 0, sizeof(struct igmp_pkt_calix_proxy_counter_t));
		while (fgets(buf, sizeof(buf), fp)) {
			//util_fdprintf(fd,"%s", buf);
			sscanf(buf, "%16[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				c->vif,
				&c->rxGenQry1,
        			&c->rxGenQry2,
        			&c->rxGenQry3,
        			&c->rxGrpQry2,
        			&c->rxGrpQry3,
        			&c->txGenQry1,
        			&c->txGenQry2,
        			&c->txGenQry3,
        			&c->txGrpQry2,
        			&c->txGrpQry3,
        			&c->rxRpt1,
        			&c->rxRpt2,
        			&c->rxRpt3,
        			&c->rxLeave,
        			&c->txRpt1,
        			&c->txRpt2,
        			&c->txRpt3,
        			&c->txLeave);
        		
        		if (strlen(c->vif) < 4) continue;
			if (strncmp(c->vif, "wlan", 4) != 0) continue;
		        
		        if (j != 0) fputs(",", log);
		        	
	        	snprintf(tmp, 256, "{\"SSID\":\"%s\"", masterssid);
	        	fputs(tmp, log);
	        	snprintf(tmp, 256, ",\"V1RepIn\":%d", c->rxRpt1);
	        	fputs(tmp, log);
	        	snprintf(tmp, 256, ",\"V2RepIn\":%d", c->rxRpt2);
	        	fputs(tmp, log);
	        	snprintf(tmp, 256, ",\"V3RepIn\":%d", c->rxRpt3);
	        	fputs(tmp, log);
	        	snprintf(tmp, 256,",\"V2LeaveIn\":%d}", c->rxLeave);
	        	fputs(tmp, log);
			j++;			
		}
		pclose(fp);
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

static void generate_lan_tx(FILE *log)
{
	FILE *fp = popen("cat /proc/igmpproxy_counter", "r");
        char tmp[256];
        
	if(fp) {
		char buf[1024];
		struct igmp_pkt_calix_proxy_counter_t counter, *c;
		c = &counter;
		
		memset(buf, 0, sizeof(buf));
		memset(&counter, 0, sizeof(struct igmp_pkt_calix_proxy_counter_t));
		while (fgets(buf, sizeof(buf), fp)) {
			//util_fdprintf(fd,"%s", buf);
			sscanf(buf, "%16[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				c->vif,
				&c->rxGenQry1,
        			&c->rxGenQry2,
        			&c->rxGenQry3,
        			&c->rxGrpQry2,
        			&c->rxGrpQry3,
        			&c->txGenQry1,
        			&c->txGenQry2,
        			&c->txGenQry3,
        			&c->txGrpQry2,
        			&c->txGrpQry3,
        			&c->rxRpt1,
        			&c->rxRpt2,
        			&c->rxRpt3,
        			&c->rxLeave,
        			&c->txRpt1,
        			&c->txRpt2,
        			&c->txRpt3,
        			&c->txLeave);
        		
        		if (strcmp(c->vif, omci_env_g.router_lan_if) != 0) continue;
		        
		        snprintf(tmp, 256, "\"GnQueryV1\":%d", c->txGenQry1);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GnQueryV2\":%d", c->txGenQry2);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GnQueryV3\":%d", c->txGenQry3);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GpQueryV2\":%d", c->txGrpQry2);
		        fputs(tmp, log);
		        snprintf(tmp, 256,",\"GpQueryV3\":%d", c->txGrpQry3);
		        fputs(tmp, log);			
		}
		pclose(fp);
	}
}

static void generate_wan(FILE *log)
{
	FILE *fp = popen("cat /proc/igmpproxy_counter", "r");
        char tmp[256];
        
	if(fp) {
		char buf[1024];
		struct igmp_pkt_calix_proxy_counter_t counter, *c;
		c = &counter;
		
		memset(buf, 0, sizeof(buf));
		memset(&counter, 0, sizeof(struct igmp_pkt_calix_proxy_counter_t));
		while (fgets(buf, sizeof(buf), fp)) {
			//util_fdprintf(fd,"%s", buf);
			sscanf(buf, "%16[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				c->vif,
				&c->rxGenQry1,
        			&c->rxGenQry2,
        			&c->rxGenQry3,
        			&c->rxGrpQry2,
        			&c->rxGrpQry3,
        			&c->txGenQry1,
        			&c->txGenQry2,
        			&c->txGenQry3,
        			&c->txGrpQry2,
        			&c->txGrpQry3,
        			&c->rxRpt1,
        			&c->rxRpt2,
        			&c->rxRpt3,
        			&c->rxLeave,
        			&c->txRpt1,
        			&c->txRpt2,
        			&c->txRpt3,
        			&c->txLeave);
        		
        		if (strcmp(c->vif, omci_env_g.router_wan_if) != 0) continue;
		        
			snprintf(tmp, 256, "\"GnQueryInV1\":%d", c->rxGenQry1);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GnQueryInV2\":%d", c->rxGenQry2);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GpQueryInV2\":%d", c->rxGrpQry2);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GnQueryInV3\":%d", c->rxGenQry3);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"GpQueryInV3\":%d", c->rxGrpQry3);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"V3ReportOut\":%d", c->txRpt1);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"V2ReportOut\":%d", c->txRpt2);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"V1ReportOut\":%d", c->txRpt3);
		        fputs(tmp, log);
		        snprintf(tmp, 256, ",\"V2LeaveOut\":%d", c->txLeave);
		        fputs(tmp, log);			
		}
		pclose(fp);
	}
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

// module functions
int
igmp_counter_calix_proxy_reset(void)
{
	// echo reset to /proc/igmpproxy_counter
	int status =0;
	int MAYBE_UNUSED ret = 0;
	if (system("echo reset > /proc/igmpproxy_counter") != -1)
		ret = WEXITSTATUS(status);
	
	count = 1;	// increment count
	
	return 0;
}

int
igmp_counter_calix_proxy_init(void)
{		
	return 0;
}

const char *igmp_counter_calix_proxy_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

int igmp_counter_calix_proxy_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
	return 0;
}

#define PRINT_CLI_ENTRY(h, s, rx, tx) util_fdprintf(h, "%-25s%16d%16d\n", s, rx, tx);

int igmp_cli_counter_calix_proxy(int fd)
{
	FILE *fp = popen("cat /proc/igmpproxy_counter", "r");

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
	if(fp) {
		char buf[1024];
		struct igmp_pkt_calix_proxy_counter_t counter, *c;
		c = &counter;
		
		memset(buf, 0, sizeof(buf));
		memset(&counter, 0, sizeof(struct igmp_pkt_calix_proxy_counter_t));
		while (fgets(buf, sizeof(buf), fp)) {
			//util_fdprintf(fd,"%s", buf);
			sscanf(buf, "%16[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				c->vif,
				&c->rxGenQry1,
        			&c->rxGenQry2,
        			&c->rxGenQry3,
        			&c->rxGrpQry2,
        			&c->rxGrpQry3,
        			&c->txGenQry1,
        			&c->txGenQry2,
        			&c->txGenQry3,
        			&c->txGrpQry2,
        			&c->txGrpQry3,
        			&c->rxRpt1,
        			&c->rxRpt2,
        			&c->rxRpt3,
        			&c->rxLeave,
        			&c->txRpt1,
        			&c->txRpt2,
        			&c->txRpt3,
        			&c->txLeave);

                	util_fdprintf(fd, "%-25s%16s%16s\n", c->vif, "RX", "TX");
                	PRINT_CLI_ENTRY(fd, "General Query v1:", c->rxGenQry1, c->txGenQry1)
                	PRINT_CLI_ENTRY(fd, "General Query v2:", c->rxGenQry2, c->txGenQry2)
                	PRINT_CLI_ENTRY(fd, "General Query v3:", c->rxGenQry3, c->rxGenQry3)
                	PRINT_CLI_ENTRY(fd, "Group Query v2:", c->rxGrpQry2, c->txGrpQry2)
                	PRINT_CLI_ENTRY(fd, "Group/Source Query v3:", c->rxGrpQry3, c->txGrpQry3)
                	PRINT_CLI_ENTRY(fd, "Report v1:", c->rxRpt1, c->txRpt1)
                	PRINT_CLI_ENTRY(fd, "Report v2:", c->rxRpt2, c->txRpt2)
                	PRINT_CLI_ENTRY(fd, "Report v3:", c->rxRpt3, c->txRpt3)
                	PRINT_CLI_ENTRY(fd, "Leave v2:", c->rxLeave, c->txLeave)
                	util_fdprintf(fd, "\n");
			
		}
		pclose(fp);
	}
	
	generate_calix_ui_dump();
	
	return 0;
}

int igmp_counter_iterator_calix_proxy(int (*iterator_function)(struct igmp_pkt_calix_proxy_counter_t *pos, void *data), void *iterator_data)
{       
        return 0;
}


struct igmp_counter_module_t calix_proxy =
{
        .module_name = igmp_counter_calix_proxy_name,
        .init = igmp_counter_calix_proxy_init,
        .reset = igmp_counter_calix_proxy_reset,
        .igmp_counter_handler = igmp_counter_calix_proxy_handler,
        .igmp_cli_handler = igmp_cli_counter_calix_proxy,
        .igmp_counter_iterator = igmp_counter_iterator_calix_proxy
};


