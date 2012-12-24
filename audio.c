/*
 * =============================================================================
 *
 *       Filename:  audio.c
 *
 *    Description:  Low-level audio structure
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

/**  INCLUDES  ****************************************************************/

#include <time.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <portaudio.h>

#include "contrib/pa_ringbuffer.h"

#include "io.h"
#include "player.h"
#include "audio.h"
#include "audio_av.h"

/**  MACROS  ******************************************************************/

#define RINGBUF_SIZE 32768	/* Number of samples in ring buffer */

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
};

/**  STATIC PROTOTYPES  *******************************************************/

static enum error init_sink(struct audio *au, int device);
static enum error init_ring_buf(struct audio *au, size_t bytes_per_sample);
static enum error free_ring_buf(struct audio *au);

/**  PUBLIC FUNCTIONS  ********************************************************/

/*
 * PortAudio callback prototypes
 */

static int
au_cb_play(const void *in,
	   void *out,
	   unsigned long frames_per_buf,
	   const PaStreamCallbackTimeInfo *timeInfo,
	   PaStreamCallbackFlags statusFlags,
	   void *v_au);

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
	if (err == E_OK)
		err = audio_av_load(&((*au)->av), path);
	if (err == E_OK)
		err = init_ring_buf(*au, audio_av_samples2bytes((*au)->av, 1));
	if (err == E_OK)
		err = init_sink(*au, device);

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
 *  Starting and stopping the stream
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

enum error
audio_error(struct audio *au)
{
	return au->last_err;
}

enum error
audio_decode(struct audio *au)
{
	char           *buf;
	size_t		n;
	enum error	err;

	err = audio_av_decode(au->av, &buf, &n);
	if (err == E_OK) {

	}
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
			       au_cb_play,
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

/*----------------------------------------------------------------------------
 *  Callbacks
 *----------------------------------------------------------------------------*/

static int
au_cb_play(const void *in,
	   void *out,
	   unsigned long frames_per_buf,
	   const PaStreamCallbackTimeInfo *timeInfo,
	   PaStreamCallbackFlags statusFlags,
	   void *v_au)
{
	/*
	 * TODO: This callback is morbidly obese, and a lot of its
	 * functionality would be better off running in the main thread,
	 * possibly.
	 */
	PaStreamCallbackResult result = paContinue;
	struct audio   *au = (struct audio *)v_au;
	char           *cout = (char *)out;
	unsigned long	frames_written = 0;

	/* Ignoring these arguments */
	in = (const void *)in;
	timeInfo = (const void *)timeInfo;
	statusFlags = statusFlags | 0;

	while (result == paContinue && frames_written < frames_per_buf) {
		if (au->frame_samples == 0) {
			enum error	err;

			au->last_err = err = audio_av_decode(au->av,
							   &(au->frame_ptr),
						      &(au->frame_samples));
			switch (err) {
			case E_OK:
				break;
			case E_EOF:
				result = paComplete;
				break;
			default:
				result = paAbort;
				break;
			}
		}
		if (result == paContinue) {
			size_t		bytes;
			unsigned long	samples;

			/* How many samples do we have? */
			if (au->frame_samples > frames_per_buf - frames_written)
				samples = frames_per_buf - frames_written;
			else
				samples = au->frame_samples;

			/* How many bytes does that translate into? */
			bytes = audio_av_samples2bytes(au->av, samples);
			memcpy(cout,
			       au->frame_ptr,
			       bytes);

			cout += bytes;
			au->frame_ptr += bytes;
			au->frame_samples -= samples;
			frames_written += samples;
		}
	}
	return (int)result;
}
