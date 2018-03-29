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

SpaceObject::SpaceObject(const string& path)
{
	// Load the file using the path
	
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


void SpaceObject::Save()
{
	FILE *file = fopen(path.c_str(), "w");
	if(file)
	{
		fprintf(file, "[SolarObj]\n");
		fprintf(file, "nickname = %s\n", nickname.c_str());
		fprintf(file, "objsolar = %s\n", archetype.c_str());
		fprintf(file, "objloadout = %s\n", loadout.c_str());
		fprintf(file, "affiliation = %u\n", affiliation);
		fprintf(file, "system = %i\n", system);
		
		fprintf(file, "pos = %0.0f, %0.0f, %0.0f\n", position.x, position.y, position.z);

		const Vector vRot = MatrixToEuler(rotation);
		fprintf(file, "rot = %0.0f, %0.0f, %0.0f\n", vRot.x, vRot.y, vRot.z);

		ini_write_wstring(file, "infoname", basename);
		for (auto i = 1; i <= MAX_PARAGRAPHS; i++)
		{
			ini_write_wstring(file, "infocardpara", infocard_para[i]);
		}

		fprintf(file, "maxhealth = %0.0f\n", maximumHealth);
		fprintf(file, "currenthealth = %0.0f\n", currentHealth);
	}
}

////////////////////////////
// HOOK HANDLING
////////////////////////////

// Handle the base being destroied
void BaseDestroyed(uint space_obj, uint client)
{
	
}