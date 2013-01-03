// Mainwindow
// Author: Max Schwarz <xqms@online.de>

#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "sourcemodel.h"
#include "titlemodel.h"
#include "streammodel.h"
#include "dvdcp.h"
#include "checkboxdelegate.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_types.h>
#include <dvdread/nav_read.h>

MainWindow::MainWindow()
 : QWidget()
 , m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	m_streamModel = new StreamModel(this);
	m_ui->streamView->setModel(m_streamModel);
	m_ui->streamView->setItemDelegateForColumn(0, new CheckBoxDelegate(this));

	m_titleModel = new TitleModel(this);
	m_ui->titleView->setModel(m_titleModel);
	connect(m_ui->titleView, SIGNAL(activated(QModelIndex)), SLOT(titleSelected(QModelIndex)));
	connect(m_titleModel, SIGNAL(changed()), SLOT(titleListChanged()));

	m_sourceModel = new SourceModel(this);
	m_ui->sourceView->setModel(m_sourceModel);
	connect(m_ui->sourceView, SIGNAL(activated(QModelIndex)), SLOT(sourceSelected(QModelIndex)));
	connect(m_sourceModel, SIGNAL(changed()), SLOT(sourceListChanged()));
	sourceListChanged();

	connect(m_ui->dirButton, SIGNAL(clicked(bool)), SLOT(selectDirectory()));
	connect(m_ui->startButton, SIGNAL(clicked(bool)), SLOT(start()));

	QSettings settings("de.x-quadraht", "dvdcp");
	m_ui->dirEdit->setText(settings.value("destDir").toString());

	m_ui->splitSpinBox->setValue(settings.value("splitSize", 4096).toInt());
	bool split = settings.value("splitEnabled", false).toBool();
	m_ui->splitCheckBox->setChecked(split);
	m_ui->splitSpinBox->setEnabled(split);

	m_cp = new DVDCP(this);
	connect(m_cp, SIGNAL(error(QString)), SLOT(error(QString)));
	connect(m_cp, SIGNAL(progress(int)), m_ui->progressBar, SLOT(setValue(int)));
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::sourceSelected(const QModelIndex& index)
{
	QString path = QDir::toNativeSeparators(m_sourceModel->path(index));

	qDebug() << "Opening" << path;

	m_reader = DVDOpen(path.toLatin1().constData());
	if(!m_reader)
	{
		QMessageBox::critical(this, tr("Error"), tr("Could not open DVD"));
		m_ui->sourceView->setCurrentIndex(QModelIndex());
		return;
	}

	char volid[32];
	DVDUDFVolumeInfo(m_reader(), volid, sizeof(volid), NULL, 0);

	qDebug() << "DVD opened:" << volid;

	m_titleModel->setReader(m_reader());

	m_ui->nameEdit->setText(volid);

	m_sourceIndex = index;
}

void MainWindow::titleListChanged()
{
	int row = m_titleModel->longestTitleRow();

	QModelIndex titleIndex = m_titleModel->index(row, 0);
	m_ui->titleView->setCurrentIndex(titleIndex);
	titleSelected(titleIndex);

	for(int i = 0; i < m_titleModel->columnCount(); ++i)
		m_ui->titleView->resizeColumnToContents(i);
}

void MainWindow::sourceListChanged()
{
	if(m_sourceIndex.isValid())
		return; // all is well

	if(m_sourceModel->rowCount() == 0)
	{
		m_titleModel->setReader(0);
		return;
	}

	QModelIndex sourceIndex = m_sourceModel->index(0, 0);
	m_ui->sourceView->setCurrentIndex(sourceIndex);
	sourceSelected(sourceIndex);
}


void MainWindow::titleSelected(const QModelIndex& index)
{
	if(index.isValid())
	{
		Q_ASSERT(m_reader);
		m_streamModel->setTitle(m_reader(), index.row()+1);
		m_ui->streamView->resizeColumnsToContents();
	}
	else
		m_streamModel->setTitle(0, 0);
}

void MainWindow::selectDirectory()
{
	QString path = QFileDialog::getExistingDirectory(this,
		tr("Select destination directory"),
		m_ui->dirEdit->text()
	);

	if(path.isNull())
		return;

	m_ui->dirEdit->setText(path);
}

void MainWindow::closeEvent(QCloseEvent* ev)
{
	QSettings settings("de.x-quadraht", "dvdcp");
	settings.setValue("destDir", m_ui->dirEdit->text());
	settings.setValue("splitSize", m_ui->splitSpinBox->value());
	settings.setValue("splitEnabled", m_ui->splitCheckBox->isChecked());
	QWidget::closeEvent(ev);
}

void MainWindow::start()
{
	m_ui->progressBar->setValue(0);
	m_cp->setDest(m_ui->dirEdit->text(), m_ui->nameEdit->text());
	m_cp->setReader(m_reader());
	m_cp->setSplitSize(m_ui->splitCheckBox->isChecked() ? 1024LL * 1024LL * m_ui->splitSpinBox->value() : 0);
	m_cp->setTitle(m_ui->titleView->currentIndex().row()+1);
	m_cp->setEnabledAudioStreams(m_streamModel->enabledAudioStreams());

	m_ui->sourceBox->setEnabled(false);
	m_ui->destinationBox->setEnabled(false);

	m_ui->startButton->setText(tr("Cancel"));
	disconnect(m_ui->startButton, SIGNAL(clicked(bool)), this, SLOT(start()));
	connect(m_ui->startButton, SIGNAL(clicked(bool)), m_cp, SLOT(cancel()));

	bool success = m_cp->run();

	m_ui->sourceBox->setEnabled(true);
	m_ui->destinationBox->setEnabled(true);

	if(!success)
		m_ui->progressBar->setValue(0);
	else
	{
		m_ui->progressBar->setValue(1000);
		QMessageBox::information(this, tr("Process finished"), tr("Title extracted successfully."));
	}

	m_ui->startButton->setText(tr("Start"));
	disconnect(m_ui->startButton, SIGNAL(clicked(bool)), m_cp, SLOT(cancel()));
	connect(m_ui->startButton, SIGNAL(clicked(bool)), SLOT(start()));
}

void MainWindow::error(const QString& msg)
{
	QMessageBox::critical(this, tr("Error"), tr("There was an error during the copy operation: %1").arg(msg));
}

