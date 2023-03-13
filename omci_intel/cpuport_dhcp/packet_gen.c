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
 * Module  : cpuport_dhcp
 * File    : parse_format.c
 *
 ******************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "list.h"
#include "util.h"
#include "packet_gen.h"

#define	ASCII		0xF0
#define	SYMBOL		0xF1
#define	SEPARATE	0xF2


struct list_head format_list_head;
struct lineidformat_t *lineidformat;
struct lineidparam_t *lineidparam;

static int is_separate(char ch) {
	if(ch == '(' || 
		ch == ')' || 
		ch == '.' || 
		ch == '/' || 
		ch == ':' || 
		ch == ';' || 
		ch == '<' || 
		ch == '>' || 
		ch == '[' || 
		ch == ']' || 
		ch == '{' || 
		ch == '}')
		return 1;
	else
		return 0;
}

/* symbol flage+ normal data; separate_char*/
//0 0/0/0:%s.%c %a/%r/%f/%S/0/%p/%m %y
//0 0/0/0:%s.%c %B/%a/%r/%f/%S/%L/0/%p %m/%n %y
//%s.%c%B/%a/%r/%f/%S/%L/%p/%m/%n/fiberhome


int packet_gen_parse_format(char *input, int len)
{
	int i;
	struct format_list_node_t *format_list_node;
	for(i=0; i < len; i++) {
		if(is_separate(input[i])) {
			//separate
			format_list_node=malloc_safe(sizeof(struct format_list_node_t));

			format_list_node->symbol.type=SEPARATE;
			format_list_node->symbol.value=input[i];
			list_add_tail(&format_list_node->format_list, &format_list_head);
		} else if( input[i] == '%') {
			format_list_node=malloc_safe(sizeof(struct format_list_node_t));
			format_list_node->symbol.type=SYMBOL;
			format_list_node->symbol.value=input[++i];
			list_add_tail(&format_list_node->format_list, &format_list_head);
		} else {

			format_list_node=malloc_safe(sizeof(struct format_list_node_t));
			format_list_node->symbol.type=ASCII;
			format_list_node->symbol.value=input[i];
			list_add_tail(&format_list_node->format_list, &format_list_head);
		}
	}
	return 0;
}

void packet_gen_release_format_list()
{
	struct format_list_node_t *pos, *temp;
	list_for_each_entry_safe(pos, temp, &format_list_head, format_list) {
		list_del(&pos->format_list);
                free_safe(pos);
        }

}

void packet_gen_dump_format_list()
{
	struct format_list_node_t *format_list_node;
	int format_len=0;

	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "Dump format list:\n");
	list_for_each_entry(format_list_node, &format_list_head, format_list) {
		if( format_list_node->symbol.type == SEPARATE) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%c", format_list_node->symbol.value);
			format_len++;
		} else if (format_list_node->symbol.type == ASCII ) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%c", format_list_node->symbol.value);
			format_len++;
		} else if (format_list_node->symbol.type == SYMBOL) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%%%c", format_list_node->symbol.value);
			format_len+=2;
		} else {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "Error\n");
		}
	}
	util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "\nformat_len=%d\n", format_len);
}

//onu and sip is the same mac addr
static int get_onu_mac_addr(unsigned char *mac, char *devname)
{
	int fd;
	struct ifreq itf;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	strncpy(itf.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFHWADDR, &itf) < 0) {
		close(fd);
		return -1;
	} else
		memcpy(mac, itf.ifr_hwaddr.sa_data, IFHWADDRLEN);
	close(fd);
	return 0;
}

