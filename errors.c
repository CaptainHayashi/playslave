/*
 * =============================================================================
 *
 *       Filename:  errors.c
 *
 *    Description:  Error and debug reporting functions
 *
 *        Version:  1.0
 *        Created:  25/12/2012 23:03:56
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York Computing Team
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

/**  INCLUDES  ****************************************************************/

#include <stdarg.h>		/* va_list etc. */
#include <stdio.h>		/* snprintf */
#include <stdlib.h>		/* calloc */

#include "errors.h"		/* enum error, enum error_blame */
#include "io.h"			/* vresponse, enum response */
#include "messages.h"		/* MSG_ERR_NOMEM */

/**  GLOBAL VARIABLES  ********************************************************/

/* Names for each error in the system.
 * Names should always just be the constant with leading E_ removed.
 * (More human-friendly information should be in the error string.)
 */
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

/* Mappings of errors to the factor that we assign blame to.
 * This is used to decide which sort of response to send the error as.
 */
const enum error_blame ERROR_BLAME[NUM_ERRORS] = {
	EB_PROGRAMMER,		/* E_OK - should never raise an error message */
	/* User errors */
	EB_USER,		/* E_NO_FILE */
	EB_USER,		/* E_BAD_STATE */
	EB_USER,		/* E_BAD_COMMAND */
	/* Environment errors */
	EB_ENVIRONMENT,		/* E_BAD_FILE */
	EB_ENVIRONMENT,		/* E_BAD_CONFIG */
	/* System errors */
	EB_ENVIRONMENT,		/* E_AUDIO_INIT_FAIL */
	EB_PROGRAMMER,		/* E_INTERNAL_ERROR */
	EB_ENVIRONMENT,		/* E_NO_MEM */
	/* Misc */
	EB_PROGRAMMER,		/* E_EOF - should never show an error message */
	EB_PROGRAMMER,		/* E_UNKNOWN - error should be more specific */
};

/* This maps error blame factors to response codes. */
const enum response BLAME_RESPONSE[NUM_ERROR_BLAMES] = {
	R_WHAT,			/* EB_USER */
	R_FAIL,			/* EB_ENVIRONMENT */
	R_OOPS,			/* EB_PROGRAMMER */
};

/**  PUBLIC FUNCTIONS  ********************************************************/

/* Sends a debug message. */
void
dbug(const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(R_DBUG, format, ap);
	va_end(ap);
}

/* Throws an error message.
 *
 * This does not propagate the error anywhere - the error code will need to be
 * sent up the control chain and handled at the top of the player.  It merely
 * sends a response through stdout and potentially stderr to let the client/logs
 * know something went wrong.
 */
enum error
error(enum error code, const char *format,...)
{
	va_list		ap;	/* Variadic arguments */
	va_list		ap2;	/* Variadic arguments */
	const char     *emsg;	/* Stores error name */
	char           *buf;	/* Temporary buffer for rendering error */
	size_t		buflen;	/* Length for creating buffer */

	buf = NULL;
	emsg = ERRORS[(int)code];

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	va_copy(ap2, ap);

	/*
	 * Problem: adding the error name into the format without changing
	 * the format string dynamically (a bad idea).
	 *
	 * Our strategy: render the error into a temporary buffer using magicks,
	 * then send it to response.
	 */

	/*
	 * Try printing the error into a null buffer to get required length
	 * (see http://stackoverflow.com/questions/4899221)
	 */
	buflen = vsnprintf(NULL, 0, format, ap);
	buf = calloc(buflen + 1, sizeof(char));
	if (buf != NULL)
		vsnprintf(buf, buflen + 1, format, ap2);

	response(BLAME_RESPONSE[ERROR_BLAME[code]], "%s %s",
		 emsg,
		 buf == NULL ? MSG_ERR_NOMEM : buf);
	va_end(ap);

	if (buf != NULL)
		free(buf);

	return code;
}
