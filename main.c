/*-
 * main.c - main function
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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libavformat/avformat.h>
#include <portaudio.h>

#include "cmd.h"
#include "io.h"
#include "player.h"

static enum error get_driver_id(int *driver, int argc, char *argv[]);
static void	main_loop(struct player *play);

static void
main_loop(struct player *pl)
{
	enum state	st;
	struct timespec t;

	t.tv_sec = 0;
	t.tv_nsec = 1000;

	for (st = player_state(pl); st != QUITTING; st = player_state(pl)) {
		check_commands(pl);
		player_update(pl);
		nanosleep(&t, NULL);
	}
}

int
main(int argc, char *argv[])
{
	/* TODO: cleanup */
	int		driver;
	int		exit_code;
	enum error	err = E_OK;
	struct player  *context = NULL;

	if (Pa_Initialize() != (int)paNoError)
		err = error(E_AUDIO_INIT_FAIL, "couldn't init portaudio");
	if (err == E_OK)
		err = get_driver_id(&driver, argc, argv);
	if (err == E_OK) {
		av_register_all();
		err = player_init(&context, driver);
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

static enum error
get_driver_id(int *driver, int argc, char *argv[])
{
	int		num_devices;
	enum error	err = E_OK;

	num_devices = Pa_GetDeviceCount();
	if (argc < 2) {
		int		i;
		const PaDeviceInfo *dev;

		err = error(E_BAD_CONFIG, "need device number, why not try:");

		/* Print out the available devices */
		for (i = 0; i < num_devices; i++) {
			dev = Pa_GetDeviceInfo(i);
			debug(0, "%u: %s", i, dev->name);
		}
	} else {
		*driver = (int)strtol(argv[1], NULL, 10);
		/* TODO: check */
	}

	return err;
}
