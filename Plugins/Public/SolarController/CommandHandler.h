#pragma once
#include "Main.h"

#define POPUPDIALOG_BUTTONS_LEFT_YES 1
#define POPUPDIALOG_BUTTONS_CENTER_NO 2
#define POPUPDIALOG_BUTTONS_RIGHT_LATER 4
#define POPUPDIALOG_BUTTONS_CENTER_OK 8

// Function which handles all of the available base commands
bool HandlePlayerCommands(uint iClientID, const wstring &args);
bool HandleAdminCommands(CCmds* cmd, const wstring &args);

namespace AdminCommands
{
	void AdminHelp(uint client);
	bool TestSpaceObj(uint client);
	void BaseDebug(uint client);
}