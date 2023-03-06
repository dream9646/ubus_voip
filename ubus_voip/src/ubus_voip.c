#include <rpcd/exec.h>
#include <rpcd/session.h>
#include <rpcd/uci.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <syslog.h>
#include <libubox/ulog.h>
#include <uci.h>
//#include "util.h"

struct blob_buf b;
int rpc_exec_timeout = RPC_EXEC_DEFAULT_TIMEOUT;
#define BUF_SIZE_1 1024
#define BUF_SIZE_2 2048
#define BUF_SIZE_3 3072
#define BUF_SIZE_4 4096
static struct ubus_context *ctx;
struct ubus_voip_cmd_list{
    struct list_head list;
	char key[BUF_SIZE_1];
	char action[BUF_SIZE_1];
};
struct ubus_voip_changes_list{
    struct list_head list;
	char key[BUF_SIZE_1];
	char value[BUF_SIZE_1];
	char ret[BUF_SIZE_1];
};
struct ubus_voip_cmd_list ubus_voip_cmd_list(struct ubus_voip_cmd_list ubus_voip_cmdlist);
struct ubus_voip_cmd_list ubus_voip_cmd_list(struct ubus_voip_cmd_list ubus_voip_cmdlist){
    FILE *fp = fopen("/etc/voip/cmd_list", "r");
    if(fp == NULL){
        printf("Error opening file");
        return ubus_voip_cmdlist;
    }
    char line[BUF_SIZE_1];
    char key[BUF_SIZE_1];
    char action[BUF_SIZE_1];
    while (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "%s %s",action,key);
        struct ubus_voip_cmd_list* tmp = (struct ubus_voip_cmd_list *)malloc(sizeof(struct ubus_voip_cmd_list));
        strcpy(tmp->action, action);
        strcpy(tmp->key, key);
        list_add(&(tmp->list), &(ubus_voip_cmdlist.list));
    }
    fclose(fp);
    return ubus_voip_cmdlist;
}
int ubus_voip_set_fv_voip(const char *key, const char *value) ;
int ubus_voip_set_fv_voip(const char *key, const char *value) {
	char input_section[BUF_SIZE_1];
	char input_key[BUF_SIZE_1];
	char input_value[BUF_SIZE_1];
	strcpy(input_value,value);
	char new_line[BUF_SIZE_4];
	char new_line_add_list[BUF_SIZE_3];
	char tmp_section[BUF_SIZE_1];
    sscanf(key, "fvt_evoip.%[^.].%s", input_section, input_key);
	snprintf(tmp_section, sizeof(tmp_section)+3, "[%s]", input_section);
	snprintf(input_section, sizeof(input_section), "%s", tmp_section);
	if(strstr(input_key,"-") != NULL||strstr(input_key,"+") != NULL){
		//list of uci
		sprintf(new_line, "%s",input_value);
	}
    FILE *fp = fopen("/etc/voip/fv_voip_uci.config", "rb+");
    if (fp == NULL) {
        return -1;
    }

    char lines[BUF_SIZE_2][BUF_SIZE_2];
    int i = 0;
    while (fgets(lines[i], sizeof(lines[i]), fp) > 0) {
        i++;
    }

    //move the file pointer to the beginning of the file
    rewind(fp);
	char fv_section[BUF_SIZE_1];
	char fv_key[BUF_SIZE_1];
    for (int j = 0; j < i; j++) {
        char line_key[BUF_SIZE_1];
        char line_value[BUF_SIZE_1];
        sscanf(lines[j], "%s = %s", line_key, line_value);
		if (line_key[0] == '['&&line_key[strlen(line_key)-1] == ']'&&strlen(line_key)>3) {
			sprintf(fv_section, "%s", line_key);
		}
		if(strstr(lines[j], "=")){
					sprintf(fv_key, "%s", line_key);
		}
        if (strncmp(line_key, input_key, strlen(input_key)) == 0&&strcmp(fv_section, input_section) == 0) {
            sprintf(new_line, "%s = %s\r\n", input_key, input_value);
            fputs(new_line, fp);
        } else if(strstr(input_key, "+")&&strncmp(line_key, input_key, strlen(line_key)) == 0&&strcmp(fv_section, input_section) == 0){
			 bool is_add = false;
			 if(strstr(lines[j],"{")){
				 is_add = true;
			 }
			 while (is_add)
			 {
				if(strstr(lines[j],"}")&&strstr(input_key,fv_key)){
					is_add = false;
					snprintf(new_line_add_list,sizeof(new_line), "%s", new_line);
					snprintf(new_line,sizeof(new_line), "        %s\r\n", new_line_add_list);
					// Make sure the new line is added to the end of the list
					fputs(new_line, fp);
					fputs(lines[j], fp);
				}else{
					fputs(lines[j], fp);
					j++;
				}
			 }
		}else if(strstr(input_key, "-")&&strncmp(line_key, input_key, strlen(line_key)) == 0&&strcmp(fv_section, input_section) == 0){
			bool is_del = false;
			if(strstr(lines[j],"{")){
				is_del = true;
			}
			while (is_del)
			{
					sscanf(lines[j], "%s = %s", line_key, line_value);
					if(strstr(lines[j], "=")){
						sprintf(fv_key, "%s", line_key);
					}
				if(strstr(lines[j],new_line)&&strstr(input_key,fv_key)){
					//Match, do not write
					is_del = false;
				}else{
					fputs(lines[j], fp);
					j++;
				}
				if( j>=i){
					//File end
					fputs(lines[j], fp);
					is_del = false;
				}
			}
		}else {
            // Does not match, write the original line
            fputs(lines[j], fp);
        }
    }
    // Sync changes
    fflush(fp);
    fclose(fp);
    return 0;
}
int ubus_voip_delete_sub_str(const char *str, const char *sub_str, char *result_str);
int ubus_voip_delete_sub_str(const char *str, const char *sub_str, char *result_str)  
{  
    int count = 0;  
    int str_len = strlen(str);  
    int sub_str_len = strlen(sub_str);  
  
    if (str == NULL)  
    {  
        result_str = NULL;  
        return 0;  
    }  
  
    if (str_len < sub_str_len || sub_str == NULL)  
    {  
        while (*str != '\0')  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
        }  
  
        return 0;  
    }  
  
    while (*str != '\0')  
    {  
        while (*str != *sub_str && *str != '\0')  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
        }  
  
        if (strncmp(str, sub_str, sub_str_len) != 0)  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
            continue;  
        }  
        else  
        {  
            count++;  
            str += sub_str_len;  
        }  
    }  
  
    *result_str = '\0';  
  
    return count;  
} 
char* ubus_voip_fv_voip_type(char *str);
char* ubus_voip_fv_voip_type(char *str) {
	char res_list_val[BUF_SIZE_2]={0} ; 
	char *p;
	char a[BUF_SIZE_1] = {0};
	char b[BUF_SIZE_1] = {0};
	char c[BUF_SIZE_1] = {0};
	char space[1]=" ";
	int newline_count = 0;
	int count=0;

	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') {
			newline_count++;
		}
		if(str[i] == ' '){
			count++;
		}
	}
	if(newline_count<3){
		p = strtok(str, "={");
		ubus_voip_delete_sub_str(p, "}", p);
		ubus_voip_delete_sub_str(p, "{", p);
		strcat(res_list_val,p);
		res_list_val[strlen(res_list_val)-1] = '\0';
		return strdup(res_list_val);
	}
	if(newline_count==3){
		p = strtok(str, "={");
		p = strtok(NULL, "\n");
		p = strtok(NULL, "\n");
		while (p != NULL) {
			//Deal one line have two value count5=3+2
			if(strstr(p, " ") != NULL&&count==5) {
				sscanf(p, " %s %s", a, b);
				if(a[0]=='['&&a[strlen(a)-1]==']'){
					ubus_voip_delete_sub_str(a, "[", a);
					ubus_voip_delete_sub_str(a, "]", a);
				}
				snprintf(p, BUF_SIZE_3, "%s.%s ", a,b);
				strcat(res_list_val,p);
				res_list_val[strlen(res_list_val)-1] = '\0';
				strcat(res_list_val,space);
			}else if(strstr(p, " ") != NULL&&count==6){
				//Deal one line have three value count6=3+3
				sscanf(p, " %s %s %s", a, b,c);
				if(a[0]=='['&&a[strlen(a)-1]==']'){
					ubus_voip_delete_sub_str(a, "[", a);
					ubus_voip_delete_sub_str(a, "]", a);
				}
				snprintf(p, BUF_SIZE_4, "%s.%s.%s ", a,b,c);
			
				strcat(res_list_val,p);
				res_list_val[strlen(res_list_val)-1] = '\0';
				strcat(res_list_val,space);
			}else{		
				ubus_voip_delete_sub_str(p, "}", p);
				strcat(res_list_val,p);
				res_list_val[strlen(res_list_val)-1] = '\0';
			}
			p = strtok(NULL, "\n");
		}
		return strdup(res_list_val);
	}
	p = strtok(str, "={");
	p = strtok(NULL, "{");
	p = strtok(NULL, "\n");
	while (p != NULL) {
		//Deal one line have two value count5=3+2
		if(strstr(p, " ") != NULL&&count==5) {
			sscanf(p, "%s %s", a, b);
			if(a[0]=='['&&a[strlen(a)-1]==']'){
				ubus_voip_delete_sub_str(a, "[", a);
				ubus_voip_delete_sub_str(a, "]", a);
			}
			snprintf(p, BUF_SIZE_3, "%s.%s ", a,b);
			strcat(res_list_val,p);
			res_list_val[strlen(res_list_val)] = '\0';
			strcat(res_list_val,space);
		}
		else if(strstr(p, " ") != NULL&&count==6){
			//Deal one line have three value count6=3+3
					sscanf(p, " %s %s %s", a, b,c);
					if(a[0]=='['&&a[strlen(a)-1]==']'){
						ubus_voip_delete_sub_str(a, "[", a);
						ubus_voip_delete_sub_str(a, "]", a);
					}
					snprintf(p, BUF_SIZE_4, "%s.%s.%s ", a,b,c);
					strcat(res_list_val,p);
					res_list_val[strlen(res_list_val)-1] = '\0';
					strcat(res_list_val,space);
				}else{		
					ubus_voip_delete_sub_str(p, "}", p);
					strcat(res_list_val,p);
					res_list_val[strlen(res_list_val)-1] = '\0';
				}
		p = strtok(NULL, "\n");
	}
	return strdup(res_list_val);
}


