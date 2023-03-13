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
 * File    : clitree.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "list.h"
#include "clitree.h"

// basically, all tokens are formed into a token tree
// each child token belongs to the child_list or child_opt_list of the parent,
// if child_list or child_opt_list is long (eg:more than 8000 nodes), the lookup would be slow
// the global hash table provides a quick lookup for (parent_node, child_token) -> child_node

static struct list_head clitree_hash_table[CLITREE_HASH_TABLE_SIZE];
static int node_index_g = 1;
static int clitree_debug = 0;

/* strtrim : removes leading spaces of a string */
static inline char *
strtrim(char *s)
{
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

/* strrtrim : removes trailing spaces from the end of a string */
static inline char *
strrtrim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	return s;
}

/* util_trim : removes leading and trailing spaces of a string */
static char *
util_trim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

static int
conv_str2array(char *s, char *delim, char *array[], int array_size)
{
	int dlen = strlen(delim);
	int i = 0;
	char *p;

	while (s != NULL) {
		p = strstr(s, delim);
		if (p != NULL) {
			*p = 0x0;
			array[i] = util_trim(s);
			if (strlen(array[i])>0)
				i++;
			s = p + dlen;
			if (i == array_size)
				break;
		} else {
			break;
		}
	}
	if (s != NULL && i < array_size) {
		array[i] = util_trim(s);
		if (strlen(array[i])>0)
			i++;
	}

	return i;
}

//////////////////////////////////////////////////////////////////////////

static int
is_optional_token(char *token)
{
	if (token && token[0]=='[' && token[strlen(token)-1]==']')
		return 1;
	return 0;
}
static char *
strdup_token_from_optional_token(char *optional_token)
{
	int len=strlen(optional_token)-2;
	char *token=malloc(len+1);	
	strncpy(token, optional_token+1, len);
	token[len] = 0;
	return token;
}

// cli node in child list is ascending sorted, and cli definition file is also ascending sorted
// we traverse the child list from bottom to top to optomize the add of new cli node from cli definition file
// return 0: not found, last_visit_child is the position for insert, add from head
// return 1: found, last_visit_child is the found one
static int
clitree_child_locate(struct list_head *child_list, char *token, struct cli_node_t **last_visit_child)
{
	struct cli_node_t *child = NULL;
	int cmp;
	list_for_each_entry_reverse(child, child_list, cli_node) {
		cmp = strcmp(child->keyword, token);
		if (cmp == 0) {
			*last_visit_child = child;
			return 1;
		} else if (cmp < 0) {
			break;	// child is ascending sorted, search is descending
		}
	}
	*last_visit_child = child;
	return 0;
}

#if 0
// this function is slow, it is replaced by clitree_child_find_by_hash()
static struct cli_node_t *
clitree_child_find(struct cli_node_t *node, char *token)
{
	char *token2;
	struct list_head *child_list; 
	struct cli_node_t *found_child;

	if (is_optional_token(token)) { 
		token2 = strdup_token_from_optional_token(token);	// strip []
		child_list = &node->child_opt_list;
	} else {
		token2 = strdup(token);
		child_list = &node->child_list;
	}
	if (clitree_child_locate(child_list, token2, &found_child)) {	// child found
		free(token2);
		return found_child;
	}
	free(token2);
	return NULL;
}
#endif

static inline int
clitree_hash_value(struct cli_node_t *parent, char *node_keyword)
{
	int node_len = strlen(node_keyword);
	unsigned int i, sum = ((unsigned int)parent)/4 + node_len;
	unsigned int tmp;
	for (i=0; i< node_len/4; i++) {
		memcpy(&tmp,(node_keyword+i*4),sizeof(unsigned int));
		sum = (sum << 1) + tmp;
	}
	switch (node_len%4) {
		case 1:  return (sum+node_keyword[node_len-1])%CLITREE_HASH_TABLE_SIZE;
		case 2:  return (sum+node_keyword[node_len-2]+node_keyword[node_len-1])%CLITREE_HASH_TABLE_SIZE;
		case 3:  return (sum+node_keyword[node_len-3]+node_keyword[node_len-2]+node_keyword[node_len-1])%CLITREE_HASH_TABLE_SIZE;
	}
	return sum%CLITREE_HASH_TABLE_SIZE;
}
void
clitree_hash_info_print(void)
{
	int count_table[CLITREE_HASH_TABLE_SIZE]={0}, i;
	int slot_total = 0, node_total = 0;

	for (i=0; i<CLITREE_HASH_TABLE_SIZE; i++) {
		struct cli_node_t *node; 
		int count = 0;
		list_for_each_entry(node, &clitree_hash_table[i], cli_hash_node)
			count++;
		count_table[count]++;
	}
	for (i=0; i<CLITREE_HASH_TABLE_SIZE; i++) {
		if (count_table[i]) {
			printf("len %4d: %4d\n", i, count_table[i]);
			if (i >0) {
				slot_total += count_table[i];
				node_total += count_table[i] * i;
			}
		}
	}
	printf("non-empty hash slot:%d, total node:%d, avg hash len:%d.%02d\n", slot_total, node_total, node_total/slot_total, (node_total*100/slot_total)%100);
}

