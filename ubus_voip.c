#include <libubox/uloop.h>
#include <libubus.h>
#include <libubox/ulog.h>
#include <uci.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdint.h>
// prototype
struct ubus_voip_cmd_list ubus_voip_cmd_list(struct ubus_voip_cmd_list ubus_voip_cmdlist);
int ubus_voip_set_fv_voip(const char *key, const char *value);
int ubus_voip_delete_sub_str(const char *str, const char *sub_str, char *result_str);
char *ubus_voip_fv_voip_type(char *str);
int ubus_voip_fv_voip_process_line(FILE *fp, char *line, const char *input_section, const char *input_key, const char *input_value);
char *ubus_voip_cmd_list_set(char *section, char *key, char *value);
int ubus_voip_uci_commit_package(char *input_package);
int ubus_voip_uci_revert_package(char *input_package);
void ubus_voip_uci_changes();
//  log
#define UBUS_VOIP_LOG_FILE "/var/log/ubus_voip.log"
#define SUCCESS 1
#define FAIL -1

uint64_t cfg_logmask = LOG_EMERG;

// definition 0..7 are borrowed from /usr/include/syslog.h
#define LOG_EMERG 0			/* system is unusable */
#define LOG_ALERT 1			/* action must be taken immediately */
#define LOG_CRIT 2			/* critical conditions */
#define LOG_ERR 3			/* error conditions */
#define LOG_WARNING 4		/* warning conditions */
#define LOG_NOTICE 5		/* normal but significant condition */
#define LOG_INFO 6			/* informational */
#define LOG_DEBUG 7			/* debug-level messages */
#define LOG_VERBOSE 8		/* verbose debug-level messages */
#define LOG_ALWAYS (1 << 8) /* override whether to print a message determined by loglevel */
#define LOG_UNDEFINED -1	/* reserved for per lm->*level threshold, which means inherit level from logm_global */

#define LOG_GLOBAL_RESET_SIZE (100 * 1024)

static int
util_logprintf(char *logfile, uint64_t loglevel, char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	int ret;
	time_t t;
	struct tm *tm_info;
	struct timeval tv;
	char timestamp[26];
	// Check if the log file size exceeds the limit
	struct stat st;
	if (stat(logfile, &st) == 0 && st.st_size >= LOG_GLOBAL_RESET_SIZE)
	{
		// Open the file in write mode to clear its contents
		if ((fp = fopen(logfile, "w")) == NULL)
			return (-1);

		fclose(fp);
	}
	if ((fp = fopen(logfile, "a")) == NULL)
		return (-1);
	// Get the current time
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	tm_info = localtime(&t);
	strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
	int milliseconds = (int)(tv.tv_usec / 1000);
	if (milliseconds < 0)
		milliseconds = 0;
	if (milliseconds > 999)
		milliseconds = 999;
	sprintf(timestamp + 19, ".%03d", milliseconds); // Append milliseconds
	// Add timestamp, cfg_logmask, and UBUS_VOIP to the log message
	fprintf(fp, "[%s][%llu][UBUS_VOIP] ", timestamp, loglevel);
	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);
	if (fclose(fp) == EOF)
		return (-2); // flush error
	return ret;
}
// log
#define RPC_EXEC_DEFAULT_TIMEOUT (120 * 1000)
struct blob_buf b;
int rpc_exec_timeout = RPC_EXEC_DEFAULT_TIMEOUT;
#define BUF_SIZE_1 1024
static struct ubus_context *ctx;
struct ubus_voip_cmd_list
{
	struct list_head list;
	char key[BUF_SIZE_1];
	char action[BUF_SIZE_1];
	char fvcli_key[BUF_SIZE_1];
};
struct ubus_voip_changes_list
{
	struct list_head list;
	char key[BUF_SIZE_1];
	char value[BUF_SIZE_1];
	char ret[BUF_SIZE_1];
};
struct ubus_voip_cmd_list ubus_voip_cmd_list(struct ubus_voip_cmd_list ubus_voip_cmdlist)
{
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to retrieve cmd_list\n");
	}
	FILE *fp = fopen("/etc/voip/cmd_list", "r");
	if (fp == NULL)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "There was an error while attempting to open cmd_list\n");
		}
		return ubus_voip_cmdlist;
	}
	char line[BUF_SIZE_1];
	char key[BUF_SIZE_1];
	char action[BUF_SIZE_1];
	char fvcli_key[BUF_SIZE_1];
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		if (line[0] == '#')
		{
			continue;
		}
		sscanf(line, "%s %s %s", action, key, fvcli_key);
		struct ubus_voip_cmd_list *tmp = (struct ubus_voip_cmd_list *)malloc(sizeof(struct ubus_voip_cmd_list));
		// It will be freed at the end of ubus_voip_cmd_list_set.
		strcpy(tmp->action, action);
		strcpy(tmp->key, key);
		strcpy(tmp->fvcli_key, fvcli_key);
		list_add(&(tmp->list), &(ubus_voip_cmdlist.list));
	}
	fclose(fp);
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished retrieving cmd_list\n");
	}
	return ubus_voip_cmdlist;
}

