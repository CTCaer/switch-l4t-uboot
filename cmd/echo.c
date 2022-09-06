/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

static void echo_char(const char c, int err)
{
	if (!err)
		putc(c);
	else
		eputc(c);
}

static void echo_string(const char *s, int err)
{
	if (!err)
		puts(s);
	else
		eputs(s);
}

static int do_echo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[], int err)
{
	int i;
	int putnl = 1;

	for (i = 1; i < argc; i++) {
		char *p = argv[i];
		char *nls; /* new-line suppression */

		if (i > 1)
			echo_char(' ', err);

		nls = strstr(p, "\\c");
		if (nls) {
			char *prenls = p;

			putnl = 0;
			/*
			 * be paranoid and guess that someone might
			 * say \c more than once
			 */
			while (nls) {
				*nls = '\0';
				echo_string(prenls, err);
				*nls = '\\';
				prenls = nls + 2;
				nls = strstr(prenls, "\\c");
			}
			echo_string(prenls, err);
		} else {
			echo_string(p, err);
		}
	}

	if (putnl)
		echo_char('\n', err);

	return 0;

}

static int do_echo_out(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return do_echo(cmdtp, flag, argc, argv, 0);
}

static int do_echo_err(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return do_echo(cmdtp, flag, argc, argv, 1);
}

U_BOOT_CMD(
	echo,	CONFIG_SYS_MAXARGS,	1,	do_echo_out,
	"echo args to stdout console",
	"[args..]\n"
	"    - echo args to console; \\c suppresses newline"
);

U_BOOT_CMD(
	echoe,	CONFIG_SYS_MAXARGS,	1,	do_echo_err,
	"echo args to stderr console",
	"[args..]\n"
	"    - echo args to console; \\c suppresses newline"
);
