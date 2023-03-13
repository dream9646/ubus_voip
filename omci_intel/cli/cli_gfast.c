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
 * Module  : cli
 * File    : cli_gfast.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "util.h"
#include "util_run.h"
#include "conv.h"
#include "cli.h"
#include "env.h"
#include "omcimain.h"
#include "memdbg.h"
#include "fwk_msgq.h"
#include "clitask.h"

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////
#define GFAST_PROMPT_ENABLE_LEN	19
#define GFAST_PROMPT_ENABLE	"(gfast-cli)> enable"
#define GFAST_PROMPT_LEN	13	// "(gfast-cli)# "
#define GFAST_PROMPT            "(gfast-cli)# "
#define GFAST_PROMPT_CFG_LEN	21	// "(gfast-cli)(config)# "
#define GFAST_PROMPT_CFG	"(gfast-cli)(config)# "
#define GFAST_SLEEP             500
#define GFAST_TIMEOUT           5 * 1000 / GFAST_SLEEP

static void
cli_gfast_long_help(int fd)
{
	util_fdprintf(fd, 
		"gfast help                              Show available commands\n"
		"      quit                              Disconnect\n"
		"      logout                            Disconnect\n"
		"      exit                              Exit from current mode\n"
		"      history                           Show a list of previously run commands\n"
		"      enable                            Turn on privileged commands\n"
		"      disable                           Turn off privileged commands\n"
		"      configure                         Enter configuration mode\n"
		"      read memory                       Read memory from device\n"
		"      write memory                      Write to device memory\n"
		"      show gfast-counters               Show G.fast counters for line\n"
		"      show network-info                 Show network information\n"
		"      show device-info                  Show device information\n"
		"      show inventory                    Show inventory information for line\n"
		"      show ETR                          Show Expected ThRoughput (ETR) for line\n"
		"      show dictionary                   Show parameter dictionary\n"
		"      show params                       Show device parameter values\n"
		"      show BITSps                       Show bit-loading per sub-carrier\n"
		"      show SNRps                        Show SNR per sub-carrier\n"
		"      show ALNps                        Show ALN per sub-carrier\n"
		"      show HLOGps                       Show HLOG per sub-carrier\n"
		"      show init-failure-cause           Show initialization failure causes\n"
		"      show 15min-counters               Show far-end line performance counters\n"
		"      show line state                   Show line state\n"
		"      show line performance-counters    Show (near-end) line performance counters\n"
		"      show channel status               Show channel status for line in US or DS direction\n"
		"      show channel performance-counters Show (near-end) line performance counters\n"
		"      show number 15min-intervals       Show number of 15 minute history intervals\n"
		"      show number devices               Show number of discovered G.fast devices\n"
		"      show number lines                 Show number of G.fast lines\n"
		"      dump 15min-history                Dump the 15-minute counter history database\n"
		"      restart line                      Restart a G.fast line or all active lines\n"
		"      restart system                    Restart the system\n"
		"      activate line                     Activate a line\n"
		"      deactivate line                   De-activate a line\n"
		"      trigger 15min                     Trigger a 15 minute event\n"
		"      rediscover                        Rediscover G.fast devices\n"
		"      reboot                            Reboot device\n"
		"      load bootloader                   Erase bootloader from flash\n"
		"      upgrade bootloader                Upgrade bootloader\n"
		"      upgrade manufacturer-block        Upgrade manufacturer block\n"
		"      upgrade firmware                  Upgrade firmware image\n"
		"      erase bootloader                  Erase bootloader from flash\n"
		"      erase manufacturer-block          Erase manufacturer block from flash\n"
		"      erase firmware                    Erase firmware image from flash\n"
		"      run                               Run a CLI script file\n"
		);
}

void
cli_gfast_help(int fd)
{
	util_fdprintf(fd, "gfast help|[subcmd...]\n");
}

