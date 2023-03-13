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
 * File    : util.h
 *
 ******************************************************************/

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <netinet/in.h>
#include <time.h>

#include "env.h"
#include "metacfg_adapter.h"

#define MAYBE_UNUSED __attribute__((unused))
#define FW_PRINTENV_BIN	"/usr/sbin/fw_printenv"
#define LINE_MAXSIZE	2048
#define PATHNAME_MAXLEN	1024

#if 1				// definition from /usr/include/syslog.h
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

#define dbprintf(level, fmt, args...)		do { if (omci_env_g.debug_level>=(level)) 		util_dbprintf(omci_env_g.debug_level, 		level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_bat(level, fmt, args...)	do { if (omci_env_g.debug_level_bat>=(level)) 		util_dbprintf(omci_env_g.debug_level_bat, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_cpuport(level, fmt, args...)	do { if (omci_env_g.debug_level_cpuport>=(level)) 	util_dbprintf(omci_env_g.debug_level_cpuport, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_cfm(level, fmt, args...)	do { if (omci_env_g.debug_level_cfm>=(level)) 		util_dbprintf(omci_env_g.debug_level_cfm, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_fwk(level, fmt, args...)	do { if (omci_env_g.debug_level_fwk>=(level)) 		util_dbprintf(omci_env_g.debug_level_fwk, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_igmp(level, fmt, args...)	do { if (omci_env_g.debug_level_igmp>=(level)) 		util_dbprintf(omci_env_g.debug_level_igmp, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_lldp(level, fmt, args...)	do { if (omci_env_g.debug_level_lldp>=(level)) 		util_dbprintf(omci_env_g.debug_level_lldp, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_vacl(level, fmt, args...)	do { if (omci_env_g.debug_level_vacl>=(level)) 		util_dbprintf(omci_env_g.debug_level_vacl, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_voip(level, fmt, args...)	do { if (omci_env_g.debug_level_voip>=(level)) 		util_dbprintf(omci_env_g.debug_level_voip, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define dbprintf_xml(level, fmt, args...)	do { if (omci_env_g.debug_level_xml>=(level)) 		util_dbprintf(omci_env_g.debug_level_xml, 	level, omci_env_g.debug_log_time, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args); } while (0)
#define malloc_safe(size)		util_malloc_safe(__FILE__, __LINE__, (char*)__FUNCTION__, size)
#define free_safe(addr)			util_free_safe(__FILE__, __LINE__, (char*)__FUNCTION__, (void *)&(addr))
#define msleep(ms)			usleep((ms)*1000)

/* this bitmap format is used for OMCI table entry,
   majorly used by util_bitmap_set_value(), util_bimap_get_value()
a. bit num started from 0
b. bit order in each byte is bit7 in msb, bit0 in lsb
c. byte order is little endian, byte 0 in lower memory
*/

static inline void
util_bitmap_set_bit(unsigned char *bitmap, int bitnum)
{
	bitmap[bitnum / 8] |= (1 << (7 - (bitnum % 8)));	// set bit
}

static inline void
util_bitmap_clear_bit(unsigned char *bitmap, int bitnum)
{
	bitmap[bitnum / 8] &= ~(1 << (7 - (bitnum % 8)));	// clear bit
}

static inline int
util_bitmap_get_bit(unsigned char *bitmap, int bitnum)
{
	return ((bitmap[bitnum / 8] >> (7 - (bitnum % 8))) & 1);	// get bit
}

/* this bitmap format is used for OMCI alarm masks
   it follows the definition in OMCI appendix II 1.5
   a. bitnum started from 0
   b. bit order in each byte is bit1 in msb, bit8 in lsb
   c. byte order is little endian, byte 0 in lower memory
*/
static inline void
util_alarm_mask_set_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 0 && bitnum < 224)
		mask[bitnum / 8] |= 1 << (7 - bitnum % 8);	// set bit
}

static inline void
util_alarm_mask_clear_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 0 && bitnum < 224)
		mask[bitnum / 8] &= ~(1 << (7 - bitnum % 8));	// clear bit
}

static inline int
util_alarm_mask_get_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 0 && bitnum < 224)
		return (mask[bitnum / 8] >> (7 - bitnum % 8)) & 1;	// get bit
	return -1;
}

static inline int
util_is_have_alarm_mask(unsigned char *mask)
{
	int i;
	for (i = 0; i < 28; i++) {
		if (mask[i] != 0)
			return 1;
	}
	return 0;
}

/* this bitmap format is used for OMCI attr mask used in avc/tca bitmaps, attr/exec mask,
   it follows the definition in OMCI appendix II 1.4
   a. bitnum started from 1, 
   b. bit order in each byte is bit1 in msb, bit8 in lsb
   c. byte order is little endian, byte 0 in lower memory
*/
static inline void
util_attr_mask_set_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 1 && bitnum <= 16)
		mask[(bitnum - 1) / 8] |= 1 << (7 - (bitnum - 1) % 8);	// set bit
}

static inline void
util_attr_mask_clear_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 1 && bitnum <= 16)
		mask[(bitnum - 1) / 8] &= ~(1 << (7 - (bitnum - 1) % 8));	// clear bit
}

static inline int
util_attr_mask_get_bit(unsigned char *mask, int bitnum)
{
	if (bitnum >= 1 && bitnum <= 16)
		return (mask[(bitnum - 1) / 8] >> (7 - (bitnum - 1) % 8)) & 1;	// get bit
	return -1;
}

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x)       (x)
#define htonll(x)       (x)
#define ntohll2(x)       (x)
#define htonll2(x)       (x)
#define ntohll3(x)       (x)
#define htonll3(x)       (x)
#define ntohll4(x)       (x)
#define htonll4(x)       (x)
#define ntohll5(x)       (x)
#define htonll5(x)       (x)
#define ntohll6(x)       (x)
#define htonll6(x)       (x)
#define ntohll7(x)       (x)
#define htonll7(x)       (x)
#define ntohll8(x)       (x)
#define htonll8(x)       (x)

