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
 * Module  : omciutil
 * File    : omciutil_usage.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include "util.h"
#include "regexp.h"

#define CPU_COUNTER_TOTAL 	7	// user nice sys idle io irq softirq
#define CPU_COUNTER_IDLE	3
#define CPU_COUNTER_ONE_SEC	100ULL	// the counter unit is jiffies, so 100 = 1sec

static inline char *
skip_token(const char *p)
{
	while (isspace(*p))
		p++;
	while (*p && !isspace(*p))
		p++;
	return (char *)p;
}

// the counter are actually the diffs of various counters
static int
omciutil_usage_cpu_counter_get(unsigned long *total_counter, unsigned long cpu_counter[])
{
	static unsigned long cp_old[CPU_COUNTER_TOTAL];	// near
	static unsigned long cp_old2[CPU_COUNTER_TOTAL];	// far
	unsigned long cp_curr[CPU_COUNTER_TOTAL];
	char buffer[512];
	int fd, len, i;
	char *p;

	fd = open("/proc/stat", O_RDONLY);
	if (fd<0)
		return -1;
	len = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	buffer[len] = '\0';

	p = skip_token(buffer);	/* "cpu" */
	for (i = 0; i < CPU_COUNTER_TOTAL; i++) {
		char *oldp = p;
		cp_curr[i] = strtoul(p, &p, 0);
		if (cp_curr[i] == 0 && oldp == p)
			break;
	}

	/* use near point to calc changes */
	*total_counter = 0;
	for (i = 0; i < CPU_COUNTER_TOTAL; i++) {
		if (cp_curr[i] > cp_old[i])
			*total_counter += (cpu_counter[i] = cp_curr[i] - cp_old[i]);
		else		/* this only happens when the counter wraps */
			*total_counter += (cpu_counter[i] = cp_old[i] - cp_curr[i]);
	}

	if (*total_counter >= CPU_COUNTER_ONE_SEC) {	// near point is more than 1 sec ago
		for (i = 0; i < CPU_COUNTER_TOTAL; i++) {
			cp_old2[i] = cp_old[i];
			cp_old[i] = cp_curr[i];
		}
	} else {				// near point is less than 1 sec ago, use far point instead
		*total_counter = 0;
		for (i = 0; i < CPU_COUNTER_TOTAL; i++) {
			if (cp_curr[i] > cp_old2[i])
				*total_counter += (cpu_counter[i] = cp_curr[i] - cp_old2[i]);
			else	/* this only happens when the counter wraps */
				*total_counter += (cpu_counter[i] = cp_old2[i] - cp_curr[i]);
		}
	}

	if (*total_counter > 60 * CPU_COUNTER_ONE_SEC) {	// if either near/far point is more than 60 sec ago
		// set the old[],old2[] to rough values of 3 sec ago
		for (i = 0; i < CPU_COUNTER_TOTAL; i++)
			cp_old[i] = cp_old2[i] = cp_curr[i] - 3 * CPU_COUNTER_ONE_SEC * cpu_counter[i] / *total_counter;
		return omciutil_usage_cpu_counter_get(total_counter, cpu_counter);
	}

	return 0;
}

char *
omciutil_usage_cpu_counter_to_str(unsigned long total_counter, unsigned long cpu_counter1[])
{
	static char str[128];
	unsigned long cpu_counter2[CPU_COUNTER_TOTAL];
	unsigned long *cpu_counter;
	int cpu_states[CPU_COUNTER_TOTAL];
	int i;

	if (cpu_counter1!=NULL) {
		cpu_counter=cpu_counter1;
	} else {
		omciutil_usage_cpu_counter_get(&total_counter, cpu_counter2);
		cpu_counter=cpu_counter2;
	}

	if (total_counter == 0)
		total_counter = 1;
	for (i = 0; i < CPU_COUNTER_TOTAL; i++)
		cpu_states[i] = (cpu_counter[i] * 1000ULL + total_counter / 2ULL) / total_counter;

	snprintf(str, 255,
		 "%2d.%01d%%user %2d.%01d%%nice %2d.%01d%%sys %2d.%01d%%idle %2d.%01d%%io %2d.%01d%%irq %2d.%01d%%softirq",
		 cpu_states[0] / 10, cpu_states[0] % 10,
		 cpu_states[1] / 10, cpu_states[1] % 10,
		 cpu_states[2] / 10, cpu_states[2] % 10,
		 cpu_states[3] / 10, cpu_states[3] % 10, cpu_states[4] / 10, cpu_states[4] % 10, cpu_states[5] / 10, cpu_states[5] % 10, cpu_states[6] / 10, cpu_states[6] % 10);

	return str;
}

int
omciutil_usage_cpu_used_percentage(void)
{
	unsigned long total_counter, cpu_counter[CPU_COUNTER_TOTAL];
	if (omciutil_usage_cpu_counter_get(&total_counter, cpu_counter) <0)
		return 0;
#ifndef MAIN
	if (omci_env_g.debug_level >= LOG_WARNING)
		dbprintf(LOG_WARNING, "%s\n", omciutil_usage_cpu_counter_to_str(total_counter, cpu_counter));
#endif
	return (total_counter-cpu_counter[CPU_COUNTER_IDLE])*10000ULL/total_counter;
}

#ifndef CPUMAIN
static int
omciutil_usage_mem_counter_get(unsigned long *mem_total, unsigned long *mem_free)
{
	char *output=NULL;
	*mem_total = *mem_free = 0;
	static struct regexp_context_t re_memtotal;
	static struct regexp_context_t re_memfree;	
	
	if (util_readcmd("cat /proc/meminfo", &output) <0) {
		if (output)
			free_safe(output);
		return -1;
	}		

	if (!regexp_is_compiled(&re_memtotal)) {
		regexp_prepare(&re_memtotal, "MemTotal: +([0-9]+) kB");
		regexp_prepare(&re_memfree, "MemFree: +([0-9]+) kB");
	}

	if (regexp_match(&re_memtotal, output))
		*mem_total=util_atoll(regexp_match_str(&re_memtotal, 1));
	if (mem_total==0)
		*mem_total=64*1024*1024;
	if (regexp_match(&re_memfree, output))
		*mem_free=util_atoll(regexp_match_str(&re_memfree, 1));
#if 0
	regexp_release(&re_memtotal);
	regexp_release(&re_memfree);
#endif
	if (output)
		free_safe(output);

	return *mem_total;
}

int
omciutil_usage_mem_used_percentage(void)
{
	unsigned long mem_total, mem_free;
	if (omciutil_usage_mem_counter_get(&mem_total, &mem_free) <0)
		return 0;

	dbprintf(LOG_WARNING, "mem total=%dkB, free=%dkB\n", mem_total, mem_free);
	return (mem_total-mem_free)*10000ULL/mem_total;
}

#endif

#ifdef CPUMAIN
int
main(void)
{
	unsigned long total_counter, cpu_counter[CPU_COUNTER_TOTAL];

	while (1) {
		usage_cpu_counter_get(&total_counter, cpu_counter);
		printf("%s\n", usage_cpu_counter2str(total_counter, cpu_counter));
		//      usleep(100000); // 100ms
		msleep(1000);	// 1000ms
	}
}
#endif
