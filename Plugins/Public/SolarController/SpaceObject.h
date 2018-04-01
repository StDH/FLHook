#pragma once
#include "Main.h"

class SpaceObject {

public:

	/* ///////////////////////////////////////////
	 * Enums relevent to the generic SpaceObject
	 *////////////////////////////////////////////
	enum SpaceObjectType
	{
		SOLAR,
		JUMP,
		BASE
	};

	/* ///////////////////////////////////////////
	* Variables shared amongst all spawned object types
	*////////////////////////////////////////////

	 // The base nickname
	string nickname;

	// The ingame hash of the base nickname
	uint base;

	// The name of the base shown to other players
	wstring basename;

	// When the object is spawned, this is the IDS of the object name
	uint solar_ids{};

	// The enum representation of what kind of object this represents. Defaults to solar.
	SpaceObjectType objectType = SpaceObjectType::SOLAR;

	// The internal name for the model archetype used by this object
	string archetype;

	// The internal name for the loadout used by this object
	string loadout;

	float currentHealth = 100000;
	float maximumHealth = 100000;

	// The internal uint representation of what system this object exists in.
	uint system{};

	// The position in space which the object is in
	Vector position{};

	// The rotation in space which the object has
	Matrix rotation{};

	// The object affiliation
	uint affiliation{};

	// The infocard (if any) for the object, defaulting to nothing
	wstring infocard;
#define MAX_CHARACTERS 500
#define MAX_PARAGRAPHS 5
	wstring infocard_para[MAX_PARAGRAPHS + 1];

	// Is this object invulnerable? Defaulting to false.
	bool invulnerable = false;

	// The path to the base ini file
	string path;

	// The physical ingame space object
	uint spaceobj{};

	// The save timer offset
	int saveTimer;

	/* ///////////////////////////////////////////
	* Function Prototypes
	*////////////////////////////////////////////

	// Generalized constructor for normal spaceobjects requiring only the basics
	SpaceObject(uint system, Vector pos, Matrix rot, const string& archetype, const string& loadout, const string& nickname, const int maxHealth);

	// Constructor which takes in a filepath for a saved object
	SpaceObject(const string &path);

	virtual ~SpaceObject() = default;
	
	// Spawn the object with the given internal settings
	virtual void Spawn();

	// Some timer function
	bool Timer(uint curr_time);

	// Load the object from its INI file
	virtual void Load();

	// Save the object to its INI file
	virtual void Save();

	// Create the internal nickname hash for the base, given the english nickname
	static string CreateObjectNickname(const string &basename);

	// Function which gets run every time damage has occured for the base
	virtual float SpaceObjDamaged(uint space_obj, uint attacking_space_obj, float curr_hitpoints, float damage);

	// Function run when the spaceobj has been destroyed
	virtual float SpaceObjDestroyed();

	// A setter for the default object settings
	virtual void SetupDefaults();

	// Function which checks the reputation of a ship against the SpaceObject's affiliation.
	virtual float GetAttitudeTowardsClient(uint client);

	// Function which sets the AI personality for the spaceobj
	static pub::AI::SetPersonalityParams MakePersonality();

	// Applies a change of reputation between all bases in a clients system, to the clients ship
	static void SyncReputationForClientShip(uint ship, uint client, uint affiliation);

	// A function returning a pointer to the current object
	static SpaceObject *GetSpaceobject(uint obj);

	// Method deleting the object
	virtual void DeleteObject();

	////////////////////////////
	//FLHook Delegations
	////////////////////////////
	//TODO: Test if this needs to be handled by __stdcall or not
	virtual void HkCb_AddDmgEntry(DamageList *dmg, unsigned short p1, float damage, enum DamageEntry::SubObjFate fate);

	virtual void ObjectDestroyed(uint space_obj, uint client);
};
