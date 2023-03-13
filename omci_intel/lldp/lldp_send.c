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
 * Module  : lldp
 * File    : lldp_send.c
 *
 ******************************************************************/

#include <errno.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include "util.h"
#include "env.h"
#include "switch.h"
#include "cpuport_frame.h"
#include "lldp_core.h"
#include "iphost.h"
#include "metacfg_adapter.h"
#include "proprietary_calix.h"

///////////////////////////////////////////////////////////////////////////////

int
get_ipaddr(char *ifname, unsigned int *ip_addr, char version)
{
	struct ifreq ifr;
	int sockfd;
	
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
	if (version == AF_INET6) {
		FILE *fp = NULL;
		char cmd[64];
		sprintf(cmd, "cat /proc/net/if_inet6 | grep %s | cut -d' ' -f1 | tail -1", ifname);
		fp = popen(cmd, "r");
		if(fp) {
			fscanf(fp, "%08x%08x%08x%08x", &ip_addr[0], &ip_addr[1], &ip_addr[2], &ip_addr[3]);
			pclose(fp);
			return 0;
		} else {
			return -1;
		}
	} else {
		struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
		ifr.ifr_addr.sa_family = AF_INET;
		addr->sin_family = AF_INET;
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
			dbprintf_lldp(LOG_ERR, "socket err: %d(%s)\n", errno, strerror(errno));
			return -1;
		}
		if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
			*ip_addr = addr->sin_addr.s_addr;
			close(sockfd);
			return 0;
		} else {
			close(sockfd);
			return -1;
		}
	}
}

#define DISABLED 0
static int lldp_get_port_mac_phy_cap(int logical_port_id, lldp_tlv_mac_phy_cap_t *mac_phy_cap)
{
	struct switch_port_mac_config_t port_mac_config;

	if (switch_get_port_config(logical_port_id, &port_mac_config) == 0) {
		if (port_mac_config.force_mode == DISABLED) {
			mac_phy_cap->auto_nego_support = 0x1;
		} else {
			mac_phy_cap->auto_nego_support = 0x3;
		}

		mac_phy_cap->adv_cap = 0x6c01;

		switch (port_mac_config.speed) {
		case SWITCH_PORT_SPEED_10M:
			/* b10baseTHD(10),     -- 10BASE-T  half duplex mode
			 * b10baseTFD(11),     -- 10BASE-T  full duplex mode */
			if (port_mac_config.duplex ==  SWITCH_PORT_HALF_DUPLEX)
				mac_phy_cap->oper = 10;
			else
				mac_phy_cap->oper = 11;
			break;
		case SWITCH_PORT_SPEED_100M:
			/* b100baseTXHD(15),   -- 100BASE-TX half duplex mode
			 * b100baseTXFD(16),   -- 100BASE-TX full duplex mode */
			if (port_mac_config.duplex ==  SWITCH_PORT_HALF_DUPLEX)
				mac_phy_cap->oper = 15;
			else
				mac_phy_cap->oper = 16;

			break;
		case SWITCH_PORT_SPEED_1000M:
			/* b1000baseTHD(29),   -- 1000BASE-T half duplex mode
			 * b1000baseTFD(30),   -- 1000BASE-T full duplex mode */
			if (port_mac_config.duplex ==  SWITCH_PORT_HALF_DUPLEX)
				mac_phy_cap->oper = 29;
			else
				mac_phy_cap->oper = 30;
			break;
		default:
			mac_phy_cap->oper = 0;
			break;
		}

	} else {
		/* default */
		mac_phy_cap->auto_nego_support = 0x0;
		mac_phy_cap->adv_cap = 1;
		mac_phy_cap->oper = 0;
	}

	return 0;
}

int lldp_get_med_cap(void)
{
	int med_cap;
	switch (omci_env_g.lldp_med_opt) {
	case LLDP_MED_OPT_FORCE:
		med_cap = (LLDP_MED_CAP_CAP | LLDP_MED_CAP_POLICY | LLDP_MED_CAP_LOCATION | LLDP_MED_CAP_MDI_PSE | LLDP_MED_CAP_MDI_PD | LLDP_MED_CAP_IVENTORY);
		break;
	case LLDP_MED_OPT_W_CAP:
		med_cap = omci_env_g.lldp_med_cap;
		break;
	case LLDP_MED_OPT_NONE:
	default:
		med_cap = 0;
		break;
	}
	return med_cap;

}


