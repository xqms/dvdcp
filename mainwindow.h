// Mainwindow
// Author: Max Schwarz <xqms@online.de>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QAbstractItemModel>

#include "cobjectptr.h"

class SourceModel;
class TitleModel;
class StreamModel;
class DVDCP;
struct dvd_reader_s;
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
	DVDReaderPtr m_reader;
	DVDCP* m_cp;

	void setSettingsEnabled(bool enabled);
};

#endif
