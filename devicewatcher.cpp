// Watches storage device changes
// Author: Max Schwarz <xqms@online.de>

#include "devicewatcher.h"

#include <QTimer>
#include <QFileSystemWatcher>

struct DeviceWatcherPrivate
{
	QFileSystemWatcher watcher;
	QTimer timer;
};

DeviceWatcher::DeviceWatcher(QObject* parent)
 : QObject(parent)
 , m_d(new DeviceWatcherPrivate)
{
	m_d->watcher.addPath("/media");
	m_d->timer.setInterval(1000);
	m_d->timer.setSingleShot(true);
	connect(&m_d->watcher, SIGNAL(directoryChanged(QString)), &m_d->timer, SLOT(start()));
	connect(&m_d->timer, SIGNAL(timeout()), SIGNAL(changed()));
}

DeviceWatcher::~DeviceWatcher()
{
	delete m_d;
}
