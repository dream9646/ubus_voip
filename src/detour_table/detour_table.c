
/*
Operation mode identification symbol, (S,PS,P,V,N)
detour number, (ex:113)
detour judging digit, (ex:0,4,3)
setting time (second).(ex:185)

PS,110,3,185
P,0501817,11
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include "detour_table.h"

/* util_trim : removes leading and trailing spaces of a string */
static inline char * strtrim(char *s)
{
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

char * util_trim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	return strtrim(s);
}
int verify_identify_symbol(char *symbol_str)
{
	if(symbol_str == NULL)
		return SYMBOL_ERR;

	if(strcmp(symbol_str, "PS")==0)
		return SYMBOL_PS;
	else if(strcmp(symbol_str, "P")==0)
		return SYMBOL_P;
	else if(strcmp(symbol_str, "S")==0)
		return SYMBOL_S;
	else if(strcmp(symbol_str, "N")==0)
		return SYMBOL_N;
	else if(strcmp(symbol_str, "V")==0)
		return SYMBOL_V;
	else if(strcmp(symbol_str, "$")==0)
		return SYMBOL_EOF;
	else
		return SYMBOL_ERR;
}

char *dialing_plan_action_str(int symbol)
{
	switch(symbol) {
	case SYMBOL_PS:
		return "pstn_dial_timer";
		break;
	case SYMBOL_P:
		return "pstn_dial";
		break;
	case SYMBOL_S:
		return "force_pstn";
		break;
	case SYMBOL_N:
		return "reject";
		break;
	case SYMBOL_V:
		return "voip_dial";
		break;
	case SYMBOL_EOF:
		return NULL;
		break;
	}
	return NULL;
}

int add_entry_to_list(int symbol, char *dialplan_str)
{
	struct detour_plan_mapping *mapping_entry;

	if(dialplan_str == NULL)
		return -1;

	mapping_entry=malloc(sizeof(struct detour_plan_mapping));
	if(mapping_entry) {
		memset(mapping_entry, 0, sizeof(mapping_entry));
		mapping_entry->symbol=symbol;
		snprintf(mapping_entry->detour2dialplan_str, DATA_LEN, "%s", dialplan_str);
		list_add_tail(&mapping_entry->plan_list_ptr, &plan_list_head);
		return 0;
	}

	return -1;
}

int write_detour_plan_list_to_file(FILE *fptr)
{
	struct list_head *list, *n;

	if(fptr == NULL)
		return -1;

	//S>N>V>P&PS
	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		if( mapping_entry->symbol == SYMBOL_S)
			fprintf(fptr, "%s", mapping_entry->detour2dialplan_str);
	}

	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		if( mapping_entry->symbol == SYMBOL_N)
			fprintf(fptr, "%s", mapping_entry->detour2dialplan_str);
	}

	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		if( mapping_entry->symbol == SYMBOL_V)
			fprintf(fptr, "%s", mapping_entry->detour2dialplan_str);
	}

	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		if( mapping_entry->symbol == SYMBOL_P)
			fprintf(fptr, "%s", mapping_entry->detour2dialplan_str);
	}

	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		if( mapping_entry->symbol == SYMBOL_PS)
			fprintf(fptr, "%s", mapping_entry->detour2dialplan_str);
	}

	return 0;
}

int release_detour_plan_list()
{
	struct list_head *list, *n;

	list_for_each_safe(list, n, &plan_list_head) {
		struct detour_plan_mapping *mapping_entry=list_entry(list, struct detour_plan_mapping, plan_list_ptr);
		list_del(&mapping_entry->plan_list_ptr);
		free(mapping_entry);
	}
	return 0;
}

void usage()
{
	printf("Usage: detour_table [-d debug_level] [-f|o|p filename]\n");
	printf("List directory contents\n");
	printf("\t -d\tdebug_level 0:disable 1: error only 2:debug; default:1\n");
	printf("\t -f\tinput detour table file name\n");
	printf("\t -o\toutput valid detour table file name\n");
	printf("\t -p\toutput extension dialing plan file name\n");
	printf("\t%s", "-------state code--------\n");
	printf("\t%s", "STATE_OK	0\n");
	printf("\t%s", "STATE_FORMAT_ERR        10\n");
	printf("\t%s", "STATE_FORCIBLY_ERR      11\n");
	printf("\t%s", "STATE_ROW_NUM_ERR       12\n");
	printf("\t%s", "STATE_SYMBOL_ERR        13\n");
	printf("\t%s", "STATE_OPEN_FILE_ERR     14\n");
	printf("\t%s", "STATE_DIGITS_LEN_ERR    15\n");
	printf("\t%s", "STATE_ERR               20\n");
}

