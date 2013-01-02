// DVD source model
// Author: Max Schwarz <xqms@online.de>

#ifndef SOURCEMODEL_H
#define SOURCEMODEL_H

#include <QAbstractListModel>
#include <QStringList>

class SourceModel : public QAbstractListModel
{
Q_OBJECT
public:
	explicit SourceModel(QObject* parent = 0);
	virtual ~SourceModel();

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QString path(const QModelIndex& index) const;
signals:
	void changed();
public slots:
	void reload();
private:
	QStringList m_paths;
};

#endif
