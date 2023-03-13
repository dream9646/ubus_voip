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
 * Module  : me_related
 * File    : me_related.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <assert.h>
#include <string.h>
#include "me_related.h"
#include "util.h"
#include "list.h"

static struct list_head me_related_hash[ME_RELATED_HASH_SIZE];

void 
me_related_preinit(void)
{
	int i;
	for (i = 0; i < ME_RELATED_HASH_SIZE; i++)
		INIT_LIST_HEAD(&me_related_hash[i]);
}

static inline unsigned int 
hash_position(unsigned short class1, unsigned short class2)
{
	unsigned int hash=0, seed=131; // 31 131 1313 13131 131313 etc..;

	//key generator; BKDRHash
	hash = (hash * seed) + class1;
	hash = (hash * seed) + class2;
	return (hash % ME_RELATED_HASH_SIZE);
}

struct me_related_entity_t *
me_related_find(unsigned short class1, unsigned short class2)
{
	struct me_related_entity_t *me_related_entity;
	unsigned int position=hash_position(class1, class2);

	list_for_each_entry(me_related_entity, &me_related_hash[position], entity_node) {
		if(class1 == me_related_entity->class1 && class2 == me_related_entity->class2)
			return me_related_entity;
	}
	return NULL;
}

int 
me_related_add(unsigned char functype, unsigned short class1, unsigned short class2, 
		int (*related_func) (struct me_t *me1, struct me_t *me2) )
{
	struct me_related_entity_t *entity;
	unsigned int position;

	if(functype != ME_RELATED_FUNC_SPEC && functype != ME_RELATED_FUNC_HW) {
                dbprintf(LOG_ERR, "functype error\n");
                return 1;
	}

	// check if entity already exist
	entity=me_related_find(class1, class2);
	if (entity) {
		if (functype==ME_RELATED_FUNC_SPEC && entity->spec_related_func==NULL) {
			entity->spec_related_func = related_func;
			return 0;
		} else if (functype==ME_RELATED_FUNC_HW && entity->hw_related_func==NULL) {
			entity->hw_related_func = related_func;
			return 0;
		} else {
			return -1;
		}
	}

	// create entity
	entity = malloc_safe(sizeof (struct me_related_entity_t));
	entity->class1 = class1;
	entity->class2 = class2;

	if(functype == ME_RELATED_FUNC_SPEC)
		entity->spec_related_func = related_func;
	else
		entity->hw_related_func = related_func;

	position=hash_position(class1, class2);
	list_add_tail(&entity->entity_node, &me_related_hash[position]);
	return 0;
}

int 
me_related_del(unsigned short class1, unsigned short class2)
{
	struct me_related_entity_t *me_related_entity, *tmp_entity;
	unsigned int position=0;

	position=hash_position(class1, class2);
	list_for_each_entry_safe(me_related_entity, tmp_entity,  &me_related_hash[position], entity_node) {
		if(class1 == me_related_entity->class1 && class2 == me_related_entity->class2) {
			list_del(&me_related_entity->entity_node);
			free_safe(me_related_entity);
			return 0;
		}
	}
	return 1;
}

int 
me_related_dump(int fd)
{
	struct me_related_entity_t *me_related_entity;
	int i;

	for (i=0; i < ME_RELATED_HASH_SIZE; i++){
		util_fdprintf(fd, "Hash key=%d\n", i);
		list_for_each_entry(me_related_entity, &me_related_hash[i], entity_node) {
			util_fdprintf(fd, "\t\t(%d,%d)\n",me_related_entity->class1,me_related_entity->class2);
		}
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR,0,"\n");
	return 0;
}

static int 
me_related_default(struct me_t *me1, struct me_t *me2)
{
	dbprintf(LOG_INFO,"Can' found me_related function with class:%d, %d; meid: %d, %d\n",
		me1->classid, me2->classid, me1->meid, me2->meid);
	return 0;	//not related
}

int 
me_related(struct me_t *me1, struct me_t *me2)
{
	struct me_related_entity_t *entity;

	if (me1==NULL || me2==NULL) {
		dbprintf(LOG_ERR, "me%d ptr null?\n", (me1==NULL)?1:2);
		return 0;
	}

	entity=me_related_find(me1->classid, me2->classid);

	if (!entity)
		return me_related_default(me1, me2);
	if (entity->spec_related_func && entity->spec_related_func(me1, me2))
		return 1;	//1: related, 0: not related
	if (entity->hw_related_func && entity->hw_related_func(me1, me2))
		return 1;	//1: related, 0: not related
	return 0;
}

