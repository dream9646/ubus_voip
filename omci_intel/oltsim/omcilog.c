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
 * Module  : oltsim
 * File    : omcilog.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>


#include <signal.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "util.h"
#include "omcistr.h"
#include "omcilog.h"

struct omcilog_t *omcilog_ptr[OMCILOG_TOTAL_MAX];	// 0..65535
int omcilog_total=0;

///////////////////////////////////////////////////////////////////////

#define POLYNOMIAL1 0x04c11db7L
static unsigned long crc_be_table[256];

/* generate the table of CRC remainders for all possible bytes */
static void
crc_be_init_table(void)
{
	register int i, j;
	register unsigned long crc_accum;
	for (i = 0; i < 256; i++) {
		crc_accum = ((unsigned long) i << 24);
		for (j = 0; j < 8; j++) {
			if (crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL1;
			else
				crc_accum = (crc_accum << 1);
		}
		crc_be_table[i] = crc_accum;
	}
	return;
}

/* update the CRC on the data block one byte at a time. */
static unsigned int
crc_be_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
	static int is_table_inited=0;
	register int i, j;

	if (!is_table_inited) {
		crc_be_init_table();
		is_table_inited=1;
	}

	for (j = 0; j < data_blk_size; j++) {
		i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_be_table[i];
	}
	return crc_accum;
}

///////////////////////////////////////////////////////////////////////

int 
omcilog_load(char *filename)
{
	FILE *fp;
	static char linebuff[1024];
	int omcilog_total_old=omcilog_total;
	int linenum=0;
	
	static unsigned char mem[128];
	int mem_offset=0;
	int is_omccrx=0;
		
	if ((fp = fopen(filename, "r")) == NULL)
		return -1;

	if (omcilog_total)
		printf("%d old pkt found\n", omcilog_total);

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
			
			n=util_hexstr2mem(mem+mem_offset, 128, hexstr);
			
			if (n>0)
				mem_offset+=n;

			if (n==0 || mem_offset>=OMCIMSG_LEN-OMCIMSG_TRAILER_LEN || is_omccrx>4) {
				struct omcilog_t *omcilog=malloc_safe(sizeof(struct omcilog_t));
				unsigned int crc, crc_accum = 0xFFFFFFFF;

				memcpy(&omcilog->msg, mem, OMCIMSG_LEN-OMCIMSG_TRAILER_LEN);

				// fill tailer
				omcilog->msg.cpcsuu_cpi = 0;
				omcilog->msg.cpcssdu_len = htons(OMCIMSG_LEN-OMCIMSG_TRAILER_LEN);
				// recalc crc since 5vt omci crc be on udp
				crc = ~(crc_be_update(crc_accum, (char *) &(omcilog->msg), OMCIMSG_BEFORE_CRC_LEN));
				omcilog->msg.crc = htonl(crc);

				omcilog->linenum=linenum;
				omcilog_ptr[omcilog_total]=omcilog;
				
				omcilog_total++;
				is_omccrx=0;
			}
		} else {
			if (strstr(linebuff, "OMCCRX")) {
				mem_offset=0;
				is_omccrx=1;
			}
		} 
		// printf("linenum=%d, omcilog_total=%d, mem_offset=%d\n", linenum, omcilog_total, mem_offset);
	}
	
	fclose(fp);

	if (omcilog_total_old==0)
		printf("%d line read, %d pkt loaded\n", linenum, omcilog_total);
	else 
		printf("%d line read, %d pkt loaded, %d pkt in total\n", 
			linenum, omcilog_total-omcilog_total_old, omcilog_total);
	return omcilog_total;
}

int 
omcilog_clearall(void)
{
	while (omcilog_total>0) {
		omcilog_total--;
		if (omcilog_ptr[omcilog_total])
			free(omcilog_ptr[omcilog_total]);
		omcilog_ptr[omcilog_total]=NULL;
	}
	return 0;
}

