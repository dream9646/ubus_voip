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
 * File    : cli_stp.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "cli.h"
#include "bitmap.h"
#include "uid_stp.h"
#include "stp_in.h"
#include "stp_to.h"
#include "cpuport.h"
#include "uid.h"
#include "bridge.h"
#include "util.h"
#include "rstp.h"

static void
print_bridge_id(int fd, UID_BRIDGE_ID_T * bridge_id, unsigned char cr)
{
	util_fdprintf(fd, "%04lX-%02x%02x%02x%02x%02x%02x",
		      (unsigned long)bridge_id->prio,
		      (unsigned char)bridge_id->addr[0],
		      (unsigned char)bridge_id->addr[1],
		      (unsigned char)bridge_id->addr[2],
		      (unsigned char)bridge_id->addr[3], (unsigned char)bridge_id->addr[4], (unsigned char)bridge_id->addr[5]);
	if (cr)
		util_fdprintf(fd, "\n");
}

static char *
stp_state2str(RSTP_PORT_STATE stp_port_state, int detail)
{
	if (detail) {
		switch (stp_port_state) {
		case UID_PORT_DISABLED:
			return "Disabled";
		case UID_PORT_DISCARDING:
			return "Discarding";
		case UID_PORT_LEARNING:
			return "Learning";
		case UID_PORT_FORWARDING:
			return "Forwarding";
		case UID_PORT_NON_STP:
			return "NoStp";
		default:
			return "Unknown";
		}
	}

	switch (stp_port_state) {
	case UID_PORT_DISABLED:
		return "Dis";
	case UID_PORT_DISCARDING:
		return "Blk";
	case UID_PORT_LEARNING:
		return "Lrn";
	case UID_PORT_FORWARDING:
		return "Fwd";
	case UID_PORT_NON_STP:
		return "Non";
	default:
		return "Unk";
	}
}

static void
CLI_out_port_id(int fd, int port, unsigned char cr)
{
	util_fdprintf(fd, "%s", STP_OUT_get_port_name(port));
	if (cr)
		util_fdprintf(fd, "\n");
}

static int
cli_stp_enable(int fd)
{
	UID_STP_CFG_T uid_cfg;
	int rc;

	uid_cfg.field_mask = BR_CFG_STATE;
	uid_cfg.stp_enabled = STP_ENABLED;
	rc = STP_IN_stpm_set_cfg(0, NULL, &uid_cfg);
	if (rc) {
		util_fdprintf(fd, "can't enable: %s\n", STP_IN_get_error_explanation(rc));
	}

	return 0;
}

static int
cli_stp_disable(int fd)
{
	UID_STP_CFG_T uid_cfg;
	int rc;

	uid_cfg.field_mask = BR_CFG_STATE;
	uid_cfg.stp_enabled = STP_DISABLED;
	rc = STP_IN_stpm_set_cfg(0, NULL, &uid_cfg);
	if (rc) {
		util_fdprintf(fd, "can't disable: %s\n", STP_IN_get_error_explanation(rc));
	}

	return 0;
}

