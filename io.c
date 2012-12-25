/*-
 * io.c - common standard input/output functions
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * playslave is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with playslave; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "io.h"

/* TODO: move to errors.c? */
const char     *ERRORS[NUM_ERRORS] = {
	"OK",			/* E_OK */
	/* User errors */
	"NO_FILE",		/* E_NO_FILE */
	"BAD_STATE",		/* E_BAD_STATE */
	"BAD_COMMAND",		/* E_BAD_COMMAND */
	/* Environment errors */
	"BAD_FILE",		/* E_BAD_FILE */
	"BAD_CONFIG",		/* E_BAD_CONFIG */
	/* System errors */
	"AUDIO_INIT_FAIL",	/* E_AUDIO_INIT_FAIL */
	"INTERNAL_ERROR",	/* E_INTERNAL_ERROR */
	"NO_MEM",		/* E_NO_MEM */
	/* Misc */
	"EOF",			/* E_EOF */
	"UNKNOWN",		/* E_UNKNOWN */
};

void
debug(int level, const char *format,...)
{
	va_list		ap;
	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);

	level = (int)level;

	fprintf(stderr, "DBUG ");
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

enum error
error(enum error code, const char *format,...)
{
	va_list		ap;
	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);

	fprintf(stderr, "OOPS %s ", ERRORS[code]);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");

	va_end(ap);

	return code;
}

int
input_waiting(void)
{
	fd_set		rfds;
	struct timeval	tv;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	/* Stop checking immediately. */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(1, &rfds, NULL, NULL, &tv);
}