static void
cli_gfast_process_output(int fd, char *filename)
{
        FILE *fp;
        char buff[1024];
        int line_num = 0;

        if ((fp = fopen(filename, "r")) == NULL) {
		util_fdprintf(fd, "exec gfast-cli err?\n");
	}
	
	while ((fgets(buff, sizeof(buff), fp)) != NULL ) {
		line_num++;

		// skip dummy lines
		if (line_num <= 2)	// skip first 5 line (which is the cmd itself)
			continue;
                // skip empty line
		if (strlen(buff) == 2)
			continue;
		if (strlen(buff)>=GFAST_PROMPT_ENABLE_LEN && strncmp(buff, GFAST_PROMPT_ENABLE, GFAST_PROMPT_ENABLE_LEN) ==0)
			continue;
		if (strlen(buff)>=GFAST_PROMPT_LEN && strncmp(buff, GFAST_PROMPT, GFAST_PROMPT_LEN) ==0)
			continue;
		if (strlen(buff)>=GFAST_PROMPT_CFG_LEN && strncmp(buff, GFAST_PROMPT_CFG, GFAST_PROMPT_LEN) ==0)
			continue;                                   		
		//print output asis
		util_fdprintf(fd, "%s", buff);
	}
	fclose(fp);
}

int
cli_gfast_cmdline(int fd, int argc, char *argv[])
{
	int config_mod = 0;
	int timeout_delay = 0;
	
	if (argc == 1 && strcmp(argv[0], "gfast")==0) {
		cli_gfast_long_help(fd);
		return CLI_OK;
	
	} else if (argc>=2 && strcmp(argv[0], "gfast")==0) {
		if (strcmp(argv[1], "help")==0) {
			cli_gfast_long_help(fd);
			return CLI_OK;	 
		}
		else
		{
                        FILE *fp_cmd;
                        char buff[1024], filename[64];
                        
                        snprintf(filename, sizeof(filename), "/tmp/tmpfile.%d.%06ld", getpid(), random()%1000000);
                        snprintf(buff, sizeof(buff), "telnet localhost:8000 > %s 2>/dev/null", filename);
                        
                        if ((fp_cmd = popen(buff, "w")) == NULL) {
                                return CLI_ERROR_INTERNAL;
                        }
                        
                        char *p;
                        int i;
                        memset(buff, 0, sizeof(buff));
                        for (i= 1, p = buff; i < argc; i++)
                        {
                        	if (strcmp(argv[1], "configure")==0) {
                        		argv[1] = "configure\r";
                        		config_mod=1;
                        	}
                                strncat(buff, argv[i], strlen(argv[i]));
                                p += strlen(argv[i]);
                                *p = ' ';
                                p++;                                 
                        }
                        *p = '\r';
                        
                        //util_fdprintf(fd, "command - %s\n", buff);
			if (strncmp("upgrade firmware", buff, 16)==0)
				timeout_delay = 1;
                        fprintf(fp_cmd, "enable\r");
                        fprintf(fp_cmd, "%s", buff);
                        fflush(fp_cmd);
                        
                        int count = 0;
                        struct timespec tim;
                        tim.tv_sec = 0;
                        tim.tv_nsec = GFAST_SLEEP * 1000 * 1000;
                               
                        while (!nanosleep(&tim, NULL))
                        {
                                if (count == GFAST_TIMEOUT && !timeout_delay)
                                        break;
				else if (count == (GFAST_TIMEOUT * 10))
                                        break;
                                               
                                char buffer[32];
                                FILE *fp_check;
				
				if (config_mod)
                                	snprintf(buff, sizeof(buff), "cat %s | grep \"(gfast-cli)(config)#\" | wc -l", filename);
                                else                                
                                	snprintf(buff, sizeof(buff), "cat %s | grep \"(gfast-cli)#\" | wc -l", filename);
                                
                                if ((fp_check = popen(buff, "r")) == NULL) {
                                        pclose(fp_cmd);
                                        unlink(filename);
                                        return CLI_ERROR_INTERNAL;
                                }
                                
                                if (fgets(buffer, sizeof(buffer), fp_check) != NULL)
                                {
                                        int c = atoi(buffer);
                                        if (c == 2)
                                        {
                                                cli_gfast_process_output(fd, filename);
                                                snprintf(buff, sizeof(buff), "logout\r");
                                                fprintf(fp_cmd, "%s", buff);
                                                fflush(fp_cmd);
                                                usleep(1000);
                                                pclose(fp_check);
                                                pclose(fp_cmd);
                                                unlink(filename);
                                                return CLI_OK;
                                        }
                                }
                                pclose(fp_check);
                                count++;
                        }
                        snprintf(buff, sizeof(buff), "logout\r");
                        fprintf(fp_cmd, "%s", buff);
                        fflush(fp_cmd);
                        usleep(1000);
                        pclose(fp_cmd);
                        unlink(filename);
                }
	}
	
	return CLI_ERROR_OTHER_CATEGORY;
}
