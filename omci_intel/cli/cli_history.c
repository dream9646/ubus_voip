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
 * File    : cli_history.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include "fwk_mutex.h"
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "coretask.h"
#include "omcimsg.h"
#include "omcidump.h"
#include "env.h"
#include "util.h"
#include "crc.h"
#include "conv.h"
#include "list.h"
#include "cli.h"


struct cli_history_list_t {
	unsigned int entry_count;
	unsigned int entry_seq;
	struct list_head entry_list;
	struct fwk_mutex_t mutex;
};

struct cli_history_entry_t {
	unsigned int seq;
	struct omcimsg_layout_t req_msg;
	struct omcimsg_layout_t resp_msg;
	struct list_head list_node;
};

struct cli_history_stat_t {
	unsigned int r_ok;
	unsigned int r_err;
	unsigned int w_ok;
	unsigned int w_err;
	unsigned int notification;
};

static struct cli_history_list_t cli_history_list_g;

int
cli_history_init(void)
{
	memset(&cli_history_list_g, 0x00, sizeof(struct cli_history_list_t));
	INIT_LIST_HEAD(&cli_history_list_g.entry_list);
	fwk_mutex_create(&cli_history_list_g.mutex);

	return 0;
}

int
cli_history_shutdown(void)
{
	struct cli_history_entry_t *history_entry_ptr, *history_entry_n;

	list_for_each_entry_safe(history_entry_ptr, history_entry_n, &cli_history_list_g.entry_list, list_node)
	{
		list_del(&history_entry_ptr->list_node);
		free_safe(history_entry_ptr);
		cli_history_list_g.entry_count--;
	}

	memset(&cli_history_list_g, 0x00, sizeof(struct cli_history_list_t));
	fwk_mutex_destroy(&cli_history_list_g.mutex);

	return 0;
}

