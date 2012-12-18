#define _POSIX_C_SOURCE 200809

#include <time.h>
#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <ao/ao.h>

#include "io.h"
#include "player.h"
#include "audio.h"

/* Yes, I know this is wrong */
#define NANOS_IN_SEC 900000000L
#define BUFFER_SIZE FF_MIN_BUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE

struct audio {
	AVFormatContext *context;
	AVCodec        *codec;
	AVStream       *stream;
	AVPacket       *packet;
	AVFrame        *frame;
	int		stream_id;
	int		ao_driver_id;
	ao_option      *ao_options;
	ao_device      *ao_device;
	uint8_t		buffer [BUFFER_SIZE];
	struct timespec frame_dur;
	int frame_finished;
	int current_pos;
	int buf_usage;
};

static enum audio_init_err audio_init_stream(struct audio *au);
static enum audio_init_err audio_init_packet(AVPacket **packet, uint8_t * buffer);
static enum audio_init_err audio_init_ao(struct audio *au);
static enum audio_init_err audio_init_codec(struct audio *au, int stream, AVCodec *codec);
static enum audio_play_err audio_handle_frame(struct audio *au);


enum audio_init_err
audio_load(struct audio **au,
	   const char *filename,
	   int ao_driver_id,
	   ao_option *ao_options)
{
	int		result = E_AINIT_OK;

	if (*au != NULL) {
		debug(0, "Audio structure exists, freeing");
		audio_unload(*au);
	}
	*au = calloc(1, sizeof(struct audio));
	if (*au == NULL)
		result = E_AINIT_CANNOT_ALLOC_AUDIO;
	if (result == E_AINIT_OK) {
		(*au)->ao_driver_id = ao_driver_id;
		(*au)->ao_options = ao_options;

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
		result = audio_init_stream(*au);
	}
	return result;
}

enum audio_play_err
audio_play_frame(struct audio *au)
{
	enum audio_play_err result = E_PLAY_OK;

	if (au->current_pos == au->buf_usage) {
	    au->frame_finished = 0;		

	    if (av_read_frame(au->context, au->packet) < 0)
		    result = E_PLAY_EOF;
	    if (result == E_PLAY_OK) {
		    if (au->packet->stream_index == au->stream_id) {
			    result = audio_handle_frame(au);
		    }
	    }
	}
	if (result == E_PLAY_OK && au->frame_finished) {

	    int bs = (au->buf_usage - au->current_pos);// % 1024;
	    if (bs == 0)
		bs = 1024;
	    char* ptr = (char *)au->frame->extended_data[0];

	    ao_play(au->ao_device,
		ptr + au->current_pos,
			bs);
	    au->current_pos += bs;

       struct timespec tim2;
	    nanosleep(&au->frame_dur, &tim2);
	}
	return result;
}

void
audio_unload(struct audio *au)
{
	if (au->ao_device != NULL) {
		ao_close(au->ao_device);
		au->ao_device = NULL;
		debug(0, "closed ao device");
	}
	if (au->context != NULL) {
		avformat_close_input(&(au->context));
		au->context = NULL;
		debug(0, "closed input file");
	}
}

/* STATIC FUNCTIONS */

static enum audio_init_err
audio_init_stream(struct audio *au)
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
		result = audio_init_codec(au, stream, codec);
	if (result == E_AINIT_OK)
		result = audio_init_ao(au);

	return result;
}

static enum audio_init_err
audio_init_codec(struct audio *au, int stream, AVCodec *codec)
{
	enum audio_init_err result = E_AINIT_OK;

	AVCodecContext *codec_context = au->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0)
		result = E_AINIT_NO_CODEC;
	if (result == E_AINIT_OK) {
		au->codec = codec;
		au->stream = au->context->streams[stream];
		au->stream_id = stream;
		result = audio_init_packet(&(au->packet), au->buffer);
	}
	if (result == E_AINIT_OK) {
		au->frame = avcodec_alloc_frame();

		if (au->frame == NULL)
			result = E_AINIT_CANNOT_ALLOC_FRAME;
	}
	if (result == E_AINIT_OK) {
		debug(0, "codec: %s", au->codec->long_name);
		debug(0, "stream id: %u", au->stream_id);
		debug(0, "from ctx: %s", au->stream->codec->codec->long_name);
	}
	return result;
}

static enum audio_init_err
audio_init_packet(AVPacket **packet, uint8_t * buffer)
{
	enum audio_init_err result = E_AINIT_OK;

	*packet = calloc(1, sizeof(AVPacket));
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
audio_init_ao(struct audio *au)
{
	enum audio_init_err result = E_AINIT_OK;

	ao_sample_format sample_format;

	AVCodecContext *codec = au->stream->codec;

	debug(0, "%s", av_get_sample_fmt_name(codec->sample_fmt));
	sample_format.bits = av_get_bytes_per_sample(codec->sample_fmt) * 8;
	sample_format.channels = codec->channels;
	sample_format.rate = codec->sample_rate;
	sample_format.byte_format = AO_FMT_NATIVE;
	sample_format.matrix = 0;

	au->ao_device = ao_open_live(au->ao_driver_id,
				     &sample_format,
				     au->ao_options);
	if (au->ao_device == NULL)
		result = E_AINIT_DEVICE_OPEN_FAIL;

	return result;
}

static enum audio_play_err
audio_handle_frame(struct audio *au)
{
	enum audio_play_err result = E_PLAY_OK;

	if (avcodec_decode_audio4(au->stream->codec,
				  au->frame,
				  &(au->frame_finished),
				  au->packet) < 0)
		result = E_PLAY_DECODE_ERR;
	if (result == E_PLAY_OK && au->frame_finished) {
	    int64_t frame_ns;
	    frame_ns = (au->packet->duration * NANOS_IN_SEC * au->stream->time_base.num)
		    / au->stream->time_base.den;
	    au->frame_dur.tv_nsec = frame_ns % NANOS_IN_SEC;
	    au->frame_dur.tv_sec = (frame_ns - au->frame_dur.tv_nsec) / NANOS_IN_SEC;

		au->buf_usage = av_samples_get_buffer_size(NULL,
						au->stream->codec->channels,
						      au->frame->nb_samples,
					      au->stream->codec->sample_fmt,
						      1);
		au->current_pos = 0;
	}
	return result;
}