int ubus_voip_fv_voip_process_line(FILE *fp, char *line, const char *input_section, const char *input_key, const char *input_value)
{
	static char fv_section[BUF_SIZE_1];
	static char fv_key[BUF_SIZE_1];
	static char new_line[BUF_SIZE_1 * 4];
	static char new_line_add_list[BUF_SIZE_1 * 3];

	char line_key[BUF_SIZE_1];
	char line_value[BUF_SIZE_1];

	sscanf(line, "%s = %s", line_key, line_value);

	if (line_key[0] == '[' &&
		line_key[strlen(line_key) - 1] == ']' &&
		strlen(line_key) > 3)
	{
		sprintf(fv_section, "%s", line_key);
	}

	if (strstr(line, "="))
	{
		sprintf(fv_key, "%s", line_key);
	}

	if (strncmp(line_key, input_key, strlen(input_key)) == 0 && strcmp(fv_section, input_section) == 0)
	{
		sprintf(new_line, "%s = %s\r\n", input_key, input_value);
		fputs(new_line, fp);
	}
	else if (strstr(input_key, "+") &&
			 strncmp(line_key, input_key, strlen(line_key)) == 0 &&
			 strcmp(fv_section, input_section) == 0)
	{
		int is_add = FAIL;
		if (strstr(line, "{"))
		{
			is_add = SUCCESS;
		}
		while (is_add == SUCCESS)
		{
			if (strstr(line, "}") &&
				strstr(input_key, fv_key))
			{
				is_add = FAIL;
				strcpy(new_line_add_list, new_line);
				snprintf(new_line, sizeof(new_line), "        %s\r\n", new_line_add_list);
				// Make sure the new line is added to the end of the list
				fputs(new_line, fp);
				fputs(line, fp);
			}
			else
			{
				fputs(line, fp);
			}
		}
	}
	else if (strstr(input_key, "-") &&
			 strncmp(line_key, input_key, strlen(line_key)) == 0 &&
			 strcmp(fv_section, input_section) == 0)
	{
		int is_del = FAIL;
		if (strstr(line, "{"))
		{
			is_del = SUCCESS;
		}
		while (is_del == SUCCESS)
		{
			sscanf(line, "%s = %s", line_key, line_value);
			if (strstr(line, "="))
			{
				sprintf(fv_key, "%s", line_key);
			}
			if (strstr(line, new_line) &&
				strstr(input_key, fv_key))
			{
				// Match, do not write
				is_del = FAIL;
			}
			else
			{
				fputs(line, fp);
			}
		}
	}
	else
	{
		// Does not match, write the original line
		fputs(line, fp);
	}

	return SUCCESS;
}

int ubus_voip_set_fv_voip(const char *key, const char *value)
{
	char input_section[BUF_SIZE_1];
	char input_key[BUF_SIZE_1];
	char input_value[BUF_SIZE_1];
	strcpy(input_value, value);

	sscanf(key, "fvt_evoip.%[^.].%s", input_section, input_key);

	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to set fv_voip_uci.config\n");
	}

	FILE *fp = fopen("/etc/voip/fv_voip_uci.config", "rb+");
	if (fp == NULL)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "There was an error while attempting to open fv_voip_uci.config\n");
		}
		return FAIL;
	}

	char line[BUF_SIZE_1 * 2];
	while (fgets(line, sizeof(line), fp) > 0)
	{
		int result = ubus_voip_fv_voip_process_line(fp, line, input_section, input_key, input_value);
		if (result == FAIL)
		{
			break;
		}
	}

	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished setting fv_voip_uci.config\n");
	}

	fflush(fp);
	fclose(fp);
	return SUCCESS;
}

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

	if (str_len < sub_str_len ||
		sub_str == NULL)
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
		while (*str != *sub_str &&
			   *str != '\0')
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