int
cli_history_add_msg(struct omcimsg_layout_t *msg)
{
	unsigned char msgtype;
	struct cli_history_entry_t *history_entry_ptr, *history_entry_n;

	if (!omci_env_g.omci_history_enable || msg == NULL)
		return -1;

	msgtype = omcimsg_header_get_type(msg);
	if (!(omci_env_g.omci_history_msgmask & (1<<msgtype)))
		return -1;

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (omcimsg_header_get_flag_ak(msg) == 0)
	{
		//request, create a node and add to list
		if (cli_history_list_g.entry_count >= omci_env_g.omci_history_size)
		{
			//release old ones
			list_for_each_entry_safe(history_entry_ptr, history_entry_n, &cli_history_list_g.entry_list, list_node)
			{
				list_del(&history_entry_ptr->list_node);
				free_safe(history_entry_ptr);
				cli_history_list_g.entry_count--;
				if (cli_history_list_g.entry_count < omci_env_g.omci_history_size)
				{
					break;
				}
			}
		}

		if (cli_history_list_g.entry_count < omci_env_g.omci_history_size)
		{
			history_entry_ptr = malloc_safe(sizeof(struct cli_history_entry_t));
			memcpy(&history_entry_ptr->req_msg, msg, sizeof(struct omcimsg_layout_t));
		
			history_entry_ptr->seq = ++cli_history_list_g.entry_seq;
			//sequence number starts with 1
			if (history_entry_ptr->seq == 0)
			{
				history_entry_ptr->seq++;
				cli_history_list_g.entry_seq++;
			}

			cli_history_list_g.entry_count++;

			list_add_tail(&history_entry_ptr->list_node, &cli_history_list_g.entry_list);
		}
	} else {
		//response
		list_for_each_entry_reverse(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
		{
			if (omcimsg_header_get_flag_ar(&history_entry_ptr->req_msg) == 1 &&
				omcimsg_header_get_transaction_id(&history_entry_ptr->req_msg) == omcimsg_header_get_transaction_id(msg))
			{
				memcpy(&history_entry_ptr->resp_msg, msg, sizeof(struct omcimsg_layout_t));
				break;
			}
		}
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_entry_print(int fd, struct cli_history_entry_t *history_entry_ptr, struct cli_history_stat_t *stat, char *findstr, int do_print)
{
	unsigned char msgtype;
	unsigned short tid;
	unsigned short classid;
	unsigned short meid;
	unsigned char result_reason = 0;
	unsigned char result_symbol = ' ';
	unsigned char time_symbol = ' ';
	long ms_diff = 0;

	if (history_entry_ptr == NULL)
		return -1;

	msgtype=omcimsg_header_get_type(&history_entry_ptr->req_msg);
	tid=omcimsg_header_get_transaction_id(&history_entry_ptr->req_msg);
	classid=omcimsg_header_get_entity_class_id(&history_entry_ptr->req_msg);
	meid=omcimsg_header_get_entity_instance_id(&history_entry_ptr->req_msg);

	if (!omcimsg_util_msgtype_is_notification(msgtype))
	{
		//request from olt
		if (omcimsg_header_get_flag_ar(&history_entry_ptr->req_msg) == 1)
		{
			if (omcimsg_header_get_transaction_id(&history_entry_ptr->resp_msg) != 0 ||
				omcimsg_header_get_device_id(&history_entry_ptr->resp_msg) != 0)
			{ 
				switch (omcimsg_header_get_type(&history_entry_ptr->resp_msg)) {
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
					result_reason=history_entry_ptr->resp_msg.msg_contents[0];
					break;
				default:
					result_reason = 0;
				}
				ms_diff = util_timeval_diff_msec(&history_entry_ptr->resp_msg.timestamp, &history_entry_ptr->req_msg.timestamp);

			} else {
				result_reason = 10;
				//dbprintf(LOG_ERR, "no response: tid=0x%04x\n", tid);
				ms_diff = 0;
			}
		} else {
			result_reason = 0;
			ms_diff = 0;
		}

		if (stat != NULL)
		{
			if (omcimsg_util_msgtype_is_write(msgtype))
			{
				if (result_reason==0)
					stat->w_ok++;
				else
					stat->w_err++;
			} else {
				if (result_reason==0)
					stat->r_ok++;
				else
					stat->r_err++;
			}
		}
	} else {
		//notification
		result_reason = 0;
		if (stat != NULL)
			stat->notification++;
		ms_diff = 0;
	}

	if (do_print) {
		char tmpstr[1024], timestr[32];

		if (history_entry_ptr->req_msg.timestamp.tv_sec > 86400 * 365* 10) {
			struct tm tm_time;
			time_t timep; 
			memset(&tm_time, 0x00, sizeof(struct tm));
			memset(timestr, 0x00, sizeof(timestr));
			timep = history_entry_ptr->req_msg.timestamp.tv_sec;
			if (localtime_r(&timep, &tm_time) != NULL)
				strftime(timestr, 32, "%m-%d %T", &tm_time);
		} else {
			snprintf(timestr, 31, "%lu.%03lus", history_entry_ptr->req_msg.timestamp.tv_sec, history_entry_ptr->req_msg.timestamp.tv_usec/1000);
		}
		
		if (result_reason==10)
			result_symbol='?';
		else if (result_reason!=0)
			result_symbol='-';

		if (ms_diff >= omci_env_g.omci_exec_time_limitation)
			time_symbol = '*';

		sprintf(tmpstr, "%c%c%4u: 0x%04x, %s: %d %s, me 0x%x(%d) %s [%ld]\n",
			time_symbol,
			result_symbol,
			history_entry_ptr->seq, tid,
			omcimsg_util_msgtype2name(msgtype),
			classid, omcimsg_util_classid2name(classid),
			meid, meid, timestr, ms_diff);

		if (findstr == NULL || util_strcasestr(tmpstr, findstr) ||
		    msgtype == OMCIMSG_REBOOT || msgtype == OMCIMSG_MIB_RESET)
			util_fdprintf(fd, "%s",tmpstr);
	}
	return 0;
}

static int
cli_history_entry_print_detail(int fd, struct cli_history_entry_t *history_entry_ptr)
{
	util_fdprintf(fd, "MSG detail, seq=%u:\n", history_entry_ptr->seq);

	//show request
	if (!omcimsg_util_msgtype_is_notification(omcimsg_header_get_type(&history_entry_ptr->req_msg)))
	{
		omcidump_print_raw(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_RX);
		omcidump_print_msg(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_RX);
		
	} else {
		omcidump_print_msg(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_TX);
		omcidump_print_raw(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_TX);
	}

	//show response
	if (omcimsg_header_get_flag_ar(&history_entry_ptr->req_msg) == 1 &&
	    omcimsg_header_get_device_id(&history_entry_ptr->resp_msg) != 0x00)
	{
		omcidump_print_msg(fd, &history_entry_ptr->resp_msg, OMCIDUMP_DIR_TX);
		omcidump_print_raw(fd, &history_entry_ptr->resp_msg, OMCIDUMP_DIR_TX);
	}

	return 0;
}

static void
cli_history_print_mib_reset_line(int fd, int timestamp)
{
	long seconds=timestamp;
	struct tm *t=(struct tm *)localtime(&seconds);
	util_fdprintf(fd, "mib reset %04d/%02d/%02d %02d:%02d:%02d --------------------\n", 
		t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
}

static int
cli_history_find(int fd, char *findstr)
{
	struct cli_history_entry_t *history_entry_ptr;
	struct cli_history_stat_t stat;

	memset(&stat, 0x00, sizeof(stat));
	
	fwk_mutex_lock(&cli_history_list_g.mutex);

	util_fdprintf(fd	, "History list find:\n");
	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		cli_history_entry_print(fd, history_entry_ptr, &stat, findstr, 1);
	}
	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_clear(int fd)
{
	struct cli_history_entry_t *history_entry_ptr, *history_entry_n;

	fwk_mutex_lock(&cli_history_list_g.mutex);

	list_for_each_entry_safe(history_entry_ptr, history_entry_n, &cli_history_list_g.entry_list, list_node)
	{
		list_del(&history_entry_ptr->list_node);
		free_safe(history_entry_ptr);
		cli_history_list_g.entry_count--;
	}

	cli_history_list_g.entry_count = 0;
	cli_history_list_g.entry_seq = 0;
	INIT_LIST_HEAD(&cli_history_list_g.entry_list);

	util_fdprintf(fd, "History clear!!\n");

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_edit(int fd, unsigned int seq, unsigned int offset, char *hexstr[])
{
	struct cli_history_entry_t *history_entry_ptr;

	fwk_mutex_lock(&cli_history_list_g.mutex);

	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		if (history_entry_ptr->seq==seq) {
			unsigned int crc, crc_accum = 0xFFFFFFFF;
			int i=0;

			unsigned char *data=(unsigned char*)&history_entry_ptr->req_msg;		
			if (offset >= sizeof(history_entry_ptr->req_msg)) {
				fwk_mutex_unlock(&cli_history_list_g.mutex);
				util_fdprintf(fd, "offset %u too large?\n", offset);
				return -1;
			}
			for (i=0; hexstr[i]!=NULL; i++) {
				if (offset+i >= sizeof(history_entry_ptr->req_msg))
					break;
				data[offset+i]=util_atoi(hexstr[i]);
			}

			// fill tailer
			history_entry_ptr->req_msg.cpcsuu_cpi = 0;
			history_entry_ptr->req_msg.cpcssdu_len = htons(OMCIMSG_LEN-OMCIMSG_TRAILER_LEN);
			// recalc crc since 5vt omci crc be on udp
			crc = ~(crc_be_update(crc_accum, (char*)data, OMCIMSG_BEFORE_CRC_LEN));
			history_entry_ptr->req_msg.crc = htonl(crc);

			// show modified msg
			util_fdprintf(fd, "MSG detail, seq=%u:\n", history_entry_ptr->seq);
			if (!omcimsg_util_msgtype_is_notification(omcimsg_header_get_type(&history_entry_ptr->req_msg))) {
				omcidump_print_raw_with_mark(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_RX, offset, i);
				omcidump_print_msg(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_RX);
			} else {
				omcidump_print_msg(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_TX);
				omcidump_print_raw_with_mark(fd, &history_entry_ptr->req_msg, OMCIDUMP_DIR_TX, offset, i);
			}
		}
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_del(int fd, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr, *history_entry_n;
	int is_in_range = 0, del = 0;

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History del: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 != 0 && seq2 == 0)
	{
		seq2 = seq1;
	}

	if (seq1 == 0 && seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	list_for_each_entry_safe(history_entry_ptr, history_entry_n, &cli_history_list_g.entry_list, list_node)
	{
		unsigned int history_seq=history_entry_ptr->seq;	
	
		if (!is_in_range && history_seq == seq1)
			is_in_range = 1;
		if (is_in_range) {
			list_del(&history_entry_ptr->list_node);
			free_safe(history_entry_ptr);
			cli_history_list_g.entry_count--;
			del++;
		}
		if (history_seq == seq2) {
			is_in_range = 0;
			break;
		}
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	util_fdprintf(fd, "History del %u..%u: %d records deleted\n", seq1, seq2, del);

	return 0;
}

static int
cli_history_show(int fd, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr;
	int is_in_range = 0;

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History detail: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 != 0 && seq2 == 0)
	{
		seq2 = seq1;
	}

	if (seq1 == 0 && seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	util_fdprintf(fd, "History detail %u..%u:\n", seq1, seq2);

	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range)
			cli_history_entry_print_detail(fd, history_entry_ptr);
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_list(int fd, unsigned int seq1, unsigned int seq2, int skip_repeat)
{
	struct cli_history_entry_t *history_entry_ptr;
	struct cli_history_stat_t stat;
	int is_in_range = 0;

	unsigned char last_msgtype=0;
	unsigned int last_repeat=0;

	memset(&stat, 0x00, sizeof(stat));

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History list: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
	}
	if (seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	util_fdprintf(fd, "History list %u..%u:\n", seq1, seq2);
	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		unsigned char msgtype=omcimsg_header_get_type(&history_entry_ptr->req_msg);

		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range) {
			if (skip_repeat) {
				if (msgtype==last_msgtype && 
				    (msgtype==OMCIMSG_GET_ALL_ALARMS_NEXT ||
				     msgtype==OMCIMSG_MIB_UPLOAD_NEXT ||
				     msgtype==OMCIMSG_DOWNLOAD_SECTION ||
				     msgtype==OMCIMSG_GET_NEXT))  {
					cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 0);
					last_repeat++;
				} else {
					if (last_repeat) {
						util_fdprintf(fd, "%u repeated msgs\n", last_repeat);
						last_repeat=0;
					}
					if (msgtype==OMCIMSG_MIB_RESET) 
						cli_history_print_mib_reset_line(fd, history_entry_ptr->req_msg.timestamp.tv_sec);
					cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 1);
				}
				last_msgtype=msgtype;
			} else {
				if (msgtype==OMCIMSG_MIB_RESET) 
					cli_history_print_mib_reset_line(fd, history_entry_ptr->req_msg.timestamp.tv_sec);
				cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 1);
			}
		}
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}
	if (last_repeat) {
		util_fdprintf(fd, "%u repeated msgs\n", last_repeat);
		last_repeat=0;
	}					

	util_fdprintf(fd, "%uR+, %uW+, %uR-, %uW-, %uNotify\n",
		stat.r_ok,
		stat.w_ok,
		stat.r_err,
		stat.w_err,
		stat.notification);

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_listz(int fd, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr;
	struct cli_history_stat_t stat;
	int is_in_range = 0;

	memset(&stat, 0x00, sizeof(stat));

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History listz: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
	}
	if (seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	util_fdprintf(fd, "History list %u..%u:\n", seq1, seq2);
	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range) {
			unsigned char msgtype=omcimsg_header_get_type(&history_entry_ptr->req_msg);
			if (msgtype==OMCIMSG_MIB_RESET || msgtype==OMCIMSG_REBOOT) {
				if (stat.r_ok || stat.w_ok || stat.r_err || stat.w_err || stat.notification) {
					util_fdprintf(fd, "%uR+, %uW+, %uR-, %uW-, %uNotify\n",
						stat.r_ok, stat.w_ok, stat.r_err, stat.w_err, stat.notification);
				}
				cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 1);
				memset(&stat, 0x00, sizeof(stat));
			} else {
				cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 0);
			}
		}
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}
	if (stat.r_ok || stat.w_ok || stat.r_err || stat.w_err || stat.notification) {
		util_fdprintf(fd, "%uR+, %uW+, %uR-, %uW-, %uNotify\n",
			stat.r_ok, stat.w_ok, stat.r_err, stat.w_err, stat.notification);
	}
	
	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int
