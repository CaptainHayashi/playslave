#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <libavformat/avformat.h>
#include <portaudio.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "cmd.h"
#include "player.h"

static void
main_loop(struct player_context *play)
{
        enum player_state state;
	for (state = player_state(play);
               state != SHUTTING_DOWN;
               state = player_state(play)) {
            check_commands(play);
            player_update(play);
	}
}

int
main(int argc, char *argv[])
{
        if (Pa_Initialize() != paNoError) {
            error(3, "couldn't init portaudio");
            exit(3);
        }

        int numDevices = Pa_GetDeviceCount();

	if (argc < 2) {
		error(3, "specify as argument to command");

                int i;
                const PaDeviceInfo *dev;

                for (i = 0; i < numDevices; i++) {
                    dev = Pa_GetDeviceInfo(i);
                    debug(0, dev->name);
                }

		exit(3);
	}

        int driver_id = (int) strtol(argv[1], NULL, 10);

	struct player_context *context = NULL;

	av_register_all();

	if (player_init(&context, driver_id) < 0) {
		error(2, "maybe out of memory?");
		exit(1);
	}

	player_eject(context);
	main_loop(context);

        Pa_Terminate();

	return 0;
}
