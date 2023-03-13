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
 * Module  : gpon_hw_prx126
 * File    : gpon_hw_prx126_util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// @sdk/system/include/


#include "util.h"
#include "util_run.h"
#include "pingtest.h"
#include "list.h"
#include "meinfo.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"
#include "switch.h"
#include "batchtab.h"
#include "batchtab_cb.h"

// protect gpon mac from traffic when doing tm hw config //////////////////////////////////////////////////////
void
gpon_hw_prx126_util_protect_start(unsigned int *state)
{

}

void
gpon_hw_prx126_util_protect_end(unsigned int state)
{

}

// clear pq related //////////////////////////////////////////////////////////////////////////

int
gpon_hw_prx126_util_clearpq_dummy(void)
{

	return 0;
}

// drain-out pq 0..127 whose page usage > page_threshold
void
gpon_hw_prx126_util_clearpq(unsigned int page_threshold)
{

}

// reset_dbru related //////////////////////////////////////////////////////////////////////////
void
gpon_hw_prx126_util_reset_dbru(void)
{

}

// this would have gpon mac back to O1->O2....., almost equivlant to re-plugin fiber
void
gpon_hw_prx126_util_reset_serdes(int msec)
{

}

// recovery by deactivate & activate //////////////////////////////////////////////////////////////////////////
void
gpon_hw_prx126_util_recovery(void)
{

}

// omccmiss related ///////////////////////////////////////////////////////////////////////////////////////////////
// ret: remaining time of omccmiss situation
int
gpon_hw_prx126_util_omccmiss_detect(void)
{

	return 0;
}

// tcontmiss related //////////////////////////////////////////////////////////////////////////////////////////////////
// ret: remaining time of tcontmiss situation
int
gpon_hw_prx126_util_tcontmiss_detect(void)
{

	return 0;
}

// o2stuck related //////////////////////////////////////////////////////////////////////////////////////////////////

// ret: remaining time of o2stuck situation
int
gpon_hw_prx126_util_o2stuck_detect(void)
{

	return 0;
}

// tcont idlw bw over requested check /////////////////////////////////////////////////////////////////////////////////

// when ont upstream traffic < usrate,
// check if any tcont idlegem bw >  CIR sum of all gemflow within the tcont + overbw
// the check is issued per interval sec and if overalloc continues for repeat times,
// return 1 (overalloc) to caller
int
gpon_hw_prx126_util_idlebw_is_overalloc(unsigned short idlebw_guard_parm[])
{
	return 0;
}