static int
cli_stp_br_get_cfg(int fd)
{
	UID_STP_STATE_T uid_state;
	UID_STP_CFG_T uid_cfg;
	int rc;

	rc = STP_IN_stpm_get_state(0, &uid_state);
	if (rc) {
		util_fdprintf(fd, "can't get rstp bridge state: %s\n", STP_IN_get_error_explanation(rc));
		return 0;
	}
	rc = STP_IN_stpm_get_cfg(0, &uid_cfg);
	if (rc) {
		util_fdprintf(fd, "can't get rstp bridge configuration: %s\n", STP_IN_get_error_explanation(rc));
		return 0;
	}
#if 0
	printf("Interface:       %-7s (tag:%d)    State: ", uid_state.vlan_name, (int)uid_state.vlan_id);
#else
//  util_fdprintf( fd,"Bridge:    %-7s         State:",
//       uid_state.vlan_name);
#endif
	switch (uid_state.stp_enabled) {
	case STP_ENABLED:
		util_fdprintf(fd, "enabled\n");
		break;
	case STP_DISABLED:
		util_fdprintf(fd, "disabled\n");
		break;
	default:
		util_fdprintf(fd, "unknown\n");
		return 0;
	}

	util_fdprintf(fd, "BridgeId:	");
	print_bridge_id(fd, &uid_state.bridge_id, 0);
	util_fdprintf(fd, "     Bridge Proirity: %lu (0x%lX)\n",
		      (unsigned long)uid_state.bridge_id.prio, (unsigned long)uid_state.bridge_id.prio);
	if (uid_cfg.force_version < 2)
		util_fdprintf(fd, "Force Version:   stp\n");

	util_fdprintf(fd, "Designated Root: ");
	print_bridge_id(fd, &uid_state.designated_root, 1);
	if (uid_state.root_port) {
		util_fdprintf(fd, "Root Port:       %04lx (", (unsigned long)uid_state.root_port);
		CLI_out_port_id(fd, uid_state.root_port & 0xfff, False);
		util_fdprintf(fd, "), Root Cost:     %-lu\n", (unsigned long)uid_state.root_path_cost);
	} else {
		util_fdprintf(fd, "Root Port:       none\n");
	}

	if (uid_state.Topo_Change)
		util_fdprintf(fd, "Topology Change Count: %lu\n", uid_state.Topo_Change_Count);
	else
		util_fdprintf(fd, "Time Since Topology Change: %lu\n", uid_state.timeSince_Topo_Change);

	util_fdprintf(fd, "Max Age:	 %2d   Bridge Max Age:       %-2d\n", (int)uid_state.max_age, (int)uid_cfg.max_age);
	util_fdprintf(fd,
		      "Hello Time:      %2d   Bridge Hello Time:    %-2d\n",
		      (int)uid_state.hello_time, (int)uid_cfg.hello_time);
	util_fdprintf(fd,
		      "Forward Delay:   %2d   Bridge Forward Delay: %-2d\n",
		      (int)uid_state.forward_delay, (int)uid_cfg.forward_delay);
	util_fdprintf(fd, "Hold Time:       %2d\n", (int)uid_cfg.hold_time);

	return 0;
}

