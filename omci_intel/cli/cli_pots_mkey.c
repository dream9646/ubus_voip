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
 * File    : cli_pots_mkey.c
 *
 ******************************************************************/
/******************************************************************
POTS    : Plain Old Telephone Service
me 53 	9.9.1	Physical path termination point POTS UNI
me 58	9.9.6	Voice service profile
me 136	9.4.3	TCP/UDP config data
me 137	9.12.3	Network address
me 138 	9.9.18	VoIP config data
me 139	9.9.4	VoIP voice CTP
me 142	9.9.5	VoIP media profile
me 143	9.9.7	RTP profile data
me 145	9.9.10	Network dial plan table
me 146	9.9.8	VoIP application service profile
me 147	9.9.9	VoIP feature access codes
me 148	9.12.4	Authentication security method
me 150 	9.9.3	SIP agent config data
me 153 	9.9.2	SIP user data
me 243		240-255 Reserved for vendor-specific managed entities
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "cli.h"
#include "me_related.h"
#include "metacfg_adapter.h"
#include "iphost.h"
#include "batchtab_cb.h"

#define NO_SUPPORT "No_Support"
#define TODO "todo"
#define VALUE_FLAHHOOKTIMEOUT_MAX 2200
#define VALUE_FLAHHOOKTIMEOUT_MIN 50
#define DIGITMAP_RULE_MAX 100
#define PORT_NUM 2
#define PROXY_SERVER_PORT_DEFAULT 5060

struct meta_diff_s {
	struct list_head list;
	char *key;
	char *value;
};

#ifdef OMCI_METAFILE_ENABLE
static unsigned short port_id_g;
struct metacfg_t kv_metafile_g, *kv_metadiff_g;
static int meta_diff_g=0;
static unsigned char ep_enable_ary_g[PORT_NUM]={0,0};
static unsigned char c_sip_mode_g=0;
#endif
static unsigned char me_153_created[PORT_NUM]={0,0};
unsigned short TCP_UDP_config_data_port_id_g[PORT_NUM];
char registrar_g[PORT_NUM][375], proxy_g[PORT_NUM][375], obproxy_g[PORT_NUM][375];
LIST_HEAD(meta_diff_list_g);
#ifdef OMCI_METAFILE_ENABLE

static char *
int2str(unsigned char type, void *data)
{
	static char buff_wheel[4][32];
	static int index = 0;
	char *buff = buff_wheel[(index++)%4];
	if (type == 1) {
		// unsigned char
		snprintf(buff, 31, "%u", *(unsigned char *)data);
	} else if (type == 2) {
		// unsigned short
		snprintf(buff, 31, "%u", *(unsigned short *)data);
	} else if (type == 3) {
		// char
		snprintf(buff, 31, "%d", *(char *)data);
	} else if (type == 4) {
		// unsigned int
		snprintf(buff, 31, "%u", *(unsigned int *)data);
	} else {
		snprintf(buff, 31, "%d", *(int *)data);
	}
	return buff;
}

static void print_local (int fd, char *str, char *format)
{
	char mkey[128];
	if (meta_diff_g == 1) {
		return;
	}
	if (format != NULL && strlen(format)) {
		if (strchr(format, '%') != NULL) {
			snprintf(mkey, 128, format, port_id_g);
		} else {
			strncpy(mkey, format, 127);
		}
		if (strstr(format, "fvcli") != NULL ||
		    strchr(format, '[') != NULL ||
		    strstr(format, NO_SUPPORT) != NULL ||
		    strstr(format, TODO) != NULL) {
			util_fdprintf(fd, "%s%s", str, mkey);
		} else {
			util_fdprintf(fd, "%s%s[%s]", str, mkey, metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
		}
	} else {
		util_fdprintf(fd, "%s", str);
	}
}

static void print_tab_local (int fd, char *str, int level, char *format)
{
	char mkey[128];
	char tab_str[32];
	if (meta_diff_g == 1) {
		return;
	}
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';
	tab_str[31]='\0';
	snprintf(mkey, 128, "\n%s%s", tab_str, str);
	print_local (fd, mkey, format);
}

static void print_tab_text (int fd, char *str, int level, char *text)
{
	char tab_str[32];
	if (meta_diff_g == 1) {
		return;
	}
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';
	tab_str[31]='\0';
	util_fdprintf(fd, "\n%s%s %s", tab_str, str, text);
}

static void
print_tab_mvalue (int fd, char *str, int level, char *attr_raw_data, char *attr_meta_value, char *mkey_format)
{
	char mkey[128];
	char tab_str[32];

	memset(tab_str, '\t', 31);
	tab_str[level]='\0';
	tab_str[31]='\0';

	if (mkey_format != NULL && strlen(mkey_format)) {
		if (strchr(mkey_format, '%') != NULL) {
			snprintf(mkey, 128, mkey_format, port_id_g);
		} else {
			strncpy(mkey, mkey_format, 127);
		}
		if (strstr(mkey_format, "fvcli") != NULL ||
		    strchr(mkey_format, '[') != NULL) {
			if (meta_diff_g==0) {
				util_fdprintf(fd, "\n%s%s%s", tab_str, str, mkey);
			}
		} else {
			if (meta_diff_g==0) {
				util_fdprintf(fd, "\n%s%s[%s]%s=[%s][%s]", tab_str, str, attr_raw_data, mkey, attr_meta_value, metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			} else if (0 != strcmp(mkey, NO_SUPPORT) && attr_meta_value != NULL && 0 != strcmp(attr_meta_value, "SKIP")) {
				if (NULL == metacfg_adapter_config_entity_locate(kv_metadiff_g, mkey)) {
					dbprintf(LOG_INFO, "kv_metadiff_g added [%s]=[%s]\n", mkey, attr_meta_value);
					metacfg_adapter_config_entity_add(kv_metadiff_g, mkey, attr_meta_value, 1);
				} else if (0 == strcmp("voip_general_digitmap_syntax", mkey)) {
					char *p=NULL, *q=NULL;
					if (NULL == strstr(metacfg_adapter_config_get_value(kv_metadiff_g, mkey, 1), attr_meta_value)) {
						p=metacfg_adapter_config_get_value(kv_metadiff_g, mkey, 1);
						q=(char *)malloc_safe(strlen(p)+strlen(attr_meta_value)+2);
						if (q != NULL) {
							sprintf(q, "%s%s", p, attr_meta_value);
							metacfg_adapter_config_entity_update(kv_metadiff_g, mkey, q);
							free_safe(q);
						}
					}
				} else if (0 != strcmp(attr_meta_value, metacfg_adapter_config_get_value(kv_metadiff_g, mkey, 1))){
					util_fdprintf(fd, "\nduplicate [%s]:[%s]=>[%s]", mkey,metacfg_adapter_config_get_value(kv_metadiff_g, mkey, 1), attr_meta_value);
				}
				/* util_fdprintf(fd, "\n%s=%s", mkey, attr_meta_value); */
			}
		}
	} else {
		if (meta_diff_g==0) {
			util_fdprintf(fd, "\n%s%s[%s]%s=[%s][%s]", tab_str, str, attr_raw_data, "Comment", attr_meta_value, "");
		}
	}
}

static void
print_tab_mvalue_meid (int fd, char *str, int level, int meid)
{
	char tab_str[32];
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';
	tab_str[31]='\0';
	if (meta_diff_g==0) {
		if (meid < 0) {
			util_fdprintf(fd, "\n%s%s", tab_str, str);
		} else {
			util_fdprintf(fd, "\n%s%s @0x%x", tab_str, str, meid);
		}
	}
}

static inline void
print_tab_mvalue_skip (int fd, char *str, int level, char *mkey_format)
{
	if (meta_diff_g == 0) {
		print_tab_mvalue(fd, str, level, "NULL", "SKIP", mkey_format);
	}
}

static inline void
print_tab_mvalue_no_support (int fd, char *str, int level)
{
	if (meta_diff_g == 0) {
		print_tab_mvalue(fd, str, level, "NULL", "SKIP", NO_SUPPORT);
	}
}

static inline char * get_dialplan_format_str(int number)
{
	static char *format_str[4]={ "undefined", "h.248", "nsc", "vendor-specific" };
	if (number>=0 && number<4)
		return format_str[number];
	return format_str[0];
}

static int
print_mkey_me_145(int fd)
{
	print_tab_mvalue_skip(fd, "1 Dial plan number: ", 3, "fvcli get digitmapnum %u");
	print_tab_mvalue_skip(fd, "2 Dial plan table max size: ", 3, "fvcli get digitmapmaxnum %u");
	print_tab_mvalue_skip(fd, "3 Critical dial timeout: ", 3, "voip_ep%u_digit_start_timeout");
	print_tab_mvalue_skip(fd, "4 Partial dial timeout: ", 3, "voip_ep%u_dialtone_timeout");
	print_tab_mvalue_skip(fd, "5 Dial plan format: ", 3, "3");
	print_tab_mvalue_skip(fd, "6 Dial plan table: ", 3, "voip_ep%u_digitmap_1");
	print_tab_mvalue_skip(fd, "                 : ", 3, "voip_ep%u_digitmap_20");
	print_tab_mvalue_skip(fd, "                 : ", 3, "voip_general_digitmap_syntax");
	return CLI_OK;
}

static int
cli_sip_user_related_dialplan_145(int fd, struct me_t *me_sip_user)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(145);
	struct me_t *me=NULL;
	int l, idx, i, n, flag=0;
	unsigned short us, us2;
	unsigned char uc;
	char buff[1024], *array[8], *arrayp128[128], *p;

	if (me_sip_user == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sip_user, me)) {
			flag=1;
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, ">[145]dialplan", 2, me->meid);
			us=(unsigned short)meinfo_util_me_attr_data(me, 1);
			print_tab_mvalue(fd, "1 Dial plan number: ", 3, int2str(2, &us), "", "fvcli get digitmapnum %u");
			us=(unsigned short)meinfo_util_me_attr_data(me, 2);
			print_tab_mvalue(fd, "2 Dial plan table max size: ", 3, int2str(2, &us), "", "fvcli get digitmapmaxnum %u");
			if (0 == (us=(unsigned short)meinfo_util_me_attr_data(me, 3))) {
				us=16000;
			}
			print_tab_mvalue(fd, "3 Critical dial timeout: ", 3, int2str(2, &us), int2str(2, &us), "voip_ep%u_digit_start_timeout");
			if (0 == (us=(unsigned short)meinfo_util_me_attr_data(me, 4))) {
				us2=16;
			} else {
				us2=(unsigned short)(us/1000);
			}
			print_tab_mvalue(fd, "4 Partial dial timeout: ", 3, int2str(2, &us), int2str(2, &us2), "voip_ep%u_dialtone_timeout");
			uc=(unsigned char)meinfo_util_me_attr_data(me, 5);
			if (uc > 4) {
				uc=0;
			}
			print_tab_mvalue(fd, "5 Dial plan format: ", 3, int2str(1, &uc), int2str(1, &uc), "voip_general_digitmap_syntax");
			struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, 6);
			idx=1;
			if (aiptr->format == ATTR_FORMAT_TABLE) {
				struct attr_table_header_t *tab_header = me->attr[6].value.ptr;
				struct attr_table_entry_t *tab_entry;
				if (tab_header) {
					int flag_alu;
					char *digitmap;
					flag_alu=0;
					if (0 == strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) &&
					    omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
						flag_alu=1;
						digitmap = (char *)malloc_safe(1024);
						*digitmap='\0';
					}
					list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
						if (tab_entry->entry_data_ptr != NULL) {
							struct attr_value_t attr;
							attr.data=0;
							attr.ptr=tab_entry->entry_data_ptr;	// use attr to encapsulate entrydata for temporary use
							buff[0]='\0';
							meinfo_conv_attr_to_string(miptr->classid, 6, &attr, buff, 1024);
							if (0 < (l=strlen(buff))) {
								p = (char *)malloc_safe(l+1);
								snprintf(p, l+1, "%s", buff);
							} else {
								p = (char *)malloc_safe(1);
								*p = '\0';
							}
							n=util_string2array(p, ", ", array, 8);
							print_tab_local(fd, "6 Dial_plan_table: ", 3, buff);

							if (array[0]) {
								print_tab_text(fd, "6.0 P Dial_plan_id: ", 4, array[0]);
							}
							if (array[1]) {
								print_tab_mvalue(fd, "6.1 Action: ", 4, array[1], array[1], NO_SUPPORT);
							}
							if (array[2]) {
								print_tab_text(fd, "6.2 Dial_plan_token: ", 4, array[2]);
								if (flag_alu) {
									util_string_concatnate(&digitmap, array[2], 1024);
								} else {
									n=util_string2array(array[2], "|", arrayp128, 128);
									for (i=0; i<n; i++) {
										snprintf(buff, 128, "voip_ep%%u_digitmap_%d", idx++);
										print_tab_mvalue(fd, "                   : ", 4, arrayp128[i], arrayp128[i], buff);
									}
								}
							}
							free_safe(p);
						}
					}
					if (flag_alu) {
						dbprintf(LOG_ERR, "(%u)digitmap string[%s]\n", port_id_g, digitmap);
						n=util_string2array(digitmap, "|", arrayp128, 128);
						idx=1;
						for (i=0; i<n; i++) {
							snprintf(buff, 128, "voip_ep%%u_digitmap_%d", idx++);
							print_tab_mvalue(fd, "                   : ", 4, arrayp128[i], arrayp128[i], buff);
						}
						free_safe(digitmap);
					}
				}
			}
		}
	}
	// give a default digitmap (^x.t) if olt doesn't provision anything
	if(list_empty(&miptr->me_instance_list)) {
		idx = 1;
		snprintf(buff, 128, "voip_ep%%u_digitmap_%d", idx++);
		print_tab_mvalue(fd, "                   : ", 4, "^x.t", "^x.t", buff);
	}
	// remove other digitmap rules
	for (i=idx; i<=DIGITMAP_RULE_MAX; i++) {
		snprintf(buff, 128, "voip_ep%%u_digitmap_%d", i);
		print_tab_mvalue(fd, "                   : ", 4, "", "", buff);
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[145]dialplan:", 2, -1);
		print_mkey_me_145(fd);
	}
	return CLI_OK;
}

