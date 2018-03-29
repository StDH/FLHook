// Template for FLHookPlugin
// February 2016 by BestDiscoveryHookDevs2016
//
// This is a template with the bare minimum to have a functional plugin.
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <sstream>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>
#include "SpaceObject.h"

#include "../hookext_plugin/hookext_exports.h"
#include "PlayerCommands.h"


void LoadSettings();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand((uint)time(nullptr));
	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		if (set_scCfgFile.length()>0)
			LoadSettings();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		// Something
	}
	return true;
}

/// Hook will call this function after calling a plugin function to see if we the
/// processing to continue
EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

bool bPluginEnabled = true;


void LoadSettings()
{

	// Verify that all of the directories required for saving bases exist. If not, create them
	char datapath[MAX_PATH];
	GetUserDataPath(datapath);
	string spaceobjdir = string(datapath) + R"(\Accts\MultiPlayer\spawned_solars\objects\)";
	string pobdir = string(datapath) + R"(\Accts\MultiPlayer\spawned_solars\playerbase\)";

	CreateDirectoryA(spaceobjdir.c_str(), nullptr);
	CreateDirectoryA(pobdir.c_str(), nullptr);

}


SpaceObject *GetSpaceObject(uint base)
{
	map<uint, SpaceObject*>::iterator i = spaceObjects.find(base);
	if (i != spaceObjects.end())
		return i->second;
	return nullptr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Actual Code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Clean up when a client disconnects */
void ClearClientInfo(uint iClientID)
{
	
	// Client cleanup here

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Handle Hooks
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BaseDestroyed_Hook(uint space_obj, uint client)
{
	returncode = DEFAULT_RETURNCODE;
	
	//@@TODO Handle destroying modules here. If some exist, make sure to set the returncode to SKIPPLUGINS
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Client command processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool(*_UserCmdProc)(uint, const wstring &, const wstring &, const wchar_t*);


/**
This function is called by FLHook when a user types a chat string. We look at the
string they've typed and see if it starts with one of the above commands. If it
does we try to process it.
*/
bool UserCmd_Process(uint iClientID, const wstring &args)
{
	returncode = DEFAULT_RETURNCODE;
	if(args.find(L"somecommand"))
	{
		// Do command
	}

	return false;
}

// Admin Command Handling
bool ExecuteCommandString_Callback(CCmds* cmd, const wstring &args)
{
	return HandleAdminCommands(cmd, args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Solar Controller by Remnant - Some code refactored from Alley/Cannon";
	p_PI->sShortName = "solarcntl";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&BaseDestroyed_Hook, PLUGIN_BaseDestroyed, 0));

	return p_PI;
}