char *ubus_voip_fv_voip_type(char *str)
{
	char res_list_val[BUF_SIZE_1 * 2] = {0};
	char *p;
	char a[BUF_SIZE_1] = {0};
	char b[BUF_SIZE_1] = {0};
	char c[BUF_SIZE_1] = {0};
	int newline_count = 0;
	int count = 0;

	for (int i = 0; i < strlen(str); i++)
	{
		if (str[i] == '\n')
			newline_count++;
		if (str[i] == ' ')
			count++;
	}

	p = strtok(str, "={");
	ubus_voip_delete_sub_str(p, "}", p);
	ubus_voip_delete_sub_str(p, "{", p);

	if (newline_count < 3)
	{
		strcpy(res_list_val, p);
	}
	else
	{
		p = strtok(NULL, "\n");
		p = strtok(NULL, "\n");

		while (p != NULL)
		{
			if (strstr(p, " ") != NULL && (count == 5 || count == 6))
			{
				sscanf(p, " %s %s", a, b);

				if (a[0] == '[' && a[strlen(a) - 1] == ']')
				{
					ubus_voip_delete_sub_str(a, "[", a);
					ubus_voip_delete_sub_str(a, "]", a);
				}

				if (count == 6)
				{
					// Deal one line have three value count6=3+3
					sscanf(p, " %s %s %s", a, b, c);
					snprintf(p, BUF_SIZE_1 * 4, "%s.%s.%s ", a, b, c);
				}
				else
				{
					// Deal one line have two value count5=3+2
					snprintf(p, BUF_SIZE_1 * 3, "%s.%s ", a, b);
				}

				strcat(res_list_val, p);
				res_list_val[strlen(res_list_val) - 1] = '\0';
				strcat(res_list_val, " ");
			}
			else
			{
				ubus_voip_delete_sub_str(p, "}", p);
				strcat(res_list_val, p);
				res_list_val[strlen(res_list_val) - 1] = '\0';
			}
			p = strtok(NULL, "\n");
		}
	}
	return strdup(res_list_val);
}

