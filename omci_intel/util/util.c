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
 * Module  : util
 * File    : util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>
#include <dirent.h>

#include <sys/sysinfo.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "memdbg.h"
#include "util.h"
#include "util_run.h"
#include "env.h"
#include "metacfg_adapter.h"

extern int errno;

static void
util_critical_error_msg(char *msg)
{
	int console_fd = open("/dev/console", O_RDWR);
	if (console_fd >=0 ) {
		util_fdprintf(console_fd, msg);	// write dev console so it wont be redirected
		close(console_fd);
	}
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, msg);
}

#include <asm/byteorder.h>
// 0: little endian, 1: big endian, -1: endian mismatch between platform & include file
int
util_check_endian(void)
{
	unsigned short number=0x1234;
	char *a=(char*)&number;
	int platform_is_big_endian = -1;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	int include_is_big_endian = 0;
#elif defined (__BIG_ENDIAN_BITFIELD)
	int include_is_big_endian = 1;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	if (a[0] == 0x12) {
		platform_is_big_endian = 1;
	} else if (a[0] == 0x34) {
		platform_is_big_endian = 0;
	}
	if (include_is_big_endian != platform_is_big_endian) {
		util_critical_error_msg("ENDIAN MISMATCH! Please fix <asm/byteorder.h>\n");
		//return -1;
		exit(1);
	}
	return platform_is_big_endian;
}

int
util_check_hwnat(void)
{
	FILE *fp = fopen("/proc/rg/debug_level", "r");
	int has_hwnat = 0;

	if (fp != NULL) {
		fclose(fp);
		has_hwnat = 1;
	}
	if (has_hwnat == 1) {
		util_critical_error_msg("Program needs SNAT, but platform is HNAT!\n");
		exit(1);
		//return -1;
	}

	return has_hwnat;
}

char *
util_purename(char *path)
{
	char *delim=rindex(path, '/');
	return delim?(delim+1):path;
}

#if 1
// precision to usec, need link -lrt
int
util_get_uptime(struct timeval *timeval)
{
	static struct timespec prev_t;
	struct timespec t;
	struct sysinfo s_info;
	int i;

	for (i=0; i<10; i++) {
		if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
			prev_t = t;
			timeval->tv_sec = t.tv_sec;
			timeval->tv_usec = t.tv_nsec/1000;
			return 0;
		}
	}

	// try sysinfo first
	if (sysinfo(&s_info) == 0 && s_info.uptime > prev_t.tv_sec) {
		timeval->tv_sec = s_info.uptime;
		timeval->tv_usec = 0;
	} else {	// use previous time
		timeval->tv_sec = prev_t.tv_sec;
		timeval->tv_usec = prev_t.tv_nsec/1000;
	}

	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "clock_gettime error?\n");
	return -1;
}
#else
#define FWK_TIMER_PROC_UPTIME_PATH "/proc/uptime"
// precision to 10ms
int
util_get_uptime(struct timeval *timeval)
{
	FILE *fp;
	char uptimestring[32];
	int i, len;

	if ((fp = fopen(FWK_TIMER_PROC_UPTIME_PATH, "r")) == NULL)
		return (-1);
	fscanf(fp, "%s", uptimestring);
	fclose(fp);

	len = strlen(uptimestring);
	for (i = 0; i < len; i++) {
		if (uptimestring[i] == '.') {
			uptimestring[i] = 0x00;
			break;
		}
	}
	if (i >= len)
		return (-1);

	timeval->tv_sec = atoi(uptimestring);
	timeval->tv_usec = atoi(&uptimestring[i + 1]) * 10000;

	return 0;
}
#endif
// precision to sec
long
util_get_uptime_sec(void)
{
	struct sysinfo s_info;
	int error = sysinfo(&s_info);
	if (error != 0)
		return -1;
	return s_info.uptime;
}

char *
util_uptime_str(void)
{
	static char buff_wheel[4][32];
	static int index = 0;

	char *buff = buff_wheel[(index++)%4];
	struct timeval tv;

	util_get_uptime(&tv);
	snprintf(buff, 32, "%d.%06d", (int)tv.tv_sec, (int)tv.tv_usec);
	return buff;
}