static int
print_mkey_me_146(int fd)
{
	print_tab_mvalue_skip(fd, "1 CID_features: ", 3, NULL);
	print_tab_mvalue_no_support(fd, "1.0 Reserved: ", 4);
	print_tab_mvalue_skip(fd, "1.1 Anonymous_CID_blocking: ", 4, "voip_ep%u_cid_blocking_option");
	print_tab_mvalue_no_support(fd, "1.2 CID_name: ", 4);
	print_tab_mvalue_no_support(fd, "1.3 CID_number: ", 4);
	print_tab_mvalue_skip(fd, "1.4 CID_blacking: ", 4, "voip_ep%u_cid_blocking_option");
	print_tab_mvalue_skip(fd, "1.5 Calling_name: ", 4, "voip_ep%u_cid_data_msg_format");
	print_tab_mvalue_skip(fd, "1.6 Calling_number: ", 4, "voip_ep%u_cid_type");
	print_tab_mvalue_skip(fd, "2 Call_waiting_features: ", 3, NULL);
	print_tab_mvalue_no_support(fd, "2.0 Reserved: ", 4);
	print_tab_mvalue_skip(fd, "2.1 Caller_ID_announcement: ", 4, "voip_ep%u_cid_type");
	print_tab_mvalue_skip(fd, "2.2 Call_waiting: ", 4, "voip_ep%u_call_waiting_option");
	print_tab_mvalue_skip(fd, "3 Call_progress_or_transfer_features: ", 3, NULL);
	print_tab_mvalue_no_support(fd, "3.0 Reserved: ", 4);
	print_tab_mvalue_no_support(fd, "3.1 6way: ", 4);
	print_tab_mvalue_no_support(fd, "3.2 Emergency_service_originating_hold: ", 4);
	print_tab_mvalue_skip(fd, "3.3 Flash_on_emergency_service_call: ", 4,  "voip_ep%u_flashhook_option");
	print_tab_mvalue_skip(fd, "3.4 Do_not_disturb: ", 4, "voip_ep%u_dnd_option");
	print_tab_mvalue_no_support(fd, "3.5 Call_park: ", 4);
	print_tab_mvalue_no_support(fd, "3.6 Call_hold: ", 4);
	print_tab_mvalue_skip(fd, "3.7 Call_transfer: ", 4, "voip_ep%u_transfer_option");
	print_tab_mvalue_skip(fd, "3.8 3way: ", 4, "voip_ep%u_threeway_option");
	print_tab_mvalue_skip(fd, "4 Call_presentation_features: ", 3, "voip_ep%u_mwi_type");
	print_tab_mvalue_skip(fd, "4.0 Reserved: ", 4, NULL);
	print_tab_mvalue_skip(fd, "4.1 Call_forwarding_indication: ", 4, NULL);
	print_tab_mvalue_skip(fd, "4.2 Message_waiting_indication_visual_indication: ", 4, NULL);
	print_tab_mvalue_skip(fd, "4.3 Message_waiting_indication_special_dial_tone: ", 4, NULL);
	print_tab_mvalue_skip(fd, "4.4 Message_waiting_indication_splash_ring: ", 4, NULL);
	print_tab_mvalue_skip(fd, "5 Direct_connect_feature: ", 3, "voip_ep%u_hotline_option");
	print_tab_mvalue_skip(fd, "5.0 Reserved: ", 4, NULL);
	print_tab_mvalue_skip(fd, "5.1 Dial_tone_feature_delay_option: ", 4, NULL);
	print_tab_mvalue_skip(fd, "5.2 Direct_connect_feature_enable: ", 4, NULL);
	print_tab_mvalue_skip(fd, "6 Direct_connect_URI_pointer [classid 137]: ", 3, "voip_ep%u_hotline_option");
	print_tab_mvalue_skip(fd, "                                          : ", 3, "voip_ep%u_hotline_delay_time");
	print_tab_mvalue_no_support(fd, "7 Bridged_line_agent_URI_pointer [classid 137]: ", 3);
	print_tab_mvalue_no_support(fd, "8 Conference_factory_URI_pointer [classid 137]: ", 3);
	print_tab_mvalue_skip(fd, "9 Dial_tone_feature_delay_Warmline_timer: ", 3, "voip_ep%u_hotline_delay_time");
	return CLI_OK;
}

static int
cli_sip_user_related_voip_app_service_profile_146_direct_connect (int fd, struct me_t *me_sip_agent, int level)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	int flag=0, meid, ret_delay=0;
	char tab_str[32];
	char registar_addr[375];
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';

	if (me_sip_agent == NULL) {
		goto exit;
	}
	struct me_t *me=meinfo_me_get_by_meid(137, meinfo_util_me_attr_data(me_sip_agent, 6));
	if (me) {
		print_tab_mvalue_meid(fd, ">[137]Network_address", level, me->meid);
		struct me_t *me_registar_addr=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 2));
		meinfo_me_refresh(me, get_attr_mask);

		if (NULL == me_registar_addr) {
			meid=-1;
		} else {
			meid=me_registar_addr->meid;
		}
		print_tab_mvalue_meid(fd, ">[157]2 Address_pointer:", level+1, meid);
		if (me_registar_addr) {
			unsigned char i, parts = (unsigned char) meinfo_util_me_attr_data(me_registar_addr, 1);
			memset(registar_addr, 0, sizeof(registar_addr));
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_registar_addr, 2+i, &attr);
				strcat(registar_addr, str);
			}
			if (parts) {
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
					char *p=strchr(registar_addr, '!');
					if (p) {
						*p='\0';
						print_tab_mvalue(fd, "Direct connect URI: ", level+2, registar_addr, registar_addr, "voip_ep%u_hotline_uri");
						print_tab_mvalue(fd, "Warmline timer: ", level+2, p+1, p+1, "voip_ep%u_hotline_delay_time");
						ret_delay=1;
					} else {
						print_tab_mvalue(fd, "Direct connect URI: ", level+2, registar_addr, registar_addr, "voip_ep%u_hotline_uri");
					}
				} else {
					print_tab_mvalue(fd, "Direct connect URI: ", level+2, registar_addr, registar_addr, "voip_ep%u_hotline_uri");
				}
			}
		}
		flag=1;
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[137]Network_address", level, -1);
		print_tab_mvalue_meid(fd, ">[157]2 Address_pointer:", level+1, -1);
	}
	return ret_delay;
}

