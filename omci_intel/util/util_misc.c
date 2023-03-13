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
 * File    : util_misc.c
 *
 ******************************************************************/

int
util_readfile_lines(const char *pathname, char *lines[], int max)
{
	FILE *fp;
	char buff[1024];
	int linenum = 0;

	if ((fp = fopen(pathname, "rb")) == NULL)
		return -1;

	while (fgets(buff, 1024, fp) > 0) {
		int len = strlen(buff);
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if ((lines[linenum] = malloc(len + 1)) == NULL)
			break;
		strcpy(lines[linenum], buff);
		linenum++;
		if (linenum >= max)
			break;
	}

	fclose(fp);
	return linenum;
}

int
util_writefile_lines(const char *pathname, char *lines[], int total)
{
	FILE *fp;
	int linenum;

	if ((fp = fopen(pathname, "wb")) == NULL)
		return -1;

	for (linenum = 0; linenum < total; linenum++) {
		if (lines[linenum] != NULL)
			if (fprintf(fp, "%s\n", lines[linenum]) < 0)
			{
				fclose(fp);
				return -2;
			}
	}
	fclose(fp);

	return linenum;
}

void
util_free_lines(char *lines[], int total)
{
	int linenum;
	for (linenum = 0; linenum < total; linenum++) {
		if (lines[linenum] != NULL)
			free_safe(lines[linenum]);
	}
}