// if me_related(a, b) return true, which means
// 1. a related to b
// 2. a has a pointer to b
// 3. a is the child of b
struct me_t *
me_related_find_parent_me(struct me_t *child_me, unsigned short parent_classid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(parent_classid);
	struct me_t *me;
	
	if (miptr==NULL)
		return NULL;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(child_me, me))
			return me;
	}
	return NULL;
}

struct me_t *
me_related_find_child_me(struct me_t *parent_me, unsigned short child_classid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(child_classid);
	struct me_t *me;
	
	if (miptr==NULL)
		return NULL;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, parent_me))
			return me;
	}
	return NULL;
}

// relation to direction upstream
int 
me_related_277_upstream(struct me_t *me)
{
	return (me->meid&0x8000);	// msb==1 means upstream
}

int 
me_related_init(void)
{
	me_related_add(ME_RELATED_FUNC_SPEC, 2, 256, me_related_002_256);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 6, me_related_005_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 7, me_related_005_007);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 11, me_related_005_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 45, me_related_005_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 262, me_related_005_262);
	me_related_add(ME_RELATED_FUNC_SPEC, 5, 263, me_related_005_263);

	me_related_add(ME_RELATED_FUNC_SPEC, 6, 5, me_related_006_005);
	me_related_add(ME_RELATED_FUNC_SPEC, 6, 11, me_related_006_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 6, 45, me_related_006_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 6, 47, me_related_006_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 6, 256, me_related_006_256);
	me_related_add(ME_RELATED_FUNC_SPEC, 6, 278, me_related_006_278);

	me_related_add(ME_RELATED_FUNC_SPEC, 7, 5, me_related_007_005);
	me_related_add(ME_RELATED_FUNC_SPEC, 7, 256, me_related_007_256);

	me_related_add(ME_RELATED_FUNC_SPEC, 11, 5, me_related_011_005);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 6, me_related_011_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 24, me_related_011_024);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 47, me_related_011_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 264, me_related_011_264);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 290, me_related_011_290);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 296, me_related_011_296);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 334, me_related_011_334);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 425, me_related_011_425);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 349, me_related_011_349);

	me_related_add(ME_RELATED_FUNC_SPEC, 24, 11, me_related_024_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 24, 273, me_related_024_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 24, 274, me_related_024_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 45, 5, me_related_045_005);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 6, me_related_045_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 46, me_related_045_046);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 47, me_related_045_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 51, me_related_045_051);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 130, me_related_045_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 298, me_related_045_298);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 301, me_related_045_301);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 305, me_related_045_305);

	me_related_add(ME_RELATED_FUNC_SPEC, 46, 45, me_related_046_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 46, 298, me_related_046_298);

	me_related_add(ME_RELATED_FUNC_SPEC, 47, 6, me_related_047_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 45, me_related_047_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 11, me_related_047_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 130, me_related_047_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 134, me_related_047_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 266, me_related_047_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 280, me_related_047_280);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 281, me_related_047_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 329, me_related_047_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 48, me_related_047_048);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 49, me_related_047_049);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 79, me_related_047_079);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 50, me_related_047_050);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 52, me_related_047_052);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 84, me_related_047_084);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 302, me_related_047_302);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 310, me_related_047_310);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 311, me_related_047_311);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 321, me_related_047_321);
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 322, me_related_047_322);

	me_related_add(ME_RELATED_FUNC_SPEC, 48, 47, me_related_048_047);

	me_related_add(ME_RELATED_FUNC_SPEC, 49, 47, me_related_049_047);

	me_related_add(ME_RELATED_FUNC_SPEC, 50, 47, me_related_050_047);

	me_related_add(ME_RELATED_FUNC_SPEC, 51, 45, me_related_051_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 51, 273, me_related_051_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 51, 274, me_related_051_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 52, 47, me_related_052_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 52, 273, me_related_052_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 52, 274, me_related_052_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 53, 140, me_related_053_140);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 141, me_related_053_141);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 144, me_related_053_144);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 266, me_related_053_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 273, me_related_053_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 274, me_related_053_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 78, 11, me_related_078_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 130, me_related_078_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 134, me_related_078_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 47, me_related_078_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 266, me_related_078_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 281, me_related_078_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 78, 329, me_related_078_329);

	me_related_add(ME_RELATED_FUNC_SPEC, 79, 47, me_related_079_047);

	me_related_add(ME_RELATED_FUNC_SPEC, 84, 47, me_related_084_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 84, 300, me_related_084_300);

	me_related_add(ME_RELATED_FUNC_SPEC, 89, 11, me_related_089_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 89, 273, me_related_089_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 89, 274, me_related_089_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 130, 11, me_related_130_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 45, me_related_130_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 266, me_related_130_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 277, me_related_130_277);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 298, me_related_130_298);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 301, me_related_130_301);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 302, me_related_130_302);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 305, me_related_130_305);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 310, me_related_130_310);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 311, me_related_130_311);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 329, me_related_130_329);

	me_related_add(ME_RELATED_FUNC_SPEC, 131, 256, me_related_131_256);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 133, 256, me_related_133_256);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 136, 134, me_related_136_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 136, 329, me_related_136_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 137, 148, me_related_137_148);
	me_related_add(ME_RELATED_FUNC_SPEC, 137, 157, me_related_137_157);
	me_related_add(ME_RELATED_FUNC_SPEC, 137, 340, me_related_137_340);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 139, 153, me_related_139_153);
	me_related_add(ME_RELATED_FUNC_SPEC, 139, 53, me_related_139_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 139, 142, me_related_139_142);

	me_related_add(ME_RELATED_FUNC_SPEC, 140, 53, me_related_140_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 140, 273, me_related_140_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 140, 274, me_related_140_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 141, 53, me_related_141_053);

	me_related_add(ME_RELATED_FUNC_SPEC, 142, 58, me_related_142_058);
	me_related_add(ME_RELATED_FUNC_SPEC, 142, 143, me_related_142_143);

	me_related_add(ME_RELATED_FUNC_SPEC, 144, 53, me_related_144_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 144, 273, me_related_144_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 144, 274, me_related_144_274);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 148, 137, me_related_148_137);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 150, 136, me_related_150_136);
	me_related_add(ME_RELATED_FUNC_SPEC, 150, 151, me_related_150_151);
	me_related_add(ME_RELATED_FUNC_SPEC, 150, 152, me_related_150_152);
	me_related_add(ME_RELATED_FUNC_SPEC, 150, 273, me_related_150_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 150, 274, me_related_150_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 151, 150, me_related_151_150);
	me_related_add(ME_RELATED_FUNC_SPEC, 151, 273, me_related_151_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 151, 274, me_related_151_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 152, 150, me_related_152_150);
	me_related_add(ME_RELATED_FUNC_SPEC, 152, 273, me_related_152_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 152, 274, me_related_152_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 153, 53, me_related_153_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 153, 145, me_related_153_145);
	me_related_add(ME_RELATED_FUNC_SPEC, 153, 146, me_related_153_146);
	me_related_add(ME_RELATED_FUNC_SPEC, 153, 147, me_related_153_147);
	me_related_add(ME_RELATED_FUNC_SPEC, 153, 150, me_related_153_150);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 157, 137, me_related_157_137);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 160, 256, me_related_160_256);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 47, me_related_171_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 130, me_related_171_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 11, me_related_171_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 266, me_related_171_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 281, me_related_171_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 171, 329, me_related_171_329);

	me_related_add(ME_RELATED_FUNC_SPEC, 256, 2, me_related_256_002);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 6, me_related_256_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 7, me_related_256_007);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 131, me_related_256_131);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 133, me_related_256_133);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 160, me_related_256_160);
	me_related_add(ME_RELATED_FUNC_SPEC, 256, 257, me_related_256_257);

	me_related_add(ME_RELATED_FUNC_SPEC, 257, 256, me_related_257_256);

	me_related_add(ME_RELATED_FUNC_SPEC, 262, 5, me_related_262_005);

	me_related_add(ME_RELATED_FUNC_SPEC, 263, 5, me_related_263_005);
	me_related_add(ME_RELATED_FUNC_SPEC, 263, 312, me_related_263_312);

	me_related_add(ME_RELATED_FUNC_SPEC, 264, 11, me_related_264_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 264, 329, me_related_264_329);

	me_related_add(ME_RELATED_FUNC_SPEC, 266, 268, me_related_266_268);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 21, me_related_266_021);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 45, me_related_266_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 68, me_related_266_068);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 128, me_related_266_128);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 130, me_related_266_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 11, me_related_266_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 12, me_related_266_012);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 13, me_related_266_013);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 294, me_related_266_294);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 271, me_related_266_271);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 272, me_related_266_272);

	me_related_add(ME_RELATED_FUNC_SPEC, 267, 268, me_related_267_268);
	me_related_add(ME_RELATED_FUNC_SPEC, 267, 273, me_related_267_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 267, 274, me_related_267_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 268, 262, me_related_268_262);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 267, me_related_268_267);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 277, me_related_268_277);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 280, me_related_268_280);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 329, me_related_268_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 401, me_related_268_401);
	me_related_add(ME_RELATED_FUNC_SPEC, 268, 402, me_related_268_402);

	me_related_add(ME_RELATED_FUNC_SPEC, 272, 276, me_related_272_276);

	me_related_add(ME_RELATED_FUNC_SPEC, 276, 272, me_related_276_272);
	me_related_add(ME_RELATED_FUNC_SPEC, 276, 273, me_related_276_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 276, 274, me_related_276_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 277, 11, me_related_277_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 130, me_related_277_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 262, me_related_277_262);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 278, me_related_277_278);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 329, me_related_277_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 401, me_related_277_401);
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 402, me_related_277_402);

	me_related_add(ME_RELATED_FUNC_SPEC, 278, 6, me_related_278_006);
	me_related_add(ME_RELATED_FUNC_SPEC, 278, 262, me_related_278_262);
	me_related_add(ME_RELATED_FUNC_SPEC, 278, 278, me_related_278_278);

	me_related_add(ME_RELATED_FUNC_SPEC, 280, 298, me_related_280_298);
	me_related_add(ME_RELATED_FUNC_SPEC, 280, 401, me_related_280_401);
	me_related_add(ME_RELATED_FUNC_SPEC, 280, 402, me_related_280_402);
	me_related_add(ME_RELATED_FUNC_SPEC, 280, 403, me_related_280_403);
	me_related_add(ME_RELATED_FUNC_SPEC, 280, 404, me_related_280_404);

	me_related_add(ME_RELATED_FUNC_SPEC, 281, 268, me_related_281_268);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 45, me_related_281_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 68, me_related_281_068);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 130, me_related_281_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 272, me_related_281_272);

	me_related_add(ME_RELATED_FUNC_SPEC, 290, 11, me_related_290_011);

	me_related_add(ME_RELATED_FUNC_SPEC, 296, 11, me_related_296_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 296, 273, me_related_296_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 296, 274, me_related_296_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 298, 45, me_related_298_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 298, 46, me_related_298_046);
	me_related_add(ME_RELATED_FUNC_SPEC, 298, 130, me_related_298_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 298, 280, me_related_298_280);

	me_related_add(ME_RELATED_FUNC_SPEC, 299, 300, me_related_299_300);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 300, 84, me_related_300_084);
	me_related_add(ME_RELATED_FUNC_SPEC, 300, 299, me_related_300_299);
	me_related_add(ME_RELATED_FUNC_SPEC, 300, 302, me_related_300_302);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 301, 45, me_related_301_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 301, 130, me_related_301_130);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 302, 47, me_related_302_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 302, 130, me_related_302_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 302, 300, me_related_302_300);
	me_related_add(ME_RELATED_FUNC_SPEC, 302, 303, me_related_302_303);
	me_related_add(ME_RELATED_FUNC_SPEC, 302, 304, me_related_302_304);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 303, 302, me_related_303_302);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 304, 302, me_related_304_302);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 305, 45, me_related_305_045);
	me_related_add(ME_RELATED_FUNC_SPEC, 305, 130, me_related_305_130);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 309, 310, me_related_309_310);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 310, 47, me_related_310_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 310, 130, me_related_310_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 310, 309, me_related_310_309);

	me_related_add(ME_RELATED_FUNC_SPEC, 311, 47, me_related_311_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 311, 130, me_related_311_130);

	me_related_add(ME_RELATED_FUNC_SPEC, 312, 263, me_related_312_263);
	me_related_add(ME_RELATED_FUNC_SPEC, 312, 273, me_related_312_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 312, 274, me_related_312_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 321, 47, me_related_321_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 321, 273, me_related_321_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 321, 274, me_related_321_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 322, 47, me_related_322_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 322, 273, me_related_322_273);
	me_related_add(ME_RELATED_FUNC_SPEC, 322, 274, me_related_322_274);

	me_related_add(ME_RELATED_FUNC_SPEC, 329, 47, me_related_329_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 134, me_related_329_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 136, me_related_329_136);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 268, me_related_329_268);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 330, me_related_329_330);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 340, me_related_329_340);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 330, 329, me_related_330_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 334, 11, me_related_334_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 334, 274, me_related_334_274);
	me_related_add(ME_RELATED_FUNC_SPEC, 334, 273, me_related_334_273);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 340, 137, me_related_340_137);
	me_related_add(ME_RELATED_FUNC_SPEC, 340, 329, me_related_340_329);
        	
	me_related_add(ME_RELATED_FUNC_SPEC, 349, 11, me_related_349_011);

	me_related_add(ME_RELATED_FUNC_SPEC, 425, 11, me_related_425_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 425, 426, me_related_425_426);
	
	return 0;
}
