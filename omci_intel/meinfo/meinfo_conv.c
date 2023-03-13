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
 * File    : meinfo_conv.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
                                          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "meinfo.h"
#include "notify_chain.h"
#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "conv.h"

int
meinfo_conv_memfields2strlist(unsigned char *mem, int mem_size,
			struct attrinfo_field_def_t *field_def, int field_total, 
			char *str, int str_size)
{
	int i;
	int start_bit=0;
	char *str_ptr=str;	

	for (i=0; i<field_total; i++) {
		struct attrinfo_field_def_t *f=&field_def[i];
		unsigned long long udata;
		char buff[64];
		char *delimeter=(i>0)?", ":"";

		switch (f->vformat) {
		case ATTR_VFORMAT_POINTER:
		case ATTR_VFORMAT_UNSIGNED_INT:
		case ATTR_VFORMAT_ENUMERATION:
			// the util_bitmap_get function will convert value from network order to host order
			udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);
			sprintf(buff, "0x%llx(%llu)", udata, udata);
			break;
		case ATTR_VFORMAT_SIGNED_INT:
			// the util_bitmap_get function will convert value from network order to host order
			udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);
			if (f->bit_width<=8) {
				sprintf(buff, "0x%x(%d)", (char)udata, (char)udata);
			} else if (f->bit_width<=16) {
				sprintf(buff, "0x%x(%d)", (short)udata, (short)udata);
			} else if (f->bit_width<=32) {
				sprintf(buff, "0x%x(%d)", (int)udata, (int)udata);
			} else {
				sprintf(buff, "0x%llx(%lld)", udata, udata);
			}
			break;
		case ATTR_VFORMAT_STRING:
			if (f->bit_width/8<64) {
				memcpy(buff, mem+start_bit/8, f->bit_width/8);
				buff[f->bit_width/8]=0;
			} else {
				memcpy(buff, mem+start_bit/8, 63);
				buff[63]=0;
			}
			break;
		case ATTR_VFORMAT_MAC:
			if (f->bit_width==48) {
				unsigned char m[6];			
				memcpy(m, mem+start_bit/8, f->bit_width/8);				
				sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", 
					m[0],m[1],m[2],m[3],m[4],m[5]);
			} else {
				sprintf(buff, "??:??:??:??:??:??");
			}	
			break;
		case ATTR_VFORMAT_IPV4:
			// the util_bitmap_get function will convert value from network order to host order
			udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);
			if (f->bit_width==32) {
				struct in_addr in;
				in.s_addr=htonl(udata);	// conv udata to network order for inet_ntoa
				sprintf(buff, "%s", inet_ntoa(in));
			} else {
				sprintf(buff, "???.???.???.???");
			}
			break;
		case ATTR_VFORMAT_IPV6:
			if (f->bit_width==128) {
				unsigned char ipv6_str[INET6_ADDRSTRLEN];
				if ( NULL == util_inet_ntop(AF_INET6, mem+start_bit/8, (char *)ipv6_str, sizeof(ipv6_str) ) )
					sprintf(buff, "????:????:????:????:????:????:????:????");
				sprintf(buff, "%s", ipv6_str);
			} else {
				sprintf(buff, "????:????:????:????:????:????:????:????");
			}
			break;
		default:	// table/bitfield in table/bitfield?
			sprintf(buff, "err?(%d)", f->vformat);
			break;
		}

		// check if str has enough space
		if (str_size-strlen(str)<=strlen(delimeter)+strlen(buff))
			break;
		// copy string to str
		str_ptr+=sprintf(str_ptr, "%s%s", delimeter, buff);

		start_bit+=f->bit_width;
	}

	return strlen(str);
}

