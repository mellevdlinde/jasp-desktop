//
// Copyright (C) 2018 University of Amsterdam
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

#include "jaspimporter.h"


#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>

#include <sys/stat.h>

#include <fcntl.h>
#include "sharedmemory.h"
#include "dataset.h"

//#include "libzip/config.h"
#include "libzip/archive.h"
#include "libzip/archive_entry.h"
#include "jsonredirect.h"
#include "filereader.h"
#include "tempfiles.h"
#include "../exporters/jaspexporter.h"

#include "resultstesting/compareresults.h"
#include "log.h"

void JASPImporter::loadDataSet(DataSetPackage *packageData, const std::string &path, boost::function<void (const std::string &, int)> progressCallback)
{	
	packageData->setIsArchive(true);
	packageData->setDataSet(SharedMemory::createDataSet()); // this is required incase the loading of the data fails so that the SharedMemory::createDataSet() can be later freed.

	readManifest(packageData, path);

	Compatibility compatibility = isCompatible(packageData);

	if (compatibility == JASPImporter::NotCompatible)	throw std::runtime_error("The file version is too new.\nPlease update to the latest version of JASP to view this file.");
	else if (compatibility == JASPImporter::Limited)	packageData->setWarningMessage("This file was created by a newer version of JASP and may not have complete functionality.");

	loadDataArchive(packageData, path, progressCallback);
	loadJASPArchive(packageData, path, progressCallback);
}


void JASPImporter::loadDataArchive(DataSetPackage *packageData, const std::string &path, boost::function<void (const std::string &, int)> progressCallback)
{
	if (packageData->dataArchiveVersion().major == 1)
		loadDataArchive_1_00(packageData, path, progressCallback);
	else
		throw std::runtime_error("The file version is not supported.\nPlease update to the latest version of JASP to view this file.");
}

