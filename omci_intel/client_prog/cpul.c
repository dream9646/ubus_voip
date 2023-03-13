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
 * Module  : src
 * File    : cpul.c
 *
 ******************************************************************/

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

#define TASK_SIZE 10

// process descriptor
struct cpul_process {
	//pid of the process
	pid_t pid;
	//pid of the process
	pid_t ppid;
	//start time
	int starttime;
	//is member of the family?
	int member; //TODO: delete this field
	//total number of jiffies used by the process at time last_sample
	int last_jiffies;
	//timestamp when last_jiffies and cpu_usage was calculated
	struct timeval last_sample;
	//actual cpu usage estimation (value in range 0-1)
	double cpu_usage;
	//1 if the process is zombie
	int is_zombie;
	//absolute path of the executable file
	char command[PATH_MAX+1];

	//system-dependent members
//TODO: delete these members for the sake of portability?
#ifdef __linux__
	//preallocate buffers for performance
	//name of /proc/PID/stat file
	char stat_file[20];
	//read buffer for /proc filesystem
	char buffer[1024];
#endif	
};


//USER_HZ detection, from openssl code
#ifndef HZ
# if defined(_SC_CLK_TCK) \
     && (!defined(OPENSSL_SYS_VMS) || __CTRL_VER >= 70000000)
#  define HZ ((double)sysconf(_SC_CLK_TCK))
# else
#  ifndef CLK_TCK
#   ifndef _BSD_CLK_TCK_ /* FreeBSD hack */
#    define HZ  100.0
#   else /* _BSD_CLK_TCK_ */
#    define HZ ((double)_BSD_CLK_TCK_)
#   endif
#  else /* CLK_TCK */
#   define HZ ((double)CLK_TCK)
#  endif
# endif
#endif

//control time slot in microseconds
//each slot is splitted in a working slice and a sleeping slice
//TODO: make it adaptive, based on the actual system load
#define TIME_SLOT 100000

//some useful macro
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

struct cpul_process cpul_process_pool_g[TASK_SIZE];

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
static inline unsigned long cpul_timediff(const struct timeval *t1,const struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_usec - t2->tv_usec);
}

// returns the start time of a process (used with pid to identify a process)
static int cpul_get_starttime(pid_t pid)
{
#ifdef __linux__
	char file[20];
	char buffer[1024];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *fd = fopen(file, "r");
		if (fd==NULL) return -1;
	if (fgets(buffer, sizeof(buffer), fd) == NULL) return -1;
	fclose(fd);
	char *p = buffer;
	p = memchr(p+1,')', sizeof(buffer) - (p-buffer));
	int sp = 20;
	while (sp--)
		p = memchr(p+1,' ',sizeof(buffer) - (p-buffer));
	//start time of the process
	int time = atoi(p+1);
	return time;
#elif defined __APPLE__
	struct process proc;
	get_proc_info(&proc, pid);
	return proc.starttime;
#endif
}

int cpul_process_init(struct cpul_process *proc, int pid)
{
	//general members
	proc->pid = pid;
	proc->starttime = cpul_get_starttime(pid);
	proc->cpu_usage = 0;
	memset(&(proc->last_sample), 0, sizeof(struct timeval));
	proc->last_jiffies = -1;
	//system dependent members
#ifdef __linux__
//TODO: delete these members for the sake of portability?
	//test /proc file descriptor for reading
	sprintf(proc->stat_file, "/proc/%d/stat", pid);
	FILE *fd = fopen(proc->stat_file, "r");
	if (fd == NULL) return 1;
	fclose(fd);
#endif
	return 0;
}

