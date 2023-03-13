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
 * File    : switch_hw_prx126_conv.c
 *
 ******************************************************************/

// functions NOT exeported to others module
// they are used by switch_hw_prx126_*.c only

#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "switch.h"
#include "switch_hw_prx126.h"

/* fvt25xx logical-physical port map

                         2510/2511/2516   2511B            2512C	    2518+rtk_wifi    2518+quantenna
portname     logical_id  physical/ext/rg  physical/ext/rg  physical/ext/rg  physical/ext/rg  physical/ext/rg
UNI0               0         0    -1   0      0    -1   0      0    -1   0      0    -1   0      0    -1   0
UNI1               1         1    -1   1     -1    -1  -1      1    -1   1      1    -1   1      1    -1   1
UNI2               2         2    -1   2     -1    -1  -1     -1    -1  -1      2    -1   2      2    -1   2
UNI3               3         3    -1   3     -1    -1  -1     -1    -1  -1      3    -1   3      3    -1   3
UNI4(RGMII)        4         5    -1   5     -1    -1  -1     -1    -1  -1      8    -1   8      8    -1   8
WAN(PON)           5         4    -1   4      1    -1   1      2    -1   2      5    -1   5      5    -1   5
CPU                6         6     0   6      2     0   2      3     0   3      9     0   9      9     0   9

Wifi0              7         6     1   7     -1    -1  -1      3     1   4      9     1  11      6    -1   6
Wifi1              8         6     2   8     -1    -1  -1     -1    -1  -1     10     1  17      7    -1   7
CPU1               9        -1    -1  -1     -1    -1  -1     -1    -1  -1     10     0  10     10     0  10
CPU2(not used)    10        -1    -1  -1     -1    -1  -1     -1    -1  -1      7     0   7     -1    -1  -1

Wifi0.Vap0        11        -1    -1  -1     -1    -1  -1     -1    -1  -1      9     2  12      6    -1   6
Wifi0.Vap1        12        -1    -1  -1     -1    -1  -1     -1    -1  -1      9     3  13      6    -1   6
Wifi0.Vap2        13        -1    -1  -1     -1    -1  -1     -1    -1  -1      9     4  14      6    -1   6
Wifi0.Vap3        14        -1    -1  -1     -1    -1  -1     -1    -1  -1      9     5  15      6    -1   6

Wifi1.Vap0        15        -1    -1  -1     -1    -1  -1     -1    -1  -1     10     2  18      7    -1   7
Wifi1.Vap1        16        -1    -1  -1     -1    -1  -1     -1    -1  -1     10     3  19      7    -1   7
Wifi1.Vap2        17        -1    -1  -1     -1    -1  -1     -1    -1  -1     10     4  20      7    -1   7
Wifi1.Vap3        18        -1    -1  -1     -1    -1  -1     -1    -1  -1     10     5  21      7    -1   7

note:
physical -1 means the port is invalid
ext      -1 means the port is not an cpuext port (but the port could be valid if its physical is not -1)
rg       -1 means the port is invalid

note2: (using 2510 as example)
when convert between logical id and phy/ext id,
the logical id 6 is always mapped to (phyid=6, extid=0) only
ps: phyid 6 is a common gate of ext id 0..5, if any one ext id 0..5 is set, the phy id 6 should be set.
*/ 

static int logical_to_physical_map[19] = 
	{ 0,1,2,3,8,5,9,	9,10,10,7,	9,9,9,9,	10,10,10,10};
static int logical_to_ext_map[19] = 
	{ -1,-1,-1,-1,-1,-1,0,	1,1,0,0,	2,3,4,5,	2,3,4,5};
static int logical_to_rg_map[19] = 
	{ 0,1,2,3,8,5,9,  	11,17,10,7,  	12,13,14,15,	18,19,20,21};

// we indicate port conv error by returning -1
// and try to not putting -1 in the returned (logical|phy|ext)_portid
// in case caller uses the returned portid without checking returned value, segfault wont happen

// sdk specific ////////////////////////////////////////////////////////////////////////////