int 
omcilog_save_result_reason(struct omcimsg_layout_t *msg, int current)
{
	unsigned char result_reason=0;
	int i;

	switch (omcimsg_header_get_type(msg)) {
	case OMCIMSG_CREATE:
	case OMCIMSG_DELETE:
	case OMCIMSG_SET:
	case OMCIMSG_GET:
	case OMCIMSG_MIB_RESET:
	case OMCIMSG_TEST:
	case OMCIMSG_START_SOFTWARE_DOWNLOAD:
	case OMCIMSG_DOWNLOAD_SECTION:
	case OMCIMSG_END_SOFTWARE_DOWNLOAD:
	case OMCIMSG_ACTIVATE_SOFTWARE:
	case OMCIMSG_COMMIT_SOFTWARE:
	case OMCIMSG_SYNCHRONIZE_TIME:	
	case OMCIMSG_REBOOT:
	case OMCIMSG_GET_NEXT:
	case OMCIMSG_GET_CURRENT_DATA:
		result_reason=msg->msg_contents[0];
	}

	for (i=current; i>=0 && i>=current-10; i--) {
		if (i>=OMCILOG_TOTAL_MAX)
			continue;
		if (omcilog_ptr[i]) {
			if (omcimsg_header_get_transaction_id(&omcilog_ptr[i]->msg)==
				omcimsg_header_get_transaction_id(msg)) {
				omcilog_ptr[i]->result_reason=result_reason;
				break;
			}
		}
	}
	return result_reason;
}

int
omcilog_list(unsigned short start, unsigned short end)
{
	char tmpfile[128];
	FILE *fp;
	int lines=0, i;

	snprintf(tmpfile, 128, "/var/tmp/oltsim.%d.%06ld", getpid(), random()%1000000);
	fp=fopen(tmpfile, "a");		
	for (i=start; i<=end; i++) {
		if (i>=OMCILOG_TOTAL_MAX)
			break;
		if (omcilog_ptr[i]) {
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			unsigned short tid=omcimsg_header_get_transaction_id(msg);
			unsigned short classid=omcimsg_header_get_entity_class_id(msg);
			unsigned short meid=omcimsg_header_get_entity_instance_id(msg);		
			fprintf(fp, "%c%4u: 0x%04x, %s: %d %s, me 0x%x(%d)\n",
				omcilog_ptr[i]->result_reason==0?' ':'-',
				i, tid,
				omcistr_msgtype2name(msgtype),
				classid, omcistr_classid2name(classid),
				meid, meid);
			lines++;
		}
	}	
	fclose(fp);
	
	util_viewfile(tmpfile, lines);
	unlink(tmpfile);
	return 0;
}

int
omcilog_listw(unsigned short start, unsigned short end)
{
	char tmpfile[128];
	FILE *fp;
	int lines=0, i;
	int read_ok=0, read_err=0;

	snprintf(tmpfile, 128, "/var/tmp/oltsim.%d.%06ld", getpid(), random()%1000000);
	fp=fopen(tmpfile, "a");		
	
	for (i=start; i<=end; i++) {
		if (i>=OMCILOG_TOTAL_MAX)
			break;
		if (omcilog_ptr[i]) {
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			if (i==start || omcistr_msgtype_is_write(msgtype)) {
				unsigned short tid=omcimsg_header_get_transaction_id(msg);
				unsigned short classid=omcimsg_header_get_entity_class_id(msg);
				unsigned short meid=omcimsg_header_get_entity_instance_id(msg);		

				if (i!=start) {
					if (read_ok)
						fprintf(fp, ", %dR+", read_ok);
					if (read_err)
						fprintf(fp, ", %dR-", read_err);
					fprintf(fp, "\n");
				}
				fprintf(fp, "%c%4u: 0x%04x, %s: %d %s, me 0x%x(%d)",
					omcilog_ptr[i]->result_reason==0?' ':'-',
					i, tid,
					omcistr_msgtype2name(msgtype),
					classid, omcistr_classid2name(classid),
					meid, meid);
				lines++;
				
				read_ok=read_err=0;
			} else {
				if (omcilog_ptr[i]->result_reason==0)
					read_ok++;
				else
					read_err++;
			}
				
		}
	}	

	if (read_ok)
		fprintf(fp, ", %dR+", read_ok);
	if (read_err)
		fprintf(fp, ", %dR-", read_err);
	fprintf(fp, "\n");
	
	fclose(fp);
	
	util_viewfile(tmpfile, lines);
	unlink(tmpfile);
	return 0;
}

