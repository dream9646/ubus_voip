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
 * Module  : meinfo
 * File    : meinfo_me_attr.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
                                          
#include "meinfo.h"
#include "meinfo_cb.h"
#include "notify_chain.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "crc.h"

// attr get
int
meinfo_me_attr_get(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_DEBUG, // slin - LOG_ERR,  
			"%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		attr->data = me->attr[attr_order].value.data;
		break;
	case ATTR_FORMAT_BIT_FIELD:	//not sure
	case ATTR_FORMAT_STRING:
		if (me->attr[attr_order].value.ptr != NULL) {
			if (attr->ptr == NULL)
				attr->ptr = malloc_safe(miptr->attrinfo[attr_order].byte_size);
			memcpy(attr->ptr, me->attr[attr_order].value.ptr,
			       miptr->attrinfo[attr_order].byte_size);
		}
		break;
	default:		// table?
		return MEINFO_ERROR_ATTR_INVALID_ACTION_ON_TABLE;
	}

	// call notifychain issue() for event getattr
	// notify chain do not implemnt this func yet

	return 0;
}

#if 0
// pm me attr get
int
meinfo_me_attr_get_pm(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//init value
	attr->data = 0;

	//return the history(old) attr value from mib
	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2)
		{
			attr->data = me->attr[attr_order].value_old.data; //history data
		} else {
			attr->data = me->attr[attr_order].value.data; //Interval end time, or Threshold data 1/2 id.
		}
		break;
	default:
		dbprintf(LOG_ERR,
			"unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
			me->classid, me->meid, attr_order);

		return MEINFO_ERROR_ATTR_INVALID_ACTION_ON_TABLE;
	}

	return MEINFO_RW_OK;
}
#endif

// pm me attr get
int
meinfo_me_attr_get_pm_current_data(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//init value
	attr->data = 0;

	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2)
		{
			attr->data = me->attr[attr_order].value_current_data.data; //current data
		} else {
			attr->data = me->attr[attr_order].value.data; //Interval end time, or Threshold data 1/2 id.
		}
		break;
	default:
		dbprintf(LOG_ERR,
				"unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
				me->classid, me->meid, attr_order);
	}

	return MEINFO_RW_OK;
}

// pm me attr get accum
int
meinfo_me_attr_get_pm_hwprev(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//init value
	attr->data = 0;

	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2) {
			attr->data = me->attr[attr_order].value_hwprev.data;
		} else {
			attr->data = me->attr[attr_order].value.data; //Interval end time, or Threshold data 1/2 id. should not happen
		}
		break;
	default:
		dbprintf(LOG_ERR,
				"unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
				me->classid, me->meid, attr_order);
	}

	return MEINFO_RW_OK;
}

// attr set
int
meinfo_me_attr_set(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr, int check_avc)
{
	struct meinfo_t *miptr;
	int value_is_changed=0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//set the attr value to mib
	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
			if (me->attr[attr_order].value.data != attr->data)
				value_is_changed=1;
		}
		me->attr[attr_order].value.data = attr->data;
		break;
	case ATTR_FORMAT_BIT_FIELD:	//not sure
		if (me->attr[attr_order].value.ptr == NULL)
			me->attr[attr_order].value.ptr = malloc_safe(miptr->attrinfo[attr_order].byte_size);
		if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
			if (strncmp(me->attr[attr_order].value.ptr, attr->ptr, miptr->attrinfo[attr_order].byte_size)!=0)
				value_is_changed=1;
		}
		memcpy(me->attr[attr_order].value.ptr, attr->ptr, miptr->attrinfo[attr_order].byte_size);
		break;
	case ATTR_FORMAT_STRING:
		if (me->attr[attr_order].value.ptr == NULL)
			me->attr[attr_order].value.ptr = malloc_safe(miptr->attrinfo[attr_order].byte_size);
		if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
			if (strncmp(me->attr[attr_order].value.ptr, attr->ptr, miptr->attrinfo[attr_order].byte_size)!=0)
				value_is_changed=1;
		}
		memcpy(me->attr[attr_order].value.ptr, attr->ptr, miptr->attrinfo[attr_order].byte_size);
		break;
	case ATTR_FORMAT_TABLE:
		meinfo_me_attr_set_table_entry(me, attr_order, attr->ptr, check_avc);
		break;
	default:
		return MEINFO_ERROR_ATTR_FORMAT_UNKNOWN;
	}

	if (value_is_changed) {
		static int is_keep_pkt_cnt_script_exist=-1;
		util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
		dbprintf(LOG_INFO, "classid=%u, meid=%u, attr_order=%d, avc changed mask set 0x%02x%02x\n", 
			me->classid, me->meid, attr_order, me->avc_changed_mask[0], me->avc_changed_mask[1]);

		if ( me->classid == 11 && attr_order== 6 ) {
			//means link status change
			if ( is_keep_pkt_cnt_script_exist == -1 )
				is_keep_pkt_cnt_script_exist=util_file_is_available("/etc/init.d/keep_pkt_cnt_for_unlink_port.sh");

			if ( is_keep_pkt_cnt_script_exist == 1 )
				util_run_by_thread("/etc/init.d/keep_pkt_cnt_for_unlink_port.sh", 0);
		}
	}

	// call notifychain issue() for event setattr
	// notify chain do not implemnt this func yet

	return 0;
}

