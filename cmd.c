/*
 * =============================================================================
 *
 *       Filename:  cmd.c
 *
 *    Description:  Command parser and executor
 *
 *        Version:  1.0
 *        Created:  26/12/2012 03:56:35
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

#define _POSIX_C_SOURCE 200809

/**  INCLUDES  ****************************************************************/

#include <ctype.h>
#include <stdbool.h>		/* bool */
#include <stdio.h>		/* getline */
#include <stdlib.h>
#include <string.h>

#include "constants.h"		/* WORD_LEN */
#include "errors.h"		/* error */
#include "io.h"			/* response */
#include "messages.h"		/* Messages (usually errors) */
#include "player.h"		/* This is where the commands live */

/**  TYPEDEFS  ****************************************************************/

typedef enum error (*nullary_cmd_ptr) (struct player *);
typedef enum error (*unary_cmd_ptr) (struct player *, const char *);

/**  DATA TYPES  **************************************************************/

/* Commands */
enum nullary_cmd {
	CMD_PLAY,		/* Plays the currently LOADed file */
	CMD_STOP,		/* Stops the currently PLAYing file */
	CMD_EJECT,		/* Ejects the currently LOADed file */
	CMD_QUIT,		/* Closes the player */
	NUM_NULLARY_CMDS,
	NULLARY_CMDS_START = 0
};

enum unary_cmd {
	CMD_LOAD,		/* Loads a file into the EJECTed player */
	CMD_SEEK,		/* Seeks somewhere in a PLAYing file */
	NUM_UNARY_CMDS,
	UNARY_CMDS_START = 0
};

/**  GLOBAL VARIABLES  ********************************************************/

/* Command words - these must all be WORD_LEN long */
static const char NULLARY_WORDS[NUM_NULLARY_CMDS][WORD_LEN] = {
	"play",			/* CMD_PLAY */
	"stop",			/* CMD_STOP */
	"ejct",			/* CMD_EJECT */
	"quit",			/* CMD_QUIT */
};

static const char UNARY_WORDS[NUM_UNARY_CMDS][WORD_LEN] = {
	"load",			/* CMD_LOAD */
	"seek",			/* CMD_SEEK */
};

static nullary_cmd_ptr NULLARY_FUNCS[NUM_NULLARY_CMDS] = {
	player_play,		/* CMD_PLAY */
	player_stop,		/* CMD_STOP */
	player_ejct,		/* CMD_EJECT */
	player_quit,		/* CMD_QUIT */
};

static unary_cmd_ptr UNARY_FUNCS[NUM_UNARY_CMDS] = {
	player_load,		/* CMD_LOAD */
	player_seek		/* CMD_SEEK */
};

/**  STATIC PROTOTYPES  *******************************************************/

static enum error handle_command(struct player *play);

static bool
try_nullary(struct player *play,
	    const char *buf, const char *arg, enum error *err);

static bool
try_unary(struct player *play,
	  const char *buf, const char *arg, enum error *err);

/**  PUBLIC FUNCTIONS  ********************************************************/

/*
 * Checks to see if there are any commands waiting on stdio and, if there
 * are, deals with them by running them on the given player.
 */
enum error
check_commands(struct player *play)
{
	enum error	err = E_OK;
	if (input_waiting())
		err = handle_command(play);

	return err;
}

/**  STATIC FUNCTIONS  ********************************************************/

static enum error
handle_command(struct player *play)
{
	size_t		length;
	enum error	err = E_OK;
	char           *buffer = NULL;
	char           *argument = NULL;
	size_t		num_bytes = 0;

	length = getline(&buffer, &num_bytes, stdin);
	dbug("got command: %s", buffer);

	/* Remember to count newline */
	if (length < WORD_LEN)
		err = error(E_BAD_COMMAND, MSG_CMD_NOWORD);
	if (err == E_OK) {
		/* Find start of argument(s) */
		size_t		i;
		ssize_t		j;
		bool		gotcmd = false;

		for (i = WORD_LEN - 1; i < length && argument == NULL; i++) {
			if (!isspace(buffer[i])) {
				/* Assume this is where the arg is */
				argument = buffer + i;
				break;
			}
		}

		/*
		 * Strip any whitespace out of the argument (by setting it to
		 * the null character, thus null-terminating the argument)
		 */
		for (j = length - 1; isspace(buffer[j]); i--)
			buffer[j] = '\0';

		gotcmd = try_nullary(play, buffer, argument, &err);
		if (!gotcmd)
			gotcmd = try_unary(play, buffer, argument, &err);
		if (!gotcmd)
			err = error(E_BAD_COMMAND, "%s", MSG_CMD_NOSUCH);
		else
			response(R_OKAY, "%s", buffer);
	}
	dbug("command processed");
	free(buffer);

	return err;
}

static bool
try_nullary(struct player *play,
	    const char *buf,
	    const char *arg, enum error *err)
{
	int		n;
	bool		gotcmd = false;

	for (n = (int)NULLARY_CMDS_START;
	     n < (int)NUM_NULLARY_CMDS && !gotcmd;
	     n += 1) {
		if (strncmp(NULLARY_WORDS[n], buf, WORD_LEN - 1) == 0) {
			gotcmd = true;
			if (arg == NULL)
				*err = NULLARY_FUNCS[n] (play);
			else
				*err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGN);
		}
	}

	return gotcmd;
}

static bool
try_unary(struct player *play,
	  const char *buf,
	  const char *arg,
	  enum error *err)
{
	int		u;
	bool		gotcmd = false;

	for (u = (int)UNARY_CMDS_START;
	     u < (int)NUM_UNARY_CMDS && !gotcmd;
	     u += 1) {
		if (strncmp(UNARY_WORDS[u], buf, WORD_LEN - 1) == 0) {
			gotcmd = true;
			if (arg != NULL)
				*err = UNARY_FUNCS[u] (play, arg);
			else
				*err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGU);
		}
	}

	return gotcmd;
}