static int
cli_sip_user_related_voip_app_service_profile_146(int fd, struct me_t *me_sipuser)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(146);
	struct me_t *me=NULL;
	int flag=0;
	unsigned char c, uc, uc2, *uc_p;
	unsigned short us, us2;
	unsigned int ui;
	char mkey[128];

	if (me_sipuser == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sipuser, me)) {
			flag=1;
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, ">[146]voip_app_service_profile", 2, me->meid);
			c=*(unsigned char*)meinfo_util_me_attr_ptr(me, 1);
			print_tab_mvalue_meid(fd, "1 CID_features: ", 3, c);
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 0, 2);
			print_tab_mvalue(fd, "1.0 Reserved: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			uc=(unsigned char)util_bitmap_get_value(&c, 8, 2, 1);
			snprintf(mkey, 127, "voip_ep%u_cid_blocking_option", port_id_g);
			ui=util_atoi(metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			if (uc) {
				ui |= 0x02;
			} else {
				ui &= ~0x02;
			}
			print_tab_mvalue(fd, "1.1 Anonymous_CID_blocking: ", 4, int2str(1, &uc), int2str(4, &ui), "voip_ep%u_cid_blocking_option");
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 3, 1);
			print_tab_mvalue(fd, "1.2 CID_name: ", 4, int2str(1, &uc), "", NO_SUPPORT);
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 4, 1);
			print_tab_mvalue(fd, "1.3 CID_number: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			uc=(unsigned char)util_bitmap_get_value(&c, 8, 5, 1);
			snprintf(mkey, 127, "voip_ep%u_cid_blocking_option", port_id_g);
			ui=util_atoi(metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			if (uc) {
				ui |= 0x01;
			} else {
				ui &= ~0x01;
			}
			print_tab_mvalue(fd, "1.4 CID_blocking: ", 4, int2str(1, &uc), int2str(4, &ui), "voip_ep%u_cid_blocking_option");

			uc=(unsigned char)util_bitmap_get_value(&c, 8, 6, 1);
			print_tab_mvalue(fd, "1.5 Calling_name: ", 4, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_cid_data_msg_format");
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 7, 1);
			if (uc == 0) {
				uc2=0;
			} else {
				uc2=3;
			}
			print_tab_mvalue(fd, "1.6 Calling_number: ", 4, int2str(1, &uc), int2str(1, &uc2), "voip_ep%u_cid_type");

			c=*(unsigned char*)meinfo_util_me_attr_ptr(me, 2);
			print_tab_mvalue_meid(fd, "2 Call_waiting_features: ", 3, c);
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 0, 6);
			print_tab_mvalue(fd, "2.0 Reserved: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			uc=(unsigned char)util_bitmap_get_value(&c, 8, 6, 1);
			snprintf(mkey, 127, "voip_ep%u_cid_type", port_id_g);
			ui=util_atoi(metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			if (uc) {
				ui |= 0x02;
			} else {
				ui &= ~0x02;
			}
			print_tab_mvalue(fd, "2.1: Caller_ID_announcement: ", 4, int2str(1, &uc), int2str(4, &ui), "voip_ep%u_cid_type");

			uc=(unsigned char)util_bitmap_get_value(&c, 8, 7, 1);
			print_tab_mvalue(fd, "2.2: Call_waiting: ", 4, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_call_waiting_option");

			uc_p=(unsigned char *)meinfo_util_me_attr_ptr(me, 3);
			us=(unsigned short)*uc_p;
			print_tab_mvalue_meid(fd, "3 Call_progress_or_transfer_features: ", 3, us);
			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 0, 8);
			print_tab_mvalue(fd, "3.0 Reserved: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 10, 1);
			snprintf(mkey, 128, "voip_ep%u_flashhook_option", port_id_g);
			ui=util_atoi(metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			if (uc == 0) {
				ui |= 0x4;
			} else {
				ui &= ~0x4;
			}
			if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) &&
				(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX))
				print_tab_mvalue(fd, "3.3: Flash_on_emergency_service_call:", 4, int2str(1, &uc), int2str(4, &ui), "voip_ep%u_flashhook_option");

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 11, 1);
			// always disable "do not disturb" feature for adtran since olt always enable it and cause can't make incoming call
			if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ADTRAN) == 0) &&
				(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ADTRAN))
				print_tab_mvalue(fd, "3.4: Do_not_disturb: ", 4, "0", "0", "voip_ep%u_dnd_option");
			else
				print_tab_mvalue(fd, "3.4: Do_not_disturb: ", 4, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_dnd_option");

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 14, 1);
			if (uc == 0) {
				uc2=0;
			} else {
				uc2=7;
			}
			print_tab_mvalue(fd, "3.7: Call_transfer: ", 4, int2str(1, &uc), int2str(1, &uc2), "voip_ep%u_transfer_option");

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 15, 1);
			print_tab_mvalue(fd, "3.8: 3way: ", 4, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_threeway_option");

			uc_p=(unsigned char *)meinfo_util_me_attr_ptr(me, 4);
			us=(unsigned short)*uc_p;
			print_tab_mvalue_meid(fd, "4 Call_presentation_features: ", 3, us);
			us2=(unsigned short)util_bitmap_get_value(uc_p, 8*2, 0, 11);
			print_tab_mvalue(fd, "4.0 Reserved: ", 4, int2str(2, &us2), "", NO_SUPPORT);

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 11, 1);
			print_tab_mvalue(fd, "4.1: Call_forwarding_indication: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			snprintf(mkey, 128, "voip_ep%u_mwi_type", port_id_g);
			ui=util_atoi(metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 14, 1);
			uc2=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 11, 1);
			if (uc) {
				ui |= 0x2;
			} else {
				ui &= ~0x2;
			}
			if (uc2) {
				ui |= 0x4;
			} else {
				ui &= ~0x4;
			}
			if (uc || uc2) {
				ui |= 0x1;
			} else {
				ui &= ~0x1;
			}

			print_tab_mvalue(fd, "4.2: Message_waiting_indication_visual_indication: ", 4, int2str(1, &uc), int2str(4, &ui), "voip_ep%u_mwi_type");
			print_tab_mvalue(fd, "4.3: Message_waiting_indication_special_dial_tone: ", 4, int2str(1, &uc2), int2str(4, &ui), "voip_ep%u_mwi_type");

			uc=(unsigned char)util_bitmap_get_value(uc_p, 8*2, 15, 1);
			print_tab_mvalue(fd, "4.4: Message_waiting_indication_splash_ring: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			c=*(unsigned char*)meinfo_util_me_attr_ptr(me, 5);
			print_tab_mvalue_meid(fd, "5: Direct_connect_feature: ", 3, c);
			uc=(unsigned char)util_bitmap_get_value(&c, 8, 0, 6);
			print_tab_mvalue(fd, "5.0 Reserved: ", 4, int2str(1, &uc), "", NO_SUPPORT);

			unsigned char uc_dial_tone_feature_delay_option = (unsigned char)util_bitmap_get_value(&c, 8, 6, 1);
			unsigned char uc_direct_connect_feature_enable = (unsigned char)util_bitmap_get_value(&c, 8, 7, 1);

			us=(unsigned short)meinfo_util_me_attr_data(me, 6);
			print_tab_mvalue_meid(fd, "6: Direct_connect_URI_pointer (2byte,pointer,RWS) [classid 137]: ", 3, us);
			uc = cli_sip_user_related_voip_app_service_profile_146_direct_connect(fd, me, 4);
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
				uc_dial_tone_feature_delay_option = uc;
			}
			if (uc_dial_tone_feature_delay_option == 0 && uc_direct_connect_feature_enable == 0) {
				uc2=0;
			} else if (uc_dial_tone_feature_delay_option == 0 && uc_direct_connect_feature_enable == 1) {
				uc2=1;
			} else {
				uc2=2;
			}
			print_tab_mvalue(fd, "5.1,2: delay direct_connect: ", 4, int2str(1, &uc2), int2str(1, &uc2), "voip_ep%u_hotline_option");

			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0) {
				uc_p=(unsigned char *)meinfo_util_me_attr_ptr(me, 9);
				if(uc_p) {
					us=(unsigned short)*uc_p;
					print_tab_mvalue(fd, "9: Dial_tone_feature_delay_Warmline_timer: ", 3, int2str(2, &us), int2str(2, &us), "voip_ep%u_hotline_delay_time");
				}
			}
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[146]voip_app_service_profile:", 2, -1);
		print_mkey_me_146(fd);
	}
	return CLI_OK;
}

static int
print_mkey_me_147(int fd)
{
	print_tab_mvalue_no_support(fd, "1 Cancel call waiting: ", 3);
	print_tab_mvalue_no_support(fd, "2 Call hold: ", 3);
	print_tab_mvalue_no_support(fd, "3 Call park: ", 3);
	print_tab_mvalue_no_support(fd, "4 Caller ID activate: ", 3);
	print_tab_mvalue_no_support(fd, "5 Caller ID deactivate: ", 3);
	print_tab_mvalue_no_support(fd, "6 Do not disturb activation: ", 3);
	print_tab_mvalue_no_support(fd, "7 Do not disturb deactivation: ", 3);
	print_tab_mvalue_no_support(fd, "8 Do not disturb PIN change: ", 3);
	print_tab_mvalue_no_support(fd, "9 Emergency service number: ", 3);
	print_tab_mvalue_no_support(fd, "10 Intercom service: ", 3);
	print_tab_mvalue_no_support(fd, "11 Unattended/blind call transfer: ", 3);
	print_tab_mvalue_no_support(fd, "12 Attended call transfer: ", 3);
	return 0;
}

static int
cli_sip_user_related_voip_feature_access_codes_147(int fd, struct me_t *me_sipuser)
{
	struct meinfo_t *miptr=meinfo_util_miptr(147);
	struct me_t *me=NULL;
	int flag=0;
	if (me_sipuser == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sipuser, me)) {
			flag=1;
			print_tab_mvalue_meid(fd, ">[147]voip_feature_access_codes", 2, me->meid);
			print_mkey_me_147(fd);
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[147]voip_feature_access_codes:", 2, -1);
		print_mkey_me_147(fd);
	}
	return CLI_OK;
}

#if 0
/* 134
1: IP_options (1byte,unsigned integer,RW)
2: MAC_address (6byte,string-mac,R)
3: Ont_identifier (25byte,string,RW)
4: IP_address (4byte,unsigned integer-ipv4,RW)
5: Mask (4byte,unsigned integer-ipv4,RW)
6: Gateway (4byte,unsigned integer-ipv4,RW)
7: Primary_DNS (4byte,unsigned integer-ipv4,RW)
8: Secondary_DNS (4byte,unsigned integer-ipv4,RW)
9: Current_address (4byte,unsigned integer-ipv4,R)
10: Current_mask (4byte,unsigned integer-ipv4,R)
11: Current_gateway (4byte,unsigned integer-ipv4,R)
12: Current_primary_DNS (4byte,unsigned integer-ipv4,R)
13: Current_secondary_DNS (4byte,unsigned integer-ipv4,R)
14: Domain_name (25byte,string,R)
15: Host_name (25byte,string,R)
*/
static int
cli_tcpudp_related_iphost(int fd, struct me_t *me_tcpudp)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(134);
	struct me_t *me=NULL;

	if (me_tcpudp == NULL) {
		print_tab_mvalue_meid(fd, ">[134]iphost:", 4, -1);
		return 0;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_tcpudp, me)) {
			char *devname = meinfo_util_get_config_devname(me);
			struct in_addr in_static;
			struct in_addr in_dhcp;
			meinfo_me_refresh(me, get_attr_mask);
			in_static.s_addr=htonl(meinfo_util_me_attr_data(me, 4));
			in_dhcp.s_addr=htonl(meinfo_util_me_attr_data(me, 9));

			print_tab_mvalue_meid(fd, ">[134]iphost", 4, me->meid);
			util_fdprintf(fd, ",dev=%s", devname?devname:"");
			util_fdprintf(fd, ",static_ip=%s", inet_ntoa(in_static));
			util_fdprintf(fd, ",dhcp_ip=%s", inet_ntoa(in_dhcp));
		}
	}
	return CLI_OK;
}
#endif

static int
cli_sip_user_related_auth_username_password(int fd, struct me_t *me, int level)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	char tab_str[32], user[64], pass[64];
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';
	tab_str[31]='\0';
	user[0] = 0;
	pass[0] = 0;
	if (me != NULL) {
		struct attr_value_t attr1, attr2;
		unsigned char buf1[26], buf2[26];
		print_tab_mvalue_meid(fd, ">[148]Authentication_security_method", level, me->meid);
		meinfo_me_refresh(me, get_attr_mask);
		memset(buf1, 0x00, sizeof(buf1));
		attr1.ptr = buf1;
		meinfo_me_attr_get(me, 2, &attr1);
		memset(buf2, 0x00, sizeof(buf2));
		attr2.ptr = buf2;
		meinfo_me_attr_get(me, 5, &attr2);
		if(strlen(buf1)==25 && strlen(buf1) != 0)
			snprintf(user, 64,  "%s%s", buf1, buf2);
		else
			snprintf(user, 64,  "%s", buf1);
		memset(buf1, 0x00, sizeof(buf1));
		attr1.ptr = buf1;
		meinfo_me_attr_get(me, 3, &attr1);
		snprintf(pass, 64,  "%s", buf1);
	} else {
		print_tab_mvalue_meid(fd, ">[148]Authentication_security_method", level, -1);
	}
	print_tab_mvalue(fd, "2 Username: ", level+1, user, user, "voip_ep%u_proxy_user_name");
	print_tab_mvalue(fd, "3 Password: ", level+1, pass, pass, "voip_ep%u_proxy_password");

	if (((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) == 0) &&
		(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ZTE)) ||
	    ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) &&
		(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_HUAWEI))) {
		struct meinfo_t *miptr = meinfo_util_miptr(150);
		struct me_t *me = NULL;
		int has_domain_name = 0;
		// check existence of domain name
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			unsigned short meid = (unsigned short) meinfo_util_me_attr_data(me, 8);
			if(meinfo_me_get_by_meid(157, meid) != NULL) {
				has_domain_name = 1;
				break;
			}
		}
		// if there is no domain name specified, fill domain name from username's fqdn if there is '@'
		if(!has_domain_name) {
			char *p = strstr(user, "@");
			if(p) print_tab_mvalue(fd, "* Domain_Name: ", level+1, p+1, p+1, "voip_ep%u_domain_name");
		}
	}

	return CLI_OK;
}

