/******************************************************************
 *
 * Copyright (C) 2015 5V Technologies Ltd.
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
 * Module  : client_prog
 * File    : clitree.h
 *
 ******************************************************************/

#include "list.h"

#define CLITREE_HASH_TABLE_SIZE		4001	// 4001, 8191, 16001
struct cli_node_t {
	unsigned short index;
	unsigned char is_optional;
	unsigned char is_end;
	char *keyword;
	char *help;
	char *validate;
	struct cli_node_t *parent;		// pointer to parent node
	struct list_head child_opt_list;
	struct list_head child_list;
	struct list_head cli_node;		// anchor for parent child_opt_list or child_list
	struct list_head cli_hash_node;		// anchor for global hash table
};

#define CLI_FILE_COMMENT \
	"#\n" \
	"# clitree cli definition file\n" \
	"#\n" \
	"# keyword : a static token that must appears in a cli command\n" \
	"# <string>: a variable token that must appears in a cli command, \n" \
	"#           it is a wildcard match, anything in <> will be ignored, so you can put short description in <>\n" \
	"# [...]   : [] means an optional token in the cli\n" \
	"#\n" \
	"# ps: space char is not allowed in any of the above tokens\n" \
	"#\n" \
	"# example:\n" \
	"# adduser <username> <homedir>\n" \
	"# adduser username=<name> homedir=<dir>\n" \
	"# adduser username=<name> homedir=<dir> [uid=<0..65535>] [gid=<0..65535>]\n" \
	"#\n"

#define HELP_FILE_COMMENT \
	"#\n" \
	"# clitree help definition file\n" \
	"#\n" \
	"# @cmd [args...]		: the cli command base for keywords following this cli\n" \
	"# keyword - help description 	: define the help for the keyword\n" \
	"#				  the keyword must be exactly the same as the one in cli definition\n" \
	"#\n"

#define VALIDATE_FILE_COMMENT \
	"#\n" \
	"# clitree validate definition file\n" \
	"#\n" \
	"# @cmd [args...]		 : the cli command base for <variable> following this cli\n" \
	"# <variable> - validate pattern : define the validate for the <variable>\n" \
	"#				   the <variable> must be exactly the same as the one in cli definition\n" \
	"#\n"

/* clitree.c */
void clitree_hash_info_print(void);
void clitree_debug_enable(int enable);
int clitree_load(struct cli_node_t *root, char *filename);
int clitree_load_help(struct cli_node_t *root, char *filename);
int clitree_load_validate(struct cli_node_t *root, char *filename);
int clitree_destroy(struct cli_node_t *root);
int clitree_save_tree(struct cli_node_t *root, char *filename);
int clitree_save_cli(struct cli_node_t *root, char *filename);
int clitree_save_help(struct cli_node_t *root, char *filename);
int clitree_save_validate(struct cli_node_t *root, char *filename);
int clitree_line_get_matchlist(struct cli_node_t *root, char *line, int with_help, char ***matchlist);
int clitree_line_get_replace(struct cli_node_t *root, char *line, char *line_result, int size);
char *clitree_get_complete_by_matchlist(char *token, char **matchlist, int match);