void JASPImporter::loadDataArchive_1_00(DataSetPackage *packageData, const std::string &path, boost::function<void (const std::string &, int)> progressCallback)
{
	bool success = false;

	Json::Value metaData;
	Json::Value xData;

	int columnCount = 0;
	int rowCount = 0;

	parseJsonEntry(metaData, path, "metadata.json", true);

	parseJsonEntry(xData, path, "xdata.json", false);

	Json::Value &dataSetDesc			= metaData["dataSet"];

	packageData->setDataFilePath(		metaData.get("dataFilePath", "").asString());
	packageData->setDataFileReadOnly(	metaData.get("dataFileReadOnly", false).asBool());
	packageData->setDataFileTimestamp(	metaData.get("dataFileTimestamp", 0).asInt());

	packageData->setDataFilter(			metaData.get("filterData", DEFAULT_FILTER).asString());

	Json::Value jsonFilterConstructor = metaData.get("filterConstructorJSON", DEFAULT_FILTER_JSON);
	packageData->setFilterConstructorJson(jsonFilterConstructor.isObject() ? jsonFilterConstructor.toStyledString() : jsonFilterConstructor.asString());
	
	Json::Value &emptyValuesJson = metaData["emptyValues"];
	if (emptyValuesJson.isNull())
	{
		// Old JASP files: the empty values were '.', 'NaN' & 'nan'
		std::vector<std::string> emptyValues;
		emptyValues.push_back("NaN");
		emptyValues.push_back("nan");
		emptyValues.push_back(".");
		Utils::setEmptyValues(emptyValues);
	}
	else
	{
		std::vector<std::string> emptyValues;
		for (Json::Value emptyValueJson  : emptyValuesJson)
			emptyValues.push_back(emptyValueJson.asString());
		Utils::setEmptyValues(emptyValues);
	}

	Json::Value &emptyValuesMapJson = dataSetDesc["emptyValuesMap"];
	packageData->resetEmptyValues();

	if (!emptyValuesMapJson.isNull())
	{
		for (Json::Value::iterator iter = emptyValuesMapJson.begin(); iter != emptyValuesMapJson.end(); ++iter)
		{
			std::string colName	= iter.key().asString();
			Json::Value mapJson	= *iter;
			std::map<int, std::string> map;

			for (Json::Value::iterator iter2 = mapJson.begin(); iter2 != mapJson.end(); ++iter2)
			{
				int row					= stoi(iter2.key().asString());
				Json::Value valueJson	= *iter2;
				std::string value		= valueJson.asString();
				map[row]				= value;
			}
			packageData->storeInEmptyValues(colName, map);
		}
	}

	columnCount = dataSetDesc["columnCount"].asInt();
	rowCount	= dataSetDesc["rowCount"].asInt();
	if (rowCount < 0 || columnCount < 0)
		throw std::runtime_error("Data size has been corrupted.");

	do
	{
		try
		{
			success				= true;
			DataSet *dataSet	= packageData->dataSet();

			dataSet->setColumnCount(columnCount);
			if (rowCount > 0)
				dataSet->setRowCount(rowCount);
		}
		catch (boost::interprocess::bad_alloc &e)
		{
			try
			{
				packageData->setDataSet(SharedMemory::enlargeDataSet(packageData->dataSet()));
				success = false;
			}
			catch(std::exception &e)	{ throw std::runtime_error("Out of memory: this data set is too large for your computer's available memory"); }
		}
		catch(std::exception e)	{ Log::log() << "n " << e.what() << std::endl;	}
		catch(...)					{ Log::log() << "something else" << std::endl;	}
	}
	while(!success);

	unsigned long long progress;
	unsigned long long lastProgress = -1;

	Json::Value &columnsDesc = dataSetDesc["fields"];
	int i = 0;
	std::map<std::string, std::map<int, int> > mapNominalTextValues;

	for (Json::Value columnDesc : columnsDesc)
	{
		std::string name					= columnDesc["name"].asString();
		Json::Value &orgStringValuesDesc	= columnDesc["orgStringValues"];
		Json::Value &labelsDesc				= columnDesc["labels"];
		std::map<int, int>& mapValues		= mapNominalTextValues[name];	// This is needed for old JASP file where factor keys where not filled in the right way

		if (labelsDesc.isNull() &&  ! xData.isNull())
		{
			Json::Value &columnlabelData = xData[name];

			if (!columnlabelData.isNull())
			{
				labelsDesc			= columnlabelData["labels"];
				orgStringValuesDesc = columnlabelData["orgStringValues"];
			}
		}

		do {
			try {
				Column &column = packageData->dataSet()->column(i);
				Column::ColumnType columnType = parseColumnType(columnDesc["measureType"].asString());

				column.setName(name);
				column.setColumnType(columnType);

				Labels &labels = column.labels();
				labels.clear();
				int index = 1;
				for (Json::Value::iterator iter = labelsDesc.begin(); iter != labelsDesc.end(); iter++)
				{
					Json::Value keyValueFilterPair = *iter;
					int zero		= 0; // ???
					int key			= keyValueFilterPair.get(zero, Json::nullValue).asInt();
					std::string val = keyValueFilterPair.get(1, Json::nullValue).asString();
					bool fil		= keyValueFilterPair.get(2, true).asBool();
					int labelValue	= key;
					if (columnType == Column::ColumnTypeNominalText)
					{
						labelValue = index;
						mapValues[key] = labelValue;
					}

					labels.add(labelValue, val, fil);

					index++;
				}

				if (!orgStringValuesDesc.isNull())
				{
					for (Json::Value keyValuePair : orgStringValuesDesc)
					{
						int zero		= 0; // ???
						int key			= keyValuePair.get(zero, Json::nullValue).asInt();
						std::string val = keyValuePair.get(1, Json::nullValue).asString();
						key = mapValues[key];
						labels.setOrgStringValues(key, val);
					}
				}
				success = true;
			}
			catch (boost::interprocess::bad_alloc &e)
			{
				try {

					packageData->setDataSet(SharedMemory::enlargeDataSet(packageData->dataSet()));
					success = false;
				}
				catch (std::exception &e)	{ throw std::runtime_error("Out of memory: this data set is too large for your computer's available memory");		}
			}
			catch (std::exception e)		{ Log::log() << "n " << e.what() << std::endl;	}
			catch (...)						{ Log::log() << "something else" << std::endl;	}
		} while (success == false);

		progress = 50 * i / columnCount;
		if (progress != lastProgress)
		{
			progressCallback("Loading Data Set", progress);
			lastProgress = progress;
		}

		i += 1;
	}

	std::string entryName = "data.bin";
	FileReader dataEntry = FileReader(path, entryName);
	if (!dataEntry.exists())
		throw std::runtime_error("Entry " + entryName + " could not be found.");

	char buff[sizeof(double) > sizeof(int) ? sizeof(double) : sizeof(int)];

	for (int c = 0; c < columnCount; c++)
	{
		Column &column					= packageData->dataSet()->column(c);
		Column::ColumnType columnType	= column.columnType();
		int typeSize					= (columnType == Column::ColumnTypeScale) ? sizeof(double) : sizeof(int);
		std::map<int, int>& mapValues	= mapNominalTextValues[column.name()];

		for (int r = 0; r < rowCount; r++)
		{
			int errorCode	= 0;
			int size		= dataEntry.readData(buff, typeSize, errorCode);

			if (errorCode != 0 || size != typeSize)
				throw std::runtime_error("Could not read 'data.bin' in JASP archive.");

			if (columnType == Column::ColumnTypeScale)
				column.setValue(r, *(double*)buff);
			else
			{
				int value = *(int*)buff;
				if (columnType == Column::ColumnTypeNominalText && value != INT_MIN)
					value = mapValues[value];
				column.setValue(r, value);
			}

			progress = 50 + (50 * ((c * rowCount) + (r + 1)) / (columnCount * rowCount));
			if (progress != lastProgress)
			{
				progressCallback("Loading Data Set", progress);
				lastProgress = progress;
			}
		}
	}
	dataEntry.close();

	if(resultXmlCompare::compareResults::theOne()->testMode())
	{
		//Read the results from when the JASP file was saved and store them in compareResults field

		FileReader	resultsEntry	= FileReader(path, "index.html");
		int			errorCode		= 0;
		std::string	html			= resultsEntry.readAllData(sizeof(char), errorCode);

		if (errorCode != 0)
			throw std::runtime_error("Could not read result from 'index.html' in JASP archive.");

		resultXmlCompare::compareResults::theOne()->setOriginalResult(QString::fromStdString(html));
	}

	packageData->computedColumnsPointer()->convertFromJson(metaData.get("computedColumns", Json::arrayValue));

	//Take out for the time being
	/*string entryName3 = "results.html";
	FileReader dataEntry3 = FileReader(path, entryName3);
	if (dataEntry3.exists())
	{
		int size1 = dataEntry3.bytesAvailable();
		char memblock1[size1];
		int startOffset1 = dataEntry3.pos();
		int errorCode = 0;
		while ((errorCode = dataEntry3.readData(&memblock1[dataEntry3.pos() - startOffset1], 8016)) > 0 ) ;
		if (errorCode < 0)
			throw runtime_error("Error reading Entry " + entryName3 + " in JASP archive.");

		packageData->analysesHTML = std::string(memblock1, size1);
		packageData->hasAnalyses = true;

		dataEntry3.close();
	}*/
}

