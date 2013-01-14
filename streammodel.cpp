// MPEG stream model
// Author: Max Schwarz <xqms@online.de>

#include "streammodel.h"
#include "dvdreader.h"

#include <QString>
#include <QLocale>

#include <dvdread/ifo_read.h>

enum Columns
{
	COL_ENABLED,
	COL_TYPE,
	COL_CODEC,
	COL_LANGUAGE,
	COL_CHANNELS,

	COL_COUNT
};

const char *mpeg_version[2] = {"mpeg1", "mpeg2"};
const char *audio_format[7] = {"ac3", "?", "mpeg1", "mpeg2", "lpcm ", "sdds ", "dts"};
const char *subp_type[16]   = {"Undefined", "Normal", "Large", "Children", "reserved", "Normal_CC", "Large_CC", "Children_CC",
        "reserved", "Forced", "reserved", "reserved", "reserved", "Director", "Large_Director", "Children_Director"};

StreamModel::StreamModel(QObject* parent)
 : QAbstractTableModel(parent)
{
}

StreamModel::~StreamModel()
{
}

int StreamModel::columnCount(const QModelIndex& parent) const
{
	if(parent.isValid())
		return 0;

	return COL_COUNT;
}

int StreamModel::rowCount(const QModelIndex& parent) const
{
	if(parent.isValid())
		return 0;

	if(!m_ifo)
		return 0;

	return m_enabledStreams.count();
}

Qt::ItemFlags StreamModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags extra = 0;

	return QAbstractTableModel::flags(index) | extra;
}

void StreamModel::setTitle(DVDReader* reader, int title)
{
	beginResetModel();

	m_reader = reader;
	int ttn = m_reader->ifo(0)->tt_srpt->title[title-1].title_set_nr;
	m_ifo = m_reader->ifo(ttn);

	if(!m_ifo)
	{
		m_reader = 0;
		m_ifo = 0;
		endResetModel();
		emit changed();
		return;
	}


	m_enabledStreams.resize(m_ifo->vtsi_mat->nr_of_vts_audio_streams + m_ifo->vtsi_mat->nr_of_vts_subp_streams + 1);
	m_enabledStreams.fill(false);
	m_enabledStreams[0] = true;

	QByteArray code = QLocale().name().left(2).toLower().toLatin1();

	// Auto-enable the first audio stream with matching language
	for(int i = 0; i < m_ifo->vtsi_mat->nr_of_vts_audio_streams; ++i)
	{
		const audio_attr_t& attr = m_ifo->vtsi_mat->vts_audio_attr[i];
		if((attr.lang_code >> 8) == code[0] && (attr.lang_code & 0xFF) == code[1])
		{
			m_enabledStreams[i+1] = true;
			break;
		}
	}

	endResetModel();

	emit changed();
}

QVariant StreamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	switch(section)
	{
		case COL_ENABLED:
			return "";
		case COL_TYPE:
			return tr("Type");
		case COL_CODEC:
			return tr("Codec");
		case COL_LANGUAGE:
			return tr("Language");
		case COL_CHANNELS:
			return tr("Channels");
	}

	return QVariant();
}

QVariant StreamModel::data(const QModelIndex& index, int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();

	int idx = index.row();

	if(index.column() == COL_ENABLED)
		return m_enabledStreams[idx];

	if(idx == 0)
	{
		switch(index.column())
		{
			case COL_TYPE:
				return tr("Video");
			case COL_CODEC:
				return tr("mpeg");
			case COL_LANGUAGE:
			case COL_CHANNELS:
				return "-";
		}

		return QVariant();
	}
	idx -= 1;

	if(idx < m_ifo->vtsi_mat->nr_of_vts_audio_streams)
	{
		const audio_attr_t& attr = m_ifo->vtsi_mat->vts_audio_attr[idx];
		switch(index.column())
		{
			case COL_TYPE:
				return tr("Audio");
			case COL_CODEC:
				return audio_format[attr.audio_format];
			case COL_LANGUAGE:
			{
				QString code;
				code.append((char)(attr.lang_code >> 8));
				code.append((char)(attr.lang_code & 0xFF));
				return code;
			}
			case COL_CHANNELS:
				if(attr.channels == 5)
					return "5.1";
				else
					return QString::number(attr.channels+1);
		}

		return QVariant();
	}
	idx -= m_ifo->vtsi_mat->nr_of_vts_audio_streams;

	if(idx < m_ifo->vtsi_mat->nr_of_vts_subp_streams)
	{
		const subp_attr_t& attr = m_ifo->vtsi_mat->vts_subp_attr[idx];
		switch(index.column())
		{
			case COL_TYPE:
				return tr("Subtitle");
			case COL_CODEC:
				return subp_type[attr.type];
			case COL_LANGUAGE:
			{
				QString code;
				code.append((char)(attr.lang_code >> 8));
				code.append((char)(attr.lang_code & 0xFF));
				return code;
			}
			case COL_CHANNELS:
				return "-";
		}
	}

	return QVariant();
}

bool StreamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(index.column() != COL_ENABLED || role != Qt::EditRole)
		return QAbstractItemModel::setData(index, value, role);

	m_enabledStreams[index.row()] = value.toBool();

	dataChanged(index, index);

	return true;
}

QVector< bool > StreamModel::enabledAudioStreams()
{
	return m_enabledStreams.mid(1, m_ifo->vtsi_mat->nr_of_vts_audio_streams);
}





