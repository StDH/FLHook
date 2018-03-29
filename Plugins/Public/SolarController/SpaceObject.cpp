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

SpaceObject::~SpaceObject()
{
	// Do something
}

void SpaceObject::Spawn()
{

	if(!spaceobj)
	{
		pub::SpaceObj::SolarInfo si{};
		memset(&si, 0, sizeof(si));
		si.iFlag = 4;

		char archname[100];

		// Prepare the settings for the space object
		_snprintf(archname, sizeof(archname), this->archetype.c_str());
		si.iLoadoutID = CreateID(this->loadout.c_str());

		si.iHitPointsLeft = 1;
		si.iSystemID = system;
		si.mOrientation = rotation;
		si.vPos = position;
		si.Costume.head = CreateID("pi_pirate2_head");
		si.Costume.body = CreateID("pi_pirate8_body");
		si.Costume.lefthand = 0;
		si.Costume.righthand = 0;
		si.Costume.accessories = 0;
		si.iVoiceID = CreateID("atc_leg_m01");
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
			HkChangeIDSString(pd->iOnlineID, this->solar_ids, basename);
		}

		// Set the base name
		FmtStr infoname(solar_ids, 0);
		infoname.begin_mad_lib(solar_ids); // scanner name
		infoname.end_mad_lib();

		FmtStr infocard(solar_ids, 0);
		infocard.begin_mad_lib(solar_ids); // infocard
		infocard.end_mad_lib();
		pub::Reputation::Alloc(si.iRep, infoname, infocard);

		// Spawn the solar object
		SpawnSolar(spaceobj, si);

		// Set the health for the Space Object to be it's maximum by default
		pub::SpaceObj::SetRelativeHealth(spaceobj, this->maximumHealth);
		if (debuggingMode)
			ConPrint(L"SpaceObj::created space_obj=%u health=%f\n", spaceobj, this->maximumHealth);

		pub::AI::SetPersonalityParams pers = MakePersonality();
		pub::AI::SubmitState(spaceobj, &pers);

	}

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


void SpaceObject::Load()
{
	// Todo
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

string SpaceObject::CreateBaseNickname(const string& basename)
{
	return "";
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
			if (debuggingMode)
				ConPrint(L"SyncReputationForClientShip:: ship=%u attitude=%f obj=%08x\n", ship, attitude, spaceObject.first);

			pub::Reputation::SetAttitude(affiliation, player_rep, attitude);
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

////////////////////////////
// HOOK HANDLING
////////////////////////////

// Handle the base being destroied
void BaseDestroyed(uint space_obj, uint client)
{
	
}