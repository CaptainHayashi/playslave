/*
 * =============================================================================
 *
 *       Filename:  io.c
 *
 *    Description:  Common input/output functions
 *
 *        Version:  1.0
 *        Created:  25/12/2012 22:12:34
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York
 *
 * =============================================================================
 */
/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define _POSIX_C_SOURCE 200809

/**  INCLUDES  ****************************************************************/

#include <stdarg.h>		/* print functions */
#include <stdbool.h>		/* booleans */
#include <stdio.h>		/* printf, fprintf */
#include <time.h>		/* select etc. */
#include <unistd.h>

#include <sys/select.h>		/* select */

#include "constants.h"		/* WORD_LEN */
#include "io.h"			/* enum response */

/**  GLOBAL VARIABLES  ********************************************************/

/* Names of responses.
 * Names should always just be the constant with leading R_ removed.
 */
const char     RESPONSES[NUM_RESPONSES][WORD_LEN] = {
	/* 'Pull' responses (initiated by client command) */
	"OKAY",			/* R_OKAY */
	"WHAT",			/* R_WHAT */
	"FAIL",			/* R_FAIL */
	"OOPS",			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	"OHAI",			/* R_OHAI */
	"TTFN",			/* R_TTFN */
	"STAT",			/* R_STAT */
	"TIME",			/* R_TIME */
	"DBUG",			/* R_DBUG */
};

/* Whether or not responses should be sent to stdout (to the client). */
bool		RESPONSE_STDOUT[NUM_RESPONSES] = {
	/* 'Pull' responses (initiated by client command) */
	true,			/* R_OKAY */
	true,			/* R_WHAT - usually not a real error per se */
	true,			/* R_FAIL */
	true,			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	true,			/* R_OHAI */
	true,			/* R_TTFN */
	true,			/* R_STAT */
	true,			/* R_TIME */
	false,			/* R_DBUG - should come up in logs etc. */
};

/* Whether or not responses should be sent to stderr (usually logs/console). */
bool		RESPONSE_STDERR[NUM_RESPONSES] = {
	/* 'Pull' responses (initiated by client command) */
	false,			/* R_OKAY */
	false,			/* R_WHAT - usually not a real error per se */
	true,			/* R_FAIL */
	true,			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	false,			/* R_OHAI */
	false,			/* R_TTFN */
	false,			/* R_STAT */
	false,			/* R_TIME */
	true,			/* R_DBUG - should come up in logs etc. */
};

/**  PUBLIC FUNCTIONS  ********************************************************/

/* Sends a response to standard out and, for certain responses, standard error.
 * This is the base function for all system responses.
 */
enum response
vresponse(enum response code, const char *format, va_list ap)
{
	va_list ap2;

	va_copy(ap2, ap);

	if (RESPONSE_STDOUT[(int)code]) {
		printf("%s ", RESPONSES[(int)code]);
		vprintf(format, ap);
		printf("\n");
	}

	if (RESPONSE_STDERR[(int)code]) {
		fprintf(stderr, "%s ", RESPONSES[(int)code]);
		vfprintf(stderr, format, ap2);
		fprintf(stderr, "\n");
	}

	return code;
}

/* Sends a response to standard out and, for certain responses, standard error.
 * This is a wrapper around 'vresponse'.
 */
enum response
response(enum response code, const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(code, format, ap);
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