char* ubus_voip_cmd_list_set(char *section,char *key, char *value);
char* ubus_voip_cmd_list_set(char *section,char *key, char *value){
	char* results = (char*)malloc(BUF_SIZE_4 * sizeof(char));
	memset(results,0,sizeof(char)*BUF_SIZE_4);
	static char rtpportlimits[BUF_SIZE_1];
	static char rtpportlimits_2[BUF_SIZE_1];
	static char sip_proxies[BUF_SIZE_1];
	static char sip_proxies_2[BUF_SIZE_1];
	static char sip_registrars[BUF_SIZE_1];
	static char sip_registrars_2[BUF_SIZE_1];
	char addr[BUF_SIZE_1], port[BUF_SIZE_1];
	struct ubus_voip_cmd_list *tmp_cmd, *tmp_cmd_next;
	struct ubus_voip_cmd_list cmdlist;
    INIT_LIST_HEAD(&cmdlist.list);
	FILE *fp_cmd;
	char cmd[BUF_SIZE_4];
	char* line = (char*)malloc(BUF_SIZE_4 * sizeof(char));
	char ep[BUF_SIZE_1];
	char master_check[BUF_SIZE_1];
	char space[1]=" ";
	//Deal EP0 and EP1
	if(strcmp(section,"EP0")==0){
		strcpy(ep,"0");
	}else if(strcmp(section,"EP1")==0){
		strcpy(ep,"1");
	}
	cmdlist=ubus_voip_cmd_list(cmdlist);
	list_for_each_entry(tmp_cmd, &cmdlist.list, list) {
	//cmdlist echo start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "echo")==0) {
		if(strstr(tmp_cmd->key,"_") != NULL){
			ubus_voip_delete_sub_str(tmp_cmd->key, "_",tmp_cmd->key);
		}
		sprintf(cmd, "%s %s %s\n",tmp_cmd->action, key, value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
	}
	//cmdlist echo end
	//cmdlist fvcli_set start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "fvcli_set")==0) {
		if(strcmp(tmp_cmd->key,"log_filter_condition")==0){
			strcpy(tmp_cmd->key, "logfilter");
		}else if(strcmp(tmp_cmd->key,"log_debug_level")==0){
			strcpy(tmp_cmd->key, "logtaglevel");
		}
		if(strstr(tmp_cmd->key,"_") != NULL){
			ubus_voip_delete_sub_str(tmp_cmd->key, "_",tmp_cmd->key);
		}
		sprintf(cmd, "fvcli set %s %s\n", tmp_cmd->key, value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
	}
	//cmdlist fvcli_set end
	//cmdlist fvcli_set_ep start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "fvcli_set_ep")==0) {
		if(strstr(tmp_cmd->key,"dp_params_") != NULL){
			ubus_voip_delete_sub_str(tmp_cmd->key, "dp_params_",tmp_cmd->key);
		}else if(strstr(tmp_cmd->key,"fax_params_") != NULL){
			ubus_voip_delete_sub_str(tmp_cmd->key, "fax_params_",tmp_cmd->key);
		}
		//forward start
		if(strcmp(tmp_cmd->key,"forward_always")==0){
			strcpy(tmp_cmd->key, "callforwardalways");
			if(key ==value){
				strcpy(tmp_cmd->key, "disablecallforwardalways");
				strcpy(value, space);
			}
		}else if(strcmp(tmp_cmd->key,"forward_onbusy")==0){
			strcpy(tmp_cmd->key, "callforwardonbusy");
			if(key ==value){
				strcpy(tmp_cmd->key, "disablecallforwardonbusy");
				strcpy(value, space);
			}
		}else if(strcmp(tmp_cmd->key,"forward_onnoanswer")==0){
			strcpy(tmp_cmd->key, "callforwardonnoanswer");
			if(key ==value){
				strcpy(tmp_cmd->key, "disablecallforwardonnoanswer");
				strcpy(value, space);
			}
		}else if(strcmp(tmp_cmd->key,"forward_always-")==0){
			strcpy(tmp_cmd->key, "disablecallforwardalways");

		}else if(strcmp(tmp_cmd->key,"forward_onbusy-")==0){
			strcpy(tmp_cmd->key, "disablecallforwardonbusy");

		}else if(strcmp(tmp_cmd->key,"forward_onnoanswer-")==0){
			strcpy(tmp_cmd->key, "disablecallforwardonnoanswer");

		}
		//forward end
		//sip start
		if(strcmp(tmp_cmd->key,"sip_ip_addr_ip_addr")==0){
			strcpy(tmp_cmd->key, "sip_ip_addr_ip");
		}else if(strcmp(tmp_cmd->key,"sip_ip_addr_protocol")==0){
			strcpy(key, "sip_ip_addr_protocol");
		}else if(strcmp(tmp_cmd->key,"sip_ip_addr_gateway_addr")==0){
			strcpy(tmp_cmd->key, "sip_ip_addr_gateway");
		}else if(strcmp(tmp_cmd->key,"sip_ip_addr_subnet_mask")==0){
			strcpy(tmp_cmd->key, "sip_ip_addr_subnet");
		}else if(strcmp(tmp_cmd->key,"sip_ip_addr_domain_name")==0){
			strcpy(tmp_cmd->key, "sip_ip_addr_domainname");
		}else if(strcmp(tmp_cmd->key,"sip_signaling_port")==0){
			strcpy(tmp_cmd->key, "siplocalport");
		}
		//sip end
		//voice gain start
		if(strcmp(tmp_cmd->key,"tx_level")==0){
			strcpy(tmp_cmd->key, "voicetxgainlevel");
		}else if(strcmp(tmp_cmd->key,"tx_level")==0){
			strcpy(key, "voicerxgainlevel");
		}
		//voice gain end
		//domain_name start
		if(strcmp(tmp_cmd->key,"domain_name")==0){
			strcpy(tmp_cmd->key, "sipdomainname");
		}									
		//domain_name end
		//proxy username and password start
		if(strcmp(tmp_cmd->key,"proxy_user_name")==0){
			strcpy(tmp_cmd->key, "username");
		}else if(strcmp(tmp_cmd->key,"proxy_password")==0){
			strcpy(key, "userpassword");
		}else if(strcmp(tmp_cmd->key,"phone_number")==0){
			strcpy(key, "usermap");
		}
		//proxy username and password end
		//proxy_outbound_flag start
		if(strcmp(tmp_cmd->key,"proxy_outbound_flag")==0){
			strcpy(tmp_cmd->key, "outboundproxyflag");
		}else if(strcmp(tmp_cmd->key,"outbound_proxies")==0){
			strcpy(key, "outboundproxy");
		}
		//proxy_outbound_flag end
		//sip_registration start
		if(strcmp(tmp_cmd->key,"sip_registration_timeout")==0){
			strcpy(tmp_cmd->key, "registrationtimeout");
		}else if(strcmp(tmp_cmd->key,"sip_registration_delay_ms")==0){
			strcpy(key, "registrationdelayms");
		}else if(strcmp(tmp_cmd->key,"sip_registration_refresh_type")==0){
			strcpy(key, "registrationrefreshtype");
		}else if(strcmp(tmp_cmd->key,"sip_reregistration_percentage")==0){
			strcpy(key, "reregistrationpercentage");
		}else if(strcmp(tmp_cmd->key,"sip_reregistration_random_base")==0){
			strcpy(key, "reregistrationpercentage");
		}									
		//sip_registration end
		//cfwdnoanswtimeout start
		if(strcmp(tmp_cmd->key,"cfwdnoanswtimeout")==0){
			strcpy(tmp_cmd->key, "callforwardonnoanswertimeout");
		}									
		//cfwdnoanswtimeout end
		//faxparameters start
		if(strcmp(tmp_cmd->key,"fax_params_t38_fax_opt")==0){
			strcpy(tmp_cmd->key, "t38faxopt");
		}else if(strcmp(tmp_cmd->key,"fax_params_vbd_voicecoder")==0){
			strcpy(key, "vbdvoicecoder");
		}
		//faxparameters end
		//max_ptime start
		if(strcmp(tmp_cmd->key,"max_ptime")==0){
			strcpy(tmp_cmd->key, "sdpmaxptime");
		}
		//max_ptime end
		//call_waiting_option start
		if(strcmp(tmp_cmd->key,"call_waiting_option")==0){
			strcpy(tmp_cmd->key, "callwaitingopt");
		}
		//call_waiting_option end
		//threeway_option start
		if(strcmp(tmp_cmd->key,"threeway_option")==0){
			strcpy(tmp_cmd->key, "threeway");
		}
		//threeway_option end
		//subscribe_ start
		if(strcmp(tmp_cmd->key,"subscribe_reg_expires_value")==0){
			strcpy(tmp_cmd->key, "subscriberegtimeout");
		}else if(strcmp(tmp_cmd->key,"subscribe_ua_expires_value")==0){
			strcpy(tmp_cmd->key, "subscribeuatimeout");
		}
		//subscribe_ end
		//mwisubscribetimeout start
		if(strcmp(tmp_cmd->key,"mwi_subscribe_expire_time")==0){
			strcpy(tmp_cmd->key, "mwisubscribetimeout");
		}
		//mwisubscribetimeout end
		//ring_vrms_volt start
		if(strcmp(tmp_cmd->key,"ring_vrms_volt")==0){
			strcpy(tmp_cmd->key, "ringvrms");
		}
		//ring_vrms_volt end
		//ajb start
		if(strcmp(tmp_cmd->key,"ajb_max_delay")==0){
			strcpy(tmp_cmd->key, "jbconfiglevel");
		}else if(strcmp(tmp_cmd->key,"ajb_normal_delay")==0){
			strcpy(tmp_cmd->key, "jbnormaldelay");
		}
		//ajb end
		//g723_bitrate start
		if(strcmp(tmp_cmd->key,"g723_bitrate")==0){
			strcpy(tmp_cmd->key, "codecrate");
			if(strstr(value,"6.3")!=NULL){
				strcpy(value, "2");
			}else if(strstr(value,"5.3")!=NULL){
				strcpy(value, "1");
			}
		}
		//g723_bitrate end
		//ptime start
		if(strcmp(tmp_cmd->key,"ptime")==0){
			strcpy(tmp_cmd->key, "txpacketsize");
		}
		//ptime end
		//codec_list start
		if(strcmp(tmp_cmd->key,"codec_list+")==0){
			strcpy(tmp_cmd->key, "codecenable");
		}else if(strcmp(tmp_cmd->key,"codec_list-")==0){
			strcpy(tmp_cmd->key, "codecremove");
		}else if(strcmp(tmp_cmd->key,"payloads+")==0){
			strcpy(tmp_cmd->key, "payloadtype");
		}
		//codec_list end
		if(strstr(tmp_cmd->key,"_") != NULL){
			ubus_voip_delete_sub_str(tmp_cmd->key, "_",tmp_cmd->key);
		}
		sprintf(cmd, "fvcli set %s %s %s\n", tmp_cmd->key,ep, value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
	}
	//cmdlist fvcli_set_ep end
	//cmdlist sip_proxies start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "sip_proxies")==0) {
		if(value[1]=='0'){
			strcpy(master_check, "0");
		}else if(value[1]=='1'){
			strcpy(master_check, "1");
		}
		ubus_voip_delete_sub_str(value,"[",value);
		ubus_voip_delete_sub_str(value,"]",value);
		strcpy(sip_proxies, value);
		char *p = strchr(value + 2, ':');
		if (p) {
			strncpy(addr, value + 2, p - value - 2);
			addr[p - value - 2] = '\0';
			strcpy(port, p + 1);
			ubus_voip_delete_sub_str(port,"\'",port);
			sprintf(cmd, "fvcli set sipproxyaddrs %s %s %s", ep,master_check,addr);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
			//port 
			sprintf(cmd, "fvcli set sipproxyports %s %s %s\n",ep, master_check,port);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
		}
		memmove(value, value+2, strlen(value)-1);
		value[strlen(value)-1] = '\0';
		sprintf(cmd, "fvcli set sipproxy %s %s\n",ep,value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
		if(strlen(sip_proxies_2)>0&&strlen(sip_proxies)>0){
			sprintf(cmd, "fvcli set sipproxys %s %s\n",ep,sip_proxies);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
			//set slave
			sprintf(cmd, "fvcli set sipproxys %s %s\n",ep,sip_proxies_2);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
		
		}
		
	}
	//cmdlist sip_proxies end
	//cmdlist sipregistrar start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "sip_registrars")==0) {
		if(value[1]=='0'){
			strcpy(master_check, "0");
		}else if(value[1]=='1'){
			strcpy(master_check, "1");
		}
		ubus_voip_delete_sub_str(value,"[",value);
		ubus_voip_delete_sub_str(value,"]",value);
		strcpy(sip_registrars, value);
		char *p = strchr(value + 2, ':');
		if (p) {
			strncpy(addr, value + 2, p - value - 2);
			addr[p - value - 2] = '\0';
			strcpy(port, p + 1);
			printf("addr: %s\n", addr);
			printf("port: %s\n", port);
			ubus_voip_delete_sub_str(port,"\'",port);

			sprintf(cmd, "fvcli set sipregistraraddrs %s %s %s\n", ep,master_check,addr);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
			//port 
	
			sprintf(cmd, "fvcli set sipregistrarports %s %s %s\n", ep,master_check,port);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
		}
		sprintf(cmd, "fvcli set sipregistrar %s %s\n",ep,value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
		if(strlen(sip_registrars_2)>0&&strlen(sip_registrars)>0){
			sprintf(cmd, "fvcli set sipregistrars %s %s\n",ep,sip_registrars);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
			//set slave
			sprintf(cmd, "fvcli set sipregistrars %s %s\n",ep,sip_registrars_2);
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd){
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					printf("system reply=%s\n", line);
					strcat(results,"[");
					strcat(results,line);
					strcat(results,"]");
				}
			}else{
				printf("fp_cmd=NULL\n");
			}
			pclose(fp_cmd);
		
		}
		
	}
	//cmdlist sipregistrar end

	//cmdlist outbound_proxies start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "outbound_proxies")==0) {
		if(value[1]=='0'){
			strcpy(master_check, "0");
		}else if(value[1]=='1'){
			strcpy(master_check, "1");
		}
		ubus_voip_delete_sub_str(value,"[",value);
		ubus_voip_delete_sub_str(value,"]",value);
		//printf("value[1]=%c\n",value[1]);
		char *p = strchr(value + 2, ':');
			if (p) {
				strncpy(addr, value + 2, p - value - 2);
				addr[p - value - 2] = '\0';
				strcpy(port, p + 1);
				ubus_voip_delete_sub_str(port,"\'",port);

				sprintf(cmd, "fvcli set outboundproxyaddrs %s %s %s\n", ep,master_check,addr);
				fp_cmd = popen(cmd, "r");
				if (NULL != fp_cmd){
					while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
					{
						printf("system reply=%s\n", line);
						strcat(results,"[");
						strcat(results,line);
						strcat(results,"]");
					}
				}else{
					printf("fp_cmd=NULL\n");
				}
				pclose(fp_cmd);
				//port 
				sprintf(cmd, "fvcli set outboundproxyports %s %s %s\n",ep,master_check, port);
				fp_cmd = popen(cmd, "r");
				if (NULL != fp_cmd){
					while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
					{
						printf("system reply=%s\n", line);
						strcat(results,"[");
						strcat(results,line);
						strcat(results,"]");
					}
				}else{
					printf("fp_cmd=NULL\n");
				}
				pclose(fp_cmd);
			}
		
		
		sprintf(cmd, "fvcli set sipregistrar %s %s",ep,value);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
	}
	//cmdlist outbound_proxies end

	//cmdlist session_timer start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "session_timer")==0) {
		char *token = strtok(value, " ");
		char sessionrefresher[BUF_SIZE_1],sessiontimerminse[BUF_SIZE_1], sessionrefreshtimeout[BUF_SIZE_1];
		ubus_voip_delete_sub_str(token,"\'",token);
		strcpy(sessionrefreshtimeout , token);
		token = strtok(NULL, " ");
		strcpy(sessiontimerminse, token);
		token = strtok(NULL, " ");
		strcpy(sessionrefresher, token);
		
		sprintf(cmd, "fvcli set sessionrefreshtimeout %s %s\n",ep,sessionrefreshtimeout);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		//sessiontimerminse
		sprintf(cmd, "fvcli set sessiontimerminse %s %s\n",ep,sessiontimerminse);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		//sessionrefresher
		sprintf(cmd, "fvcli set sessionrefresher %s %s\n",ep,sessionrefresher);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		
	}
	//cmdlist session_timer end
	//cmdlist rtpportlimits start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "rtpportlimits_1")==0) {
		strcpy(rtpportlimits, value);
		sprintf(cmd, "fvcli set rtpportbase %s\n",rtpportlimits);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}
		pclose(fp_cmd);
		if(strlen(rtpportlimits_2)!=0){
		sprintf(cmd, "fvcli set rtpportlimits %s %s\n",rtpportlimits, rtpportlimits_2);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		}
	}
	//cmdlist rtpportlimits1 end
	//cmdlist rtpportlimits2 start
	if(strcmp(key, tmp_cmd->key) == 0&&strcmp(tmp_cmd->action, "rtpportlimits_2")==0) {
		strcpy(rtpportlimits_2, value);
		sprintf(cmd, "fvcli set rtpportlimit %s\n",rtpportlimits_2);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		if(strlen(rtpportlimits)!=0){
		sprintf(cmd, "fvcli set rtpportlimits %s %s\n",rtpportlimits, rtpportlimits_2);
		fp_cmd = popen(cmd, "r");
		if (NULL != fp_cmd){
			while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
			{
				printf("system reply=%s\n", line);
				strcat(results,"[");
				strcat(results,line);
				strcat(results,"]");
			}
		}else{
			printf("fp_cmd=NULL\n");
		}

		pclose(fp_cmd);
		}
	}
	
	//cmdlist rtpportlimits2 end
	}

	list_for_each_entry_safe(tmp_cmd, tmp_cmd_next, &cmdlist.list, list) {
		list_del(&tmp_cmd->list);
		free(tmp_cmd);
	}
	return results;
	//return line;
}
void ubus_voip_start_log_config();
void ubus_voip_start_log_config() {
	// Restart the logd service system.@system[0].log_size
	//util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "\nOneway Dely Measurement:\n");
	system("uci set system.@system[0].log_type=file");
	system("uci commit system");
	system("uci set system.@system[0].log_file=/var/log/fvt_evoip.log");
	system("uci commit system");
	system("uci set system.@system[0].log_level=debug");
	system("uci commit system");
	system("uci set system.@system[0].log_size=8"); //KitB
	system("uci commit system");
	//system("/etc/init.d/log restart");
}
void ubus_voip_stop_log_config();
void ubus_voip_stop_log_config() {
	// Restart the logd service 
	/*system("uci delete system.log_type");
	system("uci delete system.log_file");
	system("uci delete system.log_level");
	system("uci delete system.log_size");
	system("uci commit system");*/
	//system("/etc/init.d/log restart");
}
bool ubus_voip_uci_commit_package(char* input_package);
bool ubus_voip_uci_commit_package(char* input_package) {
    struct uci_context *ctx = uci_alloc_context();
    if (!ctx) {
        printf("Failed to allocate UCI context\n");
        syslog(LOG_NOTICE, "Failed to allocate UCI context");
        return false;
    }

    char *pkg_name = (char *)input_package;
    struct uci_package *pkg = NULL;
    if (uci_load(ctx, pkg_name, &pkg) != UCI_OK) {
        printf("Failed to load package\n");
        syslog(LOG_NOTICE, "Failed to load package");
        uci_free_context(ctx);
        return false;
    }

    int ret = uci_commit(ctx, &pkg, false);
    uci_unload(ctx, pkg);
    uci_free_context(ctx);

    if (ret != UCI_OK) {
        printf("Failed to commit package\n");
        syslog(LOG_NOTICE, "Failed to commit package");
        return false;
    }

    return true;
}
bool ubus_voip_uci_revert_package(char *input_package);
bool ubus_voip_uci_revert_package(char *input_package) {
    struct uci_context *ctx = uci_alloc_context();
    if (!ctx) {
        printf("Failed to allocate UCI context\n");
        syslog(LOG_NOTICE, "Failed to allocate UCI context");
        return false;
    }

    char *pkg_name = (char *)input_package;
    struct uci_ptr ptr = {
        .package = pkg_name,
    };
    if (uci_lookup_ptr(ctx, &ptr, pkg_name, 1) != UCI_OK) {
        printf("Failed to revert package\n");
        syslog(LOG_NOTICE, "Failed to revert package");
        uci_free_context(ctx);
        return false;
    }

    int ret = uci_revert(ctx, &ptr);
    uci_unload(ctx, ptr.p);
    uci_free_context(ctx);

    if (ret != UCI_OK) {
        printf("Failed to revert package\n");
        syslog(LOG_NOTICE, "Failed to revert package");
        return false;
    }

    return true;
}
void ubus_voip_uci_changes(const char *owner);
void ubus_voip_uci_changes(const char *owner)
{
	// Set the log mask to include messages of LOG_NOTICE and higher levels from facility LOG_LOCAL1
	//util_dbprintf(3, LOG_ERR, 0,"\n\nmark test for util_dbprintf\n\n");
    //ubus_voip_start_log_config();
	//openlog ("fvt_evoip", LOG_CONS | LOG_PID | LOG_ODELAY, LOG_LOCAL1);
	//setlogmask(LOG_MASK(LOG_NOTICE));
	//setlogmask(LOG_MASK(LOG_LOCAL1));
	syslog (LOG_NOTICE, "version");
	char buf[BUF_SIZE_1];
	char input_section[BUF_SIZE_1];
    char input_key[BUF_SIZE_1];
	char space[1]=" ";
	FILE *fp;
	struct ubus_voip_changes_list *tmp, *tmp_next;
	struct list_head tmplist;
    INIT_LIST_HEAD(&tmplist);
	int ret=1;
	
    fp = popen("uci changes fvt_evoip", "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening pipe\n");
		syslog (LOG_NOTICE, "Error opening pipe");
        return;
    }

    while (fgets(buf, BUF_SIZE_1, fp) != NULL)
    {
		char *key = strtok(buf, "=");
		//Deal uci entry is ' ' and would be -fvt_evoip.section.key
		if(buf[0]=='-'){
			key = strtok(buf, "-");
			ubus_voip_delete_sub_str(key, "\n", key);
			strcat(key, "-");
			printf("%s\n",key);
		}
		if (key != NULL) {
			char *value = strtok(NULL, "=");
			if(buf[0]=='-'){
				value = space;
			}
			if (value != NULL) {
				tmp= (struct ubus_voip_changes_list *)malloc(sizeof(struct ubus_voip_changes_list));
				strcpy(tmp->value, value);
				strcpy(tmp->key, key);
				ubus_voip_delete_sub_str(tmp->value,"\'",tmp->value);
				ubus_voip_delete_sub_str(tmp->value,"\n",tmp->value);
				ubus_voip_delete_sub_str(tmp->key,"\n",tmp->key);
				list_add(&tmp->list, &tmplist);
			}
		}
    }
	syslog (LOG_NOTICE, "Start to set voip");
	list_for_each_entry(tmp, &tmplist, list) {
	ubus_voip_delete_sub_str(tmp->value,"\'",tmp->value);
	sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
	strcpy(tmp->ret,ubus_voip_cmd_list_set(input_section,input_key, tmp->value));
	syslog (LOG_NOTICE, "fvcli set [%s] as [%s] result: %s", tmp->key,tmp->value,tmp->ret);
	if(tmp->ret==NULL){
		ret=-1;
	}
	}
	ret=-1;
	printf("\n\n-------ret=%d\n",ret);
	//Deal fvcli result 
	if(ret ==1){
		//fvcli set ok
		syslog (LOG_NOTICE, "UCI commit fvt_evoip");
		bool check = ubus_voip_uci_commit_package("fvt_evoip");
		if (!check) {
			syslog (LOG_NOTICE, "UCI commit fvt_evoip failed");
		}
		//change the fvt_evoip_uci.config file
		list_for_each_entry(tmp, &tmplist, list) {
		//syslog (LOG_NOTICE, "tmp->key = %s\n tmp->value=%s\n", tmp->key,tmp->value);
		ubus_voip_set_fv_voip(tmp->key,tmp->value);
		}
	}else if(ret ==-1){
		//fvcli set fail
		syslog (LOG_NOTICE, "fvcli set failed");
		bool check = ubus_voip_uci_revert_package("fvt_evoip");
		if (!check) {
			syslog (LOG_NOTICE, "UCI commit fvt_evoip failed");
		}
		//voip revert
		list_for_each_entry(tmp, &tmplist, list) {
		//when reversing, change the + to - and the - to +
		if(strcmp(tmp->value, space) == 0){
			ubus_voip_delete_sub_str(tmp->key,"-",tmp->key);
		}
		if(strstr(tmp->key,"+")!=NULL){
			for(int i=0;i<strlen(input_key);i++){
				if(input_key[i] == '+'){
					input_key[i] = '-';
				}
			}
			strcpy(tmp->ret,ubus_voip_cmd_list_set(input_section,input_key, tmp->value));
			syslog (LOG_NOTICE, "fvcli revert [%s] as [%s] result: %s", tmp->key,tmp->value,tmp->ret);

		}else if(strstr(tmp->key,"-")!=NULL){
			sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
			for(int i=0;i<strlen(input_key);i++){
				if(input_key[i] == '-'){
					input_key[i] = '+';
				}
			}
			strcpy(tmp->ret,ubus_voip_cmd_list_set(input_section,input_key, tmp->value));
			syslog (LOG_NOTICE, "fvcli revert [%s] as [%s] result: %s", tmp->key,tmp->value,tmp->ret);
		}else{
		//uci test
		struct uci_context *ctx = uci_alloc_context();
		if (!ctx) {
			printf("Failed to allocate UCI context\n");
			syslog (LOG_NOTICE, "Failed to allocate UCI context");
		}

		// uci get
		sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
		struct uci_ptr ptr = {
			.package = "fvt_evoip",
			.section = input_section,
			.option = input_key,
		};
		if (uci_lookup_ptr(ctx, &ptr, NULL, 1) == UCI_OK) {
			//printf("mark uci get = %s\n", ptr.o->v.string);
			strcpy(tmp->ret,ubus_voip_cmd_list_set(input_section,input_key, ptr.o->v.string));
			syslog (LOG_NOTICE, "fvcli revert [%s] as [%s] result: %s", tmp->key,ptr.o->v.string,tmp->ret);
		} else {
			printf("Failed to get uci parameter\n");
			syslog (LOG_NOTICE, "Failed to get uci parameter");
		}
		uci_free_context(ctx);
		}
		}
	}
	char tmp_callback[BUF_SIZE_1] = {0};
	if (ret == 1) {
		strcpy(tmp_callback, "sucessful");
	} else if (ret == -1) {
		strcpy(tmp_callback, "failed");
	}
	
	/*int ret_call = call_fvt_evoip_req(owner, tmp_callback);
	if (ret_call != 0) {
		printf("Failed to call fvt_evoip req method\n");
		syslog (LOG_NOTICE, "Failed to call fvt_evoip req method");
	}*/
    list_for_each_entry_safe(tmp, tmp_next, &tmplist, list) {
		list_del(&tmp->list);
		free(tmp);
	}
    if (pclose(fp) != 0)
    {
        fprintf(stderr, "Error closing pipe\n");
		syslog (LOG_NOTICE, "Error closing pipe");
    }
	syslog (LOG_NOTICE, "Session ended");
	//ubus_voip_stop_log_config();
	//closelog ();
}

