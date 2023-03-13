/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF MANAGER
 * Module  : util
 * File    : crc.c
 *
 ******************************************************************/

#include <stdio.h>

// NOTICE!
// while netowrk byte order is big endian, 
// the bit ordering in per byte for ethernet is little endian
//
// see http://http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
//
// Bit ordering (Endianness)
// 
// When implemented in bit serial hardware, the generator polynomial uniquely
// describes the bit assignment; the first bit transmitted is always the
// coefficient of the highest power of x, and the last n bits transmitted are
// the CRC remainder R(x), starting with the coefficient of x^{n-1} and ending
// with the coefficient of x^0, a.k.a. the coefficient of 1.
// 
// However, when bits are processed a byte at a time, such as when using
// parallel transmission, byte framing as in 8B/10B encoding or RS-232-style
// asynchronous serial communication, or when implementing a CRC in software,
// it is necessary to specify the bit ordering (endianness) of the data; which
// bit in each byte is considered "first" and will be the coefficient of the
// higher power of x.
// 
// If the data is destined for serial communication, it is best to use the bit
// ordering the data will ultimately be sent in. This is because a CRC's
// ability to detect burst errors is based on proximity in the message
// polynomial M(x); if adjacent polynomial terms are not transmitted
// sequentially, a physical error burst of one length may be seen as a longer
// burst due to the rearrangement of bits.
// 
// For example, both IEEE 802 (ethernet) and RS-232 (serial port) standards
// specify least-significant bit first (little-endian) transmission, so a
// software CRC implementation to protect data sent across such a link should
// map the least significant bits in each byte to coefficients of the highest
// powers of x. On the other hand, floppy disks and most hard drives write the
// most significant bit of each byte first.
// 
// The lsbit-first CRC is slightly simpler to implement in software, so is
// somewhat more commonly seen, but many programmers find the msbit-first bit
// ordering easier to follow. Thus, for example, the XMODEM-CRC extension, an
// early use of CRCs in software, uses an msbit-first CRC.

// ethernet crc polunomial: x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0

#define CRCPOLY_LE 0xedb88320
#define CRCPOLY_BE 0x04c11db7
#define LE_TABLE_SIZE 256
#define BE_TABLE_SIZE 256

// slow version, just for reference //////////////////////////////////////////////////////////////////////////////////////////////

unsigned int 
crc_le_slow(unsigned int crc, unsigned char const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return crc;
}

unsigned int 
crc_be_slow(unsigned int crc, unsigned char const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 24;
		for (i = 0; i < 8; i++)
			crc =
			    (crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE :
					  0);
	}
	return crc;
}

// table lookup version //////////////////////////////////////////////////////////////////////////////////////////////

/**
 * crc32init_le() - allocate and initialize LE table data
 *
 * crc is the crc of the byte i; other entries are filled in based on the
 * fact that crctable[i^j] = crctable[i] ^ crctable[j].
 *
 */
static unsigned int crc32table_le[LE_TABLE_SIZE];
static int is_crc32table_le_init=0;
static void
crc32table_le_init(void)
{
	unsigned i, j;
	unsigned int crc = 1;

	crc32table_le[0] = 0;

	for (i = 1 << (8 - 1); i; i >>= 1) {
		crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
		for (j = 0; j < LE_TABLE_SIZE; j += 2 * i)
			crc32table_le[i + j] = crc ^ crc32table_le[j];
	}
	is_crc32table_le_init=1;
}

/**
 * crc32table_be_init() - allocate and initialize BE table data
 */
static unsigned int crc32table_be[BE_TABLE_SIZE];
static int is_crc32table_be_init=0;
static void
crc32table_be_init(void)
{
	unsigned i, j;
	unsigned int crc = 0x80000000;

	crc32table_be[0] = 0;

	for (i = 1; i < BE_TABLE_SIZE; i <<= 1) {
		crc = (crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE : 0);
		for (j = 0; j < i; j++)
			crc32table_be[i + j] = crc ^ crc32table_be[j];
	}
	is_crc32table_be_init=1;
}

/* update the CRC on the data block one byte at a time. */
unsigned int
crc_le_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
	register char *s = data_blk_ptr;
	if (!is_crc32table_le_init)
		crc32table_le_init();
	while (--data_blk_size >= 0)
		crc_accum = (crc_accum >> 8) ^ crc32table_le[(crc_accum ^ *s++) & 0xff];
	return crc_accum;
}

unsigned int
crc_be_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
	register char *s = data_blk_ptr;
	if (!is_crc32table_be_init)
		crc32table_be_init();
	while (--data_blk_size >= 0)
		crc_accum = (crc_accum << 8) ^ crc32table_be[((crc_accum >> 24) ^ *s++) & 0xff];
	return crc_accum;
}

