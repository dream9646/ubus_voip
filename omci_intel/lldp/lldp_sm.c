/******************************************************************
 *
 * Copyright (C) 2017 5V Technologies Ltd.
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
 * File    : lldp_sm.c
 *
 ******************************************************************/
/* LLDP state machine refer to 802.1AB-2009 */
/* 9.2.8 Transmit state machine             */
/* 9.2.9 Receive state machine              */
/* 9.2.10 Transmit timer state machine      */

#include "util.h"
#include <string.h>
#include "lldp_core.h"
#include "env.h"

extern lldp_parm_t lldp_parm[];

static void SIGNAL_TX(lldp_sm_t *sm)
{
	sm->txNow = TRUE;
	sm->localChange = FALSE;
	if (sm->txFast > 0) {
		sm->txTTR = omci_env_g.lldp_fast_tx;
	} else {
		sm->txTTR = omci_env_g.lldp_send_interval;
	}
}

static void TX_TIMER_EXPIRES(lldp_sm_t *sm)
{
	if (sm->txFast > 0) sm->txFast--;
	SIGNAL_TX(sm);
}

static void TX_FAST_START(lldp_sm_t *sm)
{
	sm->newNeighbor = FALSE;
	if (sm->txFast == 0) sm->txFast = omci_env_g.lldp_tx_fast_init;
	TX_TIMER_EXPIRES(sm);
}

int lldp_tx_sm(int port_id)
{
	lldp_sm_t *sm = &lldp_parm[port_id].sm;

	if (lldp_parm[port_id].mib.adminStatus == LLDP_RXTX || lldp_parm[port_id].mib.adminStatus == LLDP_TXONLY) {
		if(omci_env_g.debug_level_lldp == LOG_DEBUG || sm->is_shutdown == TRUE || sm->txNow == TRUE)
			dbprintf_lldp(LOG_INFO, "port[%d] is_shutdown[%d] txNow[%d] txCredit[%d]\n",  port_id, sm->is_shutdown, sm->txNow, sm->txCredit);
	}

	if (sm->is_shutdown == TRUE) {
		// send shutdownLLDPDU
		lldp_send_pkt_process(port_id);
		sm->is_shutdown = FALSE;
	} else if (sm->txNow == TRUE && sm->txCredit > 0) {
		// send lldp
		lldp_send_pkt_process(port_id);
		if (sm->txCredit > 0) sm->txCredit--;
		sm->txNow = FALSE;
	}

	return 0;
}

static void DELETE_INFO(int port_id)
{
	extern lldp_pkt_t neighbor[];
	/* remove remote MIB */
	memset(lldp_parm[port_id].mib.rx_src_mac, 0, IFHWADDRLEN);
	memset(&neighbor[port_id], 0, sizeof(lldp_pkt_t));
	lldp_parm[port_id].sm.neighbor_couter = 0;
}

static void RX_FRAME(int port_id)
{
	extern lldp_pkt_t neighbor[];
	lldp_mib_t *mib = &lldp_parm[port_id].mib;
	lldp_pkt_t* np = &neighbor[port_id];

	/* check newNeighbor */
	if (memcmp(np->src_mac, mib->rx_src_mac, IFHWADDRLEN) != 0) {
		lldp_parm[port_id].sm.newNeighbor = TRUE;
		lldp_parm[port_id].sm.neighbor_couter = 1;
		memcpy(mib->rx_src_mac, np->src_mac, IFHWADDRLEN);
	}
	mib->rxTTL = np->ttl.ttl;
	if (mib->rxTTL == 0) DELETE_INFO(port_id);
	lldp_parm[port_id].sm.rcvFrame = FALSE;
}

int lldp_rx_sm(int port_id)
{
	lldp_sm_t *sm = &lldp_parm[port_id].sm;

	if (lldp_parm[port_id].mib.adminStatus == LLDP_RXTX || lldp_parm[port_id].mib.adminStatus == LLDP_RXONLY) {
		if(omci_env_g.debug_level_lldp == LOG_DEBUG || sm->rcvFrame == TRUE || sm->rxInfoAge == TRUE)
			dbprintf_lldp(LOG_INFO, "port[%d] rcvFrame[%d] rxInfoAge[%d]\n",  port_id, sm->rcvFrame, sm->rxInfoAge);
	}

	if (sm->rcvFrame == TRUE) {
		RX_FRAME(port_id);
	} else if (sm->rxInfoAge == TRUE) {
		DELETE_INFO(port_id);
	}

	sm->rxInfoAge = FALSE;
	return 0;
}

void lldp_tx_timer_sm_init(lldp_sm_t *sm)
{
	sm->is_shutdown = FALSE;
	sm->txNow = FALSE;
	sm->txCredit = omci_env_g.lldp_credit_max;
	sm->newNeighbor = FALSE;
	sm->txTTR = 0;
	sm->txFast = 0;
	sm->localChange = FALSE;
	sm->is_tx_timer_init_state = TRUE;
}

int lldp_tx_timer_sm(int port_id)
{
	lldp_sm_t *sm = &lldp_parm[port_id].sm;

	if (lldp_parm[port_id].mib.adminStatus == LLDP_RXTX || lldp_parm[port_id].mib.adminStatus == LLDP_TXONLY) {

		if (sm->txTTR > 0) sm->txTTR--;

		if(omci_env_g.debug_level_lldp == LOG_DEBUG || sm->newNeighbor == TRUE || sm->txTTR == 0 || sm->localChange == TRUE)
			dbprintf_lldp(LOG_INFO, "port[%d] newNeighbor[%d]  txTTR[%d] localChange[%d]\n",  port_id, sm->newNeighbor, sm->txTTR, sm->localChange);

		/* TX_TICK */
		if (sm->txCredit<=omci_env_g.lldp_credit_max)
			sm->txCredit++;

		if (sm->newNeighbor == TRUE) {
			TX_FAST_START(sm);
		} else if (sm->txTTR == 0) {
			TX_TIMER_EXPIRES(sm);
		} else if (sm->localChange == TRUE) {
			SIGNAL_TX(sm);
		}

		sm->is_tx_timer_init_state = FALSE;
	} else {
		/* TX_TIMER_INITIALIZE */
		if (sm->is_tx_timer_init_state == FALSE && sm->is_shutdown == FALSE)
			lldp_tx_timer_sm_init(sm);
	}
	return 0;
}

