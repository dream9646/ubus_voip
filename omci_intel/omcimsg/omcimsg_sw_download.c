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
 * Module  : omcimsg
 * File    : omcimsg_sw_download.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include "env.h"
#include "util.h"
#include "util_run.h"
#include "crc.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "hwresource.h"
#include "er_group.h"
#include "coretask.h"
#include "omcimain.h"
#include "proprietary_alu_sw_download.h"

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "switch_hw_prx126.h"
#include "pon_img/pon_img.h"
#include "pon_adapter.h"
#include "pon_adapter_debug_common.h"
#include <errno.h>
#endif

//#define SW_NONBLOCK
#define VALID_CHK

#define PATH_LEN	256
#define STRING_LEN	256 
#define TOKEN_LEN	32

//end software download behavior

#define DEV_BUSY_AND_BG_UPDATE		0	//return device busy and update flash in background
#define DEV_OK_AND_BG_UPDATE		1 	//return ok immediately and update flash in background
#define WAIT_AND_FG_UPDATE		2	//wait update flash in foreground, return ok after finish


// Figure 9.1.4-1 Software image state diagram
#define	START_DOWNLOAD_0		100	
#define	START_DOWNLOAD_1		101	
#define	DOWNLOAD_SECTION_0		102	
#define	DOWNLOAD_SECTION_1		103	
#define	END_DOWNLOAD_0			104	
#define	END_DOWNLOAD_1			105	
#define	END_DOWNLOAD_INCORRECT_CRC_0	106	
#define	END_DOWNLOAD_INCORRECT_CRC_1	107	
#define	ACTIVATE_0			108	
#define	ACTIVATE_1			109	
#define	COMMIT_0			110	
#define	COMMIT_1			111	
#define	REBOOT				112	/*actually, this never happen*/
#define	DO_NOTHING			150
#define	S1	1
#define	S2	2
#define	S3	3
#define	S4	4
#define	S1P	5
#define	S2P	6
#define	S3P	7
#define	S4P	8

/*
u-boot-env parameter: 

sw_commit	image0	image1
--------------------------
0		1	0
1		0	1

sw_tryactive 	image0	image1
--------------------------
0		1	0	boot image0
1		0	1	boot image1
2		according commit flag

sw_valid0	image0	image1
--------------------------
0		0	X
1		1	X

sw_valid1	image0	image1
--------------------------
0		X	0
1		X	1

*/

struct system_param_t
{
	unsigned char instance_id; 
        char exec_cmd[PATH_LEN];
};

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
struct system_intel_param_t
{
	unsigned char instance_id; 
    char file_path[PATH_LEN];
};
#endif

struct instance_id_t {
	unsigned char ms;
	unsigned char ls;
};

static unsigned short last_download_section_tid = 0;

struct download_info_t {
	unsigned char image_target;	//0:ONT-G, 1..254 slot number, but current version only support 0
	unsigned char instance_id; 
	unsigned char window_size_minus1;	/* window size - 1 */
	unsigned char *section_map;
	unsigned int image_size_in_bytes;
	unsigned int last_window_end;
	unsigned char update_state;	/**/
	unsigned char sw_state, active0_ok, active1_ok;
	time_t	download_start_time;
	int image_fd;
};

struct download_info_t download_info;
static struct system_param_t system_end_sw_download, system_activate_image, system_commit_image;
static unsigned char is_active_after_end_sw_dl; 
unsigned int global_crc_accum_be = 0xFFFFFFFF;
unsigned int global_crc_accum_le = 0xFFFFFFFF;

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
extern struct switch_hw_t switch_hw_fvt_g;
static struct system_intel_param_t intel_end_sw_download, intel_active_data;
#ifdef SW_NONBLOCK
static struct system_intel_param_t intel_commit_data;
#endif
#endif

int
omcimsg_sw_download_dump_state(int fd)
{
	char state_str[][4]={"S1","S2","S3","S4","S1P","S2P","S3P","S4P"};
	unsigned int duration;

	duration=difftime(time(0),download_info.download_start_time);
	if (duration==0)
		duration=1;
	if(download_info.update_state == OMCIMSG_SW_UPDATE_DOWNLOAD)
		util_fdprintf(fd, "update state is software downloading\n");
	else if (download_info.update_state == OMCIMSG_SW_UPDATE_FLASH_WRITE)
		util_fdprintf(fd, "update state is flash updating\n");
	else	
		util_fdprintf(fd, "update state is normal\n");

	util_fdprintf(fd, "instance_id=%u\n", download_info.instance_id);
	util_fdprintf(fd, "window size minus1=%u\n", download_info.window_size_minus1);
	util_fdprintf(fd, "image_size_in_bytes=%u\n", download_info.image_size_in_bytes);
	util_fdprintf(fd, "last_window_end=%u(%u Byte/Sec)\n", download_info.last_window_end, download_info.last_window_end/duration );
	util_fdprintf(fd, "active0_ok=%u\n", download_info.active0_ok);
	util_fdprintf(fd, "active1_ok=%u\n", download_info.active1_ok);
	util_fdprintf(fd, "sw image state=%s, update_state=%d\n", state_str[download_info.sw_state-1], download_info.update_state);

	return 0;
}

int
omcimsg_sw_download_get_diagram_state()
{
	return download_info.sw_state;
}

void
omcimsg_sw_download_set_diagram_state(unsigned char val)
{
	download_info.sw_state=val;
}

int 
omcimsg_sw_download_init_diagram_state(struct me_t *me)
{
	struct attr_value_t attr;
	struct me_t *private_me;
	static unsigned char active0, active0_ok, commit0, valid0, active1, active1_ok, commit1, valid1;

	private_me=hwresource_public2private(me);
	if (private_me==NULL) {
		dbprintf(LOG_ERR, "Can't found related private me?\n");
		return -1;
	}

	if( me->instance_num == 0) {
		meinfo_me_attr_get(me, 2, &attr);
		commit0=attr.data;
		meinfo_me_attr_get(me, 3, &attr);
		active0=attr.data;
		meinfo_me_attr_get(me, 4, &attr);
		valid0=attr.data;
		active0_ok=meinfo_util_me_attr_data(private_me, 4);

	} else if( me->instance_num == 1) {
		meinfo_me_attr_get(me, 2, &attr);
		commit1=attr.data;
		meinfo_me_attr_get(me, 3, &attr);
		active1=attr.data;
		meinfo_me_attr_get(me, 4, &attr);
		valid1=attr.data;
		active1_ok=meinfo_util_me_attr_data(private_me, 4);
	}

#if 0
	dbprintf(LOG_ERR, "active0=%d, active1=%d\n", active0, active1);
	dbprintf(LOG_ERR, "commit0=%d, commit1=%d\n", commit0, commit1);
	dbprintf(LOG_ERR, "valid0=%d, valid1=%d\n", valid0, valid1);
	dbprintf(LOG_ERR, "active0_ok=%d, active1_ok=%d\n", active0_ok, active1_ok);
#endif

	if( active0 ==1 && commit0 ==1 && valid0 ==1 ) {
		if( active1 ==0 && commit1 ==0 && valid1 ==1 ) {
			download_info.sw_state=S3;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S3\n");
		} else if( active1 ==0 && commit1 ==0 && valid1 ==0 ) {
			download_info.sw_state=S1;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S1\n");
		}
	} else if( active1 ==1 && commit1 ==1 && valid1 ==1 ) {
		if( active0 ==0 && commit0 ==0 && valid0 ==1 ) {
			download_info.sw_state=S3P;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S3P\n");
		} else if( active0 ==0 && commit0 ==0 && valid0 ==0 ) {
			download_info.sw_state=S1P;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S1P\n");
		}
	} else if( active0 ==0 && commit0 ==1 && valid0 ==1 && valid1 ==1) {
			download_info.sw_state=S4;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S4\n");
	} else if( active0 ==1 && commit0 ==0 && valid0 ==1 && valid1 ==1) {
			download_info.sw_state=S4P;
			dbprintf(LOG_NOTICE, "download_info.sw_state=S4P\n");
	} else {
		dbprintf(LOG_NOTICE, "Init sw_image_state not finish!\n");
	}

	download_info.active0_ok=active0_ok;
	download_info.active1_ok=active1_ok;
	download_info.update_state=OMCIMSG_SW_UPDATE_NORMAL;

	//JKU:debug info
	//dbprintf(LOG_ERR, "download_info.sw_state=[%d]\n", download_info.sw_state);
	//dbprintf(LOG_ERR, "download_info.active0_ok=[%d],active1_ok=[%d]\n", download_info.active0_ok, download_info.active1_ok);
	return 0;
}

