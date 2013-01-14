// DVD title model
// Author: Max Schwarz <xqms@online.de>

#ifndef TITLEMODEL_H
#define TITLEMODEL_H

#include <QAbstractListModel>

class DVDReader;

class TitleModel : public QAbstractTableModel
{
Q_OBJECT
public:
	explicit TitleModel(QObject* parent = 0);
	virtual ~TitleModel();

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void setReader(DVDReader* reader);

	int longestTitleRow() const;
	int titleNumForRow(int row) const;
	double playbackTime(int row) const;
signals:
	void changed();
private:
	DVDReader* m_reader;
};

#endif
