/*-
 * audio.c - low-level audio structure
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

#include <time.h>
#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <portaudio.h>

#include "io.h"
#include "player.h"
#include "audio.h"

#define BUFFER_SIZE FF_MIN_BUFFER_SIZE

struct audio {
	enum audio_play_err last_err;
	/* libavformat state */
	AVFormatContext *context;
	AVStream       *stream;
	AVPacket       *packet;
	AVFrame        *frame;
	int		stream_id;
	/* shared state */
	uint8_t		buffer [BUFFER_SIZE];
	int		frame_finished;
	char           *frame_ptr;
	unsigned long	num_samples;
	/* PortAudio state */
	PaStream       *out_strm;	/* Output stream */
	int		device_id;	/* PortAudio device ID */
};

static enum audio_init_err au_init_stream(struct audio *au);
static enum audio_init_err au_init_packet(AVPacket **packet, uint8_t *buffer);
static enum audio_init_err au_init_sink(struct audio *au);
static enum audio_init_err au_init_codec(struct audio *au, int stream, AVCodec *codec);
static PaStreamCallbackResult au_handle_frame(struct audio *au);

enum audio_init_err
audio_load(struct audio **au,
	   const char *filename,
	   int device_id)
{
	enum audio_init_err result = E_AINIT_OK;

	if (*au != NULL) {
		debug(0, "Audio structure exists, freeing");
		audio_unload(*au);
	}
	*au = calloc((size_t)1, sizeof(struct audio));
	if (*au == NULL)
		result = E_AINIT_CANNOT_ALLOC_AUDIO;
	if (result == E_AINIT_OK) {
		(*au)->device_id = device_id;

		if (avformat_open_input(&((*au)->context),
					filename,
					NULL,
					NULL) < 0)
			result = E_AINIT_OPEN_INPUT;
	}
	if (result == E_AINIT_OK) {
		if (avformat_find_stream_info((*au)->context, NULL) < 0)
			result = E_AINIT_FIND_STREAM_INFO;
	}
	if (result == E_AINIT_OK) {
		result = au_init_stream(*au);
	}
	return result;
}

enum error
audio_start(struct audio *au)
{
	PaError		err;
	enum error	result = E_OK;

	err = Pa_StartStream(au->out_strm);
	if (err)
		result = E_INTERNAL_ERROR;
	else
		debug(0, "audio started");

	return result;
}

enum error
audio_stop(struct audio *au)
{
	PaError		err;
	enum error	result = E_OK;

	err = Pa_AbortStream(au->out_strm);
	if (err)
		result = E_INTERNAL_ERROR;
	else
		debug(0, "audio stopped");

	return result;
}

static int
au_play_frame(const void *in,
		 void *out,
		 unsigned long frames_per_buf,
		 const PaStreamCallbackTimeInfo *timeInfo,
		 PaStreamCallbackFlags statusFlags,
		 void *v_au)
{
	PaStreamCallbackResult result = paContinue;
	struct audio   *au = (struct audio *)v_au;
	char           *cout = (char *)out;
	unsigned long	frames_written = 0;

	in = (const void *)in;	/* Allow a safe ignore */
	timeInfo = (const void *)timeInfo;	/* And here */
	statusFlags = statusFlags | 0;	/* Also here */


	while (result == paContinue && frames_written < frames_per_buf) {
		if (au->num_samples == 0) {
			/* We need to decode more samples */

			au->frame_finished = 0;
			while (result == paContinue && !au->frame_finished) {
				if (av_read_frame(au->context, au->packet) < 0) {
					au->last_err = E_PLAY_EOF;
					result = paComplete;
				}
				if (result == paContinue &&
				au->packet->stream_index == au->stream_id) {
					result = au_handle_frame(au);
				}
			}

		}
		if (result == paContinue && au->frame_finished) {
			size_t		bytes;
			unsigned long	num_to_get;

			/* How many samples do we have? */
			if (au->num_samples > frames_per_buf - frames_written)
				num_to_get = frames_per_buf - frames_written;
			else
				num_to_get = au->num_samples;

			/* How many bytes does that translate into? */
			bytes = (num_to_get
				 * au->stream->codec->channels
				 * av_get_bytes_per_sample(au->stream->codec->sample_fmt));
			memcpy(cout,
			       au->frame_ptr,
			       bytes);
			cout += bytes;
			au->frame_ptr += bytes;
			frames_written += num_to_get;
			au->num_samples -= num_to_get;
		}
	}
	return (int)result;
}

void
audio_unload(struct audio *au)
{
	if (au->out_strm != NULL) {
		Pa_CloseStream(au->out_strm);
		au->out_strm = NULL;
		debug(0, "closed output stream");
	}
	if (au->context != NULL) {
		avformat_close_input(&(au->context));
		au->context = NULL;
		debug(0, "closed input file");
	}
}

