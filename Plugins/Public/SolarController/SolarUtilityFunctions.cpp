#include "SolarUtilityFunctions.h"

static wstring SolarUtility::IntToStr(uint iValue)
{
	wchar_t buf[1000];
	swprintf(buf, _countof(buf), L"%u", iValue);
	return	buf;
}

static wstring Int64ToPrettyStr(INT64 iValue)
{
	wchar_t buf[1000];
	swprintf(buf, _countof(buf), L"%I64d", iValue);
	int len = wcslen(buf);

	wstring wscBuf;
	for (int i = len - 1, j = 0; i >= 0; i--, j++)
	{
		if (j == 3)
		{
			j = 0;
			wscBuf.insert(0, L".");
		}
		wscBuf.insert(0, wstring(1, buf[i]));
	}
	return wscBuf;
}

// Send the SpaceObj status to the universe
void SolarUtility::SpaceObj::SendBaseStatus(uint client, SpaceObject *obj)
{
	const Universe::ISystem *sys = Universe::get_system(obj->system);

	wstring objStatus = L"<RDL><PUSH/>";
	objStatus += L"<TEXT>" + XMLText(obj->basename) + L", " + HkGetWStringFromIDS(sys->strid_name) + L"</TEXT><PARA/><PARA/>";

	objStatus += obj->infocard;

	if (obj->affiliation)
	{
		objStatus += L"<TEXT>Affiliation: " + HkGetWStringFromIDS(Reputation::get_name(obj->affiliation)) + L"</TEXT><PARA/>";
	}
	else
	{
		objStatus += L"<TEXT>Affiliation: None</TEXT><PARA/>";
	}

	objStatus += L"<PARA/><POP/></RDL>";

	SendSetBaseInfoText2(client, objStatus);
}