void JASPImporter::loadJASPArchive(DataSetPackage *packageData, const std::string &path, boost::function<void (const std::string &, int)> progressCallback)
{
	if (packageData->archiveVersion().major >= 1 && packageData->archiveVersion().major <= 3) //2.x version have a different analyses.json structure but can be loaded using the 1_00 loader. 3.x adds computed columns
		loadJASPArchive_1_00(packageData, path, progressCallback);
	else
		throw std::runtime_error("The file version is not supported.\nPlease update to the latest version of JASP to view this file.");
}

void JASPImporter::loadJASPArchive_1_00(DataSetPackage *packageData, const std::string &path, boost::function<void (const std::string &, int)> progressCallback)
{
	Json::Value analysesData;

	if (parseJsonEntry(analysesData, path, "analyses.json", false))
	{
		std::vector<std::string> resources = FileReader::getEntryPaths(path, "resources");
	
		for (std::string resource : resources)
		{
			FileReader resourceEntry = FileReader(path, resource);
	
			std::string filename	= resourceEntry.fileName();
			std::string dir			= resource.substr(0, resource.length() - filename.length() - 1);
			std::string destination = TempFiles::createSpecific(dir, resourceEntry.fileName());
	
			boost::nowide::ofstream file(destination.c_str(),  std::ios::out | std::ios::binary);
	
			char	copyBuff[8016];
			int		bytes		= 0,
					errorCode	= 0;
			while ((bytes = resourceEntry.readData(copyBuff, sizeof(copyBuff), errorCode)) > 0 && errorCode == 0)
				file.write(copyBuff, bytes);

			file.flush();
			file.close();
	
			if (errorCode != 0)
				throw std::runtime_error("Could not read resource files.");
		}
	}
	
	packageData->setAnalysesData(analysesData);

}


