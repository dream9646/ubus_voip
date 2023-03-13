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
 * Purpose : 5VT OMCI protocol stack
 * Module  : client_prog
 * File    : diag.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

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
static char *
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

static int
conv_str2array(char *s, char *delim, char *array[], int array_size)
{
	int dlen = strlen(delim);
	int i = 0;
	char *p;

	while (s != NULL) {
		p = strstr(s, delim);
		if (p != NULL) {
			*p = 0x0;
			array[i] = util_trim(s);
			if (strlen(array[i])>0)
				i++;
			s = p + dlen;
			if (i == array_size)
				break;
		} else {
			break;
		}
	}
	if (s != NULL && i < array_size) {
		array[i] = util_trim(s);
		if (strlen(array[i])>0)
			i++;
	}

	return i;
}

#define DIAG_PROMPT_LEN	6	// "diag> "
// wrapper to /usr/bin/diag
static int
diag(char *input_line, char *output_file)
{
	static char diag_cmd_invalid_char[] = { '|', '<', '>', '(', ')', '{', '}', '`', ';', 0};
	static char buff[1024];
	int i;

	// do nothing for comment
	if (strlen(input_line)== 0 || input_line[0] == '#')
		return -1;

	// check if 1st token is cmd for diag, to eliminate the unnecessary execution /usr/bin/diag
	{
		static char *
		diag_cmd_list[]= {
			"acl", "auto-fallback", "bandwidth", "classf", "cpu",
			"debug", "dot1x", "epon", "field-selector", "flowctrl",
			"gpon", "i2c", "igmp", "interrupt", "iol",
			"l2-table", "l34", "led", "meter", "mib", "mirror",
			"oam", "pbo", "pon", "port", "qos",
			"register", "rg", "rldp", "rlpp", "rma",
			"sdk", "security", "storm-control", "stp", "svlan", "switch",
			"time", "trap", "trunk", "vlan", "extoam",
			NULL
		};

		char *token, *end;
		strncpy(buff, input_line, 127); buff[127] = 0;
		if ((end=strchr(buff, ' ')) != NULL)
			*end = 0;
		token = util_trim(buff);

		if (strchr(token, '?') == NULL) {
			for (i = 0; diag_cmd_list[i]!=NULL; i++) {
				if (strcmp(token, diag_cmd_list[i]) == 0)
					break;
			}
			if (diag_cmd_list[i] == NULL)
				return -1;
		}
	}

	// check if command contains invalid chars
	for (i=0; diag_cmd_invalid_char[i] != 0; i++) {
		if (strchr(input_line, diag_cmd_invalid_char[i])!= NULL) {
			fprintf(stdout, "invalid char '%c' in command\n", diag_cmd_invalid_char[i]);
			return -1;
		}
	}

	// exec cmd
	snprintf(buff, 1024, "/usr/bin/diag %s > %s 2>/dev/null", input_line, output_file);
	system(buff);

	return 0;
}

#if 0
// since our prompt length is 21, diag prompt length is 6
// append 15 byte at the position dependent output of diag,
#define PREFIX_SPACE_STR	"               "
int
diag_print(char *input_line)
{
	char *input_line2 = util_trim(input_line);
	char output_file[64];
	static char buff[1024];
	FILE *fp;
	int line_num = 0;

	snprintf(output_file, 64, "/tmp/tmpfile.%d.%06ld", getpid(), random()%1000000);

	if (diag(input_line2, output_file) <0) {
		unlink(output_file);
		return -1;
	}

	if ((fp = fopen(output_file, "r")) == NULL) {
		fprintf(stdout, "diag err?\n");
		unlink(output_file);
		return -1;
	}

	// if this is a request for help, then show nothing if syntax error detected in diag
	if (input_line2[strlen(input_line2)-1] == '?') {
		while ( (fgets(buff, 1024, fp)) != NULL ) {
			if (strstr(buff, "^Parse error")) {
				fclose(fp);
				unlink(output_file);
				return 0;
			}
		}
		rewind(fp);
	}

	while ( (fgets(buff, 1024, fp)) != NULL ) {
		line_num++;

		// skip dummy lines
		if (line_num == 1)	// skip 1st line (which is the cmd itself)
			continue;
		if (line_num == 2) {	// break at 2nd line if there is position dependent error prompt
			if (strstr(buff, "^Parse error") || strstr(buff, "^Incomplete command")) {
			 	fprintf(stdout, "%s%s", PREFIX_SPACE_STR, buff);
				break;
			}
		}
		// skip last line (which is prompt)
		if (strstr(buff, "> command:"))
			continue;
		// skip line with prompt, which is supposed to be the input_line2
		if (strlen(buff)>=DIAG_PROMPT_LEN && strncmp(buff, "diag> ", DIAG_PROMPT_LEN) ==0)
			continue;
		// ignore incomplete warning & following lines
		if (strstr(buff, "^Incomplete command"))
			break;
		// show parse error with input_line2, skip following lines
		if (strstr(buff, "^Parse error") && strlen(buff)>=DIAG_PROMPT_LEN) {
			int len = strlen(input_line2);
			// print input line
			if (input_line2[len-1] == '?')
				input_line2[len-1] = 0;
			fprintf(stdout, "%s\n", input_line2);
			// skip space chars
			fprintf(stdout, "%s", buff+DIAG_PROMPT_LEN);
			break;
		}

		// print output asis
		fprintf(stdout, "%s", buff);
	}
	fclose(fp);
	unlink(output_file);
	return 0;
}
#endif

