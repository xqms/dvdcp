// DVD title model
// Author: Max Schwarz <xqms@online.de>

#ifndef TITLEMODEL_H
#define TITLEMODEL_H

#include <QAbstractListModel>
#include <vector>

#include <dvdread/ifo_types.h>

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

	void setReader(dvd_reader_s* reader);

	int longestTitleRow() const;
signals:
	void changed();
private:
	std::vector<ifo_handle_t*> m_ifos;

	double playbackTime(int ifoIndex) const;
};

#endif
