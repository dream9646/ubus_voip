/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
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
 * Module  : acl
 * File    : vacltest_util.c
 *
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <ctype.h>
#include "list.h"
#include "util_inet.h"
#include "conv.h"
#include "vacl.h"
#include "vacltest_util.h"

static int util_dbprintf_console_fd=STDERR_FILENO;	// stderr

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

char *
util_purename(char *path)
{
	char *delim=rindex(path, '/');
	return delim?(delim+1):path;
}

char *
util_gettimeofday_str(void)
{
	static char buff[64];
	struct timeval tv;
	//struct tm *t;

	gettimeofday(&tv, NULL);		
	//t=(struct tm *)localtime(&tv.tv_sec);
	//sprintf(buff, "%04d/%02d/%02d %02d:%02d:%02d.%03d", 
	//	t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec/1000);
	sprintf(buff, "%d.%06d", (int)tv.tv_sec, (int)tv.tv_usec);
	return buff;
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

	//if (omci_env_g.memdbg_enable)
	//	memdbg_add(ptr, size, filename, linenum, funcname);
	
	return ptr;
}

void
util_free_safe(char *filename, int linenum, char *funcname, void **ptr)
{
	if (*ptr==NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d %s(), free null?\n", util_purename(filename), linenum, funcname);
		exit(2);
	}

	//if (omci_env_g.memdbg_enable)
	//	memdbg_del(*ptr, filename, linenum, funcname);

	free(*ptr);
	// set to null so we will catah access to the sale pointer in seg fault
	*ptr=0;
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
		//if (errno == EINTR)	// try again if syscall interrupted
		//	continue;
		//else	// EBADF, EINVAL, ENOMEM
			return -1;
	}
	return -1;
}

int
util_fdprintf(int fd, char *fmt, ...)
{
	static char line[LINE_MAXSIZE];
	va_list ap;

	if (fmt==NULL)
		return 0;
	va_start(ap, fmt);
	vsnprintf(line, LINE_MAXSIZE, fmt, ap);
	va_end(ap);
	line[LINE_MAXSIZE-1]=0;
	return util_write_safe(fd, line, strlen(line));
}

int
util_dbprintf(int debug_threshold, int debug_level, unsigned char log_time, char *fmt, ...)
{
	static char line[LINE_MAXSIZE];
	int len=0, ret=0, ret2=0, ret3=0;
	int log_to_console=1;//omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_CONSOLE;
	int log_to_file=0;//(omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_FILE) && omci_env_g.debug_log_file;
	int log_to_syslog=0;//omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_SYSLOG;

	//if (debug_level > omci_env_g.debug_level || fmt==NULL)
	//	return 0;

	if (log_to_console || log_to_file || log_to_syslog) {
		va_list ap;
		char *s=line;
		
		if (log_time)
			s+=snprintf(s, LINE_MAXSIZE, "%s ", util_gettimeofday_str());
		va_start(ap, fmt);
		s+=vsnprintf(s, LINE_MAXSIZE-(s-line), fmt, ap);
		va_end(ap);
		line[LINE_MAXSIZE-1]=0;
		len=strlen(line);
		
		if (log_to_console) {
			ret=util_write_safe(util_dbprintf_console_fd, line, len);
		}
#if 0
		if (log_to_file) {
			FILE *fp = fopen(omci_env_g.debug_log_file, "a");
			if (fp) {
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
				openlog("OMCI", LOG_PID, LOG_USER);
				syslog(debug_level, "%s\n", syslog_linebuf);
				closelog();
				strncpy(syslog_linebuf, newline+1, 1023);
			} else {
				strncat(syslog_linebuf, line, 1023);
			}
		}		
#endif
	}
	if (ret<0 && ret2<0 && ret3 <0)	// all log fails
		return -1;
	return (len);
}

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
 * File    : crc.c
 *
 ******************************************************************/

/* CRC table for little endian */

static unsigned int crc_table_le[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
        0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
        0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
        0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
        0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
        0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
        0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
        0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
        0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
        0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/* generate the table of CRC remainders for all possible bytes */

#define POLYNOMIAL1 0x04c11db7L
static unsigned int crc_table_be[256];
static int is_crc_table_be_ready=0;

static void
gen_crc_table_be()
{
	register int i, j;
	register unsigned int crc_accum;
	for (i = 0; i < 256; i++) {
		crc_accum = ((unsigned int) i << 24);
		for (j = 0; j < 8; j++) {
			if (crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL1;
			else
				crc_accum = (crc_accum << 1);
		}
		crc_table_be[i] = crc_accum;
	}
	return;
}


/* update the CRC on the data block one byte at a time. */
unsigned int
crc_le_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
        char *s = data_blk_ptr;
        while (--data_blk_size >= 0)
                crc_accum = crc_table_le[(crc_accum ^ *s++) & 0xff] ^ (crc_accum >> 8);
        return crc_accum;
}

unsigned int
crc_be_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
	register int i, j;

	if (!is_crc_table_be_ready) {
	        gen_crc_table_be();
	        is_crc_table_be_ready=1;
        }

	for (j = 0; j < data_blk_size; j++) {
		i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_table_be[i];
	}
	return crc_accum;
}


int
crc_file_check(unsigned int *crc_accum, char *filename, unsigned char is_big_endian)
{

	FILE    *fptr;
	int	ch,i;

	if ( (fptr=fopen(filename,"r")) == NULL)       /*which file you want to parser*/
	{
		dbprintf(LOG_ERR, "can't open file %s!\n", filename);
		return -1;
	}
	if ( is_big_endian ) {	// big endian
		if (!is_crc_table_be_ready) {
			gen_crc_table_be();
			is_crc_table_be_ready=1;
		}
		while( (ch=fgetc(fptr)) != EOF ) {
			i = ((int) (*crc_accum >> 24) ^ ch) & 0xff;
			*crc_accum = (*crc_accum << 8) ^ crc_table_be[i];
		}
	} else {
		while( (ch=fgetc(fptr)) != EOF )
			*crc_accum = crc_table_le[(*crc_accum ^ ch) & 0xff] ^ (*crc_accum >> 8);
	}

	fclose(fptr);
	return 0;
}

//////////////////////////////////////////////
// switch relatives
int
switch_hw_fvt2510_uni_port_total(void)
{
	// UNI ports(logical port 0~4)
	return 5;
}




