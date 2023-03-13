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
 * File    : prbs.h
 *
 ******************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "prbs.h"

// PRBS generator:
// prbs7  = x^{7}  + x^{6}  + 1
// prbs15 = x^{15} + x^{14} + 1
// prbs23 = x^{23} + x^{18} + 1
// prbs31 = x^{31} + x^{28} + 1

unsigned char
prbs7(unsigned char seed, int len, char *buff)
{
	int newbit, i, j;
	unsigned char byte;
	
	for (i = 0; i<len; i++) {
		byte = 0;
		for (j=0; j<8 ; j++) {
			newbit = (((seed >> 6) ^ (seed >> 5)) & 1);		
			seed = ((seed << 1) | newbit) & 0x7f;
			byte = (byte>>1) | newbit<<7;	// the first generated bit will be finally be the lsb of the byte
		}
		buff[i] = byte;
	}
	return seed;
}

// according to linux-3.8/ arch/ arm/ mach-berlin/ modules/ nfc/ prbs15.c 
// when using 0x576a as seed
// the bit stream is 1111 0011 0111 1110 0010 1011 0000 0100 1111 1010 0001 1010 0001 1100 0101 1100...
// the byte sequence is 0xCF, 0x7E, 0xD4, 0x20, 0x5F, 0x58, 0x38, 0x3A,
unsigned short
prbs15(unsigned short seed, int len, char *buff)
{
	int newbit, i, j;
	unsigned char byte;
	
	for (i = 0; i<len; i++) {
		byte = 0;
		for (j=0; j<8 ; j++) {
			newbit = (((seed >> 14) ^ (seed >> 13)) & 1);		
			seed = ((seed << 1) | newbit) & 0x7fff;
#ifdef PRBS_MAIN
			printf("%02x, bit %d\n", seed, newbit);
#endif			
			byte = (byte>>1) | newbit<<7;	// the first generated bit will be finally be the lsb of the byte
		}
#ifdef PRBS_MAIN
		printf("byte=%02x\n", byte);
#endif
		buff[i] = byte;
	}
	return seed;
}

unsigned int
prbs23(unsigned int seed, int len, char *buff)
{
	int newbit, i, j;
	unsigned char byte;
	
	for (i = 0; i<len; i++) {
		byte = 0;
		for (j=0; j<8 ; j++) {
			newbit = (((seed >> 22) ^ (seed >> 17)) & 1);		
			seed = ((seed << 1) | newbit) & 0x7fffff;
			byte = (byte>>1) | newbit<<7;	// the first generated bit will be finally be the lsb of the byte
		}
		buff[i] = byte;
	}
	return seed;
}

unsigned int
prbs31(unsigned int seed, int len, char *buff)
{
	int newbit, i, j;
	unsigned char byte;
	
	for (i = 0; i<len; i++) {
		byte = 0;
		for (j=0; j<8 ; j++) {
			newbit = (((seed >> 30) ^ (seed >> 27)) & 1);		
			seed = ((seed << 1) | newbit) & 0x7fffffff;
			byte = (byte >>1) | newbit<<7;	// the first generated bit will be finally be the lsb of the byte
		}
		buff[i] = byte;
	}
	return seed;
}

// PRBS checker:
// for prbsN, any continus N bit is actually the seed of next 1 bit 
// so we use fir N bit to init seed, and verify the stream from bitN+1...
// ps: if there is a bit error in the stream, then there could be at most 3 error reported
//     one from errbit being treated as result, the other two from errbit be treated as generator

