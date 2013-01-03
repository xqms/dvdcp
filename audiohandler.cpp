// Audio handler
// Author: Max Schwarz <xqms@online.de>

#include "audiohandler.h"

#define LOG_PREFIX "[audiohandler]"
#define DEBUG 0
#include "log.h"

#include <inttypes.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/avcodec.h>
}

AVSampleFormat toNonPlanar(AVSampleFormat fmt)
{
	if(av_sample_fmt_is_planar(fmt))
		return (AVSampleFormat)(fmt - AV_SAMPLE_FMT_U8P + AV_SAMPLE_FMT_U8);
	else
		return fmt;
}

static AVCodec* findCodec(AVCodecID id, AVSampleFormat fmt)
{
// 	fmt = toNonPlanar(fmt);

	log_debug("findCodec: id %d, fmt %d", id, fmt);
	for(AVCodec* p = av_codec_next(0); p; p = av_codec_next(p))
	{
		if(p->id != id || !p->encode2)
			continue;

		for(const AVSampleFormat* f = p->sample_fmts; *f != -1; ++f)
		{
// 			int c = toNonPlanar(*f);
			log_debug("fmt: %d", *f);
			if(*f == fmt)
				return p;
		}
	}

	return 0;
}

AudioHandler::AudioHandler(QObject* parent)
 : QObject(parent)
 , m_channels("stereo")
{
}

AudioHandler::~AudioHandler()
{
}

void AudioHandler::setChannels(const QString& channels)
{
	m_channels = channels;
}

void AudioHandler::setLanguage(const QString& language)
{
	m_language = language;
}

bool AudioHandler::setupStream(AVFormatContext* ctx, AVStream* source, AVCodec* encoder)
{
	// Init decoder
	AVCodec* decoder = avcodec_find_decoder(source->codec->codec_id);
	if(avcodec_open2(source->codec, decoder, NULL) != 0)
	{
		error("Could not open audio decoder");
		return false;
	}

	char args[512];
	m_graph = avfilter_graph_alloc();

	AVFilter* filt_buffer_src = avfilter_get_by_name("abuffer");
	AVFilter* filt_buffer_sink = avfilter_get_by_name("ffabuffersink");

	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();

	/* buffer audio source: the decoded frames from the decoder will be inserted here. */
	if (!source->codec->channel_layout)
		source->codec->channel_layout = av_get_default_channel_layout(source->codec->channels);
	snprintf(args, sizeof(args),
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
		source->time_base.num, source->time_base.den, source->codec->sample_rate,
		av_get_sample_fmt_name(source->codec->sample_fmt), source->codec->channel_layout
	);

	log_debug("source args: '%s'", args);

	int ret = avfilter_graph_create_filter(&m_source, filt_buffer_src, "in",
										args, NULL, m_graph);
	if(ret < 0)
	{
		char buf[256];
		av_strerror(errno, buf, sizeof(buf));
		av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source for '%s': %s\n", args, buf);
		perror("test");
		return false;
	}

	/* buffer audio sink: to terminate the filter chain. */
	AVABufferSinkParams* abuffersink_params = av_abuffersink_params_alloc();
	abuffersink_params->sample_fmts = encoder->sample_fmts;

	ret = avfilter_graph_create_filter(
		&m_sink, filt_buffer_sink, "out",
		NULL, abuffersink_params, m_graph
	);
	av_free(abuffersink_params);

	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
		return false;
	}

	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = m_source;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name       = av_strdup("out");
	inputs->filter_ctx = m_sink;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;

	QByteArray filterString =
		QByteArray("aformat=sample_fmts\\=") + av_get_sample_fmt_name(encoder->sample_fmts[0])
			+ ":channel_layouts\\=" + m_channels.toLatin1();

	if ((ret = avfilter_graph_parse(m_graph, filterString.constData(),
	                                &inputs, &outputs, NULL)) < 0)
		return false;

	if ((ret = avfilter_graph_config(m_graph, NULL)) < 0)
		return false;

	/* Print summary of the sink buffer
		* Note: args buffer is reused to store channel layout string */
	const AVFilterLink* outlink = m_sink->inputs[0];
	av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
	av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
		(int)outlink->sample_rate,
		(char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat)outlink->format), "?"),
		args
	);

	// Init output stream
	m_ctx = ctx;
	if(!encoder)
		encoder = findCodec(source->codec->codec_id, source->codec->sample_fmt);

	if(!encoder)
	{
		error("Could not find audio encoder");
		return false;
	}


	m_stream = avformat_new_stream(ctx, encoder);
	if(!m_stream)
	{
		error("Could not open audio output stream");
		return false;
	}

	m_stream->id = source->id;
	m_stream->codec->sample_fmt = (AVSampleFormat)outlink->format;
	m_stream->codec->channel_layout = outlink->channel_layout;
	m_stream->codec->sample_rate = outlink->sample_rate;
	m_stream->codec->channels = outlink->channels;
	m_stream->codec->time_base = outlink->time_base;
	m_stream->codec->bit_rate = 192000;
	m_stream->time_base = source->time_base;
	m_stream->stream_identifier = source->stream_identifier;

	if(!m_language.isNull())
		av_dict_set(&m_stream->metadata, "language", m_language.toLatin1().constData(), 0);

	if(avcodec_open2(m_stream->codec, encoder, NULL) != 0)
	{
		error("Could not open audio output encoder");
		return false;
	}

	if(m_stream->codec->sample_rate != outlink->sample_rate)
	{
		error("Codec sample rate %d does not match filter end sample rate %d", m_stream->codec->sample_rate, outlink->sample_rate);
		return false;
	}

	av_buffersink_set_frame_size(m_sink, m_stream->codec->frame_size);

	m_filteredFrame = avcodec_alloc_frame();
	avcodec_get_frame_defaults(m_filteredFrame);

	m_frame = avcodec_alloc_frame();

	m_sourceStream = source;

	log_debug("input time base is %d/%d, input codec time base %d/%d, outlink time base %d/%d",
		source->time_base.num, source->time_base.den,
		source->codec->time_base.num, source->codec->time_base.den,
		outlink->time_base.num, outlink->time_base.den
	);

	m_stream->disposition = AV_DISPOSITION_CLEAN_EFFECTS;

	return true;
}

