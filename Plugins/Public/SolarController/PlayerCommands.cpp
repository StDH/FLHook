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
#include "PlayerCommands.h"


bool HandlePlayerCommands(uint iClientID, const wstring& args)
{
	returncode = DEFAULT_RETURNCODE;
	// Handle shit here
	
	return false;
}

bool HandleAdminCommands(CCmds* cmd, const wstring &args)
{
	returncode = DEFAULT_RETURNCODE;

	const uint client = HkGetClientIdFromCharname(cmd->GetAdminName());

	if(args.find(L"basehelp") == 0)
	{
		AdminCommands::AdminHelp(client);
		return true;
	}

	return false;
}


void AdminCommands::AdminHelp(uint client)
{
	PrintUserCmdText(client, L".testspawn - Spawn an simple solar object in front of your vessel");
	PrintUserCmdText(client, L".destroybase - Destroies a solar object, playerbase, or FLHook Jumphole");
}

