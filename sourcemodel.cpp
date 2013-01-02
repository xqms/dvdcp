// DVD source model
// Author: Max Schwarz <xqms@online.de>

#include "sourcemodel.h"

#include "devicewatcher.h"

#include <QDir>
#include <QDebug>
#include <QIcon>

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

void SourceModel::reload()
{
	qDebug() << "SourceModel::reload()";
#ifdef WIN32
	QFileInfoList mounts = QDir::drives();
#else
	QFileInfoList mounts = QDir("/media").entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
#endif

	for(int i = 0; i < m_paths.count();)
	{
		QDir mountPoint(m_paths[i]);
		if(!mountPoint.exists("VIDEO_TS"))
		{
			beginRemoveRows(QModelIndex(), i, i);
			m_paths.removeAt(i);
			endRemoveRows();
		}
		else
			++i;
	}

	foreach(const QFileInfo& info, mounts)
	{
		QString path = info.absoluteFilePath();
		QDir mountPoint(path);

		if(mountPoint.exists("VIDEO_TS") && !m_paths.contains(path))
		{
			beginInsertRows(QModelIndex(), m_paths.count(), m_paths.count());
			m_paths << path;
			endInsertRows();
		}
	}

	emit changed();
}

QVariant SourceModel::data(const QModelIndex& index, int role) const
{
	switch(role)
	{
		case Qt::DecorationRole:
			return QIcon::fromTheme("media-optical-dvd-video");
		case Qt::DisplayRole:
			return m_paths[index.row()];
	}

	return QVariant();
}

int SourceModel::rowCount(const QModelIndex& parent) const
{
	return m_paths.count();
}

QString SourceModel::path(const QModelIndex& index) const
{
	return m_paths[index.row()];
}

