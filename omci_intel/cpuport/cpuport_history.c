/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : cpuport
 * File    : cpuport_history.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <net/if_arp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <fcntl.h>
#include <errno.h>

#include "list.h"
#include "fwk_thread.h"
#include "fwk_mutex.h"
#include "fwk_msgq.h"
#include "list.h"
#include "util.h"
#include "conv.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_recv.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"

static struct list_head cpuport_history_list_g;
static struct fwk_mutex_t cpuport_history_list_mutex;
static unsigned int cpuport_history_total = 0;
static unsigned int cpuport_history_index = 0;

// static routines //////////////////////////////////////////////////////////////////////////////
static void
cpuport_history_pktinfo2line(int fd, int index, struct cpuport_info_t *pktinfo)
{
	util_fdprintf(fd, "%d: %s, rx_port=%d(%s), recv=%d.%06ds, dispatch=+%uus, process=+%uus, forward=+%uus\n",
		index, (pktinfo->thread_name)?pktinfo->thread_name:"?",
		pktinfo->rx_desc.logical_port_id, switch_get_logical_portname(pktinfo->rx_desc.logical_port_id),
		pktinfo->recv_time.tv_sec, pktinfo->recv_time.tv_usec,
		(unsigned long)util_timeval_diff_usec(&pktinfo->dispatch_time, &pktinfo->recv_time),
		(unsigned long)util_timeval_diff_usec(&pktinfo->process_time, &pktinfo->recv_time),
		(unsigned long)util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time));
}

static int
cpuport_history_pktinfo2file(FILE *fp, int index, struct cpuport_info_t *pktinfo)
{
	unsigned char *m;
	int i;

	if (pktinfo == NULL)
		return -1;

	fprintf(fp, "INDEX=%d\n", index);
	// rx_dec
	fprintf(fp, "p2p_cfg=%d\n", pktinfo->rx_desc.p2p_cfg);
	fprintf(fp, "logical_port_id=%d\n", pktinfo->rx_desc.logical_port_id);
	fprintf(fp, "router_mode=%d\n", pktinfo->rx_desc.router_mode);
	// thread name & time
	fprintf(fp, "thread=%s\n", pktinfo->thread_name);
	fprintf(fp, "recv_time=%lu.%06lu\n", (unsigned long)pktinfo->recv_time.tv_sec, (unsigned long)pktinfo->recv_time.tv_usec);
	fprintf(fp, "dispatch_time=%lu.%06lu\n", (unsigned long)pktinfo->dispatch_time.tv_sec, (unsigned long)pktinfo->dispatch_time.tv_usec);
	fprintf(fp, "process_time=%lu.%06lu\n", (unsigned long)pktinfo->process_time.tv_sec, (unsigned long)pktinfo->process_time.tv_usec);
	fprintf(fp, "forward_time=%lu.%06lu\n", (unsigned long)pktinfo->forward_time.tv_sec, (unsigned long)pktinfo->forward_time.tv_usec);
	// packet
	fprintf(fp, "frame_len=%d\n", pktinfo->frame_len);
	m = pktinfo->frame_ptr;
	for (i = 0; i < pktinfo->frame_len; i++) {
		fprintf(fp, "%02X ", m[i]);
		if ((i + 1) % 16 == 0)
			fprintf(fp, "\n");
	}
	if (i % 16 != 0)
		fprintf(fp, "\n");
	fprintf(fp, "\n");	// blank line after pkt dump

	return 0;
}