// pm me attrinfo accumulate get
int
meinfo_me_attr_get_pm_is_accumulate(struct me_t *me, unsigned char attr_order, unsigned short *value)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	*value=util_attr_mask_get_bit(me->pm_is_accumulated_mask, attr_order);

	return MEINFO_RW_OK;
}

// pm me attr swap
int
meinfo_me_attr_swap_pm_data(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//set the attr value to mib
	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2)	//swap data: his=curr, curr=0
		{
			me->attr[attr_order].value.data = me->attr[attr_order].value_current_data.data;
			me->attr[attr_order].value_current_data.data = 0;
		}
		break;
	default:
		dbprintf(LOG_ERR, "unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
				me->classid, me->meid, attr_order);
	}
	return MEINFO_RW_OK;
}

// pm me attr set
int
meinfo_me_attr_set_pm_current_data(struct me_t *me, unsigned char attr_order, struct attr_value_t attr)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//set the attr value to mib
	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2)
		{
			me->attr[attr_order].value_current_data.data = attr.data;
		} else {
			me->attr[attr_order].value.data = attr.data;
		}
		break;
	default:
		dbprintf(LOG_ERR, "unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
				me->classid, me->meid, attr_order);
	}

	// call notifychain issue() for event setattr
	// notify chain do not implemnt this func yet

	return MEINFO_RW_OK;
}

// pm me attr accum set
int
meinfo_me_attr_set_pm_hwprev(struct me_t *me, unsigned char attr_order, struct attr_value_t attr)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	//check for pm me
	if (!(meinfo_util_get_action(me->classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
	{
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME);
	}

	//set the attr value to mib
	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (attr_order > 2)
		{
			me->attr[attr_order].value_hwprev.data = attr.data;
		} else {
			me->attr[attr_order].value.data = attr.data;
		}
		break;
	default:
		dbprintf(LOG_ERR, "unexpected attr format, classid=%u, meid=%u, attr_order=%u\n",
				me->classid, me->meid, attr_order);
	}

	// call notifychain issue() for event setattr
	// notify chain do not implemnt this func yet

	return MEINFO_RW_OK;
}

// pm me attr update pm
int
meinfo_me_attr_update_pm(struct me_t *me, unsigned char attr_order, unsigned long long hw_val)
{
	struct attr_value_t attr_current, attr_hwprev;
	unsigned short is_accum;
	struct attr_value_t attr;
        
	memset(&attr, 0x0, sizeof(struct attr_value_t));
	memset(&attr_current, 0x0, sizeof(struct attr_value_t));
	memset(&attr_hwprev, 0x0, sizeof(struct attr_value_t));

	//get current data;
	meinfo_me_attr_get_pm_current_data(me, attr_order, &attr_current);
	meinfo_me_attr_get_pm_hwprev(me, attr_order, &attr_hwprev);
	meinfo_me_attr_get_pm_is_accumulate(me, attr_order, &is_accum);

	if(is_accum == 1) {
		if(hw_val < attr_hwprev.data) // if hw_val is reset, also reset hwprev counter
			attr_hwprev.data = me->attr[attr_order].value_hwprev.data = 0;
		attr.data = attr_current.data + util_ull_value_diff(hw_val, attr_hwprev.data);
		attr_hwprev.data=hw_val;
		meinfo_me_attr_set_pm_hwprev(me, attr_order, attr_hwprev);
	} else {
		attr.data = hw_val + attr_current.data;
	}
	meinfo_me_attr_set_pm_current_data(me, attr_order, attr);

	return 0;
}

// pm me attr clear pm
int
meinfo_me_attr_clear_pm(struct me_t *me, unsigned char attr_order)
{
	me->attr[attr_order].value_current_data.data = 0;
	me->attr[attr_order].value_hwprev.data = 0;
	me->attr[attr_order].value.data = 0;
	
	return 0;
}

// attr clear some data when time sync for pm
int 
meinfo_me_attr_time_sync_for_pm(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	unsigned short is_accum;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	me->attr[attr_order].value.data = 0;
	if (me->attr[attr_order].value.ptr != 0)
	{
		free_safe(me->attr[attr_order].value.ptr);
	}

	me->attr[attr_order].value_current_data.data = 0;
	if (me->attr[attr_order].value_current_data.ptr != 0)
	{
		free_safe(me->attr[attr_order].value_current_data.ptr);
		me->attr[attr_order].value_current_data.ptr = NULL;
	}

	meinfo_me_attr_get_pm_is_accumulate(me, attr_order, &is_accum);
	if(is_accum == 0) {
		me->attr[attr_order].value_hwprev.data = 0;

		if (me->attr[attr_order].value_hwprev.ptr != 0)
		{
			free_safe(me->attr[attr_order].value_hwprev.ptr);
			me->attr[attr_order].value_hwprev.ptr = NULL;
		}
	}
	memset(me->alarm_result_mask, 0x00, 28);

	return 0;
}

