/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI Protocol Stack
 * Module  : util
 * File    : url.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>

#include "util.h"
#include "url.h"

#define ISXDIGIT(x) (isxdigit((int) ((unsigned char)x)))

/* Portable character check (remember EBCDIC). Do not use isalnum() because
   its behavior is altered by the current locale.
   See http://tools.ietf.org/html/rfc3986#section-2.3
*/
static int
isunreserved(unsigned char in)
{
	int ret = 0;
	switch (in) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '-':
	case '.':
	case '_':
	case '~':
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int
urldecode(const char *string, size_t length, char **ostring, size_t * olen)
{
	size_t alloc = (length ? length : strlen(string)) + 1;
	char *ns = malloc_safe(alloc);
	unsigned char in;
	size_t strindex = 0;
	unsigned long hex_value;

	if (!ns)
		return -1;

	while (--alloc > 0) {
		in = *string;
		if (('%' == in) && (alloc > 2) && ISXDIGIT(string[1]) && ISXDIGIT(string[2])) {
			/* this is two hexadecimal digits following a '%' */
			char hexstr[3];
			char *ptr;
			hexstr[0] = string[1];
			hexstr[1] = string[2];
			hexstr[2] = 0;

			hex_value = strtoul(hexstr, &ptr, 16);

			assert(hex_value <= (unsigned long)0xFF);
			in = (unsigned char)(hex_value & (unsigned long)
					     0xFF);

			string += 2;
			alloc -= 2;
		}
#if 0		
		// comment out for \r\n compatibility 
		if (in < 0x20) {
			free_safe(ns);
			return -3;
		}
#endif		
		ns[strindex++] = in;
		string++;
	}
	ns[strindex] = 0;	/* terminate it */

	if (olen)
		/* store output size */
		*olen = strindex;

	if (ostring)
		/* store output string */
		*ostring = ns;

	return 0;
}

char *
url_escape(const char *string, int inlength)
{
	size_t alloc = (inlength ? (size_t) inlength : strlen(string)) + 1;
	char *ns;
	char *testing_ptr = NULL;
	unsigned char in;	/* we need to treat the characters unsigned */
	size_t newlen = alloc;
	size_t strindex = 0;
	size_t length;

	ns = malloc_safe(alloc);
	if (!ns)
		return NULL;

	length = alloc - 1;
	while (length--) {
		in = *string;

		if (isunreserved(in) == 0)
			/* just copy this */
			ns[strindex++] = in;
		else {
			/* encode it */
			newlen += 2;	/* the size grows with two, since this'll become a %XX */
			if (newlen > alloc) {
				alloc *= 2;
				testing_ptr = malloc_safe(alloc);
				if (!testing_ptr) {
					free_safe(ns);
					return NULL;
				} else {
					memcpy(testing_ptr, ns, strindex);
					free_safe(ns);
					ns = testing_ptr;
				}
			}

			snprintf(&ns[strindex], 4, "%%%02X", in);

			strindex += 3;
		}
		string++;
	}
	ns[strindex] = 0;	/* terminate it */
	return ns;
}

char *
url_unescape(const char *string, int length)
{
	char *str = NULL;
	size_t inputlen = length;
	size_t outputlen;
	int res = urldecode(string, inputlen, &str, &outputlen);

	if (res != 0)
		return NULL;
	return str;
}

int
url_escape_file(const char *srcfile, const char *dstfile)
{
	char buff[4];
	int srclen, dstlen, fd, i;
	char *data;

	if ((srclen = util_readfile(srcfile, &data)) < 0) {
		return -1;
	}
	if ((fd = open(dstfile, O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
		free_safe(data);
		return -2;
	}
	for (dstlen = 0, i = 0; i < srclen; i++) {
		if (isunreserved(data[i]) == 0) {
			write(fd, &data[i], 1);
			dstlen++;
		} else {
			snprintf(buff, 4, "%%%02X", data[i]);
			write(fd, buff, 3);
			dstlen += 3;
		}
	}

	free_safe(data);
	close(fd);
	return dstlen;
}

int
url_unescape_file(const char *srcfile, const char *dstfile)
{
	int srclen, dstlen, fd, i;
	char *data, hexstr[3], *ptr;
	unsigned char value;
	unsigned long hex_value;

	if ((srclen = util_readfile(srcfile, &data)) < 0) {
		return -1;
	}
	if ((fd = open(dstfile, O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
		free_safe(data);
		return -2;
	}

	for (dstlen = 0, i = 0; i < srclen; i++) {
		value = data[i];
		if ('%' == data[i] && i + 2 < srclen && ISXDIGIT(data[i + 1]) && ISXDIGIT(data[i + 2])) {
			/* this is two hexadecimal digits following a '%' */
			hexstr[0] = data[i + 1];
			hexstr[1] = data[i + 2];
			hexstr[2] = 0;

			hex_value = strtoul(hexstr, &ptr, 16);

			assert(hex_value <= (unsigned long)0xFF);
			value = (unsigned char)(hex_value & (unsigned long)0xFF);

			i += 2;
		}
		write(fd, &value, 1);
		dstlen++;
	}

	free_safe(data);
	close(fd);
	return dstlen;
}

