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
#include "cmd.h"		/* struct cmd, enum cmd_type */
#include "errors.h"		/* error */
#include "io.h"			/* response */
#include "messages.h"		/* Messages (usually errors) */
#include "player.h"		/* This is where the commands live */

/**  GLOBAL VARIABLES  ********************************************************/

static struct cmd commands[] = {
	/* Nullary commands */
	NCMD("play", player_cmd_play),
	NCMD("stop", player_cmd_stop),
	NCMD("ejct", player_cmd_ejct),
	NCMD("quit", player_cmd_quit),
	/* Unary commands */
	UCMD("load", player_cmd_load),
	UCMD("seek", player_cmd_seek),
	END_CMDS
};

/**  STATIC PROTOTYPES  *******************************************************/

static enum error handle_command(struct player *play);
static enum error exec_cmd(void *usr, const char *word, const char *arg);
static enum error exec_cmd_struct(void *usr, struct cmd *cmd, const char *arg);

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
		size_t		j;

		for (i = WORD_LEN - 1; i < length && argument == NULL; i++) {
			if (!isspace((int)buffer[i])) {
				/* Assume this is where the arg is */
				argument = buffer + i;
				break;
			}
		}

		/*
		 * Strip any whitespace out of the argument (by setting it to
		 * the null character, thus null-terminating the argument)
		 */
		for (j = length - 1; isspace((int)buffer[j]); i--)
			buffer[j] = '\0';

		err = exec_cmd((void *)play, buffer, argument);
		if (err == E_OK)
			response(R_OKAY, "%s", buffer);
	}
	dbug("command processed");
	free(buffer);

	return err;
}

static enum error
exec_cmd(void *usr, const char *word, const char *arg)
{
	bool		gotcmd;
	struct cmd     *cmd;
	enum error	err = E_OK;

	for (cmd = &commands[0], gotcmd = false;
	     cmd->function_type != C_END_OF_LIST && !gotcmd;
	     cmd++) {
		if (strncmp(cmd->word, word, WORD_LEN - 1) == 0) {
			gotcmd = true;
			err = exec_cmd_struct(usr, cmd, arg);
		}
	}

	if (!gotcmd)
		err = error(E_BAD_COMMAND, "%s", MSG_CMD_NOSUCH);

	return err;
}

static enum error
exec_cmd_struct(void *usr, struct cmd *cmd, const char *arg)
{
	enum error	err = E_OK;

	switch (cmd->function_type) {
	case C_NULLARY:
		if (arg == NULL)
			err = cmd->function.ncmd(usr);
		else
			err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGN);
		break;
	case C_UNARY:
		if (arg == NULL)
			err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGU);
		else
			err = cmd->function.ucmd(usr, arg);
		break;
	case C_END_OF_LIST:
		err = error(E_INTERNAL_ERROR, "%s", MSG_CMD_HITEND);
		break;
	}

	return err;
}