static int cpul_get_jiffies(struct cpul_process *proc) {
#ifdef __linux__
	FILE *f = fopen(proc->stat_file, "r");
	if (f==NULL) return -1;
	if (fgets(proc->buffer, sizeof(proc->buffer),f) == NULL) return -1;
	fclose(f);
	char *p = proc->buffer;
	p = memchr(p+1,')', sizeof(proc->buffer) - (p-proc->buffer));
	int sp = 12;
	while (sp--)
		p = memchr(p+1,' ',sizeof(proc->buffer) - (p-proc->buffer));
	//user mode jiffies
	int utime = atoi(p+1);
	p = memchr(p+1,' ',sizeof(proc->buffer) - (p-proc->buffer));
	//kernel mode jiffies
	int ktime = atoi(p+1);
	return utime+ktime;
#elif defined __APPLE__
	struct process proc2;
	get_proc_info(&proc2, proc->pid);
	return proc2.last_jiffies;
#endif
}

//parameter in range 0-1
#define ALFA 0.08

static int cpul_process_monitor(struct cpul_process *proc)
{
	int j = cpul_get_jiffies(proc);
	if (j<0) return -1; //error retrieving jiffies count (maybe the process is dead)
	struct timeval now;
	gettimeofday(&now, NULL);
	if (proc->last_jiffies==-1) {
		//store current time
		proc->last_sample = now;
		//store current jiffies
		proc->last_jiffies = j;
		//it's the first sample, cannot figure out the cpu usage
		proc->cpu_usage = -1;
		return 0;
	}
	//time from previous sample (in ns)
	long dt = cpul_timediff(&now, &(proc->last_sample));
	//how many jiffies in dt?
	double max_jiffies = dt * HZ / 1000000.0;
	double sample = (j - proc->last_jiffies) / max_jiffies;
	if (proc->cpu_usage == -1) {
		//initialization
		proc->cpu_usage = sample;
	}
	else {
		//usage adjustment
		proc->cpu_usage = (1-ALFA) * proc->cpu_usage + ALFA * sample;
	}

	//store current time
	proc->last_sample = now;
	//store current jiffies
	proc->last_jiffies = j;
	
	return 0;
}

static void
cpul_quit(int sig)
{
	int i;

	//let all the task continue if stopped
	for (i = 0; i < TASK_SIZE; i++)
	{
		if (cpul_process_pool_g[i].pid > 0)
		{
			if (kill(cpul_process_pool_g[i].pid,SIGCONT)!=0) {
				//process is dead, remove it from family
				printf("1 Process %d dead!\n", cpul_process_pool_g[i].pid);
			}
		}
	}

	//fix ^C little problem
	printf("\r");
	fflush(stdout);
	exit(0);
}

