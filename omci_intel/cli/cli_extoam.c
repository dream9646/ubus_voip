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
 * Module  : cli
 * File    : cli_extoam.c
 *
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "switch.h"
#include "cli.h"
#include "extoam.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
#include "extoam_cmd.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_loopback.h"
#include "extoam_event.h"
#include "extoam_object.h"

extern struct extoam_link_status_t cpe_link_status_g;
extern struct sw_dl_proc_t cpe_tx_g, cpe_rx_g;
extern struct loopback_t loopback_g;
extern int extoam_timer_qid_g; 

static int MAYBE_UNUSED
cli_extoam_link_status_get(int fd, struct extoam_link_status_t *extoam_link_status)
{
	util_fdprintf( fd, "DPU MAC %02x:%02x:%02x:%02x:%02x:%02x\nDPU port %d\nlink id 0x%x(%d)\nlink state %d\nlast state time stamp %d.%d\nlast reset timer id %d\n",
			extoam_link_status->dpu_mac[0],extoam_link_status->dpu_mac[1],
			extoam_link_status->dpu_mac[2],extoam_link_status->dpu_mac[3],
			extoam_link_status->dpu_mac[4],extoam_link_status->dpu_mac[5],
			extoam_link_status->dpu_port, extoam_link_status->link_id, extoam_link_status->link_id,
			extoam_link_status->link_state, extoam_link_status->last_phase.tv_sec, extoam_link_status->last_phase.tv_usec, extoam_link_status->last_reset_timer_id );
	return 0;
}

static int MAYBE_UNUSED
cli_extoam_sw_download_get(int fd, struct sw_dl_proc_t *cpe_xmit)
{
	util_fdprintf( fd, "File name: %s\nFile posistion: %d\nFile size: %d\nBlock: %d\nFinal block: %d\nTransaction id:%d (0x%x)\nCRC: 0x%x\nStart: %d.%d\nEnd: %d.%d\nOn: %d\nOn_write: %d\nFinish write: %d\nLast block length: %d\n"
			, cpe_xmit->fd?cpe_xmit->file_name:"NULL"
			, cpe_xmit->file_pos
			, cpe_xmit->size
			, cpe_xmit->block
			, cpe_xmit->final_block
			, cpe_xmit->transaction_id
			, cpe_xmit->transaction_id
			, cpe_xmit->crc
			, cpe_xmit->start.tv_sec
			, cpe_xmit->start.tv_usec
			, cpe_xmit->end.tv_sec
			, cpe_xmit->end.tv_usec
			, cpe_xmit->on
			, cpe_xmit->on_write
			, cpe_xmit->finish_write
			, cpe_xmit->last_block_len);
	return 0;
}


static int MAYBE_UNUSED
cli_extoam_loopback_start(int fd, struct extoam_link_status_t *cpe_link_status ) 
{
	extoam_loopback_local_send( cpe_link_status);
	loopback_g.timer_id = fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_LOOPBACK_CHECK	, 500, NULL);
	return 0;
}

static int MAYBE_UNUSED
extoam_extract_get(int fd)
{
	int enable = 0;
	if (switch_hw_g.extoam_extract_enable_get == NULL)
		util_fdprintf(fd, "Extraction: NULL\n");
	else {
		switch_hw_g.extoam_extract_enable_get(&enable);
		util_fdprintf(fd, "Extraction: %s\n", (enable==1) ? "Enable" : "Disable");
	}
	return 0;
}

static int MAYBE_UNUSED
cli_extoam_event_send(int fd, struct extoam_link_status_t *cpe_link_status, unsigned short event_id ) 
{
	extoam_event_send( cpe_link_status, event_id );
	return 0;
}

