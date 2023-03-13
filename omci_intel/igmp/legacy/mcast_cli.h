/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp cli module
 * File    : mcast_cli.h
 *
 ******************************************************************/


#ifndef __IGMP_CLI_H__
#define __IGMP_CLI_H__

int igmp_cli_cmdline(int fd, int argc, char *argv[]);

#endif