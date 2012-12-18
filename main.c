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
	}
}

void
audio_state_changed(struct player_context *play)
{
    play = (void*) play;
}

void           *
audio_main(void *arg)
{
	struct player_context *play = (struct player_context *)arg;

        enum player_state state;
	for (state = player_state(play);
               state != SHUTTING_DOWN;
               state = player_state(play)) {
            switch(state) {
            case SHUTTING_DOWN:
                /* Thread is going to exit on next turn */
                break;
            case PLAYING:
                player_update(play);
                break;
            case LOADING:
                player_do_load(play);
                break;
            default:
                /* Wait for another state. */
		player_on_state_change(play, audio_state_changed);
            }
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

        pthread_attr_t attr;
       pthread_attr_init(&attr);
       pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
       pthread_t	at;
	int		at_err = pthread_create(&at, &attr, audio_main, context);
	if (at_err) {
		error(2, "couldn't make a thread");
		exit(2);
	}
        pthread_attr_destroy(&attr);

	player_eject(context);
	main_loop(context);

        pthread_join(at, NULL);

	ao_shutdown();
	pthread_exit(NULL);

	return 0;
}
