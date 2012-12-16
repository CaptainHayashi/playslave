#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <ao/ao.h>
#include <stdint.h>
#include "io.h"
#include "player.h"

struct player_context {
        enum player_state state;
        AVFormatContext *context;
        AVCodec *codec;
        AVStream *stream;
        AVPacket *packet;
        AVFrame *frame;
        int stream_id;
        int ao_driver_id;
        ao_option *ao_options;
        ao_device *ao_device;
        uint8_t buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
};

static void player_setup_stream(struct player_context *context);
static void player_init_ao(struct player_context *context);
static void player_setup_codec(struct player_context *player, int stream, AVCodec *codec);
static void player_play_frame(struct player_context *player);

int player_init(struct player_context **context,
                int ao_driver_id,
                ao_option *ao_options)
{
        int failure = 0;

        if (*context == NULL) {
                *context = calloc(1, sizeof(struct player_context));
                if (*context == NULL) {
                        failure = -1;
                }
        }
        if (!failure) {
                player_eject(*context);
                (*context)->ao_driver_id = ao_driver_id;
                (*context)->ao_options = ao_options;
        }

        return failure;
}

void player_eject(struct player_context *player)
{
        if (player->ao_device != NULL) {
                ao_close(player->ao_device);
                player->ao_device = NULL;
                debug(0, "closed ao device");
        }
        if (player->context != NULL) {
                avformat_close_input(&(player->context));
                player->context = NULL;
                debug(0, "closed input file");
        }
        player->state = EJECTED;
        debug(0, "player ejected");
}

void player_play(struct player_context *player)
{
        player->state = PLAYING;
}

void player_stop(struct player_context *player)
{
        player->state = STOPPED;
}

void player_load(struct player_context *player, const char *filename)
{
        player_eject(player);
        if (avformat_open_input(&(player->context),
                                filename,
                                NULL,
                                NULL) < 0) {
                error(0, filename);
        } else if (avformat_find_stream_info(player->context, NULL) < 0) {
                error(0, "couldn't find stream information");
                player_eject(player);
        } else {
                player_setup_stream(player);
        }
}

static void player_setup_stream(struct player_context *player)
{
        AVCodec *codec;
        int stream = av_find_best_stream(player->context,
                        AVMEDIA_TYPE_AUDIO,
                        -1,
                        -1,
                        &codec,
                        0);
        if (stream < 0) {
                error(2, "cannot find audio stream");
                player_eject(player);
        } else {
                player_setup_codec(player, stream, codec);
        }
}

static void player_setup_codec(struct player_context *player, int stream, AVCodec *codec)
{
        AVCodecContext *codec_context = player->context->streams[stream]->codec;
        if (avcodec_open2(codec_context, codec, NULL) < 0) {
                error(2, "cannot open codec");
                player_eject(player);
        } else {
                player->codec = codec;
                player->stream = player->context->streams[stream];
                player->stream_id = stream;
                player->packet = calloc(1, sizeof(AVPacket));
                if (player->packet == NULL) {
                        error(2, "cannot allocate packet");
                        player_eject(player);
                } else {
                        av_init_packet(player->packet);
                        player->packet->data = player->buffer;
                        player->packet->size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
                        
                        player->frame = avcodec_alloc_frame();
                        if (player->frame == NULL) {
                                error(2, "cannot allocate frame");
                                player_eject(player);
                        } else {
                                debug(0, "codec: %s", player->codec->long_name);
                                debug(0, "stream id: %u", player->stream_id);
                                debug(0, "from context: %s", player->stream->codec->codec->long_name);
                                player_init_ao(player);
                                player->state = STOPPED;
                        }
                }
        }
}

void player_update(struct player_context *player)
{
        if (player->state == PLAYING) {
                player_play_frame(player);
        }        
}

void player_shutdown(struct player_context *context)
{
        player_eject(context);
        context->state = SHUTTING_DOWN;
}

enum player_state player_state(struct player_context *context)
{
	return context->state;
}

static void player_init_ao(struct player_context *player)
{
        ao_sample_format sample_format;
         
//        AVPacket dummy_packet;
//        AVFormatContext *container = player->context;
        AVCodecContext *codec_context = player->stream->codec;
//        av_read_frame(player->context, &dummy_packet);
         
        debug(0, "%s", av_get_sample_fmt_name(codec_context->sample_fmt));
        sample_format.bits = av_get_bytes_per_sample(codec_context->sample_fmt) * 8;
        sample_format.channels = codec_context->channels;
        sample_format.rate = codec_context->sample_rate;
        sample_format.byte_format = AO_FMT_NATIVE;
        sample_format.matrix = 0;
         
        // Now seek back to the beginning of the stream
//        av_seek_frame(container, player->stream_id, 0, AVSEEK_FLAG_ANY);

        ao_device* device = ao_open_live(player->ao_driver_id,
                        &sample_format,
                        player->ao_options);
        if (device == NULL) {
                error(3, "could not open device");
                player_eject(player);
        } else {
                player->ao_device = device;
        }
}

static void player_play_frame(struct player_context *player) {
        // Read one packet into `packet`
        if (av_read_frame(player->context, player->packet) < 0) {
                player_stop(player);
                return;  // End of stream. Done decoding.
        }
        
        if (player->packet->stream_index == player->stream_id) {
                int frame_finished;
                // Decodes from `packet` into the buffer
                if (avcodec_decode_audio4(player->stream->codec,
                                        player->frame,
                                        &frame_finished,
                                        player->packet) < 0) {
                        return;  // Error in decoding
                }
 
                if (frame_finished) {
                        int buffer_size = av_samples_get_buffer_size(NULL,
                                        player->stream->codec->channels,
                                        player->frame->nb_samples,
                                        player->stream->codec->sample_fmt,
                                        1);


                        ao_play(player->ao_device,
                                        (char *)player->frame->extended_data[0],
                                        buffer_size);//player->frame->linesize[0]);
                }
        }
}