int 
sw_image_state(unsigned char event)
{
	struct attr_value_t attr_zero, attr_one;
	unsigned char attr_mask[2];  	
	struct me_t *me_image0=meinfo_me_get_by_instance_num(7, 0);
	struct me_t *me_image1=meinfo_me_get_by_instance_num(7, 1);

	attr_zero.data=0; attr_zero.ptr=NULL;
	attr_one.data=1; attr_one.ptr=NULL;

	memset(attr_mask, 0x00, sizeof(attr_mask));
	switch(download_info.sw_state) {
		case S1:
			switch(event) {
				case START_DOWNLOAD_1:
					download_info.sw_state=S2;
					dbprintf(LOG_NOTICE, "sw_state change to S2\n");
					break;
				case REBOOT:
				case ACTIVATE_0:
				case COMMIT_0:
					dbprintf(LOG_NOTICE, "S1 receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_NOTICE, "S1 receive error event %d!\n", event);
					return -1;
			}
			break;
		case S1P:
			switch(event) {
				case START_DOWNLOAD_0:
					download_info.sw_state=S2P;
					dbprintf(LOG_NOTICE, "sw_state change to S2P\n");
					break;
				case REBOOT:
				case ACTIVATE_1:
				case COMMIT_1:
					dbprintf(LOG_NOTICE, "S1P receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_NOTICE, "S1P receive error event %d!\n", event);
					return -1;
			}
			break;
		case S2:
			switch(event) {
				case END_DOWNLOAD_1:
					download_info.sw_state=S3;
					dbprintf(LOG_NOTICE, "sw_state change to S3\n");
					//todo change image1 version
					//set image1_valid=1
					meinfo_me_attr_set(me_image1, 4, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 4);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_VALID1, 1);
					break;
				case REBOOT:
					//actually never come here
					download_info.sw_state=S1;
					dbprintf(LOG_NOTICE, "sw_state change to S1\n");
					break;
				case END_DOWNLOAD_INCORRECT_CRC_1:
					download_info.sw_state=S1;
					dbprintf(LOG_NOTICE, "sw_state change to S1\n");
					break;
				case ACTIVATE_0:
				case COMMIT_0:
				case START_DOWNLOAD_1:
				case DOWNLOAD_SECTION_1:
					dbprintf(LOG_NOTICE, "S2 receive event %d, state unchanging\n", event);
					return DO_NOTHING;
					break;
				default:
					dbprintf(LOG_ERR, "S2 receive error event %d!\n", event);
					return -1;
			}
			break;
		case S2P:
			switch(event) {
				case END_DOWNLOAD_0:
					download_info.sw_state=S3P;
					dbprintf(LOG_NOTICE, "sw_state change to S3P\n");
					//todo change image0 version
					//set image0_valid=1
					meinfo_me_attr_set(me_image0, 4, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 4);
					meinfo_me_flush(me_image0, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_VALID0, 1);
					break;
				case REBOOT:
					//actually never come here
					download_info.sw_state=S1P;
					dbprintf(LOG_NOTICE, "sw_state change to S1P\n");
					break;
				case END_DOWNLOAD_INCORRECT_CRC_0:
					download_info.sw_state=S1P;
					dbprintf(LOG_NOTICE, "sw_state change to S1P\n");
					break;
				case ACTIVATE_1:
				case COMMIT_1:
				case START_DOWNLOAD_0:
				case DOWNLOAD_SECTION_0:
					dbprintf(LOG_NOTICE, "S2P receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_ERR, "S2P receive error event %d!\n", event);
					return -1;
			}
			break;
		case S3:
			switch(event) {
				case START_DOWNLOAD_1:
					download_info.sw_state=S2;
					dbprintf(LOG_NOTICE, "sw_state change to S2\n");
					//todo change image1 version
					//set image1_valid=0
					meinfo_me_attr_set(me_image1, 4, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 4);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_VALID1, 0);
					break;
				case COMMIT_1:
					download_info.sw_state=S4P;
					dbprintf(LOG_NOTICE, "sw_state change to S4P\n");

					//set image0_commit=0
					meinfo_me_attr_set(me_image0, 2, &attr_zero, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 2);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_commit=1
					meinfo_me_attr_set(me_image1, 2, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 2);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_COMMIT, 0);
					break;
				case ACTIVATE_1:
					download_info.sw_state=S4;
					dbprintf(LOG_NOTICE, "sw_state change to S4\n");

					//set image0_activate=0
					meinfo_me_attr_set(me_image0, 3, &attr_zero, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 3);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_activate=1
					meinfo_me_attr_set(me_image1, 3, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 3);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_TRYACTIVE, 0);
					break;
				case REBOOT:
				case COMMIT_0:
				case ACTIVATE_0:
					dbprintf(LOG_NOTICE, "S3 receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_ERR, "S3 receive error event %d!\n", event);
					return -1;
			}
			break;
		case S3P:
			switch(event) {
				case START_DOWNLOAD_0:
					download_info.sw_state=S2P;
					dbprintf(LOG_NOTICE, "sw_state change to S2P\n");
					//todo change image0 version
					//set image0_valid=0
					meinfo_me_attr_set(me_image0, 4, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 4);
					meinfo_me_flush(me_image0, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_VALID0, 0);
					break;
				case COMMIT_0:
					download_info.sw_state=S4;
					dbprintf(LOG_NOTICE, "sw_state change to S4\n");
					//set image0_commit=1
					meinfo_me_attr_set(me_image0, 2, &attr_one, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 2);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_commit=0
					meinfo_me_attr_set(me_image1, 2, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 2);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_COMMIT, 1);
					break;
				case ACTIVATE_0:
					download_info.sw_state=S4P;
					dbprintf(LOG_NOTICE, "sw_state change to S4P\n");
					//set image0_activate=1
					meinfo_me_attr_set(me_image0, 3, &attr_one, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 3);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_activate=0
					meinfo_me_attr_set(me_image1, 3, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 3);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_TRYACTIVE, 1);
					break;
				case REBOOT:
				case COMMIT_1:
				case ACTIVATE_1:
					dbprintf(LOG_NOTICE, "S3P receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_ERR, "S3P receive error event %d!\n", event);
					return -1;
			}
			break;
		case S4:
			switch(event) {
				case ACTIVATE_0:
					download_info.sw_state=S3;
					dbprintf(LOG_NOTICE, "sw_state change to S3\n");
					//set image0_activate=1
					meinfo_me_attr_set(me_image0, 3, &attr_one, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 3);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_activate=0
					meinfo_me_attr_set(me_image1, 3, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 3);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_TRYACTIVE, 1);
					break;
				case COMMIT_1:
					download_info.sw_state=S3P;
					dbprintf(LOG_NOTICE, "sw_state change to S3P\n");
					//set image0_commit=0
					meinfo_me_attr_set(me_image0, 2, &attr_zero, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 2);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_commit=1
					meinfo_me_attr_set(me_image1, 2, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 2);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_COMMIT, 0);
					break;
				case REBOOT:
					//actually never come here
					download_info.sw_state=S3;
					dbprintf(LOG_NOTICE, "sw_state change to S3\n");
					break;
				case ACTIVATE_1:
				case COMMIT_0:
					dbprintf(LOG_NOTICE, "S4 receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_ERR, "S4 receive error event %d!\n", event);
					return -1;
			}
			break;
		case S4P:
			switch(event) {
				case ACTIVATE_1:
					download_info.sw_state=S3P;
					dbprintf(LOG_NOTICE, "sw_state change to S3P\n");
					//set image0_activate=0
					meinfo_me_attr_set(me_image0, 3, &attr_zero, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 3);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_activate=1
					meinfo_me_attr_set(me_image1, 3, &attr_one, 0);
					util_attr_mask_set_bit(attr_mask, 3);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_TRYACTIVE, 0);
					break;
				case COMMIT_0:
					download_info.sw_state=S3;
					dbprintf(LOG_NOTICE, "sw_state change to S3\n");
					//set image0_commit=1
					meinfo_me_attr_set(me_image0, 2, &attr_one, 0);
					//call er_attr_group once is enought
					//util_attr_mask_set_bit(attr_mask, 2);
					//meinfo_me_flush(me_image0, attr_mask);

					//set image1_commit=0
					meinfo_me_attr_set(me_image1, 2, &attr_zero, 0);
					util_attr_mask_set_bit(attr_mask, 2);
					meinfo_me_flush(me_image1, attr_mask);
					//update u-boot env
					//meinfo_hw_set_bootenv(SW_COMMIT, 1);
					break;
				case REBOOT:
					//actually never come here
					download_info.sw_state=S3P;
					dbprintf(LOG_NOTICE, "sw_state change to S3\n");
					break;
				case ACTIVATE_0:
				case COMMIT_1:
					dbprintf(LOG_NOTICE, "S4P receive event %d, state unchanging\n", event);
					return DO_NOTHING;
				default:
					dbprintf(LOG_ERR, "S4P receive error event %d!\n", event);
					return -1;
			}
			break;
		default:
			dbprintf(LOG_ERR, "Error state!\n");
			return -1;
	}
	return 0;
}

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
static char part_get(const uint8_t id)
{
    switch (id) {
    case 0:
        return 'A';
    case 1:
        return 'B';
    default:
        dbprintf(LOG_ERR,"OMCI specified wrong partition number! Using default.\n");
        return 'A';
    }
}

