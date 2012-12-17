#include <stdint.h>
#include <assert.h>

#include "io.h"
#include "player.h"
#include "audio.h"

struct player_context {
	enum player_state state;
	int		ao_driver_id;
	ao_option      *ao_options;
	struct audio   *au;
};


int
player_init(struct player_context **play,
	    int ao_driver_id,
	    ao_option * ao_options)
{
	int		failure = 0;

	if (*play == NULL) {
		*play = calloc(1, sizeof(struct player_context));
		if (*play == NULL) {
			failure = -1;
		}
	}
	if (!failure) {
		player_eject(*play);
		(*play)->ao_driver_id = ao_driver_id;
		(*play)->ao_options = ao_options;
	}
	return failure;
}

void
player_eject(struct player_context *play)
{
	if (play->au != NULL) {
		audio_unload(play->au);
		play->au = NULL;
	}
	play->state = EJECTED;
	debug(0, "player ejected");
}

void
player_play(struct player_context *play)
{
	switch (play->state) {
	case STOPPED:
		play->state = PLAYING;
		break;
	case PLAYING:
		error(E_BAD_STATE_CHANGE, "already playing");
		break;
	case EJECTED:
		error(E_BAD_STATE_CHANGE, "nothing loaded");
		break;
	default:
		error(E_BAD_STATE_CHANGE, "unknown state");
		break;
	}
}

void
player_stop(struct player_context *play)
{
	play->state = STOPPED;
}

void
player_load(struct player_context *play, const char *filename)
{
	int		err;

	player_eject(play);

	err = audio_load(&(play->au),
			 filename,
			 play->ao_driver_id,
			 play->ao_options);
	if (err) {
		switch (err) {
		case E_AINIT_OPEN_INPUT:
			error(E_NO_FILE, filename);
			break;
		case E_AINIT_FIND_STREAM_INFO:
			error(E_BAD_FILE, "can't find stream information");
			break;
		case E_AINIT_DEVICE_OPEN_FAIL:
			error(E_BAD_FILE, "can't open device");
			break;
		case E_AINIT_NO_STREAM:
			error(E_BAD_FILE, "can't find stream");
			break;
		case E_AINIT_CANNOT_ALLOC_AUDIO:
			error(E_NO_MEM, "can't alloc audio structure");
			break;
		case E_AINIT_CANNOT_ALLOC_PACKET:
			error(E_NO_MEM, "can't alloc packet");
			break;
		case E_AINIT_CANNOT_ALLOC_FRAME:
			error(E_NO_MEM, "can't alloc frame");
			break;
		default:
			error(E_UNKNOWN, "unknown error");
			break;
		}
		player_eject(play);
	} else {
		play->state = STOPPED;
	}
}

void
player_update(struct player_context *play)
{
	if (play->state == PLAYING) {
		assert(play->au != NULL);

		int		err;

		err = audio_play_frame(play->au);
		if (err) {
			switch (err) {
			case E_PLAY_EOF:
				break;
			case E_PLAY_DECODE_ERR:
				error(E_BAD_FILE, "decode error");
				break;
			default:
				error(E_UNKNOWN, "unknown error");
				break;
			}
			player_eject(play);

		}
	}
}

void
player_shutdown(struct player_context *play)
{
	player_eject(play);
	play->state = SHUTTING_DOWN;
}

enum player_state
player_state(struct player_context *play)
{
	return play->state;
}
