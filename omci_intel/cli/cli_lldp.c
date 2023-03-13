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
 * Module  : cli
 * File    : cli_lldp.c
 *
 ******************************************************************/
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "cli.h"
#include "switch.h"
#include "lldp_core.h"

static void
lldp_clear_counter(void)
{
	int i;
	extern lldp_parm_t lldp_parm[];
	for (i=0;i<SWITCH_LOGICAL_PORT_TOTAL;i++)
		memset(&lldp_parm[i].pkt_c, 0, sizeof(lldp_counter_t));
	return;
}

static int
lldp_print_counter(int fd)
{
	unsigned int port_id;
	extern lldp_parm_t lldp_parm[];
	util_fdprintf(fd, "PortID\tTX\tRX\n");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "%d\t%d\t%d\n", port_id,
				lldp_parm[port_id].pkt_c.tx,
				lldp_parm[port_id].pkt_c.rx);
		}
	}
	return 0;
}

static int
lldp_print_state(int fd, int is_raw)
{
	unsigned int port_id, len = 0;
	extern lldp_parm_t lldp_parm[];
	char buff[BUF_SIZE];
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config_metafile;
	int lldp_mgt = 0;
#endif
	lldp_network_policy_t* np_p;

#ifdef OMCI_METAFILE_ENABLE
	memset(&kv_config_metafile, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config_metafile);
	metacfg_adapter_config_file_load_part(&kv_config_metafile, "/etc/wwwctrl/metafile.dat", "lldp_management_capability", "lldp_management_capability");

	lldp_mgt = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, "lldp_management_capability", 1));

	metacfg_adapter_config_release(&kv_config_metafile);
#endif

	len = snprintf(buff, BUF_SIZE, "lldp status\n");
	if (!is_raw) {
		len += snprintf(buff + len, BUF_SIZE - len, "Management Owner : ");
	}
#ifdef OMCI_METAFILE_ENABLE
	len += snprintf(buff + len, BUF_SIZE - len, "%u\n", lldp_mgt);
#endif
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			np_p = &(lldp_parm[port_id].mib.network_policy);
			if (!is_raw) {
				len += snprintf(buff + len, BUF_SIZE - len, "\n%4s\t%6s\t%6s\t%6s\t%8s\t%4s\n", "Port", "Status", "Tagged", "VLANID", "Priority", "DSCP");
			}
			if (lldp_parm[port_id].mib.adminStatus == LLDP_DISABLE) {
				/* default setting at lldp is disable */
				len += snprintf(buff + len, BUF_SIZE - len, "%4d\t%6d\t%6d\t%6d\t%8d\t%4d\n", port_id, 1, 1, 4094, 1, 46);
			} else {
				len += snprintf(buff + len, BUF_SIZE - len, "%4d\t%6d\t%6d\t%6d\t%8d\t%4d\n", port_id, lldp_parm[port_id].mib.adminStatus, np_p->tagged, np_p->vid, np_p->pbit, np_p->dscp);
			}
		}
	}

	util_fdprintf(fd, "%s\n", buff);
	return 0;
}

static int lldp_cap_to_str(int cap, char* buf, int size)
{
	int len = 0, i;
	char* cap_to_str[] = {"Other", "Repeater", "Bridge", "WLAN Access Point", "Router", "Telephone", "DOCSIS Cable Device", "Station Only"};
	for (i=0;i<8;i++) {
		if ((size - len) > 0) {
			if ((((0xff & cap) >> i)) & 1) {
				len += snprintf(buf + len, size - len,"%s,", cap_to_str[i]);
			}
		}
	}
	if (len > 0) { /* Remove last "," */
		len = len - 1;
	}

	return len;
}

static int lldp_med_cap_to_str(int cap, char* buf, int size)
{
	int len = 0, i;
	char* cap_to_str[] = {"LLDP-MED Capabilities", "Networt Policy", "Location Identification", "MDI-PSE", "MDI-PD", "Inventory"};
	for (i=0;i<6;i++) {
		if ((size - len) > 0) {
			if ((((0xff & cap) >> i)) & 1) {
				len += snprintf(buf + len, size - len,"%s,", cap_to_str[i]);
			}
		}
	}
	if (len > 0) { /* Remove last "," */
		len = len - 1;
	}

	return len;
}