static size_t getFilesize(const char *filepath)
{
    struct stat st;

    if (stat(filepath, &st) != 0)
        return 0;

    return st.st_size;
}

void *
intel_commit_command(void *data)
{
	struct system_intel_param_t *val = (struct system_intel_param_t *)data;
	int ret = 0;

	switch_hw_fvt_g.sw_image_commit_set(val->instance_id);
	dbprintf(LOG_ERR, "Commit Set!!\n");
	return (void *)WEXITSTATUS(ret);
}

void *
intel_active_command(void *data)
{
	struct system_intel_param_t *val = (struct system_intel_param_t *)data;
	int ret = 0;

	switch_hw_fvt_g.sw_image_active_set(val->instance_id, 5000);
	dbprintf(LOG_ERR, "Active Set!!\n");
	sleep(10);
	util_run_by_system("reboot");
	return (void *)WEXITSTATUS(ret);
}

void *
intel_store_command(void *data)
{
	struct system_intel_param_t *val = (struct system_intel_param_t *)data;
	//int ret;
	int fd;
	enum pon_adapter_errno ret = 0;


    pon_img_valid_set(part_get(val->instance_id), false);
	
	/*
    if (ret != PON_ADAPTER_SUCCESS){
		dbprintf(LOG_ERR, "ERROR:%d\n", ret);
		return (void *)WEXITSTATUS(ret);
    }
	*/

	fd = open(val->file_path, O_RDONLY);
    if (fd < 0) {
        dbprintf(LOG_ERR,"Could not open file \"%s\": %s\n", val->file_path, strerror(errno));
        ret = PON_ADAPTER_ERROR;
        return (void *)WEXITSTATUS(ret);
    }

    pon_img_upgrade(part_get(val->instance_id), fd, getFilesize(val->file_path));

    close(fd);
	//dbprintf(LOG_ERR, "MSG: Temp File Path:%s, Instance ID:%d Return:%d\n", val->file_path, val->instance_id, ret);
	//ret = switch_hw_fvt_g.sw_image_download_store(val->instance_id, strlen(val->file_path)+1, val->file_path);

	/*
	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "MSG: write to flash fail, w_size_minus1=%d\n", download_info.window_size_minus1);
		download_info.update_state=OMCIMSG_SW_UPDATE_FLASH_WRITE_FAIL;
		return (void *)WEXITSTATUS(ret);
	}
	*/

	if( is_active_after_end_sw_dl == 1) {
		//ret = switch_hw_fvt_g.sw_image_active_set(val->instance_id, 3000);//timeout, ms
		dbprintf(LOG_ERR, "MSG: Active image after download!\n");
		switch_hw_fvt_g.sw_image_active_set(val->instance_id, 3000);//timeout, ms
		
		if( WEXITSTATUS(ret) != 0 ) {
			dbprintf(LOG_ERR, "MSG: End write to flash; active image fail\n");
			download_info.update_state=OMCIMSG_SW_UPDATE_ACTIVE_IMAGE_FAIL;
		}
		//return ret;
	}

    pon_img_valid_set(part_get(val->instance_id), true);
	download_info.update_state=OMCIMSG_SW_UPDATE_NORMAL;
	dbprintf(LOG_ERR, "MSG: End write to flash!\n");
	return (void *)WEXITSTATUS(ret);
}
#else
void *
start_system_command(void *data)
{
	struct system_param_t *val = (struct system_param_t *)data;
	int ret;

	ret=util_run_by_system(val->exec_cmd);
	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "start_system_command [%s]!\n", val->exec_cmd);
		dbprintf(LOG_ERR, "start_system_command error ret=%d!\n", WEXITSTATUS(ret));
	}

	if (strstr(val->exec_cmd, "end_download") != NULL) {
		if( WEXITSTATUS(ret) != 0 ) {
			dbprintf(LOG_ERR, "MSG: write to flash fail, w_size_minus1=%d\n", download_info.window_size_minus1);
			download_info.update_state=OMCIMSG_SW_UPDATE_FLASH_WRITE_FAIL;
			return (void *)WEXITSTATUS(ret);
		}
		if( is_active_after_end_sw_dl == 1) {
			ret=util_run_by_system(system_activate_image.exec_cmd);
			if( WEXITSTATUS(ret) != 0 ) {
				dbprintf(LOG_ERR, "MSG: End write to flash; active image fail[%s]\n", system_activate_image.exec_cmd);
				download_info.update_state=OMCIMSG_SW_UPDATE_ACTIVE_IMAGE_FAIL;
			}
		}

		dbprintf(LOG_ERR, "MSG: End write to flash, w_size_minus1=%d\n", download_info.window_size_minus1);
		download_info.update_state=OMCIMSG_SW_UPDATE_NORMAL;
	}

	return (void *)WEXITSTATUS(ret);
}
#endif

