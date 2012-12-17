#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "player.h"

typedef void    (*nullary_cmd_ptr) (struct player_context *);
typedef void    (*unary_cmd_ptr) (struct player_context *, const char *);

const int	WORD_LEN = 4;

/* Commands */
enum nullary_cmds {
	CMD_PLAY,
	CMD_STOP,
	CMD_EJCT,
	CMD_QUIT,
	NUM_NULLARY_CMDS
};

enum unary_cmds {
	CMD_LOAD,
	CMD_SEEK,
	NUM_UNARY_CMDS
};

const char     *NULLARY_WORDS[NUM_NULLARY_CMDS] = {
	"PLAY", //CMD_PLAY
	"STOP", //CMD_STOP
	"EJCT", //CMD_EJECT
	"QUIT", //CMD_QUIT
};

const char     *UNARY_WORDS[NUM_UNARY_CMDS] = {
	"LOAD", //CMD_LOAD
	"SEEK", //CMD_SEEK
};

nullary_cmd_ptr	NULLARY_FUNCS[NUM_NULLARY_CMDS] = {
	player_play,
	player_stop,
	player_eject,
	player_shutdown
};

unary_cmd_ptr	UNARY_FUNCS[NUM_UNARY_CMDS] = {
	player_load,
	NULL // TODO:player_seek
};

void
check_commands(struct player_context *context)
{
	if (input_waiting()) {
		char           *buffer = NULL;
		char           *argument = NULL;
		size_t		num_bytes = 0;
		ssize_t		length = getline(&buffer, &num_bytes, stdin);

		/* Find start of argument(s) */
		ssize_t		i;
		for (i = 4; i < length && argument == NULL; i++) {
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
		for (j = length - 1; isspace(buffer[j]); i--) {
			buffer[j] = '\0';
		}

		bool		gotcmd = false;

		//Check nullary commands first
			int		n;
		for (n = 0; n < NUM_NULLARY_CMDS && !gotcmd; n++) {
			if (strncmp(NULLARY_WORDS[n], buffer, WORD_LEN) == 0) {
				gotcmd = true;
				if (argument == NULL) {
					NULLARY_FUNCS[n] (context);
				} else {
					printf("WHAT Not expecting argument here\n");
				}
			}
		}
		//Now unaries
			int		u;
		for (u = 0; u < NUM_UNARY_CMDS && !gotcmd; n++) {
			if (strncmp(UNARY_WORDS[u], buffer, WORD_LEN) == 0) {
				gotcmd = true;
				if (argument != NULL) {
					UNARY_FUNCS[u] (context, argument);
				} else {
					printf("WHAT Expecting argument here\n");
				}
			}
		}
		if (!gotcmd) {
			printf("WHAT Not a valid command\n");
		}
		free(buffer);
	}
}
