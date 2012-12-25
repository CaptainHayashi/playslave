/*
 * =============================================================================
 *
 *       Filename:  audio.c
 *
 *    Description:  Mid-level audio structure and functions
 *
 *        Version:  1.0
 *        Created:  24/12/2012 05:36:54
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

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <portaudio.h>

#include "contrib/pa_ringbuffer.h"

#include "audio.h"
#include "audio_av.h"
#include "audio_cb.h"		/* audio_cb_play */
#include "constants.h"
#include "io.h"

/**  DATA TYPES  **************************************************************/

struct audio {
	enum error	last_err;	/* Last result of decoding */
	struct au_in   *av;	/* ffmpeg state */
	/* shared state */
	char           *frame_ptr;
	unsigned long	frame_samples;
	/* PortAudio state */
	PaUtilRingBuffer *ring_buf;
	char           *ring_data;
	PaStream       *out_strm;	/* Output stream */
	int		device_id;	/* PortAudio device ID */
	uint64_t	used_samples;	/* Counter of samples played */
};

/**  STATIC PROTOTYPES  *******************************************************/

static enum error init_sink(struct audio *au, int device);
static enum error init_ring_buf(struct audio *au, size_t bytes_per_sample);
static enum error free_ring_buf(struct audio *au);

/**  PUBLIC FUNCTIONS  ********************************************************/

/*-----------------------------------------------------------------------------
 * Loading and unloading
 *----------------------------------------------------------------------------*/

enum error
audio_load(struct audio **au,
	   const char *path,
	   int device)
{
	enum error	err = E_OK;

	if (*au != NULL) {
		debug(0, "Audio structure exists, freeing");
		audio_unload(*au);
	}
	*au = calloc((size_t)1, sizeof(struct audio));
	if (*au == NULL)
		err = error(E_NO_MEM, "can't alloc audio structure");
	if (err == E_OK) {
		(*au)->last_err = E_INCOMPLETE;
		err = audio_av_load(&((*au)->av), path);
	}
	if (err == E_OK)
		err = init_sink(*au, device);
	if (err == E_OK)
		err = init_ring_buf(*au, audio_av_samples2bytes((*au)->av, 1L));

	return err;
}

void
audio_unload(struct audio *au)
{
	if (au != NULL) {
		free_ring_buf(au);
		audio_av_unload(au->av);

		if (au->out_strm != NULL) {
			Pa_CloseStream(au->out_strm);
			au->out_strm = NULL;
			debug(0, "closed output stream");
		}
		free(au);
	}
}

/*----------------------------------------------------------------------------
 *  Playback control
 *----------------------------------------------------------------------------*/

enum error
audio_start(struct audio *au)
{
	PaError		pa_err;
	enum error	err = E_OK;

	pa_err = Pa_StartStream(au->out_strm);
	if (pa_err)
		err = error(E_INTERNAL_ERROR, "couldn't start stream");
	else
		debug(0, "audio started");

	return err;
}

enum error
audio_stop(struct audio *au)
{
	PaError		pa_err;
	enum error	err = E_OK;

	pa_err = Pa_AbortStream(au->out_strm);
	if (pa_err)
		err = error(E_INTERNAL_ERROR, "couldn't stop stream");
	else
		debug(0, "audio stopped");

	/* TODO: Possibly recover from dropping frames due to abort. */
	return err;
}

/*----------------------------------------------------------------------------
 *  Inspecting the audio structure
 *----------------------------------------------------------------------------*/

/* Returns the last decoding error, or E_OK if the last decode succeeded. */
enum error
audio_error(struct audio *au)
{
	return au->last_err;
}

/* Checks to see if audio playback has halted of its own accord.
 *
 * If audio is still playing, E_OK will be returned; otherwise the decoding
 * error that caused playback to halt will be returned.  E_UNKNOWN is returned
 * if playback has halted but the last error report was E_OK.
 */
enum error
audio_halted(struct audio *au)
{
	enum error	err = E_OK;

	if (!Pa_IsStreamActive(au->out_strm)) {
		err = au->last_err;
		/* Abnormal stream halts with error being OK are weird... */
		if (err == E_OK)
			err = E_UNKNOWN;
	}
	return err;
}

/* Gets the current played position in the song, in microseconds.
 *
 * As this may be executing whilst the playing callback is running,
 * do not expect it to be highly accurate.
 */