int 
omcimsg_sw_download_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_start_software_download_t *start_sw_download;
	struct omcimsg_start_software_download_response_t *start_sw_download_response;
	struct instance_id_t *instance_id;
	int ret;
#ifdef VALID_CHK
	struct attr_value_t attr_commit, attr_active;
	struct me_t *me;
#endif
	char exec_cmd[PATH_LEN];
	
	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	start_sw_download_response = (struct omcimsg_start_software_download_response_t *) resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 7) {	//sw image
		start_sw_download_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "classid should be sw image\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0 || instance_id->ls == 255 ) {
		start_sw_download_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	if( download_info.update_state == OMCIMSG_SW_UPDATE_DOWNLOAD) {
		dbprintf(LOG_ERR, "OLT already ask ONT start softwre download !");
		dbprintf(LOG_ERR, "But OLT ask again, so ONT reset all download process again!");
		if(download_info.image_fd > 0) {
			close(download_info.image_fd);
		}
		//init parameter
		download_info.image_fd=-1;
		download_info.last_window_end=0;
		download_info.download_start_time=0;

		if ( download_info.section_map ) {
			free_safe(download_info.section_map);
			download_info.section_map=NULL;
		}
	} else if( download_info.update_state == OMCIMSG_SW_UPDATE_FLASH_WRITE) {
		dbprintf(LOG_ALERT, "ONT during flash write state!, we reject start a new software download");
		start_sw_download_response->result_reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
		return OMCIMSG_ERROR_ATTR_FAILED_UNKNOWN;
	}

	//information will be reference later
	download_info.image_target=instance_id->ms;	//ONT-G
	download_info.instance_id=instance_id->ls;	//0 or 1

#ifdef VALID_CHK
	// check if action is valid
	me=meinfo_me_get_by_instance_num(7,download_info.instance_id);
	meinfo_me_attr_get(me, 2, &attr_commit);	//image's commit
	meinfo_me_attr_get(me, 3, &attr_active);	//image's active

	if( attr_commit.data == 1 || attr_active.data == 1 ) {
		start_sw_download_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "Action is invalid: Current value commit=%d active=%d\n", attr_commit.data, attr_active.data);
		dbprintf(LOG_ERR, "But update instance commit and active should be both 0\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}
#endif
	is_active_after_end_sw_dl=0;
	start_sw_download = (struct omcimsg_start_software_download_t *) req_msg->msg_contents;
	download_info.window_size_minus1=start_sw_download->window_size_minus1;	/* window size - 1 */
	download_info.image_size_in_bytes=ntohl(start_sw_download->image_size_in_bytes);
	download_info.last_window_end=0;

//ENV_SIZE_PATH
#if 0
	if( (download_info.image_fd=open(omci_env_g.sw_download_image_name, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0 ){
		dbprintf(LOG_ERR, "Error file open!\n");
		start_sw_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
        }
#endif

	download_info.update_state=OMCIMSG_SW_UPDATE_DOWNLOAD;	//normal case:0, start download process:1;
	//we may skip follow information in current version (value maybe 1,1, 0..1)
	/* start_sw_download->number_of_update;
	start_sw_download->target[0].image_slot;
	start_sw_download->target[0].image_instance; */

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	strcpy(exec_cmd, "[ -f /var/run/swimage ] || mkdir -p /var/run/swimage");
	util_run_by_system(exec_cmd);
#endif

#if 1
	/*DEBUG to console*/
	dbprintf(LOG_ERR, "image_target=%d\n", download_info.image_target);
	dbprintf(LOG_ERR, "instance_id=%d\n", download_info.instance_id);
	dbprintf(LOG_ERR, "window_size_minus1=%d\n", download_info.window_size_minus1);
	dbprintf(LOG_ERR, "image_size_in_bytes=%u\n", download_info.image_size_in_bytes);
	dbprintf(LOG_ERR, "update_state=%d\n", download_info.update_state);
	dbprintf(LOG_ERR, "start_sw_download->number_of_update=%d\n", start_sw_download->number_of_update);
	dbprintf(LOG_ERR, "start_sw_download->target[0].image_slot=%d\n", start_sw_download->target[0].image_slot);
	dbprintf(LOG_ERR, "start_sw_download->target[0].image_instance=%d\n",start_sw_download->target[0].image_instance);

	/*DEBUG to syslog*/
	syslog(LOG_ERR, "image_target=%d\n", download_info.image_target);
	syslog(LOG_ERR, "instance_id=%d\n", download_info.instance_id);
	syslog(LOG_ERR, "window_size_minus1=%d\n", download_info.window_size_minus1);
	syslog(LOG_ERR, "image_size_in_bytes=%u\n", download_info.image_size_in_bytes);
	syslog(LOG_ERR, "update_state=%d\n", download_info.update_state);
	syslog(LOG_ERR, "start_sw_download->number_of_update=%d\n", start_sw_download->number_of_update);
	syslog(LOG_ERR, "start_sw_download->target[0].image_slot=%d\n", start_sw_download->target[0].image_slot);
	syslog(LOG_ERR, "start_sw_download->target[0].image_instance=%d\n",start_sw_download->target[0].image_instance);
#endif
	//send response
	// ps: CORETASK_QUEUE_MAX_LEN is current receive queue buffer size
	
	{ // the max window size should be not larger than any one of (omci_env_g.sw_download_window_size, CORETASK_QUEUE_MAX_LEN-2, 255)
		int max_window_size = omci_env_g.sw_download_window_size;
		if (max_window_size > CORETASK_QUEUE_MAX_LEN-2)
			max_window_size = CORETASK_QUEUE_MAX_LEN-2;
		if (max_window_size > 255)
			max_window_size = 255;
		if (start_sw_download->window_size_minus1 > max_window_size) {
			start_sw_download_response->window_size_minus1 = max_window_size;
			download_info.window_size_minus1 = start_sw_download_response->window_size_minus1;
		} else
			start_sw_download_response->window_size_minus1 = download_info.window_size_minus1;
	}
	download_info.section_map=malloc_safe(sizeof(unsigned char) * (download_info.window_size_minus1+1));

	start_sw_download_response->number_of_responding=0;
	start_sw_download_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

	if(download_info.section_map==NULL) {
		start_sw_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		if ( download_info.section_map ) {
			free_safe(download_info.section_map);
			download_info.section_map=NULL;
		}
		return OMCIMSG_ERROR_NOMEM;
	}

	if(omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU)
		proprietary_alu_start_sw_download(start_sw_download_response);
	
	if(instance_id->ls ==0 )
		ret=sw_image_state(START_DOWNLOAD_0);
	else if(instance_id->ls ==1 )
		ret=sw_image_state(START_DOWNLOAD_1);

	if (ret < 0 ) {
		dbprintf(LOG_ERR, "Error sw_image_state(start download)!\n");
		syslog(LOG_ERR, "Error sw_image_state(start download)!\n");
		close(download_info.image_fd);
		//clean parameter
		download_info.image_fd=-1;
		download_info.last_window_end=0;
		download_info.download_start_time=0;
		if ( download_info.section_map ) {
			free_safe(download_info.section_map);
			download_info.section_map=NULL;
		}
		start_sw_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	switch_hw_fvt_g.sw_image_download_start(download_info.instance_id, download_info.image_size_in_bytes);
#endif
	dbprintf(LOG_ERR, "MSG: Start software download!\n");

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	ret = 0;
#else
	snprintf(exec_cmd, PATH_LEN, "%s start_download", omci_env_g.sw_download_script);
	//dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", exec_cmd);
	ret=util_run_by_system(exec_cmd);
#endif

	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "/usr/sbin/sw_download.sh start_download error ret=%d!\n", WEXITSTATUS(ret));
		if ( download_info.section_map ) {
			free_safe(download_info.section_map);
			download_info.section_map=NULL;
		}
		return -1;
	}
	dbprintf(LOG_ERR, "exec_cmd=[%s]\n", exec_cmd);	//test
	download_info.download_start_time=time(0);
	return OMCIMSG_RW_OK;
}

int 
omcimsg_download_section_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_download_section_t *download_section = (struct omcimsg_download_section_t *) req_msg->msg_contents;
	unsigned char flag_need_ack=omcimsg_header_get_flag_ar(req_msg);
	long long current_window_offset, file_offset, data_size;
	int ret, i;

#if 0	//detail valid check
	if (omcimsg_header_get_entity_class_id(req_msg) != 7) {	//sw image
		dbprintf(LOG_ERR, "classid should be sw image\n");
		goto exit_download_section_handler;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0 || instance_id->ls == 255 ) {
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		goto exit_download_section_handler;
	}

	if (instance_id->ls != download_info.instance_id) {
		dbprintf(LOG_ERR, "Instance id should match with start sw download\n");
		goto exit_download_section_handler;
	}

	// check if action is valid
	if( (download_info.instance_id == 0 && download_info.sw_state != S2P) || (download_info.instance_id == 1 && download_info.sw_state != S2 ) ) {
		dbprintf(LOG_ERR, "Action is invalid\n");
		goto exit_download_section_handler;
	}
#endif

	//calculate file offset, data size
	current_window_offset = download_section->download_section_num * OMCIMSG_DL_SECTION_CONTENTS_LEN;
	file_offset = download_info.last_window_end + current_window_offset;
	data_size =  download_info.image_size_in_bytes - file_offset;
	if( data_size > OMCIMSG_DL_SECTION_CONTENTS_LEN) {
		data_size=OMCIMSG_DL_SECTION_CONTENTS_LEN;
	} else if (data_size >=0) {
		dbprintf(LOG_DEBUG, "last data_size=%lld\n", data_size);
	} else {
		dbprintf(LOG_ERR, "last data_size %lld <0?\n", data_size);
		goto exit_download_section_handler;
	}
		
	if( (download_info.image_fd=open(omci_env_g.sw_download_image_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0 ){
		dbprintf(LOG_ERR, "Error file open!\n");
		goto exit_download_section_handler;
	}

	if (file_offset == 0) {
		//init crc value
		global_crc_accum_be = 0xFFFFFFFF;
		global_crc_accum_le = 0xFFFFFFFF;
	}

	for( i=0; i < 3; i++) {
		if(omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg) == last_download_section_tid) {
			dbprintf(LOG_ERR, "download_section retransmission! tid=0x%02x prev_tid=0x%02x file_offset=%lld data_size=%lld\n", omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg), last_download_section_tid, file_offset,data_size);
			syslog(LOG_ERR, "download_section retransmission! tid=0x%02x prev_tid=0x%02x file_offset=%lld data_size=%lld\n", omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg), last_download_section_tid, file_offset,data_size);
			download_info.last_window_end = download_info.last_window_end - current_window_offset - data_size;
			file_offset = download_info.last_window_end + current_window_offset;
		}
		lseek(download_info.image_fd, file_offset, SEEK_SET);
		//write to file and check return value
		ret=write(download_info.image_fd, download_section->data, data_size);
		if ( ret == data_size ) {
			last_download_section_tid = omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg);
			break;	
		}
	}
	close(download_info.image_fd);
	download_info.image_fd=-1;

	dbprintf(LOG_WARNING, "ONT receive data size=%lld\n", data_size);
	//too noisy
	//syslog(LOG_ERR, "ONT receive data size=%lld\n", data_size);
	if ( ret != data_size ) {
		dbprintf(LOG_ERR, "Write to file error=%d\n", ret);
		goto exit_download_section_handler;
	}

	// mark this section as ok within the window		
	download_info.section_map[download_section->download_section_num]=1;