char *ubus_voip_cmd_list_set(char *section, char *key, char *value)
{
	char *results = (char *)malloc(BUF_SIZE_1 * 4 * sizeof(char));
	memset(results, 0, sizeof(char) * BUF_SIZE_1 * 4);
	struct ubus_voip_cmd_list *tmp_cmd, *tmp_cmd_next;
	struct ubus_voip_cmd_list cmdlist;
	INIT_LIST_HEAD(&cmdlist.list);
	char ep[2] = "";

	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to execute fvcli using cmd_list\n");
	}
	// Deal EP0 and EP1
	if (strcmp(section, "EP0") == 0)
	{
		strcpy(ep, "0");
	}
	else if (strcmp(section, "EP1") == 0)
	{
		strcpy(ep, "1");
	}
	cmdlist = ubus_voip_cmd_list(cmdlist);
	list_for_each_entry(tmp_cmd, &cmdlist.list, list)
	{
		// cmdlist fvcli_set start
		FILE *fp_cmd;
		char cmd[BUF_SIZE_1 * 4];
		char *line = (char *)malloc(BUF_SIZE_1 * 4 * sizeof(char));
		char buf[1];
		if (strcmp(key, tmp_cmd->key) == 0 &&
			strcmp(tmp_cmd->action, "fvcli_set") == 0)
		{
			sprintf(cmd, "fvcli set %s %s\n", tmp_cmd->fvcli_key, value);
			if (cfg_logmask >= LOG_INFO)
			{
				ubus_voip_delete_sub_str(cmd, "\n", cmd);
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "Execute [%s]\n", cmd);
				strcat(cmd, "\n");
			}
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd)
			{
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					memset(buf, 0, sizeof(buf));
					if (isdigit(line[0]))
					{
						if (cfg_logmask >= LOG_INFO)
						{
							ubus_voip_delete_sub_str(cmd, "\n", cmd);
							util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "[%s] reply : [%c]\n", cmd, line[0]);
							strcat(cmd, "\n");
						}
					}
					buf[0] = line[0];
					strcat(results, buf);
				}
			}
			else
			{
				if (cfg_logmask >= LOG_ERR)
				{
					util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing [%s]\n", cmd);
				}
			}
			pclose(fp_cmd);
		}
		// cmdlist fvcli_set end
		// cmdlist fvcli_set_ep start
		if (strcmp(key, tmp_cmd->key) == 0 &&
			strcmp(tmp_cmd->action, "fvcli_set_ep") == 0)
		{
			sprintf(cmd, "fvcli set %s %s %s\n", tmp_cmd->fvcli_key, ep, value);
			if (cfg_logmask >= LOG_INFO)
			{
				ubus_voip_delete_sub_str(cmd, "\n", cmd);
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "Execute [%s]\n", cmd);
				strcat(cmd, "\n");
			}
			fp_cmd = popen(cmd, "r");
			if (NULL != fp_cmd)
			{
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					memset(buf, 0, sizeof(buf));
					if (isdigit(line[0]))
					{
						if (cfg_logmask >= LOG_INFO)
						{
							ubus_voip_delete_sub_str(cmd, "\n", cmd);
							util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "[%s] reply : [%c]\n", cmd, line[0]);
							strcat(cmd, "\n");
						}
					}
					buf[0] = line[0];
					strcat(results, buf);
				}
			}
			else
			{
				if (cfg_logmask >= LOG_ERR)
				{
					util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing [%s]\n", cmd);
				}
			}
			pclose(fp_cmd);
		}
		// cmdlist fvcli_set_ep end
		// cmdlist script start
		if (strcmp(key, tmp_cmd->key) == 0 &&
			strcmp(tmp_cmd->action, "script") == 0)
		{
			// Use popen to execute the shell script and pass the parameter to it
			FILE *fp_sh;
			char buffer_sh[BUF_SIZE_1];
			char sh_tmp[BUF_SIZE_1 * 2];
			if (cfg_logmask >= LOG_DEBUG)
			{
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to excute cmd_list.sh\n");
			}
			snprintf(sh_tmp, sizeof(sh_tmp), "/etc/voip/cmd_list.sh %s %s %s", tmp_cmd->fvcli_key, value, ep);
			fp_sh = popen(sh_tmp, "r");
			if (fp_sh == NULL)
			{
				if (cfg_logmask >= LOG_ERR)
				{
					util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "There was an error while attempting to open cmd_list.sh\n");
				}
				exit(1);
			}
			fgets(buffer_sh, sizeof(buffer_sh), fp_sh);
			pclose(fp_sh);
			if (cfg_logmask >= LOG_DEBUG)
			{
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished excuteing cmd_list.sh\n");
			}
			if (cfg_logmask >= LOG_INFO)
			{
				ubus_voip_delete_sub_str(buffer_sh, "\n", buffer_sh);
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "Execute [%s]\n", buffer_sh);
				strcat(buffer_sh, "\n");
			}
			fp_cmd = popen(buffer_sh, "r");
			if (NULL != fp_cmd)
			{
				while (fgets(line, BUF_SIZE_1, fp_cmd) != NULL)
				{
					memset(buf, 0, sizeof(buf));
					if (isdigit(line[0]))
					{
						if (cfg_logmask >= LOG_INFO)
						{
							ubus_voip_delete_sub_str(buffer_sh, "\n", buffer_sh);
							util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "[%s] reply : [%c]\n", buffer_sh, line[0]);
							strcat(buffer_sh, "\n");
						}
					}
					buf[0] = line[0];
					strcat(results, buf);
				}
			}
			else
			{
				if (cfg_logmask >= LOG_ERR)
				{

					ubus_voip_delete_sub_str(buffer_sh, "\n", buffer_sh);
					util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing [%s]\n", buffer_sh);
					strcat(buffer_sh, "\n");
				}
			}
			pclose(fp_cmd);
		}
		// cmdlist script end
	}
	list_for_each_entry_safe(tmp_cmd, tmp_cmd_next, &cmdlist.list, list)
	{
		list_del(&tmp_cmd->list);
		free(tmp_cmd);
	}
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished to execute fvcli using cmd_list\n");
	}
	return results;
}

int ubus_voip_uci_commit_package(char *input_package)
{
	struct uci_context *ctx = uci_alloc_context();
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to execute uci commit\n");
	}
	if (!ctx)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to allocate UCI context\n");
		}
		return FAIL;
	}
	char *pkg_name = (char *)input_package;
	struct uci_package *pkg = NULL;
	if (uci_load(ctx, pkg_name, &pkg) != UCI_OK)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to load package\n");
		}
		uci_free_context(ctx);
		return FAIL;
	}
	int ret = uci_commit(ctx, &pkg, false);
	uci_unload(ctx, pkg);
	uci_free_context(ctx);
	if (ret != UCI_OK)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to commit package\n");
		}
		return FAIL;
	}
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished to execute uci commit\n");
	}
	return SUCCESS;
}

