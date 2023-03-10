#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	CLI_PATH_DEFAULT	"/tmp/f2console.cli"
#define PATH_LEN_MAX		256

struct f2console_cfg {
	char file_path[PATH_LEN_MAX];
	char cli_path[PATH_LEN_MAX];
	int debug_level;
	int order;		// 0: smaller is critical, 1: larger is critical
	int exit_on_error;
	int wait_ms;
	int is_prev_file_done;
};

// cli file related routine ////////////////////////////////////////
static int
str2argv(char *str, char *argv[], int argv_size)
{
	static char buff[1024];
	char *p = buff;
	int i = 0;

	strncpy(buff, str, 1024);
	while (p != NULL) {
		argv[i] = strsep(&p, " \t");
		if (strlen(argv[i]) > 0) {
			i++;
			if (i == argv_size - 1)
				break;
		}
	}
	argv[i] = 0;
	return i;
}
static int
str2val(char *str)
{
	int val;
	if (str[0] == 'x') {
		sscanf(str + 1, "%x", &val);
	} else if (str[0] == '0' && str[1] == 'x') {
		sscanf(str + 2, "%x", &val);
	} else {
		sscanf(str, "%d", &val);
	}
	return val;
}

static int
parse_cli_line(char *input_buff, struct f2console_cfg *cfg)
{
	int argc;
	char *argv[16];
	int ret = 0;

	argc = str2argv(input_buff, argv, 16);
	if (argc >=1 && strcmp(argv[0], "debug_level") == 0) {
		if (argc == 1) {
			fprintf(stderr, "debug_level=%d\n", cfg->debug_level);
		} else {
			cfg->debug_level = str2val(argv[1]);
			fprintf(stderr, "set debug_level=%d\n", cfg->debug_level);
		}
		fprintf(stderr, "\n");

	} else if (argc >=1 && strcmp(argv[0], "order") == 0) {
		if (argc == 1) {
			fprintf(stderr, "order=%d\n", cfg->order);
		} else {
			cfg->order = str2val(argv[1]);
			fprintf(stderr, "set order=%d\n", cfg->order);
		}
		fprintf(stderr, "\n");

	} else if (argc >=1 && strcmp(argv[0], "exit_on_error") == 0) {
		if (argc == 1) {
			fprintf(stderr, "exit_on_error=%d\n", cfg->exit_on_error);
		} else {
			cfg->exit_on_error = str2val(argv[1]);
			fprintf(stderr, "set exit_on_error=%d\n", cfg->exit_on_error);
		}
		fprintf(stderr, "\n");

	} else if (argc >=1 && strcmp(argv[0], "wait_ms") == 0) {
		if (argc == 1) {
			fprintf(stderr, "wait_ms=%d\n", cfg->wait_ms);
		} else {
			cfg->wait_ms = str2val(argv[1]);
			fprintf(stderr, "set wait_ms=%d\n", cfg->wait_ms);
		}
		fprintf(stderr, "\n");

	} else if (argc == 1 && strcmp(argv[0], "dump") == 0) {
		fprintf(stderr, "file_path:%s\n", cfg->file_path);
		fprintf(stderr, "cli_path:%s\n", cfg->cli_path);
		fprintf(stderr, "debug_level=%d\n", cfg->debug_level);
		fprintf(stderr, "order=%d (%s is critical)\n", cfg->order, (cfg->order==0)?"smaller":"larger");
		fprintf(stderr, "exit_on_error=%d\n", cfg->exit_on_error);
		fprintf(stderr, "wait_ms=%d\n", cfg->wait_ms);
		fprintf(stderr, "\n");

	} else if (argc >=1) {
		int i;
		fprintf(stderr, "unknown cmd '");
		for (i=0; i<argc; i++)
			fprintf(stderr, "%s%s", (i>0)?" ":"", argv[i]);
		fprintf(stderr, "'\n");

		fprintf(stderr,
			"available cmd:\n"
			"	debug_level [n]\n"
			"	order [0|1] (0:smaller is critical, 1:larger is critical)\n"
			"	exit_on_error [0|1]\n"
			"	wait_ms [n]\n"
			"	dump\n"
			"	help\n\n");
		ret = -1;
	}
	return ret;
}
static int
parse_cli(char *cli_fname, struct f2console_cfg *cfg)
{
	char buff[1024];
	FILE *fp;

	if ((fp = fopen(cli_fname, "rb")) == NULL)
		return -1;

	buff[0] = 0;
	while (fgets(buff, 1024, fp) > 0) {
		int len = strlen(buff);
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
			buff[len - 1] = 0;
			len--;
		}
		if (len >0)
			parse_cli_line(buff, cfg);
	}
	fclose(fp);
	unlink(cli_fname);

	return 0;
}