exit_download_section_handler:
	// gen response only if AR is set
	if(flag_need_ack) {
		struct omcimsg_download_section_response_t *download_section_response = (struct omcimsg_download_section_response_t *) resp_msg->msg_contents;
		download_section_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

		for(i=0; i <= download_section->download_section_num; i++ ) {
			if( download_info.section_map[i] != 1 ) {
				dbprintf(LOG_ERR, "window %d section %d error, offset=0x%X\n", download_info.last_window_end/((download_info.window_size_minus1+1)*OMCIMSG_DL_SECTION_CONTENTS_LEN), i, download_info.last_window_end+OMCIMSG_DL_SECTION_CONTENTS_LEN*i);
				syslog(LOG_ERR, "window %d section %d error, offset=0x%X\n", download_info.last_window_end/((download_info.window_size_minus1+1)*OMCIMSG_DL_SECTION_CONTENTS_LEN), i, download_info.last_window_end+OMCIMSG_DL_SECTION_CONTENTS_LEN*i);
				download_section_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
				break;
			}
		}
		// clean section map
		memset(download_info.section_map, 0, sizeof(unsigned char) * (download_info.window_size_minus1+1));

		omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
		download_section_response->download_section_num=download_section->download_section_num;

		if (download_section_response->result_reason == OMCIMSG_RESULT_CMD_SUCCESS) {
			char *section_buf = malloc_safe(current_window_offset + data_size + 1);
			if(section_buf) {
				download_info.image_fd = open(omci_env_g.sw_download_image_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				if(download_info.image_fd >= 0) {
					lseek(download_info.image_fd, download_info.last_window_end, SEEK_SET);
					read(download_info.image_fd, section_buf, current_window_offset + data_size);
					close(download_info.image_fd);
					download_info.image_fd = -1;
					switch(omci_env_g.omcc_rx_crc_type) {
						case ENV_CRC_TYPE_LE:
							global_crc_accum_le = crc_le_update(global_crc_accum_le, section_buf, current_window_offset + data_size);
							break;
						case ENV_CRC_TYPE_BE:
							global_crc_accum_be = crc_be_update(global_crc_accum_be, section_buf, current_window_offset + data_size);
							break;
						case ENV_CRC_TYPE_AUTO:
						default:
							global_crc_accum_be = crc_be_update(global_crc_accum_be, section_buf, current_window_offset + data_size);
							global_crc_accum_le = crc_le_update(global_crc_accum_le, section_buf, current_window_offset + data_size);
							break;
					}
				}
				free_safe(section_buf);
			}
			download_info.last_window_end = download_info.last_window_end + current_window_offset + data_size;
			return OMCIMSG_RW_OK;
		} else {
			return OMCIMSG_ERROR_PARM_ERROR;
		}

	} else {
		//dbprintf(LOG_DEBUG, "No error during section download, and no need response\n");
		resp_msg->transaction_id=0;
		resp_msg->device_id_type=0;
	}
	return OMCIMSG_RW_OK;
}