static struct cpuport_info_t *
cpuport_history_clone_pktinfo(struct cpuport_info_t *pktinfo)
{
	struct cpuport_info_t *pktinfo2;
	unsigned char *buf_ptr;

	if (pktinfo == NULL)
		return NULL;

	pktinfo2 = (struct cpuport_info_t *)malloc_safe(sizeof(struct cpuport_info_t));
	if (pktinfo2 == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "fail to malloc pktinfo for cpuport history\n");
		return NULL;
	}
	buf_ptr = (unsigned char *)malloc_safe(2*CPUPORT_VTAG_LEN + pktinfo->frame_len + 8);	// 8 is for safety
	if (buf_ptr == NULL) {
		dbprintf_cpuport(LOG_NOTICE,"fail to malloc pktinfo->buf_ptr for cpuport history\n");
		free_safe(pktinfo2);
		return NULL;
	}

	pktinfo->history = pktinfo2;	// orig pktinfo has a pointer to the cloned pkt for tracking
	memcpy(pktinfo2, pktinfo, sizeof(struct cpuport_info_t));	// copy pktinfo
	pktinfo2->buf_ptr = buf_ptr;
	pktinfo2->buf_len = pktinfo->frame_len; // the buf len doesn't count the len before the frame start
	pktinfo2->frame_ptr = pktinfo2->buf_ptr + 2*CPUPORT_VTAG_LEN;
	memcpy(pktinfo2->frame_ptr, pktinfo->frame_ptr, pktinfo->frame_len);	// copy pkt frame
	// recalc nh pointer 
	pktinfo2->nh = pktinfo2->frame_ptr + ((void*)pktinfo->nh - (void*)pktinfo->frame_ptr);

	return pktinfo2;
}

static int
cpuport_history_clear_to_size(int size)
{
	struct cpuport_info_t *pktinfo, *n;

	if (cpuport_history_total <= size)
		return 0;
	list_for_each_entry_safe(pktinfo, n, &cpuport_history_list_g, node) {
		list_del(&pktinfo->node);
		free_safe(pktinfo->buf_ptr);
		free_safe(pktinfo);
		cpuport_history_total--;
		cpuport_history_index++;
		if (cpuport_history_total <= size)
			break;
	}

	return 0;
}

static int
cpuport_history_read_key_value(char *key, int key_len, char *value, int value_len, char *line)
{
	char *equal = strchr(line, '=');
	char *k, *v;

	if (equal == NULL)
		return -1;	// = not found, not a key=value line
	*equal = 0;
	k = util_trim(line);
	v = util_trim(equal+1);
	strncpy(key, k, key_len);
	strncpy(value, v, value_len);
	return 0;
}

static int
cpuport_history_str2timeval(struct timeval *tv, char *sec_dot_usec_str)
{
	char *dotptr = strchr(sec_dot_usec_str, '.');
	char *p;

	tv->tv_sec = util_atoi(sec_dot_usec_str);
	if (dotptr) {
		*dotptr = 0;
		for (p = dotptr+1; *p; p++) {
			if (*p != '0')
				break;
		}
		tv->tv_usec = (dotptr)?util_atoi(p):0;
	} else {
		tv->tv_usec = 0;
	}
	return 0;
}

static int
cpuport_history_locate_taskindex_by_name(char *name)
{
	int i;
	for (i=0; i<ENV_SIZE_TASK; i++) {
		if (omci_env_g.taskname[i]!=NULL && strcasecmp(name, omci_env_g.taskname[i]) == 0)
			return i;
	}
	return -1;
}

// public routines //////////////////////////////////////////////////////////////////////////////

void
cpuport_history_init(void)
{
	INIT_LIST_HEAD(&cpuport_history_list_g);
	fwk_mutex_create(&cpuport_history_list_mutex);
	cpuport_history_total = cpuport_history_index = 0;
}

void
cpuport_history_shutdown(void)
{
	cpuport_history_clear(0);
	fwk_mutex_destroy(&cpuport_history_list_mutex);
}

