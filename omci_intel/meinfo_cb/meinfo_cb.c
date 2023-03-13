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
 * Module  : meinfo_cb
 * File    : meinfo_cb.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

extern struct meinfo_callback_t meinfo_cb_11;
extern struct meinfo_callback_t meinfo_cb_128;
extern struct meinfo_callback_t meinfo_cb_12;
extern struct meinfo_callback_t meinfo_cb_130;
extern struct meinfo_callback_t meinfo_cb_131;
extern struct meinfo_callback_t meinfo_cb_134;
extern struct meinfo_callback_t meinfo_cb_137;
extern struct meinfo_callback_t meinfo_cb_138;
extern struct meinfo_callback_t meinfo_cb_139;
extern struct meinfo_callback_t meinfo_cb_140;
extern struct meinfo_callback_t meinfo_cb_141;
extern struct meinfo_callback_t meinfo_cb_142;
extern struct meinfo_callback_t meinfo_cb_143;
extern struct meinfo_callback_t meinfo_cb_144;
extern struct meinfo_callback_t meinfo_cb_145;
extern struct meinfo_callback_t meinfo_cb_146;
extern struct meinfo_callback_t meinfo_cb_147;
extern struct meinfo_callback_t meinfo_cb_148;
extern struct meinfo_callback_t meinfo_cb_149;
extern struct meinfo_callback_t meinfo_cb_150;
extern struct meinfo_callback_t meinfo_cb_151;
extern struct meinfo_callback_t meinfo_cb_152;
extern struct meinfo_callback_t meinfo_cb_153;
extern struct meinfo_callback_t meinfo_cb_154;
extern struct meinfo_callback_t meinfo_cb_155;
extern struct meinfo_callback_t meinfo_cb_156;
extern struct meinfo_callback_t meinfo_cb_157;
extern struct meinfo_callback_t meinfo_cb_162;
extern struct meinfo_callback_t meinfo_cb_171;
extern struct meinfo_callback_t meinfo_cb_24;
extern struct meinfo_callback_t meinfo_cb_256;
extern struct meinfo_callback_t meinfo_cb_262;
extern struct meinfo_callback_t meinfo_cb_263;
extern struct meinfo_callback_t meinfo_cb_264;
extern struct meinfo_callback_t meinfo_cb_266;
extern struct meinfo_callback_t meinfo_cb_267;
extern struct meinfo_callback_t meinfo_cb_268;
extern struct meinfo_callback_t meinfo_cb_276;
extern struct meinfo_callback_t meinfo_cb_277;
extern struct meinfo_callback_t meinfo_cb_281;
extern struct meinfo_callback_t meinfo_cb_282;
extern struct meinfo_callback_t meinfo_cb_287;
extern struct meinfo_callback_t meinfo_cb_288;
extern struct meinfo_callback_t meinfo_cb_289;
extern struct meinfo_callback_t meinfo_cb_290;
extern struct meinfo_callback_t meinfo_cb_296;
extern struct meinfo_callback_t meinfo_cb_298;
extern struct meinfo_callback_t meinfo_cb_301;
extern struct meinfo_callback_t meinfo_cb_302;
extern struct meinfo_callback_t meinfo_cb_303;
extern struct meinfo_callback_t meinfo_cb_304;
extern struct meinfo_callback_t meinfo_cb_305;
extern struct meinfo_callback_t meinfo_cb_306;
extern struct meinfo_callback_t meinfo_cb_309;
extern struct meinfo_callback_t meinfo_cb_310;
extern struct meinfo_callback_t meinfo_cb_311;
extern struct meinfo_callback_t meinfo_cb_312;
extern struct meinfo_callback_t meinfo_cb_313;
extern struct meinfo_callback_t meinfo_cb_314;
extern struct meinfo_callback_t meinfo_cb_315;
extern struct meinfo_callback_t meinfo_cb_316;
extern struct meinfo_callback_t meinfo_cb_321;
extern struct meinfo_callback_t meinfo_cb_322;
extern struct meinfo_callback_t meinfo_cb_413;
extern struct meinfo_callback_t meinfo_cb_45;
extern struct meinfo_callback_t meinfo_cb_46;
extern struct meinfo_callback_t meinfo_cb_47;
extern struct meinfo_callback_t meinfo_cb_48;
extern struct meinfo_callback_t meinfo_cb_49;
extern struct meinfo_callback_t meinfo_cb_50;
extern struct meinfo_callback_t meinfo_cb_52;
extern struct meinfo_callback_t meinfo_cb_53;
extern struct meinfo_callback_t meinfo_cb_58;
extern struct meinfo_callback_t meinfo_cb_5;
extern struct meinfo_callback_t meinfo_cb_67;
extern struct meinfo_callback_t meinfo_cb_6;
extern struct meinfo_callback_t meinfo_cb_7;
extern struct meinfo_callback_t meinfo_cb_75;
extern struct meinfo_callback_t meinfo_cb_78;
extern struct meinfo_callback_t meinfo_cb_79;
extern struct meinfo_callback_t meinfo_cb_80;
extern struct meinfo_callback_t meinfo_cb_82;
extern struct meinfo_callback_t meinfo_cb_83;
extern struct meinfo_callback_t meinfo_cb_89;
extern struct meinfo_callback_t meinfo_cb_90;
extern struct meinfo_callback_t meinfo_cb_91;
extern struct meinfo_callback_t meinfo_cb_98;
extern struct meinfo_callback_t meinfo_cb_425;
extern struct meinfo_callback_t meinfo_cb_334;

int
meinfo_cb_register(unsigned short classid, struct meinfo_callback_t *cb)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	miptr->callback = *cb;

	return 0;
}