int lldp_pkt_parse(unsigned char* frame, lldp_pkt_t* lldp_pkt_p)
{
	unsigned int frame_offset;

	memcpy(frame, lldp_pkt_p->dst_mac, IFHWADDRLEN);
	frame_offset = IFHWADDRLEN;
	memcpy(frame + frame_offset, lldp_pkt_p->src_mac, IFHWADDRLEN);
	frame_offset += IFHWADDRLEN;
	memcpy(frame + frame_offset, &lldp_pkt_p->ethertype, sizeof(lldp_pkt_p->ethertype));
	frame_offset += sizeof(lldp_pkt_p->ethertype);

	/* mandatory TLVs  */
	/* 1) Chassis ID TLV */
	memcpy(frame + frame_offset, &lldp_pkt_p->chassis_id, lldp_pkt_p->chassis_id.tlv_length + 2);
	frame_offset += (lldp_pkt_p->chassis_id.tlv_length+2);
	/* 2) Port ID TLV */
	memcpy(frame + frame_offset, &lldp_pkt_p->port_id, lldp_pkt_p->port_id.tlv_length + 2);
	frame_offset += (lldp_pkt_p->port_id.tlv_length+2);
	/* 3) Time To Live TLV */
	memcpy(frame + frame_offset, &lldp_pkt_p->ttl, lldp_pkt_p->ttl.tlv_length + 2);
	frame_offset += (lldp_pkt_p->ttl.tlv_length+2);

	if (lldp_pkt_p->ttl.ttl > 0) {
		/* Optional TLVs */
		memcpy(frame + frame_offset, &lldp_pkt_p->port_desc, lldp_pkt_p->port_desc.tlv_length + 2);
		frame_offset += (lldp_pkt_p->port_desc.tlv_length+2);

		memcpy(frame + frame_offset, &lldp_pkt_p->system_name, lldp_pkt_p->system_name.tlv_length + 2);
		frame_offset += (lldp_pkt_p->system_name.tlv_length+2);

		memcpy(frame + frame_offset, &lldp_pkt_p->system_desc, lldp_pkt_p->system_desc.tlv_length + 2);
		frame_offset += (lldp_pkt_p->system_desc.tlv_length+2);

		memcpy(frame + frame_offset, &lldp_pkt_p->system_cap, lldp_pkt_p->system_cap.tlv_length + 2);
		frame_offset += (lldp_pkt_p->system_cap.tlv_length+2);

		memcpy(frame + frame_offset, &lldp_pkt_p->manage_addr_ipv4, lldp_pkt_p->manage_addr_ipv4.tlv_length + 2);
		frame_offset += (lldp_pkt_p->manage_addr_ipv4.tlv_length+2);

		memcpy(frame + frame_offset, &lldp_pkt_p->manage_addr_ipv6, lldp_pkt_p->manage_addr_ipv6.tlv_length + 2);
		frame_offset += (lldp_pkt_p->manage_addr_ipv6.tlv_length+2);

		if (lldp_pkt_p->media_cap.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->media_cap, lldp_pkt_p->media_cap.tlv_length + 2);
			frame_offset += (lldp_pkt_p->media_cap.tlv_length+2);
		}

		if (lldp_pkt_p->location_id.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->location_id, lldp_pkt_p->location_id.tlv_length + 2);
			frame_offset += (lldp_pkt_p->location_id.tlv_length+2);
		}

		if (lldp_pkt_p->ext_power.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->ext_power, lldp_pkt_p->ext_power.tlv_length + 2);
			frame_offset += (lldp_pkt_p->ext_power.tlv_length+2);
		}

		if (lldp_pkt_p->hw_rev.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->hw_rev, lldp_pkt_p->hw_rev.tlv_length + 2);
			frame_offset += (lldp_pkt_p->hw_rev.tlv_length+2);
		}

		if (lldp_pkt_p->fw_rev.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->fw_rev, lldp_pkt_p->fw_rev.tlv_length + 2);
			frame_offset += (lldp_pkt_p->fw_rev.tlv_length+2);
		}

		if (lldp_pkt_p->sw_rev.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->sw_rev, lldp_pkt_p->sw_rev.tlv_length + 2);
			frame_offset += (lldp_pkt_p->sw_rev.tlv_length+2);
		}

		if (lldp_pkt_p->serial_no.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->serial_no, lldp_pkt_p->serial_no.tlv_length + 2);
			frame_offset += (lldp_pkt_p->serial_no.tlv_length+2);
		}

		if (lldp_pkt_p->mfg_name.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->mfg_name, lldp_pkt_p->mfg_name.tlv_length + 2);
			frame_offset += (lldp_pkt_p->mfg_name.tlv_length+2);
		}

		if (lldp_pkt_p->model_name.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->model_name, lldp_pkt_p->model_name.tlv_length + 2);
			frame_offset += (lldp_pkt_p->model_name.tlv_length+2);
		}

		if (lldp_pkt_p->network_policy.tlv_length > 0) {
			memcpy(frame + frame_offset, &lldp_pkt_p->network_policy, lldp_pkt_p->network_policy.tlv_length + 2);
			frame_offset += (lldp_pkt_p->network_policy.tlv_length+2);
		}

		memcpy(frame + frame_offset, &lldp_pkt_p->mac_phy_cap, lldp_pkt_p->mac_phy_cap.tlv_length + 2);
		frame_offset += (lldp_pkt_p->mac_phy_cap.tlv_length+2);
	}


	/* End Of LLDPDU TLV */
	memcpy(frame + frame_offset, &lldp_pkt_p->end, lldp_pkt_p->end.tlv_length + 2);
	frame_offset += (lldp_pkt_p->end.tlv_length+2);
	return frame_offset;
}