int 
omcimsg_end_sw_download_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_end_software_download_t *end_software_download;
	struct omcimsg_end_software_download_response_t *end_software_download_response;
	struct instance_id_t *instance_id;
	pthread_t system_thread;
	int ret;
	unsigned int duration;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	end_software_download_response = (struct omcimsg_end_software_download_response_t *) resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 7) {	//sw image
		end_software_download_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "classid should be sw image\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0 || instance_id->ls == 255 ) {
		end_software_download_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	if (instance_id->ls != download_info.instance_id) {
		end_software_download_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "Instance id shold match with start sw download\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}

#ifdef VALID_CHK
	// check if action is valid
	if( (download_info.instance_id == 0 && download_info.sw_state != S2P) || (download_info.instance_id == 1 && download_info.sw_state != S2) ) {
		end_software_download_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "Action is invalid\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}
#endif

	switch(download_info.update_state)
	{
	case OMCIMSG_SW_UPDATE_FLASH_WRITE_FAIL:
		syslog(LOG_ERR, "Error OMCIMSG_SW_UPDATE_FLASH_WRITE_FAIL\n");
		download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;	//change state to normal 
		end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	break;

	case OMCIMSG_SW_UPDATE_ACTIVE_IMAGE_FAIL:
		syslog(LOG_ERR, "Error OMCIMSG_SW_UPDATE_ACTIVE_IMAGE_FAIL\n");
		download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;	//change state to normal 
		end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	break;

	case OMCIMSG_SW_UPDATE_FLASH_WRITE:
		// updating flash, no crc check should be done during this state
		dbprintf(LOG_WARNING, "OLT send end software download again when flash updating.\n");
		end_software_download_response->result_reason = OMCIMSG_RESULT_DEVICE_BUSY;
		return OMCIMSG_ERROR_DEVICE_BUSY;
	break;

	case OMCIMSG_SW_UPDATE_DOWNLOAD:
		end_software_download = (struct omcimsg_end_software_download_t *) req_msg->msg_contents;
		duration=difftime(time(0),download_info.download_start_time);
		if (duration==0)
			duration=1;
		dbprintf(LOG_ERR, "Total Time:%u, Throughput: %u Byte/Sec, size=%u\n", duration, download_info.last_window_end/duration, download_info.image_size_in_bytes);
		syslog(LOG_ERR, "Total Time:%u, Throughput: %u Byte/Sec, size=%u\n", duration, download_info.last_window_end/duration, download_info.image_size_in_bytes);
		download_info.last_window_end=0;

		if ( download_info.section_map ) {
			free_safe(download_info.section_map);
			download_info.section_map=NULL;
		}

		//image CRC check
		switch(omci_env_g.omcc_rx_crc_type)
		{
		case ENV_CRC_TYPE_LE:
			if( ntohl(end_software_download->crc_32) != ~global_crc_accum_le) {
				dbprintf(LOG_ERR, "ERR: OLT crc32=%x, file crc32_le=%x\n", 
						ntohl(end_software_download->crc_32), ~global_crc_accum_le);
				syslog(LOG_ERR, "ERR: OLT crc32=%x, file crc32_le=%x\n", 
						ntohl(end_software_download->crc_32), ~global_crc_accum_le);
				end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
				if(instance_id->ls ==0 )
					sw_image_state(END_DOWNLOAD_INCORRECT_CRC_0);
				else if(instance_id->ls ==1 )
					sw_image_state(END_DOWNLOAD_INCORRECT_CRC_1);

				return OMCIMSG_ERROR_CMD_ERROR;
			}
			break;
		case ENV_CRC_TYPE_BE:
			if( ntohl(end_software_download->crc_32) != ~global_crc_accum_be) {
				dbprintf(LOG_ERR, "ERR: OLT crc32=%x, file crc32_be=%x\n", 
						ntohl(end_software_download->crc_32), ~global_crc_accum_be);
				syslog(LOG_ERR, "ERR: OLT crc32=%x, file crc32_be=%x\n", 
						ntohl(end_software_download->crc_32), ~global_crc_accum_be);

				end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
				if(instance_id->ls ==0 )
					sw_image_state(END_DOWNLOAD_INCORRECT_CRC_0);
				else if(instance_id->ls ==1 )
					sw_image_state(END_DOWNLOAD_INCORRECT_CRC_1);

				return OMCIMSG_ERROR_CMD_ERROR;
			}
			break;

		case ENV_CRC_TYPE_AUTO:
		default:
			// big endian
			if( ntohl(end_software_download->crc_32) == ~global_crc_accum_be) {
				dbprintf(LOG_ERR, "OK: OLT crc32=%x, file crc32_be=%x\n", 
					ntohl(end_software_download->crc_32), ~global_crc_accum_be);
				syslog(LOG_ERR, "OK: OLT crc32=%x, file crc32_be=%x\n", 
					ntohl(end_software_download->crc_32), ~global_crc_accum_be);
				break;
			}
			// little endian
			if(ntohl(end_software_download->crc_32) == ~global_crc_accum_le ) {
				dbprintf(LOG_ERR, "OK: OLT crc32=%x, file crc32_le=%x\n", 
					ntohl(end_software_download->crc_32), ~global_crc_accum_le);
				syslog(LOG_ERR, "OK: OLT crc32=%x, file crc32_le=%x\n", 
					ntohl(end_software_download->crc_32), ~global_crc_accum_le);
				break;
			}
			// crc eror
			dbprintf(LOG_ERR, "ERR: OLT crc32=%x, file crc32_be=%x, crc32_le=%x\n", 
				ntohl(end_software_download->crc_32), ~global_crc_accum_be, ~global_crc_accum_le);
			syslog(LOG_ERR, "ERR: OLT crc32=%x, file crc32_be=%x, crc32_le=%x\n", 
				ntohl(end_software_download->crc_32), ~global_crc_accum_be, ~global_crc_accum_le);
			end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			if(instance_id->ls ==0 )
				sw_image_state(END_DOWNLOAD_INCORRECT_CRC_0);
			else if(instance_id->ls ==1 )
				sw_image_state(END_DOWNLOAD_INCORRECT_CRC_1);
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		
		switch(instance_id->ls) {
			case 16: // voip config
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
				    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
					//update firmware to flash (/usr/sbin/sw_download.sh end_download_voip_config image_name)
					snprintf(system_end_sw_download.exec_cmd, PATH_LEN, "%s end_download_voip_config %s %d",
						omci_env_g.sw_download_script, omci_env_g.sw_download_image_name, instance_id->ls);
				}
				break;
			case 17: // rg config
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
				    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
					//update firmware to flash (/usr/sbin/sw_download.sh end_download_rg_config image_name)
					snprintf(system_end_sw_download.exec_cmd, PATH_LEN, "%s end_download_rg_config %s %d",
						omci_env_g.sw_download_script, omci_env_g.sw_download_image_name, instance_id->ls);
				}
				break;
			default:
				//update firmware to flash (/usr/sbin/sw_download.sh end_download image_name)
				snprintf(system_end_sw_download.exec_cmd, PATH_LEN, "%s end_download %s %d",
					omci_env_g.sw_download_script, omci_env_g.sw_download_image_name, instance_id->ls);
				break;
		}
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
		intel_end_sw_download.instance_id=instance_id->ls;
#else
		system_end_sw_download.instance_id=instance_id->ls;
#endif

		/*
		sw_download_end_behavior
		0: DEV_BUSY_AND_BG_UPDATE: return device busy and update flash in background
		1: DEV_OK_AND_BG_UPDATE: return ok immediately and update flash in background
		2: WAIT_AND_FG_UPDATE: wait update flash in foreground, return ok after finish
		*/
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
		char tmp_filepath[128];
		
		switch_hw_fvt_g.sw_image_download_end(download_info.instance_id, download_info.image_size_in_bytes, ntohl(end_software_download->crc_32), 128, tmp_filepath);
		dbprintf(LOG_ERR, "PRX126 Image Download End, File:%s\n", tmp_filepath);
		strncpy(intel_end_sw_download.file_path, tmp_filepath, strlen(tmp_filepath));
#else
		dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_end_sw_download.exec_cmd);
#endif
		//return device busy and update flash in background
		if ( omci_env_g.sw_download_end_behavior == DEV_BUSY_AND_BG_UPDATE) {
			download_info.update_state = OMCIMSG_SW_UPDATE_FLASH_WRITE;		//change state to updating
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
			if (pthread_create(&system_thread, NULL, intel_store_command, (void *) &intel_end_sw_download)) {
#else
			if (pthread_create(&system_thread, NULL, start_system_command, (void *) &system_end_sw_download)) {
#endif
				dbprintf(LOG_ERR, "Error creating pthread\n");
				syslog(LOG_ERR, "Error creating pthread\n");
				download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;	//change state to normal 
				end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
				return OMCIMSG_ERROR_CMD_ERROR;
			}
			end_software_download_response->result_reason = OMCIMSG_RESULT_DEVICE_BUSY;
			return OMCIMSG_ERROR_DEVICE_BUSY;
		
		//wait update flash in foreground, return ok after finish
		} else if ( omci_env_g.sw_download_end_behavior == WAIT_AND_FG_UPDATE) {
			download_info.update_state = OMCIMSG_SW_UPDATE_FLASH_WRITE;		//change state to updating
			system_thread=0;	//only avoid warning
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
			intel_store_command((void *) &intel_end_sw_download);
#else
			start_system_command((void *) &system_end_sw_download);
#endif
			download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;		//change state to normal 
		//return ok immediately and update flash in background
		} else if ( omci_env_g.sw_download_end_behavior == DEV_OK_AND_BG_UPDATE) {
			download_info.update_state = OMCIMSG_SW_UPDATE_FLASH_WRITE;		//change state to updating
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
			if (pthread_create(&system_thread, NULL, intel_store_command, (void *) &intel_end_sw_download)) {
#else
			if (pthread_create(&system_thread, NULL, start_system_command, (void *) &system_end_sw_download)) {
#endif
				dbprintf(LOG_ERR, "Error creating pthread\n");
				syslog(LOG_ERR, "Error creating pthread\n");
				download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;	//change state to normal 
				end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
				return OMCIMSG_ERROR_CMD_ERROR;
			}
		} else {
			dbprintf(LOG_ERR, "Error omcienv configure\n");
			syslog(LOG_ERR, "Error omcienv configure\n");
			download_info.update_state = OMCIMSG_SW_UPDATE_NORMAL;	//change state to normal 
			end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			return OMCIMSG_ERROR_CMD_ERROR;
		}
	break;
	}

	if(instance_id->ls ==0 )
		ret=sw_image_state(END_DOWNLOAD_0);
	else if(instance_id->ls ==1 )
		ret=sw_image_state(END_DOWNLOAD_1);

	if (ret < 0 ) {
		dbprintf(LOG_ERR, "Error sw_image_state(end download)!\n");
		syslog(LOG_ERR, "Error sw_image_state(end download)!\n");
		end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	dbprintf(LOG_ERR, "MSG: End software download!\n");
	end_software_download_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
	return OMCIMSG_RW_OK;
}

int 
omcimsg_activate_image_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_activate_image_response_t *activate_image_response;
	struct instance_id_t *instance_id;
#ifdef SW_NONBLOCK
	pthread_t system_thread;
#endif
	int ret, *retvalue=0;
#ifdef VALID_CHK
	struct me_t *me;
	struct attr_value_t attr_valid;
#endif

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	activate_image_response = (struct omcimsg_activate_image_response_t *) resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 7) {	//sw image
		activate_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "classid should be sw image\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0 ) {
		activate_image_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
	strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
		if( instance_id->ls != 0 && instance_id->ls != 1 && instance_id->ls != 16 && instance_id->ls != 17 ){
			activate_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
			dbprintf(LOG_ERR, "Instance id shold 0 or 1 or 16 or 17\n");
			return OMCIMSG_ERROR_PARM_ERROR;
		}
	} else {
		if( instance_id->ls != 0 && instance_id->ls != 1){
			activate_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
			dbprintf(LOG_ERR, "Instance id shold 0 or 1\n");
			return OMCIMSG_ERROR_PARM_ERROR;
		}
	}

	//check state if update finish, or always ok
	if (omci_env_g.sw_download_active_always_ok == 0 && download_info.update_state != OMCIMSG_SW_UPDATE_NORMAL) {
		activate_image_response->result_reason = OMCIMSG_RESULT_DEVICE_BUSY;
		return OMCIMSG_ERROR_DEVICE_BUSY;
	}

#ifdef VALID_CHK
	// check if action is valid
	me=meinfo_me_get_by_instance_num(7,download_info.instance_id);
	meinfo_me_attr_get(me, 4, &attr_valid);	//image's valid

	if( attr_valid.data == 0 ) {
		activate_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "Action is invalid: attr_valid.data == 0\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}
#endif
	switch(instance_id->ls) {
		case 16:
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
			    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
				//activate (/usr/sbin/sw_download.sh activate_voip_config instance)
				snprintf(system_activate_image.exec_cmd, PATH_LEN, "%s activate_voip_config %d",omci_env_g.sw_download_script, instance_id->ls);
				dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_activate_image.exec_cmd);
			}
			break;
		case 17:
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
			    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
				//activate (/usr/sbin/sw_download.sh activate_rg_config instance)
				snprintf(system_activate_image.exec_cmd, PATH_LEN, "%s activate_rg_config %d",omci_env_g.sw_download_script, instance_id->ls);
				dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_activate_image.exec_cmd);
			}
			break;
		default:
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
			//dbprintf(LOG_ERR, "ACTIVATE!\n");
