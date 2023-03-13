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
 * File    : cli.h
 *
 ******************************************************************/

#ifndef __CLI_H__
#define __CLI_H__

#include "omcimsg.h"
#include "metacfg_adapter.h"

#define CLI_OK				0
#define CLI_EXIT			9	// cli client terminated
#define CLI_ERROR_SYNTAX		-1	// invalid cmd
#define CLI_ERROR_RANGE			-2	// classid/order out of range/unsupported?
#define CLI_ERROR_INTERNAL		-3	// null ptr....
#define CLI_ERROR_READONLY		-4	// modification on readonly data
#define CLI_ERROR_OTHER_CATEGORY	-10	// cmd may belong to other categories

#define MAX_CLI_LINE		1024

/* cli_acl.c */
void cli_vacl_version(int fd);
int cli_vacl_util_init(void);
int cli_vacl_util_clear(void);
void cli_vacl_set_help(int fd);
void cli_vacl_set_act_help(int fd);
void cli_vacl_del_help(int fd);
void cli_vacl_hw_set_help(int fd);
void cli_vacl_help(int fd);
int cli_vacl_cmdline(int fd, int argc, char *argv[]);
/* cli_arp.c */
void cli_arp_help(int fd);
int cli_arp_cmdline(int fd, int argc, char *argv[]);
/* cli_attr.c */
void cli_attr_help(int fd);
int cli_attr_cmdline(int fd, int argc, char *argv[]);
/* cli_bat.c */
void cli_bat_help(int fd);
int cli_bat_cmdline(int fd, int argc, char *argv[]);
/* cli_bridge.c */
int cli_bridge(int fd, unsigned short meid);
void cli_bridge_help(int fd);
int cli_bridge_cmdline(int fd, int argc, char *argv[]);
/* cli.c */
int omci_cli_input(int fd, char *input_line);
/* cli_cfm.c */
void cli_cfm_help(int fd);
void cli_cfm_help_long(int fd);
int cli_cfm_cmdline(int fd, int argc, char *argv[]);
/* cli_classf.c */
int cli_classf_gem(int fd, int is_reset);
int cli_classf_stat(int fd, unsigned char flag, unsigned short vid, unsigned char type, unsigned char dir);
void cli_classf_help(int fd);
int cli_classf_cmdline(int fd, int argc, char *argv[]);
/* cli_config.c */
void cli_config_help(int fd);
int cli_config_cmdline(int fd, int argc, char *argv[]);
/* cli_cpuport.c */
void cli_cpuport_history_help(int fd);
void cli_cpuport_help(int fd);
int cli_cpuport_cmdline(int fd, int argc, char *argv[]);
/* cli_dhcp.c */
void cli_dhcp_help(int fd);
int cli_dhcp_cmdline(int fd, int argc, char *argv[]);
/* cli_env.c */
void cli_env_help(int fd);
int cli_env_cmdline(int fd, int argc, char *argv[]);
/* cli_er.c */
void cli_er_help(int fd);
int cli_er_cmdline(int fd, int argc, char *argv[]);
/* cli_extoam.c */
void cli_extoam_long_help(int fd);
void cli_extoam_help(int fd);
int cli_extoam_cmdline(int fd, int argc, char *argv[]);
/* cli_gfast.c */
void cli_gfast_help(int fd);
int cli_gfast_cmdline(int fd, int argc, char *argv[]);
/* cli_gpon.c */
void cli_gpon_help(int fd);
int cli_gpon_cmdline(int fd, int argc, char *argv[]);
/* cli_history.c */
int cli_history_init(void);
int cli_history_shutdown(void);
int cli_history_add_msg(struct omcimsg_layout_t *msg);
void cli_history_help(int fd);
void cli_history_help_long(int fd);
int cli_history_cmdline(int fd, int argc, char *argv[]);
/* cli_hwresource.c */
void cli_hwresource_help(int fd);
int cli_hwresource_cmdline(int fd, int argc, char *argv[]);
/* cli_igmp.c */
void cli_igmp_help(int fd);
int cli_igmp_cmdline(int fd, int argc, char *argv[]);
/* cli_lldp.c */
int lldp_print_neighbor(int fd);
void cli_lldp_help(int fd);
void cli_lldp_help_long(int fd);
int cli_lldp_cmdline(int fd, int argc, char *argv[]);
/* cli_me.c */
int cli_me(int fd, unsigned short classid, unsigned short meid, unsigned char attr_order);
int cli_me_related_classid(int fd, unsigned short classid);
int cli_me_related(int fd, unsigned short classid, unsigned short classid2);
void cli_me_mibreset_dump(void);
void cli_me_help(int fd);
int cli_me_cmdline(int fd, int argc, char *argv[]);
/* cli_misc.c */
int cli_access_clear(void);
int cli_anig(int fd, int anig_type);
int cli_rf(int fd);
int cli_poe(int fd);
int cli_ping(int fd, char *srcip_str, char *dstip_str, int count);
void cli_misc_help(int fd);
int cli_misc_before_cmdline(int fd, int argc, char *argv[]);
void cli_misc_after_help(int fd);
int cli_misc_after_cmdline(int fd, int argc, char *argv[]);
/* cli_nat.c */
void cli_nat_help(int fd);
void cli_nat_help_long(int fd);
int cli_nat_cmdline(int fd, int argc, char *argv[]);
/* cli_portinfo.c */
void cli_portinfo_help(int fd);
int cli_portinfo_cmdline(int fd, int argc, char *argv[]);
/* cli_pots.c */
int cli_pots(int fd, unsigned short meid);
int cli_pots_cmdline(int fd, int argc, char *argv[]);
/* cli_pots_mkey.c */
#ifdef OMCI_METAFILE_ENABLE
int pots_mkey(unsigned short meid, struct metacfg_t *kv, int is_sort);
#endif
int cli_pots_mkey_me_153(unsigned short meid);
int cli_pots_mkey_cmdline(int fd, int argc, char *argv[]);
/* cli_session.c */
int cli_session_list_init(void);
int cli_session_list_shutdown(void);
int cli_session_add(char *session_name, unsigned int session_pid);
int cli_session_del(char *session_name, unsigned int session_pid);
int cli_session_list_print(int fd);
/* cli_spec.c */
void cli_spec_help(int fd);
int cli_spec_cmdline(int fd, int argc, char *argv[]);
/* cli_stp.c */
void cli_stp_help(int fd);
void cli_stp_help_long(int fd);
int cli_stp_cmdline(int fd, int argc, char *argv[]);
/* cli_switch.c */
void cli_switch_help(int fd);
void cli_switch_help_long(int fd);
int cli_switch_cmdline(int fd, int argc, char *argv[]);
/* cli_tag.c */
void cli_tag_help(int fd);
int cli_tag_cmdline(int fd, int argc, char *argv[]);
/* cli_tcont.c */
int cli_tcont(int fd, unsigned short meid, int show_active_only);
int cli_tcont_cmdline(int fd, int argc, char *argv[]);
/* cli_unrelated.c */
int cli_unrelated(int fd);
void cli_unrelated_help(int fd);
int cli_unrelated_cmdline(int fd, int argc, char *argv[]);
/* cli_version.c */
char *omci_cli_version_str(void);

#endif
