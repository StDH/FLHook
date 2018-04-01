#pragma once
#include "Main.h"
#include "SpaceObject.h"

class SpaceObject;

namespace SolarUtility {

	// ------------ Client Commands ------------
	inline void SendCommand(uint client, const wstring &message)
	{
		HkFMsg(client, L"<TEXT>" + XMLText(message) + L"</TEXT>");
	}

	inline void SendSetBaseInfoText(uint client, const wstring &message)
	{
		SendCommand(client, wstring(L" SetBaseInfoText ") + message);
	}

	inline void SendSetBaseInfoText2(uint client, const wstring &message)
	{
		SendCommand(client, wstring(L" SetBaseInfoText2 ") + message);
	}

	inline void SendResetMarketOverride(uint client)
	{
		SendCommand(client, L" ResetMarketOverride");
	}

	// ------------ Text Formatting Functions ------------
	static wstring Int64ToPrettyStr(INT64 iValue);
	static wstring IntToStr(uint iValue);


	namespace SpaceObj {

		// Function which applies the current obj state, to the ingame object representation of what the plugin is handling
		void SendBaseStatus(uint client, SpaceObject *obj);

	}

	namespace Playerbase
	{

	}



}