char *
util_gettimeofday_str(void)
{
	static char buff_wheel[4][32];
	static int index = 0;

	char *buff = buff_wheel[(index++)%4];
	struct timeval tv;
	//struct tm *t;

	gettimeofday(&tv, NULL);
	//t=(struct tm *)localtime(&tv.tv_sec);
	//sprintf(buff, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
	//	t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec/1000);
	sprintf(buff, "%d.%06d", (int)tv.tv_sec, (int)tv.tv_usec);
	return buff;
}

int
util_dir_is_available(char *dirname)
{
	DIR *d=opendir(dirname);
	if (d) {
		closedir(d);
		return 1;
	}
	return 0;
}

int
util_file_is_available(char *filename)
{
	FILE *fp=fopen(filename, "r");
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

// for filename /path_xxx/KEYWORD, return if /var/run/cmdq/*.KEYWORD.cmd exist
#define CMD_DISPATCHER_QUEUE	"/var/run/cmd_queue"
int
util_file_in_cmd_dispatcher_queue (char *filename_keyword)
{
#ifdef OMCI_CMD_DISPATCHER_ENABLE
	char filename_postfix[512];
	DIR *dirp;
	struct dirent *dp;
	int found=0;
	char *filename_start;

	if (filename_keyword == NULL)
		return 0;
	if ((dirp = opendir(CMD_DISPATCHER_QUEUE)) == NULL) {
		//dbprintf(LOG_ERR, "%s open err\n", CMD_DISPATCHER_QUEUE);
		return 0;
	}

	// exclude path from filename_keyword
	if ((filename_start = rindex(filename_keyword, '/')) != NULL)
		filename_start++;
	else
		filename_start = filename_keyword;
	snprintf(filename_postfix, 511, ".%s.cmd", filename_start);	// .run.30075.095473.KEYWORD.cmd or 30075.095473.KEYWORD.cmd
	filename_postfix[511] = 0;

	while ((dp = readdir(dirp)) != NULL) {
		if (strstr(dp->d_name, filename_postfix)) {
			found = 1;
			break;
		}
	}
	(void)closedir(dirp);
	return found;
#else
	return 0;
#endif
}

long long
util_file_size(char *filename)
{
	struct stat st;
	int fd;
	if ((fd = open(filename, O_RDONLY)) == -1)
		return -1;
	if (fstat(fd, &st) == -1) {
		close(fd);
		return -2;
	}
	close(fd);
	return st.st_size;
}

int
util_file_rotate(char *logfile, int rotate_size, int rotate_max)
{
	char cmd[PATHNAME_MAXLEN];
	char filename1[PATHNAME_MAXLEN], filename2[PATHNAME_MAXLEN];
	int num;
	long long filesize = util_file_size(logfile);

	if (filesize <0 || rotate_size <0 || rotate_max <0)
		return -1;

	if (filesize < rotate_size)
		return 0;
	if (rotate_max>9)
		rotate_max = 9;

	// eg: when max==3, try to mv logfile.[21].gz to logfile.[32].gz
	for (num=9; num>=1; num--) {
		snprintf(filename1, PATHNAME_MAXLEN, "%s.%d.gz", logfile, num); filename1[PATHNAME_MAXLEN-1]=0;
		if (util_file_is_available(filename1)) {
			if (num +1 <= rotate_max) {
				snprintf(filename2, PATHNAME_MAXLEN, "%s.%d.gz", logfile, num+1); filename2[PATHNAME_MAXLEN-1]=0;
				rename(filename1, filename2);
			} else {
				unlink(filename1);
			}
		}
	}
	snprintf(cmd, PATHNAME_MAXLEN, "cat %s|gzip -9 > %s.1.gz", logfile, logfile); cmd[PATHNAME_MAXLEN-1]=0;
	system(cmd);
	truncate(logfile, 0);
	return 1;
}

int
util_logprintf(char *logfile, char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	int ret;

	util_file_rotate(logfile, 1024*1000, 3);	// rotate to logfile.[0123].gz if over 1Mbyte

	if ((fp = fopen(logfile, "a")) == NULL)
		return (-1);

	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);

	if (fclose(fp) == EOF)
		return (-2);	/* flush error? */

	return ret;
}

static int
util_write_safe(int fd, char *data, int len)
{
	int i;

	for (i=0; i<3; i++) {
		fd_set wmask;
		struct timeval t;
		t.tv_sec=0;
		t.tv_usec=1000;	// 1ms

		FD_ZERO (&wmask);
		FD_SET (fd, &wmask);

		// in uclibc, writing to an nonexist fd may cause seg fault
		// so we use select() to check if a fd is valid
		// 0 means timeout but the fd is valid, >0 means fd is writable
		if (select(fd+1, NULL, &wmask, NULL, &t) >=0)
			return write(fd, data, len);

		// select return -1
		if (errno == EINTR)	// try again if syscall interrupted
			continue;
		else	// EBADF, EINVAL, ENOMEM
			return -1;
	}
	return -1;
}

