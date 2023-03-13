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
 * Module  : xmlparse
 * File    : xmlparse.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>

#include "simplexml.h"
#include "xmlparse.h"
#include "util.h"

static struct xmlparse_env_t *xmlparse_env = NULL;	// static var used in xmlparse_handler

/**
 * If the string is less than MAXSIZE characters it is
 * simply copied, otherwise the first MAXSIZE-4 characters
 * are copied and an elipsis (...) is appended.
 */
static void
trim(const char *szInput, char *szOutput, int maxsize)
{
	int i = 0;
	while (szInput[i] != 0 && i < maxsize) {
		if (szInput[i] < ' ') {
			szOutput[i] = ' ';
		} else {
			szOutput[i] = szInput[i];
		}
		i++;
	}
	if (i < maxsize) {
		szOutput[i] = '\0';
	} else {
		szOutput[maxsize-4] = '.';
		szOutput[maxsize-3] = '.';
		szOutput[maxsize-2] = '.';
		szOutput[maxsize-1] = '\0';
	}
}

static int
xmlparse_url_append_tag(char *url, int size_limit, char *tag)
{
	int len_url = strlen(url);
	int len_tag = strlen(tag);

	if (url[0] != '/')
		return -1;	// not start from /

	if (url[len_url - 1] == '/') {	// url already end with /, no / is added
		if (len_url + len_tag + 1 > size_limit)
			return -1;
	} else {		// add / at the url end
		if (len_url + 1 + len_tag + 1 > size_limit)
			return -1;
		url[len_url] = '/';
		len_url++;
		url[len_url] = 0;
	}

	strcpy(url + len_url, tag);	// add tag
	return 0;
}

static int
xmlparse_url_remove_tag(char *url, char **tag)
{
	char *s;
	int len;
	static char buff[MAXSIZE_TAGNAME];

	if (url[0] != '/')
		return -1;	// not start from /
	if ((len = strlen(url)) == 1)
		return -2;	// only root / char

	if (url[len - 1] == '/') {	// remove tail / char
		len--;
		url[len] = 0;
	}

	s = strrchr(url, '/');
	if (tag) {
		strncpy(buff, s + 1, MAXSIZE_TAGNAME-1);
		tag[MAXSIZE_TAGNAME-1] = 0;
		*tag = buff;
	}
	if (s != url)		// terminate at / if it is not the root /
		*s = 0;
	else
		*(s + 1) = 0;

	return 0;
}

static int
xmlparse_readfile(char *filename, char **filedata, long *filelen)
{
	int fd;
	struct stat fstat;

	*filedata = NULL;
	*filelen = 0;

	if (stat(filename, &fstat) == -1) {
		return -1;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		dbprintf_xml(LOG_ERR, "file %s open error\n", filename);
		return -2;
	}
	*filedata = malloc(fstat.st_size);
	*filelen = read(fd, *filedata, fstat.st_size);
	close(fd);

	if (*filelen != fstat.st_size) {
		free(*filedata);
		*filedata = NULL;
		*filelen = 0;
		dbprintf_xml(LOG_ERR, "file %s read error\n", filename);
		return -4;
	}
	return 0;
}

static void *
xmlparse_handler(SimpleXmlParser parser, SimpleXmlEvent event,
		 const char *tagname, const char *attr, const char *valstr)
{
	char tagname2[MAXSIZE_TAGNAME], attr2[MAXSIZE_ATTRNAME], valstr2[MAXSIZE_VALUE];

	if (tagname != NULL) {
		trim(tagname, tagname2, MAXSIZE_TAGNAME);
	}
	if (attr != NULL) {
		trim(attr, attr2, MAXSIZE_ATTRNAME);
	}
	if (valstr != NULL) {
		trim(valstr, valstr2, MAXSIZE_VALUE);
	}
	// copy current xml line number
	xmlparse_env->line = simpleXmlGetLineNumber(parser);

	if (event == ADD_SUBTAG) {
		xmlparse_url_append_tag(xmlparse_env->url, MAXSIZE_URL, tagname2);
		xmlparse_env->depth++;

		dbprintf_xml(LOG_DEBUG, "%6li: add subtag (%s)\n", xmlparse_env->line, tagname2);
		dbprintf_xml(LOG_DEBUG, "xmlurl> +%s\n", xmlparse_env->url);
		xmlparse_env->add_tag(xmlparse_env, tagname2);

	} else if (event == ADD_ATTRIBUTE) {
		dbprintf_xml(LOG_DEBUG, "%6li: add attribute to tag %s (%s=%s)\n",
			 xmlparse_env->line, tagname2, attr2, valstr2);
		xmlparse_env->add_attr(xmlparse_env, tagname2, attr2, valstr2);

	} else if (event == ADD_CONTENT) {
		dbprintf_xml(LOG_DEBUG, "%6li: add content to tag %s (%s)\n", xmlparse_env->line, tagname2, valstr2);
		xmlparse_env->add_content(xmlparse_env, tagname2, valstr2);

	} else if (event == FINISH_ATTRIBUTES) {
		dbprintf_xml(LOG_DEBUG, "%6li: finish attributes (%s)\n", xmlparse_env->line, tagname2);
		xmlparse_env->finish_attrs(xmlparse_env, tagname2);

	} else if (event == FINISH_TAG) {
		dbprintf_xml(LOG_DEBUG, "xmlurl> -%s\n", xmlparse_env->url);
		dbprintf_xml(LOG_DEBUG, "%6li: finish tag (%s)\n", xmlparse_env->line, tagname2);
		xmlparse_env->finish_tag(xmlparse_env, tagname2);
		xmlparse_url_remove_tag(xmlparse_env->url, NULL);
		xmlparse_env->depth--;
	}

	return xmlparse_handler;
}

// 2310 cpu check
#define CPUID_2310	0x4d473032
static inline int
get_cpuid(void)
{
	unsigned int cpuid=0;
	int fd;
	if((fd=open("/dev/fvmem", O_RDONLY)) >= 0) {
		lseek(fd, 0x1820007c, SEEK_SET);
		read(fd, &cpuid, 4);
		close(fd);
	}
	return cpuid;
}

int
xmlparse_parse(char *xml_filename, struct xmlparse_env_t *env)
{
	char *filedata=NULL;
	long filelen;
	SimpleXmlParser parser;
	int ret;
/*
#ifndef X86
	if (get_cpuid() != CPUID_2310)
		return 0;
#endif
*/
	// read xml file into mem
	ret = xmlparse_readfile(xml_filename, &filedata, &filelen);
	if (ret < 0) {
		dbprintf_xml(LOG_ERR, "%s read error %d\n", xml_filename, ret);
		return -1;
	}
	// create xml parser
	parser = simpleXmlCreateParser(filedata, filelen);
	if (parser == NULL) {
		dbprintf_xml(LOG_ERR, "create parser error\n");
		free(filedata);
		return -2;
	}
	// init env
	if (env == NULL) {
		dbprintf_xml(LOG_ERR, "init env error\n");
		free(filedata);
		return -3;
	} else {
		xmlparse_env = env;
		env->url[0] = '/';
		env->url[1] = 0;
		env->depth = 0;
	}

	// do parse
	if (simpleXmlParse(parser, xmlparse_handler) != 0) {
		dbprintf_xml(LOG_ERR, "parse error on %s line %li: %s\n",
			 xml_filename, simpleXmlGetLineNumber(parser), simpleXmlGetErrorDescription(parser));
		free(filedata);
		return -4;
	}

	if (filedata)
		free(filedata);
	return 0;
}