int
cpuport_history_clear(int size)
{
	fwk_mutex_lock(&cpuport_history_list_mutex);

	cpuport_history_clear_to_size(size);
	if (size == 0) {
		// in case unsync in global var(shot mnot happen), this would correct it back
		cpuport_history_index = cpuport_history_total = 0;
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	return 0;
}

int
cpuport_history_add(struct cpuport_info_t *pktinfo)
{
	struct cpuport_info_t *pktinfo2;

	fwk_mutex_lock(&cpuport_history_list_mutex);

	cpuport_history_clear_to_size(omci_env_g.cpuport_history_size-1);

	if ((pktinfo2 = cpuport_history_clone_pktinfo(pktinfo)) == NULL) {
		fwk_mutex_unlock(&cpuport_history_list_mutex);
		return -1;
	}
	list_add_tail(&pktinfo2->node, &cpuport_history_list_g);
	cpuport_history_total++;

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	return 0;
}

// sync info from the pktinfo being processed to the one kept in history log
int
cpuport_history_sync(struct cpuport_info_t *pktinfo)
{
	struct cpuport_info_t *pktinfo2 = pktinfo->history;

	if (pktinfo2 == NULL)
		return -1;
	// assume a pktinfo is added to history after the thread_name, dispatch_time is determined
	// so the only information needs to be sync after pkt is forwarded is the forward_time
	pktinfo2->forward_time = pktinfo->forward_time;
	return 0;
}

int
cpuport_history_summary(int fd)
{
	struct cpuport_info_t *pktinfo;
	int index = cpuport_history_index;
	int start, end, i;
	unsigned long port_pktcount[SWITCH_LOGICAL_PORT_TOTAL] = {0};
	unsigned long task_pktcount[ENV_SIZE_TASK] = {0};
	unsigned long task_pktcount_unknown = 0;

	if (cpuport_history_total == 0) {
		if (omci_env_g.cpuport_history_enable == 0) {
			util_fdprintf(fd, "please issue 'env cpuport_history_enable 1' first\n");
		} else {
			util_fdprintf(fd, "empty\n");
		}
		return 0;
	}

	fwk_mutex_lock(&cpuport_history_list_mutex);

	start = cpuport_history_index + 1;
	end = cpuport_history_index + cpuport_history_total;

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		int taskindex = cpuport_history_locate_taskindex_by_name(pktinfo->thread_name);
		if (taskindex>=0)
			task_pktcount[taskindex]++;
		else
			task_pktcount_unknown++;
		if (pktinfo->rx_desc.logical_port_id < SWITCH_LOGICAL_PORT_TOTAL)
			port_pktcount[pktinfo->rx_desc.logical_port_id]++;
	}
	for (i=0; i < ENV_SIZE_TASK; i++) {
		if (task_pktcount[i] >0)
			util_fdprintf(fd, "%s:%lu  ", omci_env_g.taskname[i], task_pktcount[i]);
	}
	if (task_pktcount_unknown >0)
		util_fdprintf(fd, "?:%lu", task_pktcount_unknown);
	util_fdprintf(fd, "\n");
	
	for (i=0; i < SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (port_pktcount[i] >0)
			util_fdprintf(fd, "%s(%d):%lu  ", switch_get_logical_portname(i), i, port_pktcount[i]);
	}
	util_fdprintf(fd, "\n\n");

	if (end -10 +1 > start)
		start = end -10 +1;
	util_fdprintf(fd, "last %d packets:\n", end - start + 1);
	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (index < start)
			continue;
		if (index > end)
			break;
		cpuport_history_pktinfo2line(fd, index, pktinfo);
	}

	util_fdprintf(fd, "\ntotal %d packet (%d..%d)\n",
		cpuport_history_total,
		cpuport_history_index + 1, cpuport_history_index + cpuport_history_total);

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	return 0;
}

// start, end is referred only if both >0 && start <= end
// keyword is referred only if it is not NULL
int
cpuport_history_list(int fd, int start, int end, char *keyword)
{
	struct cpuport_info_t *pktinfo;
	int index = cpuport_history_index;
	int total = 0;

	if (start >0 && end >0 && start > end) {
		util_fdprintf(fd, "start index %d must <= end index %d\n");
		return -1;
	}

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (start >0 && end >0 && index < start)
			continue;
		if (start >0 && end >0 && index > end)
			break;
		if (keyword && strcmp(keyword, pktinfo->thread_name)!= 0)
			continue;
		cpuport_history_pktinfo2line(fd, index, pktinfo);
		total++;
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	util_fdprintf(fd, "total %d packet\n", total);
	return 0;
}

