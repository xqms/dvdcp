// Watches storage device changes
// Author: Max Schwarz <xqms@online.de>

#ifndef DEVICEWATCHER_H
#define DEVICEWATCHER_H

#include <QObject>

class DeviceWatcherPrivate;

class DeviceWatcher : public QObject
{
Q_OBJECT
public:
	DeviceWatcher(QObject* parent);
	virtual ~DeviceWatcher();
signals:
	void changed();
private:
	DeviceWatcherPrivate* m_d;
};

#endif
