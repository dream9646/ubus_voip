/*************************************
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
 * Purpose : 5VT TRAF MANAGER
 * Module  : switch_hw
 * File    : cpuport.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "cpuport_hw_trex.h"

static int pkt_recvtask_loop_g = 1;
static int recv_total = 0;

int
cpuport_recv_func(void)
{
	struct cpuport_info_t *cpuport_info;
	int recv_avail = 0;
	int ret = 0;

	while(pkt_recvtask_loop_g)
	{
		recv_avail = cpuport_hw_fvt2510_frame_recv_is_available (NULL); 
		if ( recv_avail > 0) 
		{
			int i;
			
			for( i = 0; i < recv_avail ;i++)
			{
				// receive packets
				cpuport_info = (struct cpuport_info_t *)malloc(sizeof(struct cpuport_info_t));
				memset(cpuport_info, 0, sizeof(struct cpuport_info_t));
				if (cpuport_info == NULL) {
					fprintf(stderr, "fail to malloc cpuport_info\n"); 
					continue;
				}
				cpuport_info->buf_ptr = (unsigned char *)malloc(2*CPUPORT_VTAG_LEN + CPUPORT_BUF_SIZE);
				memset(cpuport_info->buf_ptr, 0, 2*CPUPORT_VTAG_LEN + CPUPORT_BUF_SIZE);
				if (cpuport_info->buf_ptr == NULL) {
					fprintf(stderr, "fail to malloc cpuport_info->buf_ptr\n"); 
					free(cpuport_info);
					continue;
				}
				// we preserve 2*CPUPORT_VTAG_LEN before frame_ptr for future header change
				cpuport_info->frame_ptr = cpuport_info->buf_ptr + 2*CPUPORT_VTAG_LEN;
				cpuport_info->buf_len = CPUPORT_BUF_SIZE; 
				ret = cpuport_hw_fvt2510_frame_recv(cpuport_info, CPUPORT_BUF_SIZE); 

				if ( ret == 0 ) { // no pkt found in all socket, quit recv_avail loop
					free(cpuport_info->buf_ptr);
					free(cpuport_info);
					fprintf(stderr, "no pkt found in all socket, quit recv_avail loop\n"); 
					break;
					
				} else if ( ret < 0 ) { // error happend in specific socket
					free(cpuport_info->buf_ptr);
					free(cpuport_info);
					fprintf(stderr, "error happend in specific socket\n"); 

				} else if (ret == 1) {	// pkt found in specific socket
					unsigned char *m = cpuport_info->frame_ptr;
					
					recv_total++;

					fprintf(stderr, "-----------------------------------------------------------------\n");
					fprintf(stderr, "recv pkt %d, len %d from logical port %d\n", recv_total, cpuport_info->frame_len, cpuport_info->rx_desc.logical_port_id);
					fprintf(stderr, "%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n", 
						m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11],m[12],m[13],m[14],m[15]);
					fprintf(stderr, "-----------------------------------------------------------------\n");
					// uni0..4: logical port 0..4, pon/wan: logical port 5, cpu: logical port 6

					// bridging pkt between uni0 & pon
					if (cpuport_info->rx_desc.logical_port_id == 5) {
						cpuport_info->tx_desc.logical_port_id = 0;
					} else if (cpuport_info->rx_desc.logical_port_id == 0) {
						cpuport_info->tx_desc.logical_port_id = 5;
					}					
					cpuport_hw_fvt2510_frame_send(cpuport_info);
						
					free(cpuport_info->buf_ptr);
					free(cpuport_info);
				} else {	
					free(cpuport_info->buf_ptr);
					free(cpuport_info);										
					fprintf(stderr,  "ret=%d?\n", ret); 
				}
			}	// end of recv_avail loop
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	debug_level_cpuport = LOG_NOTICE;  
	
	cpuport_hw_fvt2510_init(TRAP_IFNAME);
	cpuport_recv_func();
	cpuport_hw_fvt2510_shutdown();	
	return 0;
}