static void
show_rstp_port(int fd, BITMAP_T * ports_bitmap, int detail)
{
	UID_STP_STATE_T uid_state;
	UID_STP_PORT_STATE_T uid_port;
	UID_STP_PORT_CFG_T uid_cfg;
	int port_index;
	int rc;

	rc = STP_IN_stpm_get_state(0, &uid_state);
	if (rc) {
		util_fdprintf(fd, "can't get rstp bridge state: %s\n", STP_IN_get_error_explanation(rc));
	} else if (!detail) {
		util_fdprintf(fd, " BridgeId: ");
		print_bridge_id(fd, &uid_state.bridge_id, 0);
		util_fdprintf(fd, "  RootId: ");
		print_bridge_id(fd, &uid_state.designated_root, 1);
	}

	for (port_index = 0; port_index <= number_of_ports; port_index++) {
		if (!BitmapGetBit(ports_bitmap, port_index - 1))
			continue;
		uid_port.port_no = port_index;
		rc = STP_IN_port_get_state(0, &uid_port);
		if (rc) {
			util_fdprintf(fd, "can't get rstp port state: %s\n", STP_IN_get_error_explanation(rc));
			continue;
		}
		memset(&uid_cfg, 0, sizeof(UID_STP_PORT_CFG_T));
		rc = STP_IN_port_get_cfg(0, uid_port.port_no, &uid_cfg);
		if (rc) {
			util_fdprintf(fd, "can't get rstp port config: %s\n", STP_IN_get_error_explanation(rc));
			continue;
		}

		if (detail) {
			util_fdprintf(fd, "Stp Port ");
			CLI_out_port_id(fd, port_index, False);
#if 0
			util_fdprintf(fd,
				      ": PortId: %04lx in vlan '%s' with tag %d:\n",
				      (unsigned long)uid_port.port_id, uid_state.vlan_name, (int)uid_state.vlan_id);
#else
			util_fdprintf(fd, ": PortId: %04lx\n", (unsigned long)uid_port.port_id);
#endif
			util_fdprintf(fd, "Priority:	  %-d\n", (int)(uid_port.port_id >> 8));
			util_fdprintf(fd, "State:	     %-16s", stp_state2str(uid_port.state, 1));
			util_fdprintf(fd, "       Uptime: %-9lu\n", uid_port.uptime);
			util_fdprintf(fd, "PortPathCost:      admin: ");
			if (ADMIN_PORT_PATH_COST_AUTO == uid_cfg.admin_port_path_cost)
				util_fdprintf(fd, "%-9s", "Auto");
			else
				util_fdprintf(fd, "%-9lu", uid_cfg.admin_port_path_cost);
			util_fdprintf(fd, "       oper: %-9lu\n", uid_port.oper_port_path_cost);

			util_fdprintf(fd, "Point2Point:       admin: ");
			switch (uid_cfg.admin_point2point) {
			case P2P_FORCE_TRUE:
				util_fdprintf(fd, "%-9s", "ForceYes");
				break;
			case P2P_FORCE_FALSE:
				util_fdprintf(fd, "%-9s", "ForceNo");
				break;
			case P2P_AUTO:
				util_fdprintf(fd, "%-9s", "Auto");
				break;
			}
			util_fdprintf(fd, "       oper: %-9s\n", uid_port.oper_point2point ? "Yes" : "No");
			util_fdprintf(fd,
				      "Edge:	      admin: %-9s       oper: %-9s\n",
				      uid_cfg.admin_edge ? "Y" : "N", uid_port.oper_edge ? "Y" : "N");
			util_fdprintf(fd,
				      "Partner:				  oper: %-9s\n",
				      uid_port.oper_stp_neigb ? "Slow" : "Rapid");

			if (' ' != uid_port.role) {
				if ('-' != uid_port.role) {
					util_fdprintf(fd, "PathCost:	  %-lu\n", (unsigned long)(uid_port.path_cost));
					util_fdprintf(fd, "Designated Root:   ");
					print_bridge_id(fd, &uid_port.designated_root, 1);
					util_fdprintf(fd, "Designated Cost:   %-ld\n", (unsigned long)
						      uid_port.designated_cost);
					util_fdprintf(fd, "Designated Bridge: ");
					print_bridge_id(fd, &uid_port.designated_bridge, 1);
					util_fdprintf(fd, "Designated Port:   %-4lx\n\r", (unsigned long)
						      uid_port.designated_port);
				}
				util_fdprintf(fd, "Role:	      ");
				switch (uid_port.role) {
				case 'A':
					util_fdprintf(fd, "Alternate\n");
					break;
				case 'B':
					util_fdprintf(fd, "Backup\n");
					break;
				case 'R':
					util_fdprintf(fd, "Root\n");
					break;
				case 'D':
					util_fdprintf(fd, "Designated\n");
					break;
				case '-':
					util_fdprintf(fd, "NonStp\n");
					break;
				default:
					util_fdprintf(fd, "Unknown(%c)\n", uid_port.role);
					break;
				}

				if ('R' == uid_port.role || 'D' == uid_port.role) {
					/* util_fdprintf( fd,"Tc:             %c  ", uid_port.tc ? 'Y' : 'n'); */
					util_fdprintf(fd, "TcAck:	     %c  ", uid_port.top_change_ack ? 'Y' : 'N');
					util_fdprintf(fd, "TcWhile:       %3d\n", (int)uid_port.tcWhile);
				}
			}

			if (UID_PORT_DISABLED == uid_port.state || '-' == uid_port.role) {
#if 0
				util_fdprintf(fd, "helloWhen:       %3d  ", (int)uid_port.helloWhen);
				util_fdprintf(fd, "lnkWhile:      %3d\n", (int)uid_port.lnkWhile);
				util_fdprintf(fd, "fdWhile:	 %3d\n", (int)uid_port.fdWhile);
#endif
			} else if ('-' != uid_port.role) {
				util_fdprintf(fd, "fdWhile:	 %3d  ", (int)uid_port.fdWhile);
				util_fdprintf(fd, "rcvdInfoWhile: %3d\n", (int)uid_port.rcvdInfoWhile);
				util_fdprintf(fd, "rbWhile:	 %3d  ", (int)uid_port.rbWhile);
				util_fdprintf(fd, "rrWhile:       %3d\n", (int)uid_port.rrWhile);
#if 0
				util_fdprintf(fd, "mdelayWhile:     %3d  ", (int)uid_port.mdelayWhile);
				util_fdprintf(fd, "lnkWhile:      %3d\n", (int)uid_port.lnkWhile);
				util_fdprintf(fd, "helloWhen:       %3d  ", (int)uid_port.helloWhen);
				util_fdprintf(fd, "txCount:       %3d\n", (int)uid_port.txCount);
#endif
			}

			util_fdprintf(fd, "RSTP BPDU rx:      %lu\n", (unsigned long)uid_port.rx_rstp_bpdu_cnt);
			util_fdprintf(fd, "CONFIG BPDU rx:    %lu\n", (unsigned long)uid_port.rx_cfg_bpdu_cnt);
			util_fdprintf(fd, "TCN BPDU rx:       %lu\n", (unsigned long)uid_port.rx_tcn_bpdu_cnt);
		} else {
			util_fdprintf(fd, "%c%c%c  ",
				      (uid_port.
				       oper_point2point) ? ' ' : '*',
				      (uid_port.oper_edge) ? 'E' : ' ', (uid_port.oper_stp_neigb) ? 's' : ' ');
			CLI_out_port_id(fd, port_index, False);
			util_fdprintf(fd, " %04lx %3s ", (unsigned long)uid_port.port_id, stp_state2str(uid_port.state, 0));
			util_fdprintf(fd, " ");
			print_bridge_id(fd, &uid_port.designated_root, 0);
			util_fdprintf(fd, " ");
			print_bridge_id(fd, &uid_port.designated_bridge, 0);
			util_fdprintf(fd, " %4lx %c", (unsigned long)uid_port.designated_port, uid_port.role);
			util_fdprintf(fd, "\n");
		}
	}
}

