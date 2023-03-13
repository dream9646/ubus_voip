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
 * Module  : proprietary_fiberhome
 * File    : meinfo_hw_65346.c
 *
 ******************************************************************/

#include <time.h>
#include <string.h>
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "switch.h"
#include "hwresource.h"

//classid 65346 9.99.25 Ethernet_performance_monitoring_history_data_2_private

static int      
meinfo_hw_65346_alarm(struct me_t *me_ptr, unsigned char alarm_mask[28])
{
	static unsigned int pre_outoctets[8], pre_inoctets[8], last_time[8];
	unsigned int speed_outoctets, speed_inoctets, current_time, diff_time;
	struct switch_eth_pm_data2_private_t pm_data2_private;
	unsigned char port;
	struct attr_value_t attr_value={0, NULL};
	int ret;

	port=me_ptr->instance_num;
	if ( port > 8 ) {
		dbprintf(LOG_ERR, "Port out of revert range\n");
	}

	if(last_time[port] == 0)
		last_time[port]=time(0)-omci_env_g.alarm_refresh_interval;

	current_time=time(0);

	diff_time=current_time-last_time[port];
	if( diff_time ==0 || last_time[port] > current_time) {
		diff_time=1;	//or omci_env_g.alarm_refresh_interval
		dbprintf(LOG_ERR, "inst=%d, duration error, current_time=%d, last_time=%d\n",me_ptr->instance_num, current_time, last_time[port]);
	}
	//dbprintf(LOG_ERR, "inst=%d, current_time=%d, last_time=%d\n",me_ptr->instance_num, current_time, last_time[port]);

	//UPSPEED;			//"IfInOctets/sec"
	//DOWNSPEED; 			//"IfOutOctets/sec"

	//Suppport for FH proprietary me, EVB use rtl8305
	if (switch_hw_g.eth_pm_data2_private == NULL)
		return 0;


	memset( &pm_data2_private, 0, sizeof(struct switch_eth_pm_data2_private_t) );
	if( (ret=switch_hw_g.eth_pm_data2_private(port, &pm_data2_private)) !=0 ) {
		dbprintf(LOG_ERR, "eth_pm_data2_private error, ret = %d\n", ret);
		return -1;
	}

	//downupstream speed
	speed_outoctets=(pm_data2_private.outoctets-pre_outoctets[port])/(diff_time);
	attr_value.data=speed_outoctets;
	meinfo_me_attr_set_pm_current_data(me_ptr, 12, attr_value);
	//dbprintf(LOG_ERR, "port=%d: downstream speed=%d(Bytes/sec)\n", port, (pm_data2_private.outoctets-pre_outoctets[port])/(diff_time));

	//upstream speed
	speed_inoctets=(pm_data2_private.inoctets-pre_inoctets[port])/(diff_time);
	meinfo_me_attr_update_pm(me_ptr, 11, speed_inoctets);

	attr_value.data=speed_inoctets;
	meinfo_me_attr_set_pm_current_data(me_ptr, 11, attr_value);
	//dbprintf(LOG_ERR, "port=%d: upstream speed=%d(Bytes/sec)\n", port, (pm_data2_private.inoctets-pre_inoctets[port])/(diff_time));

	pre_outoctets[port]=pm_data2_private.outoctets;
	pre_inoctets[port]=pm_data2_private.inoctets;
	last_time[port]=current_time;
	return 0;       
}                  

static int 
meinfo_hw_65346_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i,ret;
	struct switch_eth_pm_data2_private_t pm_data2_private;

	//Only for FH proprietary me, use rtl8305 4 port extsw
	memset( &pm_data2_private, 0, sizeof(struct switch_eth_pm_data2_private_t) );
	if( (ret=switch_hw_g.eth_pm_data2_private(me->instance_num, &pm_data2_private)) !=0 ) {
		dbprintf(LOG_ERR, "eth_pm_data2_private error, ret = %d\n", ret);
		return -1;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: //interval end time
			//case 2: //threshold data 1/2 id
			//case 3: //pppoefilteredframecounter
			case 4:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.outoctets);
				break;
			case 5:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.outbroadcastpackets);
				break;
			case 6:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.outmulticastpackets);
				break;
			case 7:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.outunicastpackets);
				break;
			case 8:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.inunicastpackets);
				break;
			case 13:
				meinfo_me_attr_update_pm(me, i, pm_data2_private.unipackets1519octets);
				break;
			default:
				continue;//do nothing
			}
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65346= {
	.alarm_hw	= meinfo_hw_65346_alarm,
	.get_hw		= meinfo_hw_65346_get,
};

