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
 * Module  : gpon_hw_prx126
 * File    : gpon_hw_prx126_conv.c
 *
 ******************************************************************/

#include "util.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"

static int
scheduler_queue_map(unsigned int sch_id, unsigned int *qmap)
{
	if (sch_id <= CHIP_SCHID_MAX) {
#if 0	// edwin, need modify later
		reg_array_read(CHIP_PON_SCH_QMAPr, REG_ARRAY_INDEX_NONE, sch_id, qmap);
#endif
		return 0;
	}
	dbprintf(LOG_ERR, "schid(%d) should <=%d\n", sch_id, CHIP_SCHID_MAX);
	return -1;
}

int
gpon_hw_prx126_conv_pqid_seq_to_private_clear(unsigned seq_pqid, unsigned int *private_pqid)
{
	unsigned int group, i;

	*private_pqid = seq_pqid % GPON_PQ_ID_PER_GROUP;		// private_pqid is 0..31

	group = seq_pqid / GPON_PQ_ID_PER_GROUP; 	// group is 0..3
	for (i=0; i< GPON_TCONT_ID_PER_GROUP; i++) {
		unsigned int sch_id = group * GPON_TCONT_ID_PER_GROUP + i;
		unsigned int qmap;
		if (sch_id >= GPON_TCONT_ID_TOTAL)
			break;
		if (scheduler_queue_map(sch_id, &qmap) <0) {
			dbprintf(LOG_ERR, "seq_pqid=%d, GPON_TCONT_ID_PER_GROUP=%d, GPON_PQ_ID_PER_GROUP=%d, group=%d, sch_id=%d err?\n",
				seq_pqid, GPON_TCONT_ID_PER_GROUP, GPON_PQ_ID_PER_GROUP, group, sch_id);
			continue;
		}
		if (qmap & (1<< (*private_pqid))) {	// find the sch_id owns the pq
#if 0	// edwin, need modify later
			rtk_ponmac_queue_t queue = { .queueId = *private_pqid, .schedulerId = sch_id, };
			rtk_ponmac_queue_del(&queue);
#endif
		}
	}
	return 0;
}

int
gpon_hw_prx126_conv_pqid_seq_to_private(unsigned seq_pqid, unsigned int *private_pqid, unsigned int *tcont_id)
{
	unsigned int group, i;

	*private_pqid = seq_pqid % GPON_PQ_ID_PER_GROUP;		// private_pqid is 0..31

	group = seq_pqid / GPON_PQ_ID_PER_GROUP; 	// group is 0..3
	for (i=0; i< GPON_TCONT_ID_PER_GROUP; i++) {
		unsigned int sch_id = group * GPON_TCONT_ID_PER_GROUP + i;
		unsigned int qmap;
		if (sch_id >= GPON_TCONT_ID_TOTAL)
			break;
		if (scheduler_queue_map(sch_id, &qmap) <0) {
			dbprintf(LOG_ERR, "seq_pqid=%d, GPON_TCONT_ID_PER_GROUP=%d, GPON_PQ_ID_PER_GROUP=%d, group=%d, sch_id=%d err?\n",
				seq_pqid, GPON_TCONT_ID_PER_GROUP, GPON_PQ_ID_PER_GROUP, group, sch_id);
			continue;
		}
		if (qmap & (1<< (*private_pqid))) {	// find the sch_id owns the pq
			*tcont_id = sch_id;
			return 0;
		}
	}
	// last chance guess
	if (seq_pqid == GPON_OMCI_PQ_ID) {
		*tcont_id = GPON_OMCI_TCONT_ID;
		return 0;
	}
	return -1;
}

int
gpon_hw_prx126_conv_pqid_private_to_seq(unsigned int tcont_id, unsigned int private_pqid, unsigned int *seq_pqid)
{
	unsigned int group = tcont_id/GPON_TCONT_ID_PER_GROUP;
	*seq_pqid = group * GPON_PQ_ID_PER_GROUP + private_pqid;
	return 0;
}

