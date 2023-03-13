
#ifndef __INTEL_PX126_MCC_UTIL_H__
#define __INTEL_PX126_MCC_UTIL_H__

#include "list.h"
#include "switch.h"
#include "fwk_mutex.h"

#include "intel_px126_util.h"

#include "pon_adapter.h"
#include "omci/pon_adapter_mcc.h"

enum mcc_vlan_mode {
	/** VLAN unaware mode. */
	MCC_VLAN_MODE_UNAWARE = 0,
	/** VLAN aware mode. */
	MCC_VLAN_MODE_AWARE = 1
};
/** Rate Limiter type.
*/
enum mcc_rl_type {
	/** Not available */
	MCC_RL_TYPE_NA = 0,
	/** Per port */
	MCC_RL_TYPE_PORT = 1,
	/** Per service */
	MCC_RL_TYPE_SERVICE = 2
};

/** List.
*/
struct mcc_list {
	/** List head.*/
	struct list_head head;
	/** Number of list entries.*/
	unsigned int  num;
};
/** Rate Limiter entry.
*/
struct mcc_rl_entry {
	/** VLAN identifier of the US Client sending IGMP/MLD requests.
	    0xFFFF value - Unspecified. */
	unsigned short cvid;
	/** Messages count since last limit.*/
	unsigned int msg_cnt;
	/** Reference time stamp since last limit.*/
	struct timespec ref_ts;
	/** Rate limit (messages/second) */
	unsigned int rate;

	/** Linked list entry.*/
	struct list_head le;
};
/** Rate Limiter.
*/
struct mcc_rl {
	/** Type.*/
	enum mcc_rl_type type;
	/** Rate Limiter list.*/
	struct mcc_list rl_list;
};	
/** Multicast port.
*/
struct mcc_port {
	/** Port access lock. */
	struct fwk_mutex_t lock;
	/** Join messages counter */
	unsigned int join_msg_cnt;
	/** Bandwidth exceeded messages counter */
	unsigned int exce_msg_cnt;
	/** Flow list. */
	struct mcc_list flw_list;
	/** Rate Limiter.*/
	struct mcc_rl rl;
};


/** Structure to specify Multicast Control context. */
struct mcc_ctx {
	/** Global MCC VLAN handling mode. */
	enum mcc_vlan_mode vlan_mode;
	/** OMCI Core Context.*/
	void *ctx_core;
	/** MCC ops of selected PON adapter module */
	struct pa_omci_mcc_ops const *mcc_ops;
	/** ctx for selected PON adapter module */
	void *ll_ctx;
	/** Record to handle IGMP/MLD requests.*/
	//struct mcc_rec rec;
	/** Max number of ports */
	unsigned int  max_ports;
	/** Multicast port(s).*/
	struct mcc_port *port;
	struct fwk_mutex_t mcc_lock;
};
/** Packet information details. */


/** Packet information and action. */
struct mcc_pkt {
	/** Length, [bytes]. */
	unsigned short len;
	/** Packet data. */
	unsigned char * data;
	/** Indicates if the packet should be dropped. */
	unsigned char drop;
	/** Info */
	struct pa_mcc_pkt_info llinfo;
};
/** Multicast IP address */
union px126_mcc_ip_addr {
	/** IPv4 Address */
	unsigned char ipv4[4];
	/** IPv6 Address */
	unsigned char ipv6[16];
};
enum px126_mcc_dir {
	/** Downstream */
	px126_MCC_DIR_DS = 0,
	/** Upstream */
	px126_MCC_DIR_US = 1
};

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
int intel_mcc_init(struct intel_omci_context *context);
int  intel_mcc_exit(struct intel_omci_context *context);
int intel_mcc_pkt_receive(struct mcc_pkt * mcc_pkt);
int intel_mcc_pkt_send(struct mcc_pkt * mcc_pkt);
int intel_mcc_dev_port_add(enum px126_mcc_dir dir ,unsigned char lan_port,unsigned char fid,union px126_mcc_ip_addr *ip);
int intel_mcc_dev_port_remove(unsigned char lan_port,unsigned char fid,union px126_mcc_ip_addr *ip);
int intel_mcc_dev_port_activity_get(unsigned char lport, unsigned char fid,union px126_mcc_ip_addr *ip,  unsigned char *is_active);
int intel_mcc_dev_fid_get(unsigned short o_vid,	unsigned char *fid);
int intel_mcc_dev_vlan_unaware_mode_enable( const bool enable);
int intel_mcc_me309_create(unsigned short meid,unsigned short igmp_ver);
int intel_mcc_me309_del(unsigned short meid);
int intel_mcc_me310_extvlan_set(struct switch_mc_profile_ext_vlan_update_data *up_data);

#endif
