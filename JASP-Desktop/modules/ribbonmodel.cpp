//
// Copyright (C) 2013-2018 University of Amsterdam
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


#include "ribbonmodel.h"
#include "dirs.h"
#include "log.h"

RibbonModel::RibbonModel(DynamicModules * dynamicModules, std::vector<std::string> commonModulesToLoad, std::vector<std::string> extraModulesToLoad)
	: QAbstractListModel(dynamicModules), _dynamicModules(dynamicModules)
{
	for(const std::string & moduleName : commonModulesToLoad)
		addRibbonButtonModelFromModulePath(QFileInfo(QString::fromStdString(Dirs::resourcesDir() + moduleName + "/")), true);
	for(const std::string & moduleName : extraModulesToLoad)
		addRibbonButtonModelFromModulePath(QFileInfo(QString::fromStdString(Dirs::resourcesDir() + moduleName + "/")), false);

	connect(_dynamicModules, &DynamicModules::dynamicModuleAdded,		this, &RibbonModel::addDynamicRibbonButtonModel);
	connect(_dynamicModules, &DynamicModules::dynamicModuleUninstalled,	this, &RibbonModel::removeDynamicRibbonButtonModel);

	for(const std::string & modName : _dynamicModules->moduleNames())
		addRibbonButtonModelFromDynamicModule((*_dynamicModules)[modName]);
}

void RibbonModel::addRibbonButtonModelFromDynamicModule(Modules::DynamicModule * module)
{
	addRibbonButtonModel(new RibbonButton(this, module));
}

void RibbonModel::addRibbonButtonModelFromModulePath(QFileInfo modulePath, bool isCommon)
{
	if(!modulePath.exists())
	{
		Log::log() << "Path " << modulePath.absoluteFilePath().toStdString() << " does not exist!" << std::flush;
		return;
	}

	QFile descriptionFile(modulePath.absoluteFilePath() + "/description.json");
	if(!descriptionFile.exists())
	{
		Log::log() << "Could not find description.json file in " << modulePath.absoluteFilePath().toStdString() << std::flush;
		return;
	}

	descriptionFile.open(QFile::ReadOnly);
	std::string	descriptionTxt(descriptionFile.readAll().toStdString());

	Json::Value descriptionJson;

	if(Json::Reader().parse(descriptionTxt, descriptionJson))
		addRibbonButtonModel(new RibbonButton(this, descriptionJson, isCommon));
	else
		Log::log() << "Error when reading description.json of " << modulePath.filePath().toStdString() << std::flush;
}

void RibbonModel::addRibbonButtonModel(RibbonButton* model)
{
	model->setDynamicModules(_dynamicModules);

	if(isModuleName(model->moduleName()))
		removeRibbonButtonModel(model->moduleName());

	emit beginInsertRows(QModelIndex(), rowCount(), rowCount());

	_moduleNames.push_back(model->moduleName());
	_buttonModelsByName[model->moduleName()] = model;

	emit endInsertRows();

	connect(model, &RibbonButton::iChanged, this, &RibbonModel::ribbonButtonModelChanged);
}

QVariant RibbonModel::data(const QModelIndex &index, int role) const
{
	if (index.row() >= rowCount())
		return QVariant();

	size_t row = size_t(index.row());

	switch(role)
	{
	case DisplayRole:		return ribbonButtonModelAt(row)->titleQ();
	case RibbonRole:		return QVariant::fromValue(ribbonButtonModelAt(row));
	case EnabledRole:		return ribbonButtonModelAt(row)->enabled();
	case DynamicRole:		return ribbonButtonModelAt(row)->isDynamic();
	case CommonRole:		return ribbonButtonModelAt(row)->isCommon();
	case ModuleNameRole:	return ribbonButtonModelAt(row)->moduleNameQ();
	case ModuleTitleRole:	return ribbonButtonModelAt(row)->titleQ();
	case ModuleRole:		return QVariant::fromValue(ribbonButtonModelAt(row)->myDynamicModule());
	case ClusterRole:		//To Do!
	default:				return QVariant();
	}
}