// start, end is referred only if both >0 && start <= end
// keyword is referred only if it is not NULL
int
cpuport_history_listp(int fd, int start, int end, char *keyword)
{
	struct cpuport_info_t *pktinfo;
	unsigned char prompt[64]={0};
	int index = cpuport_history_index;
	int total = 0;

	if (start >0 && end >0 && start > end) {
		util_fdprintf(fd, "start index %d must <= end index %d\n");
		return -1;
	}

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (start >0 && end >0 && index < start)
			continue;
		if (start >0 && end >0 && index > end)
			break;
		if (keyword && strcmp(keyword, pktinfo->thread_name)!= 0)
			continue;
		cpuport_history_pktinfo2line(fd, index, pktinfo);
		snprintf(prompt, 63, "thread-%s", (pktinfo->thread_name)?pktinfo->thread_name:"?");
		cpuport_sniffer_print_pkthdr(fd, prompt, pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);
		
		total++;
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	util_fdprintf(fd, "total %d packet\n", total);
	return 0;
}

// start, end is referred only if both >0 && start <= end
// keyword is referred only if it is not NULL
int
cpuport_history_show(int fd, int start, int end, char *keyword)
{
	struct cpuport_info_t *pktinfo;
	unsigned char prompt[64]={0};
	int index = cpuport_history_index;
	int total = 0;
	int i;

	if (start >0 && end >0 && start > end) {
		util_fdprintf(fd, "start index %d must <= end index %d\n");
		return -1;
	}

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (start >0 && end >0 && index < start)
			continue;
		if (start >0 && end >0 && index > end)
			break;
		if (keyword && strcmp(keyword, pktinfo->thread_name)!= 0)
			continue;
		cpuport_history_pktinfo2line(fd, index, pktinfo);

		util_fdprintf(fd, "rx_desc: p2p_cfg=%d, logical_port_id=%d, router_mode=%d\n",
		pktinfo->rx_desc.p2p_cfg, pktinfo->rx_desc.logical_port_id, pktinfo->rx_desc.router_mode);
		if (pktinfo->rx_desc.bridge_port_me) {
			util_fdprintf(fd, "rx_desc: bridge_port_me 0x%x, rx_wan_total=%d",
				pktinfo->rx_desc.bridge_port_me->meid, pktinfo->rx_desc.rx_wan_total);
		} else {
			util_fdprintf(fd, "rx_desc: bridge_port_me NULL, rx_wan_total=%d",
				pktinfo->rx_desc.rx_wan_total);
		}
		if (pktinfo->rx_desc.rx_wan_total > 0) {
			util_fdprintf(fd, ", rx_wan_me");
			for (i=0; i<pktinfo->rx_desc.rx_wan_total; i++) {
				if  (pktinfo->rx_desc.rx_wan_me[i])
					util_fdprintf(fd, " 0x%x", pktinfo->rx_desc.rx_wan_me[i]->meid);
			}
		}
		util_fdprintf(fd, "\n");
			
		util_fdprintf(fd, "tci: %dtag, otpid:0x%x, p:%d, de:%d, vid:0x%x(%d), itpid:0x%x, p:%d, de:%d, vid:0x%x(%d)\n",
			cpuport_util_get_tagnum_from_tci(&pktinfo->tci),
			pktinfo->tci.otci.tpid, pktinfo->tci.otci.priority, pktinfo->tci.otci.de, pktinfo->tci.otci.vid, pktinfo->tci.otci.vid,
			pktinfo->tci.itci.tpid, pktinfo->tci.itci.priority, pktinfo->tci.itci.de, pktinfo->tci.itci.vid, pktinfo->tci.itci.vid);	
		util_fdprintf(fd, "frame_ptr=%p, nh=%p, diff=%d\n", 
			pktinfo->frame_ptr, pktinfo->nh, (void*)(pktinfo->nh) - (void*)(pktinfo->frame_ptr));

#if 0
		util_fdprintf(fd, "time: recv=%lu.%06lu, ", (unsigned long)pktinfo->recv_time.tv_sec, (unsigned long)pktinfo->recv_time.tv_usec);
		util_fdprintf(fd, "disp=%lu.%06lu, ", (unsigned long)pktinfo->dispatch_time.tv_sec, (unsigned long)pktinfo->dispatch_time.tv_usec);
		util_fdprintf(fd, "proc=%lu.%06lu, ", (unsigned long)pktinfo->process_time.tv_sec, (unsigned long)pktinfo->process_time.tv_usec);
		util_fdprintf(fd, "fwd=%lu.%06lu\n", (unsigned long)pktinfo->forward_time.tv_sec, (unsigned long)pktinfo->forward_time.tv_usec);
#endif
		snprintf(prompt, 63, "thread-%s", (pktinfo->thread_name)?pktinfo->thread_name:"?");
		cpuport_sniffer_print_pkt(fd, prompt, pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);
		total++;
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	util_fdprintf(fd, "total %d packet\n", total);
	return 0;
}

