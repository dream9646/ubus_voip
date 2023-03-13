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
 * Module  : meinfo_hw
 * File    : meinfo_hw.c
 *
 ******************************************************************/

#include "meinfo_hw.h"
#include "util.h"

extern struct meinfo_hardware_t meinfo_hw_11;
extern struct meinfo_hardware_t meinfo_hw_131;
extern struct meinfo_hardware_t meinfo_hw_134;
extern struct meinfo_hardware_t meinfo_hw_136;
extern struct meinfo_hardware_t meinfo_hw_138;
extern struct meinfo_hardware_t meinfo_hw_139;
extern struct meinfo_hardware_t meinfo_hw_140;
extern struct meinfo_hardware_t meinfo_hw_141;
extern struct meinfo_hardware_t meinfo_hw_142;
extern struct meinfo_hardware_t meinfo_hw_143;
extern struct meinfo_hardware_t meinfo_hw_144;
extern struct meinfo_hardware_t meinfo_hw_145;
extern struct meinfo_hardware_t meinfo_hw_146;
extern struct meinfo_hardware_t meinfo_hw_147;
extern struct meinfo_hardware_t meinfo_hw_148;
extern struct meinfo_hardware_t meinfo_hw_149;
extern struct meinfo_hardware_t meinfo_hw_150;
extern struct meinfo_hardware_t meinfo_hw_151;
extern struct meinfo_hardware_t meinfo_hw_152;
extern struct meinfo_hardware_t meinfo_hw_153;
extern struct meinfo_hardware_t meinfo_hw_157;
extern struct meinfo_hardware_t meinfo_hw_171;
extern struct meinfo_hardware_t meinfo_hw_24;
extern struct meinfo_hardware_t meinfo_hw_256;
extern struct meinfo_hardware_t meinfo_hw_257;
extern struct meinfo_hardware_t meinfo_hw_262;
extern struct meinfo_hardware_t meinfo_hw_263;
extern struct meinfo_hardware_t meinfo_hw_267;
extern struct meinfo_hardware_t meinfo_hw_268;
extern struct meinfo_hardware_t meinfo_hw_276;
extern struct meinfo_hardware_t meinfo_hw_277;
extern struct meinfo_hardware_t meinfo_hw_290;
extern struct meinfo_hardware_t meinfo_hw_296;
extern struct meinfo_hardware_t meinfo_hw_312;
extern struct meinfo_hardware_t meinfo_hw_321;
extern struct meinfo_hardware_t meinfo_hw_322;
extern struct meinfo_hardware_t meinfo_hw_330;
extern struct meinfo_hardware_t meinfo_hw_334;
extern struct meinfo_hardware_t meinfo_hw_341;
extern struct meinfo_hardware_t meinfo_hw_349;
extern struct meinfo_hardware_t meinfo_hw_45;
extern struct meinfo_hardware_t meinfo_hw_46;
extern struct meinfo_hardware_t meinfo_hw_47;
extern struct meinfo_hardware_t meinfo_hw_48;
extern struct meinfo_hardware_t meinfo_hw_425;
extern struct meinfo_hardware_t meinfo_hw_50;
extern struct meinfo_hardware_t meinfo_hw_52;
extern struct meinfo_hardware_t meinfo_hw_53;
extern struct meinfo_hardware_t meinfo_hw_58;
extern struct meinfo_hardware_t meinfo_hw_6;
extern struct meinfo_hardware_t meinfo_hw_7;
extern struct meinfo_hardware_t meinfo_hw_82;
extern struct meinfo_hardware_t meinfo_hw_89;
extern struct meinfo_hardware_t meinfo_hw_90;
extern struct meinfo_hardware_t meinfo_hw_137;

int
meinfo_hw_register(unsigned short classid, struct meinfo_hardware_t *hw)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	miptr->hardware= *hw;

	return 0;
}

int
meinfo_hw_init(void)
{
	meinfo_hw_register(11, &meinfo_hw_11);
	meinfo_hw_register(131, &meinfo_hw_131);
	meinfo_hw_register(134, &meinfo_hw_134);
	meinfo_hw_register(171, &meinfo_hw_171);
	meinfo_hw_register(24, &meinfo_hw_24);
	meinfo_hw_register(256, &meinfo_hw_256);
	meinfo_hw_register(257, &meinfo_hw_257);
	meinfo_hw_register(262, &meinfo_hw_262);
	meinfo_hw_register(263, &meinfo_hw_263);
	meinfo_hw_register(267, &meinfo_hw_267);
	meinfo_hw_register(268, &meinfo_hw_268);
	meinfo_hw_register(276, &meinfo_hw_276);
	meinfo_hw_register(277, &meinfo_hw_277);
	meinfo_hw_register(290, &meinfo_hw_290);
	meinfo_hw_register(296, &meinfo_hw_296);
	meinfo_hw_register(312, &meinfo_hw_312);
	meinfo_hw_register(321, &meinfo_hw_321);
	meinfo_hw_register(322, &meinfo_hw_322);
	meinfo_hw_register(330, &meinfo_hw_330);
	meinfo_hw_register(334, &meinfo_hw_334);
	meinfo_hw_register(341, &meinfo_hw_341);
	meinfo_hw_register(349, &meinfo_hw_349);
	meinfo_hw_register(45, &meinfo_hw_45);
	meinfo_hw_register(46, &meinfo_hw_46);
	meinfo_hw_register(47, &meinfo_hw_47);
	meinfo_hw_register(48, &meinfo_hw_48);
	meinfo_hw_register(425, &meinfo_hw_425);
	meinfo_hw_register(50, &meinfo_hw_50);
	meinfo_hw_register(52, &meinfo_hw_52);
	meinfo_hw_register(6, &meinfo_hw_6);
	meinfo_hw_register(7, &meinfo_hw_7);
	meinfo_hw_register(82, &meinfo_hw_82);
	meinfo_hw_register(89, &meinfo_hw_89);
	meinfo_hw_register(90, &meinfo_hw_90);
	meinfo_hw_register(53, &meinfo_hw_53);
	meinfo_hw_register(143, &meinfo_hw_143);
	meinfo_hw_register(150, &meinfo_hw_150);
	meinfo_hw_register(153, &meinfo_hw_153);
	meinfo_hw_register(137, &meinfo_hw_137);
	meinfo_hw_register(141, &meinfo_hw_141);
	meinfo_hw_register(142, &meinfo_hw_142);
	meinfo_hw_register(145, &meinfo_hw_145);
	meinfo_hw_register(58, &meinfo_hw_58);
	meinfo_hw_register(146, &meinfo_hw_146);
	meinfo_hw_register(138, &meinfo_hw_138);
	return 0;
}