QHash<int, QByteArray> RibbonModel::roleNames() const
{
	static const auto roles = QHash<int, QByteArray>{
		{ ClusterRole,		"clusterMenu"		},
		{ DisplayRole,		"displayText"		},
		{ RibbonRole,		"ribbonButton"	},
		{ EnabledRole,		"ribbonEnabled"		},
		{ DynamicRole,		"isDynamic"			},
		{ CommonRole,		"isCommon"			},
		{ ModuleNameRole,	"moduleName"		},
		{ ModuleTitleRole,	"moduleTitle"		},
		{ ModuleRole,		"dynamicModule"		} };

	return roles;
}

RibbonButton* RibbonModel::ribbonButtonModel(std::string name) const
{
	if(_buttonModelsByName.count(name) > 0)
		return _buttonModelsByName.at(name);

	return nullptr;
}

void RibbonModel::removeRibbonButtonModel(std::string moduleName)
{
	if(!isModuleName(moduleName))
		return;

	int indexRemoved = -1;

	for(int i=_moduleNames.size() - 1; i >= 0; i--)
		if(_moduleNames[i] == moduleName)
		{
			indexRemoved = i;
			break;
		}

	emit beginRemoveRows(QModelIndex(), indexRemoved, indexRemoved);

	delete _buttonModelsByName[moduleName];
	_buttonModelsByName.erase(moduleName);

	_moduleNames.erase(_moduleNames.begin() + indexRemoved);

	emit endRemoveRows();
}

void RibbonModel::setHighlightedModuleIndex(int highlightedModuleIndex)
{
	if (_highlightedModuleIndex == highlightedModuleIndex)
		return;

	_highlightedModuleIndex = highlightedModuleIndex;
	emit highlightedModuleIndexChanged(_highlightedModuleIndex);
}

void RibbonModel::setModuleEnabled(int ribbonButtonModelIndex, bool enabled)
{
	if(ribbonButtonModelIndex < 0)
		return;

	RibbonButton * ribbonButtonModel = ribbonButtonModelAt(size_t(ribbonButtonModelIndex));

	if(ribbonButtonModel->enabled() != enabled)
	{
		ribbonButtonModel->setEnabled(enabled);
		emit dataChanged(index(ribbonButtonModelIndex), index(ribbonButtonModelIndex));
	}
}

Modules::AnalysisEntry *RibbonModel::getAnalysis(const std::string& moduleName, const std::string& analysisName)
{
	Modules::AnalysisEntry* analysis = nullptr;
	RibbonButton* ribbonButton = ribbonButtonModel(moduleName);
	if (ribbonButton)
		analysis = ribbonButton->getAnalysis(analysisName);
	
	return analysis;
}

QString RibbonModel::getModuleNameFromAnalysisName(const QString analysisName)
{
	QString result = "Common";
	std::string searchName = analysisName.toStdString();
	// This function is needed for old JASP file: they still have a reference to the common mondule that does not exist anymore.
	for (const std::string& myModuleName : _moduleNames)
	{
		RibbonButton* button = _buttonModelsByName[myModuleName];
		for (const std::string& name : button->getAllAnalysisNames())
		{
			if (name == searchName)
			{
				result = QString::fromStdString(myModuleName);
				break;
			}
		}
		if (result != "Common")
			break;

	}

	return result;
}

void RibbonModel::toggleModuleEnabled(int ribbonButtonModelIndex)
{
	if(ribbonButtonModelIndex < 0)
		return;

	RibbonButton * ribbonButtonModel = ribbonButtonModelAt(size_t(ribbonButtonModelIndex));

	ribbonButtonModel->setEnabled(!ribbonButtonModel->enabled());

	emit dataChanged(index(ribbonButtonModelIndex), index(ribbonButtonModelIndex));
}

int RibbonModel::ribbonButtonModelIndex(RibbonButton * model)	const
{
	for(auto & keyval : _buttonModelsByName)
		if(keyval.second == model)
			for(size_t i=0; i<_moduleNames.size(); i++)
				if(_moduleNames[i] == keyval.first)
					return int(i);
	return -1;
}


void RibbonModel::ribbonButtonModelChanged(RibbonButton* model)
{
	int row = ribbonButtonModelIndex(model);
	emit dataChanged(index(row), index(row));
}

void RibbonModel::moduleLoadingSucceeded(const QString & moduleName)
{
	if(moduleName == "*")
		return;

	RibbonButton * ribMod = ribbonButtonModel(moduleName.toStdString());
	ribMod->setEnabled(true);
}
