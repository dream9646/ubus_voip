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
 * File    : meinfo.h
 *
 ******************************************************************/

#ifndef __MEINFO_H__
#define __MEINFO_H__

#include "list.h"

// uncomment below to enable me "copy on write" behavior
// since it seems "copy on write" caused me256 msiing issue, we disabled it for stability
//#define USE_MEINFO_ME_COPY_ON_WRITE			1

#define MEINFO_CLASSID_OFFICIAL_START		0
#define MEINFO_CLASSID_OFFICIAL_END		511
#define MEINFO_CLASSID_PROPRIETARY_START	65280
#define MEINFO_CLASSID_PROPRIETARY_END		65535

#define MEINFO_INDEX_TOTAL_OFFICIAL		(MEINFO_CLASSID_OFFICIAL_END-MEINFO_CLASSID_OFFICIAL_START+1)
#define MEINFO_INDEX_TOTAL_PROPRIETARY		(MEINFO_CLASSID_PROPRIETARY_END-MEINFO_CLASSID_PROPRIETARY_START+1)
#define MEINFO_INDEX_TOTAL			(MEINFO_INDEX_TOTAL_OFFICIAL+MEINFO_INDEX_TOTAL_PROPRIETARY)

#define MEINFO_ATTR_ORDER_MAX	17	// 1..16 is used

#define MEINFO_RW_OK			0
#define MEINFO_RW_ERROR_UNSUPPORT	-1
#define MEINFO_RW_ERROR_FAILED		-2

#define DEVICE_NAME_PREFIX_MAX_LEN 	16
#define DEVICE_NAME_MAX_LEN		32
#define ATTRINFO_BITFIELD_FIELD_MAX	64
#define ATTRINFO_TABLE_FIELD_MAX	32

#define ATTRINFO_ACCESEE_BITMAP_READ 0x01
#define ATTRINFO_ACCESEE_BITMAP_WRITE 0x02
#define ATTRINFO_ACCESEE_BITMAP_SBC 0x04

#define MEINFO_ATTR_VALUE_TYPE_LEN 8	/*byte, attribue integer type value length */
#define MEINFO_ME_PM_TD_ATTR_NUM 7 /*threshold data entity attribute number*/

#define MEINFO_INSTANCE_DEFAULT 0xFFFF //default instance number, 65535

#define MEINFO_ACTION_MASK_CREATE			0x00000008
#define MEINFO_ACTION_MASK_CREATE_COMPLETE_CONNECTION	0x00000010
#define MEINFO_ACTION_MASK_DELETE			0x00000020
#define MEINFO_ACTION_MASK_DELETE_COMPLETE_CONNECTION	0x00000040
#define MEINFO_ACTION_MASK_SET				0x00000080
#define MEINFO_ACTION_MASK_GET				0x00000100
#define MEINFO_ACTION_MASK_GET_COMPLETE_CONNECTION	0x00000200
#define MEINFO_ACTION_MASK_GET_ALL_ALARMS		0x00000400
#define MEINFO_ACTION_MASK_GET_ALL_ALARMS_NEXT		0x00000800
#define MEINFO_ACTION_MASK_MIB_UPLOAD			0x00001000
#define MEINFO_ACTION_MASK_MIB_UPLOAD_NEXT		0x00002000
#define MEINFO_ACTION_MASK_MIB_RESET			0x00004000
#define MEINFO_ACTION_MASK_ALARM			0x00008000
#define MEINFO_ACTION_MASK_ATTRIBUTE_VALUE_CHANGE	0x00010000
#define MEINFO_ACTION_MASK_TEST				0x00020000
#define MEINFO_ACTION_MASK_START_SOFTWARE_DOWNLOAD	0x00040000
#define MEINFO_ACTION_MASK_DOWNLOAD_SECTION		0x00080000
#define MEINFO_ACTION_MASK_END_SOFTWARE_DOWNLOAD	0x00100000
#define MEINFO_ACTION_MASK_ACTIVE_SOFTWARE		0x00200000
#define MEINFO_ACTION_MASK_COMMIT_SOFTWARE		0x00400000
#define MEINFO_ACTION_MASK_SYNCHRONIZE_TIME		0x00800000
#define MEINFO_ACTION_MASK_REBOOT			0x01000000
#define MEINFO_ACTION_MASK_GET_NEXT			0x02000000
#define MEINFO_ACTION_MASK_TEST_RESULT			0x04000000
#define MEINFO_ACTION_MASK_GET_CURRENT_DATA		0x08000000