int
meinfo_cb_init(void)
{
	meinfo_cb_register(11, &meinfo_cb_11);
	meinfo_cb_register(128, &meinfo_cb_128);
	meinfo_cb_register(12, &meinfo_cb_12);
	meinfo_cb_register(130, &meinfo_cb_130);
	meinfo_cb_register(131, &meinfo_cb_131);
	meinfo_cb_register(134, &meinfo_cb_134);
	meinfo_cb_register(137, &meinfo_cb_137);
	meinfo_cb_register(138, &meinfo_cb_138);
	meinfo_cb_register(139, &meinfo_cb_139);
	meinfo_cb_register(140, &meinfo_cb_140);
	meinfo_cb_register(141, &meinfo_cb_141);
	meinfo_cb_register(142, &meinfo_cb_142);
	meinfo_cb_register(143, &meinfo_cb_143);
	meinfo_cb_register(144, &meinfo_cb_144);
	meinfo_cb_register(145, &meinfo_cb_145);
	meinfo_cb_register(146, &meinfo_cb_146);
	meinfo_cb_register(147, &meinfo_cb_147);
	meinfo_cb_register(148, &meinfo_cb_148);
	meinfo_cb_register(149, &meinfo_cb_149);
	meinfo_cb_register(150, &meinfo_cb_150);
	meinfo_cb_register(151, &meinfo_cb_151);
	meinfo_cb_register(152, &meinfo_cb_152);
	meinfo_cb_register(153, &meinfo_cb_153);
	meinfo_cb_register(154, &meinfo_cb_154);
	meinfo_cb_register(155, &meinfo_cb_155);
	meinfo_cb_register(156, &meinfo_cb_156);
	meinfo_cb_register(157, &meinfo_cb_157);
	meinfo_cb_register(162, &meinfo_cb_162);
	meinfo_cb_register(171, &meinfo_cb_171);
	meinfo_cb_register(24, &meinfo_cb_24);
	meinfo_cb_register(256, &meinfo_cb_256);
	meinfo_cb_register(262, &meinfo_cb_262);
	meinfo_cb_register(263, &meinfo_cb_263);
	meinfo_cb_register(264, &meinfo_cb_264);
	meinfo_cb_register(266, &meinfo_cb_266);
	meinfo_cb_register(267, &meinfo_cb_267);
	meinfo_cb_register(268, &meinfo_cb_268);
	meinfo_cb_register(276, &meinfo_cb_276);
	meinfo_cb_register(277, &meinfo_cb_277);
	meinfo_cb_register(281, &meinfo_cb_281);
	meinfo_cb_register(282, &meinfo_cb_282);
	meinfo_cb_register(287, &meinfo_cb_287);
	meinfo_cb_register(288, &meinfo_cb_288);
	meinfo_cb_register(289, &meinfo_cb_289);
	meinfo_cb_register(290, &meinfo_cb_290);
	meinfo_cb_register(296, &meinfo_cb_296);
	meinfo_cb_register(298, &meinfo_cb_298);
	meinfo_cb_register(301, &meinfo_cb_301);
	meinfo_cb_register(302, &meinfo_cb_302);
	meinfo_cb_register(303, &meinfo_cb_303);
	meinfo_cb_register(304, &meinfo_cb_304);
	meinfo_cb_register(305, &meinfo_cb_305);
	meinfo_cb_register(306, &meinfo_cb_306);
	meinfo_cb_register(309, &meinfo_cb_309);
	meinfo_cb_register(310, &meinfo_cb_310);
	meinfo_cb_register(311, &meinfo_cb_311);
	meinfo_cb_register(312, &meinfo_cb_312);
	meinfo_cb_register(313, &meinfo_cb_313);
	meinfo_cb_register(314, &meinfo_cb_314);
	meinfo_cb_register(315, &meinfo_cb_315);
	meinfo_cb_register(316, &meinfo_cb_316);
	meinfo_cb_register(321, &meinfo_cb_321);
	meinfo_cb_register(322, &meinfo_cb_322);
	meinfo_cb_register(413, &meinfo_cb_413);
	meinfo_cb_register(45, &meinfo_cb_45);
	meinfo_cb_register(46, &meinfo_cb_46);
	meinfo_cb_register(47, &meinfo_cb_47);
	meinfo_cb_register(48, &meinfo_cb_48);
	meinfo_cb_register(49, &meinfo_cb_49);
	meinfo_cb_register(50, &meinfo_cb_50);
	meinfo_cb_register(52, &meinfo_cb_52);
	meinfo_cb_register(53, &meinfo_cb_53);
	meinfo_cb_register(58, &meinfo_cb_58);
	meinfo_cb_register(5, &meinfo_cb_5);
	meinfo_cb_register(67, &meinfo_cb_67);
	meinfo_cb_register(6, &meinfo_cb_6);
	meinfo_cb_register(7, &meinfo_cb_7);
	meinfo_cb_register(75, &meinfo_cb_75);
	meinfo_cb_register(78, &meinfo_cb_78);
	meinfo_cb_register(79, &meinfo_cb_79);
	meinfo_cb_register(80, &meinfo_cb_80);
	meinfo_cb_register(82, &meinfo_cb_82);
	meinfo_cb_register(83, &meinfo_cb_83);
	meinfo_cb_register(89, &meinfo_cb_89);
	meinfo_cb_register(90, &meinfo_cb_90);
	meinfo_cb_register(91, &meinfo_cb_91);
	meinfo_cb_register(98, &meinfo_cb_98);
	meinfo_cb_register(425, &meinfo_cb_425);
	meinfo_cb_register(334, &meinfo_cb_334);

	return 0;
}
