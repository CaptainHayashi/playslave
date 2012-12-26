/*
 * =============================================================================
 *
 *       Filename:  player.c
 *
 *    Description:  High-level player structure and commands
 *
 *        Version:  1.0
 *        Created:  24/12/2012 20:08:15
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

#include <stdarg.h>		/* gate_state */
#include <stdbool.h>		/* bool */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "constants.h"
#include "io.h"
#include "player.h"

/**  DATA TYPES  **************************************************************/

struct player {
	struct audio   *au;	/* Audio backend structure */

	enum state	cstate;	/* Current state of player FSM */
	int		device;	/* Device ID given at program start */

	uint64_t	ptime;	/* Last observed time in song */
};

/**  GLOBAL VARIABLES  ********************************************************/

/* Names of the states in enum state. */
const char     *STATE_NAMES[NUM_STATES] = {
	"VOID",
	"EJECTED",
	"STOPPED",
	"PLAYING",
	"QUITTING",
};

/* This should be long enough to hold all the state names above separated with
 * spaces and null-terminated.
 */
const size_t	STATE_NAME_BUF = 256;

enum state	GEND = VOID;

/**  STATIC PROTOTYPES  *******************************************************/

static enum error gate_state(struct player *play, enum state s1,...);
static void	set_state(struct player *play, enum state state);

/**  PUBLIC FUNCTIONS  ********************************************************/

/*----------------------------------------------------------------------------
 *  Initialisation and de-initialisation
 *----------------------------------------------------------------------------*/

enum error
player_init(struct player **play, int device)
{
	enum error	err = E_OK;

	if (*play == NULL) {
		*play = calloc((size_t)1, sizeof(struct player));
		if (*play == NULL) {
			err = E_NO_MEM;
		}
	}
	if (err == E_OK) {
		(*play)->cstate = EJECTED;
		(*play)->device = device;
	}
	return err;
}

void
player_free(struct player *play)
{
	if (play->au)
		audio_unload(play->au);
	free(play);
}

/*----------------------------------------------------------------------------
 *  Nullary commands
 *----------------------------------------------------------------------------*/

enum error
player_ejct(struct player *play)
{
	enum error	err;

	err = gate_state(play, STOPPED, PLAYING, EJECTED, GEND);
	if (err == E_OK && player_state(play) != EJECTED) {
		if (play->au != NULL) {
			audio_unload(play->au);
			play->au = NULL;
		}
		set_state(play, EJECTED);
		play->ptime = 0;
	}
	/* Ejecting while ejected is harmlessly ignored */

	return err;
}

enum error
player_play(struct player *play)
{
	enum error	err;

	err = gate_state(play, STOPPED, GEND);
	if (err == E_OK)
		err = audio_start(play->au);
	if (err == E_OK)
		set_state(play, PLAYING);

	return err;
}

enum error
player_quit(struct player *play)
{
	enum error	err;

	err = player_ejct(play);
	set_state(play, QUITTING);

	return err;
}

enum error
player_stop(struct player *play)
{
	enum error	err;

	err = gate_state(play, PLAYING, GEND);
	if (err == E_OK)
		err = audio_stop(play->au);
	if (err == E_OK)
		set_state(play, STOPPED);

	return err;
}

/*----------------------------------------------------------------------------
 *  Unary commands
 *----------------------------------------------------------------------------*/

enum error
player_load(struct player *play, const char *filename)
{
	enum error	err;

	err = audio_load(&(play->au), filename, play->device);
	if (err)
		player_ejct(play);
	else {
		dbug("loaded %s", filename);
		set_state(play, STOPPED);
	}

	return err;
}

enum error
player_seek(struct player *play, const char *time_str)
{
	uint64_t	time;
	char           *end;
	enum state	state;
	enum error	err = E_OK;

	state = player_state(play);

	/* TODO: proper overflow checking */

	time = (uint64_t)strtoull(time_str, &end, 10);

	if (time_str == end)
		err = error(E_BAD_COMMAND, "expecting number");
	/* Allow second-based indexing for convenience */
	else if (strcmp(end, "s") == 0 ||
		 strcmp(end, "sec") == 0)
		time *= USECS_IN_SEC;

	/* Weed out any unwanted states */
	if (err == E_OK)
		err = gate_state(play, PLAYING, STOPPED, GEND);
	/* We need the player engine stopped in order to seek */
	if (err == E_OK && state == PLAYING)
		err = player_stop(play);
	if (err == E_OK)
		err = audio_seek_usec(play->au, time);
	/* If we were playing before we'd ideally like to resume */
	if (err == E_OK && state == PLAYING)
		err = player_play(play);

	return err;
}

/*----------------------------------------------------------------------------
 *  Miscellaneous
 *----------------------------------------------------------------------------*/

enum error
player_update(struct player *pl)
{
	enum error	err = E_OK;

	if (pl->cstate == PLAYING) {
		if (audio_halted(pl->au)) {
			err = player_ejct(pl);
		} else {
			/* Send a time pulse upstream every TIME_USECS usecs */
			uint64_t	time = audio_usec(pl->au);
			if (time / TIME_USECS > pl->ptime / TIME_USECS) {
				response(R_TIME, "%u", time);
			}
			pl->ptime = time;

			err = audio_decode(pl->au);
		}
	}
	return err;
}

enum state
player_state(struct player *play)
{
	return play->cstate;
}

/**  STATIC FUNCTIONS  ********************************************************/

/* Throws an error if the current state is not in the state set provided by
 * argument s1 and subsequent arguments up to 'GEND'.
 *
 * As a variadic function, the argument list MUST be terminated with 'GEND'.
 */
static enum error
gate_state(struct player *play, enum state s1,...)
{
	va_list		ap;
	int		i;
	enum state	state;
	char		state_names[STATE_NAME_BUF];
	char           *sep = " ";
	char           *snptr;
	enum error	err = E_OK;
	bool		in_state = false;

	state = player_state(play);
	snptr = state_names;
	*snptr = '\0';

	va_start(ap, s1);
	for (i = (int)s1; i != (int)GEND; i = va_arg(ap, int)) {
		if ((int)state == i) {
			in_state = true;
			break;
		} else {
			strncat(snptr,
				sep,
				STATE_NAME_BUF - (snptr - state_names));
			strncat(snptr,
				STATE_NAMES[i],
				STATE_NAME_BUF - (snptr - state_names));
		}
	}
	va_end(ap);

	if (!in_state)
		err = error(E_BAD_STATE,
			    "%s not in {%s }",
			    STATE_NAMES[state], state_names);

	return err;
}

/* Sets the player state and honks accordingly. */
static void
set_state(struct player *play, enum state state)
{
	enum state pstate = play->cstate;

	play->cstate = state;

	response(R_STAT, "%s %s", STATE_NAMES[pstate], STATE_NAMES[state]);
}