#define MEINFO_ERROR_BASE 				0x1000

#define MEINFO_ERROR_CLASSID_UNDEF 			-(MEINFO_ERROR_BASE + 0x1)
#define MEINFO_ERROR_CLASSID_NOT_SUPPORT		-(MEINFO_ERROR_BASE + 0x2)

#define MEINFO_ERROR_ME_NOT_FOUND 			-(MEINFO_ERROR_BASE + 0x11)
#define MEINFO_ERROR_ME_PTR_NULL 			-(MEINFO_ERROR_BASE + 0x12)
#define MEINFO_ERROR_ME_STILL_IN_USE 			-(MEINFO_ERROR_BASE + 0x13)
#define MEINFO_ERROR_ME_INSTANCE_FULL 			-(MEINFO_ERROR_BASE + 0x14)
#define MEINFO_ERROR_ME_COPY_ON_WRITE 			-(MEINFO_ERROR_BASE + 0x15)

#define MEINFO_ERROR_ATTR_UNDEF 			-(MEINFO_ERROR_BASE + 0x21)
#define MEINFO_ERROR_ATTR_PTR_NULL 			-(MEINFO_ERROR_BASE + 0x22)
#define MEINFO_ERROR_ATTR_READONLY 			-(MEINFO_ERROR_BASE + 0x23)
#define MEINFO_ERROR_ATTR_VALUE_INVALID 		-(MEINFO_ERROR_BASE + 0x24)
#define MEINFO_ERROR_ATTR_FORMAT_UNKNOWN 		-(MEINFO_ERROR_BASE + 0x25)
#define MEINFO_ERROR_ATTR_FORMAT_CONVERT 		-(MEINFO_ERROR_BASE + 0x26)
#define MEINFO_ERROR_ATTR_INVALID_ACTION_ON_TABLE 	-(MEINFO_ERROR_BASE + 0x27)
#define MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME 	-(MEINFO_ERROR_BASE + 0x28)

#define MEINFO_ERROR_CB_ME_GET 				-(MEINFO_ERROR_BASE + 0x31)
#define MEINFO_ERROR_CB_ME_SET 				-(MEINFO_ERROR_BASE + 0x32)
#define MEINFO_ERROR_CB_ME_CREATE 			-(MEINFO_ERROR_BASE + 0x33)
#define MEINFO_ERROR_CB_ME_DELETE 			-(MEINFO_ERROR_BASE + 0x34)
#define MEINFO_ERROR_CB_ME_ALARM 			-(MEINFO_ERROR_BASE + 0x35)

#define MEINFO_ERROR_HW_ME_GET 				-(MEINFO_ERROR_BASE + 0x41)
#define MEINFO_ERROR_HW_ME_SET 				-(MEINFO_ERROR_BASE + 0x42)
#define MEINFO_ERROR_HW_ME_CREATE 			-(MEINFO_ERROR_BASE + 0x43)
#define MEINFO_ERROR_HW_ME_DELETE 			-(MEINFO_ERROR_BASE + 0x44)
#define MEINFO_ERROR_HW_ME_ALARM 			-(MEINFO_ERROR_BASE + 0x45)

/* me required type */
enum me_required_t {
	ME_REQUIRE_R = 1,
	ME_REQUIRE_CR = 2,
	ME_REQUIRE_O = 3
};

/* me created by */
enum me_access_type_t {
	ME_ACCESS_TYPE_CB_ONT = 1,
	ME_ACCESS_TYPE_CB_OLT,
	ME_ACCESS_TYPE_CB_BOTH
};