int
util_fdprintf(int fd, char *fmt, ...)
{
#if 0
	static char line[LINE_MAXSIZE];
	va_list ap;

	if (fmt==NULL)
		return 0;
	va_start(ap, fmt);
	vsnprintf(line, LINE_MAXSIZE, fmt, ap);
	va_end(ap);
	line[LINE_MAXSIZE-1]=0;
	return util_write_safe(fd, line, strlen(line));
#else
	FILE *fp = NULL;
	va_list ap;
	int i, ret, fd2;

	//The fd is not dup'ed, and will be closed when the stream created by fdopen() is closed,
	//so we create fd2 for fdopen(), thus we can close it with fclose().

	for (i=0; i<10; i++) {
		if ((fd2 = dup(fd)) >=0)
			break;
		usleep(100);
	}
	if (fd2 <0)
		return -1;

	for (i=0; i<10; i++) {
		if ((fp = fdopen(fd2, "r+")) != NULL)
			break;
		usleep(100);
	}
	if (fp == NULL)
		return -1;

	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	//note: signal(SIGPIPE, SIG_IGN) is done in clitask.c to maskoff pipe err
	//       which might happen when write to a closed fd
	va_end(ap);
	fclose(fp);
	return ret;
#endif
}

static int util_dbprintf_console_fd=STDERR_FILENO;	// stderr
void
util_dbprintf_set_console_fd(int fd)
{
	if (fd>0)
		util_dbprintf_console_fd=fd;
}
int
util_dbprintf_get_console_fd(void)
{
	return util_dbprintf_console_fd;
}
int
util_dbprintf(int debug_threshold, int debug_level, unsigned char log_time, char *fmt, ...)
{
	int len=0, ret=0, ret2=0, ret3=0;
	int log_to_console=omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_CONSOLE;
	int log_to_file=(omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_FILE) && omci_env_g.debug_log_file;
	int log_to_syslog=omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_SYSLOG;

	if (debug_level > debug_threshold || fmt==NULL)
		return 0;

	if (log_to_console || log_to_file || log_to_syslog) {
		va_list ap;
		char *line = malloc_safe(LINE_MAXSIZE);
		char *s=line;

		if (log_time)
			s+=snprintf(s, LINE_MAXSIZE, "%s ", util_uptime_str());
		va_start(ap, fmt);
		s+=vsnprintf(s, LINE_MAXSIZE-(s-line), fmt, ap);
		va_end(ap);
		line[LINE_MAXSIZE-1]=0;
		len=strlen(line);

		if (log_to_console) {
			ret=util_write_safe(util_dbprintf_console_fd, line, len);
		}
		if (log_to_file) {
			FILE *fp;
			util_file_rotate(omci_env_g.debug_log_file, 1024*1000, 3);	// rotate to logfile.[123].gz if over 1Mbyte
			if ((fp = fopen(omci_env_g.debug_log_file, "a")) != NULL) {
				ret2=fprintf(fp, "%s", line);
				fclose(fp);
			}
		}
		if (log_to_syslog) {	// try to collect one line before calling syslog
			static char syslog_linebuf[1024];
			char *newline=strrchr(line, '\n');
			if (newline) {
				*newline=0;
				strncat(syslog_linebuf, line, 1023);
				openlog("omcimain", LOG_PID, LOG_USER);
				syslog(debug_level, "%s\n", syslog_linebuf);
				closelog();
				strncpy(syslog_linebuf, newline+1, 1023);
			} else {
				strncat(syslog_linebuf, line, 1023);
			}
		}

		free_safe(line);
	}

	if (ret<0 && ret2<0 && ret3 <0)	// all log fails
		return -1;
	return (len);
}