// pm me attrinfo accumulate set
int
meinfo_me_attr_set_pm_is_accumulate(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;

	if (me  == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order <= 2 || attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	util_attr_mask_set_bit(me->pm_is_accumulated_mask, attr_order);

	return MEINFO_RW_OK;
}

// attr table entry get
int
meinfo_me_attr_get_table_entry(struct me_t *me, unsigned char attr_order, 
				struct get_attr_table_context_t *context, void *entrydata)
{
	struct meinfo_t *miptr;
	struct attrinfo_table_t *table_ptr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR,"%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if ((table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	context->table_entry = NULL;
	context->sequence = 0;
	
	return meinfo_me_attr_get_table_entry_next(me, attr_order, context, entrydata);
}

// attr table entry get next
// before use meinfo_me_attr_get_table_entry_next should always call meinfo_me_attr_get_table_entry and
// use the value context as input which assigned from meinfo_me_attr_get_table_entry
int
meinfo_me_attr_get_table_entry_next(struct me_t *me, unsigned char attr_order, 
					struct get_attr_table_context_t *context, void *entrydata)
{
	struct meinfo_t *miptr;
	struct attr_table_entry_t *table_entry = context->table_entry;
	struct attr_table_header_t *table_header;
//	unsigned short seq;
//	int ret;
//	char *entry;
	int entry_byte_size;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr;

	entry_byte_size = meinfo_util_attr_get_table_entry_byte_size(me->classid, attr_order);

	table_entry = list_prepare_entry(table_entry, &table_header->entry_list, entry_node);
	list_for_each_entry_continue(table_entry, &table_header->entry_list, entry_node) {
		if (table_entry != NULL) {
			//succee get attr table entry from sw
			memcpy(entrydata, table_entry->entry_data_ptr, entry_byte_size);
			context->table_entry = table_entry;
			return MEINFO_RW_OK;
		}
	}
	return (MEINFO_RW_ERROR_FAILED);
}

/* compare primary key of two table entry
#define MEINFO_CB_ENTRY_GREATER		1
#define MEINFO_CB_ENTRY_LESS		-1
#define MEINFO_CB_ENTRY_MATCH		0
*/
int
meinfo_me_attr_table_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry_org, unsigned char *entry_new)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	int i, current_pos = 0;
	unsigned long long value_org, value_new;
	struct attrinfo_table_t *table_ptr;
	int has_pk = 0;

	if (miptr==NULL)
		return -1;
	
	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;	
	}

	for (i = 0; i < table_ptr->field_total; i++) {
		if (table_ptr->field[i].is_primary_key) {
			has_pk = 1;
			value_org = util_bitmap_get_value(entry_org, table_ptr->entry_byte_size*8,
						current_pos, table_ptr->field[i].bit_width);
			value_new = util_bitmap_get_value(entry_new, table_ptr->entry_byte_size*8,
						current_pos, table_ptr->field[i].bit_width);
			if (value_new > value_org)
				return MEINFO_CB_ENTRY_GREATER;
			else if (value_new < value_org)
				return MEINFO_CB_ENTRY_LESS;
		}
		current_pos += table_ptr->field[i].bit_width;
	}

	if (has_pk)
		return MEINFO_CB_ENTRY_MATCH;
	else
		return MEINFO_CB_ENTRY_LESS;
}

