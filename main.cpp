// dvdcp - DVD copy
// Author: Max Schwarz <xqms@online.de>

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include "mainwindow.h"

extern "C"
{
#include <libavformat/avformat.h>
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	av_register_all();
	avfilter_register_all();

	// i18n
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),
		QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator appTranslator;
	appTranslator.load("dvdcp_" + QLocale::system().name());
	app.installTranslator(&appTranslator);

	MainWindow win;
	win.show();

	return app.exec();
}
