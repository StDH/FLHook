#ifndef __MAIN_H__
#define __MAIN_H__ 1

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>
#include "SpaceObject.h"

using namespace std;

////////////////////////////////////////////
//A list of each existing type of object
////////////////////////////////////////////
map<uint, SpaceObject*> spaceObjects;

////////////////////////////////////////////
//BROAD NAMESPACE DECLARATIONS FOR OBJECTS
////////////////////////////////////////////
namespace ExportData
{
	void ToHTML();
	void toJSON();
}

namespace Log
{
	void LogBaseAction(string basename, const char *message);
	void LogGenericAction(string message);
}

namespace SolarPlayerCmd
{
	void SolarHelp(uint client, const wstring &args);
	void SolarDeploy(uint client, const wstring &args);
}

namespace AdminCommands
{
	void AdminHelp(uint client, const wstring &args);
}

// Are we in debugging mode for the plugin?
extern bool debuggingMode;

#endif
