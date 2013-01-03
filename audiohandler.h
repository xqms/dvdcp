// Audio handler
// Author: Max Schwarz <xqms@online.de>

#ifndef AUDIOHANDLER_H
#define AUDIOHANDLER_H

#include <QObject>

#include "cobjectptr.h"

struct AVFormatContext;
struct AVStream;
struct AVFilterContext;

class AudioHandler : public QObject
{
Q_OBJECT
public:
	explicit AudioHandler(QObject* parent = 0);
	virtual ~AudioHandler();

	bool setupStream(AVFormatContext* ctx, AVStream* source);
	   bool handleFrame(const AVPacket& packet);
private:
	AVFormatContext* m_ctx;
	AVStream* m_sourceStream;
	AVStream* m_stream;
	AVFrame* m_frame;
	AVFrame* m_filteredFrame;
	AVFilterGraphPtr m_graph;
	AVFilterContext* m_source;
	AVFilterContext* m_sink;
};

#endif
