/*
 * =============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Entry point.
 *
 *        Version:  1.0
 *        Created:  26/12/2012 01:41:52
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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libavformat/avformat.h>
#include <portaudio.h>

#include "constants.h"		/* LOOP_NSECS */
#include "cmd.h"
#include "io.h"
#include "messages.h"		/* MSG_xyz */
#include "player.h"

/**  STATIC PROTOTYPES  *******************************************************/

static enum error device_id(PaDeviceIndex *device, int argc, char *argv[]);
static void	main_loop(struct player *play);

/**  PUBLIC FUNCTIONS  ********************************************************/

/* The main entry point. */
int
main(int argc, char *argv[])
{
	/* TODO: cleanup */
	PaDeviceIndex	device;
	int		exit_code;
	enum error	err = E_OK;
	struct player  *context = NULL;

	if (Pa_Initialize() != (int)paNoError)
		err = error(E_AUDIO_INIT_FAIL, "couldn't init portaudio");
	if (err == E_OK)
		err = device_id(&device, argc, argv);
	if (err == E_OK) {
		av_register_all();
		err = player_init(&context, device);
	}
	if (err == E_OK)
		err = player_ejct(context);
	if (err == E_OK) {
		main_loop(context);
		Pa_Terminate();
	}
	if (err == E_OK)
		exit_code = EXIT_SUCCESS;
	else
		exit_code = EXIT_FAILURE;

	return exit_code;
}

/**  STATIC FUNCTIONS  ********************************************************/

/* Tries to parse the device ID. */
static enum error
device_id(PaDeviceIndex *device, int argc, char *argv[])
{
	int		num_devices;
	enum error	err = E_OK;

	num_devices = Pa_GetDeviceCount();
	/*
	 * Possible Improvement: This is rather dodgy code for getting the
	 * device ID out of the command line arguments, maybe make it a bit
	 * more robust.
	 */
	if (argc < 2) {
		int		i;
		const PaDeviceInfo *dev;

		err = error(E_BAD_CONFIG, MSG_DEV_NOID);

		/* Print out the available devices */
		for (i = 0; i < num_devices; i++) {
			dev = Pa_GetDeviceInfo(i);
			dbug("%u: %s", i, dev->name);
		}
	} else {
		*device = (int)strtoul(argv[1], NULL, 10);
		if (*device >= num_devices)
			err = error(E_BAD_CONFIG, MSG_DEV_BADID);
	}

	return err;
}

/* The main loop of the program. */
static void
main_loop(struct player *pl)
{
	struct timespec	t;

	t.tv_sec = 0;
	t.tv_nsec = LOOP_NSECS;

	response(R_OHAI, "%s", MSG_OHAI);	/* Say hello */
	while (player_state(pl) != QUITTING) {
		/*
		 * Possible Improvement: separate command checking and player
		 * updating into two threads.  Player updating is quite
		 * intensive and thus impairs the command checking latency.
		 * Do this if it doesn't make the code too complex.
		 */
		check_commands(pl);
		player_update(pl);
		nanosleep(&t, NULL);
	}
	response(R_TTFN, "%s", MSG_TTFN);	/* Wave goodbye */
}
