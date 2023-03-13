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
 * Module  : intel_prx126
 * File    : intel_prx126_sw_image.c
 *
 ******************************************************************/
#ifndef __INTEL_PX126_SW_IMAGE_H__
#define __INTEL_PX126_SW_IMAGE_H__
#include <string.h>
#ifdef LINUX
#include <dlfcn.h>
#endif
#include "list.h"
#include "util_uci.h"
#include "pon_adapter.h"
#include "pon_adapter_system.h"
#include "pon_adapter_event_handlers.h"
#include "pon_adapter_config.h"

#include "fwk_mutex.h"

int intel_omci_sw_image_version_get(unsigned char id, unsigned char version_size, char *version);
int intel_omci_sw_image_valid_get(unsigned char id, unsigned char *valid);
int intel_omci_sw_image_active_get(unsigned char id, unsigned char *active);
int intel_omci_sw_image_active_set(unsigned char id, unsigned int timeout);
int intel_omci_sw_image_commit_get(unsigned char id, unsigned char *commit);
int intel_omci_sw_image_commit_set(unsigned char id);
int intel_omci_sw_image_download_start(unsigned char id, unsigned int size);
int intel_omci_sw_image_download_stop(unsigned char id);
int intel_omci_sw_image_download_end(unsigned char id, unsigned int size, unsigned int crc, unsigned char filepath_size, char *filepath);
int intel_omci_sw_image_download_handle_window(unsigned char id, unsigned short int window_id, unsigned char *data, unsigned int size);
int intel_omci_sw_image_download_store(unsigned char id, unsigned char filepath_size, char *filepath);

#endif