// attr table entry set
int
meinfo_me_attr_set_table_entry(struct me_t *me, unsigned char attr_order, void *entrydata, int check_avc)
{
	struct meinfo_t *miptr;
	struct attr_table_entry_t *table_entry = NULL, *found_table_entry = NULL;
	struct attrinfo_table_t *table_ptr;
	struct attr_table_header_t *table_header;
	int (*cmp_func) (unsigned short classid, unsigned char attr_order,unsigned char *entry_org, unsigned char *entry_new);
	struct list_head *place_to_insert;
	int del_mode=MEINFO_CB_DELETE_NONE;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	if ((miptr=meinfo_util_miptr(me->classid))==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_NOT_SUPPORT;
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_ATTR_UNDEF;
	}

	if ((table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}
	if ( me->attr[attr_order].value.ptr == NULL ) {		//todo: should move to init
		me->attr[attr_order].value.ptr = malloc_safe( sizeof(struct attr_table_header_t) );
		table_header = me->attr[attr_order].value.ptr;
		INIT_LIST_HEAD(&table_header->entry_list);
		dbprintf(LOG_ERR, "This should not happen !!\n");
	} else {
		table_header = me->attr[attr_order].value.ptr;
	}

	if (miptr->callback.table_entry_cmp_cb == NULL)
		cmp_func = meinfo_me_attr_table_entry_cmp;	//use standard compare primary key function
	else
		cmp_func = miptr->callback.table_entry_cmp_cb;

	//omci_env_g.mib_rw_mode equal ENV_MIB_RW_MODE_SWHW or ENV_MIB_RW_MODE_SWSW ???

	place_to_insert=&table_header->entry_list;
	list_for_each_entry(table_entry, &table_header->entry_list, entry_node) {
		if (table_entry->entry_data_ptr != NULL) {
			int cmp_result=cmp_func(me->classid, attr_order, table_entry->entry_data_ptr, entrydata);
			//succee found attr table entry
			if (cmp_result==0) {
				found_table_entry = table_entry;
				break;
			}
			/* stop search if we found entry larger than us in ascending sort */
			if (table_ptr->entry_sort_method == ATTR_ENTRY_SORT_METHOD_ASCENDING && cmp_result<0) {
				break;
			/* stop search if we found entry smaller than us in dscending sort */
			} else if (table_ptr->entry_sort_method == ATTR_ENTRY_SORT_METHOD_DSCENDING && cmp_result>0) {
				break;
			}
			place_to_insert=&table_entry->entry_node;
		}
	}

	if (miptr->callback.table_entry_isdel_cb != NULL)
		del_mode=miptr->callback.table_entry_isdel_cb(me->classid, attr_order, entrydata);

	if (del_mode==MEINFO_CB_DELETE_ALL_ENTRY) {					//delete all entry
		if (table_header != NULL) {
			meinfo_me_attr_release_table_all_entries(table_header);
		}
	
	} else if (del_mode==MEINFO_CB_DELETE_ENTRY) {					// del one entry
		if (found_table_entry) {	//del entry
			// remove entry from table entry list
			list_del(&found_table_entry->entry_node);
			table_header->entry_count--;
			//release data
			free_safe(found_table_entry->entry_data_ptr);
			found_table_entry->entry_data_ptr = NULL;
			//release entry
			free_safe(found_table_entry);
			// set avc changed mask
			if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order))
				util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
			return MEINFO_RW_OK;
		} else {			// del non exist?
			if (omci_env_g.mib_nonexist_del_err==ENV_MIB_NONEXIST_DEL_ERR_ME_TABENTRY)
				return MEINFO_ERROR_ME_NOT_FOUND;
			else
				return MEINFO_RW_OK;
		}

	} else {									// modify or add	
		if (found_table_entry) {	// modify existing entry
			// set avc changed mask if data differs
			if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
				if (memcmp(found_table_entry->entry_data_ptr, entrydata, table_ptr->entry_byte_size)!=0)
					util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
			}
			memcpy(found_table_entry->entry_data_ptr, entrydata, table_ptr->entry_byte_size);
			found_table_entry->timestamp=time(0);
			return MEINFO_RW_OK;
		} else {			// add new entry
			table_entry = malloc_safe(sizeof(struct attr_table_entry_t));
						
			table_entry->entry_data_ptr = malloc_safe(table_ptr->entry_byte_size);			
			memcpy(table_entry->entry_data_ptr, entrydata, table_ptr->entry_byte_size);
			table_entry->timestamp=time(0);
			table_header->entry_count++;

			switch (table_ptr->entry_sort_method) {
			case ATTR_ENTRY_SORT_METHOD_NONE:
				list_add_tail(&table_entry->entry_node, &table_header->entry_list);
				break;
			case ATTR_ENTRY_SORT_METHOD_ASCENDING:
			case ATTR_ENTRY_SORT_METHOD_DSCENDING:
				list_add(&table_entry->entry_node, place_to_insert);
				break;
			}

			// set avc changed mask
			if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order))
				util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
		}
	}

	return MEINFO_RW_OK;
}

// get attr table entry number
int
meinfo_me_attr_get_table_entry_count(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	return table_header->entry_count;
}



int
meinfo_me_attr_get_table_entry_action(struct me_t *me, unsigned char attr_order, void *entrydata)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry;
	int (*cmp_func) (unsigned short classid, unsigned char attr_order,unsigned char *entry_org, unsigned char *entry_new);

	miptr = meinfo_util_miptr(me->classid);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}
		
	if (miptr->callback.table_entry_isdel_cb != NULL &&
		miptr->callback.table_entry_isdel_cb(me->classid, attr_order, entrydata) == MEINFO_CB_DELETE_ENTRY)
	{
		//deletion
		return ATTR_ENTRY_ACTION_DELETE;
	} else {
		if (miptr->callback.table_entry_cmp_cb == NULL)
			cmp_func = meinfo_me_attr_table_entry_cmp;	//use standard compare primary key function
		else
			cmp_func = miptr->callback.table_entry_cmp_cb;

		if ((table_header = me->attr[attr_order].value.ptr) != NULL)
		{
			list_for_each_entry(table_entry, &table_header->entry_list, entry_node) 
			{
				if (table_entry->entry_data_ptr != NULL) {
					//compare
					if (cmp_func(me->classid, attr_order, table_entry->entry_data_ptr, entrydata) == MEINFO_CB_ENTRY_MATCH)
					{
						return ATTR_ENTRY_ACTION_MODIFY;
					}
				}
			}
		}

		return ATTR_ENTRY_ACTION_ADD;
	}

}

int
meinfo_me_attr_get_table_crc32(struct me_t *me, unsigned char attr_order, unsigned int *crc32)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos, *table_entry_n;
	int entry_byte_size;

	unsigned int crc_accum = 0xFFFFFFFF;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	// accmulate crc32 of all entries
	entry_byte_size=miptr->attrinfo[attr_order].attrinfo_table_ptr->entry_byte_size;
	list_for_each_entry_safe(table_entry_pos, table_entry_n, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
			crc_accum=crc_be_update(crc_accum, (char *)table_entry_pos->entry_data_ptr, entry_byte_size);
	}

	*crc32 = ~crc_accum;

	return 0;
}

