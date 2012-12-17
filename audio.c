#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <ao/ao.h>

#include "io.h"
#include "player.h"
#include "audio.h"

#define BUFFER_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE

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
};

static enum audio_init_err audio_init_stream(struct audio *au);
static enum audio_init_err audio_init_ao(struct audio *au);
static enum audio_init_err audio_init_codec(struct audio *au, int stream, AVCodec * codec);

enum audio_init_err
audio_load(struct audio **au,
	   const char *filename,
	   int ao_driver_id,
	   ao_option * ao_options)
{
	int		result;

	if (*au != NULL) {
		debug(0, "Audio structure exists, freeing");
		audio_unload(*au);
	}
	*au = calloc(1, sizeof(struct audio));
	if (*au == NULL) {
		result = E_AINIT_CANNOT_ALLOC_AUDIO;
	} else {
		(*au)->ao_driver_id = ao_driver_id;
		(*au)->ao_options = ao_options;

		if (avformat_open_input(&((*au)->context),
					filename,
					NULL,
					NULL) < 0) {
			result = E_AINIT_OPEN_INPUT;
		} else if (avformat_find_stream_info((*au)->context, NULL) < 0) {
			result = E_AINIT_FIND_STREAM_INFO;
		} else {
			result = audio_init_stream(*au);
		}

	}
	return result;
}

enum audio_play_err
audio_play_frame(struct audio *au)
{
	enum audio_play_err result;

	if (av_read_frame(au->context, au->packet) < 0) {
		result = E_PLAY_EOF;
	} else if (au->packet->stream_index == au->stream_id) {
		int		frame_finished;

		if (avcodec_decode_audio4(au->stream->codec,
					  au->frame,
					  &frame_finished,
					  au->packet) < 0) {
			result = E_PLAY_DECODE_ERR;
		} else if (frame_finished) {
			int		buffer_size;

			buffer_size = av_samples_get_buffer_size(NULL,
						au->stream->codec->channels,
						      au->frame->nb_samples,
					      au->stream->codec->sample_fmt,
								 1);

			ao_play(au->ao_device,
				(char *)au->frame->extended_data[0],
				buffer_size);
			result = E_PLAY_OK;
		}
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
	int		result;
	AVCodec        *codec;
	int		stream = av_find_best_stream(au->context,
					  AVMEDIA_TYPE_AUDIO,
					  -1,
					  -1,
					  &codec,
					  0);
	if (stream < 0) {
		result = E_AINIT_NO_STREAM;
	} else {
		result = audio_init_codec(au, stream, codec);
	}

	return result;
}

static enum audio_init_err
audio_init_codec(struct audio *au, int stream, AVCodec * codec)
{
	int		result;

	AVCodecContext *codec_context = au->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		result = E_AINIT_NO_CODEC;
	} else {
		au->codec = codec;
		au->stream = au->context->streams[stream];
		au->stream_id = stream;
		au->packet = calloc(1, sizeof(AVPacket));
		if (au->packet == NULL) {
			result = E_AINIT_CANNOT_ALLOC_PACKET;
		} else {
			av_init_packet(au->packet);
			au->packet->data = au->buffer;
			au->packet->size = BUFFER_SIZE;

			au->frame = avcodec_alloc_frame();
			if (au->frame == NULL) {
				result = E_AINIT_CANNOT_ALLOC_FRAME;
			} else {
				debug(0, "codec: %s", au->codec->long_name);
				debug(0, "stream id: %u", au->stream_id);
				debug(0, "from context: %s", au->stream->codec->codec->long_name);
				result = audio_init_ao(au);
			}
		}
	}

	return result;
}

static enum audio_init_err
audio_init_ao(struct audio *au)
{
	int		result = E_AINIT_OK;

	ao_sample_format sample_format;

	AVCodecContext *codec_context = au->stream->codec;

	debug(0, "%s", av_get_sample_fmt_name(codec_context->sample_fmt));
	sample_format.bits = av_get_bytes_per_sample(codec_context->sample_fmt) * 8;
	sample_format.channels = codec_context->channels;
	sample_format.rate = codec_context->sample_rate;
	sample_format.byte_format = AO_FMT_NATIVE;
	sample_format.matrix = 0;

	ao_device      *device = ao_open_live(au->ao_driver_id,
					      &sample_format,
					      au->ao_options);
	if (device == NULL) {
		result = E_AINIT_DEVICE_OPEN_FAIL;
	} else {
		au->ao_device = device;
	}

	return result;
}