static int
cli_stp_pr_get_cfg(int fd, int detail, int port_index)
{
	BITMAP_T ports_bitmap;

	if (detail == 0) {
		BitmapSetAllBits(&ports_bitmap);
	} else {
		BitmapClear(&ports_bitmap);
		BitmapSetBit(&ports_bitmap, port_index - 1);
	}
	show_rstp_port(fd, &ports_bitmap, detail);
	return 0;
}

#ifdef STP_BRIDGE_CFG
static void
set_bridge_cfg_value(int fd, unsigned long value, unsigned long val_mask)
{
	UID_STP_CFG_T uid_cfg;
	char *val_name;
	int rc;

	uid_cfg.field_mask = val_mask;
	switch (val_mask) {
	case BR_CFG_STATE:
		uid_cfg.stp_enabled = value;
		val_name = "state";
		break;
	case BR_CFG_PRIO:
		uid_cfg.bridge_priority = value;
		val_name = "priority";
		break;
	case BR_CFG_AGE:
		uid_cfg.max_age = value;
		val_name = "max_age";
		break;
	case BR_CFG_HELLO:
		uid_cfg.hello_time = value;
		val_name = "hello_time";
		break;
	case BR_CFG_DELAY:
		uid_cfg.forward_delay = value;
		val_name = "forward_delay";
		break;
	case BR_CFG_FORCE_VER:
		uid_cfg.force_version = value;
		val_name = "force_version";
		break;
	case BR_CFG_AGE_MODE:
	case BR_CFG_AGE_TIME:
	default:
		util_fdprintf(fd, "Invalid value mask 0X%lx\n", val_mask);
		return;
		break;
	}

	rc = STP_IN_stpm_set_cfg(0, NULL, &uid_cfg);

	if (0 != rc) {
		util_fdprintf(fd, "Can't change rstp bridge %s:%s", val_name, STP_IN_get_error_explanation(rc));
	} else {
		util_fdprintf(fd, "Changed rstp bridge %s\n", val_name);
	}
}

static int
cli_stp_br_prio(int fd, long br_prio)
{
/*
  if (strlen (argv[1]) > 2 &&
      (! strncmp (argv[1], "0x", 2) || ! strncmp (argv[1], "0X", 2))) {
    br_prio = strtoul(argv[1] + 2, 0, 16);
  } else {
    br_prio = strtoul(argv[1], 0, 10);
  }
*/
	if (!br_prio) {
		util_fdprintf(fd, "Warning: newPriority=0, are you sure ?\n");
	}

	set_bridge_cfg_value(fd, br_prio, BR_CFG_PRIO);

	return 0;
}

