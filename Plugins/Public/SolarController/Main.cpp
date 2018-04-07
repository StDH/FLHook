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
#include "CommandHandler.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//__declspec
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Handle docking error messages
bool __stdcall HkCb_IsDockableError(uint dock_with, uint base)
{
	if (SpaceObject::GetSpaceobject(base))
		return false;
	ConPrint(L"ERROR: Base not found dock_with=%08x base=%08x\n", base, base);
	return true;
}

__declspec(naked) void HkCb_IsDockableErrorNaked()
{
	__asm
	{
		test[esi + 0x1b4], eax
		jnz no_error
		push[edi + 0xB8]
		push[esi + 0x1b4]
		call HkCb_IsDockableError
		test al, al
		jz no_error
		push 0x62b76d3
		ret
		no_error :
		push 0x62b76fc
			ret
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Start up
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadSettings();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	bool patched = false;
	srand((uint)time(nullptr));
	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		if (set_scCfgFile.length()>0)
			LoadSettings();
		if(!patched)
		{
			hModCommon = GetModuleHandleA("common.dll");
			{
				// Suppress "is dockable " error message
				byte patch[] = { 0xe9 }; // jmpr
				WriteProcMem((char*)hModCommon + 0x576cb, patch, sizeof(patch));
				PatchCallAddr((char*)hModCommon, 0x576cb, (char*)HkCb_IsDockableErrorNaked);
			}
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		for (auto& spaceObject : spaceObjects)
		{
			delete spaceObject.second;
			ConPrint(L"SolarController: Deleted Object - %s", spaceObject.second->basename.c_str());
		}

		HkUnloadStringDLLs();
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
	string spawneddir = string(datapath) + R"(\Accts\MultiPlayer\spawned_solars\)";
	string spaceobjdir = spawneddir + R"(objects\)";
	string pobdir = spawneddir + R"(playerbase\)";

	CreateDirectoryA(spawneddir.c_str(), nullptr);
	CreateDirectoryA(spaceobjdir.c_str(), nullptr);
	CreateDirectoryA(pobdir.c_str(), nullptr);

	ConPrint(L"SolarController: Directories Validated\n");
	
	int objectCount = 0;
	int baseCount = 0;

	string objPath = spaceobjdir + R"(object*.ini)"; // Path to the object files
	string pobPath = pobdir + R"(base*.ini)"; // Path to the base files

	if(debuggingMode > 0)
	{
		ConPrint(L"SolarController: Object Path: %s\n", stows(objPath).c_str());
		ConPrint(L"SolarController: Base Path: %s\n", stows(pobPath).c_str());
	}
	
	
	WIN32_FIND_DATA findfile;
	HANDLE handle = FindFirstFile(objPath.c_str(), &findfile);
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			string filepath = spawneddir + R"(objects\)" + findfile.cFileName;

			if(debuggingMode > 0) // Output all object names to the console if we are in debugmode
				ConPrint(L"%s\n", stows(filepath).c_str());

			SpaceObject *base = new SpaceObject(filepath);
			spaceObjects[base->base] = base;
			base->Spawn();
			objectCount++;
		} while (FindNextFile(handle, &findfile));
		FindClose(handle);
	}

	ConPrint(L"SolarController: Number of SpaceObjects: %u\n", objectCount);
	ConPrint(L"SolarController: Number of Bases: %u\n", baseCount);

	HkLoadStringDLLs();
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
	
	//Check that this is one of our bases
	if(spaceObjects.find(space_obj) != spaceObjects.end())
	{
		returncode = SKIPPLUGINS;
		
	}
	
}

// Handle damage being dealt to a spawned object
void __stdcall HkCb_AddDmgEntry(DamageList *dmg, unsigned short p1, float damage, enum DamageEntry::SubObjFate fate)
{
	returncode = DEFAULT_RETURNCODE;
	
	//Delegate the function to it's correct object type
	for(const auto& obj : spaceObjects)
	{
		if(debuggingMode == 2)
		{
			ConPrint(L"");
		}
		obj.second->HkCb_AddDmgEntry(dmg, p1, damage, fate);
	}

}

// Handle attempted dock requests 
void __cdecl Dock_Call(unsigned int const &iShip, unsigned int const &base, int iCancel, enum DOCK_HOST_RESPONSE response)
{
	returncode = DEFAULT_RETURNCODE;

	uint client = HkGetClientIDByShip(iShip);
	SpaceObject* obj = GetSpaceObject(base);
	ConPrint(L"%u\n", obj);
	if(obj && (response == PROCEED_DOCK || response == DOCK))
	{
		pub::Player::SendNNMessage(client, pub::GetNicknameId("info_access_denied"));
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
	}
}

// Handle requests - called before Dock_Call
void __stdcall RequestEvent(int iIsFormationRequest, unsigned int iShip, unsigned int iDockTarget, unsigned int p4, unsigned long p5, unsigned int client)
{
	returncode = DEFAULT_RETURNCODE;
	if (client)
	{
		if (!iIsFormationRequest)
		{
			SpaceObject* obj = GetSpaceObject(iDockTarget);
			if (obj) // If it is a space object
			{
				PrintUserCmdText(client, L"You cannot dock with an object.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return;
			}
		}
	}
}

// Handle undocking/spawning
void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int client)
{
	returncode = DEFAULT_RETURNCODE;
	ConPrint(L"Character Select Call");
	// Loop over all space objects and set their names
	for (auto& spaceObject : spaceObjects)
	{
		if (debuggingMode > 1)
			ConPrint(L"SolarController: Changing Base IDSString: %u to %s", spaceObject.second->solar_ids, spaceObject.second->basename.c_str());
		HkChangeIDSString(client, spaceObject.second->solar_ids, spaceObject.second->basename);
	}
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
	return HandlePlayerCommands(iClientID, args);
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
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_AddDmgEntry, PLUGIN_HkCb_AddDmgEntry, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&RequestEvent, PLUGIN_HkIServerImpl_RequestEvent, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));

	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&Dock_Call, PLUGIN_HkCb_Dock_Call, 10));

	return p_PI;
}