static int get_onu_ip_addr(const char *ifname, unsigned char ipaddr[])
{
        int sock, ifc_num;
        int i, j, found = 0;
        int ret;
        struct ifreq interface_list[32];
        struct ifconf if_list;

        if ((sock=socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
                dbprintf(LOG_ERR, "create socket error(%d)\n", sock);
                return -1;
        }

        if_list.ifc_len = (32 * sizeof(struct ifreq));
        if_list.ifc_req = interface_list;

        /* Get the all interfaces information */
        if ((ret = ioctl(sock, SIOCGIFCONF, &if_list)) < 0) {
                dbprintf(LOG_ERR, "get if configure error(%d)\n", ret);
                close(sock);
                return -1;
        }

        ifc_num = if_list.ifc_len / sizeof(struct ifreq);
        //get ip
        for (i = 0; i < ifc_num; i++) {
                if (!strcmp(if_list.ifc_req[i].ifr_name, ifname)) {
                        for (j = 0; j < 4; j++)
                                ipaddr[j] = if_list.ifc_req[i].ifr_addr.sa_data[2 + j]; // from 2 to 5
                        found = 1;
                        break;
                }
        }

        close(sock);

        if (!found) {
                dbprintf(LOG_WARNING, "get %s ip error, fill hex 00 00 00 00\n", ifname);
                return -1;
        }
	return 0;
}

static void hex_to_lower_case(unsigned char *tmpmac, unsigned char *mac)
{
	sprintf((char*)mac, "%02x%02x%02x%02x%02x%02x", *tmpmac,*(tmpmac+1),*(tmpmac+2), *(tmpmac+3),*(tmpmac+4),*(tmpmac+5));
}

int packet_gen_generate_param(unsigned char **optstr, int *len, unsigned short cvlan)
{
	struct format_list_node_t *format_list_node;
	int param_len=0,maxlen=375;	//large string 15*25;
	unsigned char *tmpptr, tmpmac[6];

	*optstr=malloc_safe(maxlen);
	tmpptr=*optstr;

	list_for_each_entry(format_list_node, &format_list_head, format_list) {
		if( format_list_node->symbol.type == SEPARATE) {
			memcpy(tmpptr,&format_list_node->symbol.value,1);
			tmpptr++;
			param_len++;
			//dbprintf(LOG_ERR, "%c", format_list_node->symbol.value);
		} else if (format_list_node->symbol.type == ASCII ) {
			memcpy(tmpptr,&format_list_node->symbol.value,1);
			tmpptr++;
			param_len++;
			//dbprintf(LOG_ERR, "%c", format_list_node->symbol.value);
		} else if (format_list_node->symbol.type == SYMBOL) {
			//dbprintf(LOG_ERR, "%%%c", format_list_node->symbol.value);

			switch (format_list_node->symbol.value) {
			case 'A'://IAD MAC
			{
				unsigned char mac[12], mac_default[12]={'8','8','6','2','2','7','8','8','8','1','1','8'};
				if( get_onu_mac_addr(mac, "pon0") == 0 ) {
					hex_to_lower_case(tmpmac, mac);
        				//dbprintf(LOG_ERR, "JKU:%s\n", mac);
					memcpy(tmpptr, &mac, 12);
				} else {
					memcpy(tmpptr, &mac_default, 12);	//maybe never happen
				}
				tmpptr+=12;
				param_len+=12;
			}
			break;
			case 'a'://50 byte
			{
			#if 1
				char acc_point_id[50];
				int len;
				sprintf(acc_point_id,"%s",lineidparam->access_point_id_a);
				len=strlen(acc_point_id);
				memcpy(tmpptr, acc_point_id, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				memcpy(tmpptr, lineidparam->access_point_id_a, 50);
				tmpptr+=50;
				param_len+=50;
			#endif
			}
			break;
			case 'B'://lan
				memcpy(tmpptr, "lan", 3);
				tmpptr+=3;
				param_len+=3;
			break;
			case 'c'://cvlan(string), "4096"
			{
				char vlan[5];
				int len;		
				sprintf(vlan,"%d",cvlan);
				len=strlen(vlan);
				memcpy(tmpptr, vlan, len);
				tmpptr+=len;
				param_len+=len;
			}
			break;
			case 'f'://frame_id	
			{
			#if 1
				char frame_id[6];	//2byte=>65535
				int len;
				sprintf(frame_id,"%d",ntohs(lineidparam->frame_id_f));
				len=strlen(frame_id);
				memcpy(tmpptr, frame_id, len);
				tmpptr+=len;
				param_len+=len;
			#else	
        			//dbprintf(LOG_ERR, "%02X\n", ntohs(lineidparam->frame_id_f));
				memcpy(tmpptr, &lineidparam->frame_id_f, 2);
				tmpptr+=2;
				param_len+=2;
			#endif
			}
			break;
			case 'I'://IAD IP
			{
				unsigned char ipaddr[4]={0};
				get_onu_ip_addr("pon0", ipaddr);
				memcpy(tmpptr, &ipaddr, 4);
				tmpptr+=4;
				param_len+=4;
			}
			break;
			case 'L'://16byte
			{
			#if 1
				char traffic_board_type[17];
				int len;
				sprintf(traffic_board_type,"%s",lineidparam->traffic_board_type_L);
				len=strlen(traffic_board_type);
				memcpy(tmpptr, traffic_board_type, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				memcpy(tmpptr, &lineidparam->traffic_board_type_L, 16);
				tmpptr+=16;
				param_len+=16;
			#endif
			}
			break;
			case 'M'://sfu: 0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 'm'://ONU MAC
			{
				unsigned char mac[12], mac_default[12]={'8','8','6','2','2','7','8','8','8','1','1','8'};
				if( get_onu_mac_addr(tmpmac, "pon0") == 0 ) {
					hex_to_lower_case(tmpmac, mac);
        				//dbprintf(LOG_ERR, "JKU:%s\n", mac);
					memcpy(tmpptr, &mac, 12);
				} else {
					memcpy(tmpptr, &mac_default, 12);
				}
				tmpptr+=12;
				param_len+=12;
			}
			break;
			case 'n'://ONU type(AN5506-04-B4)
			{
				char onu_type[]="AN5506-04-B4";
				int len;

				len=strlen(onu_type);
				memcpy(tmpptr, onu_type, len);
				tmpptr+=len;
				param_len+=len;
			}
			break;
			case 'O': //OLT manager VLAN IP
        			//dbprintf(LOG_ERR, "%02X\n", ntohl(lineidparam->olt_manage_vlanip_O));
				memcpy(tmpptr, &lineidparam->olt_manage_vlanip_O, 4);
				tmpptr+=4;
				param_len+=4;
			break;
			case 'o': //byte
			{
			#if 1
				char onu_authid[6];	//2byte=>65535
				int len;
				sprintf(onu_authid,"%d",ntohs(lineidparam->onu_authid_o));
				len=strlen(onu_authid);
				memcpy(tmpptr, onu_authid, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				//dbprintf(LOG_ERR, "%02X\n", ntohs(lineidparam->onu_authid_o));
				memcpy(tmpptr, &lineidparam->onu_authid_o, 2);
				tmpptr+=2;
				param_len+=2;
			#endif
			}
			break;
			case 'P'://sfu:0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 'p'://PON portid
			{
			#if 1
				char ponid[6];	//2byte=>65535
				int len;
				sprintf(ponid,"%d",ntohs(lineidparam->ponid_p));
				len=strlen(ponid);
				memcpy(tmpptr, ponid, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				//dbprintf(LOG_ERR, "%02X\n", ntohs(lineidparam->ponid_p));
				memcpy(tmpptr, &lineidparam->ponid_p, 2);
				tmpptr+=2;
				param_len+=2;
			#endif
			}
			break;
			case 'r'://shelf_id
			{
			#if 1
				char shelf_id[6];	//2byte=>65535
				int len;
				sprintf(shelf_id,"%d",ntohs(lineidparam->shelf_id_r));
				len=strlen(shelf_id);
				memcpy(tmpptr, shelf_id, len);
				tmpptr+=len;
				param_len+=len;
			#else	
        			//dbprintf(LOG_ERR, "%02X\n", ntohs(lineidparam->shelf_id_r));
				memcpy(tmpptr, &lineidparam->shelf_id_r, 2);
				tmpptr+=2;
				param_len+=2;
			#endif
			}
			break;
			case 'S'://slot id
			{
			#if 1
				char slotid[4];	//1byte=>256
				int len;
				sprintf(slotid,"%d",lineidparam->slotid_S);
				len=strlen(slotid);
				memcpy(tmpptr, slotid, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				memcpy(tmpptr, &lineidparam->slotid_S, 1);
				tmpptr++;
				param_len++;
			#endif
			}
			break;
			case 's'://onu SVLAN(string)
				memcpy(tmpptr, "0", 1);	//default none tag
				tmpptr+=1;
				param_len+=1;
			break;
			case 'T'://sfu:0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 't'://sfu:0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 'u': //32byte
			{
			#if 1
				char uplink_port_type[33];
				int len;
				sprintf(uplink_port_type,"%s",lineidparam->uplink_port_type_u);
				len=strlen(uplink_port_type);
				memcpy(tmpptr, uplink_port_type, len);
				tmpptr+=len;
				param_len+=len;
			#else	
				memcpy(tmpptr, lineidparam->uplink_port_type_u, 32);
				tmpptr+=32;
				param_len+=32;
			#endif
			}
			break;
			case 'X'://VPI/SVLAN(string), current filled 0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 'x'://VCI/CVLAN(string), current filled 0
				memcpy(tmpptr, "0", 1);
				tmpptr+=1;
				param_len+=1;
			break;
			case 'y': //GP
				memcpy(tmpptr, "GP", 2);
				tmpptr+=2;
				param_len+=2;
			break;

			default:
				dbprintf_cpuport(LOG_ERR, "format %%%c not exist", format_list_node->symbol.value);
				return -1;
			}

		} else {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "Error token\n");
			return -1;
		}
	}
	*len=param_len;
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "param_len=%d\n", param_len);
	return 0;
}

#if 0
int main_test()
{
	char input_data[]="0 0/0/0:%s.%c %a/%r/%f/%S/0/%p/%m %y";
	/* init list */
	INIT_LIST_HEAD(&format_list_head);
	parse_format(input_data);
	dump_format();
	return 0;
}
#endif 