// list packet deferred for more than N micro-seconds
int
cpuport_history_defer(int fd, unsigned long defer_us)
{
	struct cpuport_info_t *pktinfo;
	int index = cpuport_history_index;
	int total = 0;

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > defer_us) {
			cpuport_history_pktinfo2line(fd, index, pktinfo);
			total++;
		}
	}
	fwk_mutex_unlock(&cpuport_history_list_mutex);

	util_fdprintf(fd, "total %d packet deferred\n", total);

	return 0;
}

// start, end is referred only if both >0 && start <= end
// keyword is referred only if it is not NULL
int
cpuport_history_run(int fd, int start, int end, char *keyword)
{
	struct list_head cpuport_history_list_run;;
	struct cpuport_info_t *pktinfo, *pktinfo2;
	int index = cpuport_history_index;
	int total = 0;
	struct timeval tv_zero = {0, 0};

	if (start >0 && end >0 && start > end) {
		util_fdprintf(fd, "start index %d must <= end index %d\n");
		return -1;
	}

	INIT_LIST_HEAD(&cpuport_history_list_run);
	
	// clone pkt and add to a temporary list, so we can release the mutex ASAP
	fwk_mutex_lock(&cpuport_history_list_mutex);
	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (start >0 && end >0 && index < start)
			continue;
		if (start >0 && end >0 && index > end)
			break;
		if (keyword && strcmp(keyword, pktinfo->thread_name)!= 0)
			continue;
		if ((pktinfo2 = cpuport_history_clone_pktinfo(pktinfo)) == NULL) {
			fwk_mutex_unlock(&cpuport_history_list_mutex);
			break;
		}
		list_add_tail(&pktinfo2->node, &cpuport_history_list_run);
	}
	fwk_mutex_unlock(&cpuport_history_list_mutex);

	// since the node might be freed by other thread after cpuport_recv_dispatch(), so we use list_for_entry_safe
	list_for_each_entry_safe(pktinfo, pktinfo2, &cpuport_history_list_run, node) {
		dbprintf_cpuport(LOG_WARNING, "recv pkt from cpuport history <=====\n");
		list_del(&pktinfo->node);
		pktinfo->recv_time = pktinfo->dispatch_time = pktinfo->process_time = pktinfo->forward_time = tv_zero;
		cpuport_recv_dispatch(pktinfo);
		total++;
		usleep(1000);	// sleep so other thread can handle the pkt asap since cli task priority is quite high
	}

	util_fdprintf(fd, "total %d packet executed\n", total);
	return 0;
}

