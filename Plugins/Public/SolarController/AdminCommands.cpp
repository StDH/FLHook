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
#include "Main.h"

#define POPUPDIALOG_BUTTONS_LEFT_YES 1
#define POPUPDIALOG_BUTTONS_CENTER_NO 2
#define POPUPDIALOG_BUTTONS_RIGHT_LATER 4
#define POPUPDIALOG_BUTTONS_CENTER_OK 8

namespace AdminCommands
{
	void AdminHelp(uint client, const wstring &args)
	{
		PrintUserCmdText(client, L".testspawn - Spawn an simple solar object in front of your vessel");
		PrintUserCmdText(client, L".destroybase - Destroies a solar object, playerbase, or FLHook Jumphole");

	}
}