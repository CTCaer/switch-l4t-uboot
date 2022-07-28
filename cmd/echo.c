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

static int do_echo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	int putnl = 1;
	int puterr = 0;

	for (i = 1; i < argc; i++) {
		char *p = argv[i];
		char *nls; /* new-line suppression */

		if (i > (puterr + 1))
			echo_char(' ', puterr);

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
				echo_string(prenls, puterr);
				*nls = '\\';
				prenls = nls + 2;
				nls = strstr(prenls, "\\c");
			}
			echo_string(prenls, puterr);
		} else {
			nls = strstr(p, "\\e");
			if (nls) {
				puterr = 1;
			} else {
				echo_string(p, puterr);
			}
		}
	}

	if (putnl)
		echo_char('\n', puterr);

	return 0;
}

U_BOOT_CMD(
	echo,	CONFIG_SYS_MAXARGS,	1,	do_echo,
	"echo args to console",
	"[args..]\n"
	"    - echo args to console; \\c suppresses newline, \\e outputs to stderr"
);