// start, end is referred only if both >0 && start <= end
// keyword is referred only if it is not NULL
int
cpuport_history_save(int fd, char *filename, int start, int end, char *keyword)
{
	struct cpuport_info_t *pktinfo;
	int index = cpuport_history_index;
	int total = 0;
	FILE *fp;

	if (start >0 && end >0 && start > end) {
		util_fdprintf(fd, "start index %d must <= end index %d\n");
		return -1;
	}

	if ((fp = fopen(filename, "a")) == NULL) {
		util_fdprintf(fd, "%s open for write error (%s)\n", filename, strerror(errno));
		return -1;
	}

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (start >0 && end >0 && index < start)
			continue;
		if (start >0 && end >0 && index > end)
			break;
		if (keyword && strcmp(keyword, pktinfo->thread_name)!= 0)
			continue;
		cpuport_history_pktinfo2file(fp, index, pktinfo);
		total++;
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	fclose(fp);
	util_fdprintf(fd, "total %d packet saved in %s\n", total, filename);
	return 0;
}

int
cpuport_history_load(int fd, char *filename)
{
	struct cpuport_info_t pktinfo_mem;
	struct cpuport_info_t *pktinfo = &pktinfo_mem;
	char *framebuff, *linebuff;

	int frame_len=0;
	int is_frame_data=0;
	int total=0;
	FILE *fp;

	if ((fp = fopen(filename, "r")) == NULL) {
		util_fdprintf(fd, "file %s load error (%s)\n", filename,  strerror(errno));
		return -1;
	}

	linebuff = malloc_safe(1024);
	framebuff = malloc_safe(2*CPUPORT_VTAG_LEN + CPUPORT_BUF_SIZE);

	// init the pktinfo
	memset(pktinfo, 0, sizeof(struct cpuport_info_t));
	pktinfo->buf_ptr = framebuff;
	pktinfo->frame_ptr = pktinfo->buf_ptr + 2*CPUPORT_VTAG_LEN;
	pktinfo->buf_len = CPUPORT_BUF_SIZE;
	pktinfo->frame_len = frame_len = 0;

	while (fgets(linebuff, 1024, fp)) {
		if (is_frame_data) {
			int n = conv_hexstr_to_mem(pktinfo->frame_ptr+frame_len, 128, linebuff);
			if (n>0)
				frame_len+=n;
			if (n==0 || frame_len>=pktinfo->frame_len) {
				is_frame_data = 0;
				// end of packet content parse, add it to history log
				if (cpuport_hw_g.frame_recv_fill_after_load)
					cpuport_hw_g.frame_recv_fill_after_load(pktinfo);
				cpuport_history_add(pktinfo);
				total++;
				// init the pktinfo
				memset(pktinfo, 0, sizeof(struct cpuport_info_t));
				pktinfo->buf_ptr = framebuff;
				pktinfo->frame_ptr = pktinfo->buf_ptr + 2*CPUPORT_VTAG_LEN;
				pktinfo->buf_len = CPUPORT_BUF_SIZE;
				pktinfo->frame_len = frame_len = 0;
			}
		} else {
			char *line = util_trim(linebuff);
			char key[128], value[128];

			if (line[0] == '#')
				continue;	// comment line, skip
			if (cpuport_history_read_key_value(key, 128, value, 128, line) <0)
				continue;	// not a key=value line, skip
			if (strcmp(key, "p2p_cfg") == 0) {
				pktinfo->rx_desc.p2p_cfg = util_atoi(value);
			} else if (strcmp(key, "logical_port_id") == 0) {
				pktinfo->rx_desc.logical_port_id = util_atoi(value);
			} else if (strcmp(key, "router_mode") == 0) {
				pktinfo->rx_desc.router_mode = util_atoi(value);
			} else if (strcmp(key, "thread") == 0) {
				int taskindex = cpuport_history_locate_taskindex_by_name(value);				
				pktinfo->thread_name = (taskindex>=0)?omci_env_g.taskname[taskindex]:"?";
			} else if (strcmp(key, "recv_time") == 0) {
				cpuport_history_str2timeval(&pktinfo->recv_time, value);
			} else if (strcmp(key, "dispatch_time") == 0) {
				cpuport_history_str2timeval(&pktinfo->dispatch_time, value);
			} else if (strcmp(key, "process_time") == 0) {
				cpuport_history_str2timeval(&pktinfo->process_time, value);
			} else if (strcmp(key, "forward_time") == 0) {
				cpuport_history_str2timeval(&pktinfo->forward_time, value);
			} else if (strcmp(key, "frame_len") == 0) {
				pktinfo->frame_len = util_atoi(value);
				is_frame_data = 1;
				frame_len = 0;
				// start to parse the pkt content in next line
			}
		}
	}

	free_safe(linebuff);
	free_safe(framebuff);
	fclose(fp);

	util_fdprintf(fd, "%d records loaded\n", total);
	return(0);
}

