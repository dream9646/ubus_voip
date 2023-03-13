#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "list.h"
#include "cpuport.h"
#include "meinfo.h"

#include "uid.h"
#include "bitmap.h"
#include "uid_stp.h"
#include "bridge.h"
#include "stp_in.h"
#include "rstp_omci.h"

int
rstp_info_bridge_mac_get( unsigned char mac[]) {
	int fd;
	struct ifreq ifr;
	int i = 0;
	int ret = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "br0", IFNAMSIZ-1);
	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);

	if (ret < 0 ) 
		return -1;

	for ( i = 0 ; i < 6 ; i++ )
	{
		*(mac+i) = ifr.ifr_hwaddr.sa_data[i];
	}
	return 0;
}
/*
int
rstp_info_bridge_priority_get( unsigned short *bridge_priority ) {

	if (rstp_omci_stp_bridge_priority_get( bridge_priority) < 0)
		return -1;
	return 0;
}
*/
int
rstp_info_designated_root_get( unsigned char designated_root[8] ) {

	UID_STP_STATE_T uid_state;

	int rc;
	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;

	memcpy(designated_root, &uid_state.designated_root, sizeof(UID_BRIDGE_ID_T));

	return 0;
}

int
rstp_info_root_path_cost_get( unsigned int *root_path_cost ) {
	UID_STP_STATE_T uid_state;

	int rc;
	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;

	memcpy(root_path_cost, &uid_state.root_path_cost, sizeof(unsigned long));

	return 0;
}

int
rstp_info_bridge_port_count_get( unsigned char *bridge_port_count ) {
	*bridge_port_count = number_of_ports;
	return 0;
}

int
rstp_info_root_port_num_get( unsigned short *root_port_num ) {
	UID_STP_STATE_T uid_state;

	int rc;
	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;

	*root_port_num = uid_state.root_port;
	return 0;
}

int
rstp_info_root_hello_time_get( unsigned short *hello_time ) {
	*hello_time = 0;
	return 0;
}

int
rstp_info_root_forward_delay_get( unsigned short *fdelay ) {
	*fdelay = 0;
	return 0;
}

int
rstp_info_port_path_cost_get( struct cpuport_desc_t desc, unsigned short *path_cost) {
	UID_STP_PORT_STATE_T uid_port;
	int rc;
	int port_index;
	for (port_index = 0; port_index <= number_of_ports; port_index++) {
		if( stp_port[port_index].bridge_port_me == desc.bridge_port_me )
			break;
	}

	if( port_index > number_of_ports )
		return -1;

	uid_port.port_no = port_index;
	rc = STP_IN_port_get_state (0, &uid_port);
	if (rc) 
		return -1;
	
	*path_cost = uid_port.oper_port_path_cost;
	return 0;
}

int
rstp_info_port_identifier_get( struct cpuport_desc_t desc, unsigned short *port_id) {
	UID_STP_PORT_STATE_T uid_port;
	int rc;
	int port_index;
	for (port_index = 0; port_index <= number_of_ports; port_index++) {
		if( stp_port[port_index].bridge_port_me == desc.bridge_port_me )
			break;
	}

	if( port_index > number_of_ports )
		return -1;

	uid_port.port_no = port_index;
	rc = STP_IN_port_get_state (0, &uid_port);
	if (rc) 
		return -1;
	
	*port_id = uid_port.port_id;
	return 0;
}

int
rstp_info_bridge_forward_delay_get( unsigned short *forward_delay) {
	UID_STP_STATE_T uid_state;
	int rc;

	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;
	*forward_delay = uid_state.forward_delay;
	return 0;
}

int
rstp_info_bridge_hello_time_get( unsigned short *hello_time) {
	UID_STP_STATE_T uid_state;
	int rc;

	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;
	*hello_time = uid_state.hello_time;
	return 0;
}

int
rstp_info_bridge_max_age_get( unsigned short *max_age) {
	UID_STP_STATE_T uid_state;
	int rc;
	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;
	*max_age = uid_state.max_age;
	return 0;
}

int
rstp_info_bridge_priority_get( unsigned short *bridge_priority) {
	UID_STP_STATE_T uid_state;
	int rc;
	rc = STP_IN_stpm_get_state (0, &uid_state);
	if (rc) 
		return -1;
	*bridge_priority = uid_state.bridge_id.prio;
	return 0;
}
