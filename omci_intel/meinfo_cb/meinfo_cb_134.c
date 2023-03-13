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
 * Module  : meinfo_cb
 * File    : meinfo_cb_134.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "er_group.h"
#include "omcimsg.h"
#include "util.h"
#include "pingtest.h"
#include "trtest.h"

//classid 134 9.4.12 IP_host_config_data

static int 
meinfo_cb_134_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	unsigned char zero_attr_mask[2] = {0, 0};
	// trigger meinfo_hw get to init me with hw current value
	if (miptr->hardware.get_hw)
		miptr->hardware.get_hw(me, zero_attr_mask);
	return 0;
}

/*
struct omcimsg_test_ip_host_t {
	unsigned char select_test;
	unsigned int target_ipaddr;
	unsigned char padding[27];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_134_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_ip_host_t *test_req = (struct omcimsg_test_ip_host_t *) req;
	if (test_req->select_test==1 ||		// ping test
	    test_req->select_test==2)		// traceroute
		return 1;
	if (test_req->select_test>=8 && test_req->select_test<=15)	// vendor test
		return 0;	
	return 0;
}

/*
struct omcimsg_test_result_ip_host_t {
	unsigned char test_result;
	unsigned char meaningful_bytes_total;
	union {
		struct {
			unsigned short response_delay[15];
		} __attribute__ ((packed)) result_001;
		struct {
			unsigned int neighbout_ipaddr[7];
			unsigned char padding[2];
		} __attribute__ ((packed)) result_010;
		struct {
			unsigned char type;	// icmpmsg[0]
			unsigned char code;	// icmpmsg[1]
			unsigned short checksum;	// icmpmsg[2,3]
			unsigned char icmp_payload[26];	// icmpmsg[4..29]
		} __attribute__ ((packed)) result_011;
	} u;
} __attribute__ ((packed));
*/

#define PING_MAX_PKT		15
#define TRACEROUTE_MAX_TTL	7
static int 
meinfo_cb_134_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_ip_host_t *test_req = (struct omcimsg_test_ip_host_t *) req;
	struct omcimsg_test_result_ip_host_t *test_result=(struct omcimsg_test_result_ip_host_t *)result;

	unsigned int srcip=ntohl(meinfo_util_me_attr_data(me, 4));
	unsigned int dstip=ntohl(test_req->u.target_ipaddr);

	if (test_req->select_test==1) {	// ping test
		unsigned long triptime_log[PING_MAX_PKT];
		char errbuff[64];
		int recv, i;
		
		recv=pingtest(dstip, srcip, PING_MAX_PKT, triptime_log, errbuff, 64);
		if (recv>0) {
			for (i=0; i<PING_MAX_PKT; i++) {
				if (triptime_log[i]==0xffffffff) {
					test_result->u.result_001.response_delay[i]=0xffff;
				} else {
					// triptime log unit is 0.1ms, but omci needs unit 1 ms
					test_result->u.result_001.response_delay[i]=htons((triptime_log[i]+5)/10);	
					dbprintf(LOG_WARNING, "pingtest %d:%lu\n", i, triptime_log[i]);
				}
			}
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_RESPONSE;
			test_result->meaningful_bytes_total=2*PING_MAX_PKT;
		} else if (errbuff[2]||errbuff[3]) {	// icmp chksum!=0
			memcpy((void*)&(test_result->u.result_011), errbuff, 30);
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_UNEXPECTED_RESPONSE;
			test_result->meaningful_bytes_total=30;
		} else {
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_TIMEOUT;
			test_result->meaningful_bytes_total=0;
		}	
		return 0;

	} else if (test_req->select_test==2) { // traceroute test
		unsigned int hostlog[TRACEROUTE_MAX_TTL];
		char errbuff[64];
		int nhost=trtest(dstip, srcip, 0, 0, TRACEROUTE_MAX_TTL, 3, 0, hostlog, errbuff, 64);
		int i;
		
		if (nhost>0) {
			for (i=0; i<nhost; i++) {
				test_result->u.result_010.neighbour_ipaddr[i]=htonl(hostlog[i]);
			}
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_TIME_EXCEEDED;
			test_result->meaningful_bytes_total=4*nhost;
		} else if (errbuff[2]||errbuff[3]) {	// icmp chksum!=0
			memcpy((void*)&(test_result->u.result_011), errbuff, 30);
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_UNEXPECTED_RESPONSE;
			test_result->meaningful_bytes_total=30;
		} else {
			test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_TIMEOUT;
			test_result->meaningful_bytes_total=0;
		}	
		return 0;
	}
	test_result->test_result=OMCIMSG_TEST_RESULT_IP_HOST_UNEXPECTED_RESPONSE;
	test_result->meaningful_bytes_total=0;
	return 0;
}

struct meinfo_callback_t meinfo_cb_134 = {
	.create_cb		= meinfo_cb_134_create,
	.test_is_supported_cb	= meinfo_cb_134_test_is_supported,
	.test_cb		= meinfo_cb_134_test,
};

