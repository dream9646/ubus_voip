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
 * File    : cli_version.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "switch.h"
#include "revision.h"
#ifdef OMCI_BM_ENABLE
#include "omci_bm.h"
#endif

// define global info for used with core debugging
char *version_str_tag=TAG_REVISION;
char *version_str_time=__TIME__;
char *version_str_date=__DATE__;

#define CPUID_ADDR	0x1b010008
static unsigned int 
get_cpuid(void)
{
	unsigned int cpuid = 0xb0000000;
	int fd;
	
	if ((fd=open("/dev/fvmem", O_RDWR)) <0)
		return cpuid;
	lseek(fd, CPUID_ADDR, SEEK_SET);
	write(fd, &cpuid, 4);
	lseek(fd, CPUID_ADDR, SEEK_SET);
	read(fd, &cpuid, 4);
	close(fd);
	
	return cpuid;
}

#ifndef X86
// as the omci_cli_version_str() may be called earlier than switch_hw_fvt2510*() is registered to switch_hw_g,
// so we call the switch_hw_fvt2510_hw_version_get() instead of switch_hw_g.hw_version_get()
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
int switch_hw_prx126_hw_version_get(unsigned int *family, unsigned int *version, unsigned int *rev, unsigned int *subtype);
#endif
#endif

char *
omci_cli_version_str(void)
{
	static char version_str[128];
	unsigned int family = 0, version = 0, rev = 0, subtype = 0;
	unsigned char __attribute__((unused)) mac[6] = {0};
#ifndef X86
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	switch_hw_prx126_hw_version_get(&family, &version, &rev, &subtype);
#endif
#endif
#ifdef OMCI_BM_ENABLE
	omci_bm_getmac(mac);
	snprintf(version_str, 128, "OMCI v1.00 for %s (tag %s, %04x%02x%02x%02x, %08x, %02x%02x%02x%02x%02x%02x, %s, %s)", 
			CHIP_NAME, TAG_REVISION, family, version, rev, subtype, get_cpuid(), 
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
			__TIME__, __DATE__);
#else
	snprintf(version_str, 128, "OMCI v1.00 for %s (tag %s, %04x%02x%02x%02x, %08x, %s, %s)", 
			CHIP_NAME, TAG_REVISION, family, version, rev, subtype, get_cpuid(), 
			__TIME__, __DATE__);
#endif
	return version_str;
}
