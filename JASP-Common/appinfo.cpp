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

#include "appinfo.h"
#include <sstream> 

const Version AppInfo::version = Version(JASP_VERSION_MAJOR, JASP_VERSION_MINOR, JASP_VERSION_REVISION, JASP_VERSION_BUILD + 255); //Why the 255???
const std::string AppInfo::name = "JASP";
const std::string AppInfo::builddate = __DATE__ " " __TIME__ " (Netherlands)" ;

std::string AppInfo::getShortDesc()
{
	return AppInfo::name + " " + AppInfo::version.asString();
}

std::string AppInfo::getBuildYear()
{
	std::string datum = __DATE__;
	return datum.substr(datum.length() - 4);
}

std::string AppInfo::getRVersion()
{	
	std::stringstream ss;
	ss << CURRENT_R_VERSION;
	std::string str;
	ss >> str;
	return str;
}