/* 
   For the RDONLY tables in OMCI, as OLT can not delete the table entry,
   there may be no definition of delete entry pattern for those tables.
   however, local application may still has modify the table (eg: igmp active list table)
   so we define the locate_table_entry and delete_table_entry function for table maintainance
*/
// attr table entry locate: for local access to omci rdonly table
struct attr_table_entry_t *
meinfo_me_attr_locate_table_entry(struct me_t *me, unsigned char attr_order, void *entrydata)
{
	struct meinfo_t *miptr;
	struct attr_table_entry_t *table_entry = NULL, *found_table_entry = NULL;
	struct attrinfo_table_t *table_ptr;
	struct attr_table_header_t *table_header;
	int (*cmp_func) (unsigned short classid, unsigned char attr_order,unsigned char *entry_org, unsigned char *entry_new);

	if (me == NULL ||
	    (miptr=meinfo_util_miptr(me->classid))==NULL ||
	    miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED ||
	    attr_order > miptr->attr_total ||
	    (table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr) == NULL) {
		return NULL;
	}
	
	if ( me->attr[attr_order].value.ptr == NULL ) {		//todo: should move to init
		me->attr[attr_order].value.ptr = malloc_safe( sizeof(struct attr_table_header_t) );
		table_header = me->attr[attr_order].value.ptr;
		INIT_LIST_HEAD(&table_header->entry_list);
		dbprintf(LOG_ERR, "This should not happen !!\n");
	} else {
		table_header = me->attr[attr_order].value.ptr;
	}

	if (miptr->callback.table_entry_cmp_cb == NULL)
		cmp_func = meinfo_me_attr_table_entry_cmp;	//use standard compare primary key function
	else
		cmp_func = miptr->callback.table_entry_cmp_cb;

	list_for_each_entry(table_entry, &table_header->entry_list, entry_node) {
		if (table_entry->entry_data_ptr != NULL) {
			int cmp_result=cmp_func(me->classid, attr_order, table_entry->entry_data_ptr, entrydata);
			//succee found attr table entry
			if (cmp_result==0) {
				found_table_entry = table_entry;
				break;
			}
			/* stop search if we found entry larger than us in ascending sort */
			if (table_ptr->entry_sort_method == ATTR_ENTRY_SORT_METHOD_ASCENDING && cmp_result<0) {
				break;
			/* stop search if we found entry smaller than us in dscending sort */
			} else if (table_ptr->entry_sort_method == ATTR_ENTRY_SORT_METHOD_DSCENDING && cmp_result>0) {
				break;
			}
		}
	}
	return found_table_entry;
}

// attr table entry delete: for local access to omci rdonly table
int
meinfo_me_attr_delete_table_entry(struct me_t *me, unsigned char attr_order, struct attr_table_entry_t *del_table_entry, int check_avc)
{
	struct meinfo_t *miptr;
	struct attrinfo_table_t *table_ptr;
	struct attr_table_header_t *table_header;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	if ((miptr=meinfo_util_miptr(me->classid))==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_NOT_SUPPORT;
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_ATTR_UNDEF;
	}

	if ((table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}	
	if ( me->attr[attr_order].value.ptr == NULL ) {		//todo: should move to init
		me->attr[attr_order].value.ptr = malloc_safe( sizeof(struct attr_table_header_t) );
		table_header = me->attr[attr_order].value.ptr;
		INIT_LIST_HEAD(&table_header->entry_list);
		dbprintf(LOG_ERR, "This should not happen !!\n");
	} else {
		table_header = me->attr[attr_order].value.ptr;
	}

	if (del_table_entry) {	//del entry
		// remove entry from table entry list
		list_del(&del_table_entry->entry_node);
		table_header->entry_count--;
		//release data
		free_safe(del_table_entry->entry_data_ptr);
		del_table_entry->entry_data_ptr = NULL;
		//release entry
		free_safe(del_table_entry);
		// set avc changed mask
		if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order))
			util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
	}
	return MEINFO_RW_OK;
}

// check if a pointer points to exist me
int
meinfo_me_attr_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	unsigned short default_pointer_classid;

	if (me==NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return 0;
	}	
	if ((miptr=meinfo_util_miptr(me->classid))==NULL ||
	    miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED ||
	    attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "classid=%u, attr_order=%u, invalid classid?\n",
			me->classid, attr_order);
		return 0;
	}
	if  (meinfo_util_attr_get_format(me->classid, attr_order)!= ATTR_FORMAT_POINTER)
		return -1;

	default_pointer_classid=meinfo_util_attr_get_pointer_classid(me->classid, attr_order);
	
	if (default_pointer_classid==0 && miptr->callback.get_pointer_classid_cb) {
		return miptr->callback.get_pointer_classid_cb(me, attr_order);	
	}
	return default_pointer_classid;
}