int
util_omcipath_filename(char *buff, int buff_size, char *fmt, ...)
{
	char *omcipath[2]={omci_env_g.etc_omci_path, omci_env_g.etc_omci_path2};
	char *vendor[2]={omci_env_g.olt_proprietary_support, "00base"};
	char *filename = malloc_safe(1024);
	char *file = malloc_safe(512);
	va_list ap;
	int i, v;

	va_start(ap, fmt);
	vsnprintf(file, 511, fmt, ap);
	va_end(ap);

	if (omci_env_g.debug_level >=LOG_NOTICE) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "search %s...\n", file);
	}

	// file are located in the following order
	// p[0]/v[0]	/etc/omci/vendor
	// p[1]/v[0]	/opt/omci/vendor
	// p[0]/v[1]	/etc/omci/common
	// p[1]/v[1]	/opt/omci/common
	for (v=0; v<2; v++) {
		if (strlen(vendor[v])==0 || strcmp(vendor[v], "none")==0)
			continue;
		for (i=0; i<2; i++) {
			struct stat state;
			snprintf(filename, 1024, "%s/%s/%s", omcipath[i], vendor[v], file);
			if (v==1 && i==0) // by default, fill buff with omcipath[0]/common/filename
				strncpy(buff, filename, buff_size);
			if (stat(filename, &state) == 0) {
				strncpy(buff, filename, buff_size);
				if (omci_env_g.debug_level >=LOG_NOTICE) {
					util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "File %s found\n", filename);
				}
				free_safe(filename);
				free_safe(file);
				return strlen(buff);
			}
		}
	}

	free_safe(filename);
	free_safe(file);
	return -1;
}

char *
util_rootdir(void)
{
	static char dirname_wheel[4][256];
	static int index = 0;

	char *dirname = dirname_wheel[(index++)%4];
	char filename[256];
	char *p;

	if (strlen(dirname)>0)
		return dirname;

	snprintf(dirname, 255, "%s", omci_env_g.etc_omci_path); dirname[255] = 0;
	while ((p=strrchr(dirname, '/'))!=NULL) {
		*p=0;
		if (strlen(dirname)>0) {
			snprintf(filename, 255, "%s/%s", dirname, "etc/passwd"); filename[255] = 0;
			if (util_file_is_available(filename))
				return dirname;
		}
	}
	sprintf(dirname, "/");
	return dirname;
}

int
util_rootdir_filename(char *buff, int buff_size, char *fmt, ...)
{
	char file[512];
	va_list ap;
	char *rootdir=util_rootdir();
	char *f=file;

	va_start(ap, fmt);
	vsnprintf(file, sizeof(file), fmt, ap);
	va_end(ap);

	if (*f == '/')
		f++;
	if (rootdir[strlen(rootdir)-1] =='/') {
		snprintf(buff, buff_size, "%s%s", rootdir, f);
	} else {
		snprintf(buff, buff_size, "%s/%s", rootdir, f);
	}
	return 0;
}

/* strtrim : removes leading spaces of a string */
static inline char *
strtrim(char *s)
{
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

/* strrtrim : removes trailing spaces from the end of a string */
static inline char *
strrtrim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	return s;
}

/* util_trim : removes leading and trailing spaces of a string */
char *
util_trim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

/* util_quotation_marks_trim : removes leading and trailing quotation marks of a string */
char *
util_quotation_marks_trim(char *s)
{
	int i;
	//remove tail
	if (s) {
		i = strlen(s);
		while ((--i) > 0 && (s[i]=='\'' || s[i]=='"'))
			s[i] = 0;
	}

	//remove head
	if (s) {
		while (*s=='\'' || *s=='"')
			s++;
	}
	return s;
}

unsigned long long
util_ll_reverse(unsigned long long num, unsigned int len)
{
	unsigned long long result;
	char *pos_src = (char *) &num, *pos_dest = (char *) &result;
	int i, j;

	result = num;

	if (len != 0) {
		if (len > 8)
			len = 8;

		for (i = 0, j = len - 1; j >= i; i++, j--) {
			pos_dest[i] = pos_src[j];
			pos_dest[j] = pos_src[i];
		}
	}

	return result;
}

char *
util_ltoa(long l)
{
	static char s_wheel[4][32];
	static int index = 0;

	char *s = s_wheel[(index++)%4];
	snprintf(s, 32, "%ld", l);
	return (s);
}

int
util_atoi(char *s)
{
	char *p;
	return (int) strtol(s, &p, 0);
}

long long int
util_atoll(char *s)
{
	char *p;
	return strtoll(s, &p, 0);
}

unsigned long long int
util_atoull(char *s)
{
	char *p;
	return strtoull(s, &p, 0);
}


