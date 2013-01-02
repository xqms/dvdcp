// MPEG stream model
// Author: Max Schwarz <xqms@online.de>

#ifndef STREAMMODEL_H
#define STREAMMODEL_H

#include <QAbstractTableModel>

#include "cobjectptr.h"

class StreamModel : public QAbstractTableModel
{
Q_OBJECT
public:
	explicit StreamModel(QObject* parent);
	virtual ~StreamModel();

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;

	void setTitle(dvd_reader_t* reader, int title);
	bool videoEnabled();
	QVector<bool> enabledAudioStreams();
	QVector<bool> enabledSubtitleStreams();
signals:
	void changed();
private:
	IFOHandlePtr m_ifo;
	QVector<bool> m_enabledStreams;
};

#endif