// return: 1=valid, -1=valid but warning, 0=invalid
// this routine use me_t as parm instead of classid/meid 
// because it may be used to verify me that is not attachmed into mib (eg: me create)
int
meinfo_me_attr_is_valid(struct me_t *me, unsigned short attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr;

	if (me==NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return 0;
	}	
	if ((miptr=meinfo_util_miptr(me->classid))==NULL ||
	    miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED ||
	    attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "classid=%u, attr_order=%u, invalid classid?\n",
			me->classid, attr_order);
		return 0;
	}

	switch (meinfo_util_attr_get_format(me->classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
		if (omci_env_g.mib_pointer_check) {
			int pointer_classid=meinfo_me_attr_pointer_classid(me, attr_order);
			
			// pointer classid invalid?
			if (pointer_classid <0) {
				dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, pointer classid invalid?\n",
					me->classid, me->meid, me->meid, attr_order);
				return 0;
			} else if (pointer_classid==0) {	// pointer should be ignored
				return 1;
			}
			
			// pointer is null
			if (attr->data==0 || attr->data==0xffff) {
				if (me->is_private)
					return 1;
				// ignore null pointer owned by onu-created me where me is not modified					
				if (miptr->access == ME_ACCESS_TYPE_CB_ONT && meinfo_me_compare_config(me)==0)
					return 1;		
				dbprintf(LOG_WARNING, "classid=%u, meid=0x%x(%u), attr_order=%u, null pointer?\n",
					me->classid, me->meid, me->meid, attr_order);
				return -1;		
			}

			// pointer is to a nonexisting me
			if (meinfo_me_get_by_meid(pointer_classid, attr->data)==NULL) {
				struct meinfo_t *pointer_miptr=meinfo_util_miptr(pointer_classid);
				int ret;					

				if (pointer_miptr==NULL) {
					dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, pointer classid not supported?\n",
						me->classid, me->meid, me->meid, attr_order);
					return 0;
				}
				if (pointer_miptr->access == ME_ACCESS_TYPE_CB_ONT) {	
					// pointer to me that can only be created by onu
					ret=(omci_env_g.mib_pointer_check==ENV_MIB_POINTER_CHECK_ERROR)?0:-1;
				} else {
					// warning, as pointer to me that may be created by olt later
					ret=-1;	
				}
				dbprintf((ret==0)?LOG_ERR:LOG_WARNING, "classid=%u, meid=0x%x(%u), attr_order=%u, nonexis me pointer %u:0x%x(%u)\n",
					me->classid, me->meid, me->meid, attr_order, pointer_classid, attr->data);
			}
		}
		break;
	
	case ATTR_FORMAT_UNSIGNED_INT:
		if (omci_env_g.mib_boundary_check) {
			if (meinfo_util_attr_has_lower_limit(me->classid, attr_order)) {
				long long lower_limit=meinfo_util_attr_get_lower_limit(me->classid, attr_order);
				if (attr->data < lower_limit) {
					dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, value '%lld' < lowerlimit '%lld'\n",
						me->classid, me->meid, me->meid, attr_order, attr->data, lower_limit);
					if (omci_env_g.mib_boundary_check==ENV_MIB_BOUNDARY_CHECK_ERROR)
						return 0;
					else
						return -1;
				}
			}
			if (meinfo_util_attr_has_upper_limit(me->classid, attr_order)) {
				long long upper_limit=meinfo_util_attr_get_upper_limit(me->classid, attr_order);
				if (attr->data > upper_limit) {
					dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, value '%lld' > upperlimit '%lld'\n",
						me->classid, me->meid, me->meid, attr_order, attr->data, upper_limit);
					if (omci_env_g.mib_boundary_check==ENV_MIB_BOUNDARY_CHECK_ERROR)
						return 0;
					else
						return -1;
				}
			}
		}
		break;
	case ATTR_FORMAT_SIGNED_INT:
		if (omci_env_g.mib_boundary_check) {
			int byte_size=meinfo_util_attr_get_byte_size(me->classid, attr_order);
			char valuestr[64];
			
			if (meinfo_util_attr_has_lower_limit(me->classid, attr_order)) {
				long long lower_limit=meinfo_util_attr_get_lower_limit(me->classid, attr_order);
				int is_lower=0;
				if (byte_size==1) {
					is_lower= ((char)attr->data < lower_limit)?1:0;
					snprintf(valuestr, 64, "%d", (char)attr->data);
				} else if (byte_size==2) {
					is_lower= ((short)attr->data < lower_limit)?1:0;
					snprintf(valuestr, 64, "%d", (short)attr->data);
				} else if (byte_size<=4) {
					is_lower= ((int)attr->data < lower_limit)?1:0;
					snprintf(valuestr, 64, "%d", (int)attr->data);
				} else {
					is_lower= ((long long)attr->data < lower_limit)?1:0;
					snprintf(valuestr, 64, "%lld", attr->data);
				}
				if (is_lower) {
					dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, value '%s' < lowerlimit '%lld'\n",
						me->classid, me->meid, me->meid, attr_order, valuestr, lower_limit);
					if (omci_env_g.mib_boundary_check==ENV_MIB_BOUNDARY_CHECK_ERROR)
						return 0;
					else
						return -1;
				}
			}
			if (meinfo_util_attr_has_upper_limit(me->classid, attr_order)) {
				long long upper_limit=meinfo_util_attr_get_upper_limit(me->classid, attr_order);
				int is_upper=0;
				if (byte_size==1) {
					is_upper= ((char)attr->data > upper_limit)?1:0;
					snprintf(valuestr, 64, "%d", (char)attr->data);
				} else if (byte_size==2) {
					is_upper= ((short)attr->data > upper_limit)?1:0;
					snprintf(valuestr, 64, "%d", (short)attr->data);
				} else if (byte_size<=4) {
					is_upper= ((int)attr->data > upper_limit)?1:0;
					snprintf(valuestr, 64, "%d", (int)attr->data);
				} else {
					is_upper= ((long long)attr->data > upper_limit)?1:0;
					snprintf(valuestr, 64, "%lld", attr->data);
				}
				if (is_upper) {
					dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, value '%s' > upperlimit '%lld'\n",
						me->classid, me->meid, me->meid, attr_order, valuestr, upper_limit);
					if (omci_env_g.mib_boundary_check==ENV_MIB_BOUNDARY_CHECK_ERROR)
						return 0;
					else
						return -1;
				}
			}
		}
		break;
	case ATTR_FORMAT_ENUMERATION:
		if (omci_env_g.mib_boundary_check) {
			struct attr_code_point_t *attr_code_point;
			int found = 0;			
			list_for_each_entry(attr_code_point,
						&(meinfo_util_attr_get_code_points_list(me->classid,attr_order)), 
						code_points_list) {
				if (attr_code_point->code == attr->data) {
					found = 1;
					break;
				}
			}
			if (!found) {
				dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), attr_order=%u, value '%d' is not a valid codepoint\n",
					me->classid, me->meid, me->meid, attr_order, attr->data);
				if (omci_env_g.mib_boundary_check==ENV_MIB_BOUNDARY_CHECK_ERROR)
					return 0;
				else
					return -1;
			}
		}
		break;
	case ATTR_FORMAT_BIT_FIELD:
	case ATTR_FORMAT_STRING:
	case ATTR_FORMAT_TABLE:
	default:
		;		//do nothing, not sure how to validate
	}

	// do me attr value validation if check routine is defined
	if (miptr->callback.is_attr_valid_cb ||
	    miptr->hardware.is_attr_valid_hw) {
	    	int ret1=1, ret2=1;

		// check att value for G.984.4 limitation, eg: null pointer
		if (miptr->callback.is_attr_valid_cb) {
			ret1=miptr->callback.is_attr_valid_cb(me, attr_order, attr);
			if (ret1<=0) {
				dbprintf(ret1?LOG_WARNING:LOG_ERR, "class=%u, meid=0x%x(%u) attr %u, value invalid for cb\n", 
					me->classid, me->meid, me->meid, attr_order);
			}
		}
		// check att value for hw limitation, eg: num of pq allowed in one ts
		if (miptr->hardware.is_attr_valid_hw) {
			ret2=miptr->hardware.is_attr_valid_hw(me, attr_order, attr);
			if (ret2<=0) {
				dbprintf(ret2?LOG_WARNING:LOG_ERR, "class=%u, meid=0x%x(%u) attr %u, value invalid for hw\n", 
					me->classid, me->meid, me->meid, attr_order);
			}
		}
		if (ret1==0 || ret2==0)
			return 0;	// invalid
		if (ret1==-1 || ret2==-1)
			return -1;	// valid with warning
	}

	return 1;		// attr value is valid
}

