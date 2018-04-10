#include "SpaceObject.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

SpaceObject::SpaceObject(const uint system, const Vector pos, const Matrix rot, const string& archetype, const string&
                         loadout, const string& nicknameReadable, const int maxHealth)
{
	// Apply the so_ tag since this is a generic space object
	this->nickname = CreateObjectNickname(nicknameReadable);
	this->base = CreateID(this->nickname.c_str());
	this->loadout = loadout;
	this->system = system;
	this->position = pos;
	this->rotation = rot;
	this->archetype = archetype;
	this->maximumHealth = maximumHealth;
	this->currentHealth = maximumHealth;
	this->basename = stows(nicknameReadable);

	SpaceObject::SetupDefaults();

	// A random number between 0 and 60 is chosen, which is then multiplied by 1000 so millisecond operations are scaled to seconds
	this->saveTimer = (rand() % 60) * 1000;
	this->lastSavedTime = timeInMS();

	spaceObjects[this->base] = this;
}

SpaceObject::SpaceObject(const string& path)
{
	SpaceObject::Load(path);
}

void SpaceObject::Spawn()
{

	//@@TODO: Fix this method. Currently it's not working as intended.

	if(!spaceobj)
	{
		pub::SpaceObj::SolarInfo si{};
		memset(&si, 0, sizeof(si));
		si.iFlag = 4;

		char archname[100];

		// Prepare the settings for the space object
		_snprintf(archname, sizeof(archname), this->archetype.c_str());
		si.iArchID = CreateID(archname);
		si.iLoadoutID = CreateID(this->loadout.c_str());
		si.iHitPointsLeft = INT_MAX;
		si.iSystemID = this->system;
		si.mOrientation = this->rotation;
		si.vPos = position;
		si.Costume.head = CreateID("pi_pirate2_head");
		si.Costume.body = CreateID("pi_pirate8_body");
		si.Costume.lefthand = 0;
		si.Costume.righthand = 0;
		si.Costume.accessories = 0;
		si.iVoiceID = CreateID("atc_leg_m01");
		si.iRep = affiliation;
		strncpy_s(si.cNickName, sizeof(si.cNickName), this->nickname.c_str(), this->nickname.size());

		// Check to see if the hook IDS limit has been reached
		static uint solar_ids = 526000;
		if (++solar_ids > 526999)
		{
			solar_ids = 0;
			return;
		}

		// Send the name to all players who are online
		this->solar_ids = solar_ids;
		wstring name = this->basename;

		struct PlayerData *pd = 0;
		while ((pd = Players.traverse_active(pd)))
		{
			HkChangeIDSString(pd->iOnlineID, this->solar_ids, this->basename);
		}

		// Set the base name
		FmtStr infoname(this->solar_ids, nullptr);
		infoname.begin_mad_lib(solar_ids); // scanner name
		infoname.end_mad_lib();

		FmtStr infocard(this->solar_ids, nullptr);
		infocard.begin_mad_lib(solar_ids); // infocard
		infocard.end_mad_lib();
		pub::Reputation::Alloc(si.iRep, infoname, infocard);
		
		// Spawn the solar object
		SpawnSolar(this->spaceobj, si);

		// Set the health for the Space Object to be it's maximum by default
		pub::SpaceObj::SetRelativeHealth(this->spaceobj, (this->currentHealth / this->maximumHealth));
		if (debuggingMode > 0)
			ConPrint(L"SolarController: SpaceObj::created space_obj=%u health=%f\n", this->spaceobj, (this->currentHealth));

		SyncReputationForBaseObject(this->spaceobj);

		pub::AI::SetPersonalityParams pers = MakePersonality();
		pub::AI::SubmitState(this->spaceobj, &pers);

	}
}

void SpaceObject::Timer(mstime currTime)
{
	// Since this is a simple SpaceObj, the only timer operation we need to do is handling autosaving
	if(currTime > (this->lastSavedTime + this->saveTimer))
	{
		this->Save();
		this->lastSavedTime = timeInMS();

		// Save again 60 seconds later
		this->saveTimer = (60 * 1000);

		if (debuggingMode > 1)
		{
			{
				ConPrint(L"SolarController: SpaceObject:: (%s) running save operation\n", stows(this->nickname));
			}
		}
	}
}