static int
cli_sip_agent_related_registar_netaddr(int fd, struct me_t *me_sip_agent, int level)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	int flag=0, meid;
	char tab_str[32];
	char registar_addr[375];
	memset(tab_str, '\t', 31);
	tab_str[level]='\0';

	if (me_sip_agent == NULL) {
		goto exit;
	}
	struct me_t *me=meinfo_me_get_by_meid(137, meinfo_util_me_attr_data(me_sip_agent, 5));
	if (me) {
		print_tab_mvalue_meid(fd, ">[137]Network_address", level, me->meid);
		struct me_t *me_registar_auth=meinfo_me_get_by_meid(148, meinfo_util_me_attr_data(me, 1));
		struct me_t *me_registar_addr=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 2));
		meinfo_me_refresh(me, get_attr_mask);
		if (NULL == me_registar_auth) {
			meid=-1;
		} else {
			meid=me_registar_auth->meid;
		}
		print_tab_mvalue_meid(fd, ">[148]1 Security_pointer:", level+1, meid);
		cli_sip_user_related_auth_username_password(fd, me_registar_auth, level+2);

		if (NULL == me_registar_addr) {
			meid=-1;
		} else {
			meid=me_registar_addr->meid;
		}
		print_tab_mvalue_meid(fd, ">[157]2 Address_pointer:", level+1, meid);
		if (me_registar_addr) {
			unsigned char i, parts = (unsigned char) meinfo_util_me_attr_data(me_registar_addr, 1);
			memset(registar_addr, 0, sizeof(registar_addr));
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_registar_addr, 2+i, &attr);
				strcat(registar_addr, str);
			}
		}
		print_tab_text(fd, "2 Part_1: ", level+2, registar_addr);
		flag=1;
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[137]Network_address", level, -1);
		print_tab_mvalue_meid(fd, ">[148]1 Security_pointer:", level+1, -1);
		print_tab_mvalue_meid(fd, ">[157]2 Address_pointer:", level+1, -1);
	}
	return CLI_OK;
}

/* return 1 global port_id changed; 0 unchanged; -1 error */
static int
cli_sip_agent_related_tcpudp_attr_1_port_id(struct me_t *me_sip_agent)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	unsigned short us;
	struct me_t *me=meinfo_me_get_by_meid(136, meinfo_util_me_attr_data(me_sip_agent, 5));
	if (me_sip_agent == NULL || me == NULL) {
		return -1;
	}
	meinfo_me_refresh(me, get_attr_mask);
	us=(unsigned short)meinfo_util_me_attr_data(me, 1);
	if (0 != us && us != TCP_UDP_config_data_port_id_g[port_id_g]) {
		TCP_UDP_config_data_port_id_g[port_id_g] = us;
		return 1;
	}
	return 0;
}

static int
cli_sip_agent_related_tcpudp(int fd, struct me_t *me_sip_agent, int level)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	unsigned char uc;
	unsigned short us;
	struct me_t *me=meinfo_me_get_by_meid(136, meinfo_util_me_attr_data(me_sip_agent, 5));
	int flag=0;

	if (me_sip_agent == NULL) {
		goto exit;
	}
	if (me) {
		flag=1;
		meinfo_me_refresh(me, get_attr_mask);
		print_tab_mvalue_meid(fd, "[136]TCP/UDP_config_data:", level, me->meid);
		us=(unsigned short)meinfo_util_me_attr_data(me, 1);
		print_tab_text(fd, "1 Port_id: ", level+1, int2str(2, &us));
		uc=(unsigned char)meinfo_util_me_attr_data(me, 2);
		print_tab_mvalue(fd, "2 Protocol: ", level+1, int2str(1, &uc), int2str(1, &uc),"voip_ep%u_sip_ip_addr__dot__protocol");
		uc=(unsigned char)meinfo_util_me_attr_data(me, 3);
		print_tab_mvalue(fd, "3 TOS/diffserv_field: ", level+1, int2str(1, &uc), "", NO_SUPPORT);
		us=(unsigned short)meinfo_util_me_attr_data(me, 4);
		print_tab_mvalue_meid(fd, "4 IP_host_pointer [classid 134]: ", level+1, us);
	}
exit:
	if (!flag) {
		us=0;
		print_tab_mvalue_meid(fd, "[136]TCP/UDP_config_data:", level, 0);
		print_tab_mvalue_skip(fd, "1 Port_id: ", level+1, "voip_ep%u_sip_proxies_1");
		print_tab_mvalue_skip(fd, "           ", level+1, "voip_ep%u_outbound_proxies_1");
		print_tab_mvalue_skip(fd, "           ", level+1, "voip_ep%u_sip_registrars_1");
		print_tab_mvalue_skip(fd, "2 Protocol: ", level+1, "voip_ep%u_sip_ip_addr__dot__protocol");
		print_tab_mvalue_no_support(fd, "3 TOS/diffserv_field: ", level+1);
		us=0;
		print_tab_mvalue_meid(fd, "4 IP_host_pointer [classid 134]: ", level+1, 0);
	}
	return CLI_OK;
}

static char *
get_agent_status_str(int number)
{
	static char *status_str[6]={ "initial", "connected", "icmp_err", "malformed_resp", "inadequate_resp", "timeout" };
	if (number>=0 && number<6)
		return status_str[number];
	return status_str[3];
}

static int
print_parm_me_150(int fd, struct me_t *me)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	unsigned int ui;
	unsigned short us;
	char proxy[375];
	struct me_t *me_proxy=NULL, *me_obproxy=NULL, *me_hosturi=NULL;

	if (me == NULL) {
		print_tab_mvalue_skip(fd, "1 Proxy_server_address_pointer [classid 157]: ", 3, "voip_ep%u_sip_proxies_1");
		print_tab_mvalue_skip(fd, "2 Outbound_proxy_address_pointer [classid 157]: ", 3, "voip_ep%u_outbound_proxies_1");
		print_tab_mvalue_skip(fd, "3 Primary_SIP_DNS: ", 3, "voip_general_dns_server_1");
		print_tab_mvalue_skip(fd, "4 Secondary_SIP_DNS: ", 3, "voip_general_dns_server_2");
		print_tab_mvalue_skip(fd, "5 TCP/UDP_pointer [classid 136]: ", 3, "voip_ep%u_sip_ip_addr__dot__protocol");
		print_tab_mvalue_skip(fd, "6 SIP_reg_exp_time : ", 3, "voip_ep%u_sip_registration_timeout");
		print_tab_mvalue_skip(fd, "7 SIP_rereg_head_start_time: ", 3, "voip_ep%u_sip_reregistration_head_start_time");
		print_tab_mvalue_skip(fd, "8 Host_part_URI [classid 157]: ", 3, "voip_ep%u_domain_name");
		print_tab_mvalue_skip(fd, "9 SIP_status: ", 3, "fvcli get epregstate %d");
		print_tab_mvalue_skip(fd, "10 SIP_registrar [classid 137]: ", 3, "voip_ep%u_sip_registrars_1");
		print_tab_mvalue_skip(fd, "                              : ", 3, "voip_ep%u_sip_registrars_2");
		print_tab_mvalue_no_support(fd, "11 Softswitch: ", 3);
	} else {
		cli_sip_agent_related_tcpudp_attr_1_port_id(me);

		us=(unsigned short)meinfo_util_me_attr_data(me, 1);
		print_tab_mvalue_meid(fd, "1 Proxy_server_address_pointer [classid 157]: ", 3, us);

		//we only know in c_sip mode voip_ep[EP]_sip_proxies_1 and voip_ep[EP]_sip_registrars_1
		//is according to me 243 (meid 0x1(1) inst 0, meid 0x2(2) inst 1) attr 1 to me 157
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
			// decide c_sip_mode_g agag by me243 attr1
			/* Redmine #6951 Comment #77 ALLEN UENG added a comment - 2019-01-04 18:35:57 */
			/* We found that me 243(VoipConfigDataExt) meid=2 attr 1(PrimaryConfigServerPointer) really point to sip registrars in me 157. So we can check if c-sip mode is enabled with me 243 atr 1 in additional to me 138 attr 4. We are working on the solution. */
			struct me_t *me_243=NULL;
			if( (me_243=meinfo_me_get_by_meid(243, me->meid )) != NULL) {
				if (0xffff != (us=(unsigned short)meinfo_util_me_attr_data(me_243, 1))) {
					c_sip_mode_g = 1;
				} else {
					c_sip_mode_g = 0;
				}
			}
		}

		me_proxy=meinfo_me_get_by_meid(157, us);
		if (me_proxy) {
			unsigned char i, parts;
			meinfo_me_refresh(me_proxy, get_attr_mask);
			memset(proxy, 0, sizeof(proxy));
			parts = (unsigned char) meinfo_util_me_attr_data(me_proxy, 1);
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_proxy, 2+i, &attr);
				strcat(proxy, str);
			}
			snprintf(proxy_g[port_id_g], 375, "%s", proxy);
			snprintf(proxy, 375, "%s:%u", proxy_g[port_id_g], TCP_UDP_config_data_port_id_g[port_id_g]);
			print_tab_mvalue(fd, "                                            : ", 3, proxy, proxy, "voip_ep%u_sip_proxies_1");
		} else {
			print_tab_mvalue_skip(fd, "                                            : ", 3, "voip_ep%u_sip_proxies_1");
		}

		us=(unsigned short)meinfo_util_me_attr_data(me, 2);
		print_tab_mvalue_meid(fd, "2 Outbound_proxy_address_pointer [classid 157]: ", 3, us);
		me_obproxy=meinfo_me_get_by_meid(157, us);
		if (me_obproxy) {
			unsigned char i, parts;
			meinfo_me_refresh(me_obproxy, get_attr_mask);
			memset(proxy, 0, sizeof(proxy));
			parts = (unsigned char) meinfo_util_me_attr_data(me_obproxy, 1);
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_obproxy, 2+i, &attr);
				strcat(proxy, str);
			}
			snprintf(obproxy_g[port_id_g], 375, "%s", proxy);
			snprintf(proxy, 375, "%s:%u", obproxy_g[port_id_g], TCP_UDP_config_data_port_id_g[port_id_g]);
			print_tab_mvalue(fd, "                                              : ", 3, proxy, proxy, "voip_ep%u_outbound_proxies_1");
			// enable outbound proxy flag if outbound proxy is set
			print_tab_mvalue(fd, "                                              : ", 3, (parts)?"1":"0", (parts)?"1":"0", "voip_ep%u_proxy_outbound_flag");
		} else {
			print_tab_mvalue_skip(fd, "                                              : ", 3, "voip_ep%u_outbound_proxies_1");
			// disable outbound proxy flag if outbound proxy is unset
			print_tab_mvalue(fd, "                                              : ", 3, "0", "0", "voip_ep%u_proxy_outbound_flag");
		}

		ui=(unsigned int)meinfo_util_me_attr_data(me, 3);
		print_tab_mvalue(fd, "3 Primary_SIP_DNS: ", 3, iphost_inet_string(ui), iphost_inet_string(ui), "voip_general_dns_server_1");

		ui=(unsigned int)meinfo_util_me_attr_data(me, 4);
		print_tab_mvalue(fd, "4 Secondary_SIP_DNS: ", 3, iphost_inet_string(ui), iphost_inet_string(ui), "voip_general_dns_server_2");

		us=(unsigned short)meinfo_util_me_attr_data(me, 5);
		print_tab_mvalue_meid(fd, "5 TCP/UDP_pointer [classid 136]: ", 3, us);
		cli_sip_agent_related_tcpudp(fd, me, 3);

		ui=(unsigned int)meinfo_util_me_attr_data(me, 6);
		print_tab_mvalue(fd, "6 SIP_reg_exp_time: ", 3, int2str(4, &ui), int2str(4, &ui), "voip_ep%u_sip_registration_timeout");

		ui=(unsigned int)meinfo_util_me_attr_data(me, 7);
		print_tab_mvalue(fd, "7 SIP_rereg_head_start_time: ", 3, int2str(4, &ui), int2str(4, &ui), "voip_ep%u_sip_reregistration_head_start_time");

		us=(unsigned short)meinfo_util_me_attr_data(me, 8);
		print_tab_mvalue_meid(fd, "8 Host_part_URI [classid 157]: ", 3, us);
		me_hosturi=meinfo_me_get_by_meid(157, us);
		if (me_hosturi) {
			unsigned char i, parts;
			meinfo_me_refresh(me_hosturi, get_attr_mask);
			memset(proxy, 0, sizeof(proxy));
			parts = (unsigned char) meinfo_util_me_attr_data(me_hosturi, 1);
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_hosturi, 2+i, &attr);
				strcat(proxy, str);
			}
			print_tab_mvalue(fd, "                             : ", 3, proxy, proxy, "voip_ep%u_domain_name");
		} else {
			//If domain name setting is empty in OLT side, need to clear the default setting in metafile
			print_tab_mvalue(fd, "                             : ", 3, "", "", "voip_ep%u_domain_name");
		}

		print_tab_mvalue(fd, "9 SIP_status: ", 3, get_agent_status_str((unsigned char)meinfo_util_me_attr_data(me, 9)), "", "fvcli get epregstate %u");

		us=(unsigned short)meinfo_util_me_attr_data(me, 10);
		print_tab_mvalue_meid(fd, "10 SIP_registrar [classid 137]: ", 3, us);

		//we only know in c_sip mode voip_ep[EP]_sip_proxies_1 and voip_ep[EP]_sip_registrars_1
		//is according to me 243 (meid 0x1(1) inst 0, meid 0x2(2) inst 1) attr 1 to me 157
		if (c_sip_mode_g==1) {
			struct me_t *me_243=NULL;
			if( (me_243=meinfo_me_get_by_meid(243, me->meid )) != NULL) {
				us=(unsigned short)meinfo_util_me_attr_data(me_243, 1);
			}
		} else {
			struct me_t *me_137=NULL;
			if( (me_137=meinfo_me_get_by_meid(137, us )) != NULL) {
				us=(unsigned short)meinfo_util_me_attr_data(me_137, 2);
			}
		}
		me_proxy=meinfo_me_get_by_meid(157, us);
		if (me_proxy) {
			unsigned char i, parts;
			meinfo_me_refresh(me_proxy, get_attr_mask);
			memset(proxy, 0, sizeof(proxy));
			parts = (unsigned char) meinfo_util_me_attr_data(me_proxy, 1);
			for(i=0; i<parts; i++) {
				struct attr_value_t attr;
				unsigned char str[26];
				memset(str, 0x00, sizeof(str));
				attr.ptr = str;
				meinfo_me_attr_get(me_proxy, 2+i, &attr);
				strcat(proxy, str);
			}
			snprintf(registrar_g[port_id_g], 375, "%s", proxy);
			snprintf(proxy, 375, "%s:%u", registrar_g[port_id_g], TCP_UDP_config_data_port_id_g[port_id_g]);
			print_tab_mvalue(fd, "                              : ", 3, proxy, proxy, "voip_ep%u_sip_registrars_1");
		} else {
			print_tab_mvalue_skip(fd, "                              : ", 3, "voip_ep%u_sip_registrars_1");
		}
		cli_sip_agent_related_registar_netaddr(fd, me, 3);

		print_tab_mvalue_no_support(fd, "11 Softswitch: ", 3);
	}
	return CLI_OK;
}