static int lldp_print_neighbor_port(char * buf, int size, unsigned int port_id)
{
	extern lldp_pkt_t neighbor[];
	lldp_pkt_t* np = &neighbor[port_id];
	int len = 0;
	/* mandatory TLVs  */
	/* 1) Chassis ID TLV */
	len += snprintf(buf + len, BUF_SIZE - len,
		"{\n\"Chassis ID\":   {\n\t\"subtype\":\t%d,\n\t\"id\":    \"%02x:%02x:%02x:%02x:%02x:%02x\"\n},\n",
		np->chassis_id.subtype, np->chassis_id.chassis_id[0], np->chassis_id.chassis_id[1],
		np->chassis_id.chassis_id[2], np->chassis_id.chassis_id[3], np->chassis_id.chassis_id[4], np->chassis_id.chassis_id[5]);
	/* 2) Port ID TLV */
	len += snprintf(buf + len, BUF_SIZE - len,
		"\"Port ID\":   {\n\t\"subtype\":\t%d,\n\t\"id\":    \"%02x:%02x:%02x:%02x:%02x:%02x\"\n},\n",
		np->port_id.subtype, np->port_id.port_id[0], np->port_id.port_id[1],
		np->port_id.port_id[2], np->port_id.port_id[3], np->port_id.port_id[4], np->port_id.port_id[5]);
	/* 3) Time To Live TLV */
	len += snprintf(buf + len, BUF_SIZE - len, "\"TTL\":  %d,\n", np->ttl.ttl);

	/* Optional TLVs */
	if (np->port_desc.desc && (BUF_SIZE - len) > 0)
		len += snprintf(buf + len, BUF_SIZE - len, "\"Port Description\": \"%s\",\n", np->port_desc.desc);

	if (np->system_name.name && (BUF_SIZE - len) > 0)
		len += snprintf(buf + len, BUF_SIZE - len, "\"System Name\": \"%s\",\n", np->system_name.name);

	if (np->system_desc.desc && (BUF_SIZE - len) > 0)
		len += snprintf(buf + len, BUF_SIZE - len, "\"System Description\":  \"%s\",\n", np->system_desc.desc);

	if ((np->system_cap.en_cap != 0 || np->system_cap.cap != 0) && (BUF_SIZE - len) > 0) {
		len += snprintf(buf + len, BUF_SIZE - len, "\"System Capabilities\":  {\n\t\"Capabilities\": \"");
		len += lldp_cap_to_str(np->system_cap.cap, buf + len, BUF_SIZE - len);
		len += snprintf(buf + len, BUF_SIZE - len, "\",\n\t\"Enabled Capabilities\": \"");
		len += lldp_cap_to_str(np->system_cap.en_cap, buf + len, BUF_SIZE - len);
		len += snprintf(buf + len, BUF_SIZE - len, "\"\n},\n");
	}
	if (np->manage_addr_ipv4.subtype == 1 && (BUF_SIZE - len) > 0) {
		len += snprintf(buf + len, BUF_SIZE - len, "\"Management Address\":   {\n"
				  "\t\"subtype\":  1,\n"
				  "\t\"Management Address\": \"%d.%d.%d.%d\",\n"
				  "\t\"Interface Number\": %d\n"
				  "},\n", 0xff & (np->manage_addr_ipv4.addr >> 24), 0xff & (np->manage_addr_ipv4.addr >> 16),
				  0xff & (np->manage_addr_ipv4.addr >> 8), 0xff & np->manage_addr_ipv4.addr,
				  np->manage_addr_ipv4.itf_number);

	}
	if (np->manage_addr_ipv6.subtype == 2 && (BUF_SIZE - len) > 0) {
		len += snprintf(buf + len, BUF_SIZE - len,"\"Management Address\":   {\n"
				  "\t\"subtype\":  2,\n"
				  "\t\"Management Address\": \"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\",\n"
				  "\t\"Interface Number\": %d\n"
				  "},\n", 0xffff & (np->manage_addr_ipv6.addr[0] >> 16), 0xffff & (np->manage_addr_ipv6.addr[0]),
				  0xffff & (np->manage_addr_ipv6.addr[1] >> 16), 0xffff & (np->manage_addr_ipv6.addr[1]),
				  0xffff & (np->manage_addr_ipv6.addr[2] >> 16), 0xffff & (np->manage_addr_ipv6.addr[2]),
				  0xffff & (np->manage_addr_ipv6.addr[3] >> 16), 0xffff & (np->manage_addr_ipv6.addr[3]),
				  np->manage_addr_ipv6.itf_number);

	}
	if (np->media_cap.subtype == 1) {
		len += snprintf(buf + len, BUF_SIZE - len,"\"MED Capabilities\":   {\n"
				"\t\"Capabilities\": \"");
		len += lldp_med_cap_to_str(np->media_cap.cap, buf + len, BUF_SIZE - len);
		len += snprintf(buf + len, BUF_SIZE - len,"\",\n\t\"device type\": %d\n},\n",np->media_cap.type);
	}
	len += snprintf(buf + len, BUF_SIZE - len, "\"localPort\":    \"eth%d\"\n},\n", port_id);
	return len;
}

