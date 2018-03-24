#include "SpaceObject.h"

SpaceObject::SpaceObject(const uint system, const Vector pos, const Matrix rot, const string& archetype, const string&
                         loadout, const string& nickname)
{
	// Apply the so_ tag since this is a generic space object
	this->nickname = string("so_").append(nickname);
	this->base = CreateID(this->nickname.c_str());
	this->loadout = loadout;
	this->position = pos;
	this->rotation = rot;
	this->archetype = archetype;
	
	SpaceObject::SetupDefaults();
}



void SpaceObject::SetupDefaults()
{
	
	// The path to the save file for the base
	if(path.empty())
	{
		char datapath[MAX_PATH];
		GetUserDataPath(datapath);

		char tpath[1024];
		sprintf(tpath, R"(%s\Accts\MultiPlayer\spawned_solars\objects\base_%08x.ini)", datapath, base);
		path = tpath;
	}

	// Build the infocard text
	infocard.clear();
	for(auto i = 1; i <= MAX_PARAGRAPHS; i++)
	{
		wstring wscXML = infocard_para[i];
		if (wscXML.length())
		{
			infocard += L"<TEXT>" + wscXML + L"</TEXT><PARA/><PARA/>";
		}
	}

	// Validate the affiliation and clear it if there is no infocard
	// name assigned to it. We assume that this would be an corrupted affiliation.
	if (affiliation)
	{
		uint name;
		pub::Reputation::GetGroupName(affiliation, name);
		if (!name)
		{
			affiliation = 0;
		}
	}
}

////////////////////////////
// HOOK HANDLING
////////////////////////////

// Handle the base being destroied
void BaseDestroyed(uint space_obj, uint client)
{
	
}