static struct cli_node_t *
clitree_child_find_by_hash(struct cli_node_t *parent, char *node_keyword)
{
	unsigned int hash_value = clitree_hash_value(parent, node_keyword);
	struct cli_node_t *node;	
	list_for_each_entry(node, &clitree_hash_table[hash_value], cli_hash_node) {
		if (node->parent == parent && strcmp(node->keyword, node_keyword) == 0)
			return node;
	}
	return NULL;
}

static struct cli_node_t *
clitree_child_add(struct cli_node_t *node, char *token)
{
	struct list_head *child_list; 
	struct cli_node_t *found_child;
	struct cli_node_t *child = malloc (sizeof(struct cli_node_t));
        
	if (!child)
		return NULL;

	child->is_optional = is_optional_token(token);	
	if (child->is_optional) {
		child->keyword = strdup_token_from_optional_token(token);
		child_list = &node->child_opt_list;
	} else {
		child->keyword = strdup(token);
		child_list = &node->child_list;
	}

	// child already exist
	if ((found_child=clitree_child_find_by_hash(node, child->keyword)) != NULL) {	
		free(child->keyword);
		free(child);
		return found_child;
	}

	// child not exist, find the nearest node for the keyword	
	clitree_child_locate(child_list, child->keyword, &found_child);
	
	// creat new node and return it
	INIT_LIST_HEAD(&child->child_list);	
	INIT_LIST_HEAD(&child->child_opt_list);	
	child->index = node_index_g;
	child->help = NULL;
	child->validate = NULL;
	child->is_end = 0;
	node_index_g++;	

	child->parent = node;
	list_add(&child->cli_node, &found_child->cli_node);
	list_add_tail(&child->cli_hash_node, &clitree_hash_table[clitree_hash_value(node, child->keyword)]);
	return child;
}
		
static struct cli_node_t *
clitree_child_add_help_validate(struct cli_node_t *node, char *token, char *desc, int is_help)
{
	char *token2;
	struct cli_node_t *found_child;

	if (is_optional_token(token)) { 
		// while keyword in help/validate file is assumed to no '[' & ']', but we strip them out if exist
		token2 = strdup_token_from_optional_token(token);	
	} else {
		token2 = strdup(token);
	}
	if ((found_child = clitree_child_find_by_hash(node, token2)) == NULL) {
		free(token2);
		return NULL;
	}
	free(token2);

	// child node found since here
	if (desc && strlen(desc) >=2) {	// assume desc should be at least 2 char
		if (is_help) {
			if (found_child->help == NULL) {
				found_child->help = strdup(desc);	// help will be loaded only if curr is null or too short
			} else if (strlen(found_child->help) <2) {
				free(found_child->help);
				found_child->help = strdup(desc);
			}
		} else {
			if (found_child->validate == NULL) {
				found_child->validate = strdup(desc);	// validate will be loaded only if curr is null or too short
			} else if (strlen(found_child->validate) <2) {
				free(found_child->validate);
				found_child->validate = strdup(desc);
			}
		}
	}
	return found_child;
}
		
static int 
clitree_child_del(struct cli_node_t *node)
{
	struct list_head *child_list[2] = { &node->child_opt_list, &node->child_list };
	struct cli_node_t *child, *child_next;
	int i;
	
	for (i=0; i<2; i++) {
		list_for_each_entry_safe(child, child_next, child_list[i], cli_node)
			clitree_child_del(child);
	}
	if (strcmp(node->keyword, "ROOT")!=0) {	// dont remove root node
		free(node->keyword);
		if (node->help)
			free(node->help);
		if (node->validate)
			free(node->validate);
		list_del(&node->cli_hash_node);
		list_del(&node->cli_node);
		free(node);
	}
	return 0;
}

