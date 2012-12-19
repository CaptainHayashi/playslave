/*-
 * cmd.c - command parser
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
#define _POSIX_C_SOURCE 200809
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "player.h"

typedef enum error (*nullary_cmd_ptr) (struct player *);
typedef enum error (*unary_cmd_ptr) (struct player *, const char *);

const int	WORD_LEN = 4;

/* Commands */
enum nullary_cmds {
	CMD_PLAY,		/* Plays the currently LOADed file */
	CMD_STOP,		/* Stops the currently PLAYing file */
	CMD_EJECT,		/* Ejects the currently LOADed file */
	CMD_QUIT,		/* Closes the player */
	NUM_NULLARY_CMDS
};

enum unary_cmds {
	CMD_LOAD,		/* Loads a file into the EJECTed player */
	CMD_SEEK,		/* Seeks somewhere in a PLAYing file */
	NUM_UNARY_CMDS
};

const char     *NULLARY_WORDS[NUM_NULLARY_CMDS] = {
	"PLAY",			/* CMD_PLAY */
	"STOP",			/* CMD_STOP */
	"EJCT",			/* CMD_EJECT */
	"QUIT",			/* CMD_QUIT */
};

const char     *UNARY_WORDS[NUM_UNARY_CMDS] = {
	"LOAD",			/* CMD_LOAD */
	"SEEK",			/* CMD_SEEK */
};

nullary_cmd_ptr	NULLARY_FUNCS[NUM_NULLARY_CMDS] = {
	player_play,		/* CMD_PLAY */
	player_stop,		/* CMD_STOP */
	player_ejct,		/* CMD_EJECT */
	player_quit,		/* CMD_QUIT */
};

unary_cmd_ptr	UNARY_FUNCS[NUM_UNARY_CMDS] = {
	player_load,		/* CMD_LOAD */
	NULL			/* TODO: CMD_SEEK */
};

static void	handle_command(struct player *play);

static bool
try_nullary(struct player *play,
	    const char *buf, const char *arg, enum error *result);

static bool
try_unary(struct player *play,
	  const char *buf, const char *arg, enum error *result);

/****************************************************************************/

void
check_commands(struct player *play)
{
	if (input_waiting()) {
		handle_command(play);
	}
}

/****************************************************************************/

static void
handle_command(struct player *play)
{
	char           *buffer = NULL;
	char           *argument = NULL;
	size_t		num_bytes = 0;
	ssize_t		length;

	length = getline(&buffer, &num_bytes, stdin);
	debug(0, "got command: %s", buffer);

	/* Remember to count newline */
	if (length <= WORD_LEN) {
		printf("WHAT Need a command word\n");
	} else {
		/* Find start of argument(s) */
		ssize_t		i;

		for (i = WORD_LEN; i < length && argument == NULL; i++) {
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
		ssize_t		j;

		for (j = length - 1; isspace(buffer[j]); i--)
			buffer[j] = '\0';

		bool		gotcmd = false;
		enum error	result;

		gotcmd = try_nullary(play, buffer, argument, &result);
		if (!gotcmd)
			gotcmd = try_unary(play, buffer, argument, &result);
		if (!gotcmd)
			printf("WHAT Not a valid command\n");
	}

	debug(0, "command processed");
	free(buffer);
}

static bool
try_nullary(struct player *play,
	    const char *buf,
	    const char *arg, enum error *result)
{
	int		n;
	bool		gotcmd = false;

	for (n = 0; n < NUM_NULLARY_CMDS && !gotcmd; n++) {
		if (strncmp(NULLARY_WORDS[n], buf, WORD_LEN) == 0) {
			gotcmd = true;
			if (arg == NULL) {
				*result = NULLARY_FUNCS[n] (play);
			} else {
				printf("WHAT Not expecting argument\n");
			}
		}
	}

	return gotcmd;
}

static bool
try_unary(struct player *play,
	  const char *buf,
	  const char *arg,
	  enum error *result)
{
	int		u;
	bool		gotcmd = false;

	for (u = 0; u < NUM_UNARY_CMDS && !gotcmd; u++) {
		if (strncmp(UNARY_WORDS[u], buf, WORD_LEN) == 0) {
			gotcmd = true;
			if (arg != NULL) {
				*result = UNARY_FUNCS[u] (play, arg);
			} else {
				printf("WHAT Expecting argument\n");
			}
		}
	}

	return gotcmd;
}