int
prbs7_checker(int len, char *buff)
{
	unsigned char seed = 0;
	int newbit, databit, i, err = 0, cont_ok = 0;
	
	if (len*8 <=7)
		return -1;
	for (i=0; i<7; i++) {	// seed = first 7bit in reverse order
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		seed = ((seed << 1) | databit) & 0x7f;
	}
	for (i = 7; i < len *8; i++) {
		newbit = (((seed >> 6) ^ (seed >> 5)) & 1);	// expect value
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		if (databit != newbit) {
#ifdef PRBS_MAIN
			if (cont_ok>7 && cont_ok-4 <=7)
				printf("i=%d, char[%d]=0x%02x, seed become UNreliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif
			cont_ok -= 4;	// when err rate > 1/4, the cont_ok will become 0 finally
			if (cont_ok <0)
				cont_ok = 0;
			err++;
		} else {
#ifdef PRBS_MAIN
			if (cont_ok == 7)
				printf("i=%d, char[%d]=0x%02x, seed become reliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif				
			cont_ok++;
			if (cont_ok>256)
				cont_ok =256;	// cont max 256, so 256/4 err will clear cont_ok
		}
		// cont_ok>7 mean seed is reliable
		// if seed is reliable, next bit comes from seed; or next bit comes from data stream
		seed = ((seed << 1) | ((cont_ok>7)?newbit:databit)) & 0x7f;
	}
	return err;
}

int
prbs15_checker(int len, char *buff)
{
	unsigned short seed = 0;
	int newbit, databit, i, err = 0, cont_ok = 0;
	
	if (len*8 <=15)
		return -1;
	for (i=0; i<15; i++) {	// seed = first 15bit in reverse order
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		seed = ((seed << 1) | databit) & 0x7fff;
	}
	for (i = 15; i < len *8; i++) {
		newbit = (((seed >> 14) ^ (seed >> 13)) & 1);	// expect value
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		if (databit != newbit) {
#ifdef PRBS_MAIN
			if (cont_ok>15 && cont_ok-4 <=15)
				printf("i=%d, char[%d]=0x%02x, seed become UNreliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif
			cont_ok -= 4;	// when err rate > 1/4, the cont_ok will become 0 finally
			if (cont_ok <0)
				cont_ok = 0;
			err++;
		} else {
#ifdef PRBS_MAIN
			if (cont_ok == 15)
				printf("i=%d, char[%d]=0x%02x, seed become reliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif				
			cont_ok++;
			if (cont_ok>256)
				cont_ok =256;	// cont max 256, so 256/4 err will clear cont_ok
		}
		// cont_ok>15 mean seed is reliable
		// if seed is reliable, next bit comes from seed; or next bit comes from data stream
		seed = ((seed << 1) | ((cont_ok>15)?newbit:databit)) & 0x7fff;
	}
	return err;
}

int
prbs23_checker(int len, char *buff)
{
	unsigned int seed = 0;
	int newbit, databit, i, err = 0, cont_ok = 0;
	
	if (len*8 <=23)
		return -1;
	for (i=0; i<23; i++) {	// seed = first 23bit in reverse order
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		seed = ((seed << 1) | databit) & 0x7fffff;
	}
	for (i = 23; i < len *8; i++) {
		newbit = (((seed >> 22) ^ (seed >> 17)) & 1);	// expect value
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		if (databit != newbit) {
#ifdef PRBS_MAIN
			if (cont_ok>23 && cont_ok-4 <=23)
				printf("i=%d, char[%d]=0x%02x, seed become UNreliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif
			cont_ok -= 4;	// when err rate > 1/4, the cont_ok will become 0 finally
			if (cont_ok <0)
				cont_ok = 0;
			err++;
		} else {
#ifdef PRBS_MAIN
			if (cont_ok == 23)
				printf("i=%d, char[%d]=0x%02x, seed become reliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif				
			cont_ok++;
			if (cont_ok>256)
				cont_ok =256;	// cont max 256, so 256/4 err will clear cont_ok
		}
		// cont_ok>23 mean seed is reliable
		// if seed is reliable, next bit comes from seed; or next bit comes from data stream
		seed = ((seed << 1) | ((cont_ok>23)?newbit:databit)) & 0x7fffff;
	}
	return err;
}

int
prbs31_checker(int len, char *buff)
{
	unsigned int seed = 0;
	int newbit, databit, i, err = 0, cont_ok = 0;
	
	if (len*8 <=31)
		return -1;
	for (i=0; i<31; i++) {	// seed = first 31bit in reverse order
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		seed = ((seed << 1) | databit) & 0x7fffffff;
	}
	for (i = 31; i < len *8; i++) {
		newbit = (((seed >> 30) ^ (seed >> 27)) & 1);	// expect value
		databit = (buff[i/8] >> (i%8)) & 1;		// real value in stream
		if (databit != newbit) {
#ifdef PRBS_MAIN
			if (cont_ok>31 && cont_ok-4 <=31)
				printf("i=%d, char[%d]=0x%02x, seed become UNreliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif
			cont_ok -= 4;	// when err rate > 1/4, the cont_ok will become 0 finally
			if (cont_ok <0)
				cont_ok = 0;
			err++;
		} else {
#ifdef PRBS_MAIN
			if (cont_ok == 31)
				printf("i=%d, char[%d]=0x%02x, seed become reliable\n", i, i/8, (unsigned char)buff[i/8]);
#endif				
			cont_ok++;
			if (cont_ok>256)
				cont_ok =256;	// cont max 256, so 256/4 err will clear cont_ok
		}
		// cont_ok>31 mean seed is reliable
		// if seed is reliable, next bit comes from seed; or next bit comes from data stream
		seed = ((seed << 1) | ((cont_ok>31)?newbit:databit)) & 0x7fffffff;
	}
	return err;
}


#ifdef PRBS_MAIN
int
main(int argc, char *argv[])
{
	unsigned char buff[1024];
	unsigned int seed = 0x576a;
	int err, i;

	// prbs generator
	seed = prbs31(0x576a, 1024, buff);

	for (i=0; i<32; i++)
		printf("%02x ", buff[i]);
	printf("\n");
	// prbs checker
	err = prbs31_checker(1024, buff);
		printf("err=%d\n", err);
	
	buff[25] = 0x27;	
	for (i=0; i<32; i++)
		printf("%02x ", buff[i]);
	printf("\n");
	err = prbs31_checker(1024, buff);
		printf("err=%d\n", err);		

	buff[18] = 0x27;	
	buff[25] = 0x27;	
	for (i=0; i<32; i++)
		printf("%02x ", buff[i]);
	printf("\n");
	err = prbs31_checker(1024, buff);
		printf("err=%d\n", err);		

	buff[6] = 0x27;	
	buff[18] = 0x27;	
	buff[25] = 0x27;	
	for (i=0; i<32; i++)
		printf("%02x ", buff[i]);
	printf("\n");
	err = prbs31_checker(1024, buff);
		printf("err=%d\n", err);		
}
#endif