static int
clitree_child_save_tree(FILE *fp, struct cli_node_t *node, int level)
{
	struct list_head *child_list[2] = { &node->child_opt_list, &node->child_list };
	struct cli_node_t *child;
	int i;
	
	for (i=0; i<level; i++)
		fprintf(fp, "%s", "\t");
	fprintf(fp, "%p-->%d:%s%s\n", node, node->index, node->keyword, (node->is_optional)?"(optional)":"");

	if (node->help && strlen(node->help)>0) {
		for (i=0; i<level; i++)
			fprintf(fp, "%s", "\t");
		fprintf(fp, "help=%s\n", node->help);
	}
	if (node->validate && strlen(node->validate)>0) {
		for (i=0; i<level; i++)
			fprintf(fp, "%s", "\t");
		fprintf(fp, "validate=%s\n", node->validate);
	}

	for (i=0; i<2; i++) {	
		list_for_each_entry(child, child_list[i], cli_node)
			clitree_child_save_tree(fp, child, level+1);
	}
	return 0;
}

static int
clitree_child_save_cli(FILE *fp, struct cli_node_t *node, char *parent_path)
{
	char node_path[1024];
	struct cli_node_t *child;
	char *s;

	if (strcmp(node->keyword, "ROOT") == 0) {
		node_path[0] = 0;
	} else if (strlen(parent_path) == 0 || parent_path[strlen(parent_path)-1] == ' ') {
		snprintf(node_path, 1023, "%s", node->keyword);			
	} else {
		snprintf(node_path, 1023, "%s %s", parent_path, node->keyword);
	}
	// add all optional child to node path with []
	s = node_path + strlen(node_path);
	list_for_each_entry(child, &node->child_opt_list, cli_node)
		s += snprintf(s, 1023-strlen(node_path), " [%s]", child->keyword);
	node_path[1023] = 0;

	if (strlen(parent_path) == 0 && strcmp(node->keyword, "ROOT") != 0)
		fprintf(fp, "\n# %s ################################\n", node->keyword);	

	// print cli if node->is_end
	if (node->is_end)
		fprintf(fp, "%s\n", node_path);

	// recursive to all no optional child
	list_for_each_entry(child, &node->child_list, cli_node)
		clitree_child_save_cli(fp, child, node_path);
	return 0;
}

static int
clitree_child_save_help_validate(FILE *fp, struct cli_node_t *node, char *parent_path, int is_help)
{
	struct list_head *child_list[2] = { &node->child_opt_list, &node->child_list };
	char node_path[1024];
	struct cli_node_t *child;
	int child_total = 0;
	int is_cli_printed = 0;
	int i;

	for (i=0; i<2; i++) {
		list_for_each_entry(child, child_list[i], cli_node)
			child_total++;
	}
	if (child_total == 0)
		return 0;

	if (strcmp(node->keyword, "ROOT") == 0) {
		node_path[0] = 0;
	} else if (strlen(parent_path) == 0 || parent_path[strlen(parent_path)-1] == ' ') {
		snprintf(node_path, 1023, "%s", node->keyword);
	} else {
		snprintf(node_path, 1023, "%s %s", parent_path, node->keyword);
	}
	node_path[1023] = 0;

	if (strlen(parent_path) == 0)
		fprintf(fp, "\n# %s ################################", node->keyword);	

	// print validate for current node
	for (i=0; i<2; i++) {
		list_for_each_entry(child, child_list[i], cli_node) {
			if (is_help) {
				if (!is_cli_printed) {
					fprintf(fp, "\n@%s\n", node_path);
					is_cli_printed = 1;
				}	
				fprintf(fp, "%s - %s\n", child->keyword, child->help?child->help:"");				
			} else if (child->keyword[0] == '<') {
				if (!is_cli_printed) {
					fprintf(fp, "\n@%s\n", node_path);
					is_cli_printed = 1;
				}	
				fprintf(fp, "%s - %s\n", child->keyword, child->validate?child->validate:"");
			}
		}
	}
	// recursive to all child
	for (i=0; i<2; i++) {
		list_for_each_entry(child, child_list[i], cli_node)
			clitree_child_save_help_validate(fp, child, node_path, is_help);
	}
	return 0;
}

