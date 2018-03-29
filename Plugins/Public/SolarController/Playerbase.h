#pragma once
#include "Main.h"
#include "SpaceObject.h"


class Playerbase : public SpaceObject {

	/*
	 * Structs relevent only to playerbase objects
	 */
	struct MARKET_ITEM
	{
		MARKET_ITEM() : quantity(0), price(1.0f), min_stock(100000), max_stock(100000) {}

		// Number of units of a specific commodity stored in this base
		uint quantity;

		// Buy/Sell price for the commodity
		float price;

		// Stop selling if the base holds less than this number of items
		uint min_stock;

		// Stop buying if the base holds more than this number of items
		uint max_stock;
	};

	struct NEWS_ITEM
	{
		wstring headline;
		wstring text;
	};

	/*
	 * Variables relevent to playerbase objects
	 */

	 // A depiction of how much time is remaining, until the next filesystem save operation should be run for the base
	int timeRemainingUntilNextSave;

	// A represention of how much money exists in the playerbases account currently
	INT64 money;

	// The basic armour and commodiity storage available on this base
	uint base_level;

	// Is the base repairing itself currently?
	bool isRepairing;

	// The last known attacker for the playerbase
	wstring lastAttacker;

	/*
	 * Function prototypes
	 */
	void Save() override;
	void Load() override;
	
	void Spawn() override;

	// Sync the reputation of the base (including friendly/hostile tags) to the player
	void SyncReputationForBase();
	void SyncReputationForBaseObject(uint space_obj);
	

};

namespace PlayerCommands
{
	//Insert function prototypes for each command available to be run
}