int
lldp_send_pkt_process(int port_id)
{
	lldp_pkt_t lldp_pkt;
	struct cpuport_info_t pktinfo;
	unsigned int frame_offset = 0;
	unsigned char oui[3] = {0x00, 0x12, 0xBB}, buf[DESC_SIZE]; //TIA TR-41
	unsigned char oui_802dot3[3] = {0x00, 0x12, 0x0F};
	char model[NAME_SIZE] = {0};
	char onu_serial[NAME_SIZE] = {0};
	char hw_rev[NAME_SIZE] = {0};
	char fw_rev[NAME_SIZE] = {0};
	unsigned char frame[BUF_SIZE] = { 0 };
	char mfg_name[6] = {0};

	extern lldp_parm_t lldp_parm[];
	lldp_parm_t* lldp_parm_p = &lldp_parm[port_id];

	struct switch_port_info_t port_info;
	switch_get_port_info_by_uni_num(port_id, &port_info);

#ifdef OMCI_METAFILE_ENABLE
	metacfg_adapter_config_file_get_value(GPON_DAT_FILE, "onu_serial", onu_serial);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
		metacfg_adapter_config_file_get_value(CALIX_DAT_FILE, "calix_model", model);
		metacfg_adapter_config_file_get_value(CALIX_DAT_FILE, "calix_300_PN", hw_rev);
		strcpy(mfg_name, "Calix");
	} else {
		metacfg_adapter_config_file_get_value(SYSTEM_VERSION_FILE, "model", model);
		strcpy(hw_rev, "1.0");
		strcpy(mfg_name, "5VT");
	}
	metacfg_adapter_config_file_get_value(SYSTEM_VERSION_FILE, "version", fw_rev);