static int
cli_sip_user_related_agent_150(int fd, struct me_t *me_sip_user)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(150);
	struct me_t *me=NULL;
	int flag=0;

	if (me_sip_user == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sip_user, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, ">[150]SIP_agent_config_data", 2, me->meid);
			flag=1;
			print_parm_me_150(fd, me);
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[150]agent", 2, -1);
		cli_sip_agent_related_tcpudp(fd, NULL, 2);
		cli_sip_agent_related_registar_netaddr(fd, NULL, 2);
		print_parm_me_150(fd, NULL);
	}
	return CLI_OK;
}

static int
cli_sip_user_related_auth_148(int fd, struct me_t *me_sip_user)
{
	if (me_sip_user == NULL) {
		cli_sip_user_related_auth_username_password(fd, NULL, 2);
	} else {
		struct me_t *me=meinfo_me_get_by_meid(148, meinfo_util_me_attr_data(me_sip_user, 4));
		cli_sip_user_related_auth_username_password(fd, me, 2);
	}
	return CLI_OK;
}

static int
print_parm_153(int fd, struct me_t *me)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	unsigned short us;
	unsigned int ui;
	char *p=NULL, string[375];
	unsigned char uc;
	struct me_t *me_2=NULL;

	us=(unsigned short)meinfo_util_me_attr_data(me, 1);
	print_tab_mvalue_meid(fd, "1 SIP_agent_pointer [classid 150]: ", 2, us);
	cli_sip_user_related_agent_150(fd, me);

	us=(unsigned short)meinfo_util_me_attr_data(me, 2);
	me_2=meinfo_me_get_by_meid(157, us);
	memset(string, 0, sizeof(string));
	if (me_2) {
		unsigned char i, parts;
		meinfo_me_refresh(me_2, get_attr_mask);
		parts = (unsigned char) meinfo_util_me_attr_data(me_2, 1);
		for(i=0; i<parts; i++) {
			struct attr_value_t attr;
			unsigned char str[26];
			memset(str, 0x00, sizeof(str));
			attr.ptr = str;
			meinfo_me_attr_get(me_2, 2+i, &attr);
			strcat(string, str);
		}
		print_tab_mvalue(fd, "2 User_part_AOR [classid 157]: ", 2, string, string, "voip_ep%u_phone_number");
	} else if (me == NULL &&
		   me_153_created[port_id_g]==1) {
		print_tab_mvalue(fd, "2 User_part_AOR [classid 157]: ", 2, string, string, "voip_ep%u_phone_number");
	}

	p=meinfo_util_me_attr_ptr(me, 3);
	print_tab_mvalue(fd, "3 SIP_display_name: ", 2, p, p, "voip_ep%u_display_name");

	me_2=meinfo_me_get_by_meid(148, meinfo_util_me_attr_data(me, 4));
	if (me_2) {
		meinfo_me_refresh(me_2, get_attr_mask);
		p=meinfo_util_me_attr_ptr(me_2, 2);
	}
	us=(unsigned short)meinfo_util_me_attr_data(me, 4);
	print_tab_mvalue_meid(fd, "4 Username/password [classid 148]: ", 2, us);
	cli_sip_user_related_auth_148(fd, me);

	us=(unsigned short)meinfo_util_me_attr_data(me, 5);
	print_tab_mvalue_meid(fd, "5 Voicemail_server_SIP_URI [classid 137]: ", 2, us);
	cli_sip_agent_related_registar_netaddr(fd, me, 2);

	ui=(unsigned int)meinfo_util_me_attr_data(me, 6);
	print_tab_mvalue(fd, "6 Voicemail_subscription_expiration_time: ", 2, int2str(4, &ui), int2str(4, &ui), "voip_ep%u_mwi_subscribe_expire_time");

	us=(unsigned short)meinfo_util_me_attr_data(me, 7);
	print_tab_mvalue_meid(fd, "7 Network_dial_plan_pointer [classid 145]: ", 2, us);
	cli_sip_user_related_dialplan_145(fd, me);

	us=(unsigned short)meinfo_util_me_attr_data(me, 8);
	print_tab_mvalue_meid(fd, "8 Application_services_profile_pointer [classid 146]: ", 2, us);
	cli_sip_user_related_voip_app_service_profile_146(fd, me);

	us=(unsigned short)meinfo_util_me_attr_data(me, 9);
	print_tab_mvalue_meid(fd, "9 Feature_code_pointer [classid 147]: ", 2, us);
	cli_sip_user_related_voip_feature_access_codes_147(fd, me);

	us=(unsigned short)meinfo_util_me_attr_data(me, 10);
	print_tab_mvalue_meid(fd, "10 PPTP_pointer [classid 53]: ", 2, us);

	uc=(unsigned int)meinfo_util_me_attr_data(me, 11);
	print_tab_mvalue(fd, "11 Release_timer: ", 2, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_release_timeout");

	uc=(unsigned int)meinfo_util_me_attr_data(me, 12);
	print_tab_mvalue(fd, "12 ROH_timer: ", 2, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_roh_timeout");

	return 0;
}

static int
dump_me_153(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	int flag=0;
	struct meinfo_t *miptr=meinfo_util_miptr(153);
	struct me_t *me=NULL;

	if (me_pots == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			unsigned short pptp_meid = (unsigned short) meinfo_util_me_attr_data(me, 10);
			struct me_t *pptp_me = meinfo_me_get_by_meid(53, pptp_meid);
			flag=1;
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, "<[153]sip_user", 1, me->meid);
			print_parm_153(fd, me);
			if (pptp_me) {
				ep_enable_ary_g[pptp_me->instance_num]=1;
			}
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, "<[153]sip_user", 1, -1);
		print_parm_153(fd, NULL);
	}
	return CLI_OK;
}

static int
print_mkey_me_143(int fd)
{
	print_tab_mvalue(fd, "1 Local port min: ", 4, "NULL", "SKIP", "voip_general_rtp_port__dot__base");
	print_tab_mvalue(fd, "2 Local port max: ", 4, "NULL", "SKIP", "voip_general_rtp_port__dot__limit");
	print_tab_mvalue(fd, "3 DSCP mark: ", 4, "NULL", "SKIP", "voip_general_media_tos");
	print_tab_mvalue_skip(fd, "4 Piggyback events: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "5 Tone events: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "6 DTMF events: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "7 CAS events: ", 4, NO_SUPPORT);
	return 0;
}

static int
dump_me_143(int fd, struct me_t *me_media_profile)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(143);
	struct me_t *me=NULL;
	int flag=0;
	unsigned short us, us2;

	if (me_media_profile == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_media_profile, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			us = (unsigned short)meinfo_util_me_attr_data(me, 1);
			print_tab_mvalue_meid(fd, ">[143]rtp_profile_data", 3, me->meid);
			print_tab_mvalue(fd, "1 Local port min: ", 4,
					 int2str(2, &us),
					 int2str(2, &us),
					 "voip_general_rtp_port__dot__base");
			us = (unsigned short)meinfo_util_me_attr_data(me, 2);
			print_tab_mvalue(fd, "2 Local port max: ", 4,
					 int2str(2, &us),
					 int2str(2, &us),
					 "voip_general_rtp_port__dot__limit");
			us = (unsigned short)meinfo_util_me_attr_data(me, 3);
			us2 = us << 2; // tos = DSCP << 2
			print_tab_mvalue(fd, "3 DSCP mark: ", 4,
					 int2str(2, &us),
					 int2str(2, &us2),
					 "voip_general_media_tos");
			print_tab_mvalue_skip(fd, "4 Piggyback events: ", 4, NO_SUPPORT);
			print_tab_mvalue_skip(fd, "5 Tone events: ", 4, NO_SUPPORT);
			print_tab_mvalue_skip(fd, "6 DTMF events: ", 4, NO_SUPPORT);
			print_tab_mvalue_skip(fd, "7 CAS events: ", 4, NO_SUPPORT);
			flag=1;
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[143]rtp_profile_data", 3, -1);
		print_mkey_me_143(fd);
	}
	return CLI_OK;
}

