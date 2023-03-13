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
 * Module  : omcidump
 * File    : omcidump_test.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include "meinfo.h"
#include "omcimsg.h"
#include "omcidump.h"
#include "util.h"
#include "util_inet.h"
#include "env.h"

static char *pots_isdn_testname[]={
	"all MLT tests"
	"hazardous potential",
	"foreign EMF",
	"resistive faults",
	"receiver off-hook",
	"ringer",
	"NT1 dc signature test",
	"self test",
	"dial tone make-break test"
};

static char *selftest_result_str[]={
	"failed", 
	"passed", 
	"not complete"
};

static char *vendor_specific_test_type_str[]={
	"undefined",
	"power feed voltage, 20mv",
	"low voltage, 100uv", 
	"received optical power, 0.002db", 
	"received optical power, 0.1uw",
	"transmitted optical power, 0.002db",
	"transmitted optical power, 0.1uw",
	"video level, 0.002db",
	"video level, 200uv",
	"laser bias current, 2uA",
	"received signal quality measure Q, 0.1",
	"signal to noise, 0.1db",
	"temporature, 1/256 degree C",
};

int
omcidump_test(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(msg);

	// 256 ONT-G, 6 CircuitPack
	if (classid==256 || classid==263 || classid==6) {
		struct omcimsg_test_ont_ani_circuit_t *req = (struct omcimsg_test_ont_ani_circuit_t *) msg->msg_contents;		
		util_fdprintf(fd, "select_test: %u ", req->select_test);
		if (req->select_test==7) {		// selft test
			util_fdprintf(fd, "(%s)\n", "self test");
		} else if (req->select_test>=8 && req->select_test<=15) { // vendor specific test
			util_fdprintf(fd, "(%s)\n", "vendor specific");
		} else {
			util_fdprintf(fd, "(%s)\n", "invalid");
		}

	// 256 ONT-G, 263 ANI-G, 6 CircuitPack
	} else if (classid==263) {
		struct omcimsg_test_ont_ani_circuit_t *req = (struct omcimsg_test_ont_ani_circuit_t *) msg->msg_contents;		
		util_fdprintf(fd, "select_test: %u ", req->select_test);
		if (req->select_test==7) {		// selft test
			util_fdprintf(fd, "(%s)\n", "line test");
		} else {
			util_fdprintf(fd, "(%s)\n", "invalid");
		}

	// 134 IP Host Config Data
	} else if (classid==134) {
		struct omcimsg_test_ip_host_t *req = (struct omcimsg_test_ip_host_t *) msg->msg_contents;
		unsigned char *ip=(unsigned char*)&(req->u.target_ipaddr);
		unsigned char *ipv6=(unsigned char*)&(req->u.target_ipv6addr);
		char str[128];
		
		util_fdprintf(fd, "select_test: %u ", req->select_test);
		if (req->select_test==1) {		// ping
			util_fdprintf(fd, "(%s)\n", "ping");
		} else if (req->select_test==2) {	// traceroute
			util_fdprintf(fd, "(%s)\n", "traceroute");
		} else if (req->select_test==3) {	// extended ping
			util_fdprintf(fd, "(%s)\n", "extended ping");
		} else if (req->select_test>=8 && req->select_test<=15) {      // vendor specific
			util_fdprintf(fd, "(%s)\n", "vendor specific");
		} else {
			util_fdprintf(fd, "(%s)\n", "invalid");
		}
		if (ipv6[4] || ipv6[5] || ipv6[6] || ipv6[7] || ipv6[8] || ipv6[9] || 
		    ipv6[10] || ipv6[11] || ipv6[12] || ipv6[13] || ipv6[14] || ipv6[15]) {	// ipv6
			util_fdprintf(fd, "target ipv6: %s\n", util_inet_ntop(AF_INET6, ipv6, str, 128));
		} else {
			util_fdprintf(fd, "target ip: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
		}

	// 53 PPTP POTS UNI, 80 PPTP ISDN UNI
	} else if (classid==53 || classid==80) {
		struct omcimsg_test_pots_isdn_t *req=(struct omcimsg_test_pots_isdn_t *) msg->msg_contents;
		unsigned char test_class=(req->select_test >>4) & 0x3;
		unsigned char select_test=req->select_test& 0xf;

		if (test_class == 0) {
			util_fdprintf(fd, "test_class: %u\n", test_class);
			util_fdprintf(fd, "select_test: %u ", select_test);
			if (select_test<=8) {
				util_fdprintf(fd, "(%s)\n", pots_isdn_testname[select_test]);
			} else if (select_test>=9 && select_test<=11) {	// vendor specefic, return result in test result msg
				util_fdprintf(fd, "(%s)\n", "vendor specific");
			} else if (select_test>=12 && select_test<=15) {	// vendor specific, return reault in general purpose buffer ME
				util_fdprintf(fd, "(%s)\n", "vendor specific, result in general purpose buffer me");
			} else {
				util_fdprintf(fd, "(%s)\n", "invalid");
			}
			
			util_fdprintf(fd, "test mode: %d (%s)\n", (req->select_test&0x80)?1:0, (req->select_test&0x80)?"normal":"forced");
			util_fdprintf(fd, "      slow dial tone threshold: %d (0.1sec)\n", req->u.class0.dbdt_timer_t1);
			util_fdprintf(fd, "        no dial tone threshold: %d (0.1sec)\n", req->u.class0.dbdt_timer_t2);
			util_fdprintf(fd, "slow break dial tone threshold: %d (0.1sec)\n", req->u.class0.dbdt_timer_t3);
			util_fdprintf(fd, "  no break dial tone threshold: %d (0.1sec)\n", req->u.class0.dbdt_timer_t4);
			
			util_fdprintf(fd, "dbdt_ctrl_byte: 0x%x\n", req->u.class0.dbdt_control_byte);
			util_fdprintf(fd, "digit_to_be_dialled: 0x%x ('%c')\n", req->u.class0.digit_to_be_dialled, req->u.class0.digit_to_be_dialled);
	
			util_fdprintf(fd, "dial_tone_frequency_1: %d (Hz)\n", ntohs(req->u.class0.dial_tone_frequency_1));
			util_fdprintf(fd, "dial_tone_frequency_2: %d (Hz)\n", ntohs(req->u.class0.dial_tone_frequency_2));
			util_fdprintf(fd, "dial_tone_frequency_3: %d (Hz)\n", ntohs(req->u.class0.dial_tone_frequency_3));
	
			util_fdprintf(fd, "dial_tone_power_threshold: %d (-0.1dbm)\n", req->u.class0.dial_tone_power_threshold);
			util_fdprintf(fd, "idle_channel_power_threshold: %d (-0.1dbm)\n", req->u.class0.idle_channel_power_threshold);
			util_fdprintf(fd, "dc_hazardous_voltage_threshold: %d (volt)\n", req->u.class0.dc_hazardous_voltage_threshold);
			util_fdprintf(fd, "ac_hazardous_voltage_threshold: %d (volt)\n", req->u.class0.ac_hazardous_voltage_threshold);
			util_fdprintf(fd, "dc_foreign_voltage_threshold: %d (volt)\n", req->u.class0.dc_foreign_voltage_threshold);
			util_fdprintf(fd, "ac_foreign_voltage_threshold: %d (volt)\n", req->u.class0.ac_foreign_voltage_threshold);
			util_fdprintf(fd, "tip_ground_and_ring_ground_resistance_threshold: %d (kohm)\n", req->u.class0.tip_ground_and_ring_ground_resistance_threshold);
			util_fdprintf(fd, "tip_ring_resistance_threshold: %d (kohm)\n", req->u.class0.tip_ring_resistance_threshold);
			util_fdprintf(fd, "ringer_equivalence_min_threshold: %d (0.01REN)\n", ntohs(req->u.class0.ringer_equivalence_min_threshold));
			util_fdprintf(fd, "ringer_equivalence_max_threshold: %d (0.01REN)\n", ntohs(req->u.class0.ringer_equivalence_max_threshold));
			util_fdprintf(fd, "pointer_to_a_general_purpose_buffer_me: 0x%x(%u)\n", 
						ntohs(req->u.class0.pointer_to_a_general_purpose_buffer_me), ntohs(req->u.class0.pointer_to_a_general_purpose_buffer_me));
			util_fdprintf(fd, "pointer_to_an_octet_string_me: 0x%x(%u)\n", 
						ntohs(req->u.class0.pointer_to_an_octet_string_me), ntohs(req->u.class0.pointer_to_an_octet_string_me));
		} else if (test_class == 1) {		
			util_fdprintf(fd, "test_class: %u\n", test_class);
			util_fdprintf(fd, "select_test: sip/h.248 test call\n");
			util_fdprintf(fd, "test mode: %d (%s)\n", (req->select_test&0x80)?1:0, (req->select_test&0x80)?"normal":"forced");
			util_fdprintf(fd, "number_to_dial: %s\n", req->u.class1.number_to_dial);
		} else {
			util_fdprintf(fd, "test_class: %u (reserved)\n", test_class);
		}

	// 302 Dot1ag MEP
	} else if (classid==302) {
		struct omcimsg_test_dot1ag_mep_t *req = (struct omcimsg_test_dot1ag_mep_t *) msg->msg_contents;
		
		util_fdprintf(fd, "select_test: %u ", req->select_test);
		if (req->select_test==0) {		// loopback
			unsigned char *mac=(unsigned char*)&(req->u.lb.target_mac);
			util_fdprintf(fd, "(%s)\n", "loopback");
			util_fdprintf(fd, "target mac: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		} else if (req->select_test==1) {	// linktrace
			unsigned char *mac=(unsigned char*)&(req->u.lt.target_mac);
			util_fdprintf(fd, "(%s)\n", "linktrace");
			util_fdprintf(fd, "target mac: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		} else {
			util_fdprintf(fd, "(%s)\n", "invalid");
		}
	} else {
		util_fdprintf(fd, "invalid me classid for test\n");
	}

	return 0;
}

int
omcidump_test_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_test_response_t *test_response_contents = (struct omcimsg_test_response_t *)msg->msg_contents;
	
	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "result: %s\n", 
		omcidump_reason_str(test_response_contents->result_reason));

	util_fdprintf(fd, "\n");

	return 0;
}

int
omcidump_test_result(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(msg);

	omcidump_header(fd, msg);

	//dump msg content
	if (classid==256 || classid==6) {	// ont-g, circuit pack
		if (msg->msg_contents[0]==0) {
			struct omcimsg_test_result_ont_circuit_selftest_t *result=(struct omcimsg_test_result_ont_circuit_selftest_t *)msg->msg_contents;
			char *str="unknown";		
			if (result->self_test_result<=2)
				str=selftest_result_str[result->self_test_result];
			util_fdprintf(fd, "selftest result: %d(%s)\n", result->self_test_result, str);
		} else {
			//struct omcimsg_test_result_ont_vendortest_t *result=(struct omcimsg_test_result_ont_vendortest_t *)msg->msg_contents;
			int i;			

			util_fdprintf(fd, "vendor specific test result:\n");
			for (i=0; i<10; i++) {
				unsigned char type=*(char*)(msg->msg_contents+i);
				short val=ntohs(*(short*)(msg->msg_contents+i+1));
				if (type>=1 && type<=12) {
					util_fdprintf(fd, "\t%d(%s): %d\n", type, vendor_specific_test_type_str[type], val);
				} else if (type>12) {
					util_fdprintf(fd, "\t%d(unknwon): %d\n", type, val);
				}
			}
		}
	} else if (classid==263) {	// ani-g
		struct omcimsg_test_result_ani_linetest_t *result=(struct omcimsg_test_result_ani_linetest_t *)msg->msg_contents;
		util_fdprintf(fd, "linetest result:\n");
		util_fdprintf(fd, "\tpower_feed_voltage: %d (%.3fv)\n", ntohs(result->power_feed_voltage), ntohs(result->power_feed_voltage)*0.02);
		util_fdprintf(fd, "\treceived_optical_power: %d (%.3fdbuw)\n", ntohs(result->received_optical_power), ntohs(result->received_optical_power)*0.002);
		util_fdprintf(fd, "\ttransmitted_optical_power: %d (%.3fdbuw)\n", ntohs(result->transmitted_optical_power), ntohs(result->transmitted_optical_power)*0.002);
		util_fdprintf(fd, "\tlaser_bias_current: %u (%.3fmA)\n", ntohs(result->laser_bias_current), ntohs(result->laser_bias_current)/500.0);
		util_fdprintf(fd, "\ttemperature_degrees: %.3f C degree\n", ntohs(result->temperature_degrees)/256.0);
		
	} else if (classid==134) {	// ip host config data
		struct omcimsg_test_result_ip_host_t *result=(struct omcimsg_test_result_ip_host_t *)msg->msg_contents;
		int i;
		
		if (result->test_result==0) {
			util_fdprintf(fd, "icmp test result: no response\n");
		} else if (result->test_result==1) {
			util_fdprintf(fd, "icmp ping result:\n");
			for (i=0; i<15; i++) {
				util_fdprintf(fd, "\t%d: %u ms\n", i, ntohs(result->u.result_001.response_delay[i]));
			}
			
		} else if (result->test_result==2) {
			util_fdprintf(fd, "icmp traceroute result:\n");
			for (i=0; i<7; i++) {
				struct in_addr ipaddr;
				if (result->u.result_010.neighbour_ipaddr[i]==0)
					break;					
				ipaddr.s_addr=result->u.result_010.neighbour_ipaddr[i];	
				util_fdprintf(fd, "\t%s\n", inet_ntoa(ipaddr));
			}
		} else if (result->test_result==3) {
			util_fdprintf(fd, "icmp unexpected result:\n");
			util_fdprintf(fd, "\ttype=%d\n", result->u.result_011.type);
			util_fdprintf(fd, "\tcode=%d\n", result->u.result_011.code);
			util_fdprintf(fd, "\tchksum=0x%x\n", ntohs(result->u.result_011.checksum));
		}
	
	} else if (classid==53 || classid==80) {	// pots or isdn
		struct omcimsg_test_result_pots_isdn_t *result=(struct omcimsg_test_result_pots_isdn_t *)msg->msg_contents;
		util_fdprintf(fd, "pots_isdn pptp test result:\n");
		util_fdprintf(fd, "\tdroptest result=0x%x\n",result->select_test_and_mlt_drop_test_result);
		util_fdprintf(fd, "\tselftest result=0x%x\n", result->self_or_vendor_test_result);
		util_fdprintf(fd, "\tdialtone_make_break_flags=0x%x\n", result->dial_tone_make_break_flags);
		util_fdprintf(fd, "\tdialtone_power_flags=0x%x\n", result->dial_tone_power_flags);
		util_fdprintf(fd, "\tlooptest_dc_voltage_flags=0x%x\n", result->loop_test_dc_voltage_flags);
		util_fdprintf(fd, "\tlooptest_ac_voltage_flags=0x%x\n", result->loop_test_ac_voltage_flags);
		util_fdprintf(fd, "\tlooptest_resistance_flags_1=0x%x\n", result->loop_test_resistance_flags_1);
		util_fdprintf(fd, "\tlooptest_resistance_flags_2=0x%x\n", result->loop_test_resistance_flags_2);
		util_fdprintf(fd, "\ttime_to_draw_dial_tone=%u (0.1s)\n", result->time_to_draw_dial_tone);
		util_fdprintf(fd, "\ttime_to_break_dial_tone=%u (0.1s)\n", result->time_to_break_dial_tone);
		util_fdprintf(fd, "\ttotal_dial_tone_power_measure=%u (0.1db)\n", result->total_dial_tone_power_measurement);
		util_fdprintf(fd, "\tquiet_channel_power_measurement=%u (db)\n", result->quiet_channel_power_measurement);
		util_fdprintf(fd, "\ttip_ground_dc_v=%d (v)\n", ntohs(result->tip_ground_dc_voltage));
		util_fdprintf(fd, "\tring_ground_dc_v=%d (v)\n", ntohs(result->ring_ground_dc_voltage));
		util_fdprintf(fd, "\ttip_ground_ac_v=%d (vrms)\n", result->tip_ground_ac_voltage);
		util_fdprintf(fd, "\tring_ground_ac_v=%d (vrms)\n", result->ring_ground_ac_voltage);
		util_fdprintf(fd, "\ttip_ground_dc_r=%d (kohm)\n", ntohs(result->tip_ground_dc_resistance));
		util_fdprintf(fd, "\tring_ground_dc_r=%d (kohm)\n", ntohs(result->ring_ground_dc_resistance));
		util_fdprintf(fd, "\ttip_ring_dc_r=%d (kohm)\n", ntohs(result->tip_ring_dc_resistance));
		util_fdprintf(fd, "\tringer_equivalence=%d (0.1REN)\n", result->ringer_equivalence);
		util_fdprintf(fd, "\tgeneral_purpose_buffer_meid=0x%x(%d)\n", 
			ntohs(result->pointer_to_a_general_purpose_buffer_me), ntohs(result->pointer_to_a_general_purpose_buffer_me));
		util_fdprintf(fd, "\tloop_tip_ring_test_ac_dc_voltage_flags=%d (0.1REN)\n", result->loop_tip_ring_test_ac_dc_voltage_flags);
		util_fdprintf(fd, "\ttip_ring_ac_voltage=%d (0.1REN)\n", result->tip_ring_ac_voltage);
		util_fdprintf(fd, "\ttip_ring_dc_voltage=%d (kohm)\n", ntohs(result->tip_ring_dc_voltage));
	}
	util_fdprintf(fd, "\n");

	return 0;
}