int
meinfo_conv_attr_to_string(unsigned short classid, unsigned short attr_order, 
			struct attr_value_t *attr, char *str, int str_size)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct attrinfo_t *a;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, attr_order);
		return MEINFO_ERROR_CLASSID_NOT_SUPPORT;
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), classid, attr_order);
		return MEINFO_ERROR_ATTR_UNDEF;
	}

	a = &miptr->attrinfo[attr_order];
	switch(a->format) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_ENUMERATION:
		snprintf(str, str_size, "0x%llx(%llu)", attr->data, attr->data);
		break;
	case ATTR_FORMAT_UNSIGNED_INT:
		if (a->vformat == ATTR_VFORMAT_IPV4) {
			if (a->byte_size==4) {
				struct in_addr in;
				in.s_addr=htonl(attr->data);	// conv udata to network order for inet_ntoa
				snprintf(str, str_size, "%s", inet_ntoa(in));
			} else {
				snprintf(str, str_size, "???.???.???.???");
			}	
		} else {
			if (a->byte_size==1) {
				snprintf(str, str_size, "0x%x(%u)", (unsigned char)attr->data, (unsigned char)attr->data);
			} else if (a->byte_size==2) {
				snprintf(str, str_size, "0x%x(%u)", (unsigned short)attr->data, (unsigned short)attr->data);
			} else if (a->byte_size<=4) {
				snprintf(str, str_size, "0x%x(%u)", (unsigned int)attr->data, (unsigned int)attr->data);
			} else {
				snprintf(str, str_size, "0x%llx(%llu)", (unsigned long long)attr->data, (unsigned long long)attr->data);
			}
		}
		break;
	case ATTR_FORMAT_SIGNED_INT:
		if (a->byte_size==1) {
			snprintf(str, str_size, "0x%x(%d)", (unsigned char)attr->data, (char)attr->data);
		} else if (a->byte_size==2) {
			snprintf(str, str_size, "0x%x(%d)", (unsigned short)attr->data, (short)attr->data);
		} else if (a->byte_size<=4) {
			snprintf(str, str_size, "0x%x(%d)", (unsigned int)attr->data, (int)attr->data);
		} else {
			snprintf(str, str_size, "0x%llx(%lld)", (unsigned long long)attr->data, attr->data);
		}
		break;
	case ATTR_FORMAT_STRING:
		if (a->vformat == ATTR_VFORMAT_MAC) {
			if (attr->ptr) {
				if (a->byte_size==6) {
					unsigned char *m;
					m = (unsigned char *)attr->ptr;
					snprintf(str, str_size, "%02X:%02X:%02X:%02X:%02X:%02X", 
						m[0],m[1],m[2],m[3],m[4],m[5]);
				} else {
					snprintf(str, str_size, "??:??:??:??:??:??");
				}
			}
		} else if (a->vformat == ATTR_VFORMAT_IPV4) {
			if (attr->ptr) {
				if (a->byte_size==4) {
					struct in_addr in;
					in.s_addr=*((unsigned int *)attr->ptr);	// in is already in network order
					snprintf(str, str_size, "%s", inet_ntoa(in));
				} else {
					snprintf(str, str_size, "???.???.???.???");
				}	
			}
		} else if (a->vformat == ATTR_VFORMAT_IPV6) {
			if (attr->ptr) {
				if (a->byte_size==16) {
					char ipv6_str[50];
					inet_ntop(AF_INET6, attr->ptr, ipv6_str, 16 );
					snprintf(str, str_size, "%s", ipv6_str );
				} else {
					snprintf(str, str_size, "????:????:????:????:????:????:????:????");
				}	
			}
		} else {
			if (attr->ptr) {
				if (a->byte_size < str_size) {
					memcpy(str, (char*)attr->ptr, a->byte_size);
					str[a->byte_size]=0;
				} else {
					memcpy(str, (char*)attr->ptr, str_size);
					str[str_size-1]=0;
				}
			}
		}
		break;
	case ATTR_FORMAT_BIT_FIELD:
		{
			struct attrinfo_bitfield_t *b = a->attrinfo_bitfield_ptr;
			if (attr->ptr) {
				meinfo_conv_memfields2strlist(attr->ptr, a->byte_size, b->field, b->field_total, str, str_size);
			}
		}
		break;
	case ATTR_FORMAT_TABLE:
		{
			struct attrinfo_table_t *t = a->attrinfo_table_ptr;
			if (attr->ptr) {
				meinfo_conv_memfields2strlist(attr->ptr, t->entry_byte_size, t->field, t->field_total, str, str_size);
			}
		}
	}	

	return strlen(str);
}