#endif

	memset(&lldp_pkt, 0 , sizeof(lldp_pkt_t));
	// Fill DA
	memcpy(lldp_pkt.dst_mac, lldp_parm_p->mib.tx_dst_mac, IFHWADDRLEN);

	// Fill SA
	memcpy(lldp_pkt.src_mac, lldp_parm_p->mib.tx_src_mac, IFHWADDRLEN);

	// Fill Ethertype
	lldp_pkt.ethertype = LLDP_ETHERTYPE;

	// Fill Chassis ID TLV
	lldp_pkt.chassis_id.tlv_type = LLDP_TLV_TYPE_CHASSIS_ID;
	lldp_pkt.chassis_id.tlv_length = IFHWADDRLEN+1;
	lldp_pkt.chassis_id.subtype = 4; // MAC Address
	extern unsigned char lldp_base_mac[];
	memcpy(lldp_pkt.chassis_id.chassis_id, lldp_base_mac, IFHWADDRLEN);

	// Fill Port ID TLV
	lldp_pkt.port_id.tlv_type = LLDP_TLV_TYPE_PORT_ID;
	lldp_pkt.port_id.tlv_length = sizeof(lldp_pkt.port_id.subtype) + sizeof(lldp_pkt.port_id.port_id);
	lldp_pkt.port_id.subtype = 3;
	iphost_get_hwaddr(omci_env_g.lldp_ifname, lldp_pkt.port_id.port_id);

	if (lldp_parm_p->sm.is_shutdown == TRUE) {

		lldp_pkt.ttl.tlv_type = LLDP_TLV_TYPE_TIME_TO_LIVE;
		lldp_pkt.ttl.tlv_length = sizeof(lldp_pkt.ttl.ttl);
		lldp_pkt.ttl.ttl = 0; // Seconds
	} else {
		// Fill TTL TLV
		lldp_pkt.ttl.tlv_type = LLDP_TLV_TYPE_TIME_TO_LIVE;
		lldp_pkt.ttl.tlv_length = sizeof(lldp_pkt.ttl.ttl);
		lldp_pkt.ttl.ttl = 120; // Seconds

		// Fill System Name TLV
		lldp_pkt.system_name.tlv_type = LLDP_TLV_TYPE_SYSTEM_NAME;
		if (strcmp(omci_env_g.lldp_system_name, "")) {
			strncpy(lldp_pkt.system_name.name, omci_env_g.lldp_system_name, NAME_SIZE);
		} else {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, NAME_SIZE, "%s.%s", model, onu_serial);
			strncpy(lldp_pkt.system_name.name, buf, NAME_SIZE);
		}
		lldp_pkt.system_name.tlv_length = strlen(lldp_pkt.system_name.name);

		// Fill System Description TLV
		lldp_pkt.system_desc.tlv_type = LLDP_TLV_TYPE_SYSTEM_DESC;
		if (strcmp(omci_env_g.lldp_system_desc, "")) {
			strncpy(lldp_pkt.system_desc.desc, omci_env_g.lldp_system_desc, DESC_SIZE);
		} else {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, DESC_SIZE, "Manufacturer:%s ModelName:%s SerialNumber:%s HardwareVersion:%s SoftwareVersion:%s",
					 mfg_name, model, onu_serial, hw_rev, fw_rev);
			strncpy(lldp_pkt.system_desc.desc, buf, DESC_SIZE);
		}
		lldp_pkt.system_desc.tlv_length = strlen(lldp_pkt.system_desc.desc);

		// Fill System Capabilities TLV
		lldp_pkt.system_cap.tlv_type = LLDP_TLV_TYPE_SYSTEM_CAP;
		lldp_pkt.system_cap.tlv_length = sizeof(lldp_pkt.system_cap.cap)+sizeof(lldp_pkt.system_cap.en_cap);
		lldp_pkt.system_cap.cap = 0x14; // Router+Bridge
		lldp_pkt.system_cap.en_cap = 0x14; // Router+Bridge

		// Fill Management Address (IPv4) TLV
		lldp_pkt.manage_addr_ipv4.tlv_type = LLDP_TLV_TYPE_MANAGE_ADDR;
		lldp_pkt.manage_addr_ipv4.tlv_length = 12;
		lldp_pkt.manage_addr_ipv4.length = 5;
		lldp_pkt.manage_addr_ipv4.subtype = 1; // IPv4
		lldp_pkt.manage_addr_ipv4.itf_subtype = 1;
		lldp_pkt.manage_addr_ipv4.itf_number = 0;
		lldp_pkt.manage_addr_ipv4.oid_str_length = 0;
		get_ipaddr(omci_env_g.lldp_ifname, &lldp_pkt.manage_addr_ipv4.addr, AF_INET);

		// Fill Management Address (IPv6) TLV
		lldp_pkt.manage_addr_ipv6.tlv_type = LLDP_TLV_TYPE_MANAGE_ADDR;
		lldp_pkt.manage_addr_ipv6.tlv_length = 24;
		lldp_pkt.manage_addr_ipv6.subtype = 2; // IPv6
		lldp_pkt.manage_addr_ipv6.length = 17;
		lldp_pkt.manage_addr_ipv6.itf_subtype = 1;
		lldp_pkt.manage_addr_ipv6.itf_number = 0;
		lldp_pkt.manage_addr_ipv6.oid_str_length = 0;
		get_ipaddr(omci_env_g.lldp_ifname, lldp_pkt.manage_addr_ipv6.addr, AF_INET6);

		unsigned short med_cap;
		switch (omci_env_g.lldp_med_opt) {
		case LLDP_MED_OPT_FORCE:
			med_cap = (LLDP_MED_CAP_CAP | LLDP_MED_CAP_POLICY | LLDP_MED_CAP_LOCATION | LLDP_MED_CAP_MDI_PSE | LLDP_MED_CAP_MDI_PD | LLDP_MED_CAP_IVENTORY);
			break;
		case LLDP_MED_OPT_W_CAP:
			med_cap = omci_env_g.lldp_med_cap;
			break;
		case LLDP_MED_OPT_NONE:
		default:
			med_cap = 0;
			break;
		}
		// Fill Media Capabilities TLV
		if (med_cap & LLDP_MED_CAP_CAP) {
			lldp_pkt.media_cap.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.media_cap.tlv_length = 7;
			memcpy(lldp_pkt.media_cap.oui, oui, sizeof(oui));
			lldp_pkt.media_cap.subtype = LLDP_TLV_CUSTOM_SUBTYPE_MEDIA_CAP;
			lldp_pkt.media_cap.cap = med_cap;
			lldp_pkt.media_cap.type = 4; // Network Connectivity
		}

		if (med_cap & LLDP_MED_CAP_LOCATION) {
			// Fill Location ID TLV
			lldp_pkt.location_id.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.location_id.tlv_length = 15;
			memcpy(lldp_pkt.location_id.oui, oui, sizeof(oui));
			lldp_pkt.location_id.subtype = LLDP_TLV_CUSTOM_SUBTYPE_LOCATION_ID;
			lldp_pkt.location_id.lci_format = 3; // ECS (Emergency Call Service)
			memset(lldp_pkt.location_id.lci, 0, sizeof(lldp_pkt.location_id.lci));
		}

		if (med_cap & LLDP_MED_CAP_IVENTORY) {
			// Fill Hardware Revision TLV
			lldp_pkt.hw_rev.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.hw_rev.tlv_length = 14;
			memcpy(lldp_pkt.hw_rev.oui, oui, sizeof(oui));
			lldp_pkt.hw_rev.subtype = LLDP_TLV_CUSTOM_SUBTYPE_HW_REVISION;
			strcpy(lldp_pkt.hw_rev.revision, hw_rev);

			// Fill Firmware Revision TLV
			lldp_pkt.fw_rev.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.fw_rev.tlv_length = 14;
			memcpy(lldp_pkt.fw_rev.oui, oui, sizeof(oui));
			lldp_pkt.fw_rev.subtype = LLDP_TLV_CUSTOM_SUBTYPE_FW_REVISION;
			strcpy(lldp_pkt.fw_rev.revision, fw_rev);

			// Fill Software Revision TLV
			lldp_pkt.sw_rev.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.sw_rev.tlv_length = 14;
			memcpy(lldp_pkt.sw_rev.oui, oui, sizeof(oui));
			lldp_pkt.sw_rev.subtype = LLDP_TLV_CUSTOM_SUBTYPE_SW_REVISION;
			strcpy(lldp_pkt.sw_rev.revision, fw_rev);

			// Fill Serial Number TLV
			lldp_pkt.serial_no.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.serial_no.tlv_length = 16;
			memcpy(lldp_pkt.serial_no.oui, oui, sizeof(oui));
			lldp_pkt.serial_no.subtype = LLDP_TLV_CUSTOM_SUBTYPE_SERIAL_NO;
			strcpy(lldp_pkt.serial_no.sn, onu_serial);

			// Fill Manufacturer Name TLV
			lldp_pkt.mfg_name.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.mfg_name.tlv_length = 9;
			memcpy(lldp_pkt.mfg_name.oui, oui, sizeof(oui));
			lldp_pkt.mfg_name.subtype = LLDP_TLV_CUSTOM_SUBTYPE_MFG_NAME;
			strcpy(lldp_pkt.mfg_name.name, mfg_name);

			// Fill Model Name TLV
			lldp_pkt.model_name.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.model_name.tlv_length = 12;
			memcpy(lldp_pkt.model_name.oui, oui, sizeof(oui));
			lldp_pkt.model_name.subtype = LLDP_TLV_CUSTOM_SUBTYPE_MODEL_NAME;
			strcpy(lldp_pkt.model_name.name, model);
		}


		// Fill Port Description TLV
		lldp_pkt.port_desc.tlv_type = LLDP_TLV_TYPE_PORT_DESC;
		snprintf(lldp_pkt.port_desc.desc, sizeof(lldp_pkt.port_desc.desc), "Ethernet Port Number %d", (unsigned char)port_id+1);
		lldp_pkt.port_desc.tlv_length = strlen(lldp_pkt.port_desc.desc);

		lldp_pkt.mac_phy_cap.tlv_type = LLDP_TLV_TYPE_CUSTOM;
		lldp_pkt.mac_phy_cap.tlv_length = 9;
		lldp_pkt.mac_phy_cap.subtype = 1;
		memcpy(lldp_pkt.mac_phy_cap.oui, oui_802dot3, sizeof(oui));
		lldp_get_port_mac_phy_cap(port_info.logical_port_id, &lldp_pkt.mac_phy_cap);

		if (med_cap & LLDP_MED_CAP_MDI_PSE) {
			// Fill Extended Power TLV
			lldp_pkt.ext_power.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.ext_power.tlv_length = 7;
			memcpy(lldp_pkt.ext_power.oui, oui, sizeof(oui));
			lldp_pkt.ext_power.subtype = LLDP_TLV_CUSTOM_SUBTYPE_EXT_POWER;
			lldp_pkt.ext_power.power_type = 0; /* power source entity (PSE) : 0  or power device (PD) : 1 */
			lldp_pkt.ext_power.power_source = 1; /* 0: unknown 1: primary 2: backup */
			lldp_pkt.ext_power.power_priority = lldp_parm_p->mib.power_priority;
			lldp_pkt.ext_power.power_value = lldp_parm_p->mib.power_value; // 0.1 W

		}


		if (med_cap & LLDP_MED_CAP_POLICY) {
			lldp_pkt.network_policy.tlv_type = LLDP_TLV_TYPE_CUSTOM;
			lldp_pkt.network_policy.tlv_length = 8;
			memcpy(lldp_pkt.network_policy.oui, oui, sizeof(oui));
			lldp_pkt.network_policy.subtype = LLDP_TLV_CUSTOM_SUBTYPE_NETWORK_POLICY;
			lldp_pkt.network_policy.type = lldp_parm_p->mib.network_policy.type;
			lldp_pkt.network_policy.policy = lldp_parm_p->mib.network_policy.policy;
			lldp_pkt.network_policy.vid = lldp_parm_p->mib.network_policy.vid;
			lldp_pkt.network_policy.pbit = lldp_parm_p->mib.network_policy.pbit;
			lldp_pkt.network_policy.dscp = lldp_parm_p->mib.network_policy.dscp;
			lldp_pkt.network_policy.tagged = lldp_parm_p->mib.network_policy.tagged;
		}
	} /* End of else is_shutdown */

	// Fill End TLV
	lldp_pkt.end.tlv_type = LLDP_TLV_TYPE_END;
	lldp_pkt.end.tlv_length = 0;

	frame_offset = lldp_pkt_parse( frame, &lldp_pkt);

	pktinfo.frame_ptr = (unsigned char *) &frame;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (frame_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : frame_offset;
	pktinfo.rx_desc.logical_port_id = switch_get_cpu_logical_portid(0);
	pktinfo.tx_desc.logical_port_id = port_info.logical_port_id;
	pktinfo.rx_desc.bridge_port_me = NULL;
	pktinfo.tx_desc.bridge_port_me = port_info.bridge_port_me;

	// fill pktinfo's tci
	memset(&pktinfo.tci, 0, sizeof(struct cpuport_tci_t));
	if ( ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == LLDP_ETHERTYPE ) {
		unsigned short tci = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+2 ));
		pktinfo.tci.itci.tpid = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2 ));
		pktinfo.tci.itci.priority = tci >> 13;
		pktinfo.tci.itci.de = (tci >> 12) & 1;
		pktinfo.tci.itci.vid = tci & 0xfff;
		pktinfo.tci.ethertype = LLDP_ETHERTYPE;
	} else if ( ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == LLDP_ETHERTYPE ) {
		unsigned short tci = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+2 ));
		pktinfo.tci.otci.tpid = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2 ));
		pktinfo.tci.otci.priority = tci >> 13;
		pktinfo.tci.otci.de = (tci >> 12) & 1;
		pktinfo.tci.otci.vid = tci & 0xfff;
		tci = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET+2 ));
		pktinfo.tci.itci.tpid = ntohs( *(unsigned short *)( pktinfo.frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ));
		pktinfo.tci.itci.priority = tci >> 13;
		pktinfo.tci.itci.de = (tci >> 12) & 1;
		pktinfo.tci.itci.vid = tci & 0xfff;
		pktinfo.tci.ethertype = LLDP_ETHERTYPE;
	}

	if(cpuport_frame_send(&pktinfo) < 0) {
		dbprintf_lldp(LOG_WARNING, "lldp can't send, logical_port_id=%d\n", pktinfo.tx_desc.logical_port_id);
	} else {
		lldp_parm[pktinfo.tx_desc.logical_port_id].pkt_c.tx++;
		dbprintf_lldp(LOG_NOTICE, "lldp sent, logical_port_id=%d\n", pktinfo.tx_desc.logical_port_id);
	}

	return 0;
}
