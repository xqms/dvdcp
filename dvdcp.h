// dvdcp - dvd copier
// Author: Max Schwarz <xqms@online.de>

#ifndef DVDCP_H
#define DVDCP_H

#include <QObject>
#include <QVector>
#include <QMap>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include "cobjectptr.h"

struct AVFormatContext;
class AudioHandler;

class DVDCP : public QObject
{
Q_OBJECT
public:
	DVDCP(QObject* parent);
	virtual ~DVDCP();

	void setReader(dvd_reader_t* reader);
	void setTitle(int title);
	void setDest(const QString& path, const QString& name);
	void setSplitSize(uint64_t split_size);

	void setAudioCodec(AVCodec* codec);
	void setAudioChannels(const QString& channelSpec);

	void setEnabledAudioStreams(const QVector<bool>& streams);
signals:
	void error(const QString& msg);
	void progress(int permil);
public slots:
	bool run();
	void cancel();
private:
	dvd_reader_t* m_reader; // owned by someone else
	DVDFilePtr m_file;
	IFOHandlePtr m_ifo;
	int m_title;
	QString m_destDir;
	QString m_name;
	uint64_t m_splitSize;
	QVector<bool> m_audioStreamEnable;
	QMap<int, AudioHandler*> m_audioMap;
	QString m_audioChannels;
	AVCodec* m_audioCodec;
	bool m_shouldStop;

	AVFormatContextPtr m_oc;
	AVFormatContextPtr m_ic;

	bool openInput();
	bool openOutput();

	bool setupVideoStream();
	bool setupAudioStreams();
};

#endif