int
meinfo_conv_strlist2memfields(char *mem, int mem_size,
			struct attrinfo_field_def_t *field_def, int field_total, 
			char *str)
{
	char buff[1024];
	char *f[128];
	int start_bitnum = 0, i, n;

	strncpy(buff, str, 1024); buff[1023] = 0;
	n = conv_str2array(buff, ",", f, 128);
	if (n != field_total) {
		dbprintf(LOG_ERR, "%d fields required but data=[%s]\n", field_total, str);
		return MEINFO_ERROR_ATTR_FORMAT_CONVERT;
#if 0		
		dbprintf(LOG_WARNING, "%d fields required but data=[%s]\n", field_total, str);
		if (n > field_total)
			n = field_total;
#endif			
	}

	for (i = 0; i < n; i++) {
		if (	field_def[i].vformat==ATTR_VFORMAT_STRING ||
			field_def[i].vformat==ATTR_VFORMAT_MAC ||
			field_def[i].vformat==ATTR_VFORMAT_IPV4 ||
			field_def[i].vformat==ATTR_VFORMAT_IPV6) {
			if (start_bitnum % 8 != 0) {
				dbprintf(LOG_ERR, "str field isn't at byte boundary\n");
				return MEINFO_ERROR_ATTR_FORMAT_CONVERT;
			}
		}
		switch (field_def[i].vformat) {
		case ATTR_VFORMAT_POINTER:
		case ATTR_VFORMAT_UNSIGNED_INT:
		case ATTR_VFORMAT_ENUMERATION:
		case ATTR_VFORMAT_SIGNED_INT:
			// the util_bitmap_set function will convert value from host order to network order
			util_bitmap_set_value((unsigned char *)mem,
					      mem_size * 8,
					      start_bitnum,
					      field_def[i].bit_width, util_atoll(f[i]));
		break;
		case ATTR_VFORMAT_STRING:
			if (strcmp(f[i], "NULLSTR")==0) {
				memset(mem + start_bitnum / 8, 0, field_def[i].bit_width / 8);
			} else if (strcmp(f[i], "SPACESTR")==0) {
				memset(mem + start_bitnum / 8, ' ', field_def[i].bit_width / 8);
			} else {
				strncpy(mem + start_bitnum / 8, f[i], field_def[i].bit_width / 8);
			}		
		break;
		case ATTR_VFORMAT_MAC:
			conv_str_mac_to_mem(mem + start_bitnum / 8, f[i]);
		break;
		case ATTR_VFORMAT_IPV4:
			conv_str_ipv4_to_mem(mem + start_bitnum / 8, f[i]);
		break;
		case ATTR_VFORMAT_IPV6:
			inet_pton(AF_INET6, f[i], mem + start_bitnum / 8);
		break;
		}
		start_bitnum += field_def[i].bit_width;
	}

	return n;
}