/* me support type */
enum me_support_type_t {
	ME_SUPPORT_TYPE_SUPPORTED = 1,
	ME_SUPPORT_TYPE_NOT_SUPPORTED,
	ME_SUPPORT_TYPE_PARTIAL_SUPPORTED,
	ME_SUPPORT_TYPE_IGNORED
};

/* attr necessity */
enum attr_necessity_t {
	ATTR_NECESSITY_OPTIONAL,
	ATTR_NECESSITY_MANDATORY
};

/* attr format */
enum attr_format_t {
	ATTR_FORMAT_POINTER = 1,
	ATTR_FORMAT_BIT_FIELD,
	ATTR_FORMAT_SIGNED_INT,
	ATTR_FORMAT_UNSIGNED_INT,
	ATTR_FORMAT_STRING,
	ATTR_FORMAT_ENUMERATION,
	ATTR_FORMAT_TABLE
};

/* attr/bitfield/tablefield vformat, 
   0 is reserved for attr as null vformat for attr */
enum attr_vformat_t {
	ATTR_VFORMAT_POINTER = 1,
	ATTR_VFORMAT_BIT_FIELD,		// not used
	ATTR_VFORMAT_SIGNED_INT,
	ATTR_VFORMAT_UNSIGNED_INT,
	ATTR_VFORMAT_STRING,
	ATTR_VFORMAT_ENUMERATION,
	ATTR_VFORMAT_TABLE,		// not used
	ATTR_VFORMAT_MAC,
	ATTR_VFORMAT_IPV4,
	ATTR_VFORMAT_IPV6,
};

/* table entry sort method */
enum attr_entry_sort_method_t {
	ATTR_ENTRY_SORT_METHOD_NONE,
	ATTR_ENTRY_SORT_METHOD_ASCENDING,
	ATTR_ENTRY_SORT_METHOD_DSCENDING
};

/* table entry sort method */
enum attr_entry_action_t {
	ATTR_ENTRY_ACTION_NONE,
	ATTR_ENTRY_ACTION_ADD,
	ATTR_ENTRY_ACTION_DELETE,
	ATTR_ENTRY_ACTION_MODIFY
};

/* for get table operation, store information for consecutive get next */
struct gettable_state_t {
	char *entrypool;
	int timer_id;
	unsigned int curr_cmd_num;
	unsigned int total_cmds_num;
};

/* header node for table entry list */
struct attr_table_header_t {
	unsigned short entry_count;
	struct list_head entry_list;	/* a list that node is attr_table_entry_t */
};

/* table entry node */
struct attr_table_entry_t {
	void *entry_data_ptr;
	int timestamp;			/* the last modification time of the entry */
	struct list_head entry_node;	/* a node of list attr_table_header_t.entry_list */
};

/* get table entry node */
struct get_attr_table_context_t {
	struct attr_table_entry_t *table_entry;
	unsigned short sequence;
};

/* the definition of one field, used by table or bitfield attr */
struct attrinfo_field_def_t {
	char *name;
	unsigned short bit_width;
	unsigned char vformat;
	unsigned char is_primary_key:1;
};

/* describe the definition of bitield */
struct attrinfo_bitfield_t {
	unsigned short field_total;
	struct attrinfo_field_def_t field[ATTRINFO_BITFIELD_FIELD_MAX];	/* define bitcount/vformat of field, max 64 fields */
};

/* describe the definition of table */
struct attrinfo_table_t {
	unsigned short entry_byte_size;
	unsigned short entry_sort_method;	/* 0:none, 1:ascending, 2:descending */
	unsigned short field_total;
	struct attrinfo_field_def_t field[ATTRINFO_TABLE_FIELD_MAX];	/* define bitcount/vformat of field, max 32 fields */
};

struct attr_value_t {
	unsigned long long data;
	void *ptr;		//table point to attr_table_header_t
};

/* node to stroe value of code points */
struct attr_code_point_t {
	unsigned short code;
	struct list_head code_points_list;	/* a node of list code_points_list in attinfo_t */
};

