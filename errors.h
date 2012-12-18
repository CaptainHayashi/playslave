#ifndef ERRORS_H
#define ERRORS_H

/* Categories of error. */
enum error {
	E_OK,
	E_NO_FILE,
	E_BAD_FILE,
	E_BAD_STATE,
	E_BAD_CONFIG,
	E_INTERNAL_ERROR,
	E_NO_MEM,
	E_UNKNOWN,
	NUM_ERRORS,
};

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

#endif /* !ERRORS_H */
