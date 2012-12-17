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
	player_eject,		/* CMD_EJECT */
	player_shutdown		/* CMD_QUIT */
};

unary_cmd_ptr	UNARY_FUNCS[NUM_UNARY_CMDS] = {
	player_load,		/* CMD_LOAD */
	NULL			/* TODO: CMD_SEEK */
};

static void	handle_command(struct player_context *play);

void
check_commands(struct player_context *play)
{
	if (input_waiting()) {
		handle_command(play);
	}
}

static void
handle_command(struct player_context *play)
{
	char           *buffer = NULL;
	char           *argument = NULL;
	size_t		num_bytes = 0;
	ssize_t		length;

	length = getline(&buffer, &num_bytes, stdin);

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

		/* Check nullary commands first */
		int		n;
		for (n = 0; n < NUM_NULLARY_CMDS && !gotcmd; n++) {
			if (strncmp(NULLARY_WORDS[n], buffer, WORD_LEN) == 0) {
				gotcmd = true;
				if (argument == NULL) {
					NULLARY_FUNCS[n] (play);
				} else {
					printf("WHAT Not expecting argument\n");
				}
			}
		}

		/* Now unaries */
		int		u;
		for (u = 0; u < NUM_UNARY_CMDS && !gotcmd; u++) {
			if (strncmp(UNARY_WORDS[u], buffer, WORD_LEN) == 0) {
				gotcmd = true;
				if (argument != NULL) {
					UNARY_FUNCS[u] (play, argument);
				} else {
					printf("WHAT Expecting argument\n");
				}
			}
		}
		if (!gotcmd) {
			printf("WHAT Not a valid command\n");
		}
	}
	free(buffer);
}
