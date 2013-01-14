// Mainwindow
// Author: Max Schwarz <xqms@online.de>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QAbstractItemModel>
#include <memory>

#include "dvdreader.h"
#include "cobjectptr.h"

class SourceModel;
class TitleModel;
class StreamModel;
class DVDCP;
class DVDReader;

namespace Ui
{
	class MainWindow;
};

class MainWindow : public QWidget
{
Q_OBJECT
public:
	MainWindow();
	virtual ~MainWindow();

	virtual void closeEvent(QCloseEvent* );
private slots:
	void sourceSelected(const QModelIndex& index);
	void sourceListChanged();

	void titleSelected(const QModelIndex& index);
	void titleListChanged();

	void selectDirectory();
	void start();

	void error(const QString& msg);
private:
	Ui::MainWindow* m_ui;
	SourceModel* m_sourceModel;
	QPersistentModelIndex m_sourceIndex;

	TitleModel* m_titleModel;
	StreamModel* m_streamModel;
	std::unique_ptr<DVDReader> m_reader;
	DVDCP* m_cp;
	bool m_processing;

	void setSettingsEnabled(bool enabled);
};

#endif