/* define attribute value of me instance */
struct attr_t {
	struct attr_value_t value;		//for normal me, this is data; for pm me, this is history data
	struct attr_value_t value_current_data; //for pm me, this is current data 
	struct attr_value_t value_hwprev;    	//for pm me, used to store previous hw val, for hw counter won't reset to 0 after read
};

/* define the information for me instance */
struct me_t {
	unsigned short classid;
	unsigned short instance_num;
	unsigned short meid;
	unsigned char is_private:1;	/* hidden from olt in mibupload */
	unsigned char is_cbo:1;	/* 1 create by olt, 0 create by  */
	unsigned char is_ready:1;	/* 1 ready, 0 not ready */
	struct attr_t attr[MEINFO_ATTR_ORDER_MAX];
	unsigned char alarm_result_mask[28];	/* 224bit, alarm bits of onu curr state */
	unsigned char alarm_report_mask[28];	/* 224bit, alarm bits already reported to olt */
	unsigned char avc_changed_mask[2]; /* record which attr has been changed in attr set */
	unsigned char pm_is_accumulated_mask[2];	/* record which pm attr has accumulated values */
	struct gettable_state_t *gettable_state_ptr;	/*for get table action */
	unsigned char refcount;	/* for mibupload, alarm clone reference count */
	unsigned int arc_timestamp; //for arc interval, sec
	struct list_head instance_node;	/* a node of list meinfo_t.me_instance_list */
	struct list_head mibupload_clone_node;	/* a node of list meinfo_t.me_mibupload_clone_list */
	struct list_head alarm_clone_node;	/* a node of list meinfo_t.me_alarm_clone_list */
};

/* attribute info */
struct attrinfo_t {
	char *name;
//      unsigned short classid;
	unsigned short byte_size;	/* byte size */
	struct attr_value_t spec_default_value;

	long long lower_limit;
	long long upper_limit;
	long long bit_field_mask;
	struct list_head code_points_list;	/* a list that node is attr_code_point_t */

//      unsigned short order:4;
	unsigned short access_bitmap:3;	// bit1:r, bit2:w, bit3:set_by_create
	unsigned short necessity:2;	// enum attr_necessity_t
	unsigned short format:3;	// enum attr_format_t
	unsigned short vformat:4;	// enum attr_vformat_t, used to help parsing the input and format the output for ipv4 integer and mac str
	unsigned short is_get_refresh:1;
	unsigned short has_upper_limit:1;
	unsigned short has_lower_limit:1;
	unsigned short is_related_key:1;	/* 1: means determinate with other me*/

	unsigned short pointer_classid;
	unsigned char pm_tca_number;
	unsigned char pm_alarm_number;
	struct attrinfo_table_t *attrinfo_table_ptr;
	struct attrinfo_bitfield_t *attrinfo_bitfield_ptr;
};

/* define callback structure for each me classes */
struct meinfo_callback_t {
	unsigned short (*meid_gen_cb) (unsigned short instance_num);
	int (*create_cb) (struct me_t *me, unsigned char attr_mask[2]);	// attr_mask is sbc mask
	int (*delete_cb) (struct me_t *me);
	int (*get_cb)(struct me_t *me, unsigned char attr_mask[2]); //attr_mask is an in/out parameter
	int (*set_cb)(struct me_t *me, unsigned char attr_mask[2]); //attr_mask is an in/out parameter
	int (*table_entry_cmp_cb) (unsigned short classid, unsigned char attr_order, unsigned char *entry1, unsigned char *entry2);
	int (*table_entry_isdel_cb) (unsigned short classid, unsigned char attr_order, unsigned char *entry);
	int (*set_admin_lock_cb)(struct me_t *me, int lockval);
	int (*get_pointer_classid_cb)(struct me_t *me, unsigned char attr_order); 
	int (*alarm_cb)(struct me_t *me, unsigned char alarm_mask[28]); //alarm_mask is an out parameter
	// check attr value before it is written to mib, ret: 1=valid, 0=invalid, -1=valid with warning
	int (*is_attr_valid_cb)(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr);
	// check the lock value of me and its ancestor   
	int (*is_admin_locked_cb)(struct me_t *me);
	// check if a me has arc and arc is set or not
	int (*is_arc_enabled_cb)(struct me_t *me);
	// check if me is ready for er_group, return value, 1: ready, 0: not ready
	int (*is_ready_cb)(struct me_t *me); 
	int (*test_is_supported_cb)(struct me_t *me, unsigned char *req);
	int (*test_cb)(struct me_t *me, unsigned char *req, unsigned char *result);
};

