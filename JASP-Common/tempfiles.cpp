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

#include "tempfiles.h"

#include <sstream>
#include <boost/filesystem.hpp>

#include "boost/nowide/fstream.hpp"

#include "base64.h"
#include "utils.h"

#include "dirs.h"
#include "log.h"
using namespace std;
using namespace boost;

long					TempFiles::_sessionId		= 0;
std::string				TempFiles::_sessionDirName	= "";
std::string				TempFiles::_statusFileName	= "";
std::string				TempFiles::_clipboard		= "";
int						TempFiles::_nextFileId		= 0;
int						TempFiles::_nextTmpFolderId	= 0;
TempFiles::stringvec	TempFiles::_shmemNames		= TempFiles::stringvec();

void TempFiles::init(long sessionId)
{
	system::error_code error;

	_sessionId		= sessionId;
	_nextFileId		= 0;
	_sessionDirName	= Dirs::tempDir() + "/" + std::to_string(sessionId);
	_statusFileName	= _sessionDirName +  "/status";
	_clipboard		= Dirs::tempDir() + "/clipboard";

	filesystem::path sessionPath = Utils::osPath(_sessionDirName);

	filesystem::remove_all(sessionPath, error);
	filesystem::create_directories(sessionPath, error);

	nowide::fstream f;
	f.open(_statusFileName.c_str(), ios_base::out);
	f.close();

	//filesystem::path clipboardPath = Utils::osPath(clipboard);
	//if ( ! filesystem::exists(clipboardPath, error))
	//	filesystem::create_directories(clipboardPath, error);
}

void TempFiles::attach(long sessionId)
{
	_sessionId		= sessionId;
	_nextFileId		= 0;
	_sessionDirName	= Dirs::tempDir() + "/" + std::to_string(sessionId);
	_statusFileName	= _sessionDirName + "/status";
}


void TempFiles::deleteAll(int id)
{
	system::error_code error;
	filesystem::path dir = id >= 0 ? _sessionDirName + "/resources/" + std::to_string(id) : Utils::osPath(_sessionDirName);
	filesystem::remove_all(dir, error);
}


void TempFiles::deleteOrphans()
{
	Log::log() << "TempFiles::deleteOrphans started" << std::endl;

	system::error_code error;

	try {

		filesystem::path tempPath		= Utils::osPath(Dirs::tempDir());
		filesystem::path sessionPath	= Utils::osPath(_sessionDirName);

		filesystem::directory_iterator itr(tempPath, error);

		if (error)
		{
			perror(error.message().c_str());
			return;
		}

		for (; itr != filesystem::directory_iterator(); itr++)
		{
			filesystem::path p = itr->path();

			Log::log() << "looking at file " << p.string() << std::endl;

			if (p.compare(sessionPath) == 0)
				continue;

			for (std::string & shmemName : _shmemNames)
				if (p.compare(shmemName) == 0)
					continue;

			string fileName		= Utils::osPath(p.filename());
			bool is_directory	= filesystem::is_directory(p, error);

			if (error)
				continue;

			if (!is_directory)
			{
				if (fileName.substr(0, 5).compare("JASP-") == 0)
				{
					long modTime	= Utils::getFileModificationTime(Utils::osPath(p));
					long now		= Utils::currentSeconds();

					if (now - modTime > 70)
					{
						Log::log() << "Try to delete: " << fileName << std::endl;
						filesystem::remove(p, error);

						if (error)
						{
							Log::log() << "Error when deleting File: " << error.message() << std::endl;
							perror(error.message().c_str());
						}
					}
				}
			}
			else
			{

				if (std::atoi(fileName.c_str()) == 0)
					continue;

				filesystem::path statusFile = Utils::osPath(Utils::osPath(p) + "/status");

				if (filesystem::exists(statusFile, error))
				{
					long modTime	= Utils::getFileModificationTime(Utils::osPath(statusFile));
					long now		= Utils::currentSeconds();

					if (now - modTime > 70)
					{
						filesystem::remove_all(p, error);

						if (error)
							perror(error.message().c_str());
					}
				}
				else // no status file
				{
					filesystem::remove_all(p, error);

					if (error)
						perror(error.message().c_str());
				}
			}
		}

	}
	catch (runtime_error e)
	{
		perror("Could not delete orphans");
		perror(e.what());
		return;
	}
}