#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x)     util_ll_reverse (x, 8)
#define htonll(x)     util_ll_reverse (x, 8)
#define ntohll2(x)     util_ll_reverse (x, 2)
#define htonll2(x)     util_ll_reverse (x, 2)
#define ntohll3(x)     util_ll_reverse (x, 3)
#define htonll3(x)     util_ll_reverse (x, 3)
#define ntohll4(x)     util_ll_reverse (x, 4)
#define htonll4(x)     util_ll_reverse (x, 4)
#define ntohll5(x)     util_ll_reverse (x, 5)
#define htonll5(x)     util_ll_reverse (x, 5)
#define ntohll6(x)     util_ll_reverse (x, 6)
#define htonll6(x)     util_ll_reverse (x, 6)
#define ntohll7(x)     util_ll_reverse (x, 7)
#define htonll7(x)     util_ll_reverse (x, 7)
#define ntohll8(x)     util_ll_reverse (x, 8)
#define htonll8(x)     util_ll_reverse (x, 8)

#endif
#endif

static inline char
util_aschex2char(char c)
{
	return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
}

static inline unsigned int
util_compare_ether_addr(unsigned char *addr1, unsigned char *addr2)
{
	const unsigned short *a = (const unsigned short *) addr1;
	const unsigned short *b = (const unsigned short *) addr2;
	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

static inline long long
util_timeval_diff_usec(struct timeval *end, struct timeval *start)
{
	return (end->tv_sec - start->tv_sec) * 1000000LL +  end->tv_usec - start->tv_usec;
}
                        
static inline long
util_timeval_diff_msec(struct timeval *end, struct timeval *start)
{
	return (end->tv_sec - start->tv_sec) * 1000 +  (end->tv_usec - start->tv_usec)/1000;
}
                        
/* util.c */
int util_check_endian(void);
int util_check_hwnat(void);
char *util_purename(char *path);
int util_get_uptime(struct timeval *timeval);
long util_get_uptime_sec(void);
char *util_uptime_str(void);
char *util_gettimeofday_str(void);
int util_dir_is_available(char *dirname);
int util_file_is_available(char *filename);
int util_file_in_cmd_dispatcher_queue(char *filename_keyword);
long long util_file_size(char *filename);
int util_file_rotate(char *logfile, int rotate_size, int rotate_max);
int util_logprintf(char *logfile, char *fmt, ...);
int util_fdprintf(int fd, char *fmt, ...);
void util_dbprintf_set_console_fd(int fd);
int util_dbprintf_get_console_fd(void);
int util_dbprintf(int debug_threshold, int debug_level, unsigned char log_time, char *fmt, ...);
int util_omcipath_filename(char *buff, int buff_size, char *fmt, ...);
char *util_rootdir(void);
int util_rootdir_filename(char *buff, int buff_size, char *fmt, ...);
char *util_trim(char *s);
char *util_quotation_marks_trim(char *s);
unsigned long long util_ll_reverse(unsigned long long num, unsigned int len);
char *util_ltoa(long l);
int util_atoi(char *s);
long long int util_atoll(char *s);
unsigned long long int util_atoull(char *s);
void util_bitmap_set_value(unsigned char *bitmap, int bitmap_bitwidth, int value_start_bitnum, int value_bitwidth, unsigned long long value);
unsigned long long util_bitmap_get_value(unsigned char *bitmap, int bitmap_bitwidth, int value_start_bitnum, int value_bitwidth);
int util_strcmp_safe(char *a, char *b);
int util_strncmp_safe(char *a, char *b, int n);
char *util_strcasestr(char *haystack, const char *needle);
char *util_strptr_safe(char *str);
int util_str_trim_tailcomment(char *str);
int util_str_remove_dupspace(char *str);
char *util_str_macaddr(unsigned char *macaddr);
void *util_malloc_safe(char *filename, int linenum, char *funcname, size_t size);
void util_free_safe(char *filename, int linenum, char *funcname, void **ptr);
long util_readfile(const char *pathname, char **data);
long util_readcmd(const char *cmd, char **data);
void util_setenv_number(char *name, int number);
void util_setenv_ipv4(char *name, unsigned int ip);
void util_setenv_str(char *name, char *str);
int util_get_dmesg_level(void);
int util_set_dmesg_level(int level);
int util_create_lockfile(char *filename, int timeout);
int util_release_lockfile(char *filename);
int util_get_value_from_file(char *filename, char *key, char *value, int size);
void util_get_localtime(struct tm *ts);
int util_set_thread_name(char *name);
char *util_get_thread_name(void);
int util_str_is_number(char *str);
char *util_get_bootenv(char *key);
char *util_bps_to_ratestr(unsigned long long bps);
unsigned long long util_ull_value_diff(unsigned long long value, unsigned long long value_prev);
char *util_fgets(char *s, int size, FILE *stream, int eliminate_space, int eliminate_comment);
char *util_str_shell_escape(char *str);
int util_string_is_number(char *str);
int util_write_file(char *file, char *fmt, ...);
char *util_readlink(char *pathname);
int util_string2array(char *string_p, char *delim, char *array[], int array_size);
int util_is_file_changed(char *filepath, time_t *update_time_pre);
#ifdef OMCI_METAFILE_ENABLE
int util_kv_load_voip(struct metacfg_t *kv_config, char *filename);
#endif
int util_string_concatnate(char **dest, char *src, int dest_max_len);

#endif

