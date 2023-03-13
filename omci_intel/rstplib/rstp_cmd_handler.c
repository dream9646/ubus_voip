#include <sys/time.h>
#include "meinfo.h"
#include "cpuport.h"
#include "util.h"

#include "rstp.h"
#include "rstp_cfg.h"
#include "rstp_port.h"
#include "rstp_cmd_handler.h"
#include "rstp_omci.h"
#include "uid.h"
#include "bridge.h"

int
rstp_cmd_me_add_handler( struct rstp_cmd_msg_t *rstp_cmd_msg)
{
	if( rstp_omci_stp_bridge_instance_check()<0 )
		return 0;
	//before starting STP service make sure the attribute 1 "spanning tree ind" of  me 45  to be ture 
	bridge_shutdown();
	bridge_start();

	if (rstp_omci_stp_bridge_service_enable() == 1) {
		rstp_cfg_stp_enable();
	}
	if (rstp_omci_stp_bridge_service_enable() == 0) {
		rstp_cfg_stp_disable();
	}

	return 0;
}

int
rstp_cmd_me_del_handler( struct rstp_cmd_msg_t *rstp_cmd_msg)
{
	if( rstp_omci_stp_bridge_instance_check()<0 )
		bridge_shutdown();
	else{
		bridge_shutdown();
		bridge_start();
		if (rstp_omci_stp_bridge_service_enable() == 1) {
			rstp_cfg_stp_enable();
		}
		if (rstp_omci_stp_bridge_service_enable() == 0) {
			rstp_cfg_stp_disable();
		}
	}
	return 0;
}

int
rstp_cmd_me_update_handler( struct rstp_cmd_msg_t *rstp_cmd_msg)
{
	struct me_t *port_me, *bridge_me;

	port_me = bridge_me = meinfo_me_get_by_meid( rstp_cmd_msg->classid, rstp_cmd_msg->meid );

	if(rstp_cmd_msg->classid == 45)
	{
		unsigned short bridge_priority;
		unsigned short max_age;
		unsigned short hello_time;
		unsigned short forward_delay;

		if ( rstp_omci_stp_bridge_priority_get( &bridge_priority ) == 0 )
			rstp_cfg_br_prio_set( bridge_priority ); 
		if ( rstp_omci_stp_bridge_max_age_get( &max_age ) == 0 )
			rstp_cfg_br_maxage_set( max_age ); 
		if ( rstp_omci_stp_bridge_hello_time_get( &hello_time ) == 0)
			rstp_cfg_br_hello_time_set( hello_time );
		if ( rstp_omci_stp_bridge_forward_delay_get( &forward_delay ))
			rstp_cfg_br_fdelay_set ( forward_delay );
		if ( rstp_omci_stp_bridge_service_enable() == 1)
			rstp_cfg_stp_enable();
		if ( rstp_omci_stp_bridge_service_enable() == 0)
			rstp_cfg_stp_disable();
	}

	if(rstp_cmd_msg->classid == 47)
	{
		struct cpuport_desc_t desc;
		int idx;
		unsigned short port_identifier;
		unsigned short path_cost;

		if (rstp_omci_port_me_to_desc( port_me, &desc ) <0 )
			return 0;
		if ((idx = rstp_port_idx_get(desc, stp_port)) <0)
			return 0;

		//get port identifier from omci and set the value to stp instance config
		if (rstp_omci_stp_port_identifier_get( desc, &port_identifier) <0)
			return 0;
		rstp_cfg_prt_prio_set (idx+1 ,port_identifier);


		//get port identifier from omci and set the value to stp instance config
		if (rstp_omci_stp_port_path_cost_get( desc, &path_cost)<0)
			return 0;
		rstp_cfg_prt_pcost_set (idx+1, path_cost);

	}

	return 0;
}