struct meinfo_hardware_t {
	//int (*create_hw) (struct me_t *me);
	//int (*delete_hw) (struct me_t *me);
	int (*get_hw)(struct me_t *me, unsigned char attr_mask[2]); //attr_mask is an in/out parameter
	//int (*set_hw)(struct me_t *me, unsigned char attr_mask[2]); //attr_mask is an in/out parameter
	int (*alarm_hw)(struct me_t *me, unsigned char alarm_mask[28]); //alarm_mask is an out parameter
	// check if me is ready for hw, return value, 1: ready, 0: not ready
	int (*is_ready_hw)(struct me_t *me); 
	int (*pm_reset_hw)(struct me_t *me); // initial counter for pm
	int (*pm_swap_hw)(struct me_t *me); // swap current data and history data of pm
	// check attr value before it is written to mib
	int (*is_attr_valid_hw)(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr);
	int (*test_is_supported_hw)(struct me_t *me, unsigned char *req);
	int (*test_hw)(struct me_t *me, unsigned char *req, unsigned char *result);
};

/* define aliasid to meid mapping under specific classid */
struct meinfo_aliasid2meid_t {
	struct list_head aliasid2meid_node;
	unsigned short aliasid;
	unsigned short meid;
};

/* define configurable information for me class */
struct meinfo_config_t {
//	unsigned short classid;
	unsigned char is_inited:1; /* 1bit, is inited */ ;
	unsigned char support;
	unsigned short instance_total_max;
	struct list_head meinfo_instance_def_list;	/* a list that node is meinfo_instance_def_t */
	struct list_head meinfo_aliasid2meid_list;	/* a list that node is meinfo_aliasid2meid_t */
};

/* define related ready class node */
struct meinfo_related_ready_class_t {
	struct list_head related_ready_class_node;
	unsigned short classid;
};

/* define spec related information for me class */
struct meinfo_t {
	char *name;
	char *section;			/* section of this me in ITU-T G.984.4 */
	unsigned short classid;
	unsigned char is_private:1;	/* hidden from olt in mibupload */
	unsigned char required:2;	/* 1bit, required, conditional required, optional */ ;
	unsigned char attr_total;
	struct attrinfo_t attrinfo[MEINFO_ATTR_ORDER_MAX];
	unsigned int action_mask;	/* 25bit, b4..b28 */
	unsigned char alarm_mask[28];	/* 224bit */
	unsigned char avc_mask[2];	/* 16bit */
	unsigned short avc_polling_mask;	/* 16bit */
	unsigned char test_mask;	/* 8bit, each bit represent the number of select test */
	unsigned char access; 		/* 2bit, enum me_access_type_t */ ;
	struct meinfo_callback_t callback;	/* memory static allcated */
	struct meinfo_hardware_t hardware;	/* memory static allcated */
	struct list_head me_pointer_update_list;	/* a list that node is me_t */
	struct list_head me_instance_list;	/* a list that node is me_t */
	struct list_head me_mibupload_clone_list;	/* a list that node is me_t */
	struct list_head me_alarm_clone_list;	/* a list that node is me_t */
	struct meinfo_config_t config;
	struct list_head er_me_group_definition_ptr_list;	/* a list that node is er_me_group_definition_ptr_node */
	struct list_head related_ready_class_list; /* a list that node is meinfo_related_ready_class_t */
	unsigned char access_create_mask[2];	/* store per class access statistics */
	unsigned char access_read_mask[2];
	unsigned char access_write_mask[2];
};

