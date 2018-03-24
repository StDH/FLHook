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
#include "Main.h"

#include "../hookext_plugin/hookext_exports.h"

static int set_iPluginDebug = 0;

/// A return code to indicate to FLHook if we want the hook processing to continue.
PLUGIN_RETURNCODE returncode;

void LoadSettings();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand((uint)time(0));
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Settings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadSettings()
{

	// Load settings and INI files here

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

SpaceObject *GetSpaceObject(uint base)
{
	map<uint, SpaceObject*>::iterator i = spaceObjects.find(base);
	if (i != spaceObjects.end())
		return i->second;
	return 0;
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
void BaseDestroyed(uint space_obj, uint client)
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
}

// Admin Command Handling
bool ExecuteCommandString_Callback(CCmds* cmd, const wstring &args)
{
	returncode = DEFAULT_RETURNCODE;

	if(args.find(L"objectdestroy") == 0)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;

		uint client = HkGetClientIdFromCharname(cmd->GetAdminName());
		SpaceObject *obj;
		bool foundObj = false;

		// Scan through all available SpaceObjects, if one exists, nuke it from high orbit
		for(map<uint, SpaceObject*>::iterator i = spaceObjects.begin(); i != spaceObjects.end(); ++i)
		{
			if(i->second->basename == cmd->ArgStrToEnd(1))
			{
				obj = i->second;
				foundObj = true;
			}
		}

		if(!foundObj)
		{
			cmd->Print(L"Error: SpaceObj doesn't exist");
			return true;
		}

		obj->currentHealth = 0;

		cmd->Print(L"Ded");
		return true;
	}
	else if(args.find(L"testobj") == 0)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;

		uint client = HkGetClientIdFromCharname(cmd->GetAdminName());

		uint ship;
		pub::Player::GetShip(client, ship);
		if(!ship)
		{
			PrintUserCmdText(client, L"Error: Not in space");
			return true;
		}

		int min 100;
		int max = 5000;
		int randomsiegeint = min + (rand() % (int)(max - min + 1));

		string randomname = "TB";

		stringstream ss;
		ss << randomsiegeint;
		string str = ss.str();

		randomname.append(str);

		//Check for conflicting base name
		if (GetSpaceObject(CreateID(SpaceObject::CreateBaseNickname(randomname).c_str())))
		{
			PrintUserCmdText(client, L"ERR Deployment error, please reiterate.");
			return true;
		}

		//@@TODO Admin log this being run

		// Get the current position, rotation, and system of the player
		uint system;
		Vector pos;
		Matrix rot;
		string loadout = "null_loadout";

		pub::SpaceObj::GetSystem(ship, system);
		pub::SpaceObj::GetLocation(ship, pos, rot);

		// Set the default archtype to a simple outpost

		SpaceObject *obj = new SpaceObject(system, pos, rot, "wplatform_pbase_01", loadout, randomname);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Solar Controller by Conrad Weiser\Laz - Some code refactored from Alley/Cannon";
	p_PI->sShortName = "solarcntl";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));

	return p_PI;
}
