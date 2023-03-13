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
 * Module  : er_group
 * File    : er_group.h
 *
 ******************************************************************/

#ifndef __ER_GROUP_H__
#define __ER_GROUP_H__

#include "list.h"
#include "meinfo.h"

#define ER_ME_GROUP_MEMBER_MAX		16
#define ER_ATTR_GROUP_MEMBER_MAX	32

#define ER_GROUP_ERROR_BASE				0x8000
#define ER_GROUP_ERROR_DEFINITION_ERROR		-(ER_GROUP_ERROR_BASE + 1)
#define ER_GROUP_ERROR_INPUT_ERROR		-(ER_GROUP_ERROR_BASE + 2)

#define ER_GROUP_CLASS_NOT_FOUND 0xFF

#define ER_ATTR_GROUP_HW_OP_NONE		0
#define ER_ATTR_GROUP_HW_OP_ADD			1
#define ER_ATTR_GROUP_HW_OP_DEL			2
#define ER_ATTR_GROUP_HW_OP_UPDATE		3
#define ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE	4
#define ER_ATTR_GROUP_HW_OP_ERROR		-1

#define ER_ATTR_GROUP_HW_RESULT_UNREASONABLE 0xFFFF

#define er_attr_group_dbprintf(level, log_time, fmt, args...) {if (omci_env_g.er_group_hw_debug_enable == 1) util_dbprintf(omci_env_g.debug_level, level, log_time, fmt, ##args);}

struct er_me_relation_t {
	unsigned short classid;
	unsigned short referred_classid[ER_ME_GROUP_MEMBER_MAX];
	struct list_head er_me_relation_node;
};

struct er_me_group_definition_t {
	char *name;
	unsigned short id;
	unsigned short classid[ER_ME_GROUP_MEMBER_MAX];
	struct list_head er_me_group_definition_node;
	struct list_head er_me_relation_list;
	struct list_head er_me_group_instance_list;
	struct list_head er_attr_group_definition_list;
};

// node stru used by meinfo so one er_me_group_definition can be linked by multiple classids
struct er_me_group_definition_ptr_node_t {
	struct er_me_group_definition_t *ptr;
	struct list_head node;
};

struct er_me_group_instance_t {
	unsigned short meid[ER_ME_GROUP_MEMBER_MAX];
	struct list_head er_attr_group_instance_list;
	struct list_head er_me_group_instance_node;
};

struct er_me_group_instance_context_node_t {
	struct me_t *context_me[ER_ME_GROUP_MEMBER_MAX];
	struct list_head node;
};

struct er_me_group_instance_find_context_t {
	struct er_me_group_definition_t *er_group_definition_ptr;
	struct list_head *found_instance_list;
	struct me_t *must_have_me;
};

struct er_attr_t {
	unsigned short classid;
	unsigned char attr_order;	// 1..16 for attr_order, 0: unused, 254:inst number, 255: meid
};

struct er_attr_group_definition_t {
	char *name;
	unsigned char id;
	struct er_attr_t er_attr[ER_ATTR_GROUP_MEMBER_MAX];
	struct list_head er_attr_group_definition_node;
	unsigned int del_time_diff;
	unsigned int add_time_diff;
};

struct er_attr_instance_t {
	unsigned short meid;		// copied from er_me_group_instance when created
	struct attr_value_t attr_value;	// copied from mib when created
};

struct er_attr_group_instance_t {
	struct er_attr_instance_t er_attr_instance[ER_ATTR_GROUP_MEMBER_MAX];
	struct er_attr_group_definition_t *er_attr_group_definition_ptr;
	struct list_head next_er_attr_group_instance_node;
	int exec_result;
};

extern struct list_head er_me_group_definition_list;

#define attr_inst_classid(attr_inst, i)		((attr_inst)?attr_inst->er_attr_group_definition_ptr->er_attr[i].classid:0)
#define attr_inst_meid(attr_inst, i)		((attr_inst)?attr_inst->er_attr_instance[i].meid:0)
#define attr_inst_attr_order(attr_inst, i)	((attr_inst)?attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order:0);

/* er_attr_group.c */
int er_attr_group_me_add(struct me_t *me, unsigned char attr_mask[2]);
int er_attr_group_me_delete(struct me_t *me);
int er_attr_group_me_update(struct me_t *me, unsigned char attr_mask[2]);

/* er_attr_group_hw.c */
void er_attr_group_hw_preinit(void);
int er_attr_group_hw_add(char *name, int (*er_attr_group_hw_func)(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2]));
int er_attr_group_hw_del(char *name);
int er_attr_group_hw_dump(int fd);
int er_attr_group_hw(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *caused_me, unsigned char attr_mask[2]);

/* er_attr_group_util.c */
int er_attr_group_util_table_op_detection(struct er_attr_group_instance_t *attr_inst, int inst_order, struct attr_table_entry_t **result_entry_old, struct attr_table_entry_t **result_entry_new);
struct me_t *er_attr_group_util_attrinst2me(struct er_attr_group_instance_t *attr_inst, int inst_order);
struct me_t *er_attr_group_util_attrinst2private_me(struct er_attr_group_instance_t *attr_inst, int inst_order);
int er_attr_group_util_release_attrinst(struct er_attr_group_instance_t *attr_inst);
struct er_attr_group_instance_t *er_attr_group_util_gen_current_value_attrinst(struct er_attr_group_instance_t *old_attr_inst);
struct me_t *er_attr_group_util_find_related_me(struct me_t *me, unsigned short related_claissid);
int er_attr_group_util_get_attr_mask_by_classid(struct er_attr_group_instance_t *er_attr_group_instance, unsigned short classid, unsigned char attr_mask[2]);

/* er_me_group.c */
int er_me_group_preinit(void);
int er_me_group_instance_find(struct er_me_group_instance_find_context_t *context);

#endif