/*define me callback register function*/
struct meinfo_cb_register_t {
	unsigned short classid;
	int (*cb_register_func)(struct meinfo_callback_t *cb);
};

/* define configurable information for me instance */
struct meinfo_instance_def_t {
	unsigned short instance_num;
	char devname[DEVICE_NAME_MAX_LEN];
	unsigned short default_meid;
	unsigned char pm_is_accumulated_mask[2];
	unsigned char is_private;
	struct attr_value_t custom_default_value[MEINFO_ATTR_ORDER_MAX];
	struct list_head instance_node;	/* a node of list meinfo_config_t.meinfo_instance_def_list */
};

/* a node to store checking callback function of an alarm */
struct alarm_check_func_t {
	unsigned char alarm_num;
	char (*check_func) (struct me_t * me);	/* return, -1: not support, 0: alarm clear, 1: alarm set */
	struct list_head func_node;	/* a node of list meinfo_callback_t.alarm_check_func_list */
};

/* a node that buffers to-be-deleted me for a while to ensure all reader are gone */
struct meinfo_gc_t {
	struct list_head gc_node;
	struct me_t *me;
	int (*postfunc)(struct me_t *me);
	int time;
};

extern struct meinfo_t *meinfo_array[MEINFO_INDEX_TOTAL];

#define meinfo_util_is_proprietary(classid) \
	(classid>=MEINFO_CLASSID_PROPRIETARY_START)

#define meinfo_util_miptr(classid) \
	(meinfo_array[meinfo_classid2index(classid)])

#define meinfo_util_aiptr(classid, attr_order) \
	(&meinfo_util_miptr(classid)->attrinfo[attr_order])

#define meinfo_util_aitab_ptr(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->attrinfo_table_ptr)


#define meinfo_util_get_supported(classid) \
	(meinfo_util_miptr(classid)->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)

#define meinfo_util_get_name(classid) \
	(meinfo_util_miptr(classid)->name)

#define meinfo_util_get_attr_total(classid) \
	(meinfo_util_miptr(classid)->attr_total)

#define meinfo_util_get_action(classid) \
	(meinfo_util_miptr(classid)->action_mask)


#define meinfo_util_attr_get_name(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->name)

#define meinfo_util_attr_get_byte_size(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->byte_size)

#define meinfo_util_attr_is_read(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->access_bitmap & ATTRINFO_ACCESEE_BITMAP_READ)

#define meinfo_util_attr_is_write(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->access_bitmap & ATTRINFO_ACCESEE_BITMAP_WRITE)

#define meinfo_util_attr_is_sbc(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->access_bitmap & ATTRINFO_ACCESEE_BITMAP_SBC)

#define meinfo_util_attr_has_lower_limit(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->has_lower_limit)

#define meinfo_util_attr_has_upper_limit(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->has_upper_limit)

#define meinfo_util_attr_get_lower_limit(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->lower_limit)

#define meinfo_util_attr_get_upper_limit(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->upper_limit)

#define meinfo_util_attr_get_code_points_list(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->code_points_list)

#define meinfo_util_attr_get_format(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->format)

#define meinfo_util_attr_get_is_related_key(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->is_related_key)

#define meinfo_util_attr_get_pointer_classid(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->pointer_classid)

#define meinfo_util_attr_get_pm_tca_number(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->pm_tca_number)

#define meinfo_util_attr_get_pm_alarm_number(classid, attr_order) \
	(meinfo_util_aiptr(classid, attr_order)->pm_alarm_number)

#define meinfo_util_me_attr_data(me, attr_order) \
	(me?me->attr[attr_order].value.data:0)

#define meinfo_util_me_attr_ptr(me, attr_order)	\
	(me?me->attr[attr_order].value.ptr:NULL)