int main(int argc, char **argv)
{
	FILE	*input_fptr=NULL, *output_detour_fptr=NULL, *output_dialing_plan_fptr=NULL;
	char 	tempbuf[DATA_LEN], *token, input_detour_name[FILENAME_LEN+1], output_detour_name[FILENAME_LEN+1], output_dialing_plan_name[FILENAME_LEN+1];
	char 	identify_symbol_str[TOKEN_LEN+1], detour_number_str[TOKEN_LEN+1], total_digit_str[TOKEN_LEN+1], keep_time_str[TOKEN_LEN+1], tmp_detour_number_str[TOKEN_LEN+1];
	char	tmp_dialplan_str[DATA_LEN+1];
	int	identify_symbol, total_digit=0;
	int	state, is_create, current_id, last_current_id, line_num_in_table=0, expect_line_num_in_table=0;
	int 	field_pos, state_sym_s_exist=0, state_sym_s_err=0, opt, genator_valid_file=0;
	struct detour_table_entry forcibly_detour_entry;

	//todo provide a struct to keep symbol_s
	//
	memset(input_detour_name, 0, sizeof(input_detour_name));
	memset(output_detour_name, 0, sizeof(output_detour_name));
	memset(output_dialing_plan_name, 0, sizeof(output_dialing_plan_name));

	while ((opt = getopt(argc, argv, "d:f:o:p:h")) != -1) {
		switch (opt) {
		case 'd':
			if(optarg != NULL)
				debug_level=atoi(optarg);
			else
				debug_level=1;
			break;
		case 'f':
			if(optarg == NULL) {
				usage();
				exit(0);
			} else {
				strncpy(input_detour_name, optarg, FILENAME_LEN);
			}
			break;
		case 'o':
			if(optarg != NULL) {
				strncpy(output_detour_name, optarg, FILENAME_LEN);
				genator_valid_file=1;
			}
			break;
		case 'p':
			if(optarg != NULL) {
				strncpy(output_dialing_plan_name, optarg, FILENAME_LEN);
			}
			break;
		case 'h':
			usage();
			exit(0);
			break;
		default:
			printf("Error parameter !\n\n");
			usage();
			exit(0);
			break;
		}
	}

	//input error check
	if(input_detour_name[0]==0) {
		usage();
		exit(0);
	}

	//auto assign file name
	if(output_dialing_plan_name[0]==0)
		snprintf(output_dialing_plan_name, FILENAME_LEN, "%s.plan", input_detour_name);

	if ((input_fptr=fopen(input_detour_name,"r")) == NULL) {	/*file want to parser*/
		dbprintf(1, "can't open input detour file %s !!!\n", input_detour_name);
		usage();
		return STATE_OPEN_FILE_ERR;
	}

	//prepare output file
	if (genator_valid_file==1 && (output_detour_fptr=fopen(output_detour_name,"w+")) == NULL) {	/*file valid*/
		dbprintf(1, "can't open output valid detour file %s !!!\n", output_detour_name);
		usage();
		fclose(input_fptr);
		return STATE_OPEN_FILE_ERR;
	}

	if ((output_dialing_plan_fptr=fopen(output_dialing_plan_name,"w+")) == NULL) {	/*file for dialing plan*/
		dbprintf(1, "can't open dialing plan file %s !!!\n", output_dialing_plan_name);
		usage();
		fclose(input_fptr);
		if(genator_valid_file==1)
			fclose(output_detour_fptr);
		return STATE_OPEN_FILE_ERR;
	}

	state=STATE_OK;
	last_current_id=1;
	current_id=1;
	is_create=0;
	INIT_LIST_HEAD(&plan_list_head);

	//only one valid S
	memset(&forcibly_detour_entry, 0, sizeof(forcibly_detour_entry));

	while( fgets(tempbuf, DATA_LEN, input_fptr) != NULL ) {
		//skip empty line
		if (tempbuf[0] == '\0' || tempbuf[0] == '\n' || tempbuf[0] == '\r')
			continue;

		if (tempbuf[strlen(tempbuf)-1] == '\n')
			tempbuf[strlen(tempbuf)-1] = '\0';

		line_num_in_table++;
		//skip the first 2 line
		if( line_num_in_table==1 || line_num_in_table==2 ) {
			//005.01.1.DT001
			//#186
			if(tempbuf[0] == '#') {
				token=tempbuf+1;
				expect_line_num_in_table=atoi(token);
				dbprintf(2, "%03d: %s, %d\n", line_num_in_table, util_trim(token), expect_line_num_in_table);
				if(genator_valid_file==1)
					fprintf(output_detour_fptr, "%s\n",util_trim(tempbuf));
			} else {
				dbprintf(2,"%03d: %s\n", line_num_in_table, util_trim(tempbuf));
				if(genator_valid_file==1)
					fprintf(output_detour_fptr, "%s\n",util_trim(tempbuf));
			}
			continue;
		}
		//printf("%d\n",tempbuf[0]);

		token=strtok(tempbuf, ",");
		field_pos=0;
		identify_symbol=SYMBOL_ERR;

		memset(identify_symbol_str, 0, sizeof(identify_symbol_str));
		memset(detour_number_str, 0, sizeof(detour_number_str));
		memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
		memset(total_digit_str, 0, sizeof(total_digit_str));
		memset(keep_time_str, 0, sizeof(keep_time_str));

		while(token)
		{
			switch(field_pos) {
			case 0:
				strncpy(identify_symbol_str, util_trim(token), TOKEN_LEN);
				if((identify_symbol=verify_identify_symbol(identify_symbol_str)) == SYMBOL_ERR) {
					dbprintf(1, "symbol err in line %03d [%s]\n",line_num_in_table, tempbuf);
					fclose(input_fptr);
					if(genator_valid_file==1)
						fclose(output_detour_fptr);
					fclose(output_dialing_plan_fptr);
					release_detour_plan_list();
					return STATE_SYMBOL_ERR;
				}

				//exception handling for S and $
				if(identify_symbol==SYMBOL_S) {
					if (state_sym_s_exist == 0) {
						//keep string if valid
						forcibly_detour_entry.identify_symbol=SYMBOL_S;
						state_sym_s_exist=1;
					} else {
						state_sym_s_err=1;	//duplicate SYMBOL_S, only issue warning, other functions continue to execute
						dbprintf(1, "Warning Forcibly detour(duplicate)\n");
						state=STATE_FORCIBLY_ERR;
					}
				} else if (identify_symbol== SYMBOL_EOF) {
					//complete detour table if we have valid SYMBOL_S
					if (state_sym_s_exist == 1 && state_sym_s_err==0) {
						//attach valid S symbol
						if(genator_valid_file==1)
							fprintf(output_detour_fptr, "%s,%s,%d\n",dialing_plan_action_str(SYMBOL_S), forcibly_detour_entry.detour_number_str, forcibly_detour_entry.total_digit);
						if(forcibly_detour_entry.total_digit==0) {
							//add x.
							memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
							sprintf(tmp_detour_number_str, "%sx.", forcibly_detour_entry.detour_number_str);
						} else {
							memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
							memset(tmp_detour_number_str, 'x', forcibly_detour_entry.total_digit);
							memcpy(tmp_detour_number_str, forcibly_detour_entry.detour_number_str, strlen(forcibly_detour_entry.detour_number_str));
						}
						dbprintf(2, "%s@%s\n",tmp_detour_number_str, dialing_plan_action_str(SYMBOL_S));
						snprintf(tmp_dialplan_str, DATA_LEN, "%s@%s\n", tmp_detour_number_str, dialing_plan_action_str(SYMBOL_S));
						add_entry_to_list(SYMBOL_S, tmp_dialplan_str);
					} else {
						//use default value
						dbprintf(2, "%s@%s\n",FORCE_PSTN_DEFAULT, dialing_plan_action_str(SYMBOL_S));
						snprintf(tmp_dialplan_str, DATA_LEN, "%s@%s\n", FORCE_PSTN_DEFAULT, dialing_plan_action_str(SYMBOL_S));
						add_entry_to_list(SYMBOL_S, tmp_dialplan_str);
					}

					if(genator_valid_file==1)
						fprintf(output_detour_fptr, "$\n");
				}
				break;

			case 1:
				strncpy(detour_number_str, util_trim(token), TOKEN_LEN);
				if(identify_symbol==SYMBOL_S && state_sym_s_err == 0) {
					strncpy(forcibly_detour_entry.detour_number_str, detour_number_str,TOKEN_LEN);
				}
				break;

			case 2:
				strncpy(total_digit_str, util_trim(token), TOKEN_LEN);
				total_digit=atoi(total_digit_str);
				if(identify_symbol==SYMBOL_S && state_sym_s_err == 0) {
					forcibly_detour_entry.total_digit=total_digit;
				}
				break;

			case 3:
				if(identify_symbol==SYMBOL_PS) {
					strncpy(keep_time_str, util_trim(token), TOKEN_LEN);
				} else {
					strncpy(keep_time_str, util_trim(token), TOKEN_LEN);
					dbprintf(1, "Error field_pos=3, token=[%s]\n", util_trim(token));
					state=STATE_SYMBOL_ERR;
				}
				break;
			default:
				dbprintf(1, "Error token %s\n", util_trim(token));
				break;
			}
			field_pos++;

			//printf("[%d][%s]\n", field_pos, util_trim(token));
			token=strtok(NULL , ",");
		}

		if(total_digit!=0 && total_digit < strlen(detour_number_str)) {
			dbprintf(2, "Error digit len [%03d] [%s]\t[%s]\t[%s]\n",line_num_in_table, identify_symbol_str, detour_number_str, total_digit_str);
			fclose(input_fptr);
			if(genator_valid_file==1)
				fclose(output_detour_fptr);
			fclose(output_dialing_plan_fptr);
			release_detour_plan_list();
			return STATE_DIGITS_LEN_ERR;
		}

		if(field_pos==4) {	//for format PS,110,3,185
			dbprintf(2, "[%03d] [%s]\t[%s]\t[%s]\t[%s]\n",line_num_in_table, identify_symbol_str, detour_number_str, total_digit_str, keep_time_str);
			if(genator_valid_file==1)
				fprintf(output_detour_fptr, "%s,%s,%s,%s\n",identify_symbol_str, detour_number_str, total_digit_str, keep_time_str);
			if(total_digit==0) {
				//add x.
				memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
				sprintf(tmp_detour_number_str, "%sx.", detour_number_str);
			} else {
				memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
				memset(tmp_detour_number_str, 'x', total_digit);
				memcpy(tmp_detour_number_str, detour_number_str, strlen(detour_number_str));
			}
			dbprintf(2, "%s@%s,%s\n",tmp_detour_number_str, dialing_plan_action_str(identify_symbol), keep_time_str);
			snprintf(tmp_dialplan_str, DATA_LEN, "%s@%s,%s\n",tmp_detour_number_str, dialing_plan_action_str(identify_symbol), keep_time_str);
			add_entry_to_list(identify_symbol, tmp_dialplan_str);

		} else if(field_pos==3) {	//for format P,0501814,11; V,0120919860,10
			if (identify_symbol != SYMBOL_S) {
				dbprintf(2, "[%03d] [%s]\t[%s]\t[%s]\n",line_num_in_table, identify_symbol_str, detour_number_str, total_digit_str);
				if(genator_valid_file==1)
					fprintf(output_detour_fptr, "%s,%s,%s\n",identify_symbol_str, detour_number_str, total_digit_str);

				if(total_digit==0) {
					if (identify_symbol == SYMBOL_P) {
						//add px.
						memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
						sprintf(tmp_detour_number_str, "%spx.", detour_number_str);
					} else {
						//add x.
						memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
						sprintf(tmp_detour_number_str, "%sx.", detour_number_str);
					}
				} else {
					if (identify_symbol == SYMBOL_P) {
						memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
						memset(tmp_detour_number_str, 'x', total_digit+1);
						memcpy(tmp_detour_number_str, detour_number_str, strlen(detour_number_str));
						memcpy(tmp_detour_number_str+strlen(detour_number_str), "p", 1);
					} else {
						memset(tmp_detour_number_str, 0, sizeof(tmp_detour_number_str));
						memset(tmp_detour_number_str, 'x', total_digit);
						memcpy(tmp_detour_number_str, detour_number_str, strlen(detour_number_str));
					}
				}
				if (identify_symbol == SYMBOL_P) {
					dbprintf(2, "%s\n",tmp_detour_number_str);
					snprintf(tmp_dialplan_str, DATA_LEN, "%s\n",tmp_detour_number_str);
				} else {
					dbprintf(2, "%s@%s\n",tmp_detour_number_str, dialing_plan_action_str(identify_symbol));
					snprintf(tmp_dialplan_str, DATA_LEN, "%s@%s\n",tmp_detour_number_str, dialing_plan_action_str(identify_symbol));
				}
				add_entry_to_list(identify_symbol, tmp_dialplan_str);
			}
		} else {
			//in this state only SYMBOL_EOF is ok, else is error
			if (identify_symbol != SYMBOL_EOF) {
				dbprintf(1, "Error on item [%03d]\n",line_num_in_table);
				fclose(input_fptr);
				if(genator_valid_file==1)
					fclose(output_detour_fptr);
				fclose(output_dialing_plan_fptr);
				release_detour_plan_list();
				return STATE_FORMAT_ERR;
			}
		}
	}

	fclose(input_fptr);
	if(genator_valid_file==1)
		fclose(output_detour_fptr);

	write_detour_plan_list_to_file(output_dialing_plan_fptr);
	fclose(output_dialing_plan_fptr);
	release_detour_plan_list();

	if((line_num_in_table-NUM_OF_TABLE_BUT_NOT_SYMBOL) == expect_line_num_in_table)
		dbprintf(2, "%02d == %02d\n",line_num_in_table-NUM_OF_TABLE_BUT_NOT_SYMBOL, expect_line_num_in_table);
	else {
		dbprintf(1, "Error expect_line_num_in_table: %02d != %02d\n",line_num_in_table-NUM_OF_TABLE_BUT_NOT_SYMBOL, expect_line_num_in_table);
		return STATE_ROW_NUM_ERR;
	}

	return state;
}

