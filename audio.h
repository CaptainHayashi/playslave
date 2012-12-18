#include "player.h"

struct audio;
enum error;

enum audio_init_err
audio_load(struct audio **au, const char *filename,
	   int device_id);

void		audio_unload(struct audio *au);

enum error	audio_start(struct audio *au);
enum error	audio_stop(struct audio *au);


enum audio_init_err {
	E_AINIT_OK = 0,
	E_AINIT_OPEN_INPUT,
	E_AINIT_FIND_STREAM_INFO,
	E_AINIT_DEVICE_OPEN_FAIL,
	E_AINIT_NO_STREAM,
	E_AINIT_NO_CODEC,
	E_AINIT_CANNOT_ALLOC_AUDIO,
	E_AINIT_CANNOT_ALLOC_PACKET,
	E_AINIT_CANNOT_ALLOC_FRAME,
	E_AINIT_BAD_RATE,
};

enum audio_play_err {
	E_PLAY_OK = 0,
	E_PLAY_EOF,
	E_PLAY_DECODE_ERR,
};
