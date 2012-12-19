#define _POSIX_C_SOURCE 200809

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "io.h"
#include "player.h"

struct player {
	enum player_state state;
	int		device_id;
	struct audio   *au;
};

int
player_init(struct player **play,
	    int device_id)
{
	int		failure = 0;

	if (*play == NULL) {
		*play = calloc((size_t) 1, sizeof(struct player));
		if (*play == NULL) {
			failure = -1;
		}
	}
	if (!failure) {
		(*play)->device_id = device_id;
	}
	return failure;
}

void
player_free(struct player *play)
{
    if (play->au)
        audio_unload(play->au);
	free(play);
}

enum error
player_eject(struct player *play)
{
	enum error	result;

	switch (play->state) {
	case STOPPED:
	case PLAYING:
	case VOID:
		if (play->au != NULL) {
			audio_unload(play->au);
			play->au = NULL;
		}
		play->state = EJECTED;
		result = E_OK;
		debug(0, "player ejected");
		break;
	case EJECTED:
		/* Ejecting while ejected is harmless and common */
		break;
	case SHUTTING_DOWN:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
	}

	return result;
}

enum error
player_play(struct player *play)
{
	enum error	result;

	switch (play->state) {
	case STOPPED:
		result = audio_start(play->au);
		if (result == E_OK)
			play->state = PLAYING;
		break;
	case PLAYING:
		result = error(E_BAD_STATE, "already playing");
		break;
	case EJECTED:
		result = error(E_BAD_STATE, "nothing loaded");
		break;
	case VOID:
		result = error(E_BAD_STATE, "init only to ejected");
		break;
	case SHUTTING_DOWN:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
		break;
	}

	return result;
}

enum error
player_stop(struct player *play)
{
	enum error	result;

	switch (play->state) {
	case PLAYING:
		play->state = STOPPED;
                result = audio_stop(play->au);
		break;
	case STOPPED:
		result = error(E_BAD_STATE, "already stopped");
		break;
	case EJECTED:
		result = error(E_BAD_STATE, "can't stop - nothing loaded");
		break;
	case SHUTTING_DOWN:
		result = error(E_BAD_STATE, "player is shutting down");
		break;
	default:
		result = error(E_BAD_STATE, "unknown state");
	}

	return result;
}

enum error
player_load(struct player *play, const char *filename)
{
	enum error	result;
	enum audio_init_err err;

	err = audio_load(&(play->au),
			 filename,
			 play->device_id);
	if (err) {
		switch (err) {
		case E_AINIT_OPEN_INPUT:
			result = error(E_NO_FILE, filename);
			break;
		case E_AINIT_FIND_STREAM_INFO:
			result = error(E_BAD_FILE, "can't find stream info");
			break;
		case E_AINIT_DEVICE_OPEN_FAIL:
			result = error(E_BAD_FILE, "can't open device");
			break;
		case E_AINIT_NO_STREAM:
			result = error(E_BAD_FILE, "can't find stream");
			break;
		case E_AINIT_CANNOT_ALLOC_AUDIO:
			result = error(E_NO_MEM, "can't alloc audio structure");
			break;
		case E_AINIT_CANNOT_ALLOC_PACKET:
			result = error(E_NO_MEM, "can't alloc packet");
			break;
		case E_AINIT_CANNOT_ALLOC_FRAME:
			result = error(E_NO_MEM, "can't alloc frame");
			break;
		default:
			result = error(E_UNKNOWN, "unknown error");
			break;
		}
		player_eject(play);
	} else {
		debug(0, "loaded %s", filename);
		play->state = STOPPED;
		result = E_OK;
	}

	return result;
}

void
player_update(struct player *play)
{
	play = (void *)play;
/*	if (play->state == PLAYING) {
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
	}*/
}

enum error
player_shutdown(struct player *play)
{
	enum error	result;

	result = player_eject(play);
	play->state = SHUTTING_DOWN;

	return result;
}

enum player_state
player_state(struct player *play)
{
	return play->state;
}
