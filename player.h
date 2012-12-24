/*-
 * player.h - interface to the high-level player structure
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
#ifndef PLAYER_H
#define PLAYER_H

#include "errors.h"

/* The player structure contains all persistent state in the program.
 *
 * struct player is an opaque structure; only player.c knows its true
 * definition.
 */
struct player;

/* Enumeration of states that the player can be in.
 *
 * The player is effectively a finite-state machine whose behaviour
 * at any given time is dictated by the current state, which is
 * represented by an instance of 'enum state'.
 */
enum state {
	VOID,			/* No state (usually when player starts up) */
	EJECTED,		/* No file loaded */
	STOPPED,		/* File loaded but not playing */
	PLAYING,		/* File loaded and playing */
	QUITTING,		/* Player about to quit */
};

/*
 * Initialisation and de-initialisation
 */
enum error	player_init(struct player **pl, int driver);
void		player_free(struct player *pl);	/* Deallocates a player. */

/*
 * Nullary commands
 */
enum error	player_ejct(struct player *pl);	/* Ejects current song. */
enum error	player_play(struct player *pl);	/* Plays song. */
enum error	player_quit(struct player *pl);	/* Closes player. */
enum error	player_stop(struct player *pl);	/* Stops song. */

/*
 * Unary commands
 */
/* Loads the file at 'path' into the player 'pl'. */
enum error
player_load(struct player *pl,
	    const char *path);

/*
 * Miscellaneous
 */
enum error	player_update(struct player *pl);
enum state	player_state(struct player *pl);	/* Current player state. */

#endif				/* !PLAYER_H */
