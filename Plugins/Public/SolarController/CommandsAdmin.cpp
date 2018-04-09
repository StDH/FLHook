#include "CommandHandler.h"
#include <sstream>
#include "SpaceObject.h"

void AdminCommands::AdminHelp(uint client)
{
	PrintUserCmdText(client, L".testobj - Spawn an simple solar object in front of your vessel");
	PrintUserCmdText(client, L".destroybase - Destroies a solar object, playerbase, or FLHook Jumphole");
}

bool AdminCommands::TestSpaceObj(uint client)
{

	uint clientShip;
	pub::Player::GetShip(client, clientShip);
	if (!clientShip)
	{
		PrintUserCmdText(client, L"ERR Not in space");
		return true;
	}

	// If the ship is moving, abort the processing.
	Vector dir1;
	Vector dir2;
	pub::SpaceObj::GetMotion(clientShip, dir1, dir2);
	if (dir1.x>5 || dir1.y>5 || dir1.z>5)
	{
		PrintUserCmdText(client, L"ERR Ship is moving");
		return true;
	}

	const int min = 100;
	const int max = 5000;
	const int randomint = min + (rand() % static_cast<int>(max - min + 1));

	string randomname = "TestObject";

	stringstream ss;
	ss << randomint;
	string str = ss.str();

	randomname.append(str);

	// Check for a conflicting object name
	if (SpaceObject::GetSpaceobject(CreateID(SpaceObject::CreateObjectNickname(randomname).c_str())))
	{
		PrintUserCmdText(client, L"ERR Deployment error, please run the command again");
		return true;
	}

	const wstring charname = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));
	AddLog("NOTICE: SpaceObj created %s by %s (%s)",
		randomname.c_str(),
		wstos(charname).c_str(),
		wstos(HkGetAccountID(HkGetAccountByCharname(charname))).c_str());

	// Prepare the spaceobj
	wstring basename = stows(randomname);
	uint shipSystem;
	Vector shipPos;
	Matrix shipRot;

	// Offset the ShipPosition so the station doesn't appear directly on top of you
	Rotate180(shipRot);
	TranslateX(shipPos, shipRot, 1000);

	pub::SpaceObj::GetLocation(clientShip, shipPos, shipRot);
	pub::SpaceObj::GetSystem(clientShip, shipSystem);

	//@@TODO: Adjust the maximum health value to reflect the maximum help defined in the archetype used for the model
	SpaceObject *newObj = new SpaceObject(shipSystem, shipPos, shipRot, "dsy_playerbase_01", "null_loadout", randomname, 10000000);

	// Set the affiliation to the admins
	newObj->affiliation = CreateID("fc_admin");

	newObj->Spawn();
	newObj->Save();

	PrintUserCmdText(client, L"OK: Test SpaceObject created");

	return true;
}

void AdminCommands::BaseDebug(uint iClientID)
{
	// Spit out basic information for each object present
	for(const auto& spaceObj : spaceObjects)
	{
		PrintUserCmdText(iClientID, L"Base: %s | Type: %s | Archetype: %s | Loadout: %s | CurrHp: %f | MaxHp: %f",
			spaceObj.second->basename, stows(SpaceObject::objEnumToStringRepresentation(spaceObj.second->objectType)), stows(spaceObj.second->archetype),
			stows(spaceObj.second->loadout), spaceObj.second->currentHealth, spaceObj.second->maximumHealth);
	}
}