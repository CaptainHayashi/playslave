/*
 * =============================================================================
 *
 *       Filename:  audio_av.c
 *
 *    Description:  ffmpeg-specific code
 *
 *        Version:  1.0
 *        Created:  23/12/2012 01:21:49
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.york.ac.uk
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

#include <stdlib.h>

/* ffmpeg */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <portaudio.h>

#include "audio_av.h"
#include "errors.h"
#include "constants.h"
#include "io.h"

/**  DATA TYPES  **************************************************************/

struct au_in {
	AVFormatContext *context;
	AVStream       *stream;
	AVPacket       *packet;	/* Last undecoded packet */
	AVFrame        *frame;	/* Last decoded frame */
	unsigned char  *buffer;
	int		stream_id;
};

/**  STATIC PROTOTYPES  *******************************************************/

static enum error au_load_file(struct au_in *av, const char *path);
static enum error au_init_stream(struct au_in *av);
static enum error au_init_codec(struct au_in *av, int stream, AVCodec *codec);
static enum error au_init_frame(struct au_in *av);
static enum error au_init_packet(AVPacket **packet, uint8_t *buffer);
static enum error decode_packet(struct au_in *av, char **buf, size_t *n);
static enum error conv_sample_fmt(enum AVSampleFormat in, PaSampleFormat *out);
static enum error
setup_pa(PaSampleFormat sf, int device,
	 int chans, PaStreamParameters *pars);

/**  PUBLIC FUNCTIONS  ********************************************************/

/*-----------------------------------------------------------------------------
 *  Loading and unloading
 *----------------------------------------------------------------------------*/

enum error
audio_av_load(struct au_in **av, const char *path)
{
	enum error	err = E_OK;

	if (*av != NULL) {
		debug(0, "au_in structure exists, freeing");
		audio_av_unload(*av);
	}
	*av = calloc((size_t)1, sizeof(struct au_in));
	if (av == NULL)
		err = error(E_NO_MEM, "couldn't alloc au_in structure");
	if (err == E_OK) {
		(*av)->buffer = calloc(BUFFER_SIZE, sizeof(char));
		if ((*av)->buffer == NULL)
			err = error(E_NO_MEM, "couldn't alloc decode buffer");
	}
	if (err == E_OK)
		err = au_load_file(*av, path);
	if (err == E_OK)
		err = au_init_stream(*av);
	if (err == E_OK)
		err = au_init_packet(&((*av)->packet), (*av)->buffer);
	if (err == E_OK)
		err = au_init_frame(*av);
	if (err == E_OK) {
		debug(0, "stream id: %u", (*av)->stream_id);
		debug(0, "codec: %s", (*av)->stream->codec->codec->long_name);
	}
	return err;
}

void
audio_av_unload(struct au_in *av)
{
	if (av != NULL) {
		avcodec_free_frame(&(av->frame));
		if (av->packet != NULL) {
			debug(0, "freeing packet...");
			av_free_packet(av->packet);
			free(av->packet);
		}
		/* Stream is freed by closing its context */
		if (av->context != NULL) {
			avformat_close_input(&(av->context));
			av->context = NULL;
			debug(0, "closed input file");
		}
		if (av->buffer != NULL) {
			free(av->buffer);
			av->buffer = NULL;
			debug(0, "closed decode buffer");
		}
	}
}

/*----------------------------------------------------------------------------
 *  Interoperability with PortAudio
 *----------------------------------------------------------------------------*/

enum error
audio_av_pa_config(struct au_in *av,
		   int device,
		   PaStreamParameters *params,
		   size_t *samples_per_buf)
{
	PaSampleFormat	sf;
	enum error	err = E_OK;

	*samples_per_buf = audio_av_bytes2samples(av, BUFFER_SIZE);

	err = conv_sample_fmt(av->stream->codec->sample_fmt, &sf);
	if (err == E_OK)
		err = setup_pa(sf, device, av->stream->codec->channels, params);

	return err;
}

/*----------------------------------------------------------------------------
 *  Simple accessors
 *----------------------------------------------------------------------------*/

/* Returns the sample rate, providing av points to a properly initialised
 * au_in.
 */
double
audio_av_sample_rate(struct au_in *av)
{
	return (double)av->stream->codec->sample_rate;
}

/*----------------------------------------------------------------------------
 *  Converting between byte counts and sample counts
 *----------------------------------------------------------------------------*/

/* Converts buffer size (in bytes) to sample count (in samples). */
size_t
audio_av_bytes2samples(struct au_in *av, size_t bytes)
{
	return (bytes /
		av->stream->codec->channels /
		av_get_bytes_per_sample(av->stream->codec->sample_fmt));
}

/* Converts sample count (in samples) to buffer size (in bytes). */
size_t
audio_av_samples2bytes(struct au_in *av, size_t samples)
{
	return (samples *
		av->stream->codec->channels *
		av_get_bytes_per_sample(av->stream->codec->sample_fmt));
}

/*----------------------------------------------------------------------------
 *  Decoding frames
 *----------------------------------------------------------------------------*/

