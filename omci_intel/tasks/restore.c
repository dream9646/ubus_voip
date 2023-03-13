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
 * Module  : tasks
 * File    : restore.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "coretask.h"
#include "omcimsg.h"
#include "omcidump.h"
#include "util.h"
#include "conv.h"
#include "crc.h"
#include "cli.h"

/*
31:x,		30:x,		29:x,		28:get_current_data 	0
27:test_result,	26:get_next,	25:reboot,	24:synchronize_time 	0
23:commit_sw,	22:activate_sw,	21:end_sw_dl,	20:download_section	0
19:start_sw_dl,	18:test,	17:avc,		16:alarm 		0
15:mib_reset,	14:mib_ul_next,	13:mib_ul,	12:get_all_alarm_next	8
11:get_all_alm,	10:x,		9:get,		8:set 			1
7:x,		6:delete,	5:x,		4:create 		5
3:x,		2:x,		1:x,		0:x 			0
*/
#define RESTORE_APPENDMASK	0x00008150	// mib_reset, set, delete, create
#define RESTORE_LOADMASK	0x00000150	// set, delete, create

void
restore_append_omcclog(struct omcimsg_layout_t *msg, char *filename)
{
	unsigned char msgtype=omcimsg_header_get_type(msg);
	if (msgtype==OMCIMSG_MIB_RESET) {	// trunc restore.log to zero
		int fd=open(filename, O_RDWR|O_TRUNC);						
		close(fd);
	}
	if ((1<<msgtype) & RESTORE_APPENDMASK) {	// put set, delete, create to restore.log
		omcidump_print_raw_to_file(msg, filename, OMCIDUMP_DIR_RX);
	}
}

int 
restore_load_omcclog(char *filename)
{
	char linebuff[1024];
	unsigned char mem[128];			// for request msg
	struct omcimsg_layout_t resp_msg;	// for resp msg
	 
	int msg_total=0;
	int linenum=0;
	int mem_offset=0;
	int is_omccrx=0;
	FILE *fp;
		
	if ((fp = fopen(filename, "r")) == NULL) {
		if (errno!=2)	// error other than "file not exist"
			dbprintf(LOG_ERR, "file %s load error %d(%s)\n", filename, errno, strerror(errno));
		return -1;
	}

	while (fgets(linebuff, 1024, fp)) {
		linenum++;

		if (is_omccrx) {
			char *hexstr=linebuff;
			int n=0;

			if (linebuff[0]=='[') {
				hexstr=strstr(linebuff, "]");
				if (hexstr)
					hexstr++;
				else
					continue;
			}			
			if (strncmp(hexstr, "tid=", 4)==0)
				continue;
			
			n=conv_hexstr_to_mem(mem+mem_offset, 128, hexstr);			
			if (n>0)
				mem_offset+=n;
			if (n==0 || mem_offset>=OMCIMSG_LEN-OMCIMSG_TRAILER_LEN || is_omccrx>4) {
				struct omcimsg_layout_t *req_msg=(struct omcimsg_layout_t *)mem;

				// restore msg type: mib_reset, set, delete, create
				if ((1<<omcimsg_header_get_type(req_msg)) & RESTORE_LOADMASK) {
					unsigned int crc, crc_accum = 0xFFFFFFFF;

					// fill tailer
					req_msg->cpcsuu_cpi = 0;
					req_msg->cpcssdu_len = htons(OMCIMSG_LEN-OMCIMSG_TRAILER_LEN);

					// recalc crc since 5vt omci crc be on udp
					crc = ~(crc_be_update(crc_accum, (char *)req_msg, OMCIMSG_BEFORE_CRC_LEN));
					req_msg->crc = htonl(crc);					

					// zero buffer for response msg
					memset(&resp_msg, 0, sizeof( struct omcimsg_layout_t));
					
					util_get_uptime(&req_msg->timestamp);

					// issue the msg
					coretask_msg_dispatcher(req_msg, &resp_msg);

					util_get_uptime(&resp_msg.timestamp);

					msg_total++;
					cli_history_add_msg(req_msg);
					cli_history_add_msg(&resp_msg);
				}
				mem_offset=0;
				is_omccrx=0;
			}
		} else {
			if (strstr(linebuff, "OMCCRX")) {
				mem_offset=0;
				is_omccrx=1;
			}
		} 
	}
	fclose(fp);

	dbprintf(LOG_CRIT, "%d records restored\n", msg_total);

	return(0);
}