void TempFiles::heartbeat()
{
	Utils::touch(_statusFileName);
	for (string & shmemName : _shmemNames)
		Utils::touch(shmemName);
}

void TempFiles::purgeClipboard()
{
	system::error_code error;
	filesystem::remove_all(Utils::osPath(_clipboard), error);
}

string TempFiles::createSpecific_clipboard(const string &filename)
{
	system::error_code error;

	string fullPath				= _clipboard + "/" + filename;
	filesystem::path	path	= Utils::osPath(fullPath),
						dirPath	= path.parent_path();

	if (filesystem::exists(dirPath, error) == false || error)
		filesystem::create_directories(dirPath, error);

	return fullPath;
}

string TempFiles::createSpecific(const string &dir, const string &filename)
{
	system::error_code error;
	string fullPath			= _sessionDirName + "/" + dir;
	filesystem::path path	= Utils::osPath(fullPath);

	if (filesystem::exists(path, error) == false || error)
		filesystem::create_directories(path, error);

	return fullPath + "/" + filename;
}

void TempFiles::createSpecific(const string &name, int id, string &root, string &relativePath)
{
	root					= _sessionDirName;
	relativePath			= "resources" + (id >= 0 ? "/" + std::to_string(id) : "");
	filesystem::path path	= Utils::osPath(root + "/" + relativePath);

	system::error_code error;
	if (filesystem::exists(path, error) == false || error)
		filesystem::create_directories(path, error);

	relativePath += "/" + name;
}

void TempFiles::create(const string &extension, int id, string &root, string &relativePath)
{
	system::error_code error;

	root					= _sessionDirName;
	string resources		= root +  "/resources" + (id >= 0 ? "/" + std::to_string(id) : "");

	filesystem::path path	= Utils::osPath(resources);

	if (filesystem::exists(resources, error) == false)
		filesystem::create_directories(resources, error);

	string suffix = extension == "" ? "" : "." + extension;

	do
	{
		relativePath	= "resources/" + (id >= 0 ? std::to_string(id) + "/" : "") + "_" + std::to_string(_nextFileId++) + suffix;
		path			= Utils::osPath(root + "/" + relativePath);
	}
	while (filesystem::exists(path));
}

std::string TempFiles::createTmpFolder()
{
	system::error_code error;

	while(true)
	{
		std::string tmpFolder	= _sessionDirName + "/tmp" + std::to_string(_nextTmpFolderId++) + "/";
		filesystem::path path	= Utils::osPath(tmpFolder);

		if (!filesystem::exists(path, error))
		{
			filesystem::create_directories(path, error);
			return tmpFolder;
		}
	}
}

vector<string> TempFiles::retrieveList(int id)
{
	vector<string> files;

	system::error_code error;

	string dir = _sessionDirName;

	if (id >= 0)
		dir += "/resources/" + std::to_string(id);

	filesystem::path path = Utils::osPath(dir);

	filesystem::directory_iterator itr(path, error);

	if (error)
		return files;

	for (; itr != filesystem::directory_iterator(); itr++)
		if (filesystem::is_regular_file(itr->status()))
		{
			string absPath = itr->path().generic_string();
			string relPath = absPath.substr(_sessionDirName.size()+1);

			files.push_back(relPath);
		}

	return files;
}

void TempFiles::deleteList(const vector<string> &files)
{
	system::error_code error;

	for(const string &file : files)
	{
		string absPath		= _sessionDirName + "/" + file;
		filesystem::path p	= Utils::osPath(absPath);

		filesystem::remove(p, error);
	}
}

void TempFiles::addShmemFileName(std::string &name)
{
	_shmemNames.push_back(Dirs::tempDir()+ "/" + name);
}
