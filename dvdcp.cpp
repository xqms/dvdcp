// dvdcp - dvd copier
// Author: Max Schwarz <xqms@online.de>

#include "dvdcp.h"
#include "io_dvdread.h"
#include "io_split.h"
#include "audiohandler.h"

#include <QDebug>
#include <QDir>
#include <QApplication>

extern "C"
{
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>

#include "contrib/avlanguage.h"
}

int video_height[4] = {480, 576, -1, 576};
int video_width[4]  = {720, 704, 352, 352};

DVDCP::DVDCP(QObject* parent)
 : QObject(parent)
 , m_reader(0)
 , m_title(0)
 , m_destDir(".")
 , m_splitSize(0)
 , m_audioCodec(0)
{
}

DVDCP::~DVDCP()
{
}

void DVDCP::setDest(const QString& path, const QString& name)
{
	m_destDir = path;
	m_name = name;
}

void DVDCP::setReader(dvd_reader_t* reader)
{
	m_reader = reader;
}

void DVDCP::setSplitSize(uint64_t split_size)
{
	m_splitSize = split_size;
}

void DVDCP::setTitle(int title)
{
	m_title = title;
}

void DVDCP::setAudioChannels(const QString& channelSpec)
{
	m_audioChannels = channelSpec;
}

void DVDCP::setAudioCodec(AVCodec* codec)
{
	m_audioCodec = codec;
}

bool DVDCP::openInput()
{
	m_file = DVDOpenFile(m_reader, m_title, DVD_READ_TITLE_VOBS);

	if(!m_file)
	{
		error(tr("Could not open VOB file"));
		return false;
	}

	m_ic = avformat_alloc_context();
	if(!m_ic)
	{
		error(tr("Could not allocate AVFormatContext"));
		return false;
	}

	m_ic->pb = io_dvdread_create(m_file);
	m_ic->iformat = av_find_input_format("mpeg");

	if(!m_ic->pb)
	{
		error(tr("Could not create AVIO context"));
		return false;
	}

	AVFormatContext* temp = m_ic;
	int ret = avformat_open_input(&temp, "", NULL, NULL);
	if(ret != 0)
	{
		char buf[200];
		av_strerror(ret, buf, sizeof(buf));
		error(tr("Could not open input: %1").arg(buf));
		return false;
	}

	return true;
}

bool DVDCP::openOutput()
{
	QString dest;

	if(m_splitSize)
		dest = m_destDir + "/" + m_name + "/" + m_name + "_%02d.ts";
	else
		dest = m_destDir + "/" + m_name + ".ts";

	AVFormatContext* temp;
	if(avformat_alloc_output_context2(&temp, 0, "mpegts", dest.toLatin1().constData()) != 0)
	{
		error(tr("Could not allocate output context"));
		return false;
	}
	m_oc = temp;

	if(m_splitSize)
	{
		QDir(m_destDir).mkdir(m_name);
		m_oc->pb = io_split_create(dest.toLatin1().constData(), m_splitSize);
	}
	else
	{
		if(avio_open(&m_oc->pb, dest.toLatin1().constData(), AVIO_FLAG_WRITE) != 0)
		{
			error(tr("Could not open output file"));
			return false;
		}
	}

	if(!m_oc->pb)
	{
		error(tr("Could not open output file"));
		return false;
	}

	m_oc->oformat->flags |= AVFMT_TS_NONSTRICT;

	return true;
}

bool DVDCP::setupVideoStream()
{
	AVCodec* videoCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
	AVStream* videoStream = avformat_new_stream(m_oc, videoCodec);
	videoStream->id = 0x1ff;

	AVStream* inputVideoStream = m_ic->streams[0];
	avcodec_copy_context(videoStream->codec, inputVideoStream->codec);

	videoStream->time_base = inputVideoStream->time_base;
	videoStream->sample_aspect_ratio = videoStream->codec->sample_aspect_ratio;
	// MPEG-2 has time_base=1/50 and ticks_per_frame=1
	//  but the encoder expects 1/fps
	videoStream->codec->time_base = av_mul_q(
		videoStream->codec->time_base, (AVRational){videoStream->codec->ticks_per_frame, 1}
	);
	videoStream->codec->ticks_per_frame = 1;

	videoStream->codec->has_b_frames = inputVideoStream->codec->has_b_frames;
	videoStream->avg_frame_rate = inputVideoStream->avg_frame_rate;

	return true;
}

// Taken from lsdvd
const int AUDIO_ID[7] = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};

static AVStream* findStreamByID(AVFormatContext* ctx, int id)
{
	for(uint i = 0; i < ctx->nb_streams; ++i)
	{
		if(ctx->streams[i]->id == id)
			return ctx->streams[i];
	}

	return 0;
}

