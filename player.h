#ifndef PLAYER_H
#define PLAYER_H

#include "errors.h"

/* The player structure contains all persistent state in playslave.
 *
 * struct player is an opaque structure; only player.c knows its true
 * definition.
 */
struct player;

/* Enumeration of states that the player can be in. */
enum player_state {
	VOID,			/* No state (usually when player starts up) */
	EJECTED,		/* No file loaded */
	STOPPED,		/* File loaded but not playing */
	PLAYING,		/* File loaded and playing */
	SHUTTING_DOWN		/* Player about to quit */
};

int
player_init(struct player **pl,
	    int driver_id);
void		player_free(struct player *pl);

/* Player commands */
enum error	player_eject(struct player *pl);
enum error	player_load(struct player *pl, const char *path);
enum error	player_play(struct player *pl);
enum error	player_shutdown(struct player *pl);
enum error	player_stop(struct player *pl);

void		player_update(struct player *pl);

enum player_state player_state(struct player *pl);

#endif				/* !PLAYER_H */
