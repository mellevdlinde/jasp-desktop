//
// Copyright (C) 2013-2017 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

#include "resultsjsinterface.h"

#include <QClipboard>
#include <QStringBuilder>

#ifdef _WIN32
#include <QPainter>
#endif

#include "utilities/qutils.h"
#include "utilities/aboutmodel.h"
#include "appinfo.h"
#include "tempfiles.h"
#include <functional>
#include "timers.h"
#include "utilities/settings.h"
#include <QMimeData>
#include <QAction>
#include "gui/messageforwarder.h"
#include <QApplication>

ResultsJsInterface::ResultsJsInterface(QObject *parent) : QObject(parent)
{
	connect(this, &ResultsJsInterface::welcomeScreenIsCleared,	this, &ResultsJsInterface::welcomeScreenIsClearedHandler);
	connect(this, &ResultsJsInterface::zoomChanged,				this, &ResultsJsInterface::setZoomInWebEngine);

	setZoom(Settings::value(Settings::UI_SCALE).toDouble());
}

void ResultsJsInterface::setZoom(double zoom)
{
	if(zoom == _webEngineZoom)
		return;

	_webEngineZoom = zoom;
	emit zoomChanged();
}

void ResultsJsInterface::setZoomInWebEngine()
{
	emit runJavaScript("window.setZoom(" + QString::number(_webEngineZoom) + ")");
}


void ResultsJsInterface::resultsPageLoaded(bool succes)
{
	if (succes)
	{
		QString version = AboutModel::getJaspVersion();

		emit runJavaScript("window.setAppVersion('" + version + "')");

		setGlobalJsValues();

		emit resultsPageLoadedSignal();

		emit zoomChanged();
	}
}


void ResultsJsInterface::purgeClipboard()
{
	TempFiles::purgeClipboard();
}

void ResultsJsInterface::setExactPValuesHandler(bool exact)
{
	emit runJavaScript("window.globSet.pExact = " + QString(exact ? "true" : "false") + "; window.reRenderAnalyses();");
}

void ResultsJsInterface::setFixDecimalsHandler(QString numDecimals)
{
	if (numDecimals == "")
		numDecimals = "\"\"";
	QString js = "window.globSet.decimals = " + numDecimals + "; window.reRenderAnalyses();";
	emit runJavaScript(js);
}

void ResultsJsInterface::setGlobalJsValues()
{
	bool exactPValues = Settings::value(Settings::EXACT_PVALUES).toBool();
	QString exactPValueString = (exactPValues ? "true" : "false");
	QString numDecimals = Settings::value(Settings::NUM_DECIMALS).toString();
	QString tempFolder = "file:///" + tq(TempFiles::sessionDirName());

	QString js = "window.globSet.pExact = " + exactPValueString;
	js += "; window.globSet.decimals = " + (numDecimals.isEmpty() ? "\"\"" : numDecimals);
	js += "; window.globSet.tempFolder = \"" + tempFolder + "/\"";
	emit runJavaScript(js);
}

void ResultsJsInterface::saveTempImage(int id, QString path, QByteArray data)
{
	QByteArray byteArray = QByteArray::fromBase64(data);

	QString fullpath = tq(TempFiles::createSpecific_clipboard(fq(path)));

	QFile file(fullpath);
	file.open(QIODevice::WriteOnly);
	file.write(byteArray);
	file.close();

	QString eval = QString("window.imageSaved({ id: %1, fullPath: '%2'});").arg(id).arg(fullpath);
	emit runJavaScript(eval);
}

void ResultsJsInterface::analysisImageEditedHandler(Analysis *analysis)
{
	Json::Value imgJson = analysis->getImgResults();
	QString	results = tq(imgJson.toStyledString());
	results = escapeJavascriptString(results);
	results = "window.modifySelectedImage(" + QString::number(analysis->id()) + ", JSON.parse('" + results + "'));";
	emit runJavaScript(results);

    return;
}

void ResultsJsInterface::menuHidding()
{
	emit runJavaScript("window.analysisMenuHidden();");
}

void ResultsJsInterface::getImageInBase64(int id, const QString &path)
{
	QString fullPath = tq(TempFiles::sessionDirName()) + "/" + path;
	QFile *file = new QFile(fullPath);
	file->open(QIODevice::ReadOnly);
	QByteArray image = file->readAll();
	QString result = QString(image.toBase64());

	QString eval = QString("window.convertToBase64Done({ id: %1, result: '%2'});").arg(id).arg(result);
	emit runJavaScript(eval);

}

void ResultsJsInterface::pushToClipboard(const QString &mimeType, const QString &data, const QString &html)
{
	QMimeData *mimeData = new QMimeData();

	if (mimeType == "text/plain")
		mimeData->setText(data);

	if ( ! html.isEmpty())
		mimeData->setHtml(html);

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setMimeData(mimeData, QClipboard::Clipboard);

}