static int
cli_stp_br_maxage(int fd, long value)
{
	set_bridge_cfg_value(fd, value, BR_CFG_AGE);
	return 0;
}

static int
cli_stp_br_fdelay(int fd, long value)
{
	set_bridge_cfg_value(fd, value, BR_CFG_DELAY);
	return 0;
}

static int
cli_stp_br_fvers(int fd, long value)
{
/*
  switch (argv[1][0]) {
      case '0':
      case '1':
      case 'f':
      case 'F':
	value = 0L;
	util_fdprintf( fd,"Accepted 'force_slow'\n");
	break;
      case '2':
      case 'r':
      case 'R':
	util_fdprintf( fd,"Accepted 'rapid'\n");
	value = 2L;
	break;
      default:
	util_fdprintf( fd,"Invalid argument '%s'\n", argv[1]);
	return 0;
  }
  */
	set_bridge_cfg_value(fd, value, BR_CFG_FORCE_VER);
	return 0;
}
#endif

#ifdef STP_PORT_CFG
static void
set_rstp_port_cfg_value(int fd, int port_index, unsigned long value, unsigned long val_mask)
{
	UID_STP_PORT_CFG_T uid_cfg;
	int rc;
	char *val_name;

	if (port_index > 0) {
		BitmapClear(&uid_cfg.port_bmp);
		BitmapSetBit(&uid_cfg.port_bmp, port_index - 1);
	} else {
		BitmapSetAllBits(&uid_cfg.port_bmp);
	}

	uid_cfg.field_mask = val_mask;
	switch (val_mask) {
	case PT_CFG_MCHECK:
		val_name = "mcheck";
		break;
	case PT_CFG_COST:
		uid_cfg.admin_port_path_cost = value;
		val_name = "path cost";
		break;
	case PT_CFG_PRIO:
		uid_cfg.port_priority = value;
		val_name = "priority";
		break;
	case PT_CFG_P2P:
		uid_cfg.admin_point2point = (ADMIN_P2P_T) value;
		val_name = "p2p flag";
		break;
	case PT_CFG_EDGE:
		uid_cfg.admin_edge = value;
		val_name = "adminEdge";
		break;
	case PT_CFG_NON_STP:
		uid_cfg.admin_non_stp = value;
		val_name = "adminNonStp";
		break;
#ifdef STP_DBG
	case PT_CFG_DBG_SKIP_TX:
		uid_cfg.skip_tx = value;
		val_name = "skip tx";
		break;
	case PT_CFG_DBG_SKIP_RX:
		uid_cfg.skip_rx = value;
		val_name = "skip rx";
		break;
#endif
	case PT_CFG_STATE:
	default:
		util_fdprintf(fd, "Invalid value mask 0X%lx\n", val_mask);
		return;
	}

	rc = STP_IN_set_port_cfg(0, &uid_cfg);
	if (0 != rc) {
		util_fdprintf(fd, "can't change rstp port[s] %s: %s\n", val_name, STP_IN_get_error_explanation(rc));
	} else {
		util_fdprintf(fd, "changed rstp port[s] %s\n", val_name);
	}

	/* show_rstp_port (&uid_cfg.port_bmp, 0); */
}

static int
cli_stp_prt_prio(int fd, unsigned int port_index, unsigned long value)
{
	set_rstp_port_cfg_value(fd, port_index, value, PT_CFG_PRIO);
	return 0;
}

static int
cli_stp_prt_pcost(int fd, unsigned int port_index, unsigned long value)
{
	set_rstp_port_cfg_value(fd, port_index, value, PT_CFG_COST);
	return 0;
}

static int
cli_stp_prt_mcheck(int fd, unsigned int port_index)
{
	set_rstp_port_cfg_value(fd, port_index, 0, PT_CFG_MCHECK);
	return 0;
}