// parse argv related routine ////////////////////////////////////////

void
help(char *name)
{
	fprintf(stderr, "%s [options] -f [logfile]\n"
		"options:\n"
		"   -c [cli_filename]\n"
		"   -o [0|1] (0:smaller is critical, 1:larger is critical)\n"
		"   -d [n] (debug_level)\n"
		"   -x [0|1] (exit on error)\n"
		"   -w [wait_ms] (wait in per iteration)\n"
		, name);
}

static int
parse_argv(int argc, char *argv[], struct f2console_cfg *cfg)
{
	struct option long_options[] = {
		{"file_path", 		1, NULL, 'f'},
		{"cli_path", 		1, NULL, 'c'},
		{"debug_level", 	1, NULL, 'd'},
		{"order",		1, NULL, 'o'},
		{"exit_on_error", 	1, NULL, 'x'},
		{"wait_ms",		1, NULL, 'w'},
		{"help", 		0, NULL, 'h'},
		{ 0,    		0,    0,  0 },
	};
	char* const short_options = "f:c:o:d:x:w:h";
	int32_t c;

	while((c = getopt_long(argc, argv, short_options, long_options, NULL)) !=-1)  {
		switch (c) {
			case 'f':
				strncpy(cfg->file_path, optarg, PATH_LEN_MAX-1);
				fprintf(stderr, "set file_path=%s\n", optarg);
				break;
			case 'c':
				strncpy(cfg->cli_path, optarg, PATH_LEN_MAX-1);
				fprintf(stderr, "set cli_path=%s\n", optarg);
				break;
			case 'd':
				cfg->debug_level = atoi(optarg);
				fprintf(stderr, "set debug_level=%d\n", cfg->debug_level);
				break;
			case 'o':
				cfg->order = atoi(optarg);
				fprintf(stderr, "order=%d\n", cfg->order);
				break;
			case 'x':
				cfg->exit_on_error = atoi(optarg);
				fprintf(stderr, "set exit_on_error=%d\n", cfg->exit_on_error);
				break;
			case 'w':
				cfg->wait_ms = atoi(optarg);
				if (cfg->wait_ms <100)
					cfg->wait_ms = 100;
				if (cfg->wait_ms >5000)
					cfg->wait_ms = 5000;
				fprintf(stderr, "set wait_ms=%d\n", cfg->wait_ms);
				break;
			case 'h':
				help(argv[0]);
				break;
		}
	}

	return 0;
}

// main ////////////////////////////////////////

static void
f2console_cfg_init(struct f2console_cfg *cfg)
{
	memset((void*)cfg, 0, sizeof(struct f2console_cfg));

	cfg->file_path[0] = 0;
	strncpy(cfg->cli_path, CLI_PATH_DEFAULT, PATH_LEN_MAX-1);
	cfg->debug_level = -1;
	cfg->order = 0;
	cfg->exit_on_error = 0;
	cfg->wait_ms = 500;
	cfg->is_prev_file_done = 0;
}

static int
kill_pid_on_same_logfile(char *progname, char *logfile)
{
	FILE *fp=popen("ps w", "r");
	char buff[1024];
	char *s1, *s2;
	int found = 0;

	if (fp == NULL)
		return -1;
	while (fgets(buff, 1024, fp) != NULL) {
		buff[strlen(buff)-1] = 0;
		if ((s1=strstr(buff, progname)) != NULL &&
		    (s2=strstr(buff, " -f ")) != NULL &&
		    strncmp(s2+4, logfile, strlen(logfile))==0) {
			int pid;
			sscanf(buff, "%d", &pid);
			if (pid != getpid()) {
				fprintf(stderr, "kill previous pid %d (%s)\n", pid, s1);
				kill(pid, SIGKILL);
				found++;
			}
		}
	}
	fclose(fp);
	return found;
}

int
get_debug_level_from_line(char *line)
{
	char *s, *endptr;
	int level;

	if (line[0] != '[')	// not start with '['
		return -1;
	if ((s = strstr(line, "][")) == NULL) 	// the '][' not found
		return -1;

	level = strtol(s+2, &endptr, 10);
	if (endptr == s+2)	// no value is detected, so the next ptr is the same as start
		return -1;
	if (endptr[0] != ']')	// level is not immediately followed by ']'
		return -1;

	return level;
}