int
main(int argc, char **argv)
{
	int instance_num = 0;
	double limit;
	FILE * p_file;
	char buf[64];
	//slice of the slot in which the process is allowed to run
	struct timespec twork;
	//slice of the slot in which the process is stopped
	struct timespec tsleep;
	//when the last twork has started
	struct timeval startwork;
	//when the last twork has finished
	struct timeval endwork;
	//initialization
	memset(&twork, 0, sizeof(struct timespec));
	memset(&tsleep, 0, sizeof(struct timespec));
	memset(&startwork, 0, sizeof(struct timeval));
	memset(&endwork, 0, sizeof(struct timeval));	
	//last working time in microseconds
	unsigned long workingtime = 0;
	//counter
	int c = 0;
	double workingrate = -1;
	int i;
	struct sched_param sp;
	int policy;

	//close all file descriptors that 
	for (i = 3; i < 1024; i++)
	{
		close(i);
	}

	//read args and init process structure
	if (argc < 4)
	{
		printf("usage: cpul instance_num limit pids...\n");
		printf("	instance_num: the uniqe instance number on system\n");
		printf("	limit: maximun cpu usage for the tasks specified by pids, 0 <= limit <= 100\n");
		printf("	pids: the list of pid for cpu limitation, pid number <= %u\n", TASK_SIZE);
		exit(1);
	}

	instance_num = atoi(argv[1]);
	limit = atof(argv[2]) / 100.0;

	//init process structure
	memset(cpul_process_pool_g, 0x00, sizeof(cpul_process_pool_g));
	for (i = 0; (i < TASK_SIZE) && ((i+3) < argc); i++)
	{
		cpul_process_init(&cpul_process_pool_g[i], atoi(argv[i+3]));
	}

	//all arguments are ok!
	signal(SIGINT, cpul_quit);
	signal(SIGTERM, cpul_quit);

	//set to RR highest priority
	pthread_getschedparam(pthread_self(),&policy, &sp);
	sp.sched_priority = 99;
	pthread_setschedparam(pthread_self(), SCHED_RR, &sp);

	//create pid file
	sprintf(buf, "/var/run/cpul_%d.pid", instance_num);
	p_file = fopen(buf, "w");
	if (p_file == NULL){
		printf("Can not create pid file\n");
		exit(1);
	}else{
		fprintf(p_file,"%d",getpid());
		fclose(p_file);
        }

	while (1)
	{
		//total cpu actual usage (range 0-1)
		//1 means that the processes are using 100% cpu
		double pcpu = -1;

		//estimate how much the controlled processes are using the cpu in the working interval
		for (i = 0; i < TASK_SIZE; i++)
		{
			if (cpul_process_pool_g[i].pid > 0)
			{
				//check task pid
				if (cpul_process_monitor(&cpul_process_pool_g[i]) != 0) {
					//process is dead, remove it from family
					printf("0 Process %d dead!\n", cpul_process_pool_g[i].pid);
					continue;
				}
				if (cpul_process_pool_g[i].cpu_usage<0) {
					continue;
				}
				if (pcpu<0) pcpu = 0;
				pcpu += cpul_process_pool_g[i].cpu_usage;
			}
		}

		//adjust work and sleep time slices
		if (pcpu < 0) {
			//it's the 1st cycle, initialize workingrate
			pcpu = limit;
			workingrate = limit;
			twork.tv_nsec = TIME_SLOT*limit*1000;
		}
		else {
			//adjust workingrate
			workingrate = MIN(workingrate / pcpu * limit, 1);
			twork.tv_nsec = TIME_SLOT*1000*workingrate;
		}
		tsleep.tv_nsec = TIME_SLOT*1000-twork.tv_nsec;

/*
		if (c%200==0)
			printf("\n%%CPU\twork quantum\tsleep quantum\tactive rate\n");
		if (c%10==0 && c>0)
			printf("%0.2lf%%\t%6ld us\t%6ld us\t%0.2lf%%\n",pcpu*100,twork.tv_nsec/1000,tsleep.tv_nsec/1000,workingrate*100);
*/
		//resume processes
		for (i = 0; i < TASK_SIZE; i++)
		{
			if (cpul_process_pool_g[i].pid > 0)
			{
				if (kill(cpul_process_pool_g[i].pid,SIGCONT)!=0) {
					//process is dead, remove it from family
					printf("1 Process %d dead!\n", cpul_process_pool_g[i].pid);
				}
			}
		}

		//now processes are free to run (same working slice for all)
		gettimeofday(&startwork, NULL);
		nanosleep(&twork,NULL);
		gettimeofday(&endwork, NULL);
		workingtime = cpul_timediff(&endwork,&startwork);
		
		long delay = workingtime-twork.tv_nsec/1000;
		if (c>0 && delay>10000) {
			//delay is too much! signal to user?
			//fprintf(stderr, "%d %ld us\n", c, delay);
		}

		if (tsleep.tv_nsec>0) {
			//stop only if tsleep>0, instead it's useless
			for (i = 0; i < TASK_SIZE; i++)
			{
				if (cpul_process_pool_g[i].pid != 0)
				{
					if (kill(cpul_process_pool_g[i].pid,SIGSTOP)!=0) {
						//process is dead, remove it from family
						printf("2 Process %d dead!\n", cpul_process_pool_g[i].pid);
					}
				}
			}

			//now the processes are sleeping
			nanosleep(&tsleep,NULL);
		}
		c++;
	}

	return 0;
}