// token fuzzy unique match for cli input completion lookup 
static int 
clitree_token_match(struct cli_node_t *node, char *token)
{
	int token_len = strlen(token);

	if (clitree_debug>=2) {
		if (node->keyword == NULL) {
			fprintf(stderr, "match token=%s, node %p %d:NULL?\n", token, node, node->index);
		} else {
			fprintf(stderr, "match token=%s, node %p %d:%s\n", token, node, node->index, node->keyword);
		}
	}
	
	if (strchr(node->keyword, '>')!=NULL) {
		char *s;
		if (node->keyword[0] == '<') 	// node token is <any_str>, match for any thing
			return 1;
		if ((s=strstr(node->keyword, "=<")) != NULL) {	// node keyword is key=<any_str>, match max to key=
			int cmp_len = s-node->keyword+1;
			if (cmp_len > token_len)
				cmp_len = token_len;
			if (strncmp(node->keyword, token, cmp_len) == 0)
				return 1;
		}
	}
	if (strncmp(node->keyword, token, strlen(token)) == 0)	// node keyword is key, match max to keyword 
		return 1;
	return 0;
}	

//////////////////////////////////////////////////////////////////////////

static struct cli_node_t *
clitree_line_token_find(struct cli_node_t *root, char *tokens[], int total)
{
	int i;
	struct cli_node_t *node = root;

	if (clitree_debug) {
		if (clitree_debug>=2)
			fprintf(stderr, "\n");	
		fprintf(stderr, "line_token_find: ");
		for (i=0; i<total; i++)
			fprintf(stderr, "<%s> ", tokens[i]);
		fprintf(stderr, "\n");
	}
	for (i=0; i<total; i++) {
		//struct cli_node_t *child = clitree_child_find(node, tokens[i]);
		struct cli_node_t *child = clitree_child_find_by_hash(node, tokens[i]);
		if (child == NULL)
			return NULL;
		if (!is_optional_token(tokens[i]))
			node = child;
	}

	return node;
}

static int			
clitree_line_token_add(struct cli_node_t *root, char *tokens[], int total)
{
	int i;
	struct cli_node_t *node = root;

	if (clitree_debug) {
		if (clitree_debug>=2)
			fprintf(stderr, "\n");	
		fprintf(stderr, "line_token_add: ");
		for (i=0; i<total; i++)
			fprintf(stderr, "<%s> ", tokens[i]);
		fprintf(stderr, "\n");
	}
	for (i=0; i<total; i++) {
		struct cli_node_t *child = clitree_child_add(node, tokens[i]);
		if (child == NULL) {
			if (clitree_debug>=2)
				fprintf(stderr, "add node failed for keyword=%s\n", tokens[i]);
			return -1;
		}
		if (clitree_debug>=2)
			fprintf(stderr, "add %snode %p %d:%s\n", child->is_optional?"opt ":"", child, child->index, child->keyword);

		// recursive to child if child is not optional token
		if (!is_optional_token(tokens[i]))
			node = child;	
		// mark node as end if no more token for child
		if (i == total -1)	
			node->is_end = 1;
	}

	return 0;
}