uint64_t
audio_usec(struct audio *au)
{
	return audio_av_samples2usec(au->av, au->used_samples);
}

/* Gets the ring buffer that the playing callback should use to get decoded
 * samples.
 */
PaUtilRingBuffer *
audio_ringbuf(struct audio *au)
{
	return au->ring_buf;
}


/*----------------------------------------------------------------------------
 *  Playback position
 *----------------------------------------------------------------------------*/

/* Increments the used samples counter, which is used to determine the current
 * position in the song, by 'samples' samples.
 */
void
audio_inc_used_samples(struct audio *au, uint64_t samples)
{
	au->used_samples += samples;
}

/*----------------------------------------------------------------------------
 *  Decoding
 *----------------------------------------------------------------------------*/

enum error
audio_decode(struct audio *au)
{
	unsigned long	cap;
	unsigned long	count;
	enum error	err = E_OK;

	cap = (unsigned long)PaUtil_GetRingBufferWriteAvailable(au->ring_buf);
	count = (cap < au->frame_samples ? cap : au->frame_samples);
	if (count > 0) {
		/*
		 * We can copy some already decoded samples into the ring
		 * buffer
		 */
		unsigned long	num_written;

		num_written = PaUtil_WriteRingBuffer(au->ring_buf,
						     au->frame_ptr,
						 (ring_buffer_size_t)count);
		if (num_written != count)
			err = error(E_INTERNAL_ERROR, "ringbuf write error");

		au->frame_samples -= num_written;
		au->frame_ptr += audio_av_samples2bytes(au->av, num_written);
	}
	if (au->frame_samples == 0) {
		/* We need to decode some new frames! */
		err = audio_av_decode(au->av,
				      &(au->frame_ptr),
				      &(au->frame_samples));
	}
	au->last_err = err;
	return err;
}

/**  STATIC FUNCTIONS  ********************************************************/

static enum error
init_sink(struct audio *au, int device)
{
	PaError		pa_err;
	double		sample_rate;
	unsigned long	samples_per_buf;
	PaStreamParameters pars;
	enum error	err = E_OK;

	sample_rate = audio_av_sample_rate(au->av);

	err = audio_av_pa_config(au->av, device, &pars, &samples_per_buf);
	pa_err = Pa_OpenStream(&au->out_strm,
			       NULL,
			       &pars,
			       sample_rate,
			       samples_per_buf,
			       paClipOff,
			       audio_cb_play,
			       (void *)au);
	if (pa_err)
		err = error(E_AUDIO_INIT_FAIL, "couldn't open stream");

	return err;
}

/*----------------------------------------------------------------------------
 *  The ring buffer
 *----------------------------------------------------------------------------*/

/* Initialises an audio structure's ring buffer so that decoded
 * samples can be placed into it.
 *
 * Any existing ring buffer will be freed.
 *
 * The number of bytes for each sample must be provided; see
 * audio_av_samples2bytes for one way of getting this.
 */
static enum error
init_ring_buf(struct audio *au, size_t bytes_per_sample)
{
	enum error	err = E_OK;

	/* Get rid of any existing ring buffer stuff */
	free_ring_buf(au);

	au->ring_data = calloc((size_t)RINGBUF_SIZE, bytes_per_sample);
	if (au->ring_data == NULL)
		err = error(E_NO_MEM, "couldn't alloc ringbuf data buffer");
	if (err == E_OK) {
		au->ring_buf = calloc((size_t)1, sizeof(PaUtilRingBuffer));
		if (au->ring_buf == NULL)
			err = error(E_NO_MEM, "couldn't alloc ringbuf struct");
	}
	if (err == E_OK) {
		if (PaUtil_InitializeRingBuffer(au->ring_buf,
				       (ring_buffer_size_t)bytes_per_sample,
					   (ring_buffer_size_t)RINGBUF_SIZE,
						au->ring_data) != 0)
			err = error(E_INTERNAL_ERROR, "ringbuf failed to init");
	}
	return err;
}

/* Frees an audio structure's ring buffer. */
static enum error
free_ring_buf(struct audio *au)
{
	enum error	err = E_OK;

	if (au->ring_buf != NULL) {
		debug(0, "freeing existing ringbuf");
		free(au->ring_buf);
	}
	if (au->ring_data != NULL) {
		debug(0, "freeing existing ringbuf data buffer");
		free(au->ring_data);
	}
	return err;
}
