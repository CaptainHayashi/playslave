/*-
 * player.c - high-level player structure
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "io.h"
#include "player.h"

struct player {
	struct audio   *au;	/* Audio backend structure */

	enum state	cstate;	/* Current state of player FSM */
	int		device;	/* Device ID given at program start */
};

enum error
player_init(struct player **play,
	    int device)
{
	enum error	err = E_OK;

	if (*play == NULL) {
		*play = calloc((size_t)1, sizeof(struct player));
		if (*play == NULL) {
			err = E_NO_MEM;
		}
	}
	if (!err) {
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

enum error
player_ejct(struct player *play)
{
	enum error	result;

	switch (play->cstate) {
	case STOPPED:
	case PLAYING:
	case VOID:
		if (play->au != NULL) {
			audio_unload(play->au);
			play->au = NULL;
		}
		play->cstate = EJECTED;
		result = E_OK;
		debug(0, "player ejected");
		break;
	case EJECTED:
		/* Ejecting while ejected is harmless and common */
		break;
	case QUITTING:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
	}

	return result;
}

enum error
player_play(struct player *play)
{
	enum error	result;

	switch (play->cstate) {
	case STOPPED:
		result = audio_start(play->au);
		if (result == E_OK)
			play->cstate = PLAYING;
		break;
	case PLAYING:
		result = error(E_BAD_STATE, "already playing");
		break;
	case EJECTED:
		result = error(E_BAD_STATE, "nothing loaded");
		break;
	case VOID:
		result = error(E_BAD_STATE, "init only to ejected");
		break;
	case QUITTING:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
		break;
	}

	return result;
}

enum error
player_stop(struct player *play)
{
	enum error	result;

	switch (play->cstate) {
	case PLAYING:
		result = audio_stop(play->au);
		if (result == E_OK)
			play->cstate = STOPPED;
		break;
	case STOPPED:
		result = error(E_BAD_STATE, "already stopped");
		break;
	case EJECTED:
		result = error(E_BAD_STATE, "can't stop - nothing loaded");
		break;
	case QUITTING:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
	}

	return result;
}

enum error
player_load(struct player *play, const char *filename)
{
	enum error	err;

	err = audio_load(&(play->au),
			 filename,
			 play->device);
	if (err)
		player_ejct(play);
	else {
		debug(0, "loaded %s", filename);
		play->cstate = STOPPED;
	}

	return err;
}

void
player_update(struct player *play)
{
	enum error	err = E_OK;
	struct timespec t;

	if (play->cstate == PLAYING) {
		if (audio_halted(play->au)) {
			err = player_ejct(play);
		} else {
			err = audio_decode(play->au);
		}
	}

	t.tv_sec = 0;
	t.tv_nsec = 1000;
	nanosleep(&t, NULL);
	/* TODO: do something with err? */
}

enum error
player_quit(struct player *play)
{
	enum error	result;

	result = player_ejct(play);
	play->cstate = QUITTING;

	return result;
}

enum state
player_state(struct player *play)
{
	return play->cstate;
}