int
omcilog_listerr(unsigned short start, unsigned short end)
{
	char tmpfile[128];
	FILE *fp;
	int lines=0, i;
	int read_ok=0, write_ok=0;

	snprintf(tmpfile, 128, "/var/tmp/oltsim.%d.%06ld", getpid(), random()%1000000);
	fp=fopen(tmpfile, "a");		
		
	for (i=start; i<=end; i++) {
		if (i>=OMCILOG_TOTAL_MAX)
			break;
		if (omcilog_ptr[i]) {
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			if (i==start || msgtype==OMCIMSG_MIB_RESET || omcilog_ptr[i]->result_reason!=0) {		// list mib reset or error
				unsigned short tid=omcimsg_header_get_transaction_id(msg);
				unsigned short classid=omcimsg_header_get_entity_class_id(msg);
				unsigned short meid=omcimsg_header_get_entity_instance_id(msg);		

				if (i!=start) {
					if (read_ok)
						fprintf(fp, ", %dR+", read_ok);
					if (write_ok)
						fprintf(fp, ", %dW+", write_ok);					
					fprintf(fp, "\n");
				}
				fprintf(fp, "%c%4u: 0x%04x, %s: %d %s, me 0x%x(%d)",
					omcilog_ptr[i]->result_reason==0?' ':'-',
					i, tid,
					omcistr_msgtype2name(msgtype),
					classid, omcistr_classid2name(classid),
					meid, meid);
				lines++;

				read_ok=write_ok=0;
			} else {
				if (omcistr_msgtype_is_write(msgtype))
					write_ok++;
				else 
					read_ok++;
			}
			
		}
	}	

	if (read_ok)
		fprintf(fp, ", %dR+", read_ok);
	if (write_ok)
		fprintf(fp, ", %dW+", write_ok);					
	fprintf(fp, "\n");

	fclose(fp);
	
	util_viewfile(tmpfile, lines);
	unlink(tmpfile);
	return 0;
}

int
omcilog_listz(unsigned short start, unsigned short end)
{
	char tmpfile[128];
	FILE *fp;
	int lines=0, i;
	int read_ok=0, read_err=0, write_ok=0, write_err=0;

	snprintf(tmpfile, 128, "/var/tmp/oltsim.%d.%06ld", getpid(), random()%1000000);
	fp=fopen(tmpfile, "a");		
		
	for (i=start; i<=end; i++) {
		if (i>=OMCILOG_TOTAL_MAX)
			break;
		if (omcilog_ptr[i]) {
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			if (i==start || msgtype==OMCIMSG_MIB_RESET) {		// list mib reset
				unsigned short tid=omcimsg_header_get_transaction_id(msg);
				unsigned short classid=omcimsg_header_get_entity_class_id(msg);
				unsigned short meid=omcimsg_header_get_entity_instance_id(msg);		

				if (i!=start) {
					if (read_ok)
						fprintf(fp, ", %dR+", read_ok);
					if (write_ok)
						fprintf(fp, ", %dW+", write_ok);					
					if (read_err)
						fprintf(fp, ", %dR-", read_err);
					if (write_err)
						fprintf(fp, ", %dW-", write_err);					
					fprintf(fp, "\n");
				}
				fprintf(fp, "%c%4u: 0x%04x, %s: %d %s, me 0x%x(%d)",
					omcilog_ptr[i]->result_reason==0?' ':'-',
					i, tid,
					omcistr_msgtype2name(msgtype),
					classid, omcistr_classid2name(classid),
					meid, meid);
				lines++;

				read_ok=read_err=write_ok=write_err=0;
			} else {
				if (omcistr_msgtype_is_write(msgtype)) {
					if (omcilog_ptr[i]->result_reason==0)
						write_ok++;
					else
						write_err++;
				} else {
					if (omcilog_ptr[i]->result_reason==0)
						read_ok++;
					else
						read_err++;
				}
			}
			
		}
	}	

	if (read_ok)
		fprintf(fp, ", %dR+", read_ok);
	if (write_ok)
		fprintf(fp, ", %dW+", write_ok);					
	if (read_err)
		fprintf(fp, ", %dR-", read_err);
	if (write_err)
		fprintf(fp, ", %dW-", write_err);					
	fprintf(fp, "\n");

	fclose(fp);
	
	util_viewfile(tmpfile, lines);
	unlink(tmpfile);
	return 0;
}

