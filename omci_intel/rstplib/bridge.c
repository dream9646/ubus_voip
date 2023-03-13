/************************************************************************ 
 * RSTP library - Rapid Spanning Tree (802.1t, 802.1w) 
 * Copyright (C) 2001-2003 Optical Access 
 * Author: Alex Rozin 
 * 
 * This file is part of RSTP library. 
 * 
 * RSTP library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; version 2.1 
 * 
 * RSTP library is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with RSTP library; see the file COPYING.  If not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 * 02111-1307, USA. 
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

//#include "cli.h"
#include "uid.h"
//#include "stp_cli.h"

#include "base.h"
#include "bitmap.h"
#include "uid_stp.h"
#include "stp_in.h"

#include "meinfo.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "util.h"
#include "switch.h"
#include "bridge.h"
#include "rstp_omci.h"
#include "rstp_port.h"

//BITMAP_T    enabled_ports;
UID_SOCKET_T    uid_socket;

struct cpuport_desc_t  stp_port[MAX_STP_PORTS] ;
int number_of_ports;

int 
bridge_tx_bpdu (int port_index, unsigned char *bpdu, size_t bpdu_len)
{

	struct cpuport_info_t pktinfo;

	pktinfo.buf_ptr = malloc_safe(CPUPORT_BUF_SIZE);
	pktinfo.buf_len = CPUPORT_BUF_SIZE;
	pktinfo.tx_desc = stp_port[port_index-1];
	
	memcpy(pktinfo.buf_ptr, bpdu, bpdu_len);
	pktinfo.frame_len = bpdu_len;

	cpuport_frame_send( &pktinfo);
	free_safe(pktinfo.buf_ptr);

	return 0;
}

int bridge_start (void)
{
  BITMAP_T  ports;
  UID_STP_CFG_T uid_cfg;
  register int  iii;

  if ((number_of_ports = rstp_port_map_init(stp_port)) < 0 )
  {
	  dbprintf(LOG_ERR,"stp init err\n");
	  return -1;
  }

//dbprintf(LOG_ERR, "number_of_ports: %d\n",number_of_ports);
  STP_IN_init (number_of_ports);
//  BitmapClear(&enabled_ports);
  BitmapClear(&ports);
  for (iii = 1; iii <= number_of_ports; iii++) {
    BitmapSetBit(&ports, iii - 1);
  }
  
  uid_cfg.field_mask = BR_CFG_STATE;
  uid_cfg.stp_enabled = STP_ENABLED;
  iii = STP_IN_stpm_set_cfg (0, &ports, &uid_cfg);
  if (STP_OK != iii) {
    dbprintf (LOG_ERR,"FATAL: can't enable:%s\n",
               STP_IN_get_error_explanation (iii));
    return (-1);
  }
  return 0;
}

void bridge_shutdown (void)
{
  int       rc;

  rc = STP_IN_stpm_delete (0);
  if (STP_OK != rc) {
    dbprintf (LOG_ERR,"FATAL: can't delete:%s\n",
               STP_IN_get_error_explanation (rc));
  }
  rstp_port_map_deinit(stp_port);

}

int bridge_control (int port_index,
                    UID_CNTRL_BODY_T* cntrl)
{
  switch (cntrl->cmd) {
    case UID_PORT_CONNECT:
      dbprintf (LOG_ERR,"connected port p%02d\n", port_index);
 //     BitmapSetBit(&enabled_ports, port_index - 1);
      STP_IN_enable_port (port_index, True);
      break;
    case UID_PORT_DISCONNECT:
      dbprintf (LOG_ERR,"disconnected port p%02d\n", port_index);
 //   BitmapClearBit(&enabled_ports, port_index - 1);
      STP_IN_enable_port (port_index, False);
      break;
    case UID_BRIDGE_SHUTDOWN:
      dbprintf (LOG_ERR,"shutdown from manager :(\n");
      return 1;
    default:
      dbprintf (LOG_ERR,"Unknown control command <%d> for port %d\n",
              cntrl->cmd, port_index);
  }
  return 0;
}

int bridge_rx_bpdu (UID_MSG_T* msg, size_t msgsize)
{
  STP_IN_rx_bpdu (0, msg->header.destination_port,
                  (BPDU_T*) (msg->body.bpdu + sizeof (MAC_HEADER_T)),
                  msg->header.body_len - sizeof (MAC_HEADER_T));
  return 0;
}