void ResultsJsInterface::pushImageToClipboard(const QByteArray &base64, const QString &html)
{
	QMimeData *mimeData = new QMimeData();

	QByteArray byteArray = QByteArray::fromBase64(base64);

	QImage pm;
	if(pm.loadFromData(byteArray))
	{
#ifdef _WIN32 //needed because jpegs/clipboard doesn't support transparency in windows
		QImage image2(pm.size(), QImage::Format_ARGB32);
		image2.fill(Qt::white);
		QPainter painter(&image2);
		painter.drawImage(0, 0, pm);

		mimeData->setImageData(image2);
#else
		mimeData->setImageData(pm);
#endif

	}

	if ( ! html.isEmpty())
		mimeData->setHtml(html);

	if (mimeData->hasImage() || mimeData->hasHtml())
	{
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setMimeData(mimeData, QClipboard::Clipboard);
	}
}


void ResultsJsInterface::displayMessageFromResults(QString msg)
{
	MessageForwarder::showWarning("Results Warning", msg);
}

void ResultsJsInterface::showAnalysis(int id)
{
	emit runJavaScript("window.select(" % QString::number(id) % ")");
}

void ResultsJsInterface::exportSelected(const QString &filename)
{
	emit runJavaScript("window.exportHTML('" + filename + "');");
}

void ResultsJsInterface::analysisChanged(Analysis *analysis)
{
	Json::Value analysisJson	= analysis->asJSON();
	analysisJson["userdata"]	= analysis->userData();
	QString results				= tq(analysisJson.toStyledString());
	results						= "window.analysisChanged(JSON.parse('" + escapeJavascriptString(results) + "'));";

	emit runJavaScript(results);
}

void ResultsJsInterface::setResultsMeta(QString str)
{
	QString results = escapeJavascriptString(str);
	results = "window.setResultsMeta(JSON.parse('" + results + "'));";
	emit runJavaScript(results);
}

void ResultsJsInterface::clearWelcomeScreen(bool callDelayedLoad)
{
	emit runJavaScript("window.clearWelcomeScreen("+QString(callDelayedLoad ? "true)" : "false)"));
}

void ResultsJsInterface::resetResults()
{
	emit resultsPageUrlChanged(_resultsPageUrl);
	setWelcomeShown(true);
}

void ResultsJsInterface::setWelcomeShown(bool welcomeShown)
{
	if (_welcomeShown == welcomeShown)
		return;

	_welcomeShown = welcomeShown;
	emit welcomeShownChanged(_welcomeShown);
}


void ResultsJsInterface::unselect()
{
	emit runJavaScript("window.unselect()");
}

void ResultsJsInterface::removeAnalysis(Analysis *analysis)
{
	emit runJavaScript("window.remove(" % QString::number(analysis->id()) % ")");
}

void ResultsJsInterface::removeAnalyses()
{
	emit runJavaScript("window.removeAllAnalyses()");
}

Json::Value &ResultsJsInterface::getResultsMeta()
{
	QEventLoop loop;

	runJavaScript("window.getResultsMeta()");
	connect(this, &ResultsJsInterface::getResultsMetaCompleted, &loop, &QEventLoop::quit);
	loop.exec();

	return _resultsMeta;
}

QVariant &ResultsJsInterface::getAllUserData()
{
	QEventLoop loop;

	runJavaScript("window.getAllUserData()");
	connect(this, &ResultsJsInterface::getAllUserDataCompleted, &loop, &QEventLoop::quit);
	loop.exec();

	return _allUserData;
}

void ResultsJsInterface::showInstruction()
{
	emit runJavaScript("window.showInstructions()");
}

void ResultsJsInterface::exportPreviewHTML()
{
	emit runJavaScript("window.exportHTML('%PREVIEW%');");
}

void ResultsJsInterface::exportHTML()
{
	emit runJavaScript("window.exportHTML('%EXPORT%');");
}

QString ResultsJsInterface::escapeJavascriptString(const QString &str)
{
	QString out;
	QRegExp rx("(\\r|\\n|\\\\|\"|\')");
	int pos = 0, lastPos = 0;

	while ((pos = rx.indexIn(str, pos)) != -1)
	{
		out += str.mid(lastPos, pos - lastPos);

		switch (rx.cap(1).at(0).unicode())
		{
		case '\r':
			out += "\\r";
			break;
		case '\n':
			out += "\\n";
			break;
		case '"':
			out += "\\\"";
			break;
		case '\'':
			out += "\\'";
			break;
		case '\\':
			out += "\\\\";
			break;
		}
		pos++;
		lastPos = pos;
	}
	out += str.mid(lastPos);
	return out;
}


void ResultsJsInterface::setResultsMetaFromJavascript(QString json)
{
	Json::Reader().parse(json.toStdString(), _resultsMeta);
	emit getResultsMetaCompleted();
}

void ResultsJsInterface::setAllUserDataFromJavascript(QString json)
{
	_allUserData = json;
	emit getAllUserDataCompleted();
}

void ResultsJsInterface::setResultsPageUrl(QString resultsPageUrl)
{
	if (_resultsPageUrl == resultsPageUrl)
		return;

	_resultsPageUrl = resultsPageUrl;
	emit resultsPageUrlChanged(_resultsPageUrl);
}
