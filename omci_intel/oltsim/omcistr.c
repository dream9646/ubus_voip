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
 * Module  : oltsim
 * File    : omcistr.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

static char *omcistr_msgtype[32] = {
	NULL,	//0 reserved
	NULL,	//1 reserved
	NULL,	//2 reserved
	NULL,	//3 reserved
	"Create",	//4
	"Create complete connection (deprecated)",	//5 Deprecate
	"Delete",	//6
	"Delete complete connection (deprecated)",	//7 Deprecate
	"Set",	//8
	"Get",	//9
	"Get complete connection (deprecated)",	//10 Deprecate
	"Get all alarms",	//11
	"Get all alarms next",	//12
	"MIB upload",	//13
	"MIB upload next",	//14
	"MIB reset",	//15
	"Alarm",	//16
	"Attribute value change",	//17
	"Test",	//18
	"Start software download",	//19
	"Download section",	//20
	"End software download",	//21
	"Activate software",	//22
	"Commit software",	//23
	"Synchronize Time",	//24
	"Reboot",	//25
	"Get next",	//26
	"Test result",	//27
	"Get current data",	//28
	NULL,	//29 reserved
	NULL,	//30 reserved
	NULL,		//31 reserved
};

static unsigned char omcistr_msgtype_is_write_table[32] = {
	0,	//0 reserved
	0,	//1 reserved
	0,	//2 reserved
	0,	//3 reserved
	1,	//4 create
	0,	//5 create complete connection deprecate
	1,	//6 delete
	0,	//7 deltet complete connection deprecate
	1,	//8 set
	0,	//9 get
	0,	//10 get complete connection, deprecate
	0,	//11 get all alarms
	0,	//12 get all alarms next
	0,	//13 mib upload
	0,	//14 mib upload next
	1,	//15 mib reset
	0,	//16 alarm
	0,	//17 attr value change
	0,	//18 test
	1,	//19 start software download
	1,	//20 download section
	1,	//21 end software download
	1,	//22 activate software
	1,	//23 commit software
	1,	//24 synchronize time
	1,	//25 reboot
	0,	//26 get next
	0,	//27 test result
	0,	//28 get current data
	0,	//29 reserved
	0,	//30 reserved
	0,	//31 reserved
};


struct omcistr_classid_t {
	char *section;
	char *name;
};