/* meinfo.c */
char *meinfo_util_strerror(int errnum);
unsigned short meinfo_index2classid(unsigned short index);
unsigned short meinfo_classid2index(unsigned short classid);
struct meinfo_t *meinfo_alloc_meinfo(void);
void meinfo_preinit(void);
short meinfo_util_attr_get_table_entry_byte_size(unsigned short classid, unsigned short attr_order);
int meinfo_util_caculate_cmds_num(unsigned short classid, unsigned char capacity);
int meinfo_util_increase_datasync(void);
unsigned char meinfo_util_get_data_sync(void);
struct timeval *meinfo_util_get_data_sync_update_time(void);
int meinfo_util_load_alias_file(char *filename);
int meinfo_util_aliasid2meid(unsigned short classid, unsigned short aliasid);
int meinfo_util_gclist_collect_me(struct me_t *me, int (*postfunc)(struct me_t *me));
int meinfo_util_gclist_aging(int aging_interval);
int meinfo_util_gclist_dump(int fd, int is_detail);
char *meinfo_util_get_config_devname(struct me_t *me);

/* meinfo_config.c */
int meinfo_config_tcont_ts_pq_remap(void);
int meinfo_config_tcont_ts_pq_dump(void);
int meinfo_config_pptp_uni(int admin_lock);
int meinfo_config_card_holder(void);
int meinfo_config_circuit_pack(void);
int meinfo_config_iphost(void);
int meinfo_config_veip(void);
int meinfo_config_set_attr_by_str(unsigned short classid, unsigned short inst_num, unsigned char attr_order, char *str);
int meinfo_config_load_custom_mib(void);
char *meinfo_config_get_str_from_custom_mib(unsigned short classid, unsigned short inst_num, unsigned short attr_order);

/* meinfo_conv.c */
int meinfo_conv_memfields2strlist(unsigned char *mem, int mem_size, struct attrinfo_field_def_t *field_def, int field_total, char *str, int str_size);
int meinfo_conv_attr_to_string(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr, char *str, int str_size);
int meinfo_conv_strlist2memfields(char *mem, int mem_size, struct attrinfo_field_def_t *field_def, int field_total, char *str);
int meinfo_conv_string_to_attr(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr, char *str);
int meinfo_conv_memfields_find(unsigned char *mem, int mem_size, struct attrinfo_field_def_t *field_def, int field_total, char *keyword);
int meinfo_conv_attr_find(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr, char *keyword);

/* meinfo_instance.c */
short meinfo_instance_alloc_free_num(unsigned short classid);
struct meinfo_instance_def_t *meinfo_instance_find_def(unsigned short classid, unsigned short instance_num);
int meinfo_instance_meid_to_num(unsigned short classid, unsigned short meid);
int meinfo_instance_num_to_meid(unsigned short classid, unsigned short instance_num);