static int
print_parm_me_58(int fd)
{
	print_tab_mvalue(fd, "1 Announcement type: ", 4, "NULL","0x03", NULL);
	print_tab_mvalue(fd, "2 Jitter target: ", 4, "NULL", "1", "voip_ep%u_jb_type");
	print_tab_mvalue_skip(fd, "               : ", 4, "voip_ep%u_fjb_normal_delay");
	print_tab_mvalue_skip(fd, "3 Jitter buffer max: ", 4, "voip_ep%u_jb_type");
	print_tab_mvalue_skip(fd, "4 Echo cancel ind: ", 4, "voip_ep%u_aec_enable");
	print_tab_mvalue_skip(fd, "5 PSTN protocol variant: ", 4, "voip_general_e164_country_code");
	print_tab_mvalue_skip(fd, "6 DTMF digit levels: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "7 DTMF digit duration: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "8 Hook flash minimum time: ", 4, "voip_ep%u_flashhook_timeout_lb");
	print_tab_mvalue_skip(fd, "9 Hook flash maximum time: ", 4, "voip_ep%u_flashhook_timeout_ub");
	print_tab_mvalue_skip(fd, "10 Tone pattern table: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "11 Tone event table: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "12 Ringing pattern table: ", 4, NO_SUPPORT);
	print_tab_mvalue_skip(fd, "13 Ringing event table: ", 4, NULL);
	print_tab_mvalue_skip(fd, "13.2 Tone_file: ", 5, "voip_ep%u_distinctive_ring_enable");
	print_tab_mvalue_skip(fd, "              : ", 5, "voip_ep%u_distinctive_ring_prefix");
	print_tab_mvalue_skip(fd, "14 Network specific extensions pointer: ", 4, NO_SUPPORT);
	return 0;
}

static int
dump_me_58(int fd, struct me_t *me_media_profile)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(58);
	struct me_t *me=NULL;
	char buff[1024], *array[8], *p;
	int MAYBE_UNUSED s;
	int flag=0, l;
	unsigned short us, us2, us_ajb;
	struct metacfg_t kv;

	if (me_media_profile == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_media_profile, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, ">[58]voice_service_profile", 3, me->meid);
			flag=1;
			snprintf(buff, 1024, "[%u]Support fast busy(0x03) only", (unsigned char)meinfo_util_me_attr_data(me, 1));
			print_tab_text(fd, "1 Announcement_type: ", 4, buff);
			us=(unsigned short)meinfo_util_me_attr_data(me, 2);
			if (us > 0) {
				print_tab_mvalue(fd, "2 Jitter_target: ", 4, int2str(2, &us), "0", "voip_ep%u_jb_type");
				print_tab_mvalue(fd, "               : ", 4, int2str(2, &us), int2str(2, &us), "voip_ep%u_fjb_normal_delay");
			} else {
				print_tab_mvalue(fd, "2 Jitter_target: ", 4, int2str(2, &us), "1", "voip_ep%u_jb_type");
			}
			us=(unsigned short)meinfo_util_me_attr_data(me, 3);
			if (us == 0) {
				if (port_id_g==0 || port_id_g==1) {
					snprintf(buff, 1024, "voip_ep%u_ajb_max_delay", port_id_g);
				} else {
					snprintf(buff, 1024, "voip_ep%u_ajb_max_delay", 0);
				}
				memset(&kv, 0x0, sizeof(struct metacfg_t));
				metacfg_adapter_config_init(&kv);
				metacfg_adapter_config_file_load_part(&kv, "/rom/etc/wwwctrl/metafile.dat", buff, buff);
				us_ajb=atoi(metacfg_adapter_config_get_value(&kv, buff, 1));
				metacfg_adapter_config_release(&kv);
			} else {
				us_ajb=us;
			}
			print_tab_mvalue(fd, "3 Jitter_buffer_max: ", 4, int2str(2, &us), int2str(2, &us_ajb), "voip_ep%u_ajb_max_delay");

			us=(unsigned short)meinfo_util_me_attr_data(me, 4);
			print_tab_mvalue(fd, "4 Echo_cancel_ind: ", 4, int2str(2, &us), int2str(2, &us), "voip_ep%u_aec_enable");

			us=(unsigned short)meinfo_util_me_attr_data(me, 5);
			print_tab_mvalue(fd, "5 PSTN_protocol_variant: ", 4, int2str(2, &us), int2str(2, &us), "voip_general_e164_country_code");

			us=(unsigned short)meinfo_util_me_attr_data(me, 6);
			print_tab_mvalue(fd, "6 DTMF_digit_levels: ", 4, int2str(2, &us), "SKIP", NO_SUPPORT);

			us=(unsigned short)meinfo_util_me_attr_data(me, 7);
			print_tab_mvalue(fd, "7 DTMF_digit_duration: ", 4, int2str(2, &us), "SKIP", NO_SUPPORT);

			if (0 < (us=(unsigned short)meinfo_util_me_attr_data(me, 8))) {
				if (us < VALUE_FLAHHOOKTIMEOUT_MIN)  {
					us2 = VALUE_FLAHHOOKTIMEOUT_MIN;
				} else if (us > VALUE_FLAHHOOKTIMEOUT_MAX) {
					us2 = VALUE_FLAHHOOKTIMEOUT_MAX;
				} else {
					us2=us;
				}
				print_tab_mvalue(fd, "8 Hook_flash_minimum_time: ", 4, int2str(2, &us), int2str(2, &us2), "voip_ep%u_flashhook_timeout_lb");
			} else {
				print_tab_mvalue_skip(fd, "8 Hook_flash_minimum_time: ", 4, "voip_ep%u_flashhook_timeout_lb");
			}

			if (0 < (us=(unsigned short)meinfo_util_me_attr_data(me, 9))) {
				if (us < VALUE_FLAHHOOKTIMEOUT_MIN)  {
					us2 = VALUE_FLAHHOOKTIMEOUT_MIN;
				} else if (us > VALUE_FLAHHOOKTIMEOUT_MAX) {
					us2 = VALUE_FLAHHOOKTIMEOUT_MAX;
				} else {
					us2=us;
				}
				print_tab_mvalue(fd, "9 Hook_flash_maximum_time: ", 4, int2str(2, &us), int2str(2, &us2), "voip_ep%u_flashhook_timeout_ub");
			} else {
				print_tab_mvalue_skip(fd, "9 Hook_flash_maximum_time: ", 4, "voip_ep%u_flashhook_timeout_ub");
			}

			print_tab_mvalue_no_support(fd, "10 Tone_pattern_table: ", 4);

			print_tab_mvalue_no_support(fd, "11 Tone_event_table: ", 4);

			print_tab_mvalue_no_support(fd, "12 Ringing_pattern_table: ", 4);

			struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, 13);
			print_tab_local(fd, "13 Ringing_event_table: ", 4, NULL);
			if (aiptr->format == ATTR_FORMAT_TABLE) {
				struct attr_table_header_t *tab_header = me->attr[13].value.ptr;
				struct attr_table_entry_t *tab_entry;
				int j=0;
				if (tab_header) {
					list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
						if (tab_entry->entry_data_ptr != NULL) {
							struct attr_value_t attr;
							attr.data=0;
							attr.ptr=tab_entry->entry_data_ptr;	// use attr to encapsulate entrydata for temporary use
							buff[0]='\0';
							meinfo_conv_attr_to_string(miptr->classid, 13, &attr, buff, 1024);
							if (0 < (l=strlen(buff))) {
								p = (char *)malloc_safe(l+1);
								snprintf(p, l+1, "%s", buff);
							} else {
								p = (char *)malloc_safe(1);
								*p = '\0';
							}
							s=util_string2array(p, ", ", array, 8);
							if (meta_diff_g == 0) {
								util_fdprintf(fd, "[%s]", buff);
							}
							print_tab_mvalue(fd, "13.0 P Event: ", 5, array[0], array[0], NO_SUPPORT);
							print_tab_mvalue(fd, "13.1 Tone_pattern: ", 5, array[1], array[1], NO_SUPPORT);
							print_tab_local(fd, "13.2 Tone_file: ", 5, NULL);
							if (array[2]) {
								unsigned short ls_meid = (unsigned short) util_bitmap_get_value(attr.ptr, 8*7, 16, 16);
								struct me_t *temp_me;
								char *ringing_file, *ls_ptr=NULL;
								if ((temp_me = meinfo_me_get_by_meid(157, ls_meid)) == NULL) {
									ringing_file = NULL;
								} else {
									ls_ptr = malloc_safe(376);
									meinfo_me_copy_from_large_string_me(temp_me, ls_ptr, 375);
									ringing_file = ls_ptr;
								}
								print_tab_mvalue(fd, "              : ", 5, "1","1","voip_ep%u_distinctive_ring_enable");
								print_tab_mvalue(fd, "              : ", 5, ringing_file,ringing_file,"voip_ep%u_distinctive_ring_prefix");
								if (ls_ptr) {free_safe(ls_ptr);}
							} else {
								print_tab_mvalue(fd, "              : ", 5, "0","0","voip_ep%u_distinctive_ring_enable");
								print_tab_mvalue(fd, "              : ", 5, "","","voip_ep%u_distinctive_ring_prefix");
							}
							print_tab_mvalue(fd, "13.3 Tone_file_repetitions: ", 5, array[3], array[3], NO_SUPPORT);
							print_tab_mvalue(fd, "13.4 Ringing_text: ", 5, array[4], array[4], NO_SUPPORT);
							free_safe(p);
							j++;
						}
					}
				}
			}
			us=(unsigned short)meinfo_util_me_attr_data(me, 14);
			print_tab_mvalue_meid(fd, "14 Network_specific_extensions_pointer [classid 137]: ", 4, us);
			if (us) {
				cli_sip_agent_related_registar_netaddr(fd, me, 4);
			}
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[58]voice_service_profile", 3, -1);
		print_parm_me_58(fd);
	}
	return CLI_OK;
}

static int
get_codec_index(int number)
{
	if (number>=0 && number<19) {
		return number;
	} else {
		return 1;
	}
}

static char *
get_codec_str(int number)
{
	static char *codec_str[19]={
		"pcmu", "reserved", "reserved", "gsm", "g723",
		"dvi4-8k", "dvi4-16k", "lpc", "pcma", "g722",
		"l16_2ch", "l16_1ch", "qcelp", "cn", "mpa",
		"g728", "dvi4-11k", "dvi4-22k", "g729"
	};
	return codec_str[get_codec_index(number)];
}