int lldp_print_neighbor(int fd)
{
	int port_id, len = 0;
	extern lldp_parm_t lldp_parm[];
	char buf[BUF_SIZE] = {0};
	for (port_id=0;port_id<4;port_id++) {
		if (lldp_parm[port_id].mib.rxTTL > 0 && (BUF_SIZE - len) > 0)
			len += lldp_print_neighbor_port(buf + len, BUF_SIZE - len, port_id);
	}
	if (len > 2)
		len += snprintf(buf + (len - 2), BUF_SIZE - len + 2, "\n");
	util_fdprintf(fd, "\"neighbor_status\": [\n%s\n]\n [end_of_message]\n", buf);
	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_lldp_help(int fd)
{
	util_fdprintf(fd,
		"lldp help|[subcmd...]\n");
}

void
cli_lldp_help_long(int fd)
{
	util_fdprintf(fd,
		"lldp	start\n"
		"	stop\n"
		"	counter [show|clear]\n"
		"	neighbor\n"
		"	status\n"
		);
}

int
cli_lldp_cmdline(int fd, int argc, char *argv[])
{
	if ((lldp_task_loop_g == 1) && strcmp(argv[0], "lldp") == 0) {
		if (argc==1) {
			return lldp_print_counter(fd);
		} else if (argc>=2 && strcmp(argv[1], "status") == 0) {
			int is_raw=0;
			if (argc == 2 || strcmp(argv[2], "1") == 0) is_raw = 1;
			lldp_print_state(fd, is_raw);
			return 0;
		} else if (argc==2 && strcmp(argv[1], "start") == 0) {
			util_fdprintf(fd, "lldp task had already started\n");
			return 0;
		} else if (argc==2 && strcmp(argv[1], "neighbor") == 0) {
			lldp_print_neighbor(fd);
			return 0;
		} else if (argc==2 && strcmp(argv[1], "stop") == 0) {
			if (lldp_task_loop_g == 0) {
				util_fdprintf(fd, "lldp task did not run\n");
			} else {
				lldp_stop();
				lldp_shutdown();
			}
			return 0;
		} else if (argc==2 && strcmp("help", argv[1])==0) {
			cli_lldp_help_long(fd);
			return 0;
		} else if (argc==3 && strcmp("counter", argv[1])==0) {
			if (strcmp(argv[2], "clear") == 0) {
				lldp_clear_counter();
				lldp_print_counter(fd);
			} else if (strcmp(argv[2], "show") == 0) {
				lldp_print_counter(fd);
			} else
				cli_lldp_help_long(fd);
			return 0;
		}
		util_fdprintf(fd, "Invalid command. Valid commands:\n");
		cli_lldp_help_long(fd);
		return CLI_ERROR_SYNTAX;
	} else if ((lldp_task_loop_g == 0) && strcmp(argv[0], "lldp") == 0) {
		if (argc==2 && strcmp(argv[1], "start") == 0) {
			lldp_init();
			lldp_start();
			return 0;
		} else
			return CLI_ERROR_OTHER_CATEGORY;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}
