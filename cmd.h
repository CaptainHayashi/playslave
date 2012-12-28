/*
 * =============================================================================
 *
 *       Filename:  cmd.h
 *
 *    Description:  Interface to command parser
 *
 *        Version:  1.0
 *        Created:  26/12/2012 04:18:45
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

#ifndef CMD_H
#define CMD_H

/**  INCLUDES  ****************************************************************/

#include "constants.h"		/* WORD_LEN */
#include "errors.h"		/* enum error */
#include "player.h"		/* struct player */

/**  MACROS  ******************************************************************/

/* Helper macros for defining commands */
#define NCMD(word, func) {word, C_NULLARY, {.ncmd = func}}
#define UCMD(word, func) {word, C_UNARY, {.ucmd = func}}
#define END_CMDS {"XXXX", C_END_OF_LIST, {.ignore = '\0'}}

/**  TYPEDEFS  ****************************************************************/

typedef enum error (*nullary_cmd_ptr) (void *usr);
typedef enum error (*unary_cmd_ptr) (void *usr, const char *arg);

/**  DATA TYPES  **************************************************************/

/* Type of command, used for the tagged union in struct cmd. */
enum cmd_type {
	C_NULLARY,		/* Command accepts no arguments */
	C_UNARY,		/* Command accepts one argument */
        C_END_OF_LIST           /* Sentinel for end of command list */
};

/* Command structure */
struct cmd {
	const char	word[WORD_LEN];	/* Command word */
	enum cmd_type	function_type;	/* Tag for function union */
	union {
		nullary_cmd_ptr	ncmd;	/* No-argument command */
		unary_cmd_ptr	ucmd;	/* One-argument command */
                char            ignore; /* Use with C_END_OF_LIST */
	}		function;	/* Function pointer to actual command */
};

/**  FUNCTIONS  ***************************************************************/

enum error	check_commands(struct player *pl);

#endif				/* !CMD_H */