int
switch_hw_prx126_conv_portid_logical_to_private(unsigned int logical_portid, unsigned int *phy_portid, unsigned int *ext_portid)
{
	*phy_portid = *ext_portid = 0;
	if (logical_portid >= 0 && logical_portid < SWITCH_LOGICAL_PORT_TOTAL && logical_to_physical_map[logical_portid] >=0) {
		*phy_portid = logical_to_physical_map[logical_portid];
		if (logical_to_ext_map[logical_portid] >=0)
			*ext_portid = logical_to_ext_map[logical_portid];
		return 0;
	}
	dbprintf(LOG_WARNING, "logical portid %d err\n", logical_portid);
	return -1;
}

int
switch_hw_prx126_conv_portid_private_to_logical(unsigned int *logical_portid, unsigned int phy_portid, unsigned int ext_portid)
{
	int i;
	*logical_portid = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_physical_map[i] >= 0) {
			if (logical_to_physical_map[i] != phy_portid)
				continue;
			if (logical_to_ext_map[i]>=0) {	// check ext_portid only if it >=0
				if (logical_to_ext_map[i] != ext_portid)
					continue;
			}
			*logical_portid = i;
		  	return 0;
		}
	}
	dbprintf(LOG_ERR, "phy/ext portid %d/%d err\n", phy_portid, ext_portid);
	return -1;	    	
}
	
int
switch_hw_prx126_conv_portmask_logical_to_private(unsigned int logical_portmask, unsigned int *phy_portmask, unsigned int *ext_portmask)
{
	int i;
	*phy_portmask = *ext_portmask = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_physical_map[i] >= 0) {
			if (((1<<i) & logical_portmask) == 0)
				continue;
			*phy_portmask |= (1<<logical_to_physical_map[i]);
			if (logical_to_ext_map[i] >=0)
				*ext_portmask |= (1<<logical_to_ext_map[i]);
		}
	}
	return 0;
}
	
int
switch_hw_prx126_conv_portmask_private_to_logical(unsigned int *logical_portmask, unsigned int phy_portmask, unsigned int ext_portmask)
{
	int i;
	*logical_portmask = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_physical_map[i] >= 0) {
			if (((1<<logical_to_physical_map[i]) & phy_portmask) == 0)
				continue;
			if (logical_to_ext_map[i] >= 0) {
				if (((1<<logical_to_ext_map[i]) & ext_portmask) == 0)
					continue;
			}
			*logical_portmask |= (1<<i);
		}
	}
	return 0;
}

// rg specific ////////////////////////////////////////////////////////////////////////////

int
switch_hw_prx126_conv_portid_logical_to_rg(unsigned int logical_portid, unsigned int *rg_portid)
{
	*rg_portid = 0;
	if (logical_portid >= 0 && logical_portid < SWITCH_LOGICAL_PORT_TOTAL && logical_to_rg_map[logical_portid] >=0) {
		*rg_portid = logical_to_rg_map[logical_portid];
		return 0;
	}
	dbprintf(LOG_ERR, "logical portid %d err\n", logical_portid);
	return -1;
}

int
switch_hw_prx126_conv_portid_rg_to_logical(unsigned int *logical_portid, unsigned int rg_portid)
{
	int i;
	*logical_portid = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_rg_map[i] >= 0) {
			if (logical_to_rg_map[i] != rg_portid)
				continue;
		   	*logical_portid = i;
		    	return 0;
		}
	}
	dbprintf(LOG_ERR, "rg portid %d err\n", rg_portid);
	return -1;	    	
}
	
int
switch_hw_prx126_conv_portmask_logical_to_rg(unsigned int logical_portmask, unsigned int *rg_portmask)
{
	int i;
	*rg_portmask = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_rg_map[i] >= 0) {
			if (((1<<i) & logical_portmask) == 0)
				continue;
			*rg_portmask |= (1<<logical_to_rg_map[i]);
		}
	}
	return 0;
}
	
int
switch_hw_prx126_conv_portmask_rg_to_logical(unsigned int *logical_portmask, unsigned int rg_portmask)
{
	int i;
	*logical_portmask = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (logical_to_rg_map[i] >=0) {
			if (((1<<logical_to_rg_map[i]) & rg_portmask) == 0)
				continue;
			*logical_portmask |= (1<<i);
		}
	}
	return 0;
}

// err str ////////////////////////////////////////////////////////////////////////////

char *
switch_hw_prx126_error_string(int error)
{
	static char error_string[16];
	
	sprintf(error_string, "TBD\n");

	return error_string;
}
