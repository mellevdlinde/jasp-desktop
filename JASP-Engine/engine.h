//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef ENGINE_H
#define ENGINE_H

#include "enginedefinitions.h"
#include "dataset.h"
#include "ipcchannel.h"
#include "processinfo.h"
#include "jsonredirect.h"

/* The Engine represents the background processes.
 * It can be in a variety of states _currentEngineState and can run analyses, filters, compute columns and Rcode.
 */

class Engine
{
public:
	explicit Engine(int slaveNo, unsigned long parentPID);
	static Engine * theEngine() { return _EngineInstance; } //There is only ever one engine in a process so we might as well have a static pointer to it.
	~Engine();

	void run();
	bool receiveMessages(int timeout = 0);
	void setSlaveNo(int no);
	void sendString(std::string message) { _channel->send(message); }

	typedef engineAnalysisStatus Status;
	Status getStatus() { return _analysisStatus; }
	analysisResultStatus getStatusToAnalysisStatus();

	//return true if changed:
	bool setColumnDataAsScale(		const std::string & columnName, const	std::vector<double>			& scalarData)												{	if(!isColumnNameOk(columnName)) return false; return provideDataSet()->columns()[columnName].overwriteDataWithScale(scalarData);				}
	bool setColumnDataAsOrdinal(	const std::string & columnName,			std::vector<int>			& ordinalData, const std::map<int, std::string> & levels)	{	if(!isColumnNameOk(columnName)) return false; return setColumnDataAsNominalOrOrdinal(true,  columnName, ordinalData, levels);					}
	bool setColumnDataAsNominal(	const std::string & columnName,			std::vector<int>			& nominalData, const std::map<int, std::string> & levels)	{	if(!isColumnNameOk(columnName)) return false; return setColumnDataAsNominalOrOrdinal(false, columnName, nominalData, levels);					}
	bool setColumnDataAsNominalText(const std::string & columnName, const	std::vector<std::string>	& nominalData)												{	if(!isColumnNameOk(columnName)) return false; return provideDataSet()->columns()[columnName].overwriteDataWithNominal(nominalData);			}

	bool isColumnNameOk(std::string columnName);

	bool setColumnDataAsNominalOrOrdinal(bool isOrdinal, const std::string & columnName, std::vector<int> & data, const std::map<int, std::string> & levels);

	int dataSetRowCount()	{ return static_cast<int>(provideDataSet()->rowCount()); }

	bool paused() { return _engineState == engineState::paused; }


private: // Methods:
	void receiveRCodeMessage(			const Json::Value & jsonRequest);
	void receiveFilterMessage(			const Json::Value & jsonRequest);
	void receiveAnalysisMessage(		const Json::Value & jsonRequest);
	void receiveComputeColumnMessage(	const Json::Value & jsonRequest);
	void receiveModuleRequestMessage(	const Json::Value & jsonRequest);
	void receiveLogCfg(					const Json::Value & jsonRequest);

	void runAnalysis();
	void runComputeColumn(	const std::string & computeColumnName,	const std::string & computeColumnCode,	Column::ColumnType computeColumnType);
	void runFilter(			const std::string & filter,				const std::string & generatedFilter,	int filterRequestId);
	void runRCode(			const std::string & rCode,				int rCodeRequestId);


	void stopEngine();
	void pauseEngine();
	void resumeEngine();
	void sendEnginePaused();
	void sendEngineResumed();
	void sendEngineStopped();

	void saveImage();
	void editImage();
	void rewriteImages();
	void removeNonKeepFiles(const Json::Value & filesToKeepValue);

	void sendAnalysisResults();
	void sendFilterResult(		int filterRequestId,				const std::vector<bool> & filterResult, const std::string & warning = "");
	void sendFilterError(		int filterRequestId,				const std::string & errorMessage);
	void sendRCodeResult(		const std::string & rCodeResult,	int rCodeRequestId);
	void sendRCodeError(		int rCodeRequestId);

	std::string callback(const std::string &results, int progress);

	DataSet *provideDataSet();

	void provideTempFileName(		const std::string &extension,	std::string &root,			std::string &relativePath);
	void provideStateFileName(		std::string &root,				std::string &relativePath);
	void provideJaspResultsFileName(std::string &root,				std::string &relativePath);

private: // Data:
	static Engine * _EngineInstance;

	Status		_analysisStatus = Status::empty;

	int			_analysisId,
				_analysisRevision,
				_progress,
				_ppi = 96,
				_slaveNo = 0;

	bool		_analysisRequiresInit,
				_analysisJaspResults,
				_currentAnalysisKnowsAboutChange;

	std::string _analysisName,
				_analysisTitle,
				_analysisDataKey,
				_analysisOptions,
				_analysisResultsMeta,
				_analysisStateKey,
				_analysisResultsString,
				_imageBackground = "white",
				_analysisRFile		= "",
				_dynamicModuleCall	= "";

	Json::Value _imageOptions,
				_analysisResults;

	IPCChannel *_channel = nullptr;

	unsigned long _parentPID = 0;

	engineState _engineState = engineState::idle;
};

#endif // ENGINE_H
