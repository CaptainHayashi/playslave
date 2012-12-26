/*
 * =============================================================================
 *
 *       Filename:  io.h
 *
 *    Description:  Interface to common input/output functions
 *
 *        Version:  1.0
 *        Created:  25/12/2012 22:10:42
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

#ifndef IO_H
#define IO_H

/**  INCLUDES  ****************************************************************/

#include <stdarg.h> /* vresponse */

/**  DATA TYPES  **************************************************************/

/* Four-character response codes.
 *
 * NOTE: If you're adding new responses here, PLEASE update the arrays in io.c
 * to add a name and behaviour information to each new response.
 */
enum response {
	/* 'Pull' responses (initiated by client command) */
	R_OKAY,			/* Request was valid and produced an answer */
	R_WHAT,			/* Request was invalid/user error */
	R_FAIL,			/* Error, pointing blame at environment */
	R_OOPS,			/* Error, pointing blame at programmer */
	/* 'Push' responses (initiated by server) */
	R_OHAI,			/* Server starting up */
	R_TTFN,			/* Server shutting down */
	R_STAT,			/* Server changing state */
	R_TIME,			/* Server sending current time */
	R_DBUG,			/* Debug information */
	/*--------------------------------------------------------------------*/
	NUM_RESPONSES		/* Number of items in enum */
};

/**  FUNCTIONS  ***************************************************************/

enum response	response(enum response code, const char *format,...);
enum response	vresponse(enum response code, const char *format, va_list ap);
int		input_waiting(void);

#endif				/* not IO_H */