void JASPImporter::readManifest(DataSetPackage *packageData, const std::string &path)
{
	bool foundVersion = false;
	bool foundDataVersion = false;
	std::string manifestName = "META-INF/MANIFEST.MF";
	FileReader manifest = FileReader(path, manifestName);
	int size = manifest.bytesAvailable();
	if (size > 0)
	{
		char* data = new char[size];
		int startOffset = manifest.pos();
		int errorCode = 0;
		while (manifest.readData(&data[manifest.pos() - startOffset], 8016, errorCode) > 0 && errorCode == 0) ;

		if (errorCode < 0)
			throw std::runtime_error("Error reading Entry 'manifest.mf' in JASP archive.");

		std::string doc(data, size);
		delete[] data;

		std::stringstream st(doc);
		std::string line;
		while (std::getline(st, line))
		{
			if (line.find("JASP-Archive-Version: ") == 0)
			{
				foundVersion = true;
				packageData->setArchiveVersion(Version(line.substr(22)));
			}
			else if (line.find("Data-Archive-Version: ") == 0)
			{
				foundDataVersion = true;
				packageData->setDataArchiveVersion(Version(line.substr(22)));
			}
			if (foundDataVersion && foundVersion)
				break;
		}
	}

	if ( ! foundDataVersion || ! foundVersion)
		throw std::runtime_error("Archive missing version information.");

	manifest.close();
}

bool JASPImporter::parseJsonEntry(Json::Value &root, const std::string &path,  const std::string &entry, bool required)
{
	FileReader* dataEntry = NULL;
	try
	{
		dataEntry = new FileReader(path, entry);
	}
	catch(...)
	{
		return false;
	}
	if (!dataEntry->archiveExists())
		throw std::runtime_error("The selected JASP archive '" + path + "' could not be found.");

	if (!dataEntry->exists())
	{
		if (required)
			throw std::runtime_error("Entry '" + entry + "' could not be found in JASP archive.");

		return false;
	}

	int size = dataEntry->bytesAvailable();
	if (size > 0)
	{
		char *data = new char[size];
		int startOffset = dataEntry->pos();
		int errorCode = 0;
		while (dataEntry->readData(&data[dataEntry->pos() - startOffset], 8016, errorCode) > 0 && errorCode == 0) ;

		if (errorCode < 0)
			throw std::runtime_error("Could not read Entry '" + entry + "' in JASP archive.");

		Json::Reader jsonReader;
		jsonReader.parse(data, (char*)(data + (size * sizeof(char))), root);

		delete[] data;
	}

	dataEntry->close();

	delete dataEntry;
	return true;
}

JASPImporter::Compatibility JASPImporter::isCompatible(DataSetPackage *packageData)
{
	if (packageData->archiveVersion().major > JASPExporter::jaspArchiveVersion.major || packageData->dataArchiveVersion().major > JASPExporter::dataArchiveVersion.major)
		return JASPImporter::NotCompatible;

	if (packageData->archiveVersion().minor > JASPExporter::jaspArchiveVersion.minor || packageData->dataArchiveVersion().minor > JASPExporter::dataArchiveVersion.minor)
		return JASPImporter::Limited;

	return JASPImporter::Compatible;
}

Column::ColumnType JASPImporter::parseColumnType(std::string name)
{
	if (name == "Nominal")				return  Column::ColumnTypeNominal;
	else if (name == "NominalText")		return  Column::ColumnTypeNominalText;
	else if (name == "Ordinal")			return  Column::ColumnTypeOrdinal;
	else if (name == "Continuous")		return  Column::ColumnTypeScale;
	else								return  Column::ColumnTypeUnknown;
}