// the util_bitmap_set function will convert value from host order to network order
void
util_bitmap_set_value(unsigned char *bitmap, int bitmap_bitwidth,
		      int value_start_bitnum, int value_bitwidth, unsigned long long value)
{
	int i, j;

	for (i = value_start_bitnum, j = 0; i < bitmap_bitwidth && j < value_bitwidth; i++, j++) {
		if (value & ((unsigned long long) 1 << (value_bitwidth - 1 - j))) {
			util_bitmap_set_bit(bitmap, i);
		} else {
			util_bitmap_clear_bit(bitmap, i);
		}
	}
}

// the util_bitmap_get function will convert value from network order to host order
unsigned long long
util_bitmap_get_value(unsigned char *bitmap, int bitmap_bitwidth, int value_start_bitnum, int value_bitwidth)
{
	int i, j;
	unsigned long long value = 0;

	for (i = value_start_bitnum, j = 0; i < bitmap_bitwidth && j < value_bitwidth; i++, j++) {
		if (util_bitmap_get_bit(bitmap, i))
			value |= ((unsigned long long) 1 << (value_bitwidth - 1 - j));
	}
	return value;
}

int
util_strcmp_safe(char *a, char *b)
{
	if (a && b)
		return (strcmp(a, b));
	else if (a == NULL && b)
		return -1;
	else if (a && b == NULL)
		return 1;
	else
		return 0;
}

int
util_strncmp_safe(char *a, char *b, int n)
{
	if (a && b)
		return (strncmp(a, b, n));
	else if (a == NULL && b)
		return -1;
	else if (a && b == NULL)
		return 1;
	else
		return 0;
}

char *
util_strcasestr(char *haystack, const char *needle)
{
	char *h;
	const char *n;

	h = haystack;
	n = needle;
	while (*haystack) {
		if (tolower((unsigned char) *h) == tolower((unsigned char) *n)) {
			h++;
			n++;
			if (!*n)
				return haystack;
		} else {
			h = ++haystack;
			n = needle;
		}
	}
	return NULL;
}

char *
util_strptr_safe(char *str)
{
	if (str)
		return str;
	return "?";
}

int
util_str_trim_tailcomment(char *str)
{
	int i;

	for (i=0; str[i]!=0; i++) {
		if (str[i]=='#') {
			str[i]=0;
			break;
		}
	}
	return i;
}

int
util_str_remove_dupspace(char *str)
{
	char *s, *t;

	strrtrim(str);	// remove tail space

	s=t=str;
	while (*s) {
		if (*s==' ' && *(s+1)==' ') {
			s++;
			continue;
		}
		if (s!=t)
			*t=*s;
		s++; t++;
	}
	*t=*s;

	return strlen(str);
}

char *
util_str_macaddr(unsigned char *macaddr)
{
	static char str_wheel[4][24];
	static int index = 0;

	char *str = str_wheel[(index++)%4];
	snprintf(str, 24, "%02x:%02x:%02x:%02x:%02x:%02x",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	return str;
}

void *
util_malloc_safe(char *filename, int linenum, char *funcname, size_t size)
{
	void *ptr=malloc(size);

	if (ptr==NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d %s(), malloc(%d) return null?\n", util_purename(filename), linenum, funcname, size);
		exit(1);
	}
	memset(ptr, 0x00, size);

	if (omci_env_g.memdbg_enable)
		memdbg_add(ptr, size, filename, linenum, funcname);

	return ptr;
}

void
util_free_safe(char *filename, int linenum, char *funcname, void **ptr)
{
	if (*ptr==NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d %s(), free null?\n", util_purename(filename), linenum, funcname);
		exit(2);
	}

	if (omci_env_g.memdbg_enable)
		memdbg_del(*ptr, filename, linenum, funcname);

	free(*ptr);
	// set to null so we will catah access to the stale pointer if seg fault
	*ptr=0;
}

long
util_readfile(const char *pathname, char **data)
{
	int fd;
	struct stat st;
	long len;

	*data=NULL;
	fd = open(pathname, O_RDONLY);
	if (fd == -1)
		return -1;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return -2;
	}

	*data = (char *) malloc_safe(st.st_size + 1);
	if (*data == NULL) {
		close(fd);
		return -3;
	}

	len = read(fd, *data, st.st_size);
	if (len != st.st_size) {
		close(fd);
		return -4;
	}
	close(fd);
	(*data)[len] = '\0';

	return len;
}