// this routine will alloc mem for attr->ptr if attr->ptr is null
// so caller may need to free memory attr->ptr if it is not required
int 
meinfo_conv_string_to_attr(unsigned short classid, unsigned short attr_order, struct attr_value_t *attr, char *str)
{
	struct meinfo_t *miptr;
	struct attrinfo_t *aiptr;
	struct attrinfo_table_t *table_ptr;
	struct attrinfo_bitfield_t *b;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;
	}
	aiptr = &miptr->attrinfo[attr_order];

	switch (aiptr->format) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		if (aiptr->vformat==ATTR_VFORMAT_IPV4) {
			int ipaddr;
			conv_str_ipv4_to_mem((char*)&ipaddr, str);	// ipaddr is network order
			attr->data=ntohl(ipaddr);			// convert ipaddr to host order for attr->data
		} else {
			attr->data = util_atoll(str);
			switch (aiptr->byte_size) {
				case 1:	if ((attr->data&0xff)!=attr->data) {
						dbprintf(LOG_WARNING, "classid=%u, attr_order=%u, value 0x%x(%u) out of %d byte range\n", 
							classid, attr_order, attr->data, aiptr->byte_size);
						attr->data&=0xff; 
					}
					break;
				case 2:	if ((attr->data&0xffff)!=attr->data) {
						dbprintf(LOG_WARNING, "classid=%u, attr_order=%u, value 0x%x(%u) out of %d byte range\n", 
							classid, attr_order, attr->data, attr->data, aiptr->byte_size);
						attr->data&=0xffff; 
					}
					break;
				case 4:	if ((attr->data&0xffffffff)!=attr->data) {
						dbprintf(LOG_WARNING, "classid=%u, attr_order=%u, value 0x%x(%u) out of %d byte range\n", 
							classid, attr_order, attr->data, aiptr->byte_size);
						attr->data&=0xffffffff; 
					}
					break;
			}
		}
		return 0;
		
	case ATTR_FORMAT_STRING:
		if (attr->ptr == NULL)
			attr->ptr = malloc_safe(aiptr->byte_size);
		attr->data=0;
		if (aiptr->vformat==ATTR_VFORMAT_MAC) {
			conv_str_mac_to_mem(attr->ptr, str);
		} else if (aiptr->vformat==ATTR_VFORMAT_IPV4) {
			conv_str_ipv4_to_mem(attr->ptr, str);	// in network order
		} else if (aiptr->vformat==ATTR_VFORMAT_IPV6) {
			inet_pton(AF_INET6, str, attr->ptr);
		} else {
			if (strcmp(str, "NULLSTR")==0) {
				memset(attr->ptr, 0, aiptr->byte_size);
			} else if (strcmp(str, "SPACESTR")==0) {
				memset(attr->ptr, ' ', aiptr->byte_size);
			} else {
				strncpy(attr->ptr, str, aiptr->byte_size);	// str may be not null terminated if its size == aiptr->byte_size
			}
		}
		return 0;
		
	case ATTR_FORMAT_BIT_FIELD:
		b = aiptr->attrinfo_bitfield_ptr;
		if (b == NULL) {
			dbprintf(LOG_ERR, "classid=%u, attr_order=%u, null attrinfo_bitfield ptr\n", classid, attr_order);
			return MEINFO_ERROR_ATTR_PTR_NULL;
		}
		if (attr->ptr == NULL)
			attr->ptr = malloc_safe(aiptr->byte_size);
		if (meinfo_conv_strlist2memfields(attr->ptr, aiptr->byte_size, b->field, b->field_total, str)<0) {
			free_safe(attr->ptr);
			attr->ptr=NULL;
			dbprintf(LOG_ERR, "classid=%u, attr_order=%u, bitfield format err\n", classid, attr_order);
			return MEINFO_ERROR_ATTR_FORMAT_CONVERT;
		}
		return 0;
		
	case ATTR_FORMAT_TABLE:
		table_ptr = aiptr->attrinfo_table_ptr;
		if (table_ptr == NULL) {
			dbprintf(LOG_ERR, "classid=%u, attr_order=%u, null attrinfo_table_ptr\n", classid, attr_order);
			return MEINFO_ERROR_ATTR_PTR_NULL;
		}
		if (attr->ptr == NULL)
			attr->ptr = malloc_safe(table_ptr->entry_byte_size);
		if (meinfo_conv_strlist2memfields(attr->ptr, table_ptr->entry_byte_size, table_ptr->field, table_ptr->field_total, str)<0) {
			free_safe(attr->ptr);
			attr->ptr=NULL;
			dbprintf(LOG_ERR, "classid=%u, attr_order=%u, table format err\n", classid, attr_order);
			return MEINFO_ERROR_ATTR_FORMAT_CONVERT;
		}
		return 0;
	}

	dbprintf(LOG_ERR, "classid=%u, attr_order=%u, unknown format err %d\n", classid, attr_order, aiptr->format);
	return MEINFO_ERROR_ATTR_FORMAT_UNKNOWN;	// unknown format
}