bool DVDCP::setupAudioStreams()
{
	for(QMap<int, AudioHandler*>::iterator it = m_audioMap.begin(); it != m_audioMap.end(); ++it)
		delete it.value();
	m_audioMap.clear();

	for(int i = 0; i < m_audioStreamEnable.count(); ++i)
	{
		if(!m_audioStreamEnable[i])
			continue;

		const audio_attr_t& attr = m_ifo->vtsi_mat->vts_audio_attr[i];
		int id = AUDIO_ID[attr.audio_format] + i;

		AVStream* stream = findStreamByID(m_ic, id);
		if(!stream)
		{
			error(tr("Could not find audio stream %1").arg(i+1));
			return false;
		}

		AudioHandler* handler = new AudioHandler();
		handler->setChannels(m_audioChannels);

		QByteArray lang(2, 0);
		lang[0] = attr.lang_code >> 8;
		lang[1] = attr.lang_code & 0xFF;
		const char* term_lang = av_convert_lang_to(lang.constData(), AV_LANG_ISO639_2_TERM);
		handler->setLanguage(term_lang);
		if(!handler->setupStream(m_oc, stream, m_audioCodec))
		{
			delete handler;
			error(tr("Could not init audio handler for stream %1").arg(i+1));
			return false;
		}

		m_audioMap[id] = handler;
	}

	return true;
}

void DVDCP::setEnabledAudioStreams(const QVector<bool>& streams)
{
	m_audioStreamEnable = streams;
}

void DVDCP::cancel()
{
	m_shouldStop = true;
}

bool DVDCP::run()
{
	m_shouldStop = false;

	m_ifo = ifoOpen(m_reader, m_title);
	if(!m_ifo)
		return false;

	if(!openInput())
		return false;

	if(!openOutput())
		return false;

	avformat_find_stream_info(m_ic, NULL);
	av_dump_format(m_ic, 0, "", 0);

	m_oc->start_time = m_ic->start_time;

	if(!setupVideoStream())
		return false;

	if(!setupAudioStreams())
		return false;

	av_dump_format(m_oc, 0, "", 1);

	avformat_write_header(m_oc, 0);

	AVPacket packet;

	int64_t pts = AV_NOPTS_VALUE;
	int64_t dts = AV_NOPTS_VALUE;
	int64_t next_pts = AV_NOPTS_VALUE;
	int64_t next_dts = AV_NOPTS_VALUE;
	bool saw_first_ts = false;
	int progress_permil = 0;

	while(!m_shouldStop && av_read_frame(m_ic, &packet) == 0)
	{
		qApp->processEvents();

		AVStream* stream = m_ic->streams[packet.stream_index];
		if(!stream->codec)
		{
			qDebug() << "No codec";
		}

		if(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if(packet.dts != AV_NOPTS_VALUE && stream->duration != AV_NOPTS_VALUE)
			{
				int permil = av_rescale(packet.dts - stream->start_time, 1000, stream->duration);
				if(permil != progress_permil)
				{
					emit progress(permil);
					progress_permil = permil;
				}
			}

			AVStream* ostream = m_oc->streams[0];

			if(!saw_first_ts)
			{
				dts = stream->avg_frame_rate.num ?
					-stream->codec->has_b_frames * AV_TIME_BASE / av_q2d(stream->avg_frame_rate) : 0;
				pts = 0;
				if(packet.pts != AV_NOPTS_VALUE)
				{
					dts += av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
					pts = dts;
				}
				saw_first_ts = true;
			}

			if(next_dts == AV_NOPTS_VALUE)
				next_dts = dts;
			if(next_pts == AV_NOPTS_VALUE)
				next_pts = pts;

			dts = next_dts;
			next_dts += av_rescale_q(packet.duration, stream->time_base, AV_TIME_BASE_Q);
			pts = dts;
			next_pts = next_dts;

			if(packet.pts != AV_NOPTS_VALUE)
				packet.pts = av_rescale_q(packet.pts, stream->time_base, ostream->time_base);
			else
				packet.pts = AV_NOPTS_VALUE;

			if(packet.dts == AV_NOPTS_VALUE)
				packet.dts = av_rescale_q(dts, AV_TIME_BASE_Q, ostream->time_base);
			else
				packet.dts = av_rescale_q(packet.dts, stream->time_base, ostream->time_base);

			pts = packet.pts + packet.duration;

			packet.stream_index = ostream->index;
			if(memcmp(&stream->time_base, &ostream->time_base, sizeof(stream->time_base)) != 0)
				qDebug() << "Different timebase on output stream" << ostream->index;

			if(av_interleaved_write_frame(m_oc, &packet) != 0)
			{
			}
		}
		else if(stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if(m_audioMap.contains(stream->id))
			{
				if(!m_audioMap[stream->id]->handleFrame(packet))
				{
					error(tr("Could not handle audio packet"));
					return false;
				}
			}
		}
	}

	av_write_trailer(m_oc);

	if(m_splitSize)
		io_split_close(m_oc->pb);
	else
		avio_close(m_oc->pb);

	qDebug() << "DVDCP run finished";

	// Return whether we were successful
	return !m_shouldStop;
}