int
main(int argc, char **argv)
{
	struct f2console_cfg *cfg = (struct f2console_cfg *)malloc(sizeof(struct f2console_cfg));
	long prev_end_offset = -1;
	FILE *fp_output;

	signal(SIGHUP, SIG_IGN);

	f2console_cfg_init(cfg);
	parse_argv(argc, argv, cfg);
	if (strlen(cfg->file_path) == 0) {
		help(argv[0]);
		exit(1);
	}

	// kill f2console instance that monitors same logfile
	kill_pid_on_same_logfile(argv[0], cfg->file_path);

	// cli interaction, error msg will goto stderr
	// the content of monitored logfile will go to /dev/console, or stdout
	/*
	fp_output = fopen("/dev/console", "w");
	if (fp_output == NULL) {
		fprintf(stderr, "/dev/console not available, use stdout instead\n");
		fp_output = stdout;
	}
	*/
	fp_output = stdout;

	while (1) {
		FILE *fp;
		long end;

		parse_cli(cfg->cli_path, cfg);
		fp = fopen(cfg->file_path, "r");
		if (fp == NULL) {			// ensure file open ok
			//fprintf(stderr, "fopen error(%d:%s)\n", errno, strerror(errno));
			if (cfg->exit_on_error) {
				break;
			} else {
				sleep(1);
				continue;
			}
		}

		if (fseek(fp, 0, SEEK_END) <0) {	// ensure file is seekable
			fprintf(stderr, "fseek error(%d:%s)\n", errno, strerror(errno));
			if (cfg->exit_on_error) {
				break;
			} else {
				fclose(fp);
				sleep(1);
				continue;
			}
		}
		end = ftell(fp);
		if (prev_end_offset < 0)
			prev_end_offset = end;

		if (prev_end_offset != end) {
			char *line = NULL;
			int prev_line_level = -1;
			int is_last_line_terminated = 1;
			size_t len = 0;
			ssize_t read;

			if (end < prev_end_offset) {
				FILE *fp_prev;
				char file_path_prev[PATH_LEN_MAX];
				long end_prev;
				// new file check .1 done
				snprintf(file_path_prev, PATH_LEN_MAX, "%s.1", cfg->file_path);
				fp_prev = fopen(file_path_prev, "r");
				if (fp_prev != NULL && cfg->is_prev_file_done == 0) {
					if (fseek(fp_prev, 0, SEEK_END) <0) {	// ensure file is seekable
						fprintf(stderr, "fseek error(%d:%s)\n", errno, strerror(errno));
						if (cfg->exit_on_error) {
							break;
						} else {
							fclose(fp);
							sleep(1);
							continue;
						}
					}
					end_prev = ftell(fp_prev);
					if (end_prev > prev_end_offset) {
						fclose(fp);
						fp = fp_prev;
						fseek(fp, prev_end_offset, SEEK_SET);
						cfg->is_prev_file_done = 1;
					} else {
						fseek(fp, 0, SEEK_SET);
					}
				} else {
					// new file, start from 0
					fseek(fp, 0, SEEK_SET);
					cfg->is_prev_file_done = 0;
				}
			} else {
				// continue from last position
				fseek(fp, prev_end_offset, SEEK_SET);
			}
			while ((read = getline(&line, &len, fp)) != -1) {
				if (is_last_line_terminated) {
					if (strncmp(line, "[_ROTATION_]", 12) == 0) {
						//fprintf(fp_output, "- %s", line);
					} else {
						int level = get_debug_level_from_line(line);
						if (level == -1) {
							level = prev_line_level;	// level not found in this line, inherit level from prev
						} else {
							prev_line_level = level;
						}
						if (cfg->debug_level == -1) {
							fprintf(fp_output, "%s", line);
						} else if (cfg->order == 0) {	// smaller is critical
							if (level != -1 && level <= cfg->debug_level)
								fprintf(fp_output, "%s", line);
						} else {		// larger is critical
							if (level != -1 && level >= cfg->debug_level)
								fprintf(fp_output, "%s", line);
						}
					}
				} else {	// not a start of line, inherit level from prev
					if (cfg->debug_level == -1) {
						fprintf(fp_output, "%s", line);
					} else if (cfg->order == 0) {	// smaller is critical
						if (prev_line_level != -1 && prev_line_level <= cfg->debug_level)
							fprintf(fp_output, "%s", line);
					} else {		// larger is critical
						if (prev_line_level != -1 && prev_line_level >= cfg->debug_level)
							fprintf(fp_output, "%s", line);
					}
				}
				if (line[read-1] == '\n' || line[read-1] == '\r')
					is_last_line_terminated = 1;
				else
					is_last_line_terminated = 0;
			}
			prev_end_offset = ftell(fp);
			if (line)
				free(line);
		}

		fclose(fp);

		usleep(cfg->wait_ms*1000);
	}

	exit(1);
}