/* meinfo_me_attr.c */
int meinfo_me_attr_get(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr);
int meinfo_me_attr_get_pm_current_data(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr);
int meinfo_me_attr_get_pm_hwprev(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr);
int meinfo_me_attr_set(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr, int check_avc);
int meinfo_me_attr_get_pm_is_accumulate(struct me_t *me, unsigned char attr_order, unsigned short *value);
int meinfo_me_attr_swap_pm_data(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_set_pm_current_data(struct me_t *me, unsigned char attr_order, struct attr_value_t attr);
int meinfo_me_attr_set_pm_hwprev(struct me_t *me, unsigned char attr_order, struct attr_value_t attr);
int meinfo_me_attr_update_pm(struct me_t *me, unsigned char attr_order, unsigned long long hw_val);
int meinfo_me_attr_clear_pm(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_time_sync_for_pm(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_set_pm_is_accumulate(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_get_table_entry(struct me_t *me, unsigned char attr_order, struct get_attr_table_context_t *context, void *entrydata);
int meinfo_me_attr_get_table_entry_next(struct me_t *me, unsigned char attr_order, struct get_attr_table_context_t *context, void *entrydata);
int meinfo_me_attr_table_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry_org, unsigned char *entry_new);
int meinfo_me_attr_set_table_entry(struct me_t *me, unsigned char attr_order, void *entrydata, int check_avc);
int meinfo_me_attr_get_table_entry_count(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_get_table_entry_action(struct me_t *me, unsigned char attr_order, void *entrydata);
int meinfo_me_attr_get_table_crc32(struct me_t *me, unsigned char attr_order, unsigned int *crc32);
struct attr_table_entry_t *meinfo_me_attr_locate_table_entry(struct me_t *me, unsigned char attr_order, void *entrydata);
int meinfo_me_attr_delete_table_entry(struct me_t *me, unsigned char attr_order, struct attr_table_entry_t *del_table_entry, int check_avc);
int meinfo_me_attr_pointer_classid(struct me_t *me, unsigned char attr_order);
int meinfo_me_attr_is_valid(struct me_t *me, unsigned short attr_order, struct attr_value_t *attr);
int meinfo_me_attr_release_table_all_entries(struct attr_table_header_t *table_header_ptr);
int meinfo_me_attr_clear_value(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr);
int meinfo_me_attr_release(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr);
int meinfo_me_attr_copy_value(unsigned short classid, unsigned char attr_order, struct attr_value_t *dst_value, struct attr_value_t *src_value);
int meinfo_me_attr_set_table_all_entries_by_callback(struct me_t *me, unsigned char attr_order, int (*set_entry_cb)(struct me_t *me, unsigned char attr_order, unsigned short seq, void *entrydata), int check_avc);
int meinfo_me_attr_set_table_all_entries_by_mem(struct me_t *me, unsigned char attr_order, int memsize, char *memptr, int check_avc);
int meinfo_me_attr_is_table_empty(struct me_t *me, unsigned char attr_order);

/* meinfo_me.c */
int meinfo_me_check_ready_state(struct me_t *me, unsigned char attr_mask[2]);
int meinfo_me_issue_self_er_attr_group(struct me_t *me, int action, unsigned char attr_mask[2]);
int meinfo_me_issue_related_er_attr_group(unsigned short classid);
struct me_t *meinfo_me_alloc(unsigned short classid, unsigned short instance_num);
int meinfo_me_load_config(struct me_t *me);
int meinfo_me_compare_config(struct me_t *me);
int meinfo_me_create(struct me_t *me);
int meinfo_me_release(struct me_t *me);
int meinfo_me_delete_by_classid(unsigned short classid);
int meinfo_me_delete_by_classid_and_cbo(unsigned short classid, unsigned char is_cbo);
int meinfo_me_delete_by_instance_num(unsigned short classid, unsigned short instance_num);
struct me_t *meinfo_me_copy(struct me_t *me);
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
int meinfo_me_copy_on_write(struct me_t *me, struct me_t **me_clone);
#endif
struct me_t *meinfo_me_get_by_meid(unsigned short classid, unsigned short meid);
struct me_t *meinfo_me_get_by_instance_num(unsigned short classid, unsigned short instance_num);
int meinfo_me_is_exist(unsigned short classid, unsigned short meid);
void meinfo_me_determine_tca(unsigned short classid, unsigned short meid);
int meinfo_me_set_child_admin_lock(struct me_t *parent_me, unsigned short child_class, int lockval);
int meinfo_me_is_admin_locked(struct me_t *me);
int meinfo_me_is_arc_enabled(struct me_t *me);
int meinfo_me_is_ready(struct me_t *me);
int meinfo_me_allocate_rare_meid(unsigned short classid, unsigned short *meid);
int meinfo_me_allocate_rare2_meid(unsigned short classid, unsigned short *meid);
int meinfo_me_copy_from_large_string_me(struct me_t *me, unsigned char *buf, unsigned int len);
int meinfo_me_copy_to_large_string_me(struct me_t *me, unsigned char *buf);
int meinfo_me_refresh(struct me_t *me, unsigned char attr_mask[2]);
int meinfo_me_flush(struct me_t *me, unsigned char attr_mask[2]);
int meinfo_me_set_arc_timestamp(struct me_t *me, unsigned char attr_mask[2], unsigned char arc_attr_order, unsigned char arc_interval_attr_order);
int meinfo_me_check_arc_interval(struct me_t *me, unsigned char arc_attr_order, unsigned char arc_interval_attr_order);

#endif
