#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "me_related.h"
#include "cpuport.h"
#include "notify_chain.h"
#include "switch.h"
#include "uid.h"
#include "bridge.h"

//return number of ports or -1(error)
int
rstp_port_map_init ( struct cpuport_desc_t stp_port[] )
{
	struct meinfo_t *port_miptr;
	struct me_t *port_me, *pptp_uni_me;
	int i = 0;
	struct switch_port_info_t port_info;
	

	for ( i = 0 ; i < MAX_STP_PORTS ; i++ ) { 
		stp_port[i].logical_port_id = 0xff;
		stp_port[i].bridge_port_me = NULL;
	}

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) 
	{
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	i = 0;
	// traverse all bridge port me
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
		if ((pptp_uni_me = meinfo_me_get_by_meid(11, port_me->meid)) == NULL )
			continue;

		switch_get_port_info_by_me(port_me, &port_info);

		stp_port[i].logical_port_id = port_info.logical_port_id;
		stp_port[i].bridge_port_me = port_me;
		i++;
	}
	return i;
}

int
rstp_port_map_deinit ( struct cpuport_desc_t stp_port[] )
{
	int i = 0;

	for ( i = 0 ; i < MAX_STP_PORTS ; i++ ) {
		stp_port[i].logical_port_id = 0xff;
		stp_port[i].bridge_port_me = NULL;
	}

	return 0;
}

int
rstp_port_idx_get ( struct cpuport_desc_t desc, struct cpuport_desc_t stp_port[] )
{
	unsigned char i = 0;
	for ( i = 0; i < MAX_STP_PORTS ; i++ ){
		if( stp_port[i].bridge_port_me == desc.bridge_port_me )
			return i;	
	}
	return -1;
}
