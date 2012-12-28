/*
 * =============================================================================
 *
 *       Filename:  player.h
 *
 *    Description:  Interface to the high-level player structure
 *
 *        Version:  1.0
 *        Created:  25/12/2012 07:00:26
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
#ifndef PLAYER_H
#define PLAYER_H

/**  INCLUDES  ****************************************************************/

#include "errors.h" /* enum error */

/**  DATA TYPES  **************************************************************/

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
	S_VOID,			/* No state (usually when player starts up) */
	S_EJCT, 		/* No file loaded */
	S_STOP, 		/* File loaded but not playing */
	S_PLAY, 		/* File loaded and playing */
	S_QUIT, 		/* Player about to quit */
        /*--------------------------------------------------------------------*/
	NUM_STATES              /* Number of items in enum */
};

/**  FUNCTIONS  ***************************************************************/

/*----------------------------------------------------------------------------
 * Initialisation and de-initialisation
 *----------------------------------------------------------------------------*/
enum error	player_init(struct player **pl, int driver);
void		player_free(struct player *pl);	/* Deallocates a player. */

/*----------------------------------------------------------------------------
 *  Main loop
 *----------------------------------------------------------------------------*/
enum error      player_main_loop(struct player *pl);

/*----------------------------------------------------------------------------
 * Nullary commands
 *----------------------------------------------------------------------------*/
enum error	player_cmd_ejct(void *v_play);	/* Ejects current song. */
enum error	player_cmd_play(void *v_play);	/* Plays song. */
enum error	player_cmd_quit(void *v_play);	/* Closes player. */
enum error	player_cmd_stop(void *v_play);	/* Stops song. */

/*----------------------------------------------------------------------------
 * Unary commands
 *----------------------------------------------------------------------------*/
enum error	player_cmd_load(void *v_play, const char *path);
enum error	player_cmd_seek(void *v_play, const char *time_str);

/*----------------------------------------------------------------------------
 * Miscellaneous
 *----------------------------------------------------------------------------*/
enum state	player_state(struct player *pl);	/* Current state. */

#endif				/* not PLAYER_H */