int 
meinfo_me_attr_release_table_all_entries(struct attr_table_header_t *table_header_ptr)
{
	struct attr_table_entry_t *pos_ptr, *n_ptr;

	if (table_header_ptr==NULL)
		return -1;

	//table here
	if (table_header_ptr->entry_count > 0 && !list_empty(&table_header_ptr->entry_list)) {
		list_for_each_entry_safe(pos_ptr, n_ptr, &table_header_ptr->entry_list, entry_node) {
			if (pos_ptr->entry_data_ptr != NULL) {
				free_safe(pos_ptr->entry_data_ptr);
				pos_ptr->entry_data_ptr = NULL;
			}
			list_del(&pos_ptr->entry_node);
			free_safe(pos_ptr);
		}
	}
	table_header_ptr->entry_count=0;

	return 0;
}

// clear everything in attr value except table headr, used for attr_value reuse
int
meinfo_me_attr_clear_value(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr)
{
	switch (meinfo_util_attr_get_format(classid, attr_order)) {
	case ATTR_FORMAT_BIT_FIELD:
	case ATTR_FORMAT_STRING:
		if (attr->ptr != NULL) {
			free_safe(attr->ptr);
			attr->ptr = NULL;
		}
		break;
	case ATTR_FORMAT_TABLE:
		if (attr->ptr != NULL)
			meinfo_me_attr_release_table_all_entries(attr->ptr);	// free table entries
		break;
	default:
		;	//do nothing
	}

	return 0;
}

// release everything in attr value and table headr, used by me release
int
meinfo_me_attr_release(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr)
{
	meinfo_me_attr_clear_value(classid, attr_order, attr);
	if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE ) {
		if (attr->ptr != NULL) {
			free_safe(attr->ptr);				// free table header
			attr->ptr = NULL;
		}
	}
	return 0;
}

