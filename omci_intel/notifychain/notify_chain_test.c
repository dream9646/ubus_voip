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
 * Module  : notifychain
 * File    : notify_chain_test.c
 *
 ******************************************************************/

#include <stdio.h>
#include <syslog.h>
#include "notify_chain.h"
#include "me_createdel_table.h"
#include "notify_chain_cb.h"
#include "meinfo.h"
#include "util.h"

extern struct list_head me_createdel_table_list;

struct me_t me_test[384];
struct meinfo_t meinfo_test[384];

int
cb_class256(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 256 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class001(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 001 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class002(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 002 instance_num=%d\n", instance_num);
	//me_pointer_resolve_list_add(me_test[2].pointer_unresolv_node);
	return 0;
}

int
cb_class003(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 003 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class004(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 004 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class005(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 005 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class006(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 006 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class007(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 007 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class008(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 008 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class009(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 009 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class010(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 010 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class011(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 011 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class012(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 012 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class013(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 013 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class014(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 014 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class015(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 015 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class016(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 016 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class017(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 017 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class018(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 018 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class019(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 019 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class020(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 020 instance_num=%d\n", instance_num);
	return 0;
}

int
cb_class021(unsigned short instance_num)
{
	dbprintf(LOG_INFO, "Do cb_class 021 instance_num=%d\n", instance_num);
	return 0;
}

int
me_createdel_create_handler15(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 15) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler15: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
me_createdel_create_handler16(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 16) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler16: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
me_createdel_create_handler17(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 17) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler17: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
me_createdel_create_handler18(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 18) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler18: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
me_createdel_create_handler19(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 19) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler19: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
me_createdel_create_handler20(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE && classid == 20) {
		dbprintf(LOG_INFO,
			 "Do me_createdel_create_handler20: event=%d/classid=%u, instance_num=%d",
			 event, classid, instance_num);
		me_createdel_traverse_table(event, classid, instance_num, meid, data_ptr);
	}
	return 0;
}

int
main()
{
	notify_chain_preinit();
	if (me_createdel_table_load("../etc/omci/00base/me_chain/me_chain.conf") != 0) {
		dbprintf(LOG_ERR, "parse ../etc/omci/00base/me_chain.conf fail!");
		return 1;
	}

	dbprintf(LOG_INFO, "1:\n");
	me_createdel_table_dump();

	dbprintf(LOG_INFO, "2:hook callback function");

#if 0
	meinfo_test[256].callback.init = cb_class256;	//todo, need to check again
	//meinfo_test[0].callback.init=cb_class000;     //todo, need to check again
	meinfo_test[1].callback.init = cb_class001;	//todo, need to check again
	meinfo_test[2].callback.init = cb_class002;	//todo, need to check again
	meinfo_test[3].callback.init = cb_class003;	//todo, need to check again
	meinfo_test[4].callback.init = cb_class004;	//todo, need to check again
	meinfo_test[5].callback.init = cb_class005;	//todo, need to check again
	meinfo_test[6].callback.init = cb_class006;	//todo, need to check again
	meinfo_test[7].callback.init = cb_class007;	//todo, need to check again
	meinfo_test[8].callback.init = cb_class008;	//todo, need to check again
	meinfo_test[9].callback.init = cb_class009;	//todo, need to check again
	meinfo_test[10].callback.init = cb_class010;	//todo, need to check again
	meinfo_test[11].callback.init = cb_class011;	//todo, need to check again
	meinfo_test[12].callback.init = cb_class012;	//todo, need to check again
	meinfo_test[13].callback.init = cb_class013;	//todo, need to check again
	meinfo_test[14].callback.init = cb_class014;	//todo, need to check again
	meinfo_test[15].callback.init = cb_class015;	//todo, need to check again
	meinfo_test[16].callback.init = cb_class016;	//todo, need to check again
	meinfo_test[17].callback.init = cb_class017;	//todo, need to check again
	meinfo_test[18].callback.init = cb_class018;	//todo, need to check again
	meinfo_test[19].callback.init = cb_class019;	//todo, need to check again
	meinfo_test[20].callback.init = cb_class020;	//todo, need to check again
	meinfo_test[21].callback.init = cb_class021;	//todo, need to check again
#endif

	//notify_chain_register(NOTIFY_CHAIN_EVENT_BOOT, me_createdel_boot_handler, NULL);
	//notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler, NULL);
	//notify_chain_register(NOTIFY_CHAIN_EVENT_SHUTDOWN, me_createdel_shutdown_handler, NULL);

	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler15, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler16, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler17, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler18, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler19, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler20, NULL);

	dbprintf(LOG_INFO, "3:");
	notify_chain_notify(NOTIFY_CHAIN_EVENT_BOOT, 256, 1, 30);

	dbprintf(LOG_INFO, "4:");
	notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_CREATE, 5, 1, 20);
	//notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler, NULL);

	dbprintf(LOG_INFO, "5:");
	//notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler, NULL);
	notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_CREATE, 1, 3, 40);

	dbprintf(LOG_INFO, "6:");
	notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_CREATE, 15, 1, 7);
	dbprintf(LOG_INFO, "7:");
	notify_chain_notify(NOTIFY_CHAIN_EVENT_SHUTDOWN, 256, 1, 30);
	dbprintf(LOG_INFO, "8:");
	me_createdel_table_free();
	dbprintf(LOG_INFO, "9:\n");

	//me_test[2]

	dbprintf(LOG_INFO, "9:\n");

	return 0;
}
