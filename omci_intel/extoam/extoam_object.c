/*
 * =====================================================================================
 *
 *       Filename:  extoam_object.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  �褸2015�~03��12�� 18��09��05��
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ryan Tsai (), ryan_tsai@5vtechnologies.com
 *        Company:  5V technologies
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/wait.h>
#ifndef X86
#include "meinfo_hw.h"
#include "meinfo_hw_anig.h"
#endif
#include "util.h"
#include "util_run.h"
#include "cli.h"
#include "metacfg_adapter.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_object.h"
#include "tm.h"

extern struct extoam_link_status_t cpe_link_status_g;
static unsigned short MAYBE_UNUSED dgasp_vlan = 0xFFFF;

static unsigned short MAYBE_UNUSED
vlan_to_gem(unsigned short filter_vid, unsigned char filter_pbit, unsigned short *treatment_vid, unsigned char *treatment_pbit)
{
	// 171->84->47->(130)->266->268
	struct meinfo_t *miptr = meinfo_util_miptr(171);
	struct me_t *meptr_171 = NULL, *meptr_84 = NULL;
	int is_found = 0, ret = 0xFFFF;
	
	*treatment_vid = filter_vid;
	*treatment_pbit = filter_pbit;
	// 171
	list_for_each_entry(meptr_171, &miptr->me_instance_list, instance_node) {
		struct attr_table_header_t *table_header = meinfo_util_me_attr_ptr(meptr_171, 6);
		struct attr_table_entry_t *table_entry_pos = NULL;
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				unsigned short filter_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 36, 13);
				//unsigned char filter_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 32, 4);
				unsigned short treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 112, 13);
				//unsigned char treatment_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 108, 4);
				if(filter_inner_vid == filter_vid) {
					filter_vid = *treatment_vid = treatment_inner_vid;
					//filter_pbit = *treatment_pbit = treatment_inner_pbit;
					//dbprintf(LOG_ERR,"171=0x%04x\n",meptr_171->meid);
					break;
				}
			}
		}
	}
	
	// 171->84
	miptr = meinfo_util_miptr(84);
	list_for_each_entry(meptr_84, &miptr->me_instance_list, instance_node) {
		unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
		if((meinfo_util_me_attr_data(meptr_84, 3) == 1) &&
		    (filter_vid == util_bitmap_get_value(bitmap, 24*8, 4, 12))) {
			is_found = 1;
			//dbprintf(LOG_ERR,"84=0x%04x\n",meptr_84->meid);
			break;
		}
	}
	if(is_found) {
		// 84->47
		struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meptr_84->meid);
		unsigned char tp_type = 0;
		if(meptr_47) {
			tp_type = meinfo_util_me_attr_data(meptr_47, 3);
			//dbprintf(LOG_ERR,"47=0x%04x tp_type=%d\n",meptr_47->meid, tp_type);
		}
		if(meptr_47 && tp_type == 3) { // 802.1p
			// 47->130
			struct me_t *meptr_130 = meinfo_me_get_by_meid(130, meinfo_util_me_attr_data(meptr_47, 4));
			if(meptr_130) {
				//dbprintf(LOG_ERR,"130=0x%04x\n",meptr_130->meid);
				// 130->266
				struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_130 , 2+filter_pbit));
				if(meptr_266) {
					// 266->268
					struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
					//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
					if(meptr_268) {
						ret = meinfo_util_me_attr_data(meptr_268, 1);
					}
				}
			}
		} else if (meptr_47 && tp_type == 5) { // GEM IWTP
			// 47->266
			struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_47 , 4));
			if(meptr_266) {
				// 266->268
				struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
				//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
				if(meptr_268) {
					ret = meinfo_util_me_attr_data(meptr_268, 1);
				}
			}
		}
	}
	return ret;
}

unsigned short
current_firmware_version_get_handler( unsigned char *response_buff) {
	struct me_t *me_7_0, *me_7_1;
	unsigned char sw_active0, sw_active1;
	
	if (((me_7_0 = meinfo_me_get_by_instance_num(7, 0)) == NULL) ||
	     ((me_7_1 = meinfo_me_get_by_instance_num(7, 1)) == NULL))
	     return 0;
	
	sw_active0 = (unsigned char)meinfo_util_me_attr_data(me_7_0, 3);
	sw_active1 = (unsigned char)meinfo_util_me_attr_data(me_7_1, 3);
	
	if(sw_active0) {
		unsigned char *version_str = meinfo_util_me_attr_ptr(me_7_0, 1);
		memcpy(response_buff, version_str, strlen(version_str)+1 );
		return strlen(response_buff);
	} else if(sw_active1) {
		unsigned char *version_str = meinfo_util_me_attr_ptr(me_7_1, 1);
		memcpy(response_buff, version_str, strlen(version_str)+1 );
		return strlen(response_buff);
	} else
		return 0;
}

unsigned short
new_firmware_version_get_handler( unsigned char *response_buff) {
	struct me_t *me_7_0, *me_7_1;
	unsigned char sw_active0, sw_active1;
	
	if (((me_7_0 = meinfo_me_get_by_instance_num(7, 0)) == NULL) ||
	     ((me_7_1 = meinfo_me_get_by_instance_num(7, 1)) == NULL))
	     return 0;
	
	sw_active0 = (unsigned char)meinfo_util_me_attr_data(me_7_0, 3);
	sw_active1 = (unsigned char)meinfo_util_me_attr_data(me_7_1, 3);
	
	if(!sw_active0) {
		unsigned char *version_str = meinfo_util_me_attr_ptr(me_7_0, 1);
		memcpy(response_buff, version_str, strlen(version_str)+1 );
		return strlen(response_buff);
	} else if(!sw_active1) {
		unsigned char *version_str = meinfo_util_me_attr_ptr(me_7_1, 1);
		memcpy(response_buff, version_str, strlen(version_str)+1 );
		return strlen(response_buff);
	} else
		return 0;
}

unsigned short
registration_id_get_handler( unsigned char *response_buff) {
#ifndef X86
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	char *passwd_str;
	char *filename="/nvram/gpon.dat";
	
	if(metacfg_adapter_config_file_load_safe(&kv_config, filename ) !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}
	
	if(strlen(passwd_str = metacfg_adapter_config_get_value(&kv_config, "onu_password", 1)) > 0) {
		memcpy(response_buff, passwd_str, strlen(passwd_str)+1 );
	}

	metacfg_adapter_config_release(&kv_config);
	return strlen(response_buff);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

int
registration_id_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
#ifdef OMCI_METAFILE_ENABLE
	char *passwd_str;
	struct metacfg_t kv_config;
	char *exec_cmd = "/usr/sbin/nvram.sh commit gpon.dat";
	char *filename="/nvram/gpon.dat";
	int ret;


	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	if(metacfg_adapter_config_file_load_safe(&kv_config, filename) !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}
	
	passwd_str = (char *)buff;
	dbprintf(LOG_ERR, "key=%s\n","onu_password");
	dbprintf(LOG_ERR, "value=%s\n",passwd_str);
	metacfg_adapter_config_entity_update(&kv_config, "onu_password", passwd_str);
	metacfg_adapter_config_file_save(&kv_config, filename );
	metacfg_adapter_config_release(&kv_config);

	ret = util_run_by_system(exec_cmd);
	if ( WEXITSTATUS(ret) < 0 ) {
		dbprintf(LOG_ERR, "NVRAM gpon.dat error ret=%d!\n", WEXITSTATUS(ret));
		return -1;
	}

	dbprintf(LOG_ERR, "NVRAM gpon.dat finish!\n");
#endif
#endif
	return 0;
}

unsigned short
trap_en_get_handler( unsigned char *response_buff) {
	unsigned char *trap = (unsigned char *)response_buff;
	*trap = cpe_link_status_g.trap_state;
	return sizeof(unsigned char);
}

int
trap_en_set_handler(unsigned short buf_len, unsigned char *buff) {
	cpe_link_status_g.trap_state = (unsigned char) *buff;
	return 0;
}

unsigned short
temperature_high_warning_get_handler( unsigned char *response_buff) {
	unsigned short *temperature_high_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*temperature_high_warning = (unsigned short)meinfo_util_me_attr_data(me, 7);
#else
	*temperature_high_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
temperature_high_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *temperature_high_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *temperature_high_warning;
	meinfo_me_attr_set(me, 7, &attr, 0);
#endif
	return 0;
}

unsigned short
temperature_high_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *temperature_high_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*temperature_high_alarm = (unsigned short)meinfo_util_me_attr_data(me, 1);
#else
	*temperature_high_alarm = 0;
#endif
	return sizeof(unsigned short);
}

int
temperature_high_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *temperature_high_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *temperature_high_alarm;
	meinfo_me_attr_set(me, 1, &attr, 0);
#endif
	return 0;
}

unsigned short
temperature_low_warning_get_handler( unsigned char *response_buff) {
	unsigned short *temperature_low_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*temperature_low_warning = (unsigned short)meinfo_util_me_attr_data(me, 10);
#else
	*temperature_low_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
temperature_low_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *temperature_low_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *temperature_low_warning;
	meinfo_me_attr_set(me, 10, &attr, 0);
#endif
	return 0;
}

unsigned short
temperature_low_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *temperature_low_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*temperature_low_alarm = (unsigned short)meinfo_util_me_attr_data(me, 4);
#else
	*temperature_low_alarm = 0;
#endif
	return sizeof(unsigned short);
}

int
temperature_low_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *temperature_low_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *temperature_low_alarm;
	meinfo_me_attr_set(me, 4, &attr, 0);
#endif
	return 0;
}

unsigned short
voltage_high_warning_get_handler( unsigned char *response_buff) {
	unsigned short *voltage_high_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*voltage_high_warning = (unsigned short)meinfo_util_me_attr_data(me, 8);
#else
	*voltage_high_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
voltage_high_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *voltage_high_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *voltage_high_warning;
	meinfo_me_attr_set(me, 8, &attr, 0);
#endif
	return 0;
}

unsigned short
voltage_high_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *voltage_high_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*voltage_high_alarm = (unsigned short)meinfo_util_me_attr_data(me, 2);
#else
	*voltage_high_alarm = 0;
#endif
	return sizeof(unsigned short);
}

int
voltage_high_alarm_set_handler( unsigned short buff_len, unsigned char *buff) {
#ifndef X86
	unsigned short *voltage_high_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *voltage_high_alarm;
	meinfo_me_attr_set(me, 2, &attr, 0);
#endif
	return 0;
}

unsigned short
voltage_low_warning_get_handler( unsigned char *response_buff) {
	unsigned short *voltage_low_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*voltage_low_warning = (unsigned short)meinfo_util_me_attr_data(me, 11);
#else
	*voltage_low_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
voltage_low_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *voltage_low_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *voltage_low_warning;
	meinfo_me_attr_set(me, 11, &attr, 0);
#endif
	return 0;
}

unsigned short
voltage_low_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *voltage_low_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*voltage_low_alarm = (unsigned short)meinfo_util_me_attr_data(me, 5);
#else
	*voltage_low_alarm = 0;
#endif
	return sizeof(unsigned int);
}

int
voltage_low_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *voltage_low_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *voltage_low_alarm;
	meinfo_me_attr_set(me, 5, &attr, 0);
#endif
	return 0;
}

unsigned short
tx_power_high_warning_get_handler( unsigned char *response_buff) {
	unsigned char *tx_power_high_warning = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*tx_power_high_warning = (unsigned char)meinfo_util_me_attr_data(me, 15);
#else
	*tx_power_high_warning = 0;
#endif
	return sizeof(unsigned char);
}

int
tx_power_high_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *tx_power_high_warning = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *tx_power_high_warning;
	meinfo_me_attr_set(me, 15, &attr, 0);
#endif
	return 0;
}

unsigned short
tx_power_high_alarm_get_handler( unsigned char *response_buff) {
	unsigned char *tx_power_high_alarm = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	*tx_power_high_alarm = (unsigned char)meinfo_util_me_attr_data(me, 16);
#else
	*tx_power_high_alarm = 0;
#endif
	return sizeof(unsigned char);
}

int
tx_power_high_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *tx_power_high_alarm = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *tx_power_high_alarm;
	meinfo_me_attr_set(me, 16, &attr, 0);
#endif
	return 0;
}

unsigned short
tx_power_low_warning_get_handler( unsigned char *response_buff) {
	unsigned char *tx_power_low_warning = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*tx_power_low_warning = (unsigned char)meinfo_util_me_attr_data(me, 16);
#else
	*tx_power_low_warning = 0;
#endif
	return sizeof(unsigned char);
}

int
tx_power_low_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *tx_power_low_warning = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *tx_power_low_warning;
	meinfo_me_attr_set(me, 16, &attr, 0);
#endif
	return 0;
}

unsigned short
tx_power_low_alarm_get_handler( unsigned char *response_buff) {
	unsigned char *tx_power_low_alarm = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	*tx_power_low_alarm = (unsigned char)meinfo_util_me_attr_data(me, 15);
#else
	*tx_power_low_alarm = 0;
#endif
	return sizeof(unsigned char);
}

int
tx_power_low_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *tx_power_low_alarm = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *tx_power_low_alarm;
	meinfo_me_attr_set(me, 15, &attr, 0);
#endif
	return 0;
}

unsigned short
rx_power_high_warning_get_handler( unsigned char *response_buff) {
	unsigned char *rx_power_high_warning = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*rx_power_high_warning = (unsigned char)meinfo_util_me_attr_data(me, 13);
#else
	*rx_power_high_warning = 0;
#endif
	return sizeof(unsigned char);
}

int
rx_power_high_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *rx_power_high_warning = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *rx_power_high_warning;
	meinfo_me_attr_set(me, 13, &attr, 0);
#endif
	return 0;
}

unsigned short
rx_power_high_alarm_get_handler( unsigned char *response_buff) {
	unsigned char *rx_power_high_alarm = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	*rx_power_high_alarm = (unsigned char)meinfo_util_me_attr_data(me, 12);
#else
	*rx_power_high_alarm = 0;
#endif
	return sizeof(unsigned char);
}

int
rx_power_high_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *rx_power_high_alarm = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *rx_power_high_alarm;
	meinfo_me_attr_set(me, 12, &attr, 0);
#endif
	return 0;
}

unsigned short
rx_power_low_warning_get_handler( unsigned char *response_buff) {
	unsigned char *rx_power_low_warning = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*rx_power_low_warning = (unsigned char)meinfo_util_me_attr_data(me, 14);
#else
	*rx_power_low_warning = 0;
#endif
	return sizeof(unsigned char);
}

int
rx_power_low_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *rx_power_low_warning = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *rx_power_low_warning;
	meinfo_me_attr_set(me, 14, &attr, 0);
#endif
	return 0;
}

unsigned short
rx_power_low_alarm_get_handler( unsigned char *response_buff) {
	unsigned char *rx_power_low_warning = (unsigned char *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	*rx_power_low_warning = (unsigned char)meinfo_util_me_attr_data(me, 11);
#else
	*rx_power_low_warning = 0;
#endif
	return sizeof(unsigned char);
}

int
rx_power_low_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *rx_power_low_warning = (unsigned char *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(263, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned char) *rx_power_low_warning;
	meinfo_me_attr_set(me, 11, &attr, 0);
#endif
	return 0;
}

unsigned short
bias_current_high_warning_get_handler( unsigned char *response_buff) {
	unsigned short *bias_current_high_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*bias_current_high_warning = (unsigned short)meinfo_util_me_attr_data(me, 9);
#else
	*bias_current_high_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
bias_current_high_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *bias_current_high_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *bias_current_high_warning;
	meinfo_me_attr_set(me, 9, &attr, 0);
#endif
	return 0;
}

unsigned short
bias_current_high_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *bias_current_high_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*bias_current_high_alarm = (unsigned short)meinfo_util_me_attr_data(me, 3);
#else
	*bias_current_high_alarm = 0;
#endif
	return sizeof(unsigned short);
}

int
bias_current_high_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *bias_current_high_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *bias_current_high_alarm;
	meinfo_me_attr_set(me, 3, &attr, 0);
#endif
	return 0;
}

unsigned short
bias_current_low_warning_get_handler( unsigned char *response_buff) {
	unsigned short *bias_current_low_warning = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*bias_current_low_warning = (unsigned short)meinfo_util_me_attr_data(me, 12);
#else
	*bias_current_low_warning = 0;
#endif
	return sizeof(unsigned short);
}

int
bias_current_low_warning_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *bias_current_low_warning = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *bias_current_low_warning;
	meinfo_me_attr_set(me, 12, &attr, 0);
#endif
	return 0;
}

unsigned short
bias_current_low_alarm_get_handler( unsigned char *response_buff) {
	unsigned short *bias_current_low_alarm = (unsigned short *)response_buff;
#ifndef X86
	struct me_t *me = NULL;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	*bias_current_low_alarm = (unsigned short)meinfo_util_me_attr_data(me, 6);
#else
	*bias_current_low_alarm = 0;
#endif
	return sizeof(unsigned short);
}

int
bias_current_low_alarm_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned short *bias_current_low_alarm = (unsigned short *)buff;
	struct me_t *me = NULL;
	struct attr_value_t attr;
	
	if((me=meinfo_me_get_by_instance_num(65312, 0)) == NULL)
		return 0;
	
	attr.data = (unsigned short) *bias_current_low_alarm;
	meinfo_me_attr_set(me, 6, &attr, 0);
#endif
	return 0;
}

unsigned short
temperature_status_get_handler( unsigned char *response_buff) {
	int *temperature = (int *)response_buff;
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	int tmp;

	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;
	tmp = diag_result.temperature;
	*temperature = htonl(tmp);
#else
	*temperature = 0;	
#endif	
	return sizeof(int);
}

unsigned short
voltage_status_get_handler( unsigned char *response_buff) {
	int *voltage= (int *)response_buff;
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	int tmp;

	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;
	tmp = diag_result.vcc;
	*voltage = htonl(tmp*1000);
#else
	*voltage = 0;	
#endif
	return sizeof(int);
}

unsigned short
bias_current_status_get_handler( unsigned char *response_buff) {
	int *tx_bias_current = (int *)response_buff;
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	int tmp;

	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;
	tmp = diag_result.tx_bias_current;
	*tx_bias_current = htonl(tmp);
#else
	*tx_bias_current = 0;
#endif
	return sizeof(unsigned int);
}

unsigned short
tx_power_status_get_handler( unsigned char *response_buff) {
	int *tx_power = (int *)response_buff;
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	int tmp;

	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;
	tmp = diag_result.tx_power;
	*tx_power = htonl(tmp);
#else
	*tx_power = 0;
#endif
	return sizeof(unsigned int);
}

unsigned short
rx_power_status_get_handler( unsigned char *response_buff) {
	int *rx_power = (int *)response_buff;
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;
	int tmp;

	if (meinfo_hw_anig_get_threshold_result(omci_env_g.anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
   		return -1;
	tmp = diag_result.rx_power;
	*rx_power = htonl(tmp);
#else
	*rx_power = 0;
#endif
	return sizeof(unsigned int);
}

unsigned short
link_status_get_handler( unsigned char *response_buff) {
	unsigned char *link = (unsigned char *)response_buff;
#ifndef X86
	struct batchtab_cb_linkready_t *batchtab_cb_linkready=(struct batchtab_cb_linkready_t *)batchtab_table_data_get("linkready");
	*link = (batchtab_cb_linkready) ? htonl(batchtab_cb_linkready->uplink_is_ready) : 0;
#else
	*link = 0;
#endif
	batchtab_table_data_put("linkready");
	return sizeof(unsigned char);
}

unsigned short
serial_number_get_handler( unsigned char *response_buff) {
#ifndef X86
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	char *serial_str;
	char *filename="/nvram/gpon.dat";
	
	if(metacfg_adapter_config_file_load_safe(&kv_config, filename ) !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}
	
	if(strlen(serial_str = metacfg_adapter_config_get_value(&kv_config, "onu_serial", 1)) > 0) {
		memcpy(response_buff, serial_str, strlen(serial_str)+1 );
	}

	metacfg_adapter_config_release(&kv_config);
	return strlen(response_buff);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

int
reboot_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	int ret;
	
	dbprintf(LOG_ERR, "Reboot from extoam!\n");
	ret = util_run_by_system("reboot");
	if ( WEXITSTATUS(ret) < 0 ) {
		dbprintf(LOG_ERR, "reboot error ret=%d!\n", WEXITSTATUS(ret));
		return -1;
	}
#endif
	return 0;
}

unsigned short
eqd_get_handler( unsigned char *response_buff) {
#ifndef X86
	unsigned int *eqd = (unsigned int *)response_buff;
	FILE *fp = popen("cat /proc/eqd", "r");
	
	if(fp) {
		char buf[16];
		memset(buf, 0, sizeof(buf));
		while (fgets(buf, sizeof(buf), fp)) {
			if(strstr(buf, "eqd")) {
				strtok(buf, "=");
				*eqd = atoi(strtok(NULL, ""));
				break;
			}
		}
		pclose(fp);
	} else {
		*eqd = 0;
	}
	return sizeof(unsigned int);
#else
	return 0;
#endif
}

int
dying_gasp_data_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *dgasp_data_str, tmp[3];
	unsigned char dgasp_data[MAX_DATA_LENGTH], exec_cmd[MAX_DATA_LENGTH+32];
	int i, ret;
	
	dgasp_data_str = (char *)buff;
	memset(dgasp_data, 0, MAX_DATA_LENGTH);
	memset(exec_cmd, 0, MAX_DATA_LENGTH);
	memset(tmp, 0, 3);
	for(i=0; i<buf_len; i++) {
		sprintf(tmp, "%02x", dgasp_data_str[i]);
		strncat(dgasp_data, tmp, 2);
	}
	
	dbprintf(LOG_ERR, "dgasp_data=%s\n", dgasp_data);
	sprintf(exec_cmd, "echo dgasp_data %s > /proc/dgasp", dgasp_data);
	ret = util_run_by_system(exec_cmd);
	if ( WEXITSTATUS(ret) < 0 ) {
		dbprintf(LOG_ERR, "dgasp_data error ret=%d!\n", WEXITSTATUS(ret));
		return -1;
	}
#endif
	return 0;
}

unsigned short
dying_gasp_vlan_get_handler( unsigned char *response_buff) {
#ifndef X86
	unsigned short *vlan = (unsigned short *)response_buff;
	*vlan = dgasp_vlan;
	return sizeof(unsigned short);
#else
	return 0;
#endif
}

int
dying_gasp_vlan_set_handler(unsigned short buf_len, unsigned char *buff) {
#ifndef X86
	unsigned char *dgasp_vlan_str = (unsigned char *) buff;
	unsigned short dgasp_vid, treat_vid , gem_portid;
	unsigned char dgasp_pbit, treat_pbit;
	int ret;
	
	dgasp_pbit = (dgasp_vlan_str[0] >> 5) & 0x7;
	dgasp_vid = (dgasp_vlan_str[0] << 8 | dgasp_vlan_str[1]) & 0xFFF;
	dbprintf(LOG_ERR, "dgasp_pbit=%d, dgasp_vid=%d\n", dgasp_pbit, dgasp_vid);
	gem_portid = vlan_to_gem(dgasp_vid, dgasp_pbit, &treat_vid, &treat_pbit);
	if(gem_portid != 0xFFFF) {
		int gem_flowid = gem_flowid_get_by_gem_portid(gem_portid, 0);
		unsigned char exec_cmd[64];
		memset(exec_cmd, 0, sizeof(exec_cmd));
		dbprintf(LOG_ERR, "gem_flowid=%d\n", gem_flowid);
		sprintf(exec_cmd, "echo stream_id %d > /proc/dgasp", gem_flowid);
		ret = util_run_by_system(exec_cmd);
		if ( WEXITSTATUS(ret) < 0 ) {
			dbprintf(LOG_ERR, "dgasp_data error ret=%d!\n", WEXITSTATUS(ret));
			return -1;
		}
		dgasp_vlan = (treat_pbit << 13) | treat_vid;
	} else {
		dgasp_vlan = 0xFFFF;
		return -1;
	}
#endif
	return 0;
}

unsigned short
lof_get_handler( unsigned char *response_buff) {
#ifndef X86
	unsigned char *lof = (unsigned char *)response_buff;
	struct gpon_onu_alarm_event_t alarm_event;
	int ret = -1;
	
	if (gpon_hw_g.onu_alarm_event_get)
		ret = gpon_hw_g.onu_alarm_event_get(&alarm_event);
	
	*lof = (ret == 0) ? alarm_event.lof : 0;
	return sizeof(unsigned char);
#else
	return 0;
#endif
}

unsigned short
sf_get_handler( unsigned char *response_buff) {
#ifndef X86
	unsigned char *sf = (unsigned char *)response_buff;
	struct me_t *me;
	int sf_ind, sd_ind, ret;
	int sd_state = GPON_ONU_SDSF_UNDER, sf_state = GPON_ONU_SDSF_UNDER;
	
	if ((me = meinfo_me_get_by_instance_num(263, 0)) == NULL) {
		*sf = 0;
		return sizeof(unsigned char);
	}
	
	sf_ind = (char)meinfo_util_me_attr_data(me, 6);
	sd_ind = (char)meinfo_util_me_attr_data(me, 7);
        
	if (gpon_hw_g.onu_sdsf_get)
		ret = gpon_hw_g.onu_sdsf_get(sd_ind, sf_ind, &sd_state, &sf_state);
	
	if(ret == 0) {
		*sf = (sf_state==GPON_ONU_SDSF_OVER) ? 1 : 0;
	} else {
		*sf = 0;
	}
	return sizeof(unsigned char);
#else
	return 0;
#endif
}

unsigned short
sd_get_handler( unsigned char *response_buff) {
#ifndef X86
	unsigned char *sd = (unsigned char *)response_buff;
	struct me_t *me;
	int sf_ind, sd_ind, ret;
	int sd_state = GPON_ONU_SDSF_UNDER, sf_state = GPON_ONU_SDSF_UNDER;
	
	if ((me = meinfo_me_get_by_instance_num(263, 0)) == NULL) {
		*sd = 0;
		return sizeof(unsigned char);
	}
	
	sf_ind = (char)meinfo_util_me_attr_data(me, 6);
	sd_ind = (char)meinfo_util_me_attr_data(me, 7);
        
	if (gpon_hw_g.onu_sdsf_get)
		ret = gpon_hw_g.onu_sdsf_get(sd_ind, sf_ind, &sd_state, &sf_state);
	
	if(ret == 0) {
		*sd = (sd_state==GPON_ONU_SDSF_OVER) ? 1 : 0;
	} else {
		*sd = 0;
	}
	return sizeof(unsigned char);
#else
	return 0;
#endif
}

struct object_handler_t object_handles[]= {
	{0x1, 0, current_firmware_version_get_handler ,NULL},
	{0x2, 0, new_firmware_version_get_handler ,NULL},
	{0x3, 0, registration_id_get_handler, registration_id_set_handler},
	{0x4, 0, trap_en_get_handler, trap_en_set_handler },
	{0x5, 0, temperature_high_warning_get_handler, temperature_high_warning_set_handler },
	{0x6, 0, temperature_high_alarm_get_handler, temperature_high_alarm_set_handler },
	{0x7, 0, temperature_low_warning_get_handler, temperature_low_warning_set_handler },
	{0x8, 0, temperature_low_alarm_get_handler, temperature_low_alarm_set_handler },
	{0x9, 0, voltage_high_warning_get_handler, voltage_high_warning_set_handler },
	{0xa, 0, voltage_high_alarm_get_handler, voltage_high_alarm_set_handler },
	{0xb, 0, voltage_low_warning_get_handler, voltage_low_warning_set_handler },
	{0xc, 0, voltage_low_alarm_get_handler, voltage_low_alarm_set_handler },
	{0xd, 0, tx_power_high_warning_get_handler, tx_power_high_warning_set_handler },
	{0xe, 0, tx_power_high_alarm_get_handler, tx_power_high_alarm_set_handler },
	{0xf, 0, tx_power_low_warning_get_handler, tx_power_low_warning_set_handler },
	{0x10, 0, tx_power_low_alarm_get_handler, tx_power_low_alarm_set_handler },
	{0x11, 0, rx_power_high_warning_get_handler, rx_power_high_warning_set_handler },
	{0x12, 0, rx_power_high_alarm_get_handler, rx_power_high_alarm_set_handler },
	{0x13, 0, rx_power_low_warning_get_handler, rx_power_low_warning_set_handler },
	{0x14, 0, rx_power_low_alarm_get_handler, rx_power_low_alarm_set_handler },
	{0x15, 0, bias_current_high_warning_get_handler, bias_current_high_warning_set_handler },
	{0x16, 0, bias_current_high_alarm_get_handler, bias_current_high_alarm_set_handler },
	{0x17, 0, bias_current_low_warning_get_handler, bias_current_low_warning_set_handler },
	{0x18, 0, bias_current_low_alarm_get_handler, bias_current_low_alarm_set_handler },
	{0x19, 0, temperature_status_get_handler, NULL},
	{0x1a, 0, voltage_status_get_handler, NULL},
	{0x1b, 0, bias_current_status_get_handler, NULL},
	{0x1c, 0, tx_power_status_get_handler, NULL},
	{0x1d, 0, rx_power_status_get_handler, NULL},
	{0x1e, 0, link_status_get_handler, NULL},
	{0x1f, 0, serial_number_get_handler, NULL},
	{0x20, 0, NULL, reboot_set_handler},
	{0x21, 0, eqd_get_handler, NULL},
	{0x22, 0, NULL, dying_gasp_data_set_handler},
	{0x23, 0, dying_gasp_vlan_get_handler, dying_gasp_vlan_set_handler},
	{0x24, 0, lof_get_handler, NULL},
	{0x25, 0, sf_get_handler, NULL},
	{0x26, 0, sd_get_handler, NULL},
	{ 0, 0, NULL, NULL }
};

