#ifndef __DETOUR_TABLE_H__
#define __DETOUR_TABLE_H__

#include "list.h"

#define	STATE_OK		0
#define	STATE_FORMAT_ERR	10
#define	STATE_FORCIBLY_ERR	11
#define	STATE_ROW_NUM_ERR	12
#define	STATE_SYMBOL_ERR	13
#define	STATE_OPEN_FILE_ERR	14
#define	STATE_DIGITS_LEN_ERR	15
#define	STATE_ERR		20


#define	SYMBOL_PS		30
#define	SYMBOL_P		31
#define	SYMBOL_S		32
#define	SYMBOL_N		33
#define	SYMBOL_V		34
#define	SYMBOL_EOF		35
#define	SYMBOL_ERR		36

#define	TOKEN_LEN		33
#define	DATA_LEN		256
#define	FILENAME_LEN		1024

#define FORCE_PSTN_DEFAULT	"0000"

/*
005.01.1.DT001
#186
P,#,5
*snip*
$

==> #186 not includ "File version", "#186" and "$"
*/

#define NUM_OF_TABLE_BUT_NOT_SYMBOL	3

int debug_level;
struct list_head plan_list_head;

#define dbprintf(level, fmt, args...) \
	do { if (debug_level>=(level)) printf(fmt, ##args); } while (0)

struct detour_table_entry {
	int	identify_symbol;
	char 	detour_number_str[TOKEN_LEN+1];
	int 	total_digit;
	int	keep_time;
};

struct detour_plan_mapping
{
        int symbol;
        char detour2dialplan_str[DATA_LEN+1];
        struct list_head plan_list_ptr;
};

#endif