// This is only a spaceobject, we don't need to do fancy playerbase functions here
void SpaceObject::DeleteObject()
{
	char datapath[MAX_PATH];
	GetUserDataPath(datapath);

	string objarchivedir = string(datapath) + R"(\Accts\MultiPlayer\spawned_solars\objects\archive\)";
	CreateDirectoryA(objarchivedir.c_str(), nullptr);

	const string timestamp = boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());

	char namehash[16];
	sprintf(namehash, "%08x", this->base);
		
	string fullpath = objarchivedir + "obj_" + namehash + "." + timestamp + ".ini";
	if(!MoveFile(this->path.c_str(), fullpath.c_str()))
	{
		AddLog("ERROR: Base destruction MoveFile FAILED! Error code: %s",
		       std::to_string(GetLastError()).c_str());
	}

	spaceObjects.erase(this->base);
	delete this;
}


void SpaceObject::SetupDefaults()
{
	
	// The path to the save file for the base
	if(path.empty())
	{
		char datapath[MAX_PATH];
		GetUserDataPath(datapath);

		char tpath[1024];
		sprintf(tpath, R"(%s\Accts\MultiPlayer\spawned_solars\objects\object_%08x.ini)", datapath, base);
		this->path = tpath;

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


void SpaceObject::Load(const string& path)
{
	INI_Reader ini;
	if (ini.open(path.c_str(), false))
	{
		while (ini.read_header())
		{
			if (ini.is_header("SolarObj"))
			{
				while (ini.read_value())
				{
					// A value used to ensure that we place each infocardPara ini value into the correct array index
					static int lastInfocardParaLoaded = 0;

					if (ini.is_value("nickname"))
					{
						this->nickname = ini.get_value_string();
					}
					else if (ini.is_value("nickname_readable"))
					{
						this->basename = stows(ini.get_value_string());
					}
					else if (ini.is_value("objsolar"))
					{
						this->archetype = ini.get_value_string();
					}
					else if (ini.is_value("objloadout"))
					{
						this->loadout = ini.get_value_string();
					}
					else if (ini.is_value("affiliation"))
					{
						this->affiliation = ini.get_value_int(0);
					}
					else if (ini.is_value("system"))
					{
						this->system = ini.get_value_int(0);
					}
					else if (ini.is_value("pos"))
					{
						Vector objPos{};
						objPos.x = ini.get_value_float(0);
						objPos.y = ini.get_value_float(1);
						objPos.z = ini.get_value_float(2);

						this->position = objPos;
					}
					else if (ini.is_value("rot"))
					{
						Vector objRot{};
						objRot.x = ini.get_value_float(0);
						objRot.y = ini.get_value_float(1);
						objRot.z = ini.get_value_float(2);

						this->rotation = EulerMatrix(objRot);
					}
					else if (ini.is_value("infoname"))
					{
						this->infocard = stows(ini.get_value_string());
					}
					else if (ini.is_value("infocardpara"))
					{
						if (stows(ini.get_value_string()).length() <= 0) continue; // Break if the string is empty
						this->infocard_para[lastInfocardParaLoaded] = stows(ini.get_value_string());
						lastInfocardParaLoaded++;
					}
					else if (ini.is_value("maxhealth"))
					{
						this->maximumHealth = ini.get_value_int(0);
					}
					else if (ini.is_value("currenthealth"))
					{
						this->currentHealth = ini.get_value_int(0);
					}
				}
			}
		}

		// Set the object path to the one it was just loaded from
		this->path = path;

		// Create a random save timer value in milliseconds
		this->saveTimer = (rand() % 60) * 1000;

		// Hash the nickname to get the internal base id
		this->base = CreateID(this->nickname.c_str());

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

		if (debuggingMode >= 1)
		{
			ConPrint(L"SolarController: SpaceObj::Loaded base: %s settings from file\n", stows(this->nickname));
		}
	}
	ini.close();
}

void SpaceObject::Save()
{
	const auto file = fopen(path.c_str(), "w");
	if(file)
	{
		fprintf(file, "[SolarObj]\n");
		fprintf(file, "nickname = %s\n", nickname.c_str());
		fprintf(file, "nickname_readable = %s\n", wstos(basename).c_str());
		fprintf(file, "objsolar = %s\n", archetype.c_str());
		fprintf(file, "objloadout = %s\n", loadout.c_str());
		fprintf(file, "affiliation = %u\n", affiliation);
		fprintf(file, "system = %u\n", system);
		
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
	ConPrint(L"SolarController: Write operation: Closing file\n");
	fclose(file);
}

string SpaceObject::CreateObjectNickname(const string& objname)
{
	return string("so_") + objname;
}

float SpaceObject::SpaceObjDamaged(uint space_obj, uint attacking_space_obj, float curr_hitpoints, float damage)
{
	// Make sure that the attacking player is hostile
	const uint client = HkGetClientIDByShip(attacking_space_obj);
	if(client)
	{
		pub::Reputation::SetAttitude(attacking_space_obj, space_obj, -1.0f);
	}

	// Given that this function is for simple SpaceObjects, and is only used for rep purposes, we do not do any damage value manipulations.
	return damage;
}

void SpaceObject::SpaceObjDestroyed()
{
	// Do something
}

float SpaceObject::GetAttitudeTowardsClient(uint client)
{
	// Default to a neutral affiliation
	float attitude = 0.0;
	wstring charname = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));

	// If an affiliation for the object is set, get the attitude difference between the obj and player
	if(affiliation)
	{
		int rep;
		pub::Player::GetRep(client, rep);
		pub::Reputation::GetGroupFeelingsTowards(rep, affiliation, attitude);
	}

	return attitude;
}

void SpaceObject::SyncReputationForClientShip(uint ship, uint client, uint affiliation)
{
	int player_rep;
	pub::SpaceObj::GetRep(ship, player_rep);

	uint system;
	pub::SpaceObj::GetSystem(ship, system);

	for (auto& spaceObject : spaceObjects)
	{
		if (spaceObject.second->system == system)
		{
			const float attitude = spaceObject.second->GetAttitudeTowardsClient(client);
			if (debuggingMode > 0)
				ConPrint(L"SolarController: SyncReputationForClientShip:: ship=%u attitude=%f obj=%08x\n", ship, attitude, spaceObject.first);

			pub::Reputation::SetAttitude(affiliation, player_rep, attitude);
		}
	}
}

void SpaceObject::SyncReputationForBaseObject(uint space_obj)
{
	struct PlayerData *pd = 0;
	while (pd = Players.traverse_active(pd))
	{
		if (pd->iShipID && pd->iSystemID == system)
		{
			int player_rep;
			pub::SpaceObj::GetRep(pd->iShipID, player_rep);
			float attitude = GetAttitudeTowardsClient(pd->iOnlineID);

			int obj_rep;
			pub::SpaceObj::GetRep(space_obj, obj_rep);
			pub::Reputation::SetAttitude(obj_rep, player_rep, attitude);
		}
	}
}



pub::AI::SetPersonalityParams SpaceObject::MakePersonality()
{
	pub::AI::SetPersonalityParams p;
	p.state_graph = pub::StateGraph::get_state_graph("NOTHING", pub::StateGraph::TYPE_STANDARD);
	p.state_id = true;

	p.personality.EvadeDodgeUse.evade_dodge_style_weight[0] = 0.4f;
	p.personality.EvadeDodgeUse.evade_dodge_style_weight[1] = 0.0f;
	p.personality.EvadeDodgeUse.evade_dodge_style_weight[2] = 0.4f;
	p.personality.EvadeDodgeUse.evade_dodge_style_weight[3] = 0.2f;
	p.personality.EvadeDodgeUse.evade_dodge_cone_angle = 1.5708f;
	p.personality.EvadeDodgeUse.evade_dodge_interval_time = 10.0f;
	p.personality.EvadeDodgeUse.evade_dodge_time = 1.0f;
	p.personality.EvadeDodgeUse.evade_dodge_distance = 75.0f;
	p.personality.EvadeDodgeUse.evade_activate_range = 100.0f;
	p.personality.EvadeDodgeUse.evade_dodge_roll_angle = 1.5708f;
	p.personality.EvadeDodgeUse.evade_dodge_waggle_axis_cone_angle = 1.5708f;
	p.personality.EvadeDodgeUse.evade_dodge_slide_throttle = 1.0f;
	p.personality.EvadeDodgeUse.evade_dodge_turn_throttle = 1.0f;
	p.personality.EvadeDodgeUse.evade_dodge_corkscrew_roll_flip_direction = true;
	p.personality.EvadeDodgeUse.evade_dodge_interval_time_variance_percent = 0.5f;
	p.personality.EvadeDodgeUse.evade_dodge_cone_angle_variance_percent = 0.5f;
	p.personality.EvadeDodgeUse.evade_dodge_direction_weight[0] = 0.25f;
	p.personality.EvadeDodgeUse.evade_dodge_direction_weight[1] = 0.25f;
	p.personality.EvadeDodgeUse.evade_dodge_direction_weight[2] = 0.25f;
	p.personality.EvadeDodgeUse.evade_dodge_direction_weight[3] = 0.25f;

	p.personality.EvadeBreakUse.evade_break_roll_throttle = 1.0f;
	p.personality.EvadeBreakUse.evade_break_time = 1.0f;
	p.personality.EvadeBreakUse.evade_break_interval_time = 10.0f;
	p.personality.EvadeBreakUse.evade_break_afterburner_delay = 0.0f;
	p.personality.EvadeBreakUse.evade_break_turn_throttle = 1.0f;
	p.personality.EvadeBreakUse.evade_break_direction_weight[0] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_direction_weight[1] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_direction_weight[2] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_direction_weight[3] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_style_weight[0] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_style_weight[1] = 1.0f;
	p.personality.EvadeBreakUse.evade_break_style_weight[2] = 1.0f;

	p.personality.BuzzHeadTowardUse.buzz_min_distance_to_head_toward = 500.0f;
	p.personality.BuzzHeadTowardUse.buzz_min_distance_to_head_toward_variance_percent = 0.25f;
	p.personality.BuzzHeadTowardUse.buzz_max_time_to_head_away = 1.0f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_engine_throttle = 1.0f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_turn_throttle = 1.0f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_roll_throttle = 1.0f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_turn_throttle = 1.0f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_cone_angle = 1.5708f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_cone_angle_variance_percent = 0.5f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_waggle_axis_cone_angle = 0.3491f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_roll_angle = 1.5708f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_interval_time = 10.0f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_interval_time_variance_percent = 0.5f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_direction_weight[0] = 0.25f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_direction_weight[1] = 0.25f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_direction_weight[2] = 0.25f;
	p.personality.BuzzHeadTowardUse.buzz_dodge_direction_weight[3] = 0.25f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_style_weight[0] = 0.33f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_style_weight[1] = 0.33f;
	p.personality.BuzzHeadTowardUse.buzz_head_toward_style_weight[2] = 0.33f;

	p.personality.BuzzPassByUse.buzz_distance_to_pass_by = 1000.0f;
	p.personality.BuzzPassByUse.buzz_pass_by_time = 1.0f;
	p.personality.BuzzPassByUse.buzz_break_direction_cone_angle = 1.5708f;
	p.personality.BuzzPassByUse.buzz_break_turn_throttle = 1.0f;
	p.personality.BuzzPassByUse.buzz_drop_bomb_on_pass_by = true;
	p.personality.BuzzPassByUse.buzz_break_direction_weight[0] = 1.0f;
	p.personality.BuzzPassByUse.buzz_break_direction_weight[1] = 1.0f;
	p.personality.BuzzPassByUse.buzz_break_direction_weight[2] = 1.0f;
	p.personality.BuzzPassByUse.buzz_break_direction_weight[3] = 1.0f;
	p.personality.BuzzPassByUse.buzz_pass_by_style_weight[2] = 1.0f;

	p.personality.TrailUse.trail_lock_cone_angle = 0.0873f;
	p.personality.TrailUse.trail_break_time = 0.5f;
	p.personality.TrailUse.trail_min_no_lock_time = 0.1f;
	p.personality.TrailUse.trail_break_roll_throttle = 1.0f;
	p.personality.TrailUse.trail_break_afterburner = true;
	p.personality.TrailUse.trail_max_turn_throttle = 1.0f;
	p.personality.TrailUse.trail_distance = 100.0f;

	p.personality.StrafeUse.strafe_run_away_distance = 100.0f;
	p.personality.StrafeUse.strafe_attack_throttle = 1.0f;

	p.personality.EngineKillUse.engine_kill_search_time = 0.0f;
	p.personality.EngineKillUse.engine_kill_face_time = 1.0f;
	p.personality.EngineKillUse.engine_kill_use_afterburner = true;
	p.personality.EngineKillUse.engine_kill_afterburner_time = 2.0f;
	p.personality.EngineKillUse.engine_kill_max_target_distance = 100.0f;

	p.personality.RepairUse.use_shield_repair_pre_delay = 0.0f;
	p.personality.RepairUse.use_shield_repair_post_delay = 1.0f;
	p.personality.RepairUse.use_shield_repair_at_damage_percent = 0.2f;
	p.personality.RepairUse.use_hull_repair_pre_delay = 0.0f;
	p.personality.RepairUse.use_hull_repair_post_delay = 1.0f;
	p.personality.RepairUse.use_hull_repair_at_damage_percent = 0.2f;

	p.personality.GunUse.gun_fire_interval_time = 0.5f;
	p.personality.GunUse.gun_fire_interval_variance_percent = 0.0f;
	p.personality.GunUse.gun_fire_burst_interval_time = 0.5f;
	p.personality.GunUse.gun_fire_burst_interval_variance_percent = 0.0f;
	p.personality.GunUse.gun_fire_no_burst_interval_time = 0.0f;
	p.personality.GunUse.gun_fire_accuracy_cone_angle = 0.00001f;
	p.personality.GunUse.gun_fire_accuracy_power = 100.0f;
	p.personality.GunUse.gun_range_threshold = 1.0f;
	p.personality.GunUse.gun_target_point_switch_time = 0.0f;
	p.personality.GunUse.fire_style = 0;
	p.personality.GunUse.auto_turret_interval_time = 0.5f;
	p.personality.GunUse.auto_turret_burst_interval_time = 0.5f;
	p.personality.GunUse.auto_turret_no_burst_interval_time = 0.0f;
	p.personality.GunUse.auto_turret_burst_interval_variance_percent = 0.0f;
	p.personality.GunUse.gun_range_threshold_variance_percent = 0.3f;
	p.personality.GunUse.gun_fire_accuracy_power_npc = 100.0f;

	p.personality.MineUse.mine_launch_interval = 8.0f;
	p.personality.MineUse.mine_launch_cone_angle = 0.7854f;
	p.personality.MineUse.mine_launch_range = 200.0f;

	p.personality.MissileUse.missile_launch_interval_time = 0.5f;
	p.personality.MissileUse.missile_launch_interval_variance_percent = 0.5f;
	p.personality.MissileUse.missile_launch_range = 800.0f;
	p.personality.MissileUse.missile_launch_cone_angle = 0.01745f;
	p.personality.MissileUse.missile_launch_allow_out_of_range = false;

	p.personality.DamageReaction.evade_break_damage_trigger_percent = 1.0f;
	p.personality.DamageReaction.evade_dodge_more_damage_trigger_percent = 0.25f;
	p.personality.DamageReaction.engine_kill_face_damage_trigger_percent = 1.0f;
	p.personality.DamageReaction.engine_kill_face_damage_trigger_time = 0.2f;
	p.personality.DamageReaction.roll_damage_trigger_percent = 0.4f;
	p.personality.DamageReaction.roll_damage_trigger_time = 0.2f;
	p.personality.DamageReaction.afterburner_damage_trigger_percent = 0.2f;
	p.personality.DamageReaction.afterburner_damage_trigger_time = 0.5f;
	p.personality.DamageReaction.brake_reverse_damage_trigger_percent = 1.0f;
	p.personality.DamageReaction.drop_mines_damage_trigger_percent = 0.25f;
	p.personality.DamageReaction.drop_mines_damage_trigger_time = 1.0f;
	p.personality.DamageReaction.fire_guns_damage_trigger_percent = 0.5f;
	p.personality.DamageReaction.fire_guns_damage_trigger_time = 0.5f;
	p.personality.DamageReaction.fire_missiles_damage_trigger_percent = 0.5f;
	p.personality.DamageReaction.fire_missiles_damage_trigger_time = 0.5f;

	p.personality.MissileReaction.evade_missile_distance = 800.0f;
	p.personality.MissileReaction.evade_break_missile_reaction_time = 1.0f;
	p.personality.MissileReaction.evade_slide_missile_reaction_time = 1.0f;
	p.personality.MissileReaction.evade_afterburn_missile_reaction_time = 1.0f;

	p.personality.CountermeasureUse.countermeasure_active_time = 5.0f;
	p.personality.CountermeasureUse.countermeasure_unactive_time = 0.0f;

	p.personality.FormationUse.force_attack_formation_active_time = 0.0f;
	p.personality.FormationUse.force_attack_formation_unactive_time = 0.0f;
	p.personality.FormationUse.break_formation_damage_trigger_percent = 0.01f;
	p.personality.FormationUse.break_formation_damage_trigger_time = 1.0f;
	p.personality.FormationUse.break_formation_missile_reaction_time = 1.0f;
	p.personality.FormationUse.break_apart_formation_missile_reaction_time = 1.0f;
	p.personality.FormationUse.break_apart_formation_on_evade_break = true;
	p.personality.FormationUse.break_formation_on_evade_break_time = 1.0f;
	p.personality.FormationUse.formation_exit_top_turn_break_away_throttle = 1.0f;
	p.personality.FormationUse.formation_exit_roll_outrun_throttle = 1.0f;
	p.personality.FormationUse.formation_exit_max_time = 5.0f;
	p.personality.FormationUse.formation_exit_mode = 1;

	p.personality.Job.wait_for_leader_target = false;
	p.personality.Job.maximum_leader_target_distance = 3000;
	p.personality.Job.flee_when_leader_flees_style = false;
	p.personality.Job.scene_toughness_threshold = 4;
	p.personality.Job.flee_scene_threat_style = 4;
	p.personality.Job.flee_when_hull_damaged_percent = 0.01f;
	p.personality.Job.flee_no_weapons_style = true;
	p.personality.Job.loot_flee_threshold = 1;
	p.personality.Job.attack_subtarget_order[0] = 5;
	p.personality.Job.attack_subtarget_order[1] = 6;
	p.personality.Job.attack_subtarget_order[2] = 7;
	p.personality.Job.field_targeting = 3;
	p.personality.Job.loot_preference = 1;
	p.personality.Job.combat_drift_distance = 15000;
	p.personality.Job.attack_order[0].distance = 5000;
	p.personality.Job.attack_order[0].type = 11;
	p.personality.Job.attack_order[0].flag = 15;
	p.personality.Job.attack_order[1].type = 12;

	return p;
}

SpaceObject* SpaceObject::GetSpaceobject(uint obj)
{
	const auto it = spaceObjects.find(obj);
	if (it != spaceObjects.end())
		return it->second;
	return nullptr;
}



////////////////////////////
// HOOK HANDLING
////////////////////////////

void SpaceObject::HkCb_AddDmgEntry(DamageList* dmg, unsigned short p1, float damage, DamageEntry::SubObjFate fate)
{

	// Handle if the invulernable flag is toggled
	if(this->invulnerable)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		iDmgToSpaceID = 0;
		return;
	}

	if (iDmgToSpaceID && dmg->get_inflictor_id())
	{
		float curr, max;
		pub::SpaceObj::GetHealth(iDmgToSpaceID, curr, max);

		// Debugging stuff
		if(debuggingMode == 2)
		{
			ConPrint(L"SolarController: HkCb_AddDmgEntry iDmgToSpaceID=%u get_inflictor_id=%u curr=%0.2f max=%0.0f damage=%0.2f cause=%u is_player=%u player_id=%u fate=%u\n",
				iDmgToSpaceID, dmg->get_inflictor_id(), curr, max, damage, dmg->get_cause(), dmg->is_inflictor_a_player(), dmg->get_inflictor_owner_player(), fate);
		}

		// A work around for an apparent bug where mines/missiles at the base
		// causes the base damage to jump down to 0 even if the base is
		// otherwise healthy.
		if (damage == 0.0f && curr> 200000)
		{
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			if (debuggingMode == 2)
				ConPrint(L"SolarController: HkCb_AddDmgEntry[1] - invalid damage?\n");
			return;
		}

		// If this is an NPC hit then suppress the call completely
		if (!dmg->is_inflictor_a_player())
		{
			if (debuggingMode == 2)
				ConPrint(L"SolarController: HkCb_AddDmgEntry[2] suppressed - npc\n");
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			iDmgToSpaceID = 0;
			return;
		}

		// This call is for us, skip all plugins.		
		const float newDamage = SpaceObjDamaged(iDmgToSpaceID, dmg->get_inflictor_id(), curr, damage);
		returncode = SKIPPLUGINS;

		if (newDamage != 0.0f)
		{
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			if (debuggingMode == 2)
				ConPrint(L"HkCb_AddDmgEntry[3] suppressed - shield up - new_damage=%0.0f\n", newDamage);
			dmg->add_damage_entry(p1, newDamage, fate);
			iDmgToSpaceID = 0;
			return;
		}
	}
}

// Handle the base being destroyed
void SpaceObject::ObjectDestroyed(uint space_obj, uint client)
{
	if (debuggingMode > 0)
		ConPrint(L"SolarController: SpaceObject::destroyed space_obj=%u\n", space_obj);

	pub::SpaceObj::LightFuse(space_obj, "player_base_explode_fuse", 0);
	spaceObjects.erase(space_obj);

	this->spaceobj = 0;

	//TODO: Decide if we need to send audio, and alert players that the object has been destroyed

	SpaceObject::DeleteObject();

}

void SpaceObject::DockCall(uint ship, uint base, int iCancel, enum DOCK_HOST_RESPONSE response)
{
	returncode = SKIPPLUGINS_NOFUNCTIONCALL;
}