static int
cli_stp_prt_edge(int fd, int port_index, unsigned long value)
{
	set_rstp_port_cfg_value(fd, port_index, value, PT_CFG_EDGE);
	return 0;
}

static int
cli_stp_prt_non_stp(int fd, int port_index, unsigned long value)
{
	set_rstp_port_cfg_value(fd, port_index, value, PT_CFG_NON_STP);
	return 0;
}

static int
cli_stp_prt_p2p(int fd, int port_index, unsigned long value)
{
	switch (value) {
	case 0:
		value = P2P_FORCE_FALSE;
		break;
	case 1:
		value = P2P_FORCE_TRUE;
		break;
	case 2:
		value = P2P_AUTO;
		break;
	default:
		util_fdprintf(fd, "Invalid value\n");
		return 0;
	}

	set_rstp_port_cfg_value(fd, port_index, (ADMIN_P2P_T) value, PT_CFG_P2P);
	return 0;
}
#endif

static int
cli_stp_dump_stp_ports(int fd)
{
	int i;
	util_fdprintf(fd, "port_id\tclassid\tmeid\tp2pcfg\n");

	for (i = 0; i < number_of_ports; i++) {
	        unsigned short classid=0;
	        unsigned short meid=0;
	        if (stp_port[i].bridge_port_me) {
	                classid = stp_port[i].bridge_port_me->classid;
	                meid = stp_port[i].bridge_port_me->meid;
                }
		
		util_fdprintf(fd, "0x%x\t0x%x\t\t0x%x\t0x%x\n",
			      stp_port[i].logical_port_id, classid, meid, stp_port[i].p2p_cfg);
	}
	return 0;
}

#ifdef STP_DBG
static int
cli_trace(int argc, char **argv)
{
	BITMAP_T ports_bitmap;
	int port_index;

	if ('a' == argv[1][0]) {
		BitmapSetAllBits(&ports_bitmap);
	} else {
		port_index = strtoul(argv[1], 0, 10);
		BitmapClear(&ports_bitmap);
		BitmapSetBit(&ports_bitmap, port_index - 1);
	}

	STP_IN_dbg_set_port_trace(argv[2], argv[3][0] != 'n' && argv[3][0] != 'N', 0, &ports_bitmap, 1);
	return 0;
}

#endif

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_stp_help(int fd)
{
	util_fdprintf(fd, "stp help|[subcmd...]\n");
}

void
cli_stp_help_long(int fd)
{
	util_fdprintf(fd,
		      "stp start\n"
		      "    show bridge\n" "    show port [port_index] (port_index start from 1)\n" "    show stp_port\n");
}

int
cli_stp_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "stp") == 0) {
		if (argc == 1 || (argc == 2 && strcmp(argv[1], "help") == 0)) {
			cli_stp_help_long(fd);
			return 0;
		} else if (argc == 2 && strcmp(argv[1], "start") == 0) {
			if (rstptask_loop_g == 1) {
				util_fdprintf(fd, "stp is running already \n");
			} else {
				rstp_init();
				rstp_start();
			}
			return 0;

		} else if (argc == 2 && strcmp(argv[1], "stop") == 0) {
			if (rstptask_loop_g == 0) {
				util_fdprintf(fd, "rstp did not run\n");
			} else {
				rstp_stop();
				rstp_shutdown();
			}
			return 0;
		} else if (argc == 2 && strcmp(argv[1], "enable") == 0) {
			cli_stp_enable(fd);
		} else if (argc == 2 && strcmp(argv[1], "disable") == 0) {
			cli_stp_disable(fd);
		} else if (argc == 3 && strcmp(argv[1], "show") == 0) {
			if (strcmp(argv[2], "stp_port") == 0) {
				cli_stp_dump_stp_ports(fd);
			} else if (strcmp(argv[2], "bridge") == 0) {
				cli_stp_br_get_cfg(fd);
			} else if (strcmp(argv[2], "port") == 0) {
				cli_stp_pr_get_cfg(fd, 0, 0);
			}
		} else if (argc == 4 && strcmp(argv[1], "show") == 0) {
			if (strcmp(argv[2], "port") == 0) {
				cli_stp_pr_get_cfg(fd, 1, util_atoi(argv[3]));
			}
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}