static int
print_parm_me_142(int fd, struct me_t *me)
{
	unsigned char i, uc, uc2, codec_list_ary[20], index=0, flag;
	uc=(unsigned char)meinfo_util_me_attr_data(me, 1);
	flag=0;
	if (uc == 0) {
		uc2=3;
	} else if (uc == 1) {
		uc2=2;
	} else {
		flag=1;
		print_tab_text(fd, "1 Fax_mode: ", 3, int2str(1, &uc));
	}
	if (!flag) {
		print_tab_mvalue(fd, "1 Fax_mode: ", 3, int2str(1, &uc), int2str(1, &uc2), "voip_ep%u_fax_params__dot__t38_fax_opt");
	}
	print_tab_local(fd, "2 Voice_service_profile_pointer [classid 58]: ", 3, NULL);
	dump_me_58(fd, me);
	uc = (unsigned char)get_codec_index((int)meinfo_util_me_attr_data(me, 3));
	codec_list_ary[index++]=uc;
	print_tab_mvalue(fd, "3 Codec_selection_1st_order: ", 3, int2str(1, &uc), get_codec_str((int)uc), "voip_ep%u_codec_list_1");
	if (0 < (uc=(unsigned char)meinfo_util_me_attr_data(me, 4))) {
		print_tab_mvalue(fd, "4 Packet_period_selection_1st_order: ", 3, int2str(1, &uc), int2str(1, &uc), "voip_ep%u_ptime");
	}
	if (0 == (uc=(unsigned char)meinfo_util_me_attr_data(me, 5))) {
		print_tab_mvalue(fd, "5 Silence_suppression_1st_order: ", 3, int2str(1, &uc), "1", "voip_ep%u_vad_type");
		uc2=1;
	} else {
		print_tab_mvalue(fd, "5 Silence_suppression_1st_order: ", 3, int2str(1, &uc), "3", "voip_ep%u_vad_type");
	}

	uc = (unsigned char)get_codec_index((int)meinfo_util_me_attr_data(me, 6));
	flag=0;
	for (i=0; i<index; i++) {
		if (codec_list_ary[i] == uc)
			flag=1;
	}
	if (!flag) {
		codec_list_ary[index++]=uc;
		print_tab_mvalue(fd, "6 Codec_selection_2nd_order: ", 3, int2str(1, &uc), get_codec_str((int)uc), "voip_ep%u_codec_list_2");
	} else {
		print_tab_mvalue(fd, "6 Codec_selection_2nd_order: ", 3, int2str(1, &uc), "", "voip_ep%u_codec_list_2");
	}
	uc=(unsigned char)meinfo_util_me_attr_data(me, 7);
	print_tab_mvalue(fd, "7 Packet_period_selection_2nd_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);
	uc=(unsigned char)meinfo_util_me_attr_data(me, 8);
	print_tab_mvalue(fd, "8 Silence_suppression_2nd_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);

	uc = (unsigned char)get_codec_index((int)meinfo_util_me_attr_data(me, 9));
	flag=0;
	for (i=0; i<index; i++) {
		if (codec_list_ary[i] == uc)
			flag=1;
	}
	if (!flag) {
		codec_list_ary[index++]=uc;
		print_tab_mvalue(fd, "9 Codec_selection_3rd_order: ", 3, int2str(1, &uc), get_codec_str((int)uc), "voip_ep%u_codec_list_3");
	} else {
		print_tab_mvalue(fd, "9 Codec_selection_3rd_order: ", 3, int2str(1, &uc), "", "voip_ep%u_codec_list_3");
	}
	uc=(unsigned char)meinfo_util_me_attr_data(me, 10);
	print_tab_mvalue(fd, "10 Packet_period_selection_3rd_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);
	uc=(unsigned char)meinfo_util_me_attr_data(me, 11);
	print_tab_mvalue(fd, "11 Silence_suppression_3rd_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);

	uc = (unsigned char)get_codec_index((int)meinfo_util_me_attr_data(me, 12));
	flag=0;
	for (i=0; i<index; i++) {
		if (codec_list_ary[i] == uc)
			flag=1;
	}
	if (!flag) {
		codec_list_ary[index++]=uc;
		print_tab_mvalue(fd, "12 Codec_selection_4th_order: ", 3, int2str(1, &uc), get_codec_str((int)uc), "voip_ep%u_codec_list_4");
	} else {
		print_tab_mvalue(fd, "12 Codec_selection_4th_order: ", 3, int2str(1, &uc), "", "voip_ep%u_codec_list_4");
	}
	uc=(unsigned char)meinfo_util_me_attr_data(me, 13);
	print_tab_mvalue(fd, "13 Packet_period_selection_4th_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);
	uc=(unsigned char)meinfo_util_me_attr_data(me, 14);
	print_tab_mvalue(fd, "14 Silence_suppression_4th_order: ", 3, int2str(1, &uc), "SKIP", NO_SUPPORT);

	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_5");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_6");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_7");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_8");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_9");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_10");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_11");
	print_tab_mvalue(fd, "", 3, "", "", "voip_ep%u_codec_list_12");

	flag=0;
	uc = (unsigned char)meinfo_util_me_attr_data(me, 15);
	if (uc == 2) {
		uc2=4;
	} else if (uc == 1) {
		uc2=3;
	} else if (uc == 0) {
		uc2=0;
	} else {
		flag=1;
		print_tab_text(fd, "15 OOB_DTMF: ", 3, int2str(1, &uc));
	}
	if (!flag) {
		print_tab_mvalue(fd, "15 OOB_DTMF: ", 3, int2str(1, &uc), int2str(1, &uc2), "voip_ep%u_dtmf_type");
	}
	print_tab_local(fd, "16 RTP_profile_pointer [classid 143]: ", 3, NULL);
	dump_me_143(fd, me);
	return 0;
}

static int
dump_me_142(int fd, struct me_t *me_voice_ctp)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(142);
	struct me_t *me=NULL;
	int flag=0;

	if (me_voice_ctp == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_voice_ctp, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			flag=1;
			print_tab_mvalue_meid(fd, ">[142]voip_media_profile", 2, me->meid);
			print_parm_me_142(fd, me);
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, ">[142]voip_media_profile", 2, -1);
		print_parm_me_142(fd, NULL);
	}
	return CLI_OK;
}

static int
print_parm_me_139(int fd, struct me_t *me)
{
	unsigned short us;
	char c;
	if (me == NULL) {
		print_tab_mvalue(fd, "1 User_protocol_pointer [classid 153]: ", 2, "", "", NULL);
		print_tab_mvalue(fd, "2 PPTP_pointer [classid 53]: ", 2, int2str(2, &us), "", NULL);
		print_tab_mvalue(fd, "3 VoIP_media_profile_pointer [classid 142]: ", 2, "", "", NULL);
		dump_me_142(fd, me);
		print_tab_mvalue(fd, "4 Signalling_code: ", 2, "", "Support \"Loop start\" only", NULL);
	} else {
		us=(unsigned short)meinfo_util_me_attr_data(me, 1);
		print_tab_mvalue(fd, "1 User_protocol_pointer [classid 153]: ", 2, int2str(2, &us), "", NULL);
		us=(unsigned short)meinfo_util_me_attr_data(me, 2);
		print_tab_mvalue(fd, "2 PPTP_pointer [classid 53]: ", 2, int2str(2, &us), "", NULL);
		us=(unsigned short)meinfo_util_me_attr_data(me, 3);
		print_tab_mvalue(fd, "3 VoIP_media_profile_pointer [classid 142]: ", 2, int2str(2, &us), "", NULL);
		dump_me_142(fd, me);
		c=(unsigned short)meinfo_util_me_attr_data(me, 4);
		print_tab_mvalue(fd, "4 Signalling_code: ", 2, int2str(1, &c), "Support \"Loop start\" only", NULL);
	}
	return CLI_OK;
}

static int
dump_me_139(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(139);
	struct me_t *me=NULL;
	int flag=0;

	if (me_pots == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			flag=1;
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, "<[139]voice_ctp", 1, me->meid);
			print_parm_me_139(fd, me);
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, "<[139]voice_ctp", 1, -1);
		print_parm_me_139(fd, me);
	}
	return CLI_OK;
}

static char *
get_session_type_str(int number)
{
	static char *type_str[6]={ "none", "2way", "3way", "fax", "telem", "conference" };
	if (number>=0 && number<5)
		return type_str[number];
	return type_str[0];
}

#define MKEY_VOIP_PAYLOAD_INDEX_START 1
#define MKEY_VOIP_PAYLOAD_INDEX_END 16
#define MVALUE_VOIP_PAYLOAD_CSIP "_codec=telephone-event;_type=99"
#define MVALUE_VOIP_PAYLOAD_SIP "_codec=telephone-event;_type=101"
static void
cli_pots_calix_sip_mode_payload_type_mkey_value (int fd, int level)
{
	int i, j;
	char mkey[64], mvalue[256];
	for (i=0; i<PORT_NUM; i++) {
		for (j=MKEY_VOIP_PAYLOAD_INDEX_START; j<=MKEY_VOIP_PAYLOAD_INDEX_END; j++) {
			snprintf(mkey, 64, "voip_ep%d_payloads_%d", i, j);
			snprintf(mvalue, 256, "%s", metacfg_adapter_config_get_value(&kv_metafile_g, mkey, 1));
			if (0 < strlen(mvalue) &&
			    NULL != strstr(mvalue, "_codec=telephone-event")) {
				if (c_sip_mode_g == 1) {
					print_tab_mvalue(fd, "csip payload: ", level, MVALUE_VOIP_PAYLOAD_CSIP, MVALUE_VOIP_PAYLOAD_CSIP, mkey);
				} else {
					print_tab_mvalue(fd, "sip payload: ", level, MVALUE_VOIP_PAYLOAD_SIP, MVALUE_VOIP_PAYLOAD_SIP, mkey);
				}
			}
		}
	}
}

static int
cli_pots_related_line_status(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(141);
	struct me_t *me=NULL;
	int flag=0;

	if (meta_diff_g == 1) {
		return 0;
	}
	if (me_pots == NULL) {
		goto exit;
	}
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			flag=1;
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, "-[141]voip_line_status", 1, me->meid);
			util_fdprintf(fd, ",codec=%d(%s)",
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 1)));
			util_fdprintf(fd, ",section_type=%d(%s)",
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				get_session_type_str((unsigned char)meinfo_util_me_attr_data(me, 3)));
			print_local(fd, "\n\t\t1 Voip codec used: ", "fvcli get currentcodec %u");
			print_local(fd, "\n\t\t3 Voip port session type: ", "fvcli get voipsessiontype %u");
		}
	}
exit:
	if (!flag) {
		print_tab_mvalue_meid(fd, "-[141]voip_line_status", 1, -1);
		print_local(fd, "\n\t\t1 Voip codec used: ", "fvcli get currentcodec %u");
		print_local(fd, "\n\t\t3 Voip port session type: ", "fvcli get voipsessiontype %u");
	}
	return CLI_OK;
}

static int
print_parm_me_53(int fd, struct me_t *me)
{
	unsigned char uc, uc2;
	char c;
	int flag;

	if (0 == (uc=(unsigned char)meinfo_util_me_attr_data(me, 1))) {
		uc2=1;
	} else {
		uc2=0;
	}

	print_tab_mvalue_no_support(fd, "2 Interworking_TP_pointer [classid 266]: ", 1);
	print_tab_mvalue_no_support(fd, "3 ARC: ", 1);
	print_tab_mvalue_no_support(fd, "4 ARC_interval: ", 1);
	uc=(unsigned char)meinfo_util_me_attr_data(me, 5);
	flag=0;
	if (uc == 0) {
		uc2=0;
	} else if (uc == 1) {
		uc2=9;
	} else if (uc == 2) {
		uc2=1;
	} else if (uc == 3) {
		uc2=3;
	} else if (uc == 4) {
		uc2=10;
	} else {
		flag=1;
		print_tab_text(fd, "5 Impedance: ", 1, int2str(1, &uc));
	}
	if (!flag) {
		print_tab_mvalue(fd, "5 Impedance: ", 1, int2str(1, &uc), int2str(1, &uc2), "voip_ep%u_tdi_impedance_index");
	}

	print_tab_mvalue_no_support(fd, "6 Transmission_path: ", 1);
	c=(char)meinfo_util_me_attr_data(me, 7);
	print_tab_mvalue(fd, "7 Rx_gain: ", 1, int2str(3, &c), int2str(3, &c),	"voip_ep%u_sysrx_gain_cb");
	c=(char)meinfo_util_me_attr_data(me, 8);
	print_tab_mvalue(fd, "8 Tx_gain: ", 1, int2str(3, &c), int2str(3, &c), "voip_ep%u_systx_gain_cb");
	print_tab_mvalue_no_support(fd, "9 Operational_state: ", 1);
	uc=(unsigned char)meinfo_util_me_attr_data(me, 10);
	print_tab_mvalue(fd, "10 Hook_state: ", 1, int2str(1, &uc), "", "fvcli get hookstate %u");
	print_tab_mvalue_no_support(fd, "11 POTS_holdover_time: ", 1);
	return 0;
}

