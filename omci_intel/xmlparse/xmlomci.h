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
 * Module  : xmlparse
 * File    : xmlomci.h
 *
 ******************************************************************/

#ifndef __XMLOMCI_H__
#define __XMLOMCI_H__

#include "list.h"

/* xmlomci.c */
int xmlomci_strlist2codepoints(char *str, struct list_head *code_points_list_ptr);
int xmlomci_loadstr(char **new_ptr, char *old_ptr);
char *xmlomci_abbrestr(char *str);
int xmlomci_init(char *env_file_path, char *spec_passwd, char *config_passwd, char *er_group_passwd);

/* xmlomci_config.c */
int xmlomci_config_init(char *spec_xml_filename);

/* xmlomci_env.c */
int xmlomci_env_init(char *env_xml_filename);

/* xmlomci_er_group.c */
int xmlomci_er_group_init(char *er_group_xml_filename);

/* xmlomci_spec.c */
int xmlomci_spec_init(char *spec_xml_filename);

#endif