long
util_readcmd(const char *cmd, char **data)
{
	char buff[1024];
	char filename[64];
	FILE *tmpf;
	long len;

	snprintf(filename, 64, "/tmp/tmpfile.%d.%06ld", getpid(), random()%1000000);
	tmpf = fopen(filename, "a+");
	fclose(tmpf);
	snprintf(buff, 1024, "%s > %s 2>/dev/null", cmd, filename);

	util_run_by_system(buff);
	len = util_readfile(filename, data);
	unlink(filename);

	return (len);
}

void
util_setenv_number(char *name, int number)
{
	char buff[32];
	snprintf(buff, 32, "%d", number);
	//dbprintf(LOG_EMERG, "name=%s, str=%s\n", name, buff);
	setenv(name, buff, 1);
}

void
util_setenv_ipv4(char *name, unsigned int ip) // the ip here is in network order!
{
	struct in_addr ipaddr;
	char *s;
	ipaddr.s_addr=htonl(ip);
	s=inet_ntoa(ipaddr);
	//dbprintf(LOG_EMERG, "name=%s, ip=0x%x, str=%s\n", name, ip, s);
	setenv(name, s, 1);
}

void
util_setenv_str(char *name, char *str)
{
	if (str)
		setenv(name, str, 1);
	else
		setenv(name, "", 1);
}

int
util_get_dmesg_level(void)
{
	char buff[8];
	int fd, level=4;

	if ((fd=open("/proc/sys/kernel/printk", O_RDONLY))>0) {
		read(fd, buff, 8);
		close(fd);
		level = atoi(buff);
	}
	return level;
}

int
util_set_dmesg_level(int level)
{
	char buff[8];
	int ret, fd;

	if ((fd=open("/proc/sys/kernel/printk", O_RDWR))<0) {
		return fd;
	}
	snprintf(buff, 8, "%d\n", level);
	ret=write(fd, buff, 8);
	close(fd);
	return ret;
}

int
util_create_lockfile(char *filename, int timeout)
{
	char buff[64];
	int fd, t;

	snprintf(buff, 64, "%u", getpid());
	for (t=0; t <= timeout *10; t++) {
		if ((fd=open(filename, O_CREAT|O_EXCL))>=0) {
			write(fd, buff, strlen(buff));
			close(fd);
			return 0;
		}
		usleep(100*1000);	// 0.1 sec
	}
	return -1;
}

int
util_release_lockfile(char *filename)
{
	if (unlink(filename)<0)
		return -1;
	return 0;
}