long
util_readfile(const char *pathname, char **data)
{
	int fd;
	struct stat st;
	long len;

#ifdef DEBUG
	util_logprintf(LOG, "%s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, pathname);
#endif
	fd = open(pathname, O_RDONLY);
	if (fd == -1) {
#ifdef DEBUG
		util_logprintf(LOG, "%s:%d:%s(): %s: %s\n", __FILE__, __LINE__, __func__, pathname, strerror(errno));
#endif
		return -1;
	}

	if (fstat(fd, &st) == -1) {
#ifdef DEBUG
		util_logprintf(LOG, "%s:%d:%s(): %s: %s\n", __FILE__, __LINE__, __func__, pathname, strerror(errno));
#endif
		close(fd);
		return -2;
	}

	*data = (char *) malloc(st.st_size + 1);
	if (*data == NULL) {
#ifdef DEBUG
		util_logprintf(LOG, "%s:%d:%s(): %s: %s\n", __FILE__, __LINE__, __func__, pathname, strerror(errno));
#endif
		close(fd);
		return -3;
	}

	len = read(fd, *data, st.st_size);
	if (len != st.st_size) {
#ifdef DEBUG
		util_logprintf(LOG, "%s:%d:%s(): %s: %s\n", __FILE__, __LINE__, __func__, pathname, strerror(errno));
#endif
		close(fd);
		return -4;
	}
	close(fd);
	(*data)[len] = '\0';

	return len;
}

#if __BSD_VISIBLE
#define uptime_output 	"08:01:56 up 2 min, load average: 0.28, 0.21, 0.08"

#define ifconfig_eth0_output \
"eth0      Link encap:Ethernet  HWaddr 00:04:23:B0:1A:49\n" \
"	  inet addr:10.20.0.2  Bcast:10.20.255.255  Mask:255.255.0.0\n" \
"	  UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1\n" \
"	  RX packets:14369604 errors:0 dropped:0 overruns:0 frame:0\n" \
"	  TX packets:14648434 errors:0 dropped:0 overruns:0 carrier:0\n" \
"	  collisions:0 txqueuelen:100\n" \
"	  RX bytes:3574639328 (3409.0 Mb)  TX bytes:2455797998 (2342.0 Mb)\n" \
"	  Base address:0xdc00 Memory:fcfe0000-fd000000\n";

#define ifconfig_eth1_output \
"eth1      Link encap:Ethernet  HWaddr 0D:0A:03:BA:D0:0F\n" \
"	  inet addr:10.20.6.73  Bcast:10.255.255.255  Mask:255.255.0.0\n" \
"	  UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1\n" \
"	  RX packets:51 errors:0 dropped:0 overruns:0 frame:0\n" \
"	  TX packets:3 errors:0 dropped:0 overruns:0 carrier:0\n" \
"	  collisions:0 txqueuelen:100\n" \
"	  RX bytes:7790 (7.6 KiB)  TX bytes:1770 (1.7 KiB)\n" \
"	  Interrupt:8\n";

#define netstat_rn_output \
"Kernel IP routing table\n" \
"Destination     Gateway	 Genmask	 Flags Metric Ref    Use Iface\n" \
"10.20.0.0       *	       255.255.0.0     U     0      0	0 eth0\n" \
"10.20.0.0       *	       255.255.0.0     U     0      0	0 eth1\n" \
"0.0.0.0	 10.20.0.1       0.0.0.0	 UG    0      0	0 eth1\n" \
"0.0.0.0	 10.20.0.1       0.0.0.0	 UG    0      0	0 eth0\n";
#endif

long
util_readcmd(const char *cmd, char **data)
{
	char buff[256];
	char filename[64];
	FILE *tmpf;
	long len;

#if __BSD_VISIBLE
	char *str = NULL;
	if (strcmp("/bin/uptime", cmd) == 0) {
		str = uptime_output;
	} else if (strcmp("/sbin/ifconfig eth0", cmd) == 0) {
		str = ifconfig_eth0_output;
	} else if (strcmp("/sbin/ifconfig eth1", cmd) == 0) {
		str = ifconfig_eth1_output;
	} else if (strcmp("/bin/netstat -rn", cmd) == 0) {
		str = netstat_rn_output;
	}
	if (str != NULL) {
		*data = malloc(strlen(str) + 1);
		strncpy(*data, str, strlen(str) + 1);
		return strlen(str);
	}
#endif
	snprintf(filename, 64, "/tmp/webctrl.tmp.%d.%06ld", getpid(), random()%1000000);
	tmpf = fopen(filename, "a+");
	fclose(tmpf);
	snprintf(buff, 256, "%s > %s 2>/dev/null", cmd, filename);

	util_run_by_system(buff);
	len = util_readfile(filename, data);
	unlink(filename);

	return (len);
}

/* if fill is null or "", then label will be removed from data */
int
util_templatefill(char **data, char *label, char *fill)
{
	char *occurance[64];	/* store the occurance of label */
	register char *src, *dst;
	const char *value;
	int found, i;
	char *newdata;

	if (*data == NULL || label == NULL || strlen(label) == 0)
		return -1;

	if (fill)
		value = fill;
	else
		value = "";

	src = (char *) *data;
	found = 0;
	while ((src = strstr(src, label)) != NULL) {
		occurance[found] = src;
		src += strlen(label);
		if (found++ == 64)
			break;
	}
	if (found == 0)
		return 0;

	newdata = (char *) malloc(strlen(*data) + (strlen(value) - strlen(label)) * found + 1);
	if (newdata == NULL) {
#ifdef DEBUG
		util_logprintf(LOG, "%s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, strerror(errno));
#endif
		return -2;
	}

	src = (char *) *data;
	dst = newdata;
	i = 0;			/* occurance */
	while (*src) {
		if (i < found && src == occurance[i]) {
			memcpy(dst, value, strlen(value));
			dst += strlen(value);
			src += strlen(label);
			i++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst = 0;
	free_safe(*data);
	*data = newdata;

	return (found);
}

int
util_is_same_subnet(char *ipstr1, char *ipstr2, char *netmaskstr)
{
	int ip1 = inet_addr(ipstr1);
	int ip2 = inet_addr(ipstr2);
	int mask = inet_addr(netmaskstr);
	return ((ip1 & mask) == (ip2 & mask));
}

static char *hex = "0123456789ABCDEF";
static unsigned char is_safe_map[96] = {	/* 0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xF, 0xE, 0x0, 0xF, 0xF, 0xC,	/* 2x  !"#$%&'()*+,-./   */
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0,	/* 3x 0123456789:;<=>?   */
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,	/* 4x @ABCDEFGHIJKLMNO   */
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0xF,	/* 5X PQRSTUVWXYZ[\]^_   */
	0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,	/* 6x `abcdefghijklmno   */
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0	/* 7X pqrstuvwxyz{\}~DEL */
};

#define is_safe_char(a)	( a>=32 && a<128 && (is_safe_map[a-32]))
static inline char
aschex2char(char c)
{
	return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
}

char *
util_escape(const char *str)
{
	static char result[256];
	const char *p;
	char *q;
	int unacceptable = 0;

	if (!str)
		return "";

	for (p = str; *p; p++) {
		if (!is_safe_char((unsigned char) *p))
			unacceptable++;
	}
	if (p - str + unacceptable + unacceptable + 1 > 256)
		return NULL;
	for (q = result, p = str; *p; p++) {
		unsigned char a = *p;
		if (!is_safe_char(a)) {
			*q++ = '%';	/* Means hex commming */
			*q++ = hex[a >> 4];
			*q++ = hex[a & 15];
		} else {
			*q++ = *p;
		}
	}
	*q++ = 0;		/* Terminate */

	return result;
}

char *
util_unescape(const char *str)
{
	static char result[256];
	char *p = (char *) str;
	char *q = result;

	if (!str || strlen(str) >= 256)
		return "";

	while (*p) {
		if (*p == '%') {
			p++;
			if (*p)
				*q = aschex2char(*p++) * 16;
			if (*p) {
				*q = *q + aschex2char(*p);
				++p;
			}
			q++;
		} else {
			*q++ = *p++;
		}
	}
	*q++ = 0;

	return result;
}

int
util_str2html(char *buf1, char *buf2, int buf2size)
{
	int i, j;

	for (i = 0, j = 0; buf1[i] != 0x0 && j < buf2size - 5; i++) {
		if (buf1[i] == '<') {
			strcpy(buf2 + j, "&lt;");
			j += 4;
		} else if (buf1[i] == '>') {
			strcpy(buf2 + j, "&gt;");
			j += 4;
		} else {
			buf2[j] = buf1[i];
			j++;
		}
	}
	buf2[j] = 0x0;
	return j;
}

int
util_password2star(char *buff)
{
	char *ptr;
	char pattern[] = "password=";

	if ((ptr = strstr(buff, pattern)) != NULL) {
		ptr += strlen(pattern);
		while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r') {
			*ptr = '*';
			ptr++;
		}
		return 1;
	}
	return 0;
}

