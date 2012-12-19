#ifndef CMD_H
#define CMD_H

#include "player.h"

/*
 * Checks to see if there are any commands waiting on stdio and, if there
 * are, deals with them by running them on the given player.
 */
void		check_commands(struct player *pl);

#endif				/* !CMD_H */