bool AudioHandler::handleFrame(const AVPacket& packet)
{
	avcodec_get_frame_defaults(m_frame);

	log_debug("got packet with PTS %10"PRId64, packet.pts);

	int got_frame;
	if(avcodec_decode_audio4(m_sourceStream->codec, m_frame, &got_frame, &packet) < 0)
	{
		char buf[256];
		av_strerror(errno, buf, sizeof(buf));
		error("Could not decode audio frame: %s", buf);
		perror("test");
		return false;
	}
	m_frame->pts = av_rescale_q(packet.pts, m_stream->codec->time_base, m_sourceStream->time_base);

	if(!got_frame)
		return true;

	log_debug("got audio frame with PTS %10"PRId64, m_frame->pts);
	if(av_buffersrc_add_frame(m_source, m_frame, 0) < 0)
	{
		error("Could not push frame to audio filter");
		return false;
	}

	AVFilterBufferRef *samplesref;
	while(1)
	{
		int ret = av_buffersink_get_buffer_ref(m_sink, &samplesref, 0);
		if(ret == AVERROR(EAGAIN) || ret == AVERROR(EOF))
			break;

		if(ret < 0)
			return false;

		if(!samplesref)
			continue;

		avfilter_copy_buf_props(m_filteredFrame, samplesref);
		m_filteredFrame->pts = samplesref->pts;
		log_debug("filtered frame pts: %10"PRId64, m_filteredFrame->pts);

		AVPacket packet;
		av_init_packet(&packet);
		packet.data = NULL;
		packet.size = 0;

		int got_packet = 0;
		if(avcodec_encode_audio2(m_stream->codec, &packet, m_filteredFrame, &got_packet) != 0)
		{
			error("Could not encode audio");
			av_free(packet.data);
			return false;
		}
		log_debug("encoded");

		if(got_packet)
		{
			packet.stream_index = m_stream->index;
			log_debug("writing frame, pts = %10"PRId64", dts = %10"PRId64", stream index %d", packet.pts, packet.dts, packet.stream_index);

			if(av_interleaved_write_frame(m_ctx, &packet) != 0)
			{
				av_free(packet.data);
				error("Could not write audio packet");
// 				return false;
			}
		}


		avfilter_unref_bufferp(&samplesref);
	}

	return true;
}