int ubus_voip_uci_revert_package(char *input_package)
{
	struct uci_context *ctx = uci_alloc_context();
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip is beginning to execute uci revert\n");
	}
	if (!ctx)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to allocate UCI context\n");
		}
		return FAIL;
	}
	char *pkg_name = (char *)input_package;
	struct uci_ptr ptr = {
		.package = pkg_name,
	};
	if (uci_lookup_ptr(ctx, &ptr, pkg_name, 1) != UCI_OK)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to revert package\n");
		}
		uci_free_context(ctx);
		return FAIL;
	}
	int ret = uci_revert(ctx, &ptr);
	uci_unload(ctx, ptr.p);
	uci_free_context(ctx);
	if (ret != UCI_OK)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to revert package\n");
		}
		return FAIL;
	}
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip has finished to execute uci revert\n");
	}
	return SUCCESS;
}

void ubus_voip_uci_changes()
{
	char buf[BUF_SIZE_1];
	char input_section[BUF_SIZE_1];
	char input_key[BUF_SIZE_1];
	int ret = 1;
	char *key;
	char *value;
	FILE *fp;
	struct ubus_voip_changes_list *tmp, *tmp_next;
	struct list_head tmplist;
	struct uci_context *ctx = uci_alloc_context();
	INIT_LIST_HEAD(&tmplist);
	// log
	struct uci_ptr ptr = {
		.package = "fvt_evoip",
		.section = "GENERAL",
		.option = "ubus_voip_log_level",
	};
	if (uci_lookup_ptr(ctx, &ptr, NULL, true) == UCI_OK)
	{
		int log_level = atoi(ptr.o->v.string);
		switch (log_level)
		{
		case LOG_EMERG:
			cfg_logmask = LOG_EMERG;
			break;
		case LOG_ALERT:
			cfg_logmask = LOG_ALERT;
			break;
		case LOG_CRIT:
			cfg_logmask = LOG_CRIT;
			break;
		case LOG_ERR:
			cfg_logmask = LOG_ERR;
			break;
		case LOG_WARNING:
			cfg_logmask = LOG_WARNING;
			break;
		case LOG_NOTICE:
			cfg_logmask = LOG_NOTICE;
			break;
		case LOG_INFO:
			cfg_logmask = LOG_INFO;
			break;
		case LOG_DEBUG:
			cfg_logmask = LOG_DEBUG;
			break;
		case LOG_VERBOSE:
			cfg_logmask = LOG_VERBOSE;
			break;
		default:
			cfg_logmask = LOG_UNDEFINED;
			break;
		}
	}
	uci_free_context(ctx);
	if (cfg_logmask >= LOG_DEBUG)
	{
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Ubus_voip log level = %s\n", cfg_logmask);
		util_logprintf(UBUS_VOIP_LOG_FILE, LOG_DEBUG, "Start ubus_voip uci changes\n");
	}
	//
	fp = popen("uci changes fvt_evoip", "r");
	if (fp == NULL)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing [uci changes fvt_evoip]\n");
		}
		return;
	}
	while (fgets(buf, BUF_SIZE_1, fp) != NULL)
	{
		key = strtok(buf, "=");
		//  Deal uci entry is ' ' and would be -fvt_evoip.section.key
		if (buf[0] == '-')
		{
			key = strtok(buf, "-");
			ubus_voip_delete_sub_str(key, "\n", key);
			strcat(key, "-");
		}
		if (key != NULL)
		{
			value = strtok(NULL, "=");
			if (buf[0] == '-')
			{
				value = "";
			}
			if (value != NULL)
			{
				tmp = (struct ubus_voip_changes_list *)malloc(sizeof(struct ubus_voip_changes_list));
				strcpy(tmp->value, value);
				strcpy(tmp->key, key);
				ubus_voip_delete_sub_str(tmp->value, "\'", tmp->value);
				ubus_voip_delete_sub_str(tmp->value, "\n", tmp->value);
				ubus_voip_delete_sub_str(tmp->key, "\n", tmp->key);
				list_add(&tmp->list, &tmplist);
			}
		}
	}
	list_for_each_entry(tmp, &tmplist, list)
	{
		ubus_voip_delete_sub_str(tmp->value, "\'", tmp->value);
		sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
		strcpy(tmp->ret, ubus_voip_cmd_list_set(input_section, input_key, tmp->value));
		if (tmp->ret[0] != '1' &&
			tmp->ret[0] != '0')
		{
			if (cfg_logmask >= LOG_ERR)
			{
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing uci changes : [%s.%s=%s] ret = [%c]\n", input_section, input_key, tmp->value, tmp->ret[0]);
			}
			ret = FAIL;
		}
	}
	// Deal fvcli result
	if (ret == SUCCESS)
	{
		// fvcli set ok
		if (cfg_logmask >= LOG_INFO)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "Fvcli command all success\n");
		}
		int check = ubus_voip_uci_commit_package("fvt_evoip");
		if (check == FAIL)
		{
			if (cfg_logmask >= LOG_ERR)
			{
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "UCI commit fvt_evoip failed\n");
			}
		}
		// change the fvt_evoip_uci.config file
		list_for_each_entry(tmp, &tmplist, list)
		{
			ubus_voip_set_fv_voip(tmp->key, tmp->value);
		}
	}
	else if (ret == FAIL)
	{
		// fvcli set fail
		if (cfg_logmask >= LOG_INFO)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_INFO, "Fvcli command failed\n");
		}
		int check = ubus_voip_uci_revert_package("fvt_evoip");
		if (check == FAIL)
		{
			if (cfg_logmask >= LOG_ERR)
			{
				util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "UCI revert fvt_evoip failed\n");
			}
		}
		// voip revert
		list_for_each_entry(tmp, &tmplist, list)
		{
			// when reversing, change the + to - and the - to +
			if (strcmp(tmp->value, " ") == 0)
			{
				ubus_voip_delete_sub_str(tmp->key, "-", tmp->key);
			}
			if (strstr(tmp->key, "+") != NULL)
			{
				for (int i = 0; i < strlen(input_key); i++)
				{
					if (input_key[i] == '+')
					{
						input_key[i] = '-';
					}
				}
				strcpy(tmp->ret, ubus_voip_cmd_list_set(input_section, input_key, tmp->value));
			}
			else if (strstr(tmp->key, "-") != NULL)
			{
				sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
				for (int i = 0; i < strlen(input_key); i++)
				{
					if (input_key[i] == '-')
					{
						input_key[i] = '+';
					}
				}
				strcpy(tmp->ret, ubus_voip_cmd_list_set(input_section, input_key, tmp->value));
			}
			else
			{
				// uci test
				ctx = uci_alloc_context();
				if (!ctx)
				{
					if (cfg_logmask >= LOG_ERR)
					{
						util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to allocate UCI context\n");
					}
				}
				// uci get
				sscanf(tmp->key, "fvt_evoip.%[^.].%s", input_section, input_key);
				struct uci_ptr ptr = {
					.package = "fvt_evoip",
					.section = input_section,
					.option = input_key,
				};
				if (uci_lookup_ptr(ctx, &ptr, NULL, 1) == UCI_OK)
				{
					strcpy(tmp->ret, ubus_voip_cmd_list_set(input_section, input_key, ptr.o->v.string));
				}
				else
				{
					if (cfg_logmask >= LOG_ERR)
					{
						util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "Failed to get uci parameter\n");
					}
				}
				uci_free_context(ctx);
			}
		}
	}
	list_for_each_entry_safe(tmp, tmp_next, &tmplist, list)
	{
		list_del(&tmp->list);
		free(tmp);
	}
	if (pclose(fp) != 0)
	{
		if (cfg_logmask >= LOG_ERR)
		{
			util_logprintf(UBUS_VOIP_LOG_FILE, LOG_ERR, "An error occurred while executing [uci changes fvt_evoip]\n");
		}
	}
}

static int
ubus_voip_cmd_end(struct ubus_context *ctx, struct ubus_object *obj,
				  struct ubus_request_data *req, const char *method,
				  struct blob_attr *msg)
{
	// Call ubus_voip_uci_changes function with owner as argument
	ubus_voip_uci_changes();
	return 0;
}
int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;
	while ((ch = getopt(argc, argv, "s:t:")) != -1)
	{
		switch (ch)
		{
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
	if (rpc_exec_timeout < 1000 || rpc_exec_timeout > 600000)
	{
		return -1;
	}
	signal(SIGPIPE, SIG_IGN);
	uloop_init();
	ctx = ubus_connect(ubus_socket);
	if (!ctx)
	{
		return FAIL;
	}
	ubus_add_uloop(ctx);
	static const struct ubus_method uci_methods[] = {
		{.name = "cmd_end", .handler = ubus_voip_cmd_end}};
	static struct ubus_object_type uci_type =
		UBUS_OBJECT_TYPE("fvt_evoip", uci_methods);
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