static int MAYBE_UNUSED
cli_extoam_object_list(int fd) 
{
	util_fdprintf(fd,
			"id\tdescription\t\t\t\t\t\tRead/Write\n"
			"0x01\tFirmware version ( CPE current firmware version)         R\n"
			"0x02\tNew firmware version                                     R\n"
			"0x03\tRegistration ID                                          R/W\n"
			"0x04\ttrap enable/disable                                      R/W\n"
			"0x05\ttemperature high warning threshold                       R/W\n"
			"0x06\ttemperature high alarm threshold                         R/W\n"
			"0x07\ttemperature low warning threshold                        R/W\n"
			"0x08\ttemperature low alarm threshold                          R/W\n"
			"0x09\tvoltage high warning threshold                           R/W\n"
			"0x0a\tvoltage high alarm threshold                             R/W\n"
			"0x0b\tvoltage low warning threshold                            R/W\n"
			"0x0c\tvoltage low alarm threshold                              R/W\n"
			"0x0d\tTx Power high warning threshold                          R/W\n"
			"0x0e\tTx Power high alarm threshold                            R/W\n"
			"0x0f\tTx Power low warning threshold                           R/W\n"
			"0x10\tTx Power low alarm threshold                             R/W\n"
			"0x11\tRx power high warning threshold                          R/W\n"
			"0x12\tRx power high alarm threshold                            R/W\n"
			"0x13\tRx power low warning threshold                           R/W\n"
			"0x14\tRx power low alarm threshold                             R/W\n"
			"0x15\tBias-current high warning                                R/W\n"
			"0x16\tbias-current high alarm                                  R/W\n"
			"0x17\tBias-current low warning                                 R/W\n"
			"0x18\tbias-current low alarm                                   R/W\n"
			"0x19\ttemperature status                                       R\n"
			"0x1a\tVoltage status                                           R\n"
			"0x1b\tBias-current status                                      R\n"
			"0x1c\tTx power status                                          R\n"
			"0x1d\tRx power status                                          R\n"
		     );
	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_extoam_long_help(int fd)
{
	util_fdprintf(fd, 
			"extoam extract [0|1]\n"
			"extoam link\n"
			"extoam sw_dl [tx|rx]\n"
			"extoam loopback start\n"
			"extoam event [event_id]\n"
			"extoam object [list | object id]\n"
			"extoam object [object id] [object value]\n"
		     );	
}

void
cli_extoam_help(int fd)
{
	util_fdprintf(fd, "extoam help|[subcmd...]\n");
}

extern struct object_handler_t object_handles[];
int 
cli_extoam_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "extoam")==0) { 
#ifndef X86 
		if (argc == 2 ) {
			if (strcmp(argv[1], "help")==0) {
				cli_extoam_long_help(fd);
				return CLI_OK;
			}else if ( strcmp(argv[1], "link") == 0) {
				cli_extoam_link_status_get(fd, &cpe_link_status_g);
				return CLI_OK;
			}else if (strcmp(argv[1], "loopback") == 0) {
				if (strcmp(argv[2], "start") == 0 ) {
					cli_extoam_loopback_start(fd, &cpe_link_status_g ); 
					return CLI_OK;
				}
			}
		}else if (argc == 3) {
			if (strcmp("extract", argv[1]) == 0) {
				if (switch_hw_g.extoam_extract_enable_set != NULL)
					switch_hw_g.extoam_extract_enable_set(util_atoi(argv[2])?1:0);
				return extoam_extract_get(fd);
			}else if (strcmp(argv[1], "sw_dl") == 0) {
				if (strcmp(argv[2], "tx") == 0) {
					cli_extoam_sw_download_get( fd, &cpe_tx_g );
				} else if (strcmp(argv[2], "rx") == 0) {
					cli_extoam_sw_download_get( fd, &cpe_rx_g );
				}
				return CLI_OK;
			} else if (strcmp(argv[1], "event") == 0) {
				unsigned short event_id = util_atoi(argv[2]);
				cli_extoam_event_send(fd, &cpe_link_status_g, event_id ); 
				return CLI_OK;
			} else if (strcmp(argv[1], "object") == 0) {
				if (strcmp(argv[2], "list") == 0) {
					cli_extoam_object_list(fd);
				}else{
					unsigned short object_id = util_atoi(argv[2]);
					unsigned char buff[1024];
					memset(buff, 0, 1024);
					unsigned short i = 0 ;
					int len = 0;
					for ( ;; ) {
						if ( object_handles[i].obj_id == 0 )
							break;
						if ( object_handles[i].obj_id == object_id ) {
							len = object_handles[i].object_get_handler(buff);
							if (len <= 0) {
								util_fdprintf(fd, "value length <= 0\n");
								break;
							}
							if ( object_handles[i].type == 0 ) { //string
								util_fdprintf(fd, "%s\n", (char *)buff);
							}else if ( object_handles[i].type == 1 ) {//int
								int value;
								value = *(int *)buff;
								util_fdprintf(fd, "%d\n", value);
							}
							break;
						}	
						i++;
					}
				}
				return CLI_OK;
			}
		} else if (argc == 4){
			unsigned short object_id = util_atoi(argv[2]);
			unsigned short i = 0 ;
			int ret ;
			for ( ;; ) {
				if ( object_handles[i].obj_id == 0 )
					break;
				if ( object_handles[i].obj_id == object_id ) {
					if ( object_handles[i].type == 0 ) { //string
						ret = object_handles[i].object_set_handler(strlen(argv[3]) + 1, argv[3]);
						if (ret < 0) {
							util_fdprintf(fd, "Object %d set fail!\n", object_id);
							break;
						}
					}else if ( object_handles[i].type == 1 ) {//int
						int obj_val = util_atoi(argv[3]);
						ret = object_handles[i].object_set_handler(sizeof(int), (unsigned char *)&obj_val );
						if (ret < 0) {
							util_fdprintf(fd, "Object %d set fail!\n", object_id);
							break;
						}
					}
					break;
				}	
				i++;
			}
			return CLI_OK;
		}
#else
		util_fdprintf(fd, "extoam no available on X86\n");
		return CLI_OK;
#endif
	}
	return CLI_ERROR_OTHER_CATEGORY;
}