int
diag_get_matchlist(char *input_line, int with_help, char ***av)
{
	static char buff[1024];
	char output_file[64];
	char *token, *s;
	FILE *fp;
	int line_num = 0;
	int is_input_wrong = 0;

	int token_len, match = 0;
	char **copy;

	{	// get the last token in input line
		static char buff2[1024];
		char *tokens[64];
		int token_total;
		int end_is_space = (strlen(input_line)>0 && isspace(input_line[strlen(input_line)-1]));

		strncpy(buff2, input_line, 1023);
		token_total = conv_str2array(buff2, " ", tokens, 64);
		if (token_total == 0 || (end_is_space && token_total<63)) {
			tokens[token_total] = "";
			token_total++;
		}
		token = tokens[token_total-1];
	}

	snprintf(output_file, 64, "/tmp/tmpfile.%d.%06ld", getpid(), random()%1000000);
	snprintf(buff, 1023, "%s?", input_line);
//fprintf(stderr, "\ncmd='%s', token='%s'\n", buff, token);

	if (diag(buff, output_file) <0) {
		unlink(output_file);
		return -1;
	}

	if ((fp = fopen(output_file, "r")) == NULL) {
		fprintf(stdout, "diag err?\n");
		unlink(output_file);
		return -1;
	}

	copy = (char **) malloc (128 * sizeof(char *));	// assume no more than 128 match in the list
	token_len = strlen(token);
	while ( (fgets(buff, 1024, fp)) != NULL ) {
		line_num++;

		// skip dummy lines
		if (line_num == 1)	// skip 1st line (which is the cmd itself)
			continue;
		if (line_num == 2) {	// break at 2nd line if there is position dependent error prompt
			if (strstr(buff, "^Parse error") || strstr(buff, "^Incomplete command"))
				break;
		}
		// skip last line (which is prompt)
		if (strstr(buff, "> command:"))
			continue;
		// skip line with prompt, which is supposed to be the input_line2
		if (strlen(buff)>=DIAG_PROMPT_LEN && strncmp(buff, "diag> ", DIAG_PROMPT_LEN) ==0)
			continue;
		// ignore incomplete warning & following lines
		if (strstr(buff, "^Incomplete command"))
			break;
		// if parse error happens within the input line, assume user input is wrong
		if ((s=strstr(buff, "^Parse error")) != NULL && strlen(buff)>=DIAG_PROMPT_LEN) {
		        if (s - buff - DIAG_PROMPT_LEN < strlen(input_line))
        			is_input_wrong = 1;
			break;
		}

		// parse output asis
		if (!isalpha(buff[0]) && !isdigit(buff[0]) && buff[0] != '<')
			continue;
		if (token_len == 0 || strncmp(buff, token, token_len) ==0) {
			char *delim = strstr(buff, " - ");
			char *keyword, *help;
//fprintf(stderr, "match %d: %s\n", match, buff);
			if (!delim)
				continue;
			*delim = 0;
			help = util_trim(delim+3);
			keyword = util_trim(buff);
			if (strlen(keyword) == 0 || strcmp(keyword, "Error")==0)
				continue;
			if (with_help) {
				static char buff2[1024];
				snprintf(buff2, 1023, "%-36s - %s", keyword, help);
				if (match <127) {
					copy[match] = strdup (buff2);
					match++;
				}
			} else {
				if (match <127) {
					copy[match] = strdup (keyword);
					match++;
				}
			}
		}
	}
	fclose(fp);
	unlink(output_file);

	// return null matchlist in case any parse error happens
	if (is_input_wrong) {
		int i;
		for (i=0; i< match; i++)
			free(copy[i]);
		free(copy);
		return 0;
	}

	if (match == 0) {
		copy[match] = strdup("[enter]");
		match++;
	}
	*av = copy;
	return match;
}