static int
clitree_line_token_match(struct cli_node_t *root, char *tokens[], int total, int with_help, char ***matchlist)
{
	struct list_head *child_list[2];
	struct cli_node_t *node = root;
	struct cli_node_t *child, *matched_child;
	int *tokens_index=malloc(sizeof(int)*total);
	char **copy;
	int match, exact_match, i, j;
	char buff[128];

	if (clitree_debug) {
		fprintf(stderr, "line_token_match: ");
		for (i=0; i<total; i++)
			fprintf(stderr, "<%s> ", tokens[i]);
		fprintf(stderr, "\n");
	}

	if (total <= 0) {
		free(tokens_index);
		return 0;		// no tokens in line, break
	}

	// token except the last token should be either fuzzy or exact matched with one node in clitree
	for (i=0; i<total-1; i++) {
		child_list[0] = &node->child_opt_list;
		child_list[1] = &node->child_list;
		match = 0;
		exact_match = 0;
		for (j=0; j<2; j++) {
			list_for_each_entry(child, child_list[j], cli_node) {
				if (strcmp(child->keyword, tokens[i]) == 0 && strchr(child->keyword, '<') == NULL) {	// node key has no wildcard and exact matched with token
					matched_child = child;
					exact_match++;
					match++;
				} else if (clitree_token_match(child, tokens[i])) {
					if (!exact_match)
						matched_child = child;
					match++;
				}
			}
		}
		if (clitree_debug) {
			fprintf(stderr, "token %s, match=%d, exact_match=%d\n", tokens[i], match, exact_match);
		}
		if (match != 1 && exact_match != 1) {
			free(tokens_index);
			return 0;		// no uniq found on all child, break
		}
		tokens_index[i] = matched_child->index;	// record node already used
		if (!matched_child->is_optional)
			node = matched_child;
	}

	// get the match result with last token 'tokens[total-1]'
	copy = (char **) malloc (128 * sizeof(char *)); 	// assume no more than 128 match in the list
	match = 0;

	list_for_each_entry(child, &node->child_opt_list, cli_node) {
		// as optional token list will be reused repeatly
		// we only put node not used in the cli to the match list
		if (clitree_token_match(child, tokens[total-1])) {
			int optional_node_is_used = 0;
			for (i=0; i<total-1; i++) {
				if (child->index == tokens_index[i]) {
					optional_node_is_used = 1;
					break;
				}
			}
			if (!optional_node_is_used) {	
				if (match < 127) {
					if (with_help) {
						if (child->help && strlen(child->help)>0) {
							snprintf(buff, 127, "%-36s - %s", child->keyword, child->help);
						} else {
							snprintf(buff, 127, "%-36s -", child->keyword);
						}
						buff[127] = 0;
						copy[match] = strdup (buff);						
					} else {
						copy[match] = strdup (child->keyword);
					}
					match++;
				}
			}
		}
	}
	list_for_each_entry(child, &node->child_list, cli_node) {
		if (clitree_token_match(child, tokens[total-1])) {
			if (match <127) {
				if (with_help) {
					if (child->help && strlen(child->help)>0) {
						snprintf(buff, 127, "%-36s - %s", child->keyword, child->help);
					} else {
						snprintf(buff, 127, "%-36s -", child->keyword);
					}
					buff[127] = 0;
					copy[match] = strdup (buff);						
				} else {
					copy[match] = strdup (child->keyword);
				}
				match++;
			}
		}
	}
	if (strlen(tokens[total-1]) == 0 && node->is_end) {
		copy[match] = strdup("[enter]");
		match++;
	}	
	
	if (clitree_debug) {
		fprintf(stderr, "token %s, match=%d\n", tokens[total-1], match);
	}

	*matchlist = copy;
	free(tokens_index);	
	return match;
}