static int
ubus_voip_cmd_end(struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg)
{
	fprintf(stderr,"mark test notify protocol\n");
	char owner[BUF_SIZE_1]="uci";
    // Call ubus_voip_uci_changes function with owner as argument
    ubus_voip_uci_changes(owner);
	return 0;
}
int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "s:t:")) != -1) {
		switch (ch) {
		case 's':
			ubus_socket = optarg;
			break;

		case 't':
			rpc_exec_timeout = 1000 * strtol(optarg, NULL, 0);
			break;

		default:
			break;
		}
	}
	if (rpc_exec_timeout < 1000 || rpc_exec_timeout > 600000) {
		fprintf(stderr, "Invalid execution timeout specified\n");
		return -1;
	}
	signal(SIGPIPE, SIG_IGN); 
	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}
	ubus_add_uloop(ctx);
	static const struct ubus_method uci_methods[] = {
		{ .name = "cmd_end", .handler = ubus_voip_cmd_end }
	};
	
	static struct ubus_object_type uci_type =
	UBUS_OBJECT_TYPE("fvt_evoip", uci_methods);
	fprintf(stdout, "Finish init uci_type!\n");
	static struct ubus_object obj = {
		.name = "fvt_evoip",
		.type = &uci_type,
		.methods = uci_methods,
		.n_methods = ARRAY_SIZE(uci_methods),
	};
	ubus_add_object(ctx, &obj);
	uloop_run();
	ubus_free(ctx);
	uloop_done();
	return 0;
}