/* Tries to decode an entire frame and points to its contents.
 *
 * The current state in *av is used to try run ffmpeg's decoder.
 *
 * If successful, returns E_OK and sets 'buf' and 'n' to a pointer to the buffer
 * and number of bytes decoded into it respectively.
 *
 * If the return value is E_EOF, we have run out of frames to decode; any other
 * return value signifies a decode error.  Do NOT rely on 'buf' and 'n' having
 * sensible values if E_OK is not returned.
 */
enum error
audio_av_decode(struct au_in *av, char **buf, size_t *n)
{
	enum error	err = E_INCOMPLETE;

	/* Keep decoding until we hit an error or finish a frame */
	while (err == E_INCOMPLETE) {
		if (av_read_frame(av->context, av->packet) < 0) {
			err = E_EOF;
		}
		if (err == E_INCOMPLETE &&
		    av->packet->stream_index == av->stream_id) {
			err = decode_packet(av, buf, n);
		}
	}

	return err;
}

/**  STATIC FUNCTIONS  ********************************************************/

/* Converts from ffmpeg sample format to PortAudio sample format.
 *
 * The conversion is currently done straight with no attempts to
 * convert disallowed sample formats, and as such may fail with more
 * esoteric ffmpeg sample formats.
 */
static enum error
conv_sample_fmt(enum AVSampleFormat in, PaSampleFormat *out)
{
	enum error	err = E_OK;

	switch (in) {
	case AV_SAMPLE_FMT_U8:
		*out = paUInt8;
		break;
	case AV_SAMPLE_FMT_S16:
		*out = paInt16;
		break;
	case AV_SAMPLE_FMT_S32:
		*out = paInt32;
		break;
	case AV_SAMPLE_FMT_FLT:
		*out = paFloat32;
		break;
	default:
		err = error(E_BAD_FILE, "unusable sample rate");
	}

	return err;
}

/* Sets up a PortAudio parameter set ready for ffmpeg frames to be thrown at it.
 *
 * The parameter set pointed to by *params MUST already be allocated, and its
 * contents should only be used if this function returns E_OK.
 */
static enum error
setup_pa(PaSampleFormat sf, int device, int chans, PaStreamParameters *pars)
{
	enum error	err = E_OK;	/* Nothing can go wrong atm. */

	memset(pars, 0, sizeof(*pars));
	pars->channelCount = chans;
	pars->device = device;
	pars->hostApiSpecificStreamInfo = NULL;
	pars->sampleFormat = sf;
	pars->suggestedLatency = (Pa_GetDeviceInfo(device)->
				  defaultLowOutputLatency);

	return err;
}

static enum error
au_load_file(struct au_in *av, const char *path)
{
	enum error	err = E_OK;

	if (avformat_open_input(&(av->context),
				path,
				NULL,
				NULL) < 0)
		err = error(E_NO_FILE, "couldn't open %s", path);

	return err;
}

static enum error
au_init_stream(struct au_in *av)
{
	AVCodec        *codec;
	int		stream;
	enum error	err = E_OK;

	if (avformat_find_stream_info(av->context, NULL) < 0)
		err = error(E_BAD_FILE, "no stream information");
	if (err == E_OK) {
		stream = av_find_best_stream(av->context,
					     AVMEDIA_TYPE_AUDIO,
					     -1,
					     -1,
					     &codec,
					     0);
		if (stream < 0)
			err = error(E_BAD_FILE, "no audio stream in file");
	}
	if (err == E_OK)
		err = au_init_codec(av, stream, codec);

	return err;
}

static enum error
au_init_codec(struct au_in *av, int stream, AVCodec *codec)
{
	enum error	err = E_OK;

	AVCodecContext *codec_context = av->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0)
		err = error(E_BAD_FILE, "can't open codec for file");
	if (err == E_OK) {
		av->stream = av->context->streams[stream];
		av->stream_id = stream;
	}
	return err;
}

static enum error
au_init_frame(struct au_in *av)
{
	enum error	err = E_OK;

	av->frame = avcodec_alloc_frame();

	if (av->frame == NULL)
		err = error(E_NO_MEM, "can't alloc frame");

	return err;
}

static enum error
au_init_packet(AVPacket **packet, uint8_t *buffer)
{
	enum error	err = E_OK;

	*packet = calloc((size_t)1, sizeof(AVPacket));
	if (*packet == NULL)
		err = error(E_NO_MEM, "can't alloc packet");
	if (err == E_OK) {
		av_init_packet(*packet);
		(*packet)->data = buffer;
		(*packet)->size = BUFFER_SIZE;
	}
	return err;
}


/*----------------------------------------------------------------------------
 *  Decoding a frame
 *----------------------------------------------------------------------------*/

/*  Also see the non-static functions for the frontend for frame decoding */

static enum error
decode_packet(struct au_in *av, char **buf, size_t *n)
{
	enum error	err = E_OK;
	int		frame_finished = 0;

	if (avcodec_decode_audio4(av->stream->codec,
				  av->frame,
				  &frame_finished,
				  av->packet) < 0) {
		/* Decode error */
		err = error(E_BAD_FILE, "decoding error");
	}
	/* Have we decoded successfully but not finished? */
	if (err == E_OK && !frame_finished)
		err = E_INCOMPLETE;
	if (err == E_OK) {
		/* Record data that we'll use in the play loop */
		*buf = (char *)av->frame->extended_data[0];
		*n = av->frame->nb_samples;
	}
	return err;
}