static unsigned char zero_bytes[6] = {0};
int
cpuport_history_prepare_find(int fd, struct cpuport_history_find_t *find, int argc, char *argv[])
{
	int i;
	memset(find, 0, sizeof(struct cpuport_history_find_t));
	for (i = 0; i < argc; i++) {
		if (strncmp(argv[i], "rx_logical_port_mask=", 21)==0) {
			find->rx_logical_port_mask = util_atoi(argv[i]+21);
		} else if (strncmp(argv[i], "mac_addr=", 9)==0) {
			conv_str_mac_to_mem(find->mac_addr, argv[i]+9);
		} else if (strncmp(argv[i], "mac_mask=", 9)==0) {
			conv_str_mac_to_mem(find->mac_mask, argv[i]+9);
		} else if (strncmp(argv[i], "eth_proto=", 10) == 0) {
			find->eth_proto = util_atoi(argv[i]+10);
		} else if (strncmp(argv[i], "ip_proto=", 9) == 0) {
			find->ip_proto = util_atoi(argv[i]+9);
		} else if (strncmp(argv[i], "ip_addr=", 8) == 0) {
			int addr[4];
			if (sscanf(argv[i]+8, "%d.%d.%d.%d", addr,addr+1,addr+2,addr+3)==4) {
				if (addr[0]>=0 && addr[0]<=255 &&
				    addr[1]>=0 && addr[1]<=255 &&
				    addr[2]>=0 && addr[2]<=255 &&
				    addr[3]>=0 && addr[3]<=255) {
				    	find->ip_addr[0]=addr[0];
				    	find->ip_addr[1]=addr[1];
				    	find->ip_addr[2]=addr[2];
				    	find->ip_addr[3]=addr[3];
				}
			}
		} else if (strncmp(argv[i], "tcpudp_port=", 12) == 0) {
			find->tcpudp_port=util_atoi(argv[i]+12);
		}
	}

	if (memcmp(find->mac_addr, zero_bytes, 6) !=0 &&
	    memcmp(find->mac_mask, zero_bytes, 6) ==0)
	    	memset(find->mac_mask, 0xff, 6);
	return 0;
}