// meid: 0xffff: show all, other: show specific
static int
dump_me_53(int fd, unsigned short meid)
{
	unsigned char get_attr_mask[PORT_NUM] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(53);
	struct me_t *me=NULL;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		struct me_t *me_138;
		int voip_configuration_method_used;
		unsigned char e164=1;

		if( (me_138=meinfo_me_get_by_instance_num(138, 0)) != NULL) {
			voip_configuration_method_used=meinfo_util_me_attr_data(me_138, 4);
			print_tab_mvalue_meid(fd, "[138]VoIP_config_data", 0, me_138->meid);
			if(voip_configuration_method_used == 241 || voip_configuration_method_used == 242) {
				c_sip_mode_g=1;
			} else {
				c_sip_mode_g=0;
			}
		}
		if (c_sip_mode_g == 0) {
			// decide c_sip_mode_g again by me243 attr1
			/* Redmine #6951 Comment #77 ALLEN UENG added a comment - 2019-01-04 18:35:57 */
			/* We found that me 243(VoipConfigDataExt) meid=2 attr 1(PrimaryConfigServerPointer) really point to sip registrars in me 157. So we can check if c-sip mode is enabled with me 243 atr 1 in additional to me 138 attr 4. We are working on the solution. */
			struct me_t *me_243=NULL;
			unsigned short us;
			if( (me_243=meinfo_me_get_by_meid(243, 2)) != NULL) {
				if (0xffff != (us=(unsigned short)meinfo_util_me_attr_data(me_243, 1))) {
					c_sip_mode_g = 1;
				}
			}
		}
		if (c_sip_mode_g == 1) {
			print_tab_mvalue(fd, "4: VoIP_configuration_method_used: ", 1, int2str(1, &c_sip_mode_g), int2str(1, &c_sip_mode_g), "voip_general_sip_mode");
			print_tab_mvalue(fd, "Vendor ask modify PSTN protocol variant: ", 1, int2str(1, &e164), int2str(1, &e164), "voip_general_e164_country_code");
		} else {
			print_tab_mvalue(fd, "4: VoIP_configuration_method_used: ", 1, int2str(1, &c_sip_mode_g), int2str(1, &c_sip_mode_g), "voip_general_sip_mode");
		}
		cli_pots_calix_sip_mode_payload_type_mkey_value(fd, 1);
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meid==0xffff || meid==me->meid) {
			meinfo_me_refresh(me, get_attr_mask);
			print_tab_mvalue_meid(fd, "[53]pots", 0, me->meid);
			port_id_g = me->instance_num;
			print_parm_me_53(fd, me);
			dump_me_153(fd, me);
			dump_me_139(fd, me);
			cli_pots_related_line_status(fd, me);
			util_fdprintf(fd, "\n");
		}
	}
	print_tab_mvalue(fd, "SIP_user_data [classid 153][1]: ", 0, int2str(1, &ep_enable_ary_g[0]), int2str(1, &ep_enable_ary_g[0]), "voip_ep0_ep_enable");
	print_tab_mvalue(fd, "SIP_user_data [classid 153][2]: ", 0, int2str(1, &ep_enable_ary_g[1]), int2str(1, &ep_enable_ary_g[1]), "voip_ep1_ep_enable");
	util_fdprintf(fd, "\n");
	return CLI_OK;
}

static void
list_delete (struct meta_diff_s *meta_diff_p)
{
	list_del(&meta_diff_p->list);
	free_safe(meta_diff_p->key);
	free_safe(meta_diff_p->value);
	free_safe(meta_diff_p);
}

static void
list_free (void)
{
	struct meta_diff_s *meta_diff_p;
	while (meta_diff_list_g.next != &meta_diff_list_g) {
		meta_diff_p = list_entry(meta_diff_list_g.next, struct meta_diff_s, list);
		list_delete(meta_diff_p);
	}
}

static int
meta_diff_element_add_list (void * kv_config, const char *key, const char *value, void *data)
{
	int l, cmp, *is_over_w;
	struct meta_diff_s *meta_diff_p=NULL;
	struct list_head *ilist=NULL;

	/* dbprintf(LOG_DEBUG, "key[%s] value[%s]\n", (char *)key, (char *)value); */
	is_over_w=(int *)data;
	list_for_each(ilist, &meta_diff_list_g) {
		meta_diff_p = list_entry(ilist, struct meta_diff_s, list);
		if (meta_diff_p->key == NULL) {
			continue;
		}
		/* dbprintf(LOG_ERR, "meta_diff_p->key[%s], key[%s]\n", meta_diff_p->key, key); */
		if (0 == (cmp=strcmp(meta_diff_p->key, key))) {
			if (!*is_over_w) {
				continue;
			}
			dbprintf(LOG_ERR, "key duplicate to replace it [%s]:[%s]=>[%s]\n", key, meta_diff_p->value, value);
			if (meta_diff_p->value) {
				free_safe(meta_diff_p->value);
			}
			if (value && 0 < (l=strlen(value))) {
				meta_diff_p->value = (char *)malloc_safe(l+1);
				snprintf(meta_diff_p->value, l+1, "%s", value);
			} else {
				meta_diff_p->value = (char *)malloc_safe(1);
				*(meta_diff_p->value) = '\0';
			}
			return 0;
		} else if (cmp>0) {
			/* dbprintf(LOG_ERR, "to sort meta_diff_p->key[%s], key[%s]\n", meta_diff_p->key, key); */
			break;
		}
	}
	meta_diff_p = (struct meta_diff_s *)malloc_safe(sizeof(struct meta_diff_s));
	__list_add(&meta_diff_p->list, ilist->prev, ilist);
	if (key && 0 != (l=strlen(key))) {
		meta_diff_p->key = (char *)malloc_safe(l+1);
		snprintf(meta_diff_p->key, l+1, "%s", key);
	} else {
		meta_diff_p->key = (char *)malloc_safe(1);
		*(meta_diff_p->key) = '\0';
	}
	if (value && 0 != (l=strlen(value))) {
		meta_diff_p->value = (char *)malloc_safe(l+1);
		snprintf(meta_diff_p->value, l+1, "%s", value);
	} else {
		meta_diff_p->value = (char *)malloc_safe(1);
		*(meta_diff_p->value) = '\0';
	}
	return 0;
}

static void
meta_diff_list_copy_to_empty_kv (struct metacfg_t *kv, int is_over_w)
{
	struct meta_diff_s *meta_diff_p=NULL;
	struct list_head *ilist=NULL;

	/* dbprintf(LOG_ERR, "meta_diff_list_copy_to_empty_kv\n"); */
	list_for_each(ilist, &meta_diff_list_g) {
		meta_diff_p = list_entry(ilist, struct meta_diff_s, list);
		if (meta_diff_p->key == NULL) {
			continue;
		}
		/* dbprintf(LOG_ERR, "to add meta_diff_p->key[%s]\n", meta_diff_p->key); */
		metacfg_adapter_config_entity_add(kv, meta_diff_p->key, meta_diff_p->value, is_over_w);
	}
	list_free();
}

int
pots_mkey (unsigned short meid, struct metacfg_t *kv, int is_sort)
{
#if 1
	int is_over_w=1;
	char *p;
	meta_diff_g=1;
	

	//metacfg_adapter_config_init(&kv_metafile_g);
	metacfg_adapter_config_file_load_safe(kv, "/etc/wwwctrl/metafile.dat");
	p = metacfg_adapter_config_get_value(kv, "system_voip_management_protocol", 1);
	if (p != NULL && 0 != strlen(p) && '0' != *p) {
		dbprintf(LOG_ERR, "system_voip_management_protocol(%s)!=0, so voip setting is out of OMCI control\n", p);
	} else {
		int i;
		for (i=0; i<PORT_NUM; i++) {
			TCP_UDP_config_data_port_id_g[i] = PROXY_SERVER_PORT_DEFAULT;
			registrar_g[i][0] = '\0';
			proxy_g[i][0] = '\0';
			obproxy_g[i][0] = '\0';
			ep_enable_ary_g[i] = 0;
		}
		dump_me_53(0, meid);
		if (is_sort) {
			// sort kv_config to list for output easy reading
			metacfg_adapter_config_iterate(kv, meta_diff_element_add_list, &is_over_w);
			metacfg_adapter_config_release(kv);
			memset(kv, 0x0, sizeof(struct metacfg_t));
			metacfg_adapter_config_init(kv);
			meta_diff_list_copy_to_empty_kv(kv, 1);
		}
	}
	//metacfg_adapter_config_release(kv_metafile_g);
#else
	int is_over_w=1;
	char *p;
	meta_diff_g=1;
	kv_metadiff_g=kv;

	metacfg_adapter_config_init(&kv_metafile_g);
	metacfg_adapter_config_file_load_safe(&kv_metafile_g, "/etc/wwwctrl/metafile.dat");
	p = metacfg_adapter_config_get_value(&kv_metafile_g, "system_voip_management_protocol", 1);
	if (p != NULL && 0 != strlen(p) && '0' != *p) {
		dbprintf(LOG_ERR, "system_voip_management_protocol(%s)!=0, so voip setting is out of OMCI control\n", p);
	} else {
		int i;
		for (i=0; i<PORT_NUM; i++) {
			TCP_UDP_config_data_port_id_g[i] = PROXY_SERVER_PORT_DEFAULT;
			registrar_g[i][0] = '\0';
			proxy_g[i][0] = '\0';
			obproxy_g[i][0] = '\0';
			ep_enable_ary_g[i] = 0;
		}
		dump_me_53(0, meid);
		if (is_sort) {
			// sort kv_config to list for output easy reading
			metacfg_adapter_config_iterate(kv, meta_diff_element_add_list, &is_over_w);
			metacfg_adapter_config_release(kv);
			metacfg_adapter_config_init(kv);
			meta_diff_list_copy_to_empty_kv(kv, 1);
		}
	}
	metacfg_adapter_config_release(&kv_metafile_g);
#endif	
	return 0;
}
#endif

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////
int
cli_pots_mkey_me_153(unsigned short meid)
{
	if (meid==1) {
		me_153_created[0]=1;
	} else if (meid==2) {
		me_153_created[1]=1;
	}
	return 0;
}

int
cli_pots_mkey_cmdline(int fd, int argc, char *argv[])
{
	int ret=0;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv;

	memset(&kv, 0x0, sizeof(struct metacfg_t));
	if (argc > 0 && strcmp(argv[0], "pots_mkey")==0) {
		if (argc == 2 && strcmp(argv[1], "all")==0) {
			metacfg_adapter_config_init(&kv);
			meta_diff_g=0;
			kv_metadiff_g=&kv;
			metacfg_adapter_config_init(&kv_metafile_g);
			metacfg_adapter_config_file_load_safe(&kv_metafile_g, "/etc/wwwctrl/metafile.dat");
			ret = dump_me_53(fd, 0xffff);
			metacfg_adapter_config_release(&kv_metafile_g);
			metacfg_adapter_config_release(&kv);
		} else if (argc == 2 && strcmp(argv[1], "bat")==0) {
			ret=batchtab_cb_pots_is_hw_sync();
			switch(ret) {
			case BATCHTAB_POTS_MKEY_INIT:
				util_fdprintf(fd, "batchtab_cb status: init\n");
				break;
			case BATCHTAB_POTS_MKEY_GEN_TODO:
				util_fdprintf(fd, "batchtab_cb status: table_gen todo\n");
				break;
			case BATCHTAB_POTS_MKEY_GEN_DONE:
				util_fdprintf(fd, "batchtab_cb status: table_gen done\n");
				break;
			case BATCHTAB_POTS_MKEY_HW_SYNC_TODO:
				util_fdprintf(fd, "batchtab_cb status: hw_sync todo\n");
				break;
			case BATCHTAB_POTS_MKEY_HW_SYNC_DONE:
				util_fdprintf(fd, "batchtab_cb status: hw_sync done\n");
				break;
			case BATCHTAB_POTS_MKEY_FINISH:
				util_fdprintf(fd, "batchtab_cb status: finish\n");
				break;
			case BATCHTAB_POTS_MKEY_RELEASE:
				util_fdprintf(fd, "batchtab_cb status: release\n");
				break;
			default:
				util_fdprintf(fd, "batchtab_cb status: unkown(%d)\n", ret);
				break;
			}
			ret=0;
		} else {
			metacfg_adapter_config_init(&kv);
			meta_diff_g=1;
			ret=pots_mkey(0xffff, &kv, 1);
			if (!metacfg_adapter_config_is_empty(&kv)) {
				metacfg_adapter_config_dump(fd, &kv, NULL);
			}
			metacfg_adapter_config_release(&kv);
		}
	} else {
		ret = CLI_ERROR_OTHER_CATEGORY;
	}
#else
	ret = CLI_ERROR_OTHER_CATEGORY;
#endif
	return ret;
}
