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
 * Module  : voip_hw
 * File    : voip_hw_util.c
 *
 ******************************************************************/
#include <string.h>

#include "util.h"
#include "voip_hw.h"

int voip_hw_voice_service_profile_release_table(struct voip_hw_voice_service_profile_t *vvsp)
{
	struct voip_hw_tone_pattern_entry_t *tp_entry_pos, *tp_entry_n;
	struct voip_hw_tone_event_entry_t *te_entry_pos, *te_entry_n;
	struct voip_hw_ringing_pattern_entry_t *rp_entry_pos, *rp_entry_n;
	struct voip_hw_ringing_event_entry_t *re_entry_pos, *re_entry_n;

	if (vvsp == NULL)
	{
		return -1;
	}

	//tone pattern table
	list_for_each_entry_safe(tp_entry_pos, tp_entry_n, &vvsp->tone_pattern_list, tone_pattern_node)
	{
		list_del(&tp_entry_pos->tone_pattern_node);		
		free_safe(tp_entry_pos);
	}

	//tone event table
	list_for_each_entry_safe(te_entry_pos, te_entry_n, &vvsp->tone_event_list, tone_event_node)
	{
		if (te_entry_pos->tone_file != NULL)
		{
			free_safe(te_entry_pos->tone_file);
		}
		list_del(&te_entry_pos->tone_event_node);		
		free_safe(te_entry_pos);
	}

	//ringing pattern table
	list_for_each_entry_safe(rp_entry_pos, rp_entry_n, &vvsp->ringing_pattern_list, ringing_pattern_node)
	{
		list_del(&rp_entry_pos->ringing_pattern_node);
		free_safe(rp_entry_pos);
	}

	//ringing event table
	list_for_each_entry_safe(re_entry_pos, re_entry_n, &vvsp->ringing_event_list, ringing_event_node)
	{
		if (re_entry_pos->ringing_file != NULL)
		{
			free_safe(re_entry_pos->ringing_file);
		}
		if (re_entry_pos->ringing_text != NULL)
		{
			free_safe(re_entry_pos->ringing_text);
		}
		list_del(&re_entry_pos->ringing_event_node);		
		free_safe(re_entry_pos);
	}

	return 0;
}

int voip_hw_network_dial_plan_table_release_table(struct voip_hw_network_dial_plan_table_t *ndpt)
{
	struct voip_hw_dial_plan_entry_t *dp_entry_pos, *dp_entry_n;

	if (ndpt == NULL)
	{
		return -1;
	}

	list_for_each_entry_safe(dp_entry_pos, dp_entry_n, &ndpt->dial_plan_list, dial_plan_node)
	{
		list_del(&dp_entry_pos->dial_plan_node);		
		free_safe(dp_entry_pos);
	}

	return 0;
}

int voip_hw_wri_pos_config_data_release_table(struct voip_hw_wri_pos_config_data_t *wpcd)
{
	struct voip_hw_pos_data_entry_t *pd_entry_pos, *pd_entry_n;

	if (wpcd == NULL)
	{
		return -1;
	}

	list_for_each_entry_safe(pd_entry_pos, pd_entry_n, &wpcd->pos_data_list, pos_data_node)
	{
		list_del(&pd_entry_pos->pos_data_node);
		free_safe(pd_entry_pos);
	}

	return 0;
}

int voip_hw_wri_ipt_config_data_release_table(struct voip_hw_wri_ipt_config_data_t *wicd)
{
	struct voip_hw_pos_data_entry_t *pd_entry_pos, *pd_entry_n;

	if (wicd == NULL)
	{
		return -1;
	}

	list_for_each_entry_safe(pd_entry_pos, pd_entry_n, &wicd->ipt_data_list, pos_data_node)
	{
		list_del(&pd_entry_pos->pos_data_node);		
		free_safe(pd_entry_pos);
	}

	return 0;
}

int voip_hw_sip_config_portal_release_table(struct voip_hw_sip_config_portal_t *scp)
{
	struct voip_hw_sip_config_portal_entry_t *scp_entry_pos, *scp_entry_n;

	if (scp == NULL)
	{
		return -1;
	}

	list_for_each_entry_safe(scp_entry_pos, scp_entry_n, &scp->sip_config_portal_list, sip_config_portal_node)
	{
		list_del(&scp_entry_pos->sip_config_portal_node);		
		free_safe(scp_entry_pos);
	}

	return 0;
}

int
voip_hw_util_get_codec_id (char *codec)
{
	if (codec == NULL || strlen(codec) == 0) {
		return -1;
	} else if (strcmp(codec, "PCMU")) {
		return CODEC_CLASS_ATTR_ID_PCMU;
	} else if (strcmp(codec, "PCMA")) {
		return CODEC_CLASS_ATTR_ID_PCMA;
	} else if (strcmp(codec, "G729")) {
		return CODEC_CLASS_ATTR_ID_G729;
	} else if (strcmp(codec, "G723")) {
		return CODEC_CLASS_ATTR_ID_G723;
	} else {
		dbprintf_voip(LOG_ERR, "Not support codec [%s]\n", codec);
		return -2;
	}
}
