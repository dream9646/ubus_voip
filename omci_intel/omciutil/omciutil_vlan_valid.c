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
 * Module  : omciutil
 * File    : omciutil_vlan_valid.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "me_related.h"

static int
omciutil_vlan_is_valid_filter_vid(unsigned short vid)
{
	if (vid<=4096)	// 0..4095, 4096(don't care)
		return 1;
	return 0;
}

static int
omciutil_vlan_is_valid_filter_tpid_de(unsigned short tpid)
{
	if (tpid==0 ||tpid==4 ||tpid==5 ||tpid==6 ||tpid==7)
		return 1;
	if (tpid==OMCIUTIL_VLAN_FILTER_TPID_DE_1000 ||
	    tpid==OMCIUTIL_VLAN_FILTER_TPID_DE_1001 ||
	    tpid==OMCIUTIL_VLAN_FILTER_TPID_DE_1010 ||
	    tpid==OMCIUTIL_VLAN_FILTER_TPID_DE_1011 ||
	    tpid==OMCIUTIL_VLAN_FILTER_TPID_DE_1100)
	    	return 1;
	return 0;
}

int
omciutil_vlan_is_valid_treat_priority(unsigned char priority, int rule_tag_num)
{
	if (priority<=7) {	//0..7
		return 1;
	} else if (priority==8) {	// ipri
		return (rule_tag_num>=1)?1:0;
	} else if (priority==9) {	// opri
		return (rule_tag_num>=2)?1:0;		
	} else if (priority==10) {	// dscp map
		return 1;
	} else if (priority==15) {	// do not add a tag
		return 1;
	} else {
		return 0;
	}
}

int
omciutil_vlan_is_valid_treat_vid(unsigned short vid, int rule_tag_num)
{
	if (vid<=4095) {
		return 1;
	} else if (vid==4096) {	// ivid
		return (rule_tag_num>=1)?1:0;
	} else if (vid==4097) {	// ovid
		return (rule_tag_num>=2)?1:0;
	} else {
		return 0;
	}
}

static int
omciutil_vlan_is_valid_treat_tpid_de(unsigned char tpid_de, int rule_tag_num)
{
	switch(tpid_de) {
	case 0:	return (rule_tag_num>=1)?1:0;	// tpid_de=itpid_de
	case 1: return (rule_tag_num>=2)?1:0;	// tpid_de=otpid_de
	case 2:	return (rule_tag_num>=1)?1:0;	// tpid=output_tpid,de=itag_de
	case 3:	return (rule_tag_num>=2)?1:0;	// tpid=output_tpid,de=otag_de
	case 4:	return 1;			// 0x8100
	case 6:	return 1;			// tpid=output_tpid,de=0
	case 7:	return 1;			// tpid=output_tpid,de=1
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000:	// tpid=input_tpid,de=0
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001:	// tpid=input_tpid,de=1
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:	// tpid=0x8100,de=0
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:	// tpid=0x8100,de=1
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100:	// tpid=input_tpid,de=itag_de?
		return (rule_tag_num>=1)?1:0;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101:	// tpid=input_tpid,de=otag_de?
		return (rule_tag_num>=2)?1:0;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110:	// tpid=0x88a8,de=0
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111:	// tpid=0x88a8,de=0
		return 1;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000:	// tpid=copy_inner,de=0
		return (rule_tag_num>=1)?1:0;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001:	// tpid=copy_inner,de=1
		return (rule_tag_num>=1)?1:0;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010:	// tpid=copy_outer,de=0
		return (rule_tag_num>=2)?1:0;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011:	// tpid=copy_outer,de=1
		return (rule_tag_num>=2)?1:0;
	default:
		return 0;
	}
}

int
omciutil_vlan_is_valid_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int rule_tag_num;

	if (rule_fields==NULL)
		return 0;		

	rule_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields);
	if (rule_tag_num<0)
		return 0;
	
	// match outer
	if (rule_tag_num>=2) {
		if (!omciutil_vlan_is_valid_filter_vid(rule_fields->filter_outer.vid) ||
		    !omciutil_vlan_is_valid_filter_tpid_de(rule_fields->filter_outer.tpid_de))
		    	return 0;
	}
	// match inner
	if (rule_tag_num>=1) {
		if (!omciutil_vlan_is_valid_filter_vid(rule_fields->filter_inner.vid) ||
		    !omciutil_vlan_is_valid_filter_tpid_de(rule_fields->filter_inner.tpid_de))
		    	return 0;
	}

	// match ether type
	if (rule_fields->filter_ethertype>OMCIUTIL_VLAN_FILTER_ET_ARP &&
		rule_fields->filter_ethertype != OMCIUTIL_VLAN_FILTER_ET_IP_ARP &&
		rule_fields->filter_ethertype != OMCIUTIL_VLAN_FILTER_ET_IPV6 &&
		rule_fields->filter_ethertype != OMCIUTIL_VLAN_FILTER_ET_DEFAULT)
		return 0;

	// treatment
	if (rule_fields->treatment_tag_to_remove>3)
		return 0;
	if (rule_fields->treatment_tag_to_remove<=2) {
		if (rule_fields->treatment_tag_to_remove>rule_tag_num)
			return 0;	
		if (!omciutil_vlan_is_valid_treat_priority(rule_fields->treatment_outer.priority, rule_tag_num) ||
		    !omciutil_vlan_is_valid_treat_priority(rule_fields->treatment_inner.priority, rule_tag_num)) 
		    	return 0;
		//treatment outer
		if (rule_fields->treatment_outer.priority!=15) {
			if (!omciutil_vlan_is_valid_treat_vid(rule_fields->treatment_outer.vid, rule_tag_num) ||
			    !omciutil_vlan_is_valid_treat_tpid_de(rule_fields->treatment_outer.tpid_de, rule_tag_num)) 
			    	return 0;
		}
		//treatment inner
		if (rule_fields->treatment_inner.priority!=15) {
			if (!omciutil_vlan_is_valid_treat_vid(rule_fields->treatment_inner.vid, rule_tag_num) ||
			    !omciutil_vlan_is_valid_treat_tpid_de(rule_fields->treatment_inner.tpid_de, rule_tag_num))
			    	return 0;
		}
	}
	return 1;
}