int
meinfo_me_attr_copy_value(unsigned short classid, unsigned char attr_order, 
	struct attr_value_t *dst_value, struct attr_value_t *src_value)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	int format;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}	

	format = meinfo_util_attr_get_format(classid, attr_order);

	if (format == ATTR_FORMAT_TABLE) {
		struct attr_table_header_t *table_header_ptr, *table_header_ptr2;
		struct attr_table_entry_t *table_entry_ptr, *table_entry_ptr2;
		int entry_byte_size;

		if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL) {
			dbprintf(LOG_ERR, "table attr is not defined in meinfo, classid=%u, attr=%d\n", classid, attr_order);
			return MEINFO_ERROR_ATTR_PTR_NULL;
		}
		entry_byte_size = miptr->attrinfo[attr_order].attrinfo_table_ptr->entry_byte_size;

		// copy table header 
		dst_value->ptr = malloc_safe(sizeof (struct attr_table_header_t));
		table_header_ptr = src_value->ptr;
		table_header_ptr2 = dst_value->ptr;
		*table_header_ptr2 = *table_header_ptr;

		INIT_LIST_HEAD(&table_header_ptr2->entry_list);

		// copy all entries
		list_for_each_entry(table_entry_ptr, &table_header_ptr->entry_list, entry_node) {
			table_entry_ptr2 = malloc_safe(sizeof (struct attr_table_entry_t));
			*table_entry_ptr2 = *table_entry_ptr;

			if (table_entry_ptr->entry_data_ptr != NULL) {
				table_entry_ptr2->entry_data_ptr = malloc_safe(entry_byte_size);
				memcpy(table_entry_ptr2->entry_data_ptr,
				       table_entry_ptr->entry_data_ptr, entry_byte_size);
			}

			list_add_tail(&table_entry_ptr2->entry_node, &table_header_ptr2->entry_list);
		}

	} else {
		int byte_size = miptr->attrinfo[attr_order].byte_size;

		switch (format) {
		case ATTR_FORMAT_BIT_FIELD:	//not sure
		case ATTR_FORMAT_STRING:
			if (src_value->ptr != NULL) {
				if (dst_value->ptr == NULL) {
					dbprintf(LOG_NOTICE, "classid=%u, attr_order=%u, dst attr value ptr null? try to alloc mem for it\n", 
						classid, attr_order);
					dst_value->ptr = malloc_safe(byte_size);
				} else if (dst_value->ptr == src_value->ptr) {
					dbprintf(LOG_ERR, "classid=%u, attr_order=%u, dst attr value ptr is the same as src attr value ptr?\n", 
						classid, attr_order);
					dst_value->ptr = malloc_safe(byte_size);
				}
				memcpy(dst_value->ptr, src_value->ptr, byte_size);
			}
			break;
		default:
			dst_value->data = src_value->data;
		}
	}

	return 0;
}

int
meinfo_me_attr_set_table_all_entries_by_callback(struct me_t *me, unsigned char attr_order, 
	int (*set_entry_cb) (struct me_t *me, unsigned char attr_order, unsigned short seq, void *entrydata), int check_avc)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	int entry_byte_size;
	char *entrydata;
	unsigned int seq = 0, count = 0;
	unsigned int crc32_old=0, crc32_new=0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order))
		meinfo_me_attr_get_table_crc32(me, attr_order, &crc32_old);

	// release all old entries
	meinfo_me_attr_release_table_all_entries(table_header);

	entry_byte_size=miptr->attrinfo[attr_order].attrinfo_table_ptr->entry_byte_size;
	seq = 0;
	count = 0;
	// set all entries
	while(count < 65536) {	// assume the omci table entry wont be more than 65536, better than while(1)
		entrydata = malloc_safe(entry_byte_size);
		//get each entry from hw, until return < 0
		if (set_entry_cb(me, attr_order, seq++, entrydata) != 0) {
			free_safe(entrydata);
			break;
		}		
		//add entry to mib table
		table_entry_pos = malloc_safe(sizeof(struct attr_table_entry_t));
		table_entry_pos->entry_data_ptr = entrydata;
		table_header->entry_count++;
		list_add_tail(&table_entry_pos->entry_node, &table_header->entry_list);
		count++;
	}

	if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
		meinfo_me_attr_get_table_crc32(me, attr_order, &crc32_new);
		if (crc32_new!=crc32_old)
			util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
	}

	return 0;
}

int
meinfo_me_attr_set_table_all_entries_by_mem(struct me_t *me, unsigned char attr_order, int memsize, char *memptr, int check_avc)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	int entry_byte_size;
	char *entrydata;
	unsigned int offset = 0;
	unsigned int crc32_old=0, crc32_new=0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order))
		meinfo_me_attr_get_table_crc32(me, attr_order, &crc32_old);

	// release all old entries
	meinfo_me_attr_release_table_all_entries(table_header);

	entry_byte_size=miptr->attrinfo[attr_order].attrinfo_table_ptr->entry_byte_size;
	// set all entries
	for (offset=0; offset < memsize; offset+=entry_byte_size) {
		int left_size= memsize-offset;

		entrydata = malloc_safe(entry_byte_size);
		if (left_size>=entry_byte_size) {
			memcpy(entrydata, memptr+offset, entry_byte_size);
		} else {
			memcpy(entrydata, memptr+offset, left_size);
		}

		//add entry to mib table
		table_entry_pos = malloc_safe(sizeof(struct attr_table_entry_t));
		table_entry_pos->entry_data_ptr = entrydata;

		table_header->entry_count++;
		list_add_tail(&table_entry_pos->entry_node, &table_header->entry_list);
	}

	if (check_avc && util_attr_mask_get_bit(miptr->avc_mask, attr_order)) {
		meinfo_me_attr_get_table_crc32(me, attr_order, &crc32_new);
		if (crc32_new!=crc32_old)
			util_attr_mask_set_bit(me->avc_changed_mask, attr_order);
	}

	return 0;
}

/*0: not empty, 1: empty, <0: error*/
int
meinfo_me_attr_is_table_empty(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	struct attr_table_header_t *table_header;

	if (me == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	miptr = meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me->meid, attr_order);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), me->classid, me->meid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (miptr->attrinfo[attr_order].attrinfo_table_ptr == NULL ||
		(table_header = (struct attr_table_header_t *) me->attr[attr_order].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "Operator should be table attribute\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	return list_empty(&table_header->entry_list);
}

