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
 * File    : cli_switch.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
 
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cli.h"
#include "me_related.h"
#include "hwresource.h"
#include "switch.h"
#include "coretask.h"	// for coretask_cmd();

static int
cli_switch_summary(int fd)
{
	switch_hw_g.eth_address_print(fd);
	switch_hw_g.hw_usage_print(fd);
	switch_hw_g.port_status_print(fd);
	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_switch_help(int fd)
{
	util_fdprintf(fd, "switch help|[subcmd...]\n");
}

void
cli_switch_help_long(int fd)
{
	util_fdprintf(fd, 
		"switch storm|rldp|stp\n"
		"       pkt [sum|pkt|stat|err|*|reset]\n"
		"       pkt [reset_port] portid\n"
		"       mactab [uc|mc|ipmc|*]\n"
		"       port [status|config|*]\n"
		"       port phy [port_id]\n"
		"       port phy [reg_id] [value]\n"
		"       rate [status|ctrl|meter|*]\n"
		"       qos [portcfg|dscp2pri|prisel|pri2q|q2meter|*]\n"
		"       intr [stat|ctrl|*]\n"
		"       iso [0|1]\n"
		"       iso [src_port] [dst_portmask]\n"
		"       history|h [0..14]|reset\n"
		);
}

int
cli_switch_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "switch")==0 || strcmp(argv[0], "sw")==0) {
		if (argc==1) {
			cli_switch_summary(fd);
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "help")==0) {
			cli_switch_help_long(fd);
			return CLI_OK;

		} else if (argc>=2 && strcmp(argv[1], "pkt")==0) {
			if (argc==2) {
				switch_hw_g.pm_summary_print(fd, 0);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "sum")==0) {
					switch_hw_g.pm_summary_print(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "pkt")==0) {
					switch_hw_g.pm_summary_print(fd, 1);
					return CLI_OK;
				} else if (strcmp(argv[2], "stat")==0) {
					switch_hw_g.pm_summary_print(fd, 2);
					return CLI_OK;
				} else if (strcmp(argv[2], "err")==0) {
					switch_hw_g.pm_summary_print(fd, 3);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.pm_summary_print(fd, 4);
					return CLI_OK;
				} else if (strcmp(argv[2], "reset")==0) {
					switch_hw_g.pm_summary_print(fd, -1);
					return CLI_OK;
				} else {
					int type = util_atoi(argv[2]);
					switch_hw_g.pm_summary_print(fd, type);
					return CLI_OK;
				}
			} else if (argc==4) {
				if (strcmp(argv[2], "reset_port")==0) {
					int port = util_atoi(argv[3]);
					switch_hw_g.pm_reset_port(port);
					return CLI_OK;
				}
			}

		} else if (argc>=2 && strcmp(argv[1], "mactab")==0) {
			if (argc==2) {
				switch_hw_g.mac_table_print_valid(fd, 4);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "uc")==0) {
					switch_hw_g.mac_table_print_valid(fd, 0);
					return CLI_OK;
				} else if (strcmp(argv[2], "mc")==0) {
					switch_hw_g.mac_table_print_valid(fd, 1);
					return CLI_OK;
				} else if (strcmp(argv[2], "ipmc")==0) {
					switch_hw_g.mac_table_print_valid(fd, 2);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.mac_table_print_valid(fd, 4);
					return CLI_OK;
				} else if (strcmp(argv[2], "flush")==0) {
					// flush all dynamic entry
					switch_hw_g.mac_table_flush(switch_get_all_logical_portmask(), 0);
				} else {
					int type = util_atoi(argv[2]);
					switch_hw_g.mac_table_print_valid(fd, type);
					return CLI_OK;
				}
			}

		} else if (argc>=2 && strcmp(argv[1], "port")==0) {
			if (argc==2) {
				switch_hw_g.port_status_print(fd);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "status")==0) {
					switch_hw_g.port_status_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "config")==0) {
					switch_hw_g.port_config_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.port_status_print(fd);
					switch_hw_g.port_config_print(fd);
					return CLI_OK;
				}				
			} else if (argc==4) {
				if (strcmp(argv[2], "phy")==0) {
					int port = util_atoi(argv[3]);
					switch_hw_g.port_phy_reg_print(fd, port);
					return CLI_OK;
				}
			} else if (argc==5) {
				if (strcmp(argv[2], "phy")==0) {
					int reg_id = util_atoi(argv[3]);
					int value = util_atoi(argv[4]);
					switch_hw_g.port_phy_reg_interpret_print(fd, reg_id, value);
					return CLI_OK;
				}
			}
		} else if (argc>=2 && strcmp(argv[1], "qos")==0) {
			if (argc==2) {
				switch_hw_g.qos_portcfg_print(fd);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "portcfg")==0) {
					switch_hw_g.qos_portcfg_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "dscp2pri")==0) {
					switch_hw_g.qos_dscp2pri_print(fd);		// dscp2pri
					return CLI_OK;
				} else if (strcmp(argv[2], "dot1p2pri")==0) {
					switch_hw_g.qos_dot1p2pri_print(fd);		// dot1p2pri
					return CLI_OK;
				} else if (strcmp(argv[2], "prisel")==0) {
					switch_hw_g.qos_prisel_weight_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "pri2q")==0) {
					switch_hw_g.qos_p2q_print(fd);		// pri2queue
					return CLI_OK;
				} else if (strcmp(argv[2], "q2meter")==0) {
					switch_hw_g.qos_meter_print(fd);		// queue2meter
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.qos_portcfg_print(fd);
					switch_hw_g.qos_dscp2pri_print(fd);		// dscp2pri
					switch_hw_g.qos_dot1p2pri_print(fd);		// dot1p2pri
					switch_hw_g.qos_prisel_weight_print(fd);
					switch_hw_g.qos_p2q_print(fd);		// pri2queue
					switch_hw_g.qos_meter_print(fd);		// queue2meter
					return CLI_OK;
				}
			}

		} else if (argc>=2 && strcmp(argv[1], "rate")==0) {
			if (argc==2) {
				switch_hw_g.rate_status_print(fd);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "status")==0) {
					switch_hw_g.rate_status_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "ctrl")==0) {
					switch_hw_g.rate_control_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "meter")==0) {
					switch_hw_g.rate_meter_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.rate_status_print(fd);
					switch_hw_g.rate_control_print(fd);
					switch_hw_g.rate_meter_print(fd);
					return CLI_OK;
				}				
			}
			
		} else if (argc>=2 && strcmp(argv[1], "intr")==0) {
			if (argc==2) {
				switch_hw_g.int_ctrl_print(fd);
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "status")==0) {
					switch_hw_g.int_stat_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "ctrl")==0) {
					switch_hw_g.int_ctrl_print(fd);
					return CLI_OK;
				} else if (strcmp(argv[2], "*")==0) {
					switch_hw_g.int_stat_print(fd);
					switch_hw_g.int_ctrl_print(fd);
					return CLI_OK;
				}				
			}

		} else if (argc>=2 && strcmp(argv[1], "iso")==0) {
			if (argc==2) {
				switch_hw_g.port_isolation_print(fd, 0);
				return CLI_OK;
			} else if (argc==3) {
				int mode = util_atoi(argv[2]);
				switch_hw_g.port_isolation_print(fd, mode);
				return CLI_OK;
			} else if (argc==4) {
				unsigned int src_port = util_atoi(argv[2]);
				unsigned int dst_portmask = util_atoi(argv[3]);
				switch_hw_g.port_isolation_set(src_port, dst_portmask);
				return CLI_OK;
			}

		} else if (argc==2 && strcmp(argv[1], "storm")==0) {
			unsigned int valid_portmask = switch_get_all_logical_portmask_without_cpuext();
			int i;
			for (i=0; i< SWITCH_LOGICAL_PORT_TOTAL; i++) {
				if ((1<<i) & valid_portmask)
					switch_hw_g.rate_storm_print(fd, i);
			}
			return CLI_OK;
			
		} else if (argc==2 && strcmp(argv[1], "rldp")==0) {
			switch_hw_g.rldp_loop_print(fd);
			return CLI_OK;

		} else if (argc==2 && strcmp(argv[1], "stp")==0) {
			switch_hw_g.stp_state_print(fd);
			return CLI_OK;

		} else if (argc>=2 && (strcmp(argv[1], "history")==0 || strcmp(argv[1], "h")==0)) {
			if (argc==2) {
				switch_hw_g.history_print(fd, -1);	// show type help
				util_fdprintf(fd, "  reset - clear switch history\n");
				return CLI_OK;
			} else if (argc==3) {
				if (strcmp(argv[2], "reset") == 0) {
					switch_hw_g.history_init();
					coretask_cmd(CORETASK_CMD_SWITCH_HISTORY_INTERVAL_CHANGE);
					return CLI_OK;					
				} else {
					int type = util_atoi(argv[2]);
					switch_hw_g.history_print(fd, type);
					return CLI_OK;
				}
			}

		} else if (argc>=2 && strcmp(argv[1], "mtu")==0) {
			if (argc == 2) {	// sw mtu
				unsigned int mtu, i;
				for (i=0; i< SWITCH_LOGICAL_PORT_TOTAL; i++) {
					if ( ((1<<i) & switch_get_uni_logical_portmask()) ||
					     i == switch_get_cpu_logical_portid(0) ||
					     i == switch_get_wan_logical_portid()) {
					     	if (switch_hw_g.port_eth_mtu_get(i, &mtu) == 0) {
					     		util_fdprintf(fd, "port %d mtu = %d\n", i, mtu);
						}
					}
				}
			} else if (argc == 4) {	// sw mtu port value
				int port = util_atoi(argv[2]);
				int mtu = util_atoi(argv[3]);
				switch_hw_g.port_eth_mtu_set(port, mtu);
			}
			
		} else {
			return CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'switch'
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}
