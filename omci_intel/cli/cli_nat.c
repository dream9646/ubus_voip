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
 * Module  : cli
 * File    : cli_nat.c
 *
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "switch.h"
#include "cli.h"

static void
cli_nat_show_hw(int fd)
{
	switch_hw_g.nat_session_hw_print(fd);

	return;
}

static void
cli_nat_show_sw(int fd)
{
	switch_hw_g.nat_session_sw_print(fd);

	return;
}

static void
cli_nat_show_all(int fd)
{
	cli_nat_show_hw(fd);
	util_fdprintf(fd, "\n");
	cli_nat_show_sw(fd);
	return;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_nat_help(int fd)
{
	util_fdprintf(fd, "nat help|[subcmd...]\n");
}

void
cli_nat_help_long(int fd)
{
	util_fdprintf(fd, "nat show [hw|sw]\n");
}

int
cli_nat_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "nat") == 0) {
		if (argc == 1 || (argc == 2 && strcmp(argv[1], "help") == 0)) {
			cli_nat_help_long(fd);
			return 0;
		} else if (argc == 2 && strcmp(argv[1], "show") == 0) {
			cli_nat_show_all(fd);	
			return 0;
		} else if (argc == 3 && strcmp(argv[1], "show") == 0) {
			if (strcmp(argv[2], "hw") == 0) {
				cli_nat_show_hw(fd);
				return 0;
			} else if (strcmp(argv[2], "sw") == 0) {
				cli_nat_show_sw(fd);
				return 0;
			}
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