static int
cpuport_history_pktinfo_is_hit(struct cpuport_info_t *pktinfo, struct cpuport_history_find_t *find)
{
	struct iphdr *ip = pktinfo->nh;
	struct arphdr *arp = pktinfo->nh;

	if (find->rx_logical_port_mask != 0) {
		if ((find->rx_logical_port_mask & (1<<pktinfo->rx_desc.logical_port_id)) ==0)
			return 0;
	}
	if (memcmp(find->mac_addr, zero_bytes, 6) != 0) {
		int i, smac_hit = 1, dmac_hit = 1;
		for (i=0; i<6; i++) {
			if ((find->mac_addr[i] & find->mac_mask[i]) != (pktinfo->src_mac[i] & find->mac_mask[i])) {
				smac_hit = 0;
				break;
			}
		}
		for (i=0; i<6; i++) {
			if ((find->mac_addr[i] & find->mac_mask[i]) != (pktinfo->dst_mac[i] & find->mac_mask[i])) {
				dmac_hit = 0;
				break;
			}
		}
		if (!smac_hit && !dmac_hit)
			return 0;
	}	
	if (find->eth_proto != 0) {
		dbprintf_cpuport(LOG_INFO, "find->eth_proto=0x%x, eth_proto=0x%x\n", find->eth_proto, pktinfo->tci.ethertype);		
		if (find->eth_proto != pktinfo->tci.ethertype)
			return 0;
	}
	if (memcmp(find->ip_addr, zero_bytes, 4) != 0) {	// once ip is specfied, match ethtype 0x800 only
		if (pktinfo->tci.ethertype == 0x0800) {
			unsigned char *s=(unsigned char*)&(ip->saddr);
			unsigned char *d=(unsigned char*)&(ip->daddr);
			dbprintf_cpuport(LOG_INFO, "find->ip_addr=%d.%d.%d.%d, IP: sip=%d.%d.%d.%d, dip=%d.%d.%d.%d\n", 
				find->ip_addr[0],find->ip_addr[1],find->ip_addr[2],find->ip_addr[3], 
				s[0],s[1],s[2],s[3], d[0],d[1],d[2],d[3]);
			if (memcmp(find->ip_addr, s, 4) != 0 && memcmp(find->ip_addr, d, 4) != 0)
				return 0;
		} else if (pktinfo->tci.ethertype == 0x0806) {
			unsigned char *smac=(void *)arp + sizeof(struct arphdr);
			unsigned char *s=smac+6;
			unsigned char *dmac=s+4;
			unsigned char *d=dmac+6;
			dbprintf_cpuport(LOG_INFO, "find->ip_addr=%d.%d.%d.%d, ARP: sip=%d.%d.%d.%d, dip=%d.%d.%d.%d\n", 
				find->ip_addr[0],find->ip_addr[1],find->ip_addr[2],find->ip_addr[3], 
				s[0],s[1],s[2],s[3], d[0],d[1],d[2],d[3]);
			if (memcmp(find->ip_addr, s, 4) != 0 && memcmp(find->ip_addr, d, 4) != 0)
				return 0;
		} else {
			return 0;	
		}
	}	
	if (find->ip_proto != 0) {	// once ip_proto specfied, match ip only
		if (pktinfo->tci.ethertype == 0x0800) {
			dbprintf_cpuport(LOG_INFO, "find->ip_proto=%d, ip->proto=%d\n", find->ip_proto, ip->protocol);
			if (find->ip_proto != ip->protocol)
				return 0;
		} else {
			return 0;
		}
	}					
	if (find->tcpudp_port !=0) {	// once tcpudp_port is specfied, ethertype 0x800, ip_proto tcp/udp is assumed
		struct tcphdr *tcp=(void *)ip+ip->ihl*4;
		struct udphdr *udp=(void *)ip+ip->ihl*4;	
		unsigned short sp, dp;
		if (pktinfo->tci.ethertype == 0x0800) {
			if (ip->protocol==IPPROTO_TCP) {
				sp=ntohs(tcp->source);
				dp=ntohs(tcp->dest);
			} else if (ip->protocol==IPPROTO_UDP) {
				sp=ntohs(udp->source);
				dp=ntohs(udp->dest);
			} else {
				return 0;
			}
		} else {
			return 0;	
		}
		dbprintf_cpuport(LOG_INFO, "find->tcpudp_port=%d, sp=%d, dp=%d\n", find->tcpudp_port, sp, dp);		
		if (find->tcpudp_port != sp && find->tcpudp_port !=dp)
			return 0;
	}
	return 1;
}

int
cpuport_history_list_find(int fd, struct cpuport_history_find_t *find)
{
	struct cpuport_info_t *pktinfo;
	int index = cpuport_history_index;
	int total = 0;

	fwk_mutex_lock(&cpuport_history_list_mutex);

	list_for_each_entry(pktinfo, &cpuport_history_list_g, node) {
		index++;
		if (cpuport_history_pktinfo_is_hit(pktinfo, find)) {
			cpuport_history_pktinfo2line(fd, index, pktinfo);
			total++;
		}
	}

	fwk_mutex_unlock(&cpuport_history_list_mutex);

	util_fdprintf(fd, "total %d packet found\n", total);
	return 0;
}