// ret 0: not found, 1: found
int
util_get_value_from_file(char *filename, char *key, char *value, int size)
{
	FILE *fp = fopen(filename, "r");
	char line[LINE_MAXSIZE], *s;

	if (fp == NULL)
		return -1;

	while( fgets(line, LINE_MAXSIZE, fp) != NULL) {
		if (line[0]=='#')	// comment, ignore
			continue;
		if ((s = strchr(line, '=')) == NULL)
			continue;	// '=' not found in this line, ignore
		*s = 0;			// sep line into 2 part: key and value
		if (strncmp(key, util_trim(line), size) == 0) {
			strncpy(value, util_trim(s+1), size);
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

void
util_get_localtime(struct tm *ts)
{
	time_t time_l;
	time(&time_l);
	localtime_r(&time_l, ts);
}

int
util_set_thread_name(char *name)
{
	if (prctl(PR_SET_NAME, name, 0, 0, 0) <0) {
		return -1;
	}
	return 0;
}

char *
util_get_thread_name(void)
{
	static char thread_name_wheel[4][17];
	static int index = 0;

	char *thread_name = thread_name_wheel[(index++)%4];

	thread_name[16]=0;
	if (prctl(PR_GET_NAME, thread_name, 0, 0, 0) <0)
		return "?";
	return thread_name;
}

int
util_str_is_number(char *str)
{
	int i, len=strlen(str);
	int is_str=0;
	int has_hex=0, number_of_dot=0;

	for (i=0; i<len; i++) {
		if (str[i]>='0' && str[i]<='9') {
			continue;
		} else if ((str[i]>='A' && str[i]<='F')||
			   (str[i]>='a' && str[i]<='f')) {
			has_hex=1;
			continue;
		} else if (str[i]=='+' || str[i]=='-') {
			if (i==0)
				continue;
		} else if (str[i]=='.') {
			number_of_dot++;
			continue;
		} else if (str[i]=='X' || str[i]=='x') {
			if (i==1 && str[0]=='0') {
				has_hex=1;
				continue;
			}
		}
		is_str=1;
	}

	if (is_str==0) {
		if (number_of_dot==0)		// integer or hex
			return 1;
		else if (number_of_dot==1 && !has_hex)	// float
			return 2;
	}
	return 0;
}

char *
util_get_bootenv(char *key)
{
	static char buff_wheel[4][128];
	static int index = 0;

	char *buff = buff_wheel[(index++)%4];
	char cmd[128];
	char *output=NULL;
	FILE *fp;

	// try fw_printenv.conf first
	if ((fp=fopen("/var/run/fw_printenv.conf","r")) != NULL) {
		int key_len=strlen(key);
		while (fgets(buff, 128, fp)) {
			if (strncmp(buff, key, key_len)==0 && buff[key_len]=='=') {
				fclose(fp);
				return util_trim(buff+key_len+1);
			}
		}
		fclose(fp);
	}
	// then try flash, much slower
	snprintf(cmd, 128, "%s %s", FW_PRINTENV_BIN, key);
	if (util_readcmd(cmd, &output)>0) {
		if (output) {
			char *value = NULL;
			strtok(util_trim(output), "="); // parse key first
			if((value=strtok(NULL, "\n")) != NULL) // extract value from string "key=value"
				strncpy(buff, value, 128);
			free_safe(output);
			return value ? buff : NULL;
		}
	}
	return NULL;
}

char *
util_bps_to_ratestr(unsigned long long bps)
{
	static char ratestr_wheel[4][32];	// trick to allow re-entry 0..3 times
	static int index = 0;

	char *ratestr = ratestr_wheel[(index++)%4];
	unsigned int value;

	if (bps > 1024*1024) {
		value = bps *100ULL/(1000*1000);
		snprintf(ratestr, 31, "%u.%02uMbps", value/100, value%100);
	} else {
		value = bps*100/1000;
		snprintf(ratestr, 31, "%u.%02uKbps", value/100, value%100);
	}
	return ratestr;
}

unsigned long long
util_ull_value_diff(unsigned long long value, unsigned long long value_prev)
{
	if (value >= value_prev) {
		return value - value_prev;
	} else { // try to handle overwrap happened on hw by guessing the upper limit of hw counter
		if (value_prev & 0xffffffff00000000ULL)
			return value + 1 + (0xffffffffffffffffULL - value_prev);
		else if (value_prev & 0xffff0000)
			return value + 1 + (0xffffffffUL - value_prev);
		else if (value_prev & 0xff00)
			return value + 1 + (0xffffUL - value_prev);
		else
			return value + 1 + (0xffUL - value_prev);
	}
}

char *
util_fgets(char *s, int size, FILE *stream, int eliminate_space, int eliminate_comment)
{
	char *buff = malloc_safe(LINE_MAXSIZE);
	int sstart=0;
	int read_next_line=0;

	do {
		int len, i;
		char *b=buff;

		read_next_line=0;
		if (fgets(buff, LINE_MAXSIZE, stream)==NULL) {
			free_safe(buff);
			if (sstart>0)
				return s;
			else
				return NULL;
		}

		if(eliminate_comment) {
			if (buff[0]=='#') {
				read_next_line=1;
				continue;
			}
			util_str_trim_tailcomment(buff);
		}

		len=strlen(buff);

		for (i=len-1; i>=0; i--) {
			if ((buff[i]==' ' && eliminate_space) || buff[i]=='\n' || buff[i]=='\r') {
				buff[i]=0;
			} else if (buff[i]=='\\') {
				buff[i]=0; read_next_line=1;
			} else {
				break;
			}
		}
		if (eliminate_space) {
			for (i=0; i<len; i++) {
				if (buff[i]!=' ') {
					b=&buff[i];
					break;
				}
			}
		}

		len=strlen(b);
		if (size-sstart-1 > len) {
			strcpy(s+sstart, b);
			sstart+=len;
		} else {
			strncpy(s+sstart, b, size-sstart-1);
			s[size-1]=0;
			free_safe(buff);
			return s;
		}
	} while (read_next_line);

	free_safe(buff);
	return s;
}

/* escape (1) double quote, (2) dollar, (3) backquote, (4) backslash for shell string */
char *
util_str_shell_escape(char *str)
{
	static char buff_wheel[4][1024];
	static int index = 0;

	char *buff = buff_wheel[(index++)%4];
	char *s=str, *t=buff;
	while(*s) {
		if (*s=='"' || *s=='$' || *s=='`' || *s=='\\' || *s=='{' || *s=='}') {
			*t='\\'; t++;
		}
		*t=*s; t++; s++;
		if (t-buff>=1023)
			break;
	}
	*t=0;

	return(buff);
}

int
util_string_is_number(char *str)
{
	char *s = str;
	int i;

	if (s[0] == '+' || s[0] == '-')
		s++;
	if (strncmp(s, "0x", 2) == 0 ||
	    strncmp(s, "0X", 2) == 0) {
		s+=2;
		for (i=0; i<strlen(s); i++)
			if (!isxdigit(s[i]))
				return 0;
	} else {
		for (i=0; i<strlen(s); i++)
			if (!isdigit(s[i]))
				return 0;
	}
	return 1;
}

int
util_write_file(char *file, char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	int ret;

	if ((fp = fopen(file, "a")) == NULL)
		return -1;
	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);
	if (fclose(fp) == EOF)
		return -2;	/* flush error? */
	return ret;
}

char *
util_readlink(char *pathname)
{
	static char realpath[1024];
	int ret = readlink(pathname, realpath, 1023);

	realpath[1023] = 0;
	if (ret >=0) {
		char *e;
		int dirname_len, realpath_len;

		realpath[ret] = 0;
		if (realpath[0] == '/' || (e = strrchr(pathname, '/')) == NULL)
			return realpath;
		dirname_len = e-pathname+1;	// dirname including the tail /
		realpath_len = strlen(realpath);
		if (dirname_len + realpath_len <1024) {
			memmove(realpath+dirname_len, realpath, realpath_len+1);	// realpth plus the termination 0
			memcpy(realpath, pathname, dirname_len);
			return realpath;
		}
	}
	return pathname;
}

/*
  strtok_r work on string_p
  array[] pointer to string_p
  return the number of token
  caller have to free memory
 */
int
util_string2array(char *string_p, char *delim, char *array[], int array_size)
{
	int i;
	char *token, *saveptr;
	for (i=0; i<array_size; i++, string_p=NULL) {
		token = strtok_r(string_p, delim, &saveptr);
		if (token == NULL)
			break;
		array[i] = token;
	}
	return i;
}

/* return value -1:error 0:no_change 1:changed */
int
util_is_file_changed (char *filepath, time_t *update_time_pre)
{
	int ret=0;
	struct tm *foo;
	struct stat attrib;
	time_t update_time;
	if (filepath == NULL) {
		/* dbprintf(LOG_ERR, "filepath is NULL\n"); */
		return -1;
	}
	stat(filepath, &attrib);
	foo = gmtime(&(attrib.st_mtime));
	update_time = mktime(foo);
	if (*update_time_pre <= 0) {
		ret=1;
		/* dbprintf(LOG_NOTICE, "file[%s] initialization\n", filepath); */
	} else if (*update_time_pre == update_time) {
		ret=0;
		/* dbprintf(LOG_DEBUG, "file[%s] not change\n", filepath); */
	} else {
		ret=1;
		/* dbprintf(LOG_NOTICE, "file[%s] changed!\n", filepath); */
	}
	*update_time_pre = update_time;
	return ret;
}

#ifdef OMCI_METAFILE_ENABLE
int
util_kv_load_voip(struct metacfg_t *kv_config, char *filename)
{
	int ret=0;
	if (0 == (ret=metacfg_adapter_config_file_load_group(kv_config, filename, "voip_"))) {
		ret=metacfg_adapter_config_file_load_group(kv_config, filename, "system_voip_");
	}
	return ret;
}
#endif

int
util_string_concatnate(char **dest, char *src, int dest_max_len)
{
	int l, ll;
	if (*dest == NULL || src == NULL || dest_max_len <= 0) {
		return -1;
	}
	if (0 < (l=strlen(src))) {
		ll=strlen(*dest);
		if (l >= dest_max_len-ll) {
			char *p=NULL, *q=NULL;
			p = (char *)malloc_safe(ll+1);
			if (p == NULL) {
				return -1;
			}
			*p='\0';
			strncat(p, *dest, ll);
			q = (char *)malloc_safe(ll+dest_max_len+l);
			if (q == NULL) {
				free_safe(p);
				return -1;
			}
			free_safe(*dest);
			*dest = q;
			strncat(*dest, p, ll);
			free_safe(p);
		}
		strncat(*dest, src, l);
	}
	return 0;
}