int
crc_file_check(unsigned int *crc_accum, char *filename, unsigned char is_big_endian)
{
	FILE    *fptr;
	int	ch;

	if (!is_crc32table_be_init)
		crc32table_be_init();
	if (!is_crc32table_le_init)
		crc32table_le_init();

	if ( (fptr=fopen(filename,"r")) == NULL) {
		// dbprintf(LOG_ERR, "can't open file %s!\n", filename);
		return -1;
	}
	while( (ch=fgetc(fptr)) != EOF ) {
		if ( !is_big_endian ) {	// big endian
			*crc_accum = (*crc_accum >> 8) ^ crc32table_le[(*crc_accum ^ ch) & 0xff] ;
		} else {
			*crc_accum = (*crc_accum << 8) ^ crc32table_be[((*crc_accum >> 24) ^ ch) & 0xff];
		}
	}
	fclose(fptr);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CRC_MAIN

// some reference for crc_le_update_by_be_table
// http://blog.csdn.net/delacroix_xu/article/details/7589333
// http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code

unsigned long long
bits_reflect(unsigned long long orig, unsigned char total_bits)
{
	unsigned long long value = 0;
	int i;
	for (i = 1; i < (total_bits + 1); i++) {
		if (orig & 1)
			value |= 1 << (total_bits - i);
		orig >>= 1;
	}
	return value;
}

// this is a little bit slower than crc_le_update
unsigned int
crc_le_update_by_be_table(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size)
{
	register char *s = data_blk_ptr;
	if (!is_crc32table_be_init)
		crc32table_be_init();
	while (--data_blk_size >= 0)
		crc_accum = (crc_accum << 8) ^ crc32table_be[((crc_accum >> 24) ^ bits_reflect(*s++,8)) & 0xff];
	return crc_accum;
}

int
main(int argc, char **argv)
{
	char zero[60] = {0};
	char ones[60] = {0};
	unsigned char tx_data[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1f,	//8  
	0x29, 0x00, 0xb5, 0xfa, 0x08, 0x06, 0x00, 0x01,	//15  
	0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0x1f,	//24  
	0x29, 0x00, 0xb5, 0xfa, 0xac, 0x15, 0x0e, 0xd9,	//32  
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xac, 0x15,	//40  
	0x0e, 0x8e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	//48  
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	//56  
	0x00, 0x00, 0x00, 0x00, 0x8b, 0x6b, 0xf5, 0x13	//64  
	};

	int i;
	unsigned int accum_be;
	unsigned int accum_le;
	unsigned int accum_le_by_be_table;

	for (i=0; i<60; i++)
		ones[i] = 0xff;

	crc32table_le_init();
	crc32table_be_init();

	printf("crc32 le table: polynomial:%08x\n", CRCPOLY_LE);
	for (i=0; i<256; i++) {
		printf("%08x ", crc32table_le[i]);
		if (i%8==3)
			printf(" ");
		if (i%8==7)
			printf("\n");
	}
	printf("\n");

	printf("crc32 be table: polynomial:%08x (bits_reflect of %08x)\n", CRCPOLY_BE, CRCPOLY_LE);
	for (i=0; i<256; i++) {
		printf("%08x ", crc32table_be[i]);
		if (i%8==3)
			printf(" ");
		if (i%8==7)
			printf("\n");
	}
	printf("\n");

	accum_be = crc_be_update(0xffffffff, zero, 60);
	accum_le = crc_le_update(0xffffffff, zero, 60);
	accum_le_by_be_table = crc_le_update_by_be_table(0xffffffff, zero, 60);
	printf("zero[60]:\n");
	printf("accum_be = 0x%08x\n", accum_be);
	printf("accum_le = 0x%08x\n", accum_le);
	printf("accum_le_by_betable = 0x%08x\n", accum_le_by_be_table);
	printf("crc = ~accum_le = 0x%08x\n", ~accum_le);
	printf("crc = ~bits_reflect(accum_le_by_be_table) = 0x%08x\n", ~bits_reflect(accum_le_by_be_table, 32));
	printf("\n");

	accum_be = crc_be_update(0xffffffff, ones, 60);
	accum_le = crc_le_update(0xffffffff, ones, 60);
	accum_le_by_be_table = crc_le_update_by_be_table(0xffffffff, ones, 60);
	printf("ones[60]:\n");
	printf("accum_be = 0x%08x\n", accum_be);
	printf("accum_le = 0x%08x\n", accum_le);
	printf("accum_le_by_betable = 0x%08x\n", accum_le_by_be_table);
	printf("crc = ~accum_le = 0x%08x\n", ~accum_le);
	printf("crc = ~bits_reflect(accum_le_by_be_table) = 0x%08x\n", ~bits_reflect(accum_le_by_be_table, 32));
	printf("\n");

	accum_be = crc_be_update(0xffffffff, tx_data, 60);
	accum_le = crc_le_update(0xffffffff, tx_data, 60);
	accum_le_by_be_table = crc_le_update_by_be_table(0xffffffff, tx_data, 60);
	printf("tx_data[60]:\n");
	printf("accum_be = 0x%08x\n", accum_be);
	printf("accum_le = 0x%08x\n", accum_le);
	printf("accum_le_by_betable = 0x%08x\n", accum_le_by_be_table);
	printf("crc = ~accum_le = 0x%08x\n", ~accum_le);
	printf("crc = ~bits_reflect(accum_le_by_be_table) = 0x%08x\n", ~bits_reflect(accum_le_by_be_table, 32));
	printf("\n");

	printf("frame tx_data crc should be %02x %02x %02x %02x\n", 
		tx_data[60], tx_data[61], tx_data[62], tx_data[63]);
}
#endif