#else
			//activate (/usr/sbin/sw_download.sh activate instance)
			snprintf(system_activate_image.exec_cmd, PATH_LEN, "%s activate %d",omci_env_g.sw_download_script, instance_id->ls);
			dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_activate_image.exec_cmd);
#endif
			break;
	}

	if(instance_id->ls == 0)
		ret=sw_image_state(ACTIVATE_0);
	else if(instance_id->ls ==1 )
		ret=sw_image_state(ACTIVATE_1);

	if (ret < 0 ) {
		dbprintf(LOG_ERR, "Error sw_image_state in ACTIVATE!\n");
		syslog(LOG_ERR, "Error sw_image_state in ACTIVATE!\n");
		activate_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	} else if ( ret == DO_NOTHING ) {
		activate_image_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
		return OMCIMSG_RW_OK;
	}

	if (omci_env_g.sw_download_active_always_ok == 1 && download_info.update_state != OMCIMSG_SW_UPDATE_NORMAL) {
		is_active_after_end_sw_dl=1;
	} else {
#ifdef SW_NONBLOCK
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
		intel_active_data.instance_id=instance_id->ls;

		if (pthread_create(&system_thread, NULL, intel_active_command, (void *) &intel_active_data)) {
			dbprintf(LOG_ERR, "Error creating pthread\n");
			syslog(LOG_ERR, "Error creating pthread\n");
			activate_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		dbprintf(LOG_ERR, "ACTIVATE!\n");
#else
		//dbprintf(LOG_ERR, "Command in ACTIVATE NONBLOCK!\n");
		if (pthread_create(&system_thread, NULL, start_system_command, (void *) &system_activate_image)) {
			dbprintf(LOG_ERR, "Error creating pthread\n");
			syslog(LOG_ERR, "Error creating pthread\n");
			activate_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			return OMCIMSG_ERROR_CMD_ERROR;
		}
#endif
#else
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
		//dbprintf(LOG_ERR, "Command in ACTIVATE!\n");
		//retvalue = (void *)switch_hw_fvt_g.sw_image_active_set(instance_id->ls, 10000);
		pthread_t system_thread;

		intel_active_data.instance_id=instance_id->ls;
		if (pthread_create(&system_thread, NULL, intel_active_command, (void *) &intel_active_data)) {
			dbprintf(LOG_ERR, "Error creating pthread\n");
			syslog(LOG_ERR, "Error creating pthread\n");
			activate_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		dbprintf(LOG_ERR, "ACTIVATE!\n");
#else
		retvalue=start_system_command((void *) &system_activate_image);
#endif
#endif
	}

	if( retvalue != 0 ) {
		dbprintf(LOG_ERR, "Error command in ACTIVATE!\n");
		activate_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}
	activate_image_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
	return OMCIMSG_RW_OK;
}

int 
omcimsg_commit_image_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_commit_image_response_t *commit_image_response;
	struct instance_id_t *instance_id;
#ifdef SW_NONBLOCK
	pthread_t system_thread;
#endif
	int ret=0;
#ifdef VALID_CHK
	struct me_t *me;
	struct attr_value_t attr_valid;
#endif

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	commit_image_response = (struct omcimsg_commit_image_response_t *) resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 7) {	//sw image
		commit_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "classid should be sw image\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0) {
		commit_image_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
	strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
		if( instance_id->ls != 0 && instance_id->ls != 1 && instance_id->ls != 16 && instance_id->ls != 17 ){
			commit_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
			dbprintf(LOG_ERR, "Instance id shold 0 or 1 or 16 or 17\n");
			return OMCIMSG_ERROR_PARM_ERROR;
		}
	} else {
		if( instance_id->ls != 0 && instance_id->ls != 1){
			commit_image_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
			dbprintf(LOG_ERR, "Instance id shold 0 or 1\n");
			return OMCIMSG_ERROR_PARM_ERROR;
		}
	}

	//check state if update finish, or always ok
	if ( !omci_env_g.sw_download_commit_always_ok && download_info.update_state != OMCIMSG_SW_UPDATE_NORMAL) {
		commit_image_response->result_reason = OMCIMSG_RESULT_DEVICE_BUSY;
		return OMCIMSG_ERROR_DEVICE_BUSY;
	}

