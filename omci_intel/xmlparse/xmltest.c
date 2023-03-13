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
 * File    : xmltest.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <syslog.h>

#include "meinfo.h"
#include "er_group.h"
#include "xmlparse.h"
#include "xmlomci.h"
#include "util.h"
#include "env.h"

int
main(int argc, char **argv)
{
	char filename[512];
	int subsection = 0;

	if (argc != 2) {
		fprintf(stderr, "syntax: %s subsecion\n", argv[0]);
		exit(1);
	}

	subsection = atoi(argv[1]);
	if (subsection > 99 || subsection < 1) {
		fprintf(stderr, "subsecion should between 1..99\n");
		exit(1);
	}

	env_set_to_default();
	omci_env_g.debug_level = LOG_DEBUG;
	
	fprintf(stderr, ">>>>>>>>>>>>> init\n");
	meinfo_preinit();
#if 0
	sprintf(filename, "../etc/omci/spec/spec_9_%02d.xml", subsection);
	fprintf(stderr, ">>>>>>>>>>>>> load %s\n", filename);
	xmlomci_spec_init(filename);

	sprintf(filename, "../etc/omci/config/config_9_%02d.xml", subsection);
	fprintf(stderr, ">>>>>>>>>>>>> load %s\n", filename);
	return xmlomci_config_init(filename);
#endif

	er_me_group_preinit();
	sprintf(filename, "../etc/omci/er_group/er_group.xml");
	fprintf(stderr, ">>>>>>>>>>>>> load %s\n", filename);
	return xmlomci_er_group_init(filename);

}
