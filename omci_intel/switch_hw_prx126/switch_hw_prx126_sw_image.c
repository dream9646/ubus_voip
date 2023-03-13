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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_sw_image.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

// @sdk/include


#include "util.h"
#include "regexp.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "util_run.h"
#include "switch.h"
#include "vacl.h"
#include "env.h"
#include "mcast.h"

#include "intel_px126_util.h"
#include "intel_px126_sw_image.h"


// preassign filter ///////////////////////////////////////////////////////////////////////////



int switch_hw_prx126_sw_image_version_get(unsigned char id, unsigned char version_size,  char *version)
{

	return intel_omci_sw_image_version_get(id, version_size, version);
}


int switch_hw_prx126_sw_image_valid_get(unsigned char id, unsigned char *valid)
{

	return intel_omci_sw_image_valid_get(id, valid);
}


int switch_hw_prx126_sw_image_active_get(unsigned char id, unsigned char *active)
{

	return intel_omci_sw_image_active_get(id, active);
}

int switch_hw_prx126_sw_image_active_set(unsigned char id, unsigned int timeout)
{

	return intel_omci_sw_image_active_set(id, timeout);
}


int switch_hw_prx126_sw_image_commit_get(unsigned char id, unsigned char *commit)
{

	return intel_omci_sw_image_commit_get(id, commit);
}


int switch_hw_prx126_sw_image_commit_set(unsigned char id)
{

	return intel_omci_sw_image_commit_set(id);
}


int switch_hw_prx126_sw_image_download_start(unsigned char id, unsigned int size)
{

	return intel_omci_sw_image_download_start(id, size);
}


int switch_hw_prx126_sw_image_download_stop(unsigned char id)
{

	return intel_omci_sw_image_download_stop(id);
}


int switch_hw_prx126_sw_image_download_end(unsigned char id, unsigned int size, unsigned int crc, unsigned char filepath_size, char *filepath)
{

	return intel_omci_sw_image_download_end(id, size, crc, filepath_size, filepath);
}


int switch_hw_prx126_sw_image_download_handle_window(unsigned char id, unsigned short int window_id, unsigned char *data, unsigned int size)
{

	return intel_omci_sw_image_download_handle_window(id, window_id, data, size);
}


int switch_hw_prx126_sw_image_download_store(unsigned char id, unsigned char filepath_size, char *filepath)
{

	return intel_omci_sw_image_download_store(id, filepath_size, filepath);
}