#ifdef VALID_CHK
	// check if action is valid
	me=meinfo_me_get_by_instance_num(7, download_info.instance_id);
	meinfo_me_attr_get(me, 4, &attr_valid);	//image's valid

	if(attr_valid.data == 0) {
		commit_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "Action is invalid: image is still invalid == 0\n");
		syslog(LOG_ERR, "Action is invalid: image is still invalid == 0\n");
		return OMCIMSG_ERROR_CMD_ERROR;
	}

#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
	if ( !omci_env_g.sw_download_commit_always_ok ) {
		if( (instance_id->ls == 0 && download_info.active0_ok == 0 )
			|| (instance_id->ls == 1 && download_info.active1_ok == 0)) {
			commit_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			dbprintf(LOG_ERR, "commit fail because active_ok=0\n");
			syslog(LOG_ERR, "commit fail because active_ok=0\n");
			return OMCIMSG_ERROR_CMD_ERROR;
		}	
	}
#endif
#endif
	switch(instance_id->ls) {
		case 16:
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
			    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
				//commit (/usr/sbin/sw_download.sh commit_voip_config instance)
				snprintf(system_commit_image.exec_cmd, PATH_LEN, "%s commit_voip_config %d", omci_env_g.sw_download_script, instance_id->ls);
				dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_commit_image.exec_cmd);
			}
			break;
		case 17:
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 ||
			    strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0) {
				//commit (/usr/sbin/sw_download.sh commit_rg_config instance)
				snprintf(system_commit_image.exec_cmd, PATH_LEN, "%s commit_rg_config %d", omci_env_g.sw_download_script, instance_id->ls);
				dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_commit_image.exec_cmd);
			}
			break;
		default:
			//commit (/usr/sbin/sw_download.sh commit instance)
			snprintf(system_commit_image.exec_cmd, PATH_LEN, "%s commit %d", omci_env_g.sw_download_script, instance_id->ls);
			dbprintf(LOG_NOTICE, "exec_cmd=[%s]\n", system_commit_image.exec_cmd);
			if(instance_id->ls ==0 )
				ret=sw_image_state(COMMIT_0);
			else if(instance_id->ls ==1 )
				ret=sw_image_state(COMMIT_1);
			break;
	}

	if (ret < 0 ) {
		dbprintf(LOG_ERR, "Error sw_image_state(commit error)!\n");
		syslog(LOG_ERR, "Error sw_image_state(commit error)!\n");
		commit_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	} else if ( ret == DO_NOTHING ) {
		commit_image_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
		return OMCIMSG_RW_OK;
	}

#ifdef SW_NONBLOCK
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	dbprintf(LOG_ERR, "Commit Set!!\n");
	intel_commit_data.instance_id=instance_id->ls;
	if (pthread_create(&system_thread, NULL, intel_commit_command, (void *) &intel_commit_data)) {
		dbprintf(LOG_ERR, "Error creating pthread\n");
		syslog(LOG_ERR, "Error creating pthread\n");
		commit_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}
#else
	//dbprintf(LOG_ERR, "Commit Set NONBLOCK!!\n");
	if (pthread_create(&system_thread, NULL, start_system_command, (void *) &system_commit_image)) {
		dbprintf(LOG_ERR, "Error creating pthread\n");
		syslog(LOG_ERR, "Error creating pthread\n");
		commit_image_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}
#endif
#else
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	dbprintf(LOG_ERR, "Commit Set!!\n");
	switch_hw_fvt_g.sw_image_commit_set(instance_id->ls);
#else
	start_system_command((void *) &system_commit_image);
#endif
#endif
	
	commit_image_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
	return OMCIMSG_RW_OK;
}