static int
clitree_line_token_replace(struct cli_node_t *root, char *tokens[], char *new_tokens[], int total)
{
	struct cli_node_t *node = root;
	struct cli_node_t *child, *matched_child;
	int match, i, j;

	if (clitree_debug) {
		fprintf(stderr, "line_token_replace: ");
		for (i=0; i<total; i++)
			fprintf(stderr, "<%s> ", tokens[i]);
		fprintf(stderr, "\n");
	}

	if (total <= 0)
		return 0;		// no tokens in line, break

	// token except the last token should be exactly matched in clitree
	for (i=0; i<total; i++) {
		struct list_head *child_list[2] = { &node->child_opt_list, &node->child_list };
		match = 0;
		for (j=0; j<2; j++) {
			list_for_each_entry(child, child_list[j], cli_node) {
				if (clitree_token_match(child, tokens[i])) {
					matched_child = child;
					match++;
				}
			}
		}
		if (clitree_debug) {
			fprintf(stderr, "token %s, match=%d\n", tokens[i], match);
		}
		if (match != 1) {
			return 0;		// no uniq found on all child, break
		}
		if (strchr(matched_child->keyword, '<')) {
			new_tokens[i] = tokens[i];
		} else {
			new_tokens[i] = matched_child->keyword;
		}
		if (!matched_child->is_optional)
			node = matched_child;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////

void
clitree_debug_enable(int enable)
{
	clitree_debug = enable;
}

int
clitree_load(struct cli_node_t *root, char *filename)
{
	static char buff[1024];
	char *tokens[64];
	int token_total, i;
	FILE *fp;
	
	if (root == NULL || filename == NULL)
		return -1;

	for (i=0; i<CLITREE_HASH_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&clitree_hash_table[i]);	
	INIT_LIST_HEAD(&root->child_opt_list);
	INIT_LIST_HEAD(&root->child_list);
	root->keyword = "ROOT";
	root->index = 0;
	root->help = root->validate = NULL;
	
	if ((fp = fopen(filename, "r")) == NULL) {
		//fprintf(stderr, "clitree load %s err?\n", filename);
		return -1;
	}
	while ( (fgets(buff, 1024, fp)) != NULL ) {	
		token_total = conv_str2array(buff, " ", tokens, 64);
		if (token_total <= 0)
			continue;
		if (tokens[0][0] == '#')
			continue;

		clitree_line_token_add(root, tokens, token_total);
	}
	fclose(fp);
	return 0;
}
		
static int
clitree_load_help_validate(struct cli_node_t *root, char *filename, int is_help)
{
	static char buff[1024], basebuff[1024];
	struct cli_node_t *base = root;
	FILE *fp;
	int is_newbase = 0;

	if (root == NULL || filename == NULL)
		return -1;
	
	if ((fp = fopen(filename, "r")) == NULL) {
		//fprintf(stderr, "clitree load validate %s err?\n", filename);
		return -1;
	}
	while ( (fgets(buff, 1024, fp)) != NULL) {
		if (buff[0] == '@') { // change base node
			strncpy(basebuff, buff, 1024);
			is_newbase = 1;
		} else {
			struct cli_node_t *node;
			char *s, *keyword, *desc;
			if ((s = strstr(buff, " - ")) != NULL) {
				*s = 0;
				keyword = util_trim(buff);
				desc = util_trim(s+3);	// desc means either help or validate
				if (strlen(desc) < 2)
					continue;

				// change to newbase if valid desc found for node under the newbase
				if (is_newbase) {
					struct cli_node_t *newbase;
					char *tokens[64];
					int token_total = conv_str2array(basebuff+1, " ", tokens, 64);
					if (token_total > 0) {
						newbase = clitree_line_token_find(root, tokens, token_total);
						if (newbase)
							base = newbase;
						if (clitree_debug>=2) {
							if (newbase) {
								fprintf(stderr, "base change to: %d:%s\n", base->index, base->keyword);
							} else {
								fprintf(stderr, "base unchanged: %d:%s\n", base->index, base->keyword);
							}
						}
					} else if (token_total == 0) {
						base = newbase = root;
						if (clitree_debug>=2)
							fprintf(stderr, "base change to: %d:%s\n", base->index, base->keyword);
					}
					is_newbase = 0;
				}

				// add desc to node
				node = clitree_child_add_help_validate(base, keyword, desc, is_help);
				if (clitree_debug>=2) {
					if (node) {
						fprintf(stderr, "add %s to %d:%s (%s)\n", is_help?"help":"validate", node->index, node->keyword, desc);
					} else {
						fprintf(stderr, "drop %s for keyword %s (%s)\n", is_help?"help":"validate", keyword, desc);
					}
				}
			}
		}
	}
	fclose(fp);
	return 0;
}
int
clitree_load_help(struct cli_node_t *root, char *filename)
{
	return clitree_load_help_validate(root, filename, 1);
}
int
clitree_load_validate(struct cli_node_t *root, char *filename)
{
	return clitree_load_help_validate(root, filename, 0);
}
		
int 
clitree_destroy(struct cli_node_t *root)
{
	if (root == NULL)
		return -1;
	clitree_child_del(root);
	return 0;
}

int
clitree_save_tree(struct cli_node_t *root, char *filename)
{
	char filename2[1024];
	FILE *fp;

	if (root == NULL || filename == NULL)
		return -1;

	snprintf(filename2, 1023, "%s.old", filename);
	rename(filename, filename2);		
		
	if ((fp = fopen(filename, "a+")) == NULL) {
		fprintf(stderr, "clitree save tree %s err?\n", filename);
		return -1;
	}
	clitree_child_save_tree(fp, root, 0);
	fclose(fp);
	return 0;
}

int
clitree_save_cli(struct cli_node_t *root, char *filename)
{
	static char filename2[1024];
	FILE *fp;

	if (root == NULL || filename == NULL)
		return -1;

	snprintf(filename2, 1023, "%s.old", filename);
	rename(filename, filename2);
		
	if ((fp = fopen(filename, "a+")) == NULL) {
		fprintf(stderr, "clitree save cli %s err?\n", filename);
		return -1;
	}	
	fprintf(fp, "%s", CLI_FILE_COMMENT);
	clitree_child_save_cli(fp, root, "");
	fclose(fp);
	
	return 0;
}

static int
clitree_save_help_validate(struct cli_node_t *root, char *filename, int is_help)
{
	static char filename2[1024];
	FILE *fp;

	if (root == NULL || filename == NULL)
		return -1;

	snprintf(filename2, 1023, "%s.old", filename);
	rename(filename, filename2);
		
	if ((fp = fopen(filename, "a+")) == NULL) {
		fprintf(stderr, "clitree save %s %s err?\n", is_help?"help":"validate", filename);
		return -1;
	}
	if (is_help)	
		fprintf(fp, "%s", HELP_FILE_COMMENT);
	else
		fprintf(fp, "%s", VALIDATE_FILE_COMMENT);

	clitree_child_save_help_validate(fp, root, "", is_help);

	fclose(fp);
	return 0;
}
int
clitree_save_help(struct cli_node_t *root, char *filename) 
{
	return clitree_save_help_validate(root, filename, 1);
}
int
clitree_save_validate(struct cli_node_t *root, char *filename) 
{
	return clitree_save_help_validate(root, filename, 0);
}

int
clitree_line_get_matchlist(struct cli_node_t *root, char *line, int with_help, char ***matchlist)
{
	static char buff[1024];
	char *tokens[64];
	int token_total, end_is_space;
	
	if (root == NULL || line == NULL)
		return -1;

	if (clitree_debug)
		fprintf(stderr, "\nclitree_line_get_matchlist(): line='%s'\n", line);
		
	end_is_space = (strlen(line)>0 && isspace(line[strlen(line)-1]));
	strncpy(buff, line, 1023);
	token_total = conv_str2array(buff, " ", tokens, 64);
	if (token_total == 0 || (end_is_space && token_total<63)) {
		tokens[token_total] = "";
		token_total++;
	}
	return clitree_line_token_match(root, tokens, token_total, with_help, matchlist);
}

// try to complete all token in a cmd line if there is a unique cmd match found
int 
clitree_line_get_replace(struct cli_node_t *root, char *line, char *line_result, int size)
{
	static char buff[1024];
	char *tokens[64], *new_tokens[64];
	int token_total;
	int found = 0, i;
	
	if (root == NULL || line == NULL || line_result == NULL)
		return -1;

	if (clitree_debug)
		fprintf(stderr, "\nclitree_line_get_replace(): line='%s'\n", line);

	line_result[0] = 0;		
	strncpy(buff, line, 1023);
	token_total = conv_str2array(buff, " ", tokens, 64);
	if (token_total == 0)
		return 0;	
	found = clitree_line_token_replace(root, tokens, new_tokens, token_total);
	if (found) {
		char *s = line_result;
		for (i=0; i<token_total; i++)
			s += snprintf(s, size - (s-line_result) -1, "%s%s", new_tokens[i], (i==token_total-1)?"":" ");
		return 1;
	}
	return 0;
}	

// generate complete str from match str returned in get_matchlist
// return in static, the caller has to do strdup for editline callback 
char *
clitree_get_complete_by_matchlist(char *token, char **matchlist, int match)
{
	static char match_str[128];
	char *s;
	int token_len = strlen(token);
	
	if (token_len <= 0 || match <= 0) {
		return NULL;

	} else if (match == 1) {	// single match, use asis
		snprintf(match_str, 127, "%s ", matchlist[0]);
		match_str[127] = 0;
	} else {	// multiple match, try to find out common part
		int i, common_len = 0, min_len = -1;
		for (i = 0; i< match; i++) {
			if (matchlist[i][0] != '<') {
				if (min_len == -1 || strlen(matchlist[i]) < min_len)
					min_len = strlen(matchlist[i]);
			}
			if (clitree_debug)
				fprintf(stderr, "clitree_get_complete_by_matchlist(): [%d/%d]%s, min_len=%d\n", i, match, matchlist[i], min_len);
		}
		// set min_len with matchlist[0] if min_len has not been set before
		if (min_len == -1) 
			min_len = strlen(matchlist[0]);
		if (clitree_debug)
			fprintf(stderr, "clitree_get_complete_by_matchlist(): min_len=%d\n", min_len);
		
		while (common_len < min_len+1) {	// 0..min_len, total min_len+1 byte, string part plus the \0
			int is_same = 1;
			for (i=0; i<match-1; i++)  {
				if (matchlist[i][0] != '<' && matchlist[match-1][common_len] != matchlist[i][common_len]) {
					is_same = 0;
					break;
				}
			}
			if (!is_same)
				break;
			common_len++ ;
		}
		strncpy(match_str, matchlist[match-1], 127);
		// if last same char in all keyword is \0, then we put ' ' instead of \0
		if (common_len >0 && match_str[common_len-1] == 0)
			match_str[common_len-1] = ' ';		
		match_str[common_len] = 0;
	}
	
	// matchlist is sorted by alphabet, so there is token <..>, it will appear in matchlist[0]
	// if matchlist[match-1] is <...> also, which means all tokens are <...>	
	
	// truncate <...> from  <...> or key=<...> in returned match_str
	if ((s=strchr(match_str, '<')) != NULL)		
		*s = 0;
	
	// find the token end position in the match string
	if (strlen(match_str) >= token_len && strncmp(match_str, token, token_len)==0) {
		return  match_str+strlen(token);
	} else { 
		// workaround editline bug! 
		// when token has '=', editline passed only the part after = to the auto-complete handler
		// so we compare the token with the string after the last occurance of '=' in the match_str
		char *equal_char_start = strrchr(match_str, '=');
		if (equal_char_start) {
			char *match_str2 = equal_char_start+1;
			if  (strlen(match_str2) >= token_len && strncmp(match_str2, token, token_len)==0)
				return  match_str2+strlen(token);
		}
	}
	return NULL;
}

///////////////////////////////////////////////
#ifdef CLITREE_UPDATE
int
main(int argc, char **argv)
{
	struct cli_node_t tree_root_g;
	char **av;
	int size, i;
	int found;
	char *complete_str;
	char result[1024];

	// clitree_update ../etc/traf/traf.cli ../etc/traf/traf.help
	if (argc < 3) {
		fprintf(stderr, "clitree_update [cli_file] [help_file...]\n");
		exit(1);
	}
	if (clitree_load(&tree_root_g, argv[1]) <0) {
		fprintf(stderr, "cli file '%s' not found?\n", argv[1]);
		exit(2);
	}
	for (i=2; i<argc; i++) {	
		if (clitree_load_help(&tree_root_g, argv[i]) <0) {
			fprintf(stderr, "help file '%s' not found?\n", argv[i]);
			exit(3);
		}
	}
	if (clitree_save_cli(&tree_root_g, argv[1]) <0) {
		fprintf(stderr, "cli file '%s' save err?\n", argv[1]);
		exit(4);
	}
	if (clitree_save_help(&tree_root_g, argv[2]) <0) {
		fprintf(stderr, "help file '%s' save err?\n", argv[2]);
		exit(5);
	}
	
	exit(0);
}
#endif

#ifdef CLITREE_TEST

int
main(int argc, char **argv)
{
	struct cli_node_t tree_root_g;
	char **av;
	int size, i;
	int found;
	char *complete_str;
	char result[1024];

	clitree_debug_enable(0);
	clitree_load(&tree_root_g, "clitree.cli");

	clitree_debug_enable(0);
	clitree_load_help(&tree_root_g, "clitree.help");

	clitree_debug_enable(0);
	clitree_save_tree(&tree_root_g, "clitree.tree.save");
	clitree_save_cli(&tree_root_g, "clitree.cli.save");
	clitree_save_help(&tree_root_g, "clitree.help.save");

	while (1) {
		static char buff[1024];
		char *s;

		printf("\n-----------------\ninput: ");
		s = fgets(buff, 1023, stdin);
		printf("-----------------\n");
		if (buff[strlen(buff)-1] == '\n')
			buff[strlen(buff)-1] = 0;

		clitree_debug_enable(0);
		size = clitree_line_get_matchlist(&tree_root_g, buff, 0, &av);
		{
			// get last token in input line 
			static char buff2[1024];
			char *tokens[64];
			int token_total;
			strncpy(buff2, buff, 1023);		
			token_total = conv_str2array(buff2, " ", tokens, 64);		
			if (token_total ==0) {
				tokens[token_total] = "";
				token_total++;
			}
			// get complete str 
			clitree_debug_enable(2);
			complete_str = clitree_get_complete_by_matchlist(tokens[token_total-1], av, size);
		}

		printf("matchlist size: %d\n", size);
		if (size >0) {
			for (i=0; i<size; i++) {
				if (av[i] != NULL) {
					printf("%d: %s\n", i, av[i]);
					free(av[i]);
				}
			}
			free(av);
		}
		if (complete_str) {
			printf("\ncomplete_str: '%s'\n",  complete_str);
		} else {
			printf("\ncomplete_str: NULL\n");
		}		

		clitree_debug_enable(0);
		size = clitree_line_get_matchlist(&tree_root_g, buff, 1, &av);
		printf("\nhelp matchlist size: %d\n", size);
		if (size >0) {
			for (i=0; i<size; i++) {
				if (av[i] != NULL) {
					printf("%d: %s\n", i, av[i]);
					free(av[i]);
				}
			}
			free(av);
		}
		
		clitree_debug_enable(0);
		found = clitree_line_get_replace(&tree_root_g, buff, result, 1024);
		printf("\nreplace_found=%d\n",  found);
		if (found) {
			printf("replace: %s\n", result);
		}
		
	}	
	return 0;
}

#endif
