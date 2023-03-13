#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "me_related.h"

// 65297 Onu_opt_threshold

static struct me_t *
meinfo_cb_65297_create_private_me(unsigned short classid, unsigned short meid)
{
	short instance_num;
	struct me_t *me;
	
	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0) {
		return NULL;
	}
	me = meinfo_me_alloc(classid, instance_num);
	if (!me) {
		return NULL;
	}

	// set default from config
	meinfo_me_load_config(me);

	// assign meid
	me->meid = meid;
	
	//create by olt
	me->is_cbo = 1;
	//set as priovate
	me->is_private = 1;

	if (meinfo_me_create(me) < 0) {
		meinfo_me_release(me);
		return NULL;
	}
	return me;
}

static int 
meinfo_cb_65297_set(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short alarm_code = (unsigned short)meinfo_util_me_attr_data(me, 1);
	int alarm_code_is_valid=0;
	
	if ((alarm_code>=997 && alarm_code <=1004) ||
	    (alarm_code>=1023 && alarm_code <=1024) ||
	    (alarm_code>=1007 && alarm_code <=1016))
	    	alarm_code_is_valid=1;	

	if (me->meid == 0 && alarm_code_is_valid) {
		struct me_t *me2 = meinfo_me_get_by_meid(me->classid, alarm_code);
		if (me2==NULL)
			me2 = meinfo_cb_65297_create_private_me(me->classid, alarm_code);
		if (me2 == NULL) {
			dbprintf(LOG_ERR, "me classid=%u, meid=%u create err?\n", 65297, alarm);
			return MEINFO_ERROR_CB_ME_SET;
		}			

		if (util_attr_mask_get_bit(attr_mask, 1)) {	// alarm_code
			struct attr_value_t attr={0, NULL};
			attr.data=meinfo_util_me_attr_data(me, 1);
			meinfo_me_attr_set(me2, 1, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 2)) {	// alarm_threshold
			struct attr_value_t attr={0, NULL};
			attr.ptr=meinfo_util_me_attr_ptr(me, 2);
			meinfo_me_attr_set(me2, 2, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 3)) {	// clear_threshold
			struct attr_value_t attr={0, NULL};
			attr.ptr=meinfo_util_me_attr_ptr(me, 3);
			meinfo_me_attr_set(me2, 3, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 4)) {	// alarm_open_enable
			struct attr_value_t attr={0, NULL};
			attr.data=meinfo_util_me_attr_data(me, 4);
			meinfo_me_attr_set(me2, 4, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 5)) {	// onu_pon_id
			struct attr_value_t attr={0, NULL};
			attr.data=meinfo_util_me_attr_data(me, 5);
			meinfo_me_attr_set(me2, 5, &attr, 0);
		}
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65297 = {
	.set_cb		= meinfo_cb_65297_set,
};
