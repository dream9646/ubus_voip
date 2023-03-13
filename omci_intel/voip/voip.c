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
 * Module  : voip
 * File    : voip.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>

#include "util.h"

#define VOIP_SIGNATURE_STRING "/var/run/voip.pid"

static enum env_voip_enable_t
voip_get_voip_enable_by_voipsig(void)
{
	int result = ENV_VOIP_ENABLE;
	FILE *fptr;
	
	// voipsig
	if ((fptr = fopen(VOIP_SIGNATURE_STRING, "r")) == NULL)
	{
		dbprintf_voip(LOG_ERR, "Can't open voip signature file: %s\n", VOIP_SIGNATURE_STRING);
		result = ENV_VOIP_DISABLE;
		return result;
	}

	fclose(fptr);

	return result;
}

void
voip_update_voip_enable_auto(void)
{
	if (omci_env_g.voip_enable == ENV_VOIP_AUTO) //auto detect
	{
		omci_env_g.voip_enable = voip_get_voip_enable_by_voipsig();

		dbprintf_voip(LOG_ERR, "voip enable=%d\n", 
			omci_env_g.voip_enable);
	}
}

