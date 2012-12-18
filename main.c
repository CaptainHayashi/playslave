#define _POSIX_C_SOURCE 200809
#include <pthread.h>
#include <stdio.h>
#include <libavformat/avformat.h>
#include <ao/ao.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "cmd.h"
#include "player.h"

void 
main_loop(struct player_context *context)
{
	while (player_state(context) != SHUTTING_DOWN
	       && !feof(stdin)) {
		check_commands(context);
		player_update(context);
	}
}

void
audio_state_changed(struct player_context *play)
{
        debug(0, "STATE CHANGED OMG %u", player_state(play));
}

void *
audio_main(void *arg)
{
    struct player_context *play = (struct player_context*) arg;

    while (player_state(play) != SHUTTING_DOWN) {
        player_on_state_change(play, audio_state_changed);
    }

    pthread_exit(NULL);
}

int 
main(int argc, char *argv[])
{
	if (argc < 2) {
		error(3, "specify as argument to command");
		exit(3);
	}
	ao_initialize();
	int		driver_id = ao_default_driver_id();
	if (driver_id < 0) {
		error(3, "couldn't get default libao driver");
		exit(3);
	}
	ao_option      *options = NULL;
	ao_append_option(&options, "id", argv[1]);

	struct player_context *context = NULL;

	av_register_all();

	if (player_init(&context, driver_id, options) < 0) {
		error(2, "maybe out of memory?");
		exit(1);
	}

        pthread_t at;
        int at_err = pthread_create(&at, NULL, audio_main, context);
        if (at_err) {
            error(2, "couldn't make a thread");
            exit(2);
        }

        player_eject(context);
	main_loop(context);

	ao_shutdown();
        pthread_exit(NULL);

	return 0;
}
