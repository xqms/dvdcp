// DVD title model
// Author: Max Schwarz <xqms@online.de>

#include "titlemodel.h"
#include "dvdreader.h"

extern "C"
{
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
}

#include <QDebug>

enum Columns
{
	COL_NUM,
	COL_NUM_CHAPTERS,
	COL_DURATION,

	NUM_COLS
};

// Copied from vobcopy which copied from lsdvd
static int dvdtime2msec(const dvd_time_t *dt)
{
	double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};
	double fps = frames_per_s[(dt->frame_u & 0xc0) >> 6];
	long   ms;
	ms  = (((dt->hour &   0xf0) >> 3) * 5 + (dt->hour   & 0x0f)) * 3600000;
	ms += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60000;
	ms += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1000;

	if(fps > 0)
	ms += ((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f) * 1000.0 / fps;

	return ms;
}

TitleModel::TitleModel(QObject* parent)
 : QAbstractTableModel(parent)
 , m_reader(0)
{
}

TitleModel::~TitleModel()
{
}

int TitleModel::rowCount(const QModelIndex& parent) const
{
	if(!m_reader)
		return 0;

	if(parent.isValid())
		return 0;

	return m_reader->ifo(0)->tt_srpt->nr_of_srpts;
}

int TitleModel::columnCount(const QModelIndex& parent) const
{
	if(parent.isValid())
		return 0;

	return NUM_COLS;
}

int TitleModel::titleNumForRow(int row) const
{
	return m_reader->ifo(0)->tt_srpt->title[row].title_set_nr;
}

double TitleModel::playbackTime(int row) const
{
	const title_info_t& title = m_reader->ifo(0)->tt_srpt->title[row];

	if(!m_reader->ifo(title.title_set_nr))
		return 0.0;

	const ifo_handle_t& ifo = *m_reader->ifo(title.title_set_nr);

	if(!ifo.vts_ptt_srpt)
		return 0.0;

	if(title.vts_ttn > ifo.vts_ptt_srpt->nr_of_srpts)
		return 0.0;

	if(!ifo.vts_ptt_srpt->title[title.vts_ttn-1].nr_of_ptts)
		return 0.0;

	int pgcn = ifo.vts_ptt_srpt->title[title.vts_ttn-1].ptt[0].pgcn;
	if(pgcn > ifo.vts_pgcit->nr_of_pgci_srp)
		return 0.0;
	const pgc_t& pgc = *ifo.vts_pgcit->pgci_srp[pgcn-1].pgc;

	return dvdtime2msec(&pgc.playback_time) / 1000.0;
}

QVariant TitleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::TextAlignmentRole)
		return (int)(Qt::AlignRight | Qt::AlignVCenter);

	if(role != Qt::DisplayRole)
		return QVariant();

	switch(section)
	{
		case COL_NUM:          return tr("#");
		case COL_NUM_CHAPTERS: return tr("#Chapters");
		case COL_DURATION:     return tr("Duration");
	}

	return QVariant();
}

QVariant TitleModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::TextAlignmentRole)
	{
		return (int)(Qt::AlignRight | Qt::AlignVCenter);
	}
	else if(role != Qt::DisplayRole)
		return QVariant();

	const title_info_t& title = m_reader->ifo(0)->tt_srpt->title[index.row()];

	switch(index.column())
	{
		case COL_NUM:
			return QString::number(index.row()+1);
		case COL_NUM_CHAPTERS:
			return QString::number(title.nr_of_ptts);
		case COL_DURATION:
			if(!m_reader->ifo(index.row()))
				return QString("-");
			int secs = playbackTime(index.row());
			return QString("%1:%2:%3")
				.arg(secs / 60 / 60, 2, 10, QChar('0'))
				.arg((secs / 60) % 60, 2, 10, QChar('0'))
				.arg(secs % 60, 2, 10, QChar('0'))
			;
	}

	return QVariant();
}

void TitleModel::setReader(DVDReader* reader)
{
	beginResetModel();

	m_reader = reader;

	endResetModel();

	emit changed();
}

int TitleModel::longestTitleRow() const
{
	int ret = -1;
	double pbTime = 0;

	for(size_t i = 1; i < m_reader->ifoCount(); ++i)
	{
		if(!m_reader->ifo(i))
			continue;

		double cur_time = playbackTime(i-1);

		if(ret == -1 || cur_time > pbTime)
		{
			pbTime = cur_time;
			ret = i;
		}
	}

	return ret-1;
}
