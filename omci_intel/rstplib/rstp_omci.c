#include "fwk_msgq.h"
#include "meinfo.h"
#include "hwresource.h"
#include "util.h"
#include "switch.h"
#include "cpuport.h"
#include "notify_chain.h"

#include "rstp.h"

int
rstp_omci_stp_bridge_instance_check( void ) {

	struct meinfo_t *port_miptr;
	struct me_t *port_me, *pptp_uni_me;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	// traverse all bridge port me and make sure the number of ports >= 2
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
		if ((pptp_uni_me = meinfo_me_get_by_meid(11, port_me->meid)) == NULL )
			continue;
		else
			return 0;
	}
	return -1;
}

int
rstp_omci_stp_bridge_service_enable( void ) {

	struct meinfo_t *port_miptr;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
	
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	if (list_empty(&port_miptr->me_instance_list))
		return 0;
	else
		return 1;

	return -1;
}


int
rstp_omci_stp_bridge_priority_get( unsigned short *bridge_priority) {

	struct meinfo_t *port_miptr;
	struct me_t *port_me, *bridge_me;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
	
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}
	// traverse all bridge port me and make sure the number of ports >= 2
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
	
		bridge_me=meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(port_me, 1));
		if ( bridge_me == NULL )
			return -1;
		else {
			*bridge_priority =  meinfo_util_me_attr_data(bridge_me, 4);
			return 0;
		}
	}
	return -1;
}


int
rstp_omci_stp_bridge_max_age_get( unsigned short *max_age) {

	struct meinfo_t *port_miptr;
	struct me_t *port_me, *bridge_me;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
	
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	// traverse all bridge port me and make sure the number of ports >= 2
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
	
		bridge_me=meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(port_me, 1));
		if ( bridge_me == NULL )
			return -1;
		else{
			*max_age =  meinfo_util_me_attr_data(bridge_me, 5)/256;
			if (*max_age < 6 ) 
				*max_age = 6;
			if (*max_age > 40)
				*max_age = 40;
			return 0;
		}
	}
	return -1;
}


int
rstp_omci_stp_bridge_hello_time_get( unsigned short *hello_time) {

	struct meinfo_t *port_miptr;
	struct me_t *port_me, *bridge_me;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
	
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	// traverse all bridge port me and make sure the number of ports >= 2
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
	
		bridge_me=meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(port_me, 1));
		if ( bridge_me == NULL ) 
			return -1;
		else {
			*hello_time =  meinfo_util_me_attr_data(bridge_me, 6)/256;
			if (*hello_time < 1 ) 
				*hello_time = 1;
			if (*hello_time > 10)
				*hello_time = 10;
			return 0;
		}
	}
	return -1;
}


int
rstp_omci_stp_bridge_forward_delay_get( unsigned short *forward_delay) {

	struct meinfo_t *port_miptr;
	struct me_t *port_me, *bridge_me;

	// 47 MAC_bridge_port_configuration_data
	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) {
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}

	// traverse all bridge port me and make sure the number of ports >= 2
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
	
		bridge_me=meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(port_me, 1));
		if ( bridge_me == NULL )
			return -1;
		else {
			*forward_delay =  meinfo_util_me_attr_data(bridge_me, 7)/256;
			if (*forward_delay< 4 ) 
				*forward_delay= 4;
			if (*forward_delay> 10)
				*forward_delay= 10;
			return 0;
		}
	}
	return -1;
}

int
rstp_omci_stp_port_priority_get( struct cpuport_desc_t desc, unsigned short *priority) {

	if (desc.bridge_port_me == NULL) {
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 47);
		return -1;
	}	
	if ((*priority = meinfo_util_me_attr_data( desc.bridge_port_me, 5)) > 255 )
	       return -1;	
	return 0;
}

int
rstp_omci_stp_port_identifier_get( struct cpuport_desc_t desc, unsigned short *port_id) {

	if(desc.bridge_port_me == NULL ){
		dbprintf(LOG_ERR, "bridge port me == NULL\n");
		return -1;
	}
	*port_id = (meinfo_util_me_attr_data( desc.bridge_port_me, 5) << 8) + meinfo_util_me_attr_data( desc.bridge_port_me, 2);
	return 0;
}


int
rstp_omci_stp_port_path_cost_get( struct cpuport_desc_t desc, unsigned short *path_cost)
{
	if(desc.bridge_port_me == NULL ){
		dbprintf(LOG_ERR, "bridge port me == NULL\n");
		return -1;
	}
	*path_cost = meinfo_util_me_attr_data(desc.bridge_port_me , 6);

	return 0;
}

int
rstp_omci_port_me_to_desc(struct me_t *port_me, struct cpuport_desc_t *desc)
{
	struct me_t *iport_me;

	iport_me=hwresource_public2private(port_me);	
	if(iport_me == NULL)
		return -1;

	desc->logical_port_id = meinfo_util_me_attr_data(iport_me, 6);
	desc->bridge_port_me = port_me;
	return 0;
}


int
rstp_omci_notifychain_handler(unsigned char event, unsigned short classid, 
				unsigned short instance_num, unsigned short meid, void *data_ptr)
{
//dbprintf(LOG_ERR, "classid: %d meid: %d event:%d\n",classid,meid,event);
	if (classid != 45 && classid != 47 )
		return 0;

	struct rstp_cmd_msg_t *rstp_cmd_msg; 
	rstp_cmd_msg = malloc_safe(sizeof(struct rstp_cmd_msg_t));
	INIT_LIST_NODE(&rstp_cmd_msg->q_entry);

	rstp_cmd_msg->classid = classid; 
	rstp_cmd_msg->meid = meid; 

	switch (event) {
		case NOTIFY_CHAIN_EVENT_ME_CREATE:
			rstp_cmd_msg->cmd = RSTP_CMD_ME_ADD_MSG; 
			break;
		case NOTIFY_CHAIN_EVENT_ME_DELETE:
			rstp_cmd_msg->cmd = RSTP_CMD_ME_DEL_MSG; 
			break;
		case NOTIFY_CHAIN_EVENT_ME_UPDATE:
			rstp_cmd_msg->cmd = RSTP_CMD_ME_UPDATE_MSG; 
			break;
		default:
			free_safe( rstp_cmd_msg );
			return 0;

	}


	if ( fwk_msgq_send(rstp_cmd_qid_g, &rstp_cmd_msg->q_entry) < 0) {
	
		free_safe( rstp_cmd_msg );
		dbprintf(LOG_ERR, "send rstp create me cmd err\n");
	}

	return 0;
}


void
rstp_omci_notify_chain_init(void)
{
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, rstp_omci_notifychain_handler, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_DELETE, rstp_omci_notifychain_handler, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_UPDATE, rstp_omci_notifychain_handler, NULL);
}

void
rstp_omci_notify_chain_finish(void)
{
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_CREATE, rstp_omci_notifychain_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_DELETE, rstp_omci_notifychain_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_UPDATE, rstp_omci_notifychain_handler, NULL);
}