static struct omcistr_classid_t omcistr_classid[] = {
	{NULL, NULL},	// 0
	{NULL, NULL},	// 1
	{"9.1.3", "ONT data"},	// 2
	{NULL, NULL},	// 3
	{NULL, NULL},	// 4
	{"9.1.5", "Cardholder"},	// 5
	{"9.1.6", "Circuit pack"},	// 6
	{"9.1.4", "Software image"},	// 7
	{NULL, NULL},	// 8
	{NULL, NULL},	// 9
	{NULL, NULL},	// 10
	{"9.5.1", "Physical path termination point Ethernet UNI"},	// 11
	{"9.8.1", "Physical path termination point CES UNI"},	// 12
	{"9.8.2", "Logical N x 64kbit/s sub-port connection termination point"},	// 13
	{"9.13.4", "Interworking VCC termination point"},	// 14
	{NULL, NULL},	// 15
	{"9.13.5", "AAL 5 profile"},	// 16
	{NULL, NULL},	// 17
	{"9.13.6", "AAL 5 performance monitoring history data"},	// 18
	{NULL, NULL},	// 19
	{NULL, NULL},	// 20
	{"9.8.3", "CES service profile-G"},	// 21
	{NULL, NULL},	// 22
	{"9.8.4", "CES physical interface performance monitoring history data"},	// 23
	{"9.5.2", "Ethernet performance monitoring history data"},	// 24
	{"9.13.9", "VP network CTP-G"},	// 25
	{NULL, NULL},	// 26
	{NULL, NULL},	// 27
	{NULL, NULL},	// 28
	{NULL, NULL},	// 29
	{NULL, NULL},	// 30
	{NULL, NULL},	// 31
	{NULL, NULL},	// 32
	{NULL, NULL},	// 33
	{NULL, NULL},	// 34
	{NULL, NULL},	// 35
	{NULL, NULL},	// 36
	{NULL, NULL},	// 37
	{NULL, NULL},	// 38
	{NULL, NULL},	// 39
	{NULL, NULL},	// 40
	{NULL, NULL},	// 41
	{NULL, NULL},	// 42
	{NULL, NULL},	// 43
	{NULL, NULL},	// 44
	{"9.3.1", "MAC bridge service profile"},	// 45
	{"9.3.2", "MAC bridge configuration data"},	// 46
	{"9.3.4", "MAC bridge port configuration data"},	// 47
	{"9.3.5", "MAC bridge port designation data"},	// 48
	{"9.3.6", "MAC bridge port filter table data"},	// 49
	{"9.3.8", "MAC bridge port bridge table data"},	// 50
	{"9.3.3", "MAC bridge performance monitoring history data"},	// 51
	{"9.3.9", "MAC bridge port performance monitoring history data"},	// 52
	{"9.9.1", "Physical path termination point POTS UNI"},	// 53
	{NULL, NULL},	// 54
	{NULL, NULL},	// 55
	{NULL, NULL},	// 56
	{NULL, NULL},	// 57
	{"9.9.6", "Voice service profile"},	// 58
	{NULL, NULL},	// 59
	{NULL, NULL},	// 60
	{NULL, NULL},	// 61
	{"9.13.10", "VP performance monitoring history data"},	// 62
	{NULL, NULL},	// 63
	{NULL, NULL},	// 64
	{NULL, NULL},	// 65
	{NULL, NULL},	// 66
	{"9.4.3", "IP port configuration data"},	// 67
	{"9.4.1", "IP router service profile"},	// 68
	{"9.4.2", "IP router configuration data"},	// 69
	{"9.4.6", "IP router performance monitoring history data 1"},	// 70
	{"9.4.7", "IP router performance monitoring history data 2"},	// 71
	{"9.4.8", "ICMP performance monitoring history data 1"},	// 72
	{"9.4.9", "ICMP performance monitoring history data 2"},	// 73
	{"9.4.4", "IP route table"},	// 74
	{"9.4.5", "IP static routes"},	// 75
	{"9.4.10", "ARP service profile"},	// 76
	{"9.4.11", "ARP configuration data"},	// 77
	{"9.3.12", "VLAN tagging operation configuration data"},	// 78
	{"9.3.7", "MAC bridge port filter preassign table"},	// 79
	{"9.9.21", "Physical path termination point ISDN UNI"},	// 80
	{NULL, NULL},	// 81
	{"9.13.1", "Physical path termination point video UNI"},	// 82
	{"9.13.3", "Physical path termination point LCT UNI"},	// 83
	{"9.3.11", "VLAN tagging filter data"},	// 84
	{NULL, NULL},	// 85
	{NULL, NULL},	// 86
	{NULL, NULL},	// 87
	{NULL, NULL},	// 88
	{"9.5.3", "Ethernet performance monitoring history data 2"},	// 89
	{"9.13.2", "Physical path termination point video ANI"},	// 90
	{"9.6.1", "Physical path termination point 802.11 UNI"},	// 91
	{"9.6.2", "802.11 station management data 1"},	// 92
	{"9.6.3", "802.11 station management data 2"},	// 93
	{"9.6.4", "802.11 general purpose object"},	// 94
	{"9.6.5", "802.11 MAC and PHY operation and antenna data"},	// 95
	{"9.6.7", "802.11 performance monitoring history data"},	// 96
	{"9.6.6", "802.11 PHY FHSS DSSS IR tables"},	// 97
	{"9.7.1", "Physical path termination point xDSL UNI part 1"},	// 98
	{"9.7.2", "Physical path termination point xDSL UNI part 2"},	// 99
	{"9.7.12", "xDSL line inventory and status data part 1"},	// 100
	{"9.7.13", "xDSL line inventory and status data part 2"},	// 101
	{"9.7.19", "xDSL channel downstream status data"},	// 102
	{"9.7.20", "xDSL channel upstream status data"},	// 103
	{"9.7.3", "xDSL line configuration profile part 1"},	// 104
	{"9.7.4", "xDSL line configuration profile part 2"},	// 105
	{"9.7.5", "xDSL line configuration profile part 3"},	// 106
	{"9.7.7", "xDSL channel configuration profile"},	// 107
	{"9.7.8", "xDSL subcarrier masking downstream profile"},	// 108
	{"9.7.9", "xDSL subcarrier masking upstream profile"},	// 109
	{"9.7.10", "xDSL PSD mask profile"},	// 110
	{"9.7.11", "xDSL downstream RFI bands profiles"},	// 111
	{"9.7.21", "xDSL xTU-C performance monitoring history data"},	// 112
	{"9.7.22", "xDSL xTU-R performance monitoring history data"},	// 113
	{"9.7.23", "xDSL xTU-C channel performance monitoring history data"},	// 114
	{"9.7.24", "xDSL xTU-R channel performance monitoring history data"},	// 115
	{"9.7.25", "TC adaptor performance monitoring history data xDSL"},	// 116
	{NULL, NULL},	// 117
	{NULL, NULL},	// 118
	{NULL, NULL},	// 119
	{NULL, NULL},	// 120
	{NULL, NULL},	// 121
	{NULL, NULL},	// 122
	{NULL, NULL},	// 123
	{NULL, NULL},	// 124
	{NULL, NULL},	// 125
	{NULL, NULL},	// 126
	{NULL, NULL},	// 127
	{"9.13.7", "Video return path service profile"},	// 128
	{"9.13.8", "Video return path performance monitoring history data"},	// 129
	{"9.3.10", "802.1p mapper service profile"},	// 130
	{"9.12.2", "OLT-G"},	// 131
	{NULL, NULL},	// 132
	{"9.1.7", "ONT power shedding"},	// 133
	{"9.4.12", "IP host config data"},	// 134
	{"9.4.13", "IP host performance monitoring history data"},	// 135
	{"9.4.14", "TCP/UDP config data"},	// 136
	{"9.12.3", "Network address"},	// 137
	{"9.9.18", "VoIP config data"},	// 138
	{"9.9.4", "VoIP voice CTP"},	// 139
	{"9.9.12", "Call control performance monitoring history data"},	// 140
	{"9.9.11", "VoIP line status"},	// 141
	{"9.9.5", "VoIP media profile"},	// 142
	{"9.9.7", "RTP profile data"},	// 143
	{"9.9.13", "RTP performance monitoring history data"},	// 144
	{"9.9.10", "Network dial plan table"},	// 145
	{"9.9.8", "VoIP application service profile"},	// 146
	{"9.9.9", "VoIP feature access codes"},	// 147
	{"9.12.4", "Authentication security method"},	// 148
	{"9.9.19", "SIP config portal"},	// 149
	{"9.9.3", "SIP agent config data"},	// 150
	{"9.9.14", "SIP agent performance monitoring history data"},	// 151
	{"9.9.15", "SIP call initiation performance monitoring history data"},	// 152
	{"9.9.2", "SIP user data"},	// 153
	{"9.9.20", "MGC config portal"},	// 154
	{"9.9.16", "MGC config data"},	// 155
	{"9.9.17", "MGC performance monitoring history data"},	// 156
	{"9.12.5", "Large string"},	// 157
	{"9.1.12", "ONT remote debug"},	// 158
	{"9.1.11", "Equipment protection profile"},	// 159
	{"9.1.9", "Equipment extension package"},	// 160
	{NULL, NULL},	// 161
	{"9.10.1", "Physical path termination point MoCA UNI"},	// 162
	{"9.10.2", "MoCA Ethernet performance monitoring history data"},	// 163
	{"9.10.3", "MoCA interface performance monitoring history data"},	// 164
	{"9.7.6", "VDSL2 line configuration extensions"},	// 165
	{"9.7.14", "xDSL line inventory and status data part 3"},	// 166
	{"9.7.15", "xDSL line inventory and status data part 4"},	// 167
	{"9.7.16", "VDSL2 line inventory and status data part 1"},	// 168
	{"9.7.17", "VDSL2 line inventory and status data part 2"},	// 169
	{"9.7.18", "VDSL2 line inventory and status data part 3"},	// 170
	{"9.3.13", "Extended VLAN tagging operation configuration data"},	// 171
	{NULL, NULL},	// 172
	{NULL, NULL},	// 173
	{NULL, NULL},	// 174
	{NULL, NULL},	// 175
	{NULL, NULL},	// 176
	{NULL, NULL},	// 177
	{NULL, NULL},	// 178
	{NULL, NULL},	// 179
	{NULL, NULL},	// 180
	{NULL, NULL},	// 181
	{NULL, NULL},	// 182
	{NULL, NULL},	// 183
	{NULL, NULL},	// 184
	{NULL, NULL},	// 185
	{NULL, NULL},	// 186
	{NULL, NULL},	// 187
	{NULL, NULL},	// 188
	{NULL, NULL},	// 189
	{NULL, NULL},	// 190
	{NULL, NULL},	// 191
	{NULL, NULL},	// 192
	{NULL, NULL},	// 193
	{NULL, NULL},	// 194
	{NULL, NULL},	// 195
	{NULL, NULL},	// 196
	{NULL, NULL},	// 197
	{NULL, NULL},	// 198
	{NULL, NULL},	// 199
	{NULL, NULL},	// 200
	{NULL, NULL},	// 201
	{NULL, NULL},	// 202
	{NULL, NULL},	// 203
	{NULL, NULL},	// 204
	{NULL, NULL},	// 205
	{NULL, NULL},	// 206
	{NULL, NULL},	// 207
	{NULL, NULL},	// 208
	{NULL, NULL},	// 209
	{NULL, NULL},	// 210
	{NULL, NULL},	// 211
	{NULL, NULL},	// 212
	{NULL, NULL},	// 213
	{NULL, NULL},	// 214
	{NULL, NULL},	// 215
	{NULL, NULL},	// 216
	{NULL, NULL},	// 217
	{NULL, NULL},	// 218
	{NULL, NULL},	// 219
	{NULL, NULL},	// 220
	{NULL, NULL},	// 221
	{NULL, NULL},	// 222
	{NULL, NULL},	// 223
	{NULL, NULL},	// 224
	{NULL, NULL},	// 225
	{NULL, NULL},	// 226
	{NULL, NULL},	// 227
	{NULL, NULL},	// 228
	{NULL, NULL},	// 229
	{NULL, NULL},	// 230
	{NULL, NULL},	// 231
	{NULL, NULL},	// 232
	{NULL, NULL},	// 233
	{NULL, NULL},	// 234
	{NULL, NULL},	// 235
	{NULL, NULL},	// 236
	{NULL, NULL},	// 237
	{NULL, NULL},	// 238
	{NULL, NULL},	// 239
	{NULL, NULL},	// 240
	{NULL, NULL},	// 241
	{NULL, NULL},	// 242
	{NULL, NULL},	// 243
	{NULL, NULL},	// 244
	{NULL, NULL},	// 245
	{NULL, NULL},	// 246
	{NULL, NULL},	// 247
	{NULL, NULL},	// 248
	{NULL, NULL},	// 249
	{NULL, NULL},	// 250
	{NULL, NULL},	// 251
	{NULL, NULL},	// 252
	{NULL, NULL},	// 253
	{NULL, NULL},	// 254
	{NULL, NULL},	// 255
	{"9.1.1", "ONT-G"},	// 256
	{"9.1.2", "ONT2-G"},	// 257
	{NULL, NULL},	// 258
	{NULL, NULL},	// 259
	{NULL, NULL},	// 260
	{NULL, NULL},	// 261
	{"9.2.2", "T-CONT"},	// 262
	{"9.2.1", "ANI-G"},	// 263
	{"9.12.1", "UNI-G"},	// 264
	{NULL, NULL},	// 265
	{"9.2.4", "GEM interworking termination point"},	// 266
	{"9.2.6", "GEM port performance monitoring history data"},	// 267
	{"9.2.3", "GEM port network CTP"},	// 268
	{NULL, NULL},	// 269
	{NULL, NULL},	// 270
	{"9.2.9", "GAL TDM profile"},	// 271
	{"9.2.7", "GAL Ethernet profile"},	// 272
	{"9.12.6", "Threshold data 1"},	// 273
	{"9.12.7", "Threshold data 2"},	// 274
	{"9.2.10", "GAL TDM performance monitoring history data"},	// 275
	{"9.2.8", "GAL Ethernet performance monitoring history data"},	// 276
	{"9.11.1", "Priority queue-G"},	// 277
	{"9.11.2", "Traffic scheduler-G"},	// 278
	{"9.1.10", "Protection data"},	// 279
	{"9.11.3", "GEM traffic descriptor"},	// 280
	{"9.2.5", "Multicast GEM interworking termination point"},	// 281
	{"9.8.5", "Pseudowire termination point"},	// 282
	{"9.8.6", "RTP pseudowire parameters"},	// 283
	{"9.8.7", "Pseudowire maintenance profile"},	// 284
	{"9.8.8", "Pseudowire performance monitoring history data"},	// 285
	{"9.8.9", "Ethernet flow termination point"},	// 286
	{"9.12.8", "OMCI"},	// 287
	{"9.12.9", "Managed entity"},	// 288
	{"9.12.10", "Attribute"},	// 289
	{"9.3.14", "Dot1X port extension package"},	// 290
	{"9.3.15", "Dot1X configuration profile"},	// 291
	{"9.3.16", "Dot1X performance monitoring history data"},	// 292
	{"9.3.17", "Radius performance monitoring history data"},	// 293
	{"9.8.10", "TU CTP"},	// 294
	{"9.8.11", "TU performance monitoring history data"},	// 295
	{"9.5.4", "Ethernet performance monitoring history data 3"},	// 296
	{"9.1.8", "Port mapping package-G"},	// 297
	{"9.3.18", "Dot1 rate limiter"},	// 298
	{"9.3.19", "Dot1ag maintenance domain"},	// 299
	{"9.3.20", "Dot1ag maintenance association"},	// 300
	{"9.3.21", "Dot1ag default MD level"},	// 301
	{"9.3.22", "Dot1ag MEP"},	// 302
	{"9.3.23", "Dot1ag MEP status"},	// 303
	{"9.3.24", "Dot1ag MEP CCM database"},	// 304
	{"9.3.25", "Dot1ag CFM stack"},	// 305
	{"9.3.26", "Dot1ag chassis-management info"},	// 306
	{"9.12.11", "Octet string"},	// 307
	{"9.12.12", "General purpose buffer"},	// 308
	{"9.3.27", "Multicast operations profile"},	// 309
	{"9.3.28", "Multicast subscriber config info"},	// 310
	{"9.3.29", "Multicast subscriber monitor"},	// 311
	{"9.2.11", "FEC performance monitoring history data"},	// 312
	{"9.14.1", "RE ANI-G"},	// 313
	{"9.14.2", "Physical path termination point RE UNI"},	// 314
	{"9.14.3", "RE upstream amplifier"},	// 315
	{"9.14.4", "RE downstream amplifier"},	// 316
	{"9.14.5", "RE config portal"},	// 317
	{"9.12.13", "File transfer controller"},	// 318
	{"9.8.12", "CES physical interface performance monitoring history data 2"},	// 319
	{"9.8.13", "CES physical interface performance monitoring history data 3"},	// 320
	{"9.3.31", "Ethernet frame performance monitoring history data downstream"},	// 321
	{"9.3.30", "Ethernet frame performance monitoring history data upstream"},	// 322
	{"9.7.26", "VDSL2 line configuration extensions 2"},	// 323
	{"9.7.27", "xDSL impulse noise monitor performance monitoring history data"},	// 324
	{"9.7.28", "xDSL line inventory and status data part 5"},	// 325
	{"9.7.29", "xDSL line inventory and status data part 6"},	// 326
	{"9.7.30", "xDSL line inventory and status data part 7"},	// 327
	{"9.14.6", "RE common amplifier parameters"},	// 328
        {"9.3.32", "Ethernet frame extended PM"},   //334
        {"9.3.24", "Ethernet frame extended PM 64-Bit"},   // 425
        {"9.12.17", "Threshold data 3"}, // 426
	{NULL, NULL},	// 329
};

char *
omcistr_classid2name(unsigned short classid)
{
	char *name=NULL;
	if (classid <=329)
		name=omcistr_classid[classid].name;
	if (name==NULL)
		return "proprietary class?";
	return name;
}
		
char *
omcistr_classid2section(unsigned short classid)
{
	char *section=NULL;
	if (classid <=329)
		section=omcistr_classid[classid].section;
	if (section==NULL)
		return "proprietary?";
	return section;
}
		
char *
omcistr_msgtype2name(unsigned char msgtype)
{
	char *name=omcistr_msgtype[msgtype&0x1f];
	if (name==NULL)
		return "unknow action";
	return name;
}

int
omcistr_msgtype_is_write(unsigned char msgtype)
{
 	return  omcistr_msgtype_is_write_table[msgtype];
}