// below is for me_find in cli
int
meinfo_conv_memfields_find(unsigned char *mem, int mem_size,
			struct attrinfo_field_def_t *field_def, int field_total, 
			char *keyword)
{
	int i;
	int start_bit=0;
	int is_number=util_str_is_number(keyword);

	for (i=0; i<field_total; i++) {
		struct attrinfo_field_def_t *f=&field_def[i];
		unsigned long long udata;
		char buff[64];

		switch (f->vformat) {
		case ATTR_VFORMAT_POINTER:
		case ATTR_VFORMAT_UNSIGNED_INT:
		case ATTR_VFORMAT_ENUMERATION:
			if (is_number) {
				// the util_bitmap_get function will convert value from network order to host order
				udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);
				if (util_atoll(keyword)==udata)
					return 1;
			}
			break;
		case ATTR_VFORMAT_SIGNED_INT:
			if (is_number) {
				// the util_bitmap_get function will convert value from network order to host order
				udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);

				if (f->bit_width<=8) {
					if (util_atoi(keyword)==(char)udata)
						return 1;
				} else if (f->bit_width<=16) {
					if (util_atoi(keyword)==(short)udata)
						return 1;
				} else if (f->bit_width<=32) {
					if (util_atoi(keyword)==(int)udata)
						return 1;
				} else {
					if (util_atoll(keyword)==(long long int)udata)
						return 1;
				}
			}
			break;
		case ATTR_VFORMAT_STRING:
			if (f->bit_width/8<64) {
				memcpy(buff, mem+start_bit/8, f->bit_width/8);
				buff[f->bit_width/8]=0;
			} else {
				memcpy(buff, mem+start_bit/8, 63);
				buff[63]=0;
			}
			if (util_strcasestr(buff, keyword))
				return 1;
			break;
		case ATTR_VFORMAT_MAC:
			if (f->bit_width==48) {
				unsigned char m[6];			
				memcpy(m, mem+start_bit/8, f->bit_width/8);				
				sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", 
					m[0],m[1],m[2],m[3],m[4],m[5]);
				if (util_strcasestr(buff, keyword))
					return 1;
			}	
			break;
		case ATTR_VFORMAT_IPV4:
			// the util_bitmap_get function will convert value from network order to host order
			udata = util_bitmap_get_value(mem, mem_size * 8, start_bit, f->bit_width);
			if (f->bit_width==32) {
				struct in_addr in;
				in.s_addr=htonl(udata);	// conv udata to network order for inet_ntoa
				sprintf(buff, "%s", inet_ntoa(in));
				if (util_strcasestr(buff, keyword))
					return 1;
			}
			break;
		case ATTR_VFORMAT_IPV6:
			if (f->bit_width==128) {
				char ipv6_str[50];
				inet_ntop(AF_INET6, mem, ipv6_str, 16) ;
				sprintf(buff, "%s", ipv6_str);
				if (util_strcasestr(buff, keyword))
					return 1;
			}
			break;
		default:	// table/bitfield in table/bitfield?
			break;
		}

		start_bit+=f->bit_width;
	}

	return 0;
}

int
meinfo_conv_attr_find(unsigned short classid, unsigned short attr_order, 
			struct attr_value_t *attr, char *keyword)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct attrinfo_t *a;
	long long int kval;
	int is_number=util_str_is_number(keyword);

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return 0;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, attr_order);
		return 0;
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), classid, attr_order);
		return 0;
	}

	a = &miptr->attrinfo[attr_order];
	switch(a->format) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
		if (is_number) {
			kval=util_atoll(keyword);
			if (kval==attr->data)
				return 1;
		}
		break;
	case ATTR_FORMAT_SIGNED_INT:
		if (is_number) {
			kval=util_atoll(keyword);
			if (a->byte_size==1) {
				if (kval==(char)attr->data)
					return 1;
			} else if (a->byte_size==2) {
				if (kval==(short)attr->data)
					return 1;
			} else if (a->byte_size<=4) {
				if (kval==(int)attr->data)
					return 1;
			} else {
				if (kval==attr->data)
					return 1;
			}
		}
		break;
	case ATTR_FORMAT_STRING:
		if (attr->ptr) {
			if (util_strcasestr((char*)attr->ptr, keyword))
				return 1;
		}
		break;
	case ATTR_FORMAT_BIT_FIELD:
		{
			struct attrinfo_bitfield_t *b = a->attrinfo_bitfield_ptr;
			if (attr->ptr) {
				return meinfo_conv_memfields_find(attr->ptr, a->byte_size, b->field, b->field_total, keyword);
			}
		}
		break;
	case ATTR_FORMAT_TABLE:
		{
			struct attrinfo_table_t *t = a->attrinfo_table_ptr;
			if (attr->ptr) {
				return meinfo_conv_memfields_find(attr->ptr, t->entry_byte_size, t->field, t->field_total, keyword);
			}
		}
	}	

	return 0;
}