/* STATIC FUNCTIONS */

static enum audio_init_err
au_init_stream(struct audio *au)
{
	AVCodec        *codec;
	int		stream;
	enum audio_init_err result = E_AINIT_OK;

	stream = av_find_best_stream(au->context,
				     AVMEDIA_TYPE_AUDIO,
				     -1,
				     -1,
				     &codec,
				     0);
	if (stream < 0)
		result = E_AINIT_NO_STREAM;

	if (result == E_AINIT_OK)
		result = au_init_codec(au, stream, codec);
	if (result == E_AINIT_OK)
		result = au_init_sink(au);

	return result;
}

static enum audio_init_err
au_init_codec(struct audio *au, int stream, AVCodec *codec)
{
	enum audio_init_err result = E_AINIT_OK;

	AVCodecContext *codec_context = au->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0)
		result = E_AINIT_NO_CODEC;
	if (result == E_AINIT_OK) {
		au->stream = au->context->streams[stream];
		au->stream_id = stream;
		result = au_init_packet(&(au->packet), au->buffer);
	}
	if (result == E_AINIT_OK) {
		au->frame = avcodec_alloc_frame();

		if (au->frame == NULL)
			result = E_AINIT_CANNOT_ALLOC_FRAME;
	}
	if (result == E_AINIT_OK) {
		debug(0, "stream id: %u", au->stream_id);
		debug(0, "codec: %s", au->stream->codec->codec->long_name);
	}
	return result;
}

static enum audio_init_err
au_init_packet(AVPacket **packet, uint8_t *buffer)
{
	enum audio_init_err result = E_AINIT_OK;

	*packet = calloc((size_t)1, sizeof(AVPacket));
	if (*packet == NULL)
		result = E_AINIT_CANNOT_ALLOC_PACKET;
	if (result == E_AINIT_OK) {
		av_init_packet(*packet);
		(*packet)->data = buffer;
		(*packet)->size = BUFFER_SIZE;
	}
	return result;
}

static enum audio_init_err
au_init_sink(struct audio *au)
{
	enum audio_init_err result = E_AINIT_OK;
	AVCodecContext *codec = au->stream->codec;
	int		dv = au->device_id;

	PaStreamParameters pars;

	/*
	 * Sample format conversion from libavformat's understanding
	 * of it to portaudio's... this isn't perfect, but should
	 * hopefully cover most cases.
	 */
	PaSampleFormat	sf;
	int		bytes_per_sample;
	switch (codec->sample_fmt) {
	case AV_SAMPLE_FMT_U8:
		sf = paUInt8;
		bytes_per_sample = 1;
		break;
	case AV_SAMPLE_FMT_S16:
		sf = paInt16;
		bytes_per_sample = 2;
		break;
	case AV_SAMPLE_FMT_S32:
		sf = paInt32;
		bytes_per_sample = 4;
		break;
	case AV_SAMPLE_FMT_FLT:
		sf = paFloat32;
		bytes_per_sample = 4;
		break;
	default:
		result = E_AINIT_BAD_RATE;
	}
	if (result == E_AINIT_OK) {
		unsigned long	frames_per_buf;
		PaError		err;

		frames_per_buf = (BUFFER_SIZE / bytes_per_sample) / codec->channels;

		memset(&pars, 0, sizeof(pars));
		pars.channelCount = codec->channels;
		pars.device = dv;
		pars.hostApiSpecificStreamInfo = NULL;
		pars.sampleFormat = sf;
		pars.suggestedLatency = Pa_GetDeviceInfo(dv)->defaultLowOutputLatency;

		err = Pa_OpenStream(&au->out_strm,
				    NULL,
				    &pars,
				    (double)codec->sample_rate,
				    frames_per_buf,
				    paClipOff,
				    au_play_frame,
				    (void *)au);
		if (err)
			result = E_AINIT_DEVICE_OPEN_FAIL;
	}
	return result;
}

static PaStreamCallbackResult
au_handle_frame(struct audio *au)
{
	PaStreamCallbackResult result = paContinue;

	if (avcodec_decode_audio4(au->stream->codec,
				  au->frame,
				  &(au->frame_finished),
				  au->packet) < 0) {
		result = paAbort;
		au->last_err = E_PLAY_DECODE_ERR;
	}
	if (result == paContinue && au->frame_finished) {
		/* Record data that we'll use in the play loop */
		au->frame_ptr = (char *)au->frame->extended_data[0];
		au->num_samples = au->frame->nb_samples;
	}
	return result;
}
