#ifndef AUDIO_H
#define AUDIO_H

#include "errors.h"

/* The audio structure contains all state pertaining to the currently
 * playing audio file.
 */
struct audio;

/* Loads a file and constructs an audio structure to hold the playback
 * state.
 */
enum audio_init_err
audio_load(struct audio **au,	/* Location for the audio struct pointer */
	   const char *path,	/* File to load into the audio struct */
	   int device_id);	/* ID of the device to play out on */

void		audio_unload(struct audio *au);	/* Frees an audio struct */
enum error	audio_start(struct audio *au);	/* Starts playback */
enum error	audio_stop(struct audio *au);	/* Stops playback */

#endif				/* !AUDIO_H */
