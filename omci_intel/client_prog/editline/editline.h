/* Internal header file for editline library.
 *
 * Copyright (c) 1992, 1993  Simmule Turner and Rich Salz. All rights reserved.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or of the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 * 1. The authors are not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits must appear in the documentation.
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits must appear in the documentation.
 * 4. This notice may not be removed or altered.
 */

#ifndef __PRIVATE_EDITLINE_H__
#define __PRIVATE_EDITLINE_H__

#include <config.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef SYS_UNIX
#include "unix.h"
#endif
#ifdef SYS_OS9
#include "os9.h"
#endif
/* The following two are for TIOCGWINSZ */
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

#define MEM_INC         64
#define SCREEN_INC      256

/* http://stackoverflow.com/questions/1598773/is-there-a-standard-function-in-c-that-would-return-the-length-of-an-array/1598827#1598827 */
#define ARRAY_ELEMENTS(arr) ((sizeof(arr)/sizeof(0[arr])) / ((size_t)(!(sizeof(arr) % sizeof(0[arr])))))

/*
**  Variables and routines internal to this package.
*/
extern int	rl_eof;
extern int	rl_erase;
extern int	rl_intr;
extern int	rl_kill;
extern int	rl_quit;
#ifdef CONFIG_SIGSTOP
extern int	rl_susp;
#endif
void  rl_ttyset(int Reset);
void  rl_add_slash(char *path, char *p);
char *rl_complete(char *token, int *match);
int   rl_list_possib(char *token, char ***av);

#ifndef HAVE_STDLIB_H
extern char	*getenv(const char *name);
extern char	*malloc(size_t size);
extern char	*realloc(void *ptr, size_t size);
extern char	*memcpy(void *dest, const void *src, size_t n);
extern char	*strcat(char *dest, const char *src);
extern char	*strchr(const char *s, int c);
extern char	*strrchr(const char *s, int c);
extern char	*strcpy(char *dest, const char *src);
extern char	*strdup(const char *s);
extern int	strcmp(const char *s1, const char *s2);
extern int	strlen(const char *s);
extern int	strncmp(const char *s1, const char *s2, size_t n);
#endif/* !HAVE_STDLIB_H */

#ifndef HAVE_STRDUP
extern char	*strdup(const char *s);
#endif

/* Minix editline
 *
 * Copyright (c) 1992, 1993  Simmule Turner and Rich Salz. All rights reserved.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or of the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 * 1. The authors are not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits must appear in the documentation.
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits must appear in the documentation.
 * 4. This notice may not be removed or altered.
 */
#ifndef __EDITLINE_H__
#define __EDITLINE_H__

/* Handy macros when binding keys. */
#define CTL(x)          ((x) & 0x1F)
#define ISCTL(x)        ((x) && (x) < ' ')
#define UNCTL(x)        ((x) + 64)
#define META(x)         ((x) | 0x80)
#define ISMETA(x)       ((x) & 0x80)
#define UNMETA(x)       ((x) & 0x7F)

/* Command status codes. */
typedef enum {
    CSdone = 0,                 /* OK */
    CSeof,                      /* Error, or EOF */
    CSmove,
    CSdispatch,
    CSstay,
    CSsignal
} el_status_t;

/* Editline specific types, despite rl_ prefix.  From Heimdal project. */
typedef char* rl_complete_func_t(char*, int*);
typedef int rl_list_possib_func_t(char*, char***);
typedef el_status_t el_keymap_func_t(void);
typedef int  rl_hook_func_t(void);
typedef int  rl_getc_func_t(void);
typedef void rl_voidfunc_t(void);
typedef void rl_vintfunc_t(int);

/* Display 8-bit chars "as-is" or as `M-x'? Toggle with M-m. (Default:0 - "as-is") */
extern int rl_meta_chars;

/* Editline specific functions. */
extern char *      el_find_word(void);
extern void        el_print_columns(int ac, char **av);
extern el_status_t el_ring_bell(void);
extern el_status_t el_del_char(void);

extern el_status_t el_bind_key(int key, el_keymap_func_t function);
extern el_status_t el_bind_key_in_metamap(int key, el_keymap_func_t function);

extern char       *rl_complete(char *token, int *match);
extern int         rl_list_possib(char *token, char ***av);

/* For compatibility with FSF readline. */
extern int         rl_point;
extern int         rl_mark;
extern int         rl_end;
extern int         rl_inhibit_complete;
extern char       *rl_line_buffer;
extern const char *rl_readline_name;
extern FILE       *rl_instream;  /* The stdio stream from which input is read. Defaults to stdin if NULL - Not supported yet! */
extern FILE       *rl_outstream; /* The stdio stream to which output is flushed. Defaults to stdout if NULL - Not supported yet! */
extern int         el_no_echo;   /* e.g under emacs, don't echo except prompt */
extern int         el_hist_size; /* size of history scrollback buffer, default: 15 */

extern void rl_initialize(void);
extern void rl_reset_terminal(const char *terminal_name);

extern char *readline(const char *prompt);
extern void add_history(const char *line);

extern int read_history(const char *filename);
extern int write_history(const char *filename);

rl_complete_func_t    *rl_set_complete_func(rl_complete_func_t *func);
rl_list_possib_func_t *rl_set_list_possib_func(rl_list_possib_func_t *func);

void rl_prep_terminal(int meta_flag);
void rl_deprep_terminal(void);

int rl_getc(void);

void tty_puts(const char *p);
void tty_string(char *p);

#endif  /* __EDITLINE_H__ */

//#include "../include/editline.h"
#endif  /* __PRIVATE_EDITLINE_H__ */