cli_history_listw(int fd, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr;
	struct cli_history_stat_t stat;
	int is_in_range = 0;

	memset(&stat, 0x00, sizeof(stat));

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History listz: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
	}
	if (seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	util_fdprintf(fd, "History list %u..%u:\n", seq1, seq2);
	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range) {
			unsigned char msgtype=omcimsg_header_get_type(&history_entry_ptr->req_msg);
			int is_write_request=0;
			switch (msgtype) {
				case OMCIMSG_CREATE:
				case OMCIMSG_DELETE:
				case OMCIMSG_SET:
				case OMCIMSG_MIB_RESET:
				case OMCIMSG_TEST:
				case OMCIMSG_START_SOFTWARE_DOWNLOAD:
				case OMCIMSG_DOWNLOAD_SECTION:
				case OMCIMSG_END_SOFTWARE_DOWNLOAD:
				case OMCIMSG_ACTIVATE_SOFTWARE:
				case OMCIMSG_COMMIT_SOFTWARE:
				case OMCIMSG_SYNCHRONIZE_TIME:	
				case OMCIMSG_REBOOT:
					is_write_request=1;
					break;
			}
			if (is_write_request) {
				if (stat.r_ok || stat.r_err || stat.notification) {
					util_fdprintf(fd, "%uR+, %uR-, %uNotify\n",
						stat.r_ok, stat.r_err, stat.notification);
				}
				if (msgtype==OMCIMSG_MIB_RESET) 
					cli_history_print_mib_reset_line(fd, history_entry_ptr->req_msg.timestamp.tv_sec);
				cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 1);
				memset(&stat, 0x00, sizeof(stat));
			} else {
				cli_history_entry_print(fd, history_entry_ptr, &stat, NULL, 0);
			}
		}
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}
	if (stat.r_ok || stat.w_ok || stat.r_err || stat.w_err || stat.notification) {
		util_fdprintf(fd, "%uR+, %uW+, %uR-, %uW-, %uNotify\n",
			stat.r_ok,stat.w_ok, stat.r_err, stat.w_err, stat.notification);
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

#define RINGBUFF_SIZE	101
static int
cli_history_last(int fd, int last_n, int skip_repeat)
{
	unsigned int ringbuff[101]={0};
	int total=0;
	int seq_reset=0;

	struct cli_history_entry_t *history_entry_ptr;
	unsigned char msgtype, last_msgtype=0;
	unsigned int last_repeat=0;

	fwk_mutex_lock(&cli_history_list_g.mutex);
	list_for_each_entry(history_entry_ptr, &cli_history_list_g.entry_list, list_node)
	{
		msgtype=omcimsg_header_get_type(&history_entry_ptr->req_msg);

		if (seq_reset==0 || msgtype==OMCIMSG_MIB_RESET || msgtype==OMCIMSG_REBOOT)
			seq_reset=history_entry_ptr->seq;
		
		if (skip_repeat) {
			if (msgtype==last_msgtype && 
			    (msgtype==OMCIMSG_GET_ALL_ALARMS_NEXT ||
			     msgtype==OMCIMSG_MIB_UPLOAD_NEXT ||
			     msgtype==OMCIMSG_DOWNLOAD_SECTION ||
			     msgtype==OMCIMSG_GET_NEXT))  {
				last_repeat++;
			} else {
				if (last_repeat)
					last_repeat=0;
				ringbuff[total%RINGBUFF_SIZE]=history_entry_ptr->seq;
				total++;
			}
			last_msgtype=msgtype;
		} else {
			ringbuff[total%RINGBUFF_SIZE]=history_entry_ptr->seq;
			total++;
		}
	}
	fwk_mutex_unlock(&cli_history_list_g.mutex);

	if (last_n==0) {
		return cli_history_list(fd, seq_reset, 0, skip_repeat);
	} else {
		if (last_n>total)
			last_n=total;
		if (last_n>RINGBUFF_SIZE-1)
			last_n=RINGBUFF_SIZE-1;	
		return cli_history_list(fd, ringbuff[(total-last_n)%RINGBUFF_SIZE], 0, skip_repeat);
	}
}

static void
cli_history_run_omcimsg(int fd, struct cli_history_entry_t *history_entry) 
{
	struct omcimsg_layout_t *msg=&history_entry->req_msg;
	struct omcimsg_layout_t *msg_run;
	struct coretask_msg_t *coretask_msg;
	unsigned char msgtype=omcimsg_header_get_type(msg);
	int retry, ret=0;

	if (omcimsg_util_msgtype_is_notification(msgtype))
		return;

	// skip msgtype not matched in history_log_msgmask
	{
		#define OMCIMSG_MIB_UPLOAD_MASK		((1<<OMCIMSG_MIB_UPLOAD)|(1<<OMCIMSG_MIB_UPLOAD_NEXT))
		unsigned int history_log_msgmask = omci_env_g.omci_history_msgmask;
		// turn off both mib_up/mib_up_next bits if any one of them is off
		if ((history_log_msgmask & OMCIMSG_MIB_UPLOAD_MASK) != OMCIMSG_MIB_UPLOAD_MASK) {
			history_log_msgmask &= (~OMCIMSG_MIB_UPLOAD_MASK);
		}
		if ((history_log_msgmask & (1<<msgtype)) == 0)
			return;
	}

	if (omci_env_g.omcidump_msgmask & (1<<msgtype)) {
		switch (omci_env_g.omcidump_enable) {
#if 0
		case 1:	omcidump_print_msg(fd, msg, OMCIDUMP_DIR_RX);
			break;
		case 2:	omcidump_print_raw(fd, msg, OMCIDUMP_DIR_RX);
			break;
		case 3:	omcidump_print_raw(fd, msg, OMCIDUMP_DIR_RX);
			omcidump_print_msg(fd, msg, OMCIDUMP_DIR_RX);
			break;
		case 4:	omcidump_print_line(fd, msg, OMCIDUMP_DIR_RX);
			break;
#endif
		case 8:	omcidump_print_char(fd, msg, OMCIDUMP_DIR_RX);
			break;
		}
	}

	/*alloc omci msg memory, omci data */
	coretask_msg = malloc_safe(sizeof (struct coretask_msg_t));
	INIT_LIST_NODE(&coretask_msg->q_entry);

	msg_run = malloc_safe(sizeof(struct omcimsg_layout_t));
	memcpy(msg_run, msg, OMCIMSG_LEN);
	coretask_msg->omci_data = (unsigned char *)msg_run;	

	coretask_msg->msglen = sizeof(struct omcimsg_layout_t);
	coretask_msg->cmd = CORETASK_CMD_PROCESS_MSG;
	coretask_msg->from = 1; //from log

	util_get_uptime(&msg_run->timestamp);

	for (retry=0; retry<1000; retry++) {
		ret=-1;
		if (omcimsg_util_get_msg_priority(msg)) {
			if (fwk_msgq_len(coretask_hi_qid_g)<CORETASK_QUEUE_MAX_LEN)
				ret=fwk_msgq_send(coretask_hi_qid_g, &coretask_msg->q_entry);
		} else {
			if (fwk_msgq_len(coretask_lo_qid_g)<CORETASK_QUEUE_MAX_LEN)
				ret=fwk_msgq_send(coretask_lo_qid_g, &coretask_msg->q_entry);
		}
		fwk_thread_yield();
		if (ret==0) {
			if (retry>0) {
				dbprintf((retry>10)?LOG_CRIT:LOG_WARNING, "seq=%u, tid=%x run successed after retry %d\n", 
					history_entry->seq, omcimsg_header_get_transaction_id(msg), retry);
			}
			return;
		}
		msleep(5);	// 5ms
	}

	// run fail becuase too many retry
	dbprintf(LOG_CRIT, "seq=%u, tid=%x, run failed because of too many retry(%d)\n", 
		history_entry->seq, omcimsg_header_get_transaction_id(msg), retry);
	free_safe(coretask_msg->omci_data);
	free_safe(coretask_msg);
}

static int
cli_history_run(int fd, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr, *nptr;
	struct cli_history_stat_t stat;
	int is_in_range = 0;

	memset(&stat, 0x00, sizeof(stat));

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History run: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	//fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
	}

	if (seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	util_fdprintf(fd, "History run %u..%u:\n", seq1, seq2);
	list_for_each_entry_safe(history_entry_ptr, nptr, &cli_history_list_g.entry_list, list_node)
	{
		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range)
			cli_history_run_omcimsg(fd, history_entry_ptr);
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}

	//fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

static int 
cli_history_load(int fd, char *filename)
{
	char *linebuff;
	unsigned char *mem;

	int msg_total=0;
	int mem_offset=0;
	int is_omcc=0;
	FILE *fp;
	struct timeval timestamp;
		
	if ((fp = fopen(filename, "r")) == NULL) {
		util_fdprintf(fd, "file %s load error (%s)\n", filename,  strerror(errno));
		return -1;
	}

	linebuff = malloc_safe(1024);
	mem = malloc_safe(128);

	while (fgets(linebuff, 1024, fp)) {
		if (is_omcc) {
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
			if (n==0 || mem_offset>=OMCIMSG_LEN-OMCIMSG_TRAILER_LEN) {
				struct omcimsg_layout_t *msg=(struct omcimsg_layout_t *)mem;
				unsigned char msgtype=omcimsg_header_get_type(msg);
				if (omci_env_g.omccrx_log_msgmask & (1<<msgtype)) {
					unsigned int crc, crc_accum = 0xFFFFFFFF;
					// fill tailer
					msg->cpcsuu_cpi = 0;
					msg->cpcssdu_len = htons(OMCIMSG_LEN-OMCIMSG_TRAILER_LEN);
					// recalc crc since 5vt omci crc be on udp
					crc = ~(crc_be_update(crc_accum, (char *)msg, OMCIMSG_BEFORE_CRC_LEN));
					msg->crc = htonl(crc);					

					msg_total++;
					msg->timestamp = timestamp;
					cli_history_add_msg(msg);
				}
				mem_offset=0;
				is_omcc=0;
			}
		} else {
			char *timestamp_str, *tmp_str;
			if (strstr(linebuff, "OMCCRX") || strstr(linebuff, "OMCCTX")) {
				//jump to OMCCRX[/OMCCTX[
				timestamp_str = linebuff + 7;
				if ((tmp_str = strstr(timestamp_str, ".")))
				{
					memset(&timestamp, 0x00, sizeof(struct timeval));
					tmp_str[0] = 0;
					tmp_str[7] = 0; //6-digits micro second
					timestamp.tv_sec = atoi(timestamp_str);
					timestamp.tv_usec = atoi(&tmp_str[1]);
				}
				mem_offset=0;
				is_omcc=1;
			}
		}
	}

	free_safe(linebuff);
	free_safe(mem);
	fclose(fp);

	util_fdprintf(fd, "%d records loaded\n", msg_total);

	return(0);
}

static int
cli_history_save(int fd, char *savefile, unsigned int seq1, unsigned int seq2)
{
	struct cli_history_entry_t *history_entry_ptr, *nptr;
	struct cli_history_stat_t stat;
	int is_in_range = 0;
	int is_saved=0;

	memset(&stat, 0x00, sizeof(stat));

	if (seq1 != 0 && seq2 != 0 && seq2 < seq1)
	{
		util_fdprintf(fd, "History run: sequence unacceptable, %u < %u\n", seq2, seq1);
		return -1;
	}

	fwk_mutex_lock(&cli_history_list_g.mutex);

	if (seq1 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.next, struct cli_history_entry_t, list_node);
		seq1 = history_entry_ptr->seq;
	}

	if (seq2 == 0 && !list_empty(&cli_history_list_g.entry_list))
	{
		history_entry_ptr = list_entry(cli_history_list_g.entry_list.prev, struct cli_history_entry_t, list_node);
		seq2 = history_entry_ptr->seq;
	}

	list_for_each_entry_safe(history_entry_ptr, nptr, &cli_history_list_g.entry_list, list_node)
	{
		if (!is_in_range && history_entry_ptr->seq == seq1)
			is_in_range = 1;
		if (is_in_range) {
			unsigned char msgtype=omcimsg_header_get_type(&(history_entry_ptr->req_msg));
			if (omci_env_g.omccrx_log_msgmask & (1<<msgtype)) {
				omcidump_print_raw_to_file(&history_entry_ptr->req_msg, savefile, OMCIDUMP_DIR_RX);
				is_saved++;
				if (omcimsg_header_get_transaction_id(&history_entry_ptr->resp_msg) != 0 ||
					omcimsg_header_get_device_id(&history_entry_ptr->resp_msg) != 0)
				{ 
					omcidump_print_raw_to_file(&history_entry_ptr->resp_msg, savefile, OMCIDUMP_DIR_TX);
					is_saved++;
				}
			}
		}
		if (history_entry_ptr->seq == seq2) {
			is_in_range = 0;
			break;
		}
	}
	if (is_saved) {
		util_fdprintf(fd, "History save %u records(%u..%u) to file %s\n", is_saved, seq1, seq2, savefile);
	} else {
		util_fdprintf(fd, "History entry %u not found?", seq1);
	}

	fwk_mutex_unlock(&cli_history_list_g.mutex);

	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_history_help(int fd)
{
	util_fdprintf(fd, 
		"history|h help|[subcmd...]\n");
}

void
cli_history_help_long(int fd)
{
	util_fdprintf(fd, 
		"history|h clear\n"
		"          last [n]\n"
		"          list|listw|listz|show|del|run [n1] [n2]\n"
		"          edit [n] [offset] [hex]...\n"
		"          find [string]\n"
		"          load [filename]\n"
		"          save [filename] [n1] [n2]\n");
}

int 
cli_history_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "history")==0 || strcmp(argv[0], "h")==0) {
		if (argc == 1) {
			return cli_history_list(fd, 0, 0, 1);
		} else {
			if (strcmp(argv[1], "help") == 0 ) {
				cli_history_help_long(fd);
				return 0;
			} else if (strcmp(argv[1], "clear") == 0) {
				return cli_history_clear(fd);
			} else if (strcmp(argv[1], "last") == 0) {
				if (argc == 3)
				{
					return cli_history_last(fd, util_atoi(argv[2]), 0);
				} else {
					return cli_history_last(fd, 20, 0);
				}
			} else if (strcmp(argv[1], "edit") == 0 && argc>=5) {
				return cli_history_edit(fd, util_atoi(argv[2]), util_atoi(argv[3]), &argv[4]);
			} else if (strcmp(argv[1], "del") == 0) {
				if (argc == 3) {
					return cli_history_del(fd, util_atoi(argv[2]), 0);
				} else if (argc == 4) {
					return cli_history_del(fd, util_atoi(argv[2]), util_atoi(argv[3]));
				} else {
					return cli_history_del(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "show") == 0) {
				if (argc == 3)
				{
					return cli_history_show(fd, util_atoi(argv[2]), 0);
				} else if (argc == 4) {
					return cli_history_show(fd, util_atoi(argv[2]), util_atoi(argv[3]));
				} else {
					return cli_history_show(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "list") == 0) {
				if (argc == 3)
				{
					return cli_history_list(fd, util_atoi(argv[2]), 0, 0);
				} else if (argc == 4) {
					return cli_history_list(fd, util_atoi(argv[2]), util_atoi(argv[3]), 0);
				} else {
					return cli_history_list(fd, 0, 0, 0);
				}
			} else if (strcmp(argv[1], "listw") == 0) {
				if (argc == 3)
				{
					return cli_history_listw(fd, util_atoi(argv[2]), 0);
				} else if (argc == 4) {
					return cli_history_listw(fd, util_atoi(argv[2]), util_atoi(argv[3]));
				} else {
					return cli_history_listw(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "listz") == 0) {
				if (argc == 3) {
					return cli_history_listz(fd, util_atoi(argv[2]), 0);
				} else if (argc == 4) {
					return cli_history_listz(fd, util_atoi(argv[2]), util_atoi(argv[3]));
				} else {
					return cli_history_listz(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "run") == 0) {
				if (argc == 3)
				{
					return cli_history_run(fd, util_atoi(argv[2]), 0);
				} else if (argc == 4) {
					return cli_history_run(fd, util_atoi(argv[2]), util_atoi(argv[3]));
				} else {
					return cli_history_run(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "load") == 0) {
				static char last_load_file[512]={0};
				if (argc == 3) {
					strncpy(last_load_file, argv[2], 512);
					return cli_history_load(fd, argv[2]);
				} else if (argc ==2) {
					util_fdprintf(fd, "current load = %s\n", last_load_file);
					return CLI_OK;
				}
			} else if (strcmp(argv[1], "find") == 0) {
				if (argc == 3)
				{
					return cli_history_find(fd, argv[2]);
				}
			} else if (strcmp(argv[1], "save") == 0) {
				if (argc == 5) {
					return cli_history_save(fd, argv[2], util_atoi(argv[3]), util_atoi(argv[4]));
				} else if (argc == 4) {
					return cli_history_save(fd, argv[2], util_atoi(argv[3]), 0);
				} else if (argc == 3) {
					return cli_history_save(fd, argv[2], 0, 0);
				}
			}
		}
		return CLI_ERROR_SYNTAX;		

	} else if (strcmp(argv[0], "last")==0 || strcmp(argv[0], "l")==0) {
		if (argc == 2) {
			return cli_history_last(fd, util_atoi(argv[1]), 1);
		} else {
			return cli_history_last(fd, 20, 1);
		}

	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}
