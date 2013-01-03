// DVD source model
// Author: Max Schwarz <xqms@online.de>

#include "sourcemodel.h"

#include "devicewatcher.h"
#include "cobjectptr.h"

#include <QDir>
#include <QDebug>
#include <QIcon>
#include <dvdread/dvd_reader.h>

SourceModel::SourceModel(QObject* parent)
 : QAbstractListModel(parent)
{
	DeviceWatcher* watcher = new DeviceWatcher(this);
	connect(watcher, SIGNAL(changed()), SLOT(reload()));

	reload();
}

SourceModel::~SourceModel()
{
}

bool SourceModel::hasEntry(const QString& path)
{
	foreach(const SourceEntry& entry, m_entries)
	{
		if(entry.path == path)
			return true;
	}

	return false;
}

void SourceModel::reload()
{
	qDebug() << "SourceModel::reload()";
#ifdef WIN32
	QFileInfoList mounts = QDir::drives();
#else
	QFileInfoList mounts = QDir("/media").entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
#endif

	for(int i = 0; i < m_entries.count();)
	{
		QDir mountPoint(m_entries[i].path);
		if(!mountPoint.exists("VIDEO_TS"))
		{
			beginRemoveRows(QModelIndex(), i, i);
			m_entries.removeAt(i);
			endRemoveRows();
		}
		else
			++i;
	}

	foreach(const QFileInfo& info, mounts)
	{
		QString path = info.absoluteFilePath();
		QDir mountPoint(path);

		if(hasEntry(path))
			continue;

		if(!mountPoint.exists("VIDEO_TS"))
			continue;

		DVDReaderPtr reader;
		reader = DVDOpen(QDir::toNativeSeparators(path).toLatin1().constData());
		if(!reader)
			continue;

		char volid[32];
		DVDUDFVolumeInfo(reader, volid, sizeof(volid), NULL, 0);

		beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
		SourceEntry entry;
		entry.path = path;
		entry.title = volid;
		m_entries << entry;
		endInsertRows();
	}

	emit changed();
}

QVariant SourceModel::data(const QModelIndex& index, int role) const
{
	switch(role)
	{
		case Qt::DecorationRole:
			return QIcon::fromTheme("media-optical-dvd-video", QIcon("icons/media-optical-dvd-video.png"));
		case Qt::DisplayRole:
			const SourceEntry& entry = m_entries[index.row()];
			return QString("%1 (%2)")
				.arg(entry.title)
				.arg(QDir::toNativeSeparators(entry.path));
	}

	return QVariant();
}

int SourceModel::rowCount(const QModelIndex& parent) const
{
	return m_entries.count();
}

QString SourceModel::path(const QModelIndex& index) const
{
	return m_entries[index.row()].path;
}

