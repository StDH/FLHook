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
#include "CommandHandler.h"
#include "SpaceObject.h"



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
	if(args.find(L"testobj") == 0)
	{
		return AdminCommands::TestSpaceObj(client);
	}
	if(args.find(L"basedebug") == 0)
	{
		AdminCommands::BaseDebug(client);
		return true;
	}

	return false;
}
