#pragma once


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

using namespace std;

// Are we in debugging mode for the plugin?
static bool debuggingMode = true;

static PLUGIN_RETURNCODE returncode;

class SpaceObject;

////////////////////////////////////////////
//A list of each existing type of object
////////////////////////////////////////////
static map<uint, SpaceObject*> spaceObjects;

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

////////////////////////////////////////////
//Functions
////////////////////////////////////////////

