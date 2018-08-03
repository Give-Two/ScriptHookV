#include "script.h"
#include "keyboard.h"

bool is_menu_open = false;
bool playerControl = false;
bool show_test_window = false;
bool show_another_window = false;
ImVec4 clear_col = ImColor(114, 144, 154);
HWND					 windowHandle = (HWND)0;
ID3D11Device*			 pDevice = nullptr;
IDXGISwapChain*			 pSwapChain = nullptr;
ID3D11DeviceContext*	 pDeviceContext = nullptr;
ID3D11RenderTargetView*  pRenderTargetView = nullptr;

std::deque<std::function<void()>> g_nativeQueue;
std::vector<std::tuple<std::function<bool()>, std::function<void()>>> g_callResults;

void CallOnResult(const std::function<bool()>& result, const std::function<void()>& call)
{
	g_callResults.push_back({ result, call });
}

bool trainer_switch_pressed()
{
	return IsKeyJustUp(VK_NUMPAD0);
}

void menu_beep()
{
	AUDIO::PLAY_SOUND_FRONTEND(-1, "NAV_UP_DOWN", "HUD_FRONTEND_DEFAULT_SOUNDSET", 0);
}

std::string statusText;
DWORD statusTextDrawTicksMax;
bool statusTextGxtEntry;

void update_status_text()
{
	if (GetTickCount() < statusTextDrawTicksMax)
	{
		UI::SET_TEXT_FONT(0);
		UI::SET_TEXT_SCALE(0.55, 0.55);
		UI::SET_TEXT_COLOUR(255, 255, 255, 255);
		UI::SET_TEXT_WRAP(0.0, 1.0);
		UI::SET_TEXT_CENTRE(1);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		if (statusTextGxtEntry)
		{
			UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT((char *)statusText.c_str());
		} else
		{
			UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
			UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char *)statusText.c_str());
		}
		UI::END_TEXT_COMMAND_DISPLAY_TEXT(0.5, 0.5, 0);
	}
}

void set_status_text(std::string str, DWORD time = 2500, bool isGxtEntry = false)
{
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time;
	statusTextGxtEntry = isGxtEntry;
}

// features
bool featurePlayerInvincible			=	false;
bool featurePlayerInvincibleUpdated		=	false;
bool featurePlayerNeverWanted			=	false;
bool featurePlayerIgnored				=	false;
bool featurePlayerIgnoredUpdated		=	false;
bool featurePlayerUnlimitedAbility		=	false;
bool featurePlayerNoNoise				=	false;
bool featurePlayerNoNoiseUpdated		=	false;
bool featurePlayerFastSwim				=	false;
bool featurePlayerFastSwimUpdated		=	false;
bool featurePlayerFastRun				=	false;
bool featurePlayerFastRunUpdated		=	false;
bool featurePlayerSuperJump				=	false;

bool featureWeaponNoReload				=	false;
bool featureWeaponFireAmmo				=	false;
bool featureWeaponTpImpact				=	false;
bool featureWeaponExplosiveAmmo			=	false;
bool featureWeaponExplosiveMelee		=	false;
bool featureWeaponVehRockets			=	false;

DWORD featureWeaponVehShootLastTime		=	0;

bool featureVehInvincible				=	false;
bool featureVehInvincibleUpdated		=	false;
bool featureVehInvincibleWheels			=	false;
bool featureVehInvincibleWheelsUpdated	=	false;
bool featureVehSeatbelt					=	false;
bool featureVehSeatbeltUpdated			=	false;
bool featureVehSpeedBoost				=	false;
bool featureVehWrapInSpawned			=	false;

bool featureWorldMoonGravity			=	false;
bool featureWorldRandomCops				=	true;
bool featureWorldRandomTrains			=	true;
bool featureWorldRandomBoats			=	true;
bool featureWorldGarbageTrucks			=	true;

bool featureTimePaused					=	false;
bool featureTimePausedUpdated			=	false;
bool featureTimeSynced					=	false;

bool featureWeatherWind					=	false;
bool featureWeatherPers					=	false;

bool featureMiscLockRadio				=	false;
bool featureMiscHideHud					=	false;

bool skinchanger_used					=	false;

bool vehicleSpawnWarpInto				= false;
bool vehicleSpawnUpgrade				= false;

int ProcessPlayerButton					=	-1;
int ProcessWeaponButton					=	-1;
int ProcessVehicleButton				=	-1;
int ProcessWorldButton					=	-1;
int ProcessTimeButton					=	-1;
int ProcessWeatherButton				=	-1;
int ProcessMiscButton					=	-1;

std::string currentWeatherStatus;

bool LoadModel(Hash model, int TimeOut)
{
	if (!STREAMING::IS_MODEL_VALID(model)) return false;
	STREAMING::REQUEST_MODEL(model);
	if (TimeOut > -1)
	{
		DWORD64 now = GetTickCount64();
		while (!STREAMING::HAS_MODEL_LOADED(model) && GetTickCount64() < now + TimeOut) WAIT(0);
	}
	return STREAMING::HAS_MODEL_LOADED(model) == TRUE;
}

bool LoadPedModel(Hash model, int TimeOut)
{
	if (!STREAMING::IS_MODEL_VALID(model)) return false;
	STREAMING::REQUEST_MENU_PED_MODEL(model);
	if (TimeOut > -1)
	{
		DWORD64 now = GetTickCount64();
		while (!STREAMING::HAS_MODEL_LOADED(model) && GetTickCount64() < now + TimeOut) WAIT(0);
	}
	return STREAMING::HAS_MODEL_LOADED(model) == TRUE;
}

// player model control, switching on normal ped model when needed	
void check_player_model(Hash skin) 
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();	
	
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

	if (skin)
	{
		Vehicle veh = NULL; int seat = -1;
		auto inVeh = PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0);
		if (inVeh)
		{
			veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
			Hash vehModel = ENTITY::GET_ENTITY_MODEL(veh);
			for (auto i = -1; i < VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(vehModel); ++i)
			{
				if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, i, 0) == playerPed) seat = i;
			}
		}

		if (LoadPedModel(skin, 2000))
		{
			PED::CLEAR_ALL_PED_PROPS(playerPed);
			PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), skin);
			playerPed = PLAYER::PLAYER_PED_ID();
			PED::SET_PED_DEFAULT_COMPONENT_VARIATION(playerPed);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(skin);
			if (inVeh) PED::SET_PED_INTO_VEHICLE(playerPed, veh, seat);
		}
	}
	else if (ENTITY::IS_ENTITY_DEAD(playerPed, 0) || PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE))
	{
		static Hash model = ENTITY::GET_ENTITY_MODEL(playerPed);

		if (model != GAMEPLAY::GET_HASH_KEY("player_zero") &&
			model != GAMEPLAY::GET_HASH_KEY("player_one") &&
			model != GAMEPLAY::GET_HASH_KEY("player_two"))
		{
			set_status_text("turning to normal");
			WAIT(1000);

			const Hash defaultModel = GAMEPLAY::GET_HASH_KEY("player_zero");

			if (LoadPedModel(defaultModel, 2000))
			{
				PED::CLEAR_ALL_PED_PROPS(playerPed);
				PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), defaultModel);
				playerPed = PLAYER::PLAYER_PED_ID();
				PED::SET_PED_DEFAULT_COMPONENT_VARIATION(playerPed);
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(defaultModel);
			}

			// wait until player is ressurected
			while (ENTITY::IS_ENTITY_DEAD(PLAYER::PLAYER_PED_ID(), 0) || PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE))
				WAIT(0);

			if (LoadPedModel(model, 2000))
			{
				PED::CLEAR_ALL_PED_PROPS(playerPed);
				PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), model);
				playerPed = PLAYER::PLAYER_PED_ID();
				PED::SET_PED_DEFAULT_COMPONENT_VARIATION(playerPed);
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
			}
		}
		else skinchanger_used = false;
	}
}

void teleport_to_location(Vector3 coords)
{
	// get entity to teleport
	Entity entity = PLAYER::PLAYER_PED_ID();
	if (PED::IS_PED_IN_ANY_VEHICLE(entity, 0))
		entity = PED::GET_VEHICLE_PED_IS_USING(entity);

	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(entity, coords.x, coords.y, coords.z, 0, 0, 1);

	WAIT(0);
	set_status_text("teleported");
}

void PaintVehRandom(Vehicle veh)
{
	if (!ENTITY::DOES_ENTITY_EXIST(veh)) return;

	int mainColor = GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, 160);
	int extraColor = GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, 160);

	VEHICLE::SET_VEHICLE_MOD_KIT(veh, 0);

	VEHICLE::SET_VEHICLE_MOD_COLOR_1(veh, 0, 0, 0);
	VEHICLE::SET_VEHICLE_MOD_COLOR_2(veh, 0, 0);

	VEHICLE::SET_VEHICLE_COLOURS(veh, mainColor, mainColor);

	VEHICLE::SET_VEHICLE_EXTRA_COLOURS(veh, extraColor, extraColor);

	VEHICLE::_SET_VEHICLE_DASHBOARD_COLOUR(veh, extraColor);

	VEHICLE::_SET_VEHICLE_INTERIOR_COLOUR(veh, extraColor);
}

void UpgradeVehMaximum(Vehicle veh, bool notify)
{
	if (!ENTITY::DOES_ENTITY_EXIST(veh)) return;
	int color[3] = { GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, 255), GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, 255), GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, 255) };
	int wheelType = VehicleWheelTypeStock;
	Hash vehicleModel = ENTITY::GET_ENTITY_MODEL(veh);
	if (VEHICLE::IS_THIS_MODEL_A_BIKE(vehicleModel)) wheelType = VehicleWheelTypeBikeWheels;
	else if (VEHICLE::GET_NUM_VEHICLE_MODS(veh, VehicleModHydraulics) > 0) wheelType = VehicleWheelTypeSuperMod;
	else wheelType = VehicleWheelTypeHighEnd;
	const static std::string WHEEL_CATEGORY_NAMES[] = { "Sports", "Muscle", "Lowrider", "SUV", "Offroad", "Tuner", "High End", "Super Mod" };
	const static int WHEEL_CATEGORY_COUNTS[] = { 25, 18, 15, 19, 10, 24, 20, 217 };
	auto rdmWheels = GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, WHEEL_CATEGORY_COUNTS[wheelType - 1]);

	//mods

	for (int i = -1; i < 49; i++)
	{
		int mods = VEHICLE::GET_NUM_VEHICLE_MODS(veh, i);
		VEHICLE::SET_VEHICLE_MOD_KIT(veh, 0);
		switch (i)
		{
		case VehicleModHorns:
			VEHICLE::SET_VEHICLE_MOD(veh, i, GAMEPLAY::GET_RANDOM_INT_IN_RANGE(-1, 48), 0);
			break;
		case VehicleToggleModTurbo:
			VEHICLE::TOGGLE_VEHICLE_MOD(veh, i, TRUE);
			break;
		case VehicleToggleModTireSmoke:
			VEHICLE::TOGGLE_VEHICLE_MOD(veh, i, TRUE);
			VEHICLE::SET_VEHICLE_MOD(veh, 20, 1, 0);
			VEHICLE::SET_VEHICLE_TYRE_SMOKE_COLOR(veh, color[0], color[1], color[2]);
			break;
		case VehicleToggleModXenonHeadlights:
			VEHICLE::TOGGLE_VEHICLE_MOD(veh, i, TRUE);
			break;
		case VehicleModFrontWheels:
			VEHICLE::SET_VEHICLE_WHEEL_TYPE(veh, wheelType);
			VEHICLE::SET_VEHICLE_MOD(veh, i, rdmWheels, 1);
			VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(veh, FALSE);
			break;
		case VehicleModBackWheels:
			VEHICLE::SET_VEHICLE_WHEEL_TYPE(veh, wheelType);
			VEHICLE::SET_VEHICLE_MOD(veh, i, rdmWheels, 1);
			VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(veh, FALSE);
			break;
		case VehicleModLiveries:
			if (mods > 0) {
				VEHICLE::SET_VEHICLE_MOD(veh, i,
					mods - GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, mods + 1), 1);
			}
			VEHICLE::SET_VEHICLE_LIVERY(veh, VEHICLE::GET_VEHICLE_LIVERY_COUNT(veh) - GAMEPLAY::GET_RANDOM_INT_IN_RANGE(0, VEHICLE::GET_VEHICLE_LIVERY_COUNT(veh)) + 1);
			break;
		default:
			if (mods > 0)
				VEHICLE::SET_VEHICLE_MOD(veh, i, mods - 1, 1);
			break;
		}

		//Extra Stuff
		PaintVehRandom(veh);

		VEHICLE::SET_VEHICLE_WINDOW_TINT(veh, VehicleWindowTintPureBlack); //Window Tint

		for (int i = 0; i <= 3; i++)
			VEHICLE::_SET_VEHICLE_NEON_LIGHT_ENABLED(veh, i, TRUE); // Neon Lights
		VEHICLE::_SET_VEHICLE_NEON_LIGHTS_COLOUR(veh, color[0], color[1], color[2]);

		if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(veh, 0))
			VEHICLE::LOWER_CONVERTIBLE_ROOF(veh, TRUE); //Convertible

		for (int i = 1; i < 10; i++) //Extras - roof etc..
		{
			if (!VEHICLE::DOES_EXTRA_EXIST(veh, i)) continue;
			VEHICLE::SET_VEHICLE_EXTRA(veh, i, VEHICLE::IS_VEHICLE_EXTRA_TURNED_ON(veh, i) ? 0 : 1);
		}

		VEHICLE::SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(veh, NumberPlateTypeYellowOnBlack); // -- NumberPlates

																						 //number plate text - 8 chars max
		std::string tmp(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(veh)));
		VEHICLE::SET_VEHICLE_NUMBER_PLATE_TEXT(veh, (char *)tmp.substr(0, 8).c_str());

		if (VEHICLE::_IS_VEHICLE_DAMAGED(veh))
			VEHICLE::SET_VEHICLE_FIXED(veh);
		VEHICLE::SET_VEHICLE_DIRT_LEVEL(veh, 0);
	}

	if (notify)
		set_status_text("~b~Vehicle ~s~Upgraded");
}

void SpawnVehicle(Hash vehicleHash)
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	if (bPlayerExists)
	{

		bool isHeli = VEHICLE::IS_THIS_MODEL_A_HELI(vehicleHash);
		bool isPlane = VEHICLE::IS_THIS_MODEL_A_PLANE(vehicleHash);
		bool isAircraft = isHeli || isPlane;

		Vector3 modelMin, modelMax;
		GAMEPLAY::GET_MODEL_DIMENSIONS(vehicleHash, &modelMin, &modelMax);
		float length = (modelMax.y - modelMin.y) * 2;

		float speed = ENTITY::GET_ENTITY_SPEED(playerPed);
		Vector3 velocity = ENTITY::GET_ENTITY_VELOCITY(playerPed);
		auto forwardPos = ENTITY::GET_ENTITY_FORWARD_VECTOR(playerPed);
		auto playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, 0);
		auto spawnPos = Vector3(playerPos.x + (forwardPos.x * length), playerPos.y + (forwardPos.y * length), playerPos.z + (forwardPos.z * length));
		float heading = ENTITY::GET_ENTITY_HEADING(playerPed);
		if (isAircraft && vehicleSpawnWarpInto) spawnPos.z = std::max(spawnPos.z + 500, spawnPos.z);

		Vehicle vehicle = VEHICLE::CREATE_VEHICLE(vehicleHash, spawnPos.x, spawnPos.y, spawnPos.z, heading, TRUE, TRUE, 0);

		if (ENTITY::DOES_ENTITY_EXIST(vehicle))
		{
			if (vehicleSpawnWarpInto)
			{
				PED::SET_PED_INTO_VEHICLE(playerPed, vehicle, -1);
				VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, TRUE, TRUE, FALSE);
				ENTITY::SET_ENTITY_VELOCITY(vehicle, velocity.x, velocity.y, std::max(-5.f, velocity.z));
				VEHICLE::SET_VEHICLE_FORWARD_SPEED(vehicle, speed);

				if (isAircraft)
				{
					if (isHeli) VEHICLE::SET_HELI_BLADES_FULL_SPEED(vehicle);
					else if (isPlane) VEHICLE::CONTROL_LANDING_GEAR(vehicle, 3);
					VEHICLE::SET_VEHICLE_FORWARD_SPEED(vehicle, 500.0f);
				}
				else VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(vehicle, 0);
			}
			else if (!isAircraft) VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(vehicle, 0);

			if (vehicleSpawnUpgrade)
			{
				UpgradeVehMaximum(vehicle, false);
			}

			set_status_text(FMT("~b~Spawned %s", UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(vehicleHash))));
			ENTITY::SET_VEHICLE_AS_NO_LONGER_NEEDED(&vehicle);
		}

		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(vehicleHash);
	}
}


void update_vehicle_guns()
{
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();	

	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !featureWeaponVehRockets) return;

	bool bSelect = IsKeyDown(0x6B); // num plus
	if (bSelect && featureWeaponVehShootLastTime + 150 < GetTickCount() &&
		PLAYER::IS_PLAYER_CONTROL_ON(player) &&	PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
	{
		Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);

		Vector3 v0, v1;
		GAMEPLAY::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(veh), &v0, &v1);

		Hash weaponAssetRocket = GAMEPLAY::GET_HASH_KEY("WEAPON_VEHICLE_ROCKET");
		if (!WEAPON::HAS_WEAPON_ASSET_LOADED(weaponAssetRocket))
		{
			WEAPON::REQUEST_WEAPON_ASSET(weaponAssetRocket, 31, 0);
			while (!WEAPON::HAS_WEAPON_ASSET_LOADED(weaponAssetRocket))
				WAIT(0);
		}

		Vector3 coords0from = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -(v1.x + 0.25f), v1.y + 1.25f, 0.1);
		Vector3 coords1from = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh,  (v1.x + 0.25f), v1.y + 1.25f, 0.1);
		Vector3 coords0to = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, -v1.x, v1.y + 100.0f, 0.1f);
		Vector3 coords1to = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh,  v1.x, v1.y + 100.0f, 0.1f);

		GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords0from.x, coords0from.y, coords0from.z, 
													 coords0to.x, coords0to.y, coords0to.z, 
													 250, 1, weaponAssetRocket, playerPed, 1, 0, -1.0);
		GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(coords1from.x, coords1from.y, coords1from.z, 
													 coords1to.x, coords1to.y, coords1to.z, 
													 250, 1, weaponAssetRocket, playerPed, 1, 0, -1.0);

		featureWeaponVehShootLastTime = GetTickCount();
	}
}

void setGameInputToEnabled(bool enabled)
{
	if (!enabled)
	{
		CONTROLS::ENABLE_ALL_CONTROL_ACTIONS(1);
	}
	else if (enabled)
	{
		CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(1);
	}
}

void ProcessPlayerButtons()
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	menu_beep();

	switch (ProcessPlayerButton)
	{
	case 0:// fix player
	{
		ENTITY::SET_ENTITY_HEALTH(playerPed, ENTITY::GET_ENTITY_MAX_HEALTH(playerPed));
		PED::ADD_ARMOUR_TO_PED(playerPed, PLAYER::GET_PLAYER_MAX_ARMOUR(player) - PED::GET_PED_ARMOUR(playerPed));
		if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
		{
			Vehicle playerVeh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
			if (ENTITY::DOES_ENTITY_EXIST(playerVeh) && !ENTITY::IS_ENTITY_DEAD(playerVeh, 0))
				VEHICLE::SET_VEHICLE_FIXED(playerVeh);
		}
		set_status_text("player fixed");
	}
	break;

	case 1:// reset model skin
	{
		PED::SET_PED_DEFAULT_COMPONENT_VARIATION(playerPed);
		set_status_text("using default model skin");
	}
	break;

	case 2:// add cash
		for (int i = 0; i < 3; i++)
		{
			char statNameFull[32];
			sprintf_s(statNameFull, "SP%d_TOTAL_CASH", i);
			Hash hash = GAMEPLAY::GET_HASH_KEY(statNameFull);
			int val;
			STATS::STAT_GET_INT(hash, &val, -1);
			val += 1000000;
			STATS::STAT_SET_INT(hash, val, 1);
		}
		set_status_text("cash added");
		break;

	case 3:// wanted up
		if (bPlayerExists && PLAYER::GET_PLAYER_WANTED_LEVEL(player) < 5)
		{
			PLAYER::SET_PLAYER_WANTED_LEVEL(player, PLAYER::GET_PLAYER_WANTED_LEVEL(player) + 1, 0);
			PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, 0);
			set_status_text("wanted up");
		}
		break;

	case 4:// wanted down
		if (bPlayerExists && PLAYER::GET_PLAYER_WANTED_LEVEL(player) > 0)
		{
			PLAYER::SET_PLAYER_WANTED_LEVEL(player, PLAYER::GET_PLAYER_WANTED_LEVEL(player) - 1, 0);
			PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, 0);
			set_status_text("wanted down");
		}
		break;
	default:
		break;
	}
	ProcessPlayerButton = -1;
}

struct DlcComponentData{
int attachBone;
int padding1;
int bActiveByDefault;
int padding2;
int unk;
int padding3;
int componentHash;
int padding4;
int unk2;
int padding5;
int componentCost;
int padding6;
char nameLabel[64];
char descLabel[64];
};

struct DlcWeaponData{
int emptyCheck; //use DLC1::_IS_DLC_DATA_EMPTY on this
int padding1;
int weaponHash;
int padding2;
int unk;
int padding3;
int weaponCost;
int padding4;
int ammoCost;
int padding5;
int ammoType;
int padding6;
int defaultClipSize;
int padding7;
char nameLabel[64];
char descLabel[64];
char desc2Label[64]; //usually "the" + name
char upperCaseNameLabel[64];
std::vector<DlcComponentData> ComponentData;
};

std::vector<DlcWeaponData> GetAllWeaponData()
{
	static std::vector<DlcWeaponData> data;

	if (data.empty())
	{
		for (int i = 0; i < FILES::GET_NUM_DLC_WEAPONS(); ++i)
		{
			DlcWeaponData weaponData;

			std::memset(&weaponData, 0, sizeof(weaponData));

			if (FILES::GET_DLC_WEAPON_DATA(i, (int*)&weaponData))
			{
				if (int componentIndex = FILES::GET_NUM_DLC_WEAPON_COMPONENTS(i))
				{
					DlcComponentData componentData;
					for (int j = 0; j < componentIndex; ++j)
					{
						if (FILES::GET_DLC_WEAPON_COMPONENT_DATA(i, j, (int*)&componentData))
						{
							weaponData.ComponentData.push_back(componentData);
						}
					}
				}
				if (!FILES::_IS_DLC_DATA_EMPTY(weaponData.emptyCheck)) {
					data.push_back(weaponData);
				}
			}
		}
	}

	return data;
}

void GivePedAllWeapons(Ped ped)
{
	if (!ENTITY::DOES_ENTITY_EXIST(ped)) return;
	for (const auto& weapon_data : GetAllWeaponData())
	{
		int maxAmmo;
		auto hash = weapon_data.weaponHash;
		auto component_data = weapon_data.ComponentData;

		WEAPON::GIVE_WEAPON_TO_PED(ped, hash, WEAPON::GET_MAX_AMMO(ped, hash, &maxAmmo) ? maxAmmo : 9999, FALSE, TRUE);
		for (const auto& comp : component_data)
		{
			if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(hash, comp.componentHash)) WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(ped, hash, comp.componentHash);
		}
		if (auto TintCount = WEAPON::GET_WEAPON_TINT_COUNT(hash) > 0)
		{
			WEAPON::SET_PED_WEAPON_TINT_INDEX(ped, hash, TintCount - 1);
		}
	}
}

void ProcessWeaponButtons()
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	menu_beep();

	switch (ProcessWeaponButton)
	{
	case 0:
		GivePedAllWeapons(playerPed);
		set_status_text("all weapon added");
		break;
	default:
		break;
	}
	ProcessWeaponButton = -1;
}

void ProcessVehicleButtons()
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	menu_beep();

	switch (ProcessVehicleButton)
	{
	case 0:
		if (bPlayerExists)
		{
			if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
			{
				Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
				PaintVehRandom(veh);
			}
			else
			{
				set_status_text("player isn't in a vehicle");
			}
		}
		break;
	case 1:
		if (bPlayerExists)
		{
			if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
			{
				Vehicle veh = PED::GET_VEHICLE_PED_IS_USING(playerPed);
				UpgradeVehMaximum(veh, true);
			}
			else
			{
				set_status_text("player isn't in a vehicle");
			}
		}
		break;

	case 2:
		if (bPlayerExists)
			if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
				VEHICLE::SET_VEHICLE_FIXED(PED::GET_VEHICLE_PED_IS_USING(playerPed));
			else
				set_status_text("player isn't in a vehicle");
		break;
	default:
		break;
	}
	ProcessVehicleButton = -1;
}

void ProcessWorldButtons()
{
	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	menu_beep();

	switch (ProcessWorldButton)
	{
	case 0:
		featureWorldMoonGravity = !featureWorldMoonGravity;
		GAMEPLAY::SET_GRAVITY_LEVEL(featureWorldMoonGravity ? 2 : 0);
		break;
	case 1:
		// featureWorldRandomCops being set in update_features
		PED::SET_CREATE_RANDOM_COPS(!featureWorldRandomCops);
		break;
	case 2:
		featureWorldRandomTrains = !featureWorldRandomTrains;
		VEHICLE::SET_RANDOM_TRAINS(featureWorldRandomTrains);
		break;
	case 3:
		featureWorldRandomBoats = !featureWorldRandomBoats;
		VEHICLE::SET_RANDOM_BOATS(featureWorldRandomBoats);
		break;
	case 4:
		featureWorldGarbageTrucks = !featureWorldGarbageTrucks;
		VEHICLE::SET_GARBAGE_TRUCKS(featureWorldGarbageTrucks);
		break;
	default:
		break;
	}
	ProcessWorldButton = -1;
}

void ProcessTimeButtons()
{
	menu_beep();

	switch (ProcessTimeButton)
	{
		// hour forward/backward
	case 0:
	case 1:
	{
		int h = TIME::GET_CLOCK_HOURS();
		if (ProcessTimeButton == 0) h = (h == 23) ? 0 : h + 1; else h = (h == 0) ? 23 : h - 1;
		int m = TIME::GET_CLOCK_MINUTES();
		TIME::SET_CLOCK_TIME(h, m, 0);
		char text[32];
		sprintf_s(text, "time %d:%d", h, m);
		set_status_text(text);
	}
	break;
	default:
		break;
	}
	ProcessTimeButton = -1;
}

void ProcessWeatherButtons()
{
	// common variables
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	menu_beep();

	switch (ProcessWeatherButton)
	{
		// wind
	case 0:
		featureWeatherWind = !featureWeatherWind;
		if (featureWeatherWind)
		{
			GAMEPLAY::SET_WIND(1.f);
			GAMEPLAY::SET_WIND_SPEED(11.99f);
			GAMEPLAY::SET_WIND_DIRECTION(bPlayerExists ? ENTITY::GET_ENTITY_HEADING(playerPed) : 0.f);
		}
		else
		{
			GAMEPLAY::SET_WIND(0.f);
			GAMEPLAY::SET_WIND_SPEED(0.f);
		}
		break;
		// set persist
	case 1:
		featureWeatherPers = !featureWeatherPers;
		break;
		// set weather
	default:
		GAMEPLAY::CLEAR_OVERRIDE_WEATHER();
		if (featureWeatherPers)
		{
			GAMEPLAY::SET_OVERRIDE_WEATHER(currentWeatherStatus.c_str());
		}
		else
		{
			GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST(currentWeatherStatus.c_str());
			GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
		}
		set_status_text(currentWeatherStatus);
	}
	ProcessWeatherButton = -1;
}

void ProcessMiscButtons()
{
	menu_beep();

	switch (ProcessMiscButton)
	{
	case 0: // next radio track
			AUDIO::SKIP_RADIO_FORWARD();
		break;
	default:
		break;
	}
	ProcessMiscButton = -1;
}

template<typename... Args>
std::vector<const char*> get_labels(std::vector<std::tuple<const char*, Args...>> items)
{
	std::vector<const char*> labels;
	for (const auto& tuple : items)
	{
		labels.push_back(std::get<0>(tuple));
	}
	return labels;
}

/* =================== Player Skins =================== */

const std::vector<std::tuple<const char*, Hash>> skins_players = { {"Michael", "player_zero"_joaat}, {"Franklin", "player_one"_joaat}, {"Trevor", "player_two"_joaat}, {"MP Female", "mp_f_freemode_01"_joaat}, {"MP Male", "mp_m_freemode_01"_joaat} };

const std::vector<std::tuple<const char*, Hash>> skins_animals = { {"Boar","a_c_boar"_joaat}, {"Cat", "a_c_cat_01"_joaat}, {"Chimp", "a_c_chimp"_joaat}, {"Chop", "a_c_chop"_joaat}, {"Cormorant", "a_c_cormorant"_joaat}, {"Cow", "a_c_cow"_joaat}, {"Coyote", "a_c_coyote"_joaat}, {"Crow", "a_c_crow"_joaat}, {"Deer", "a_c_deer"_joaat}, {"Dolphin", "a_c_dolphin"_joaat}, {"Fish", "a_c_fish"_joaat}, {"German Shepherd", "a_c_shepherd"_joaat}, {"Grey Whale", "a_c_whalegrey"_joaat}, {"Hammerhead Shark", "a_c_sharkhammer"_joaat}, {"Hawk", "a_c_chickenhawk"_joaat}, {"Hen", "a_c_hen"_joaat}, {"Humpback", "a_c_humpback"_joaat}, {"Husky", "a_c_husky"_joaat}, {"Killer Whale", "a_c_killerwhale"_joaat}, {"Mountain Lion", "a_c_mtlion"_joaat}, {"Pig", "a_c_pig"_joaat}, {"Pigeon", "a_c_pigeon"_joaat}, {"Poodle", "a_c_poodle"_joaat}, {"Pug", "a_c_pug"_joaat}, {"Rabbit", "a_c_rabbit_01"_joaat}, {"Rat", "a_c_rat"_joaat}, {"Retriever", "a_c_retriever"_joaat}, {"Rhesus", "a_c_rhesus"_joaat}, {"Rottweiler", "a_c_rottweiler"_joaat}, {"Seagull", "a_c_seagull"_joaat}, {"Stingray", "a_c_stingray"_joaat}, {"Tiger Shark", "a_c_sharktiger"_joaat}, {"Westy", "a_c_westy"_joaat} };

const std::vector<std::tuple<const char*, Hash>> skins_general = { { "Abigail Mathers (CS)","csb_abigail"_joaat },{ "Abigail Mathers (IG)","ig_abigail"_joaat },{ "Abner","u_m_y_abner"_joaat },{ "African American Male","a_m_m_afriamer_01"_joaat },{ "Agent (CS)","csb_agent"_joaat },{ "Agent (IG)","ig_agent"_joaat },{ "Agent 14 (CS)","csb_mp_agent14"_joaat },{ "Agent 14 (IG)","ig_mp_agent14"_joaat },{ "Air Hostess","s_f_y_airhostess_01"_joaat },{ "Air Worker Male","s_m_y_airworker"_joaat },{ "Al Di Napoli Male","u_m_m_aldinapoli"_joaat },{ "Alien","s_m_m_movalien_01"_joaat },{ "Altruist Cult Mid-Age Male","a_m_m_acult_01"_joaat },{ "Altruist Cult Old Male 2","a_m_o_acult_02"_joaat },{ "Altruist Cult Old Male","a_m_o_acult_01"_joaat },{ "Altruist Cult Young Male 2","a_m_y_acult_02"_joaat },{ "Altruist Cult Young Male","a_m_y_acult_01"_joaat },{ "Amanda De Santa (CS)","cs_amandatownley"_joaat },{ "Amanda De Santa (IG)","ig_amandatownley"_joaat },{ "Ammu-Nation City Clerk","s_m_y_ammucity_01"_joaat },{ "Ammu-Nation Rural Clerk","s_m_m_ammucountry"_joaat },{ "Andreas Sanchez (CS)","cs_andreas"_joaat },{ "Andreas Sanchez (IG)","ig_andreas"_joaat },{ "Anita Mendoza","csb_anita"_joaat },{ "Anton Beaudelaire","csb_anton"_joaat },{ "Anton Beaudelaire","u_m_y_antonb"_joaat },{ "Armenian Boss","g_m_m_armboss_01"_joaat },{ "Armenian Goon 2","g_m_y_armgoon_02"_joaat },{ "Armenian Goon","g_m_m_armgoon_01"_joaat },{ "Armenian Lieutenant","g_m_m_armlieut_01"_joaat },{ "Armoured Van Security 2","s_m_m_armoured_02"_joaat },{ "Armoured Van Security","mp_s_m_armoured_01"_joaat },{ "Armoured Van Security","s_m_m_armoured_01"_joaat },{ "Army Mechanic","s_m_y_armymech_01"_joaat },{ "Ashley Butler (CS)","cs_ashley"_joaat },{ "Ashley Butler (IG)","ig_ashley"_joaat },{ "Autopsy Tech","s_m_y_autopsy_01"_joaat },{ "Autoshop Worker 2","s_m_m_autoshop_02"_joaat },{ "Autoshop Worker","s_m_m_autoshop_01"_joaat },{ "Azteca","g_m_y_azteca_01"_joaat },{ "Baby D","u_m_y_babyd"_joaat },{ "Ballas East Male","g_m_y_ballaeast_01"_joaat },{ "Ballas Female","g_f_y_ballas_01"_joaat },{ "Ballas OG (IG)","ig_ballasog"_joaat },{ "Ballas OG","csb_ballasog"_joaat },{ "Ballas Original Male (IG)","g_m_y_ballaorig_01"_joaat },{ "Ballas South Male","g_m_y_ballasout_01"_joaat },{ "Bank Manager (CS)","cs_bankman"_joaat },{ "Bank Manager (IG)","ig_bankman"_joaat },{ "Bank Manager Male","u_m_m_bankman"_joaat },{ "Barber Female","s_f_m_fembarber"_joaat },{ "Barman","s_m_y_barman_01"_joaat },{ "Barry (CS)","cs_barry"_joaat },{ "Barry (IG)","ig_barry"_joaat },{ "Bartender (Rural)","s_m_m_cntrybar_01"_joaat },{ "Bartender","s_f_y_bartender_01"_joaat },{ "Baywatch Female","s_f_y_baywatch_01"_joaat },{ "Baywatch Male","s_m_y_baywatch_01"_joaat },{ "Beach Female","a_f_m_beach_01"_joaat },{ "Beach Male 2","a_m_m_beach_02"_joaat },{ "Beach Male","a_m_m_beach_01"_joaat },{ "Beach Muscle Male 2","a_m_y_musclbeac_02"_joaat },{ "Beach Muscle Male","a_m_y_musclbeac_01"_joaat },{ "Beach Old Male","a_m_o_beach_01"_joaat },{ "Beach Tramp Female","a_f_m_trampbeac_01"_joaat },{ "Beach Tramp Male","a_m_m_trampbeac_01"_joaat },{ "Beach Young Female","a_f_y_beach_01"_joaat },{ "Beach Young Male 2","a_m_y_beach_02"_joaat },{ "Beach Young Male 3","a_m_y_beach_03"_joaat },{ "Beach Young Male","a_m_y_beach_01"_joaat },{ "Best Man (IG)","ig_bestmen"_joaat },{ "Beverly Felton (CS)","cs_beverly"_joaat },{ "Beverly Felton (IG)","ig_beverly"_joaat },{ "Beverly Hills Female 2","a_f_m_bevhills_02"_joaat },{ "Beverly Hills Female","a_f_m_bevhills_01"_joaat },{ "Beverly Hills Male 2","a_m_m_bevhills_02"_joaat },{ "Beverly Hills Male","a_m_m_bevhills_01"_joaat },{ "Beverly Hills Young Female 2","a_f_y_bevhills_02"_joaat },{ "Beverly Hills Young Female 3","a_f_y_bevhills_03"_joaat },{ "Beverly Hills Young Female 4","a_f_y_bevhills_04"_joaat },{ "Beverly Hills Young Female","a_f_y_bevhills_01"_joaat },{ "Beverly Hills Young Male 2","a_m_y_bevhills_02"_joaat },{ "Beverly Hills Young Male","a_m_y_bevhills_01"_joaat },{ "Bigfoot (CS)","cs_orleans"_joaat },{ "Bigfoot (IG)","ig_orleans"_joaat },{ "Bike Hire Guy","u_m_m_bikehire_01"_joaat },{ "Biker Chic Female","u_f_y_bikerchic"_joaat },{ "Black Ops Soldier 2","s_m_y_blackops_02"_joaat },{ "Black Ops Soldier 3","s_m_y_blackops_03"_joaat },{ "Black Ops Soldier","s_m_y_blackops_01"_joaat },{ "Black Street Male 2","a_m_y_stbla_02"_joaat },{ "Black Street Male","a_m_y_stbla_01"_joaat },{ "Bodybuilder Female","a_f_m_bodybuild_01"_joaat },{ "Bouncer","s_m_m_bouncer_01"_joaat },{ "Brad (CS)","cs_brad"_joaat },{ "Brad (IG)","ig_brad"_joaat },{ "Brad's Cadaver (CS)","cs_bradcadaver"_joaat },{ "Breakdancer Male","a_m_y_breakdance_01"_joaat },{ "Bride (IG)","ig_bride"_joaat },{ "Bride","csb_bride"_joaat },{ "Burger Drug Worker","csb_burgerdrug"_joaat },{ "Burger Drug Worker","u_m_y_burgerdrug_01"_joaat },{ "Busboy","s_m_y_busboy_01"_joaat },{ "Business Casual","a_m_y_busicas_01"_joaat },{ "Business Female 2","a_f_m_business_02"_joaat },{ "Business Male","a_m_m_business_01"_joaat },{ "Business Young Female 2","a_f_y_business_02"_joaat },{ "Business Young Female 3","a_f_y_business_03"_joaat },{ "Business Young Female 4","a_f_y_business_04"_joaat },{ "Business Young Female","a_f_y_business_01"_joaat },{ "Business Young Male 2","a_m_y_business_02"_joaat },{ "Business Young Male 3","a_m_y_business_03"_joaat },{ "Business Young Male","a_m_y_business_01"_joaat },{ "Busker","s_m_o_busker_01"_joaat },{ "Car 3 Guy 1 (IG)","ig_car3guy1"_joaat },{ "Car 3 Guy 1","csb_car3guy1"_joaat },{ "Car 3 Guy 2 (IG)","ig_car3guy2"_joaat },{ "Car 3 Guy 2","csb_car3guy2"_joaat },{ "Car Buyer (CS)","cs_carbuyer"_joaat },{ "Casey (CS)","cs_casey"_joaat },{ "Casey (IG)","ig_casey"_joaat },{ "Chef (CS)","csb_chef2"_joaat },{ "Chef (IG)","ig_chef"_joaat },{ "Chef (IG)","ig_chef2"_joaat },{ "Chef","csb_chef"_joaat },{ "Chef","s_m_y_chef_01"_joaat },{ "Chemical Plant Security","s_m_m_chemsec_01"_joaat },{ "Chemical Plant Worker","g_m_m_chemwork_01"_joaat },{ "Chinese Boss","g_m_m_chiboss_01"_joaat },{ "Chinese Goon 2","g_m_m_chigoon_02"_joaat },{ "Chinese Goon Older","g_m_m_chicold_01"_joaat },{ "Chinese Goon","csb_chin_goon"_joaat },{ "Chinese Goon","g_m_m_chigoon_01"_joaat },{ "Chip","u_m_y_chip"_joaat },{ "Claude Speed","mp_m_claude_01"_joaat },{ "Clay Jackson (The Pain Giver) (IG)","ig_claypain"_joaat },{ "Clay Simons (The Lost) (CS)","cs_clay"_joaat },{ "Clay Simons (The Lost) (IG)","ig_clay"_joaat },{ "Cletus (IG)","ig_cletus"_joaat },{ "Cletus","csb_cletus"_joaat },{ "Clown","s_m_y_clown_01"_joaat },{ "Construction Worker 2","s_m_y_construct_02"_joaat },{ "Construction Worker","s_m_y_construct_01"_joaat },{ "Cop Female","s_f_y_cop_01"_joaat },{ "Cop Male","s_m_y_cop_01"_joaat },{ "Cop","csb_cop"_joaat },{ "Corpse Female","u_f_m_corpse_01"_joaat },{ "Corpse Young Female 2","u_f_y_corpse_02"_joaat },{ "Corpse Young Female","u_f_y_corpse_01"_joaat },{ "Crew Member","s_m_m_ccrew_01"_joaat },{ "Cris Formage (CS)","cs_chrisformage"_joaat },{ "Cris Formage (IG)","ig_chrisformage"_joaat },{ "Customer","csb_customer"_joaat },{ "Cyclist Male","a_m_y_cyclist_01"_joaat },{ "Cyclist Male","u_m_y_cyclist_01"_joaat },{ "Dale (CS)","cs_dale"_joaat },{ "Dale (IG)","ig_dale"_joaat },{ "Dave Norton (CS)","cs_davenorton"_joaat },{ "Dave Norton (IG)","ig_davenorton"_joaat },{ "Dead Hooker","mp_f_deadhooker"_joaat },{ "Dealer","s_m_y_dealer_01"_joaat },{ "Debra (CS)","cs_debra"_joaat },{ "Denise (CS)","cs_denise"_joaat },{ "Denise (IG)","ig_denise"_joaat },{ "Denise's Friend","csb_denise_friend"_joaat },{ "Devin (CS)","cs_devin"_joaat },{ "Devin (IG)","ig_devin"_joaat },{ "Devin's Security","s_m_y_devinsec_01"_joaat },{ "Dima Popov (CS)","csb_popov"_joaat },{ "Dima Popov (IG)","ig_popov"_joaat },{ "DOA Man","u_m_m_doa_01"_joaat },{ "Dock Worker","s_m_m_dockwork_01"_joaat },{ "Dock Worker","s_m_y_dockwork_01"_joaat },{ "Doctor","s_m_m_doctor_01"_joaat },{ "Dom Beasley (CS)","cs_dom"_joaat },{ "Dom Beasley (IG)","ig_dom"_joaat },{ "Doorman","s_m_y_doorman_01"_joaat },{ "Downhill Cyclist","a_m_y_dhill_01"_joaat },{ "Downtown Female","a_f_m_downtown_01"_joaat },{ "Downtown Male","a_m_y_downtown_01"_joaat },{ "Dr. Friedlander (CS)","cs_drfriedlander"_joaat },{ "Dr. Friedlander (IG)","ig_drfriedlander"_joaat },{ "Dressy Female","a_f_y_scdressy_01"_joaat },{ "DW Airport Worker 2","s_m_y_dwservice_02"_joaat },{ "DW Airport Worker","s_m_y_dwservice_01"_joaat },{ "East SA Female 2","a_f_m_eastsa_02"_joaat },{ "East SA Female","a_f_m_eastsa_01"_joaat },{ "East SA Male 2","a_m_m_eastsa_02"_joaat },{ "East SA Male","a_m_m_eastsa_01"_joaat },{ "East SA Young Female 2","a_f_y_eastsa_02"_joaat },{ "East SA Young Female 3","a_f_y_eastsa_03"_joaat },{ "East SA Young Female","a_f_y_eastsa_01"_joaat },{ "East SA Young Male 2","a_m_y_eastsa_02"_joaat },{ "East SA Young Male","a_m_y_eastsa_01"_joaat },{ "Ed Toh","u_m_m_edtoh"_joaat },{ "Epsilon Female","a_f_y_epsilon_01"_joaat },{ "Epsilon Male 2","a_m_y_epsilon_02"_joaat },{ "Epsilon Male","a_m_y_epsilon_01"_joaat },{ "Epsilon Tom (CS)","cs_tomepsilon"_joaat },{ "Epsilon Tom (IG)","ig_tomepsilon"_joaat },{ "Ex-Army Male","mp_m_exarmy_01"_joaat },{ "Ex-Mil Bum","u_m_y_militarybum"_joaat },{ "Fabien (CS)","cs_fabien"_joaat },{ "Fabien (IG)","ig_fabien"_joaat },{ "Factory Worker Female","s_f_y_factory_01"_joaat },{ "Factory Worker Male","s_m_y_factory_01"_joaat },{ "Families CA Male","g_m_y_famca_01"_joaat },{ "Families DD Male","mp_m_famdd_01"_joaat },{ "Families DNF Male","g_m_y_famdnf_01"_joaat },{ "Families Female","g_f_y_families_01"_joaat },{ "Families FOR Male","g_m_y_famfor_01"_joaat },{ "Families Gang Member? (IG)","ig_ramp_gang"_joaat },{ "Families Gang Member?","csb_ramp_gang"_joaat },{ "Farmer","a_m_m_farmer_01"_joaat },{ "Fat Black Female","a_f_m_fatbla_01"_joaat },{ "Fat Cult Female","a_f_m_fatcult_01"_joaat },{ "Fat Latino Male","a_m_m_fatlatin_01"_joaat },{ "Fat White Female","a_f_m_fatwhite_01"_joaat },{ "Female Agent","a_f_y_femaleagent"_joaat },{ "Ferdinand Kerimov (Mr. K) (CS)","cs_mrk"_joaat },{ "Ferdinand Kerimov (Mr. K) (IG)","ig_mrk"_joaat },{ "FIB Architect","u_m_m_fibarchitect"_joaat },{ "FIB Mugger","u_m_y_fibmugger_01"_joaat },{ "FIB Office Worker 2","s_m_m_fiboffice_02"_joaat },{ "FIB Office Worker","s_m_m_fiboffice_01"_joaat },{ "FIB Security","mp_m_fibsec_01"_joaat },{ "FIB Security","s_m_m_fibsec_01"_joaat },{ "FIB Suit (CS)","cs_fbisuit_01"_joaat },{ "FIB Suit (IG)","ig_fbisuit_01"_joaat },{ "Financial Guru","u_m_o_finguru_01"_joaat },{ "Fireman Male","s_m_y_fireman_01"_joaat },{ "Fitness Female 2","a_f_y_fitness_02"_joaat },{ "Fitness Female","a_f_y_fitness_01"_joaat },{ "Floyd Hebert (CS)","cs_floyd"_joaat },{ "Floyd Hebert (IG)","ig_floyd"_joaat },{ "FOS Rep?","csb_fos_rep"_joaat },{ "Free Mode Female","mp_f_freemode_01"_joaat },{ "Free Mode Male","mp_m_freemode_01"_joaat },{ "Gaffer","s_m_m_gaffer_01"_joaat },{ "Garbage Worker","s_m_y_garbage"_joaat },{ "Gardener","s_m_m_gardener_01"_joaat },{ "Gay Male 2","a_m_y_gay_02"_joaat },{ "Gay Male","a_m_y_gay_01"_joaat },{ "General Fat Male 2","a_m_m_genfat_02"_joaat },{ "General Fat Male","a_m_m_genfat_01"_joaat },{ "General Hot Young Female","a_f_y_genhot_01"_joaat },{ "General Street Old Female","a_f_o_genstreet_01"_joaat },{ "General Street Old Male","a_m_o_genstreet_01"_joaat },{ "General Street Young Male 2","a_m_y_genstreet_02"_joaat },{ "General Street Young Male","a_m_y_genstreet_01"_joaat },{ "Gerald","csb_g"_joaat },{ "GLENSTANK? Male","u_m_m_glenstank_01"_joaat },{ "Golfer Male","a_m_m_golfer_01"_joaat },{ "Golfer Young Female","a_f_y_golfer_01"_joaat },{ "Golfer Young Male","a_m_y_golfer_01"_joaat },{ "Griff","u_m_m_griff_01"_joaat },{ "Grip","s_m_y_grip_01"_joaat },{ "Groom (IG)","ig_groom"_joaat },{ "Groom","csb_groom"_joaat },{ "Grove Street Dealer","csb_grove_str_dlr"_joaat },{ "Guadalope (CS)","cs_guadalope"_joaat },{ "Guido","u_m_y_guido_01"_joaat },{ "Gun Vendor","u_m_y_gunvend_01"_joaat },{ "GURK? (CS)","cs_gurk"_joaat },{ "Hairdresser Male","s_m_m_hairdress_01"_joaat },{ "Hao (IG)","ig_hao"_joaat },{ "Hao","csb_hao"_joaat },{ "Hasidic Jew Male","a_m_m_hasjew_01"_joaat },{ "Hasidic Jew Young Male","a_m_y_hasjew_01"_joaat },{ "Hick (IG)","ig_ramp_hic"_joaat },{ "Hick","csb_ramp_hic"_joaat },{ "High Security 2","s_m_m_highsec_02"_joaat },{ "High Security","s_m_m_highsec_01"_joaat },{ "Highway Cop","s_m_y_hwaycop_01"_joaat },{ "Hiker Female","a_f_y_hiker_01"_joaat },{ "Hiker Male","a_m_y_hiker_01"_joaat },{ "Hillbilly Male 2","a_m_m_hillbilly_02"_joaat },{ "Hillbilly Male","a_m_m_hillbilly_01"_joaat },{ "Hippie Female","a_f_y_hippie_01"_joaat },{ "Hippie Male","a_m_y_hippy_01"_joaat },{ "Hippie Male","u_m_y_hippie_01"_joaat },{ "Hipster (IG)","ig_ramp_hipster"_joaat },{ "Hipster Female 2","a_f_y_hipster_02"_joaat },{ "Hipster Female 3","a_f_y_hipster_03"_joaat },{ "Hipster Female 4","a_f_y_hipster_04"_joaat },{ "Hipster Female","a_f_y_hipster_01"_joaat },{ "Hipster Male 2","a_m_y_hipster_02"_joaat },{ "Hipster Male 3","a_m_y_hipster_03"_joaat },{ "Hipster Male","a_m_y_hipster_01"_joaat },{ "Hipster","csb_ramp_hipster"_joaat },{ "Hooker 2","s_f_y_hooker_02"_joaat },{ "Hooker 3","s_f_y_hooker_03"_joaat },{ "Hooker","s_f_y_hooker_01"_joaat },{ "Hospital Scrubs Female","s_f_y_scrubs_01"_joaat },{ "Hot Posh Female","u_f_y_hotposh_01"_joaat },{ "Hugh Welsh","csb_hugh"_joaat },{ "Hunter (CS)","cs_hunter"_joaat },{ "Hunter (IG)","ig_hunter"_joaat },{ "IAA Security","s_m_m_ciasec_01"_joaat },{ "Impotent Rage","u_m_y_imporage"_joaat },{ "Imran Shinowa","csb_imran"_joaat },{ "Indian Male","a_m_m_indian_01"_joaat },{ "Indian Old Female","a_f_o_indian_01"_joaat },{ "Indian Young Female","a_f_y_indian_01"_joaat },{ "Indian Young Male","a_m_y_indian_01"_joaat },{ "Jane","u_f_y_comjane"_joaat },{ "Janet (CS)","cs_janet"_joaat },{ "Janet (IG)","ig_janet"_joaat },{ "Janitor","csb_janitor"_joaat },{ "Janitor","s_m_m_janitor"_joaat },{ "Jay Norris (IG)","ig_jay_norris"_joaat },{ "Jesco White (Tapdancing Hillbilly)","u_m_o_taphillbilly"_joaat },{ "Jesus","u_m_m_jesus_01"_joaat },{ "Jetskier","a_m_y_jetski_01"_joaat },{ "Jewel Heist Driver","hc_driver"_joaat },{ "Jewel Heist Gunman","hc_gunman"_joaat },{ "Jewel Heist Hacker","hc_hacker"_joaat },{ "Jewel Thief","u_m_m_jewelthief"_joaat },{ "Jeweller Assistant (CS)","cs_jewelass"_joaat },{ "Jeweller Assistant (IG)","ig_jewelass"_joaat },{ "Jeweller Assistant","u_f_y_jewelass_01"_joaat },{ "Jeweller Security","u_m_m_jewelsec_01"_joaat },{ "Jimmy Boston (CS)","cs_jimmyboston"_joaat },{ "Jimmy Boston (IG)","ig_jimmyboston"_joaat },{ "Jimmy De Santa (CS)","cs_jimmydisanto"_joaat },{ "Jimmy De Santa (IG)","ig_jimmydisanto"_joaat },{ "Jogger Female","a_f_y_runner_01"_joaat },{ "Jogger Male 2","a_m_y_runner_02"_joaat },{ "Jogger Male","a_m_y_runner_01"_joaat },{ "John Marston","mp_m_marston_01"_joaat },{ "Johnny Klebitz (CS)","cs_johnnyklebitz"_joaat },{ "Johnny Klebitz (IG)","ig_johnnyklebitz"_joaat },{ "Josef (CS)","cs_josef"_joaat },{ "Josef (IG)","ig_josef"_joaat },{ "Josh (CS)","cs_josh"_joaat },{ "Josh (IG)","ig_josh"_joaat },{ "Juggalo Female","a_f_y_juggalo_01"_joaat },{ "Juggalo Male","a_m_y_juggalo_01"_joaat },{ "Justin","u_m_y_justin"_joaat },{ "Karen Daniels (CS)","cs_karen_daniels"_joaat },{ "Karen Daniels (IG)","ig_karen_daniels"_joaat },{ "Kerry McIntosh (IG)","ig_kerrymcintosh"_joaat },{ "Kifflom Guy","u_m_y_baygor"_joaat },{ "Korean Boss","g_m_m_korboss_01"_joaat },{ "Korean Female 2","a_f_m_ktown_02"_joaat },{ "Korean Female","a_f_m_ktown_01"_joaat },{ "Korean Lieutenant","g_m_y_korlieut_01"_joaat },{ "Korean Male","a_m_m_ktown_01"_joaat },{ "Korean Old Female","a_f_o_ktown_01"_joaat },{ "Korean Old Male","a_m_o_ktown_01"_joaat },{ "Korean Young Male 2","a_m_y_ktown_02"_joaat },{ "Korean Young Male 2","g_m_y_korean_02"_joaat },{ "Korean Young Male","a_m_y_ktown_01"_joaat },{ "Korean Young Male","g_m_y_korean_01"_joaat },{ "Lamar Davis (CS)","cs_lamardavis"_joaat },{ "Lamar Davis (IG)","ig_lamardavis"_joaat },{ "Latino Handyman Male","s_m_m_lathandy_01"_joaat },{ "Latino Street Male 2","a_m_m_stlat_02"_joaat },{ "Latino Street Young Male","a_m_y_stlat_01"_joaat },{ "Latino Young Male","a_m_y_latino_01"_joaat },{ "Lazlow (CS)","cs_lazlow"_joaat },{ "Lazlow (IG)","ig_lazlow"_joaat },{ "Lester Crest (CS)","cs_lestercrest"_joaat },{ "Lester Crest (IG)","ig_lestercrest"_joaat },{ "Life Invader (CS)","cs_lifeinvad_01"_joaat },{ "Life Invader (IG)","ig_lifeinvad_01"_joaat },{ "Life Invader 2 (IG)","ig_lifeinvad_02"_joaat },{ "Life Invader Male","s_m_m_lifeinvad_01"_joaat },{ "Line Cook","s_m_m_linecook"_joaat },{ "Love Fist Willy","u_m_m_willyfist"_joaat },{ "LS Metro Worker Male","s_m_m_lsmetro_01"_joaat },{ "Magenta (CS)","cs_magenta"_joaat },{ "Magenta (IG)","ig_magenta"_joaat },{ "Maid","s_f_m_maid_01"_joaat },{ "Malibu Male","a_m_m_malibu_01"_joaat },{ "Mani","u_m_y_mani"_joaat },{ "Manuel (CS)","cs_manuel"_joaat },{ "Manuel (IG)","ig_manuel"_joaat },{ "Mariachi","s_m_m_mariachi_01"_joaat },{ "Marine 2","s_m_m_marine_02"_joaat },{ "Marine Young 2","s_m_y_marine_02"_joaat },{ "Marine Young 3","s_m_y_marine_03"_joaat },{ "Marine Young","s_m_y_marine_01"_joaat },{ "Marine","csb_ramp_marine"_joaat },{ "Marine","s_m_m_marine_01"_joaat },{ "Mark Fostenburg","u_m_m_markfost"_joaat },{ "Marnie Allen (CS)","cs_marnie"_joaat },{ "Marnie Allen (IG)","ig_marnie"_joaat },{ "Martin Madrazo (CS)","cs_martinmadrazo"_joaat },{ "Mary-Ann Quinn (CS)","cs_maryann"_joaat },{ "Mary-Ann Quinn (IG)","ig_maryann"_joaat },{ "Maude (IG)","ig_maude"_joaat },{ "Maude","csb_maude"_joaat },{ "Maxim Rashkovsky (CS)","csb_rashcosvki"_joaat },{ "Maxim Rashkovsky (IG)","ig_rashcosvki"_joaat },{ "Mechanic 2","s_m_y_xmech_02"_joaat },{ "Mechanic","s_m_y_xmech_01"_joaat },{ "Merryweather Merc","csb_mweather"_joaat },{ "Meth Addict","a_m_y_methhead_01"_joaat },{ "Mexican (IG)","ig_ramp_mex"_joaat },{ "Mexican Boss 2","g_m_m_mexboss_02"_joaat },{ "Mexican Boss","g_m_m_mexboss_01"_joaat },{ "Mexican Gang Member","g_m_y_mexgang_01"_joaat },{ "Mexican Goon 2","g_m_y_mexgoon_02"_joaat },{ "Mexican Goon 3","g_m_y_mexgoon_03"_joaat },{ "Mexican Goon","g_m_y_mexgoon_01"_joaat },{ "Mexican Labourer","a_m_m_mexlabor_01"_joaat },{ "Mexican Rural","a_m_m_mexcntry_01"_joaat },{ "Mexican Thug","a_m_y_mexthug_01"_joaat },{ "Mexican","csb_ramp_mex"_joaat },{ "Michelle (CS)","cs_michelle"_joaat },{ "Michelle (IG)","ig_michelle"_joaat },{ "Migrant Female","s_f_y_migrant_01"_joaat },{ "Migrant Male","s_m_m_migrant_01"_joaat },{ "Milton McIlroy (CS)","cs_milton"_joaat },{ "Milton McIlroy (IG)","ig_milton"_joaat },{ "Mime Artist","s_m_y_mime"_joaat },{ "Minuteman Joe (CS)","cs_joeminuteman"_joaat },{ "Minuteman Joe (IG)","ig_joeminuteman"_joaat },{ "Miranda","u_f_m_miranda"_joaat },{ "Mistress","u_f_y_mistress"_joaat },{ "Misty","mp_f_misty_01"_joaat },{ "Molly (CS)","cs_molly"_joaat },{ "Molly (IG)","ig_molly"_joaat },{ "Money Man (CS)","csb_money"_joaat },{ "Money Man (IG)","ig_money"_joaat },{ "Motocross Biker 2","a_m_y_motox_02"_joaat },{ "Motocross Biker","a_m_y_motox_01"_joaat },{ "Movie Astronaut","s_m_m_movspace_01"_joaat },{ "Movie Director","u_m_m_filmdirector"_joaat },{ "Movie Premiere Female (CS)","cs_movpremf_01"_joaat },{ "Movie Premiere Female","s_f_y_movprem_01"_joaat },{ "Movie Premiere Male (CS)","cs_movpremmale"_joaat },{ "Movie Premiere Male","s_m_m_movprem_01"_joaat },{ "Movie Star Female","u_f_o_moviestar"_joaat },{ "Mrs. Phillips (CS)","cs_mrsphillips"_joaat },{ "Mrs. Phillips (IG)","ig_mrsphillips"_joaat },{ "Mrs. Thornhill (CS)","cs_mrs_thornhill"_joaat },{ "Mrs. Thornhill (IG)","ig_mrs_thornhill"_joaat },{ "Natalia (CS)","cs_natalia"_joaat },{ "Natalia (IG)","ig_natalia"_joaat },{ "Nervous Ron (CS)","cs_nervousron"_joaat },{ "Nervous Ron (IG)","ig_nervousron"_joaat },{ "Nigel (CS)","cs_nigel"_joaat },{ "Nigel (IG)","ig_nigel"_joaat },{ "Niko Bellic","mp_m_niko_01"_joaat },{ "O'Neil Brothers (IG)","ig_oneil"_joaat },{ "OG Boss","a_m_m_og_boss_01"_joaat },{ "Old Man 1 (CS)","cs_old_man1a"_joaat },{ "Old Man 1 (IG)","ig_old_man1a"_joaat },{ "Old Man 2 (CS)","cs_old_man2"_joaat },{ "Old Man 2 (IG)","ig_old_man2"_joaat },{ "Omega (CS)","cs_omega"_joaat },{ "Omega (IG)","ig_omega"_joaat },{ "Ortega (IG)","ig_ortega"_joaat },{ "Ortega","csb_ortega"_joaat },{ "Oscar","csb_oscar"_joaat },{ "Paige Harris (CS)","csb_paige"_joaat },{ "Paige Harris (IG)","ig_paige"_joaat },{ "Paparazzi Male","a_m_m_paparazzi_01"_joaat },{ "Paparazzi Young Male","u_m_y_paparazzi"_joaat },{ "Paramedic","s_m_m_paramedic_01"_joaat },{ "Party Target","u_m_m_partytarget"_joaat },{ "Partygoer","u_m_y_party_01"_joaat },{ "Patricia (CS)","cs_patricia"_joaat },{ "Patricia (IG)","ig_patricia"_joaat },{ "Pest Control","s_m_y_pestcont_01"_joaat },{ "Peter Dreyfuss (CS)","cs_dreyfuss"_joaat },{ "Peter Dreyfuss (IG)","ig_dreyfuss"_joaat },{ "Pilot 2","s_m_m_pilot_02"_joaat },{ "Pilot","s_m_m_pilot_01"_joaat },{ "Pilot","s_m_y_pilot_01"_joaat },{ "Pogo the Monkey","u_m_y_pogo_01"_joaat },{ "Polynesian Goon 2","g_m_y_pologoon_02"_joaat },{ "Polynesian Goon","g_m_y_pologoon_01"_joaat },{ "Polynesian Young","a_m_y_polynesian_01"_joaat },{ "Polynesian","a_m_m_polynesian_01"_joaat },{ "Poppy Mitchell","u_f_y_poppymich"_joaat },{ "Porn Dude","csb_porndudes"_joaat },{ "Postal Worker Male 2","s_m_m_postal_02"_joaat },{ "Postal Worker Male","s_m_m_postal_01"_joaat },{ "Priest (CS)","cs_priest"_joaat },{ "Priest (IG)","ig_priest"_joaat },{ "Princess","u_f_y_princess"_joaat },{ "Prison Guard","s_m_m_prisguard_01"_joaat },{ "Prisoner (Muscular)","s_m_y_prismuscl_01"_joaat },{ "Prisoner","s_m_y_prisoner_01"_joaat },{ "Prisoner","u_m_y_prisoner_01"_joaat },{ "Prologue Driver","csb_prologuedriver"_joaat },{ "Prologue Driver","u_m_y_proldriver_01"_joaat },{ "Prologue Host Female","a_f_m_prolhost_01"_joaat },{ "Prologue Host Male","a_m_m_prolhost_01"_joaat },{ "Prologue Host Old Female","u_f_o_prolhost_01"_joaat },{ "Prologue Mourner Female","u_f_m_promourn_01"_joaat },{ "Prologue Mourner Male","u_m_m_promourn_01"_joaat },{ "Prologue Security 2 (CS)","cs_prolsec_02"_joaat },{ "Prologue Security 2 (IG)","ig_prolsec_02"_joaat },{ "Prologue Security","csb_prolsec"_joaat },{ "Prologue Security","u_m_m_prolsec_01"_joaat },{ "PROS?","mp_g_m_pros_01"_joaat },{ "Ranger Female","s_f_y_ranger_01"_joaat },{ "Ranger Male","s_m_y_ranger_01"_joaat },{ "Reporter","csb_reporter"_joaat },{ "Republican Space Ranger","u_m_y_rsranger_01"_joaat },{ "Rival Paparazzo","u_m_m_rivalpap"_joaat },{ "Road Cyclist","a_m_y_roadcyc_01"_joaat },{ "Robber","s_m_y_robber_01"_joaat },{ "Rocco Pelosi (IG)","ig_roccopelosi"_joaat },{ "Rocco Pelosi","csb_roccopelosi"_joaat },{ "Rural Meth Addict Female","a_f_y_rurmeth_01"_joaat },{ "Rural Meth Addict Male","a_m_m_rurmeth_01"_joaat },{ "Russian Drunk (CS)","cs_russiandrunk"_joaat },{ "Russian Drunk (IG)","ig_russiandrunk"_joaat },{ "Sales Assistant (High-End)","s_f_m_shop_high"_joaat },{ "Sales Assistant (Low-End)","s_f_y_shop_low"_joaat },{ "Sales Assistant (Mask Stall)","s_m_y_shop_mask"_joaat },{ "Sales Assistant (Mid-Price)","s_f_y_shop_mid"_joaat },{ "Salton Female","a_f_m_salton_01"_joaat },{ "Salton Male 2","a_m_m_salton_02"_joaat },{ "Salton Male 3","a_m_m_salton_03"_joaat },{ "Salton Male 4","a_m_m_salton_04"_joaat },{ "Salton Male","a_m_m_salton_01"_joaat },{ "Salton Old Female","a_f_o_salton_01"_joaat },{ "Salton Old Male","a_m_o_salton_01"_joaat },{ "Salton Young Male","a_m_y_salton_01"_joaat },{ "Salvadoran Boss","g_m_y_salvaboss_01"_joaat },{ "Salvadoran Goon 2","g_m_y_salvagoon_02"_joaat },{ "Salvadoran Goon 3","g_m_y_salvagoon_03"_joaat },{ "Salvadoran Goon","g_m_y_salvagoon_01"_joaat },{ "Scientist","s_m_m_scientist_01"_joaat },{ "Screenwriter (IG)","ig_screen_writer"_joaat },{ "Screenwriter","csb_screen_writer"_joaat },{ "Security Guard","s_m_m_security_01"_joaat },{ "Sheriff Female","s_f_y_sheriff_01"_joaat },{ "Sheriff Male","s_m_y_sheriff_01"_joaat },{ "Shopkeeper","mp_m_shopkeep_01"_joaat },{ "Simeon Yetarian (CS)","cs_siemonyetarian"_joaat },{ "Simeon Yetarian (IG)","ig_siemonyetarian"_joaat },{ "Skater Female","a_f_y_skater_01"_joaat },{ "Skater Male","a_m_m_skater_01"_joaat },{ "Skater Young Male 2","a_m_y_skater_02"_joaat },{ "Skater Young Male","a_m_y_skater_01"_joaat },{ "Skid Row Female","a_f_m_skidrow_01"_joaat },{ "Skid Row Male","a_m_m_skidrow_01"_joaat },{ "Snow Cop Male","s_m_m_snowcop_01"_joaat },{ "Solomon Richards (CS)","cs_solomon"_joaat },{ "Solomon Richards (IG)","ig_solomon"_joaat },{ "South Central Female 2","a_f_m_soucent_02"_joaat },{ "South Central Female","a_f_m_soucent_01"_joaat },{ "South Central Latino Male","a_m_m_socenlat_01"_joaat },{ "South Central Male 2","a_m_m_soucent_02"_joaat },{ "South Central Male 3","a_m_m_soucent_03"_joaat },{ "South Central Male 4","a_m_m_soucent_04"_joaat },{ "South Central Male","a_m_m_soucent_01"_joaat },{ "South Central MC Female","a_f_m_soucentmc_01"_joaat },{ "South Central Old Female 2","a_f_o_soucent_02"_joaat },{ "South Central Old Female","a_f_o_soucent_01"_joaat },{ "South Central Old Male 2","a_m_o_soucent_02"_joaat },{ "South Central Old Male 3","a_m_o_soucent_03"_joaat },{ "South Central Old Male","a_m_o_soucent_01"_joaat },{ "South Central Young Female 2","a_f_y_soucent_02"_joaat },{ "South Central Young Female 3","a_f_y_soucent_03"_joaat },{ "South Central Young Female","a_f_y_soucent_01"_joaat },{ "South Central Young Male 2","a_m_y_soucent_02"_joaat },{ "South Central Young Male 3","a_m_y_soucent_03"_joaat },{ "South Central Young Male 4","a_m_y_soucent_04"_joaat },{ "South Central Young Male","a_m_y_soucent_01"_joaat },{ "Sports Biker","u_m_y_sbike"_joaat },{ "Spy Actor","u_m_m_spyactor"_joaat },{ "Spy Actress","u_f_y_spyactress"_joaat },{ "Stag Party Groom","u_m_y_staggrm_01"_joaat },{ "Steve Haines (CS)","cs_stevehains"_joaat },{ "Steve Haines (IG)","ig_stevehains"_joaat },{ "Street Performer","s_m_m_strperf_01"_joaat },{ "Street Preacher","s_m_m_strpreach_01"_joaat },{ "Street Punk 2","g_m_y_strpunk_02"_joaat },{ "Street Punk","g_m_y_strpunk_01"_joaat },{ "Street Vendor Young","s_m_y_strvend_01"_joaat },{ "Street Vendor","s_m_m_strvend_01"_joaat },{ "Stretch (CS)","cs_stretch"_joaat },{ "Stretch (IG)","ig_stretch"_joaat },{ "Stripper 2","csb_stripper_02"_joaat },{ "Stripper 2","s_f_y_stripper_02"_joaat },{ "Stripper Lite","mp_f_stripperlite"_joaat },{ "Stripper Lite","s_f_y_stripperlite"_joaat },{ "Stripper","csb_stripper_01"_joaat },{ "Stripper","s_f_y_stripper_01"_joaat },{ "Sunbather Male","a_m_y_sunbathe_01"_joaat },{ "Surfer","a_m_y_surfer_01"_joaat },{ "SWAT","s_m_y_swat_01"_joaat },{ "Sweatshop Worker Young","s_f_y_sweatshop_01"_joaat },{ "Sweatshop Worker","s_f_m_sweatshop_01"_joaat },{ "Talina (IG)","ig_talina"_joaat },{ "Tanisha (CS)","cs_tanisha"_joaat },{ "Tanisha (IG)","ig_tanisha"_joaat },{ "Tao Cheng (CS)","cs_taocheng"_joaat },{ "Tao Cheng (IG)","ig_taocheng"_joaat },{ "Tao's Translator (CS)","cs_taostranslator"_joaat },{ "Tao's Translator (IG)","ig_taostranslator"_joaat },{ "Tattoo Artist","u_m_y_tattoo_01"_joaat },{ "Tennis Coach (CS)","cs_tenniscoach"_joaat },{ "Tennis Coach (IG)","ig_tenniscoach"_joaat },{ "Tennis Player Female","a_f_y_tennis_01"_joaat },{ "Tennis Player Male","a_m_m_tennis_01"_joaat },{ "Terry (CS)","cs_terry"_joaat },{ "Terry (IG)","ig_terry"_joaat },{ "The Lost MC Female","g_f_y_lost_01"_joaat },{ "The Lost MC Male 2","g_m_y_lost_02"_joaat },{ "The Lost MC Male 3","g_m_y_lost_03"_joaat },{ "The Lost MC Male","g_m_y_lost_01"_joaat },{ "Tom (CS)","cs_tom"_joaat },{ "Tonya (IG)","ig_tonya"_joaat },{ "Tonya","csb_tonya"_joaat },{ "Topless","a_f_y_topless_01"_joaat },{ "Tourist Female","a_f_m_tourist_01"_joaat },{ "Tourist Male","a_m_m_tourist_01"_joaat },{ "Tourist Young Female 2","a_f_y_tourist_02"_joaat },{ "Tourist Young Female","a_f_y_tourist_01"_joaat },{ "Tracey De Santa (CS)","cs_tracydisanto"_joaat },{ "Tracey De Santa (IG)","ig_tracydisanto"_joaat },{ "Traffic Warden (IG)","ig_trafficwarden"_joaat },{ "Traffic Warden","csb_trafficwarden"_joaat },{ "Tramp Female","a_f_m_tramp_01"_joaat },{ "Tramp Male","a_m_m_tramp_01"_joaat },{ "Tramp Old Male","a_m_o_tramp_01"_joaat },{ "Tramp Old Male","u_m_o_tramp_01"_joaat },{ "Transport Worker Male","s_m_m_gentransport"_joaat },{ "Transvestite Male 2","a_m_m_tranvest_02"_joaat },{ "Transvestite Male","a_m_m_tranvest_01"_joaat },{ "Trucker Male","s_m_m_trucker_01"_joaat },{ "Tyler Dixon (IG)","ig_tylerdix"_joaat },{ "Undercover Cop","csb_undercover"_joaat },{ "United Paper Man (CS)","cs_paper"_joaat },{ "United Paper Man (IG)","ig_paper"_joaat },{ "UPS Driver 2","s_m_m_ups_02"_joaat },{ "UPS Driver","s_m_m_ups_01"_joaat },{ "US Coastguard","s_m_y_uscg_01"_joaat },{ "Vagos Female","g_f_y_vagos_01"_joaat },{ "Valet","s_m_y_valet_01"_joaat },{ "Vespucci Beach Male 2","a_m_y_beachvesp_02"_joaat },{ "Vespucci Beach Male","a_m_y_beachvesp_01"_joaat },{ "Vinewood Douche","a_m_y_vindouche_01"_joaat },{ "Vinewood Female 2","a_f_y_vinewood_02"_joaat },{ "Vinewood Female 3","a_f_y_vinewood_03"_joaat },{ "Vinewood Female 4","a_f_y_vinewood_04"_joaat },{ "Vinewood Female","a_f_y_vinewood_01"_joaat },{ "Vinewood Male 2","a_m_y_vinewood_02"_joaat },{ "Vinewood Male 3","a_m_y_vinewood_03"_joaat },{ "Vinewood Male 4","a_m_y_vinewood_04"_joaat },{ "Vinewood Male","a_m_y_vinewood_01"_joaat },{ "Wade (CS)","cs_wade"_joaat },{ "Wade (IG)","ig_wade"_joaat },{ "Waiter","s_m_y_waiter_01"_joaat },{ "Wei Cheng (CS)","cs_chengsr"_joaat },{ "Wei Cheng (IG)","ig_chengsr"_joaat },{ "White Street Male 2","a_m_y_stwhi_02"_joaat },{ "White Street Male","a_m_y_stwhi_01"_joaat },{ "Window Cleaner","s_m_y_winclean_01"_joaat },{ "Yoga Female","a_f_y_yoga_01"_joaat },{ "Yoga Male","a_m_y_yoga_01"_joaat },{ "Zimbor (CS)","cs_zimbor"_joaat },{ "Zimbor (IG)","ig_zimbor"_joaat },{ "Zombie","u_m_y_zombie_01"_joaat } };

/* =================== Teleport Locations =================== */

const std::vector<std::tuple<const char*, Vector3>> locations_safe
{
	{ "Michael's Safehouse", Vector3(-827.138f, 176.368f, 70.4999f) },
	{ "Franklin's Safehouse", Vector3(-18.0355f, -1456.94f, 30.4548f) },
	{ "Franklin's Safehouse 2", Vector3(10.8766f, 545.654f, 175.419f) },
	{ "Trevor's Safehouse", Vector3(1982.13f, 3829.44f, 32.3662f) },
	{ "Trevor's Safehouse 2", Vector3(-1157.05f, -1512.73f, 4.2127f) },
	{ "Trevor's Safehouse 3", Vector3(91.1407f, -1280.65f, 29.1353f) },
	{ "Michael's Safehouse Inside", Vector3(-813.603f, 179.474f, 72.1548f) },
	{ "Franklin's Safehouse Inside", Vector3(-14.3803f, -1438.51f, 31.1073f) },
	{ "Franklin's Safehouse 2 Inside", Vector3(7.11903f, 536.615f, 176.028f) },
	{ "Trevor's Safehouse Inside", Vector3(1972.61f, 3817.04f, 33.4278f) },
	{ "Trevor's Safehouse 2 Inside", Vector3(-1151.77f, -1518.14f, 10.6327f) },
	{ "Trevor's Safehouse 3 Inside", Vector3(96.1536f, -1290.73f, 29.2664f) },
};

const std::vector<std::tuple<const char*, Vector3>> locations_landmarks
{
	{ "Airport Entrance", Vector3(-1034.6f, -2733.6f, 13.8f) },
	{ "Airport Field", Vector3(-1336.0f, -3044.0f, 13.9f) },
	{ "Airport Helipad", Vector3(-1102.290f, -2894.520f, 13.947f) },
	{ "Altruist Cult Camp", Vector3(-1170.841f, 4926.646f, 224.295f) },
	{ "Calafia Train Bridge", Vector3(-517.869f, 4425.284f, 89.795f) },
	{ "Avi Hideout", Vector3(-2177.440f, 5182.596f, 16.475f) },
	{ "Cargo Ship", Vector3(899.678f, -2882.191f, 19.013f) },
	{ "Chumash", Vector3(-3192.6f, 1100.0f, 20.2f) },
	{ "Chumash Historic Family Pier", Vector3(-3426.683f, 967.738f, 8.347f) },
	{ "Del Perro Pier", Vector3(-1850.127f, -1231.751f, 13.017f) },
	{ "Devin Weston's House", Vector3(-2639.872f, 1866.812f, 160.135f) },
	{ "El Burro Heights", Vector3(1384.0f, -2057.1f, 52.0f) },
	{ "Elysian Island", Vector3(338.2f, -2715.9f, 38.5f) },
	{ "Far North San Andreas", Vector3(24.775f, 7644.102f, 19.055f) },
	{ "Ferris Wheel", Vector3(-1670.7f, -1125.0f, 13.0f) },
	{ "Fort Zancudo", Vector3(-2047.4f, 3132.1f, 32.8f) },
	{ "God's Thumb", Vector3(-1006.402f, 6272.383f, 1.503f) },
	{ "Hippy Camp", Vector3(2476.712f, 3789.645f, 41.226f) },
	{ "Impound Lot", Vector3(391.475f, -1637.975f, 29.315f) },
	{ "Jetsam", Vector3(760.4f, -2943.2f, 5.8f) },
	{ "Jolene Cranley-Evans Ghost", Vector3(3059.620f, 5564.246f, 197.091f) },
	{ "Kortz Center", Vector3(-2243.810f, 264.048f, 174.615f) },
	{ "Lighthouse", Vector3(3426.033f, 5174.606f, 7.382f) },
	{ "Main LS Customs", Vector3(-365.425f, -131.809f, 37.873f) },
	{ "Marlowe Vineyards", Vector3(-1868.971f, 2095.674f, 139.115f) },
	{ "McKenzie Airfield", Vector3(2121.7f, 4796.3f, 41.1f) },
	{ "Merryweather Dock", Vector3(486.417f, -3339.692f, 6.070f) },
	{ "Military - Jet Spawn", Vector3(-2148.350f, 3031.762f, 32.810f) },
	{ "Mineshaft", Vector3(-595.342f, 2086.008f, 131.412f) },
	{ "Mors Mutual Impound", Vector3(-222.198f, -1185.850f, 23.029f) },
	{ "Mt. Chiliad", Vector3(425.4f, 5614.3f, 766.5f) },
	{ "Mt. Chiliad Summit", Vector3(450.718f, 5566.614f, 806.183f) },
	{ "Mt. Gordo", Vector3(2876.984f, 5911.455f, 370.000f) },
	{ "Mt. Josiah", Vector3(-1212.987f, 3848.685f, 491.000f) },
	{ "NOOSE Headquarters", Vector3(2535.243f, -383.799f, 92.993f) },
	{ "Paleto Bay Pier", Vector3(-275.522f, 6635.835f, 7.425f) },
	{ "Paleto Bay Beach", Vector3(178.330f, 7041.822f, 1.867f) },
	{ "Pegasus Aircraft, LSIA", Vector3(-1098.490f, -2415.405f, 14.000f) },
	{ "Playboy Mansion", Vector3(-1475.234f, 167.088f, 55.841f) },
	{ "Quarry", Vector3(2954.196f, 2783.410f, 41.004f) },
	{ "Raton Canyon", Vector3(-589.532f, 4395.390f, 18.148f) },
	{ "Sandy Shores Airfield", Vector3(1747.0f, 3273.7f, 41.1f) },
	{ "Satellite Dishes", Vector3(2062.123f, 2942.055f, 47.431f) },
	{ "Sisyphus Theater Stage", Vector3(686.245f, 577.950f, 130.461f) },
	{ "Tongva Valley River Rapids", Vector3(-1520.259f, 1493.288f, 111.592f) },
	{ "Vespucci Beach", Vector3(-1600.090f, -1041.890f, 13.021f) },
	{ "Weazel Plaza Entrance", Vector3(-914.272f, -457.109f, 39.650f) },
	{ "Weed Farm", Vector3(2208.777f, 5578.235f, 53.735f) },
	{ "Wind Farm", Vector3(2354.0f, 1830.3f, 101.1f) },
};

/* =================== Vehicle Model Lists =================== */

std::map<std::string, std::vector<std::tuple<const char*, const char*, uint32_t>>> Vehicles
{
	{ "Compacts",
	{
		{ "Blista", "Dinka", 0xeb70965f },
		{ "Brioso R/A", "Grotti", 0x5c55cb39 },
		{ "Dilettante", "Karin", 0xbc993509 },
		{ "Dilettante Security", "Karin", 0x64430650 },
		{ "Issi", "Weeny", 0xb9cb3b69 },
		{ "Issi Classic", "Weeny", 0x378236e1 },
		{ "Panto", "Benefactor", 0xe644e480 },
		{ "Prairie", "Bollokan", 0xa988d3a2 },
		{ "Rhapsody", "Declasse", 0x322cf98f },
	}
	},
	{ "Sedans",
	{
		{ "Asea", "Declasse", 0x94204d89 },
		{ "Asea Snow", "Declasse", 0x9441d8d5 },
		{ "Asterope", "Karin", 0x8e9254fb },
		{ "Cognoscenti", "Enus", 0x86fe0b60 },
		{ "Cognoscenti (Armored)", "Enus", 0xdbf2d57a },
		{ "Cognoscenti 55", "Enus", 0x360a438e },
		{ "Cognoscenti 55 (Armored)", "Enus", 0x29fcd3e4 },
		{ "Emperor", "Albany", 0xd7278283 },
		{ "Emperor Rusty", "Albany", 0x8fc3aadc },
		{ "Emperor Snow", "Albany", 0xb5fcf74e },
		{ "Fugitive", "Cheval", 0x71cb2ffb },
		{ "Glendale", "Benefactor", 0x47a6bc1 },
		{ "Ingot", "Vulcar", 0xb3206692 },
		{ "Intruder", "Karin", 0x34dd8aa1 },
		{ "Premier", "Declasse", 0x8fb66f9b },
		{ "Primo", "Albany", 0xbb6b404f },
		{ "Primo Custom", "Albany", 0x86618eda },
		{ "Regina", "Dundreary", 0xff22d208 },
		{ "Romero Hearse", "Chariot", 0x2560b2fc },
		{ "Schafter", "Benefactor", 0xb52b5113 },
		{ "Schafter LWB (Armored)", "Benefactor", 0x72934be4 },
		{ "Schafter V12 (Armored)", "Benefactor", 0xcb0e7cd9 },
		{ "Stafford", "Enus", 0x1324e960 },
		{ "Stanier", "Vapid", 0xa7ede74d },
		{ "Stratum", "Zirconium", 0x66b4fc45 },
		{ "Stretch", "Dundreary", 0x8b13f083 },
		{ "Super Diamond", "Enus", 0x42f2ed16 },
		{ "Surge", "Cheval", 0x8f0e3594 },
		{ "Tailgater", "Obey", 0xc3ddfdce },
		{ "Turreted Limo", "Benefactor", 0xf92aec4d },
		{ "Warrener", "Vulcar", 0x51d83328 },
		{ "Washington", "Albany", 0x69f06b57 },
	}
	},
	{ "SUVs",
	{
		{ "Baller", "Gallivanter", 0x8852855 },
		{ "Baller LE", "Gallivanter", 0x6ff0f727 },
		{ "Baller LE (Armored)", "Gallivanter", 0x1c09cf5e },
		{ "Baller LE LWB", "Gallivanter", 0x25cbe2e2 },
		{ "Baller LE LWB (Armored)", "Gallivanter", 0x27b4e6b0 },
		{ "Baller Mk1", "Gallivanter", 0xcfca3668 },
		{ "BeeJay XL", "Karin", 0x32b29a4b },
		{ "Cavalcade", "Albany", 0x779f23aa },
		{ "Cavalcade Mk2", "Albany", 0xd0eb2be5 },
		{ "Contender", "Vapid", 0x28b67aca },
		{ "Dubsta", "Benefactor", 0x462fe277 },
		{ "Dubsta Black Mk2", "Benefactor", 0xe882e5f6 },
		{ "FQ 2", "Fathom", 0xbc32a33b },
		{ "Granger", "Declasse", 0x9628879c },
		{ "Gresley", "Bravado", 0xa3fc0f4d },
		{ "Habanero", "Emperor", 0x34b7390f },
		{ "Huntley S", "Enus", 0x1d06d681 },
		{ "Landstalker", "Dundreary", 0x4ba4e8dc },
		{ "Mesa", "Canis", 0x36848602 },
		{ "Mesa Snow", "Canis", 0xd36a4b44 },
		{ "Patriot", "Mammoth", 0xcfcfeb3b },
		{ "Patriot Stretch", "Mammoth", 0xe6e967f8 },
		{ "Radius", "Vapid", 0x9d96b45b },
		{ "Rocoto", "Obey", 0x7f5c91f1 },
		{ "Seminole", "Canis", 0x48ceced3 },
		{ "Serrano", "Benefactor", 0x4fb1a214 },
		{ "XLS", "Benefactor", 0x47bbcf2e },
		{ "XLS (Armored)", "Benefactor", 0xe6401328 },
	}
	},
	{ "Coupes",
	{
		{ "Cognoscenti Cabrio", "Enus", 0x13b57d8a },
		{ "Exemplar", "Dewbauchee", 0xffb15b5e },
		{ "F620", "Ocelot", 0xdcbcbe48 },
		{ "Felon", "Lampadati", 0xe8a8bda8 },
		{ "Felon GT", "Lampadati", 0xfaad85ee },
		{ "Jackal", "Ocelot", 0xdac67112 },
		{ "Oracle", "Ubermacht", 0xe18195b2 },
		{ "Oracle XS", "Ubermacht", 0x506434f6 },
		{ "Sentinel", "Ubermacht", 0x3412ae2d },
		{ "Sentinel XS", "Ubermacht", 0x50732c82 },
		{ "Windsor", "Enus", 0x5e4327c8 },
		{ "Windsor Drop", "Enus", 0x8cf5cae1 },
		{ "Zion", "Ubermacht", 0xbd1b39c3 },
		{ "Zion Cabrio", "Ubermacht", 0xb8e2ae18 },
	}
	},
	{ "Muscle",
	{
		{ "Blade", "Vapid", 0xb820ed5e },
		{ "Buccaneer", "Albany", 0xd756460c },
		{ "Buccaneer Custom", "Albany", 0xc397f748 },
		{ "Burger Shot Stallion", "Declasse", 0xe80f67ee },
		{ "Chino", "Vapid", 0x14d69010 },
		{ "Chino Custom", "Vapid", 0xaed64a63 },
		{ "Coquette BlackFin", "Invetero", 0x2ec385fe },
		{ "Dominator", "Vapid", 0x4ce68ac },
		{ "Dominator GTX", "Vapid", 0xc52c6b93 },
		{ "Duke O'Death", "Imponte", 0xec8f7094 },
		{ "Dukes", "Imponte", 0x2b26f456 },
		{ "Ellie", "Vapid", 0xb472d2b5 },
		{ "Faction", "Willard", 0x81a9cddf },
		{ "Faction Custom", "Willard", 0x95466bdb },
		{ "Faction Custom Donk", "Willard", 0x866bce26 },
		{ "Gauntlet", "Bravado", 0x94b395c5 },
		{ "Hermes", "Albany", 0xe83c17 },
		{ "Hotknife", "Vapid", 0x239e390 },
		{ "Hustler", "Vapid", 0x23ca25f2 },
		{ "Lost Slamvan", "Vapid", 0x31adbbfc },
		{ "Lurcher", "Albany", 0x7b47a6a7 },
		{ "Moonbeam", "Declasse", 0x1f52a43f },
		{ "Moonbeam Custom", "Declasse", 0x710a2b9b },
		{ "Nightshade", "Imponte", 0x8c2bd0dc },
		{ "Phoenix", "Imponte", 0x831a21d5 },
		{ "Picador", "Cheval", 0x59e0fbf3 },
		{ "Pisswasser Dominator", "Vapid", 0xc96b73d9 },
		{ "Rat-Loader", "", 0xd83c13ce },
		{ "Rat-Truck", "Bravado", 0xdce1d9f7 },
		{ "Redwood Gauntlet", "Bravado", 0x14d22159 },
		{ "Ruiner", "Imponte", 0xf26ceff9 },
		{ "Ruiner 2000", "Imponte", 0x381e10bd },
		{ "Ruiner Wreck", "Imponte", 0x2e5afd37 },
		{ "Sabre Turbo", "Declasse", 0x9b909c94 },
		{ "Sabre Turbo Custom", "Declasse", 0xd4ea603 },
		{ "Slamvan", "Vapid", 0x2b7f9de3 },
		{ "Slamvan Custom", "Vapid", 0x42bc5e19 },
		{ "Stallion", "Declasse", 0x72a4c31e },
		{ "Tampa", "Declasse", 0x39f9c898 },
		{ "Vigero", "Declasse", 0xcec6b9b7 },
		{ "Virgo", "Albany", 0xe2504942 },
		{ "Virgo Classic", "Dundreary", 0xfdffb0 },
		{ "Virgo Classic Custom", "Dundreary", 0xca62927a },
		{ "Voodoo", "Declasse", 0x1f3766e3 },
		{ "Voodoo Custom", "Declasse", 0x779b4f2d },
		{ "Weaponized Tampa", "Declasse", 0xb7d9f7f1 },
		{ "Yosemite", "Declasse", 0x6f946279 },
	}
	},
	{ "Sports Classics",
	{
		{ "190z", "Karin", 0x3201dd49 },
		{ "Ardent", "Ocelot", 0x97e5533 },
		{ "Casco", "Lampadati", 0x3822bdfe },
		{ "Cheburek", "RUNE", 0xc514aae0 },
		{ "Cheetah Classic", "Grotti", 0xd4e5f4d },
		{ "Coquette Classic", "Invetero", 0x3c4e2113 },
		{ "Deluxo", "Imponte", 0x586765fb },
		{ "Fagaloa", "Vulcar", 0x6068ad86 },
		{ "Fränken Stange", "Albany", 0xce6b35a4 },
		{ "GT500", "Grotti", 0x8408f33a },
		{ "Infernus Classic", "Pegassi", 0xac33179c },
		{ "JB 700", "Dewbauchee", 0x3eab5555 },
		{ "Mamba", "Declasse", 0x9cfffc56 },
		{ "Manana", "Albany", 0x81634188 },
		{ "Michelli GT", "Lampadati", 0x3e5bd8d9 },
		{ "Monroe", "Pegassi", 0xe62b361b },
		{ "Peyote", "Vapid", 0x6d19ccbc },
		{ "Pigalle", "Lampadati", 0x404b6381 },
		{ "Rapid GT Classic", "Dewbauchee", 0x7a2ef5e4 },
		{ "Retinue", "Vapid", 0x6dbd6c0a },
		{ "Roosevelt", "Albany", 0x6ff6914 },
		{ "Roosevelt Valor", "Albany", 0xdc19d101 },
		{ "Savestra", "Annis", 0x35ded0dd },
		{ "Stinger", "Grotti", 0x5c23af9b },
		{ "Stinger GT", "Grotti", 0x82e499fa },
		{ "Stirling GT", "Benefactor", 0xa29d6d10 },
		{ "Stromberg", "Ocelot", 0x34dba661 },
		{ "Swinger", "Ocelot", 0x1dd4c0ff },
		{ "Torero", "Pegassi", 0x59a9e570 },
		{ "Tornado", "Declasse", 0x1bb290bc },
		{ "Tornado Cabrio", "Declasse", 0x5b42a5c4 },
		{ "Tornado Cabrio Rusty", "Declasse", 0x86cf7cdd },
		{ "Tornado Custom", "Declasse", 0x94da98ef },
		{ "Tornado Rat Rod", "Declasse", 0xa31cb573 },
		{ "Tornado Rusty", "Declasse", 0x690a4153 },
		{ "Turismo Classic", "Grotti", 0xc575df11 },
		{ "Viseris", "Lampadati", 0xe8a8ba94 },
		{ "Z-Type", "Truffade", 0x2d3bd401 },
	}
	},
	{ "Sports",
	{
		{ "9F", "Obey", 0x3d8fa25c },
		{ "9F Cabrio", "Obey", 0xa8e38b01 },
		{ "Alpha", "Albany", 0x2db8d1aa },
		{ "Banshee", "Bravado", 0xc1e908d2 },
		{ "Bestia GTS", "Grotti", 0x4bfcf28b },
		{ "Blista Compact", "Dinka", 0x3dee5eda },
		{ "Buffalo", "Bravado", 0xedd516c6 },
		{ "Buffalo S", "Bravado", 0x2bec3cbe },
		{ "Carbonizzare", "Grotti", 0x7b8ab45f },
		{ "Comet", "Pfister", 0xc1ae4d16 },
		{ "Comet Retro Custom", "Pfister", 0x877358ad },
		{ "Comet SR", "Pfister", 0x276d98a3 },
		{ "Comet Safari", "Pfister", 0x5d1903f9 },
		{ "Coquette", "Invetero", 0x67bc037 },
		{ "Drift Tampa", "Declasse", 0xc0240885 },
		{ "Elegy RH8", "Annis", 0xde3d9d22 },
		{ "Elegy Retro Custom", "Annis", 0xbba2261 },
		{ "Feltzer", "Benefactor", 0x8911b9f5 },
		{ "Flash GT", "Vapid", 0xb4f32118 },
		{ "Furore GT", "Lampadati", 0xbf1691e0 },
		{ "Fusilade", "Schyster", 0x1dc0ba53 },
		{ "Futo", "Karin", 0x7836ce2f },
		{ "GB200", "Vapid", 0x71cbea98 },
		{ "Go Go Monkey Blista", "Dinka", 0xdcbc1c3b },
		{ "Hotring Sabre", "Declasse", 0x42836be5 },
		{ "Jester", "Dinka", 0xb2a716a3 },
		{ "Jester (Racecar)", "Dinka", 0xbe0e6126 },
		{ "Jester Classic", "Dinka", 0xf330cb6a },
		{ "Khamelion", "Hijak", 0x206d1b68 },
		{ "Kuruma", "Karin", 0xae2bfe94 },
		{ "Kuruma (Armored)", "Karin", 0x187d938d },
		{ "Lynx", "Ocelot", 0x1cbdc10b },
		{ "Massacro", "Dewbauchee", 0xf77ade32 },
		{ "Massacro (Racecar)", "Dewbauchee", 0xda5819a3 },
		{ "Neon", "Pfister", 0x91ca96ee },
		{ "Omnis", "Obey", 0xd1ad4937 },
		{ "Pariah", "Ocelot", 0x33b98fe2 },
		{ "Penumbra", "Maibatsu", 0xe9805550 },
		{ "Raiden", "Coil", 0xa4d99b7d },
		{ "Rapid GT", "Dewbauchee", 0x679450af },
		{ "Rapid GT", "Dewbauchee", 0x8cb29a14 },
		{ "Raptor", "BF", 0xd7c56d39 },
		{ "Revolter", "Ubermacht", 0xe78cc3d9 },
		{ "Ruston", "Hijak", 0x2ae524a8 },
		{ "Schafter LWB", "Benefactor", 0x58cf185c },
		{ "Schafter V12", "Benefactor", 0xa774b5a6 },
		{ "Schwartzer", "Benefactor", 0xd37b7976 },
		{ "Sentinel Classic", "Ubermacht", 0x41d149aa },
		{ "Seven-70", "Dewbauchee", 0x97398a4b },
		{ "Specter", "Dewbauchee", 0x706e2b40 },
		{ "Specter Custom", "Dewbauchee", 0x400f5147 },
		{ "Sprunk Buffalo", "Bravado", 0xe2c013e },
		{ "Streiter", "Benefactor", 0x67d2b389 },
		{ "Sultan", "Karin", 0x39da2754 },
		{ "Surano", "Benefactor", 0x16e478c1 },
		{ "Tropos Rallye", "Lampadati", 0x707e63a4 },
		{ "Verlierer", "Bravado", 0x41b77fa4 },
	}
	},
	{ "Super",
	{
		{ "811", "Pfister", 0x92ef6e04 },
		{ "Adder", "Truffade", 0xb779a091 },
		{ "Autarch", "Overflod", 0xed552c74 },
		{ "Banshee 900R", "Bravado", 0x25c5af13 },
		{ "Bullet", "Vapid", 0x9ae6dda1 },
		{ "Cheetah", "Grotti", 0xb1d95da0 },
		{ "Cyclone", "Coil", 0x52ff9437 },
		{ "ETR1", "Emperor", 0x30d3f6d8 },
		{ "Entity XF", "Overflod", 0xb2fe5cf9 },
		{ "Entity XXR", "Overflod", 0x8198aedc },
		{ "FMJ", "Vapid", 0x5502626c },
		{ "GP1", "Progen", 0x4992196c },
		{ "Infernus", "Pegassi", 0x18f25ac7 },
		{ "Itali GTB", "Progen", 0x85e8e76b },
		{ "Itali GTB Custom", "Progen", 0xe33a477b },
		{ "Nero", "Truffade", 0x3da47243 },
		{ "Nero Custom", "Truffade", 0x4131f378 },
		{ "Osiris", "Pegassi", 0x767164d6 },
		{ "Penetrator", "Ocelot", 0x9734f3ea },
		{ "RE-7B", "Annis", 0xb6846a55 },
		{ "Reaper", "Pegassi", 0xdf381e5 },
		{ "Rocket Voltic", "Coil", 0x3af76f4a },
		{ "SC1", "Ubermacht", 0x5097f589 },
		{ "Scramjet", "Declasse", 0xd9f0503d },
		{ "Sultan RS", "Karin", 0xee6024bc },
		{ "T20", "Progen", 0x6322b39a },
		{ "Taipan", "Cheval", 0xbc5dc07e },
		{ "Tempesta", "Pegassi", 0x1044926f },
		{ "Tezeract", "Pegassi", 0x3d7c6410 },
		{ "Turismo R", "Grotti", 0x185484e1 },
		{ "Tyrant", "Overflod", 0xe99011c2 },
		{ "Tyrus", "Progen", 0x7b406efb },
		{ "Vacca", "Pegassi", 0x142e0dc3 },
		{ "Vagner", "Dewbauchee", 0x7397224c },
		{ "Vigilante", "", 0xb5ef4c33 },
		{ "Visione", "Grotti", 0xc4810400 },
		{ "Voltic", "Coil", 0x9f4b77be },
		{ "X80 Proto", "Grotti", 0x7e8f677f },
		{ "XA-21", "Ocelot", 0x36b4a8a9 },
		{ "Zentorno", "Pegassi", 0xac5df515 },
	}
	},
	{ "Motorcycles",
	{
		{ "Akuma", "Dinka", 0x63abade7 },
		{ "Avarus", "LCC", 0x81e38f7f },
		{ "BF400", "Nagasaki", 0x5283265 },
		{ "Bagger", "Western", 0x806b9cc3 },
		{ "Bati 801", "Pegassi", 0xf9300cc5 },
		{ "Bati 801RR", "Pegassi", 0xcadd5d2d },
		{ "Carbon RS", "Nagasaki", 0xabb0c0 },
		{ "Chimera", "Nagasaki", 0x675ed7 },
		{ "Cliffhanger", "Western", 0x17420102 },
		{ "Daemon", "Western", 0xac4e93c9 },
		{ "Daemon The Lost MC", "Western", 0x77934cee },
		{ "Defiler", "Shitzu", 0x30ff0190 },
		{ "Diabolus", "Principe", 0xf1b44f44 },
		{ "Diabolus Custom", "Principe", 0x6abdf65e },
		{ "Double-T", "Dinka", 0x9c669788 },
		{ "Enduro", "Dinka", 0x6882fa73 },
		{ "Esskey", "Pegassi", 0x794cb30c },
		{ "FCR 1000", "Pegassi", 0x25676eaf },
		{ "FCR 1000 Custom", "Pegassi", 0xd2d5e00e },
		{ "Faggio", "Pegassi", 0x350d1ab },
		{ "Faggio Mod", "Pegassi", 0xb328b188 },
		{ "Faggio Sport", "Pegassi", 0x9229e4eb },
		{ "Gargoyle", "Western", 0x2c2c2324 },
		{ "Hakuchou", "Shitzu", 0x4b6c568a },
		{ "Hakuchou Drag", "Shitzu", 0xf0c2a91f },
		{ "Hexer", "LCC", 0x11f76c14 },
		{ "Innovation", "LCC", 0xf683eaca },
		{ "Lectro", "Principe", 0x26321e67 },
		{ "Manchez", "Maibatsu", 0xa5325278 },
		{ "Nemesis", "Principe", 0xda288376 },
		{ "Nightblade", "Western", 0xa0438767 },
		{ "Oppressor", "Pegassi", 0x34b82784 },
		{ "Oppressor Mk II", "Pegassi", 0x7b54a9d3 },
		{ "PCJ 600", "Shitzu", 0xc9ceaf06 },
		{ "Rat Bike", "Western", 0x6facdf31 },
		{ "Ruffian", "Pegassi", 0xcabd11e8 },
		{ "Sanchez", "Maibatsu", 0xa960b13e },
		{ "Sanchez (livery)", "Maibatsu", 0x2ef89e46 },
		{ "Sanctus", "LCC", 0x58e316c7 },
		{ "Shotaro", "Nagasaki", 0xe7d2a16e },
		{ "Sovereign", "Western", 0x2c509634 },
		{ "Thrust", "Dinka", 0x6d6f8f43 },
		{ "Vader", "Shitzu", 0xf79a00f7 },
		{ "Vindicator", "Dinka", 0xaf599f01 },
		{ "Vortex", "Pegassi", 0xdba9dbfc },
		{ "Wolfsbane", "Western", 0xdb20a373 },
		{ "Zombie Bobber", "Western", 0xc3d7c72b },
		{ "Zombie Chopper", "Western", 0xde05fb87 },
	}
	},
	{ "Off-Road",
	{
		{ "Bifta", "BF", 0xeb298297 },
		{ "Blazer", "Nagasaki", 0x8125bcf9 },
		{ "Blazer Aqua", "Nagasaki", 0xa1355f67 },
		{ "Blazer Lifeguard", "Nagasaki", 0xfd231729 },
		{ "Bodhi", "Canis", 0xaa699bb6 },
		{ "Brawler", "Coil", 0xa7ce1bc5 },
		{ "Caracara", "Vapid", 0x4abebf23 },
		{ "Desert Raid", "Vapid", 0xd876dbe2 },
		{ "Dubsta 6x6", "Benefactor", 0xb6410173 },
		{ "Dune Buggy", "BF", 0x9cf21e0f },
		{ "Dune FAV", "BF", 0x711d4738 },
		{ "Duneloader", "Bravado", 0x698521e3 },
		{ "Freecrawler", "Canis", 0xfcc2f483 },
		{ "Hot Rod Blazer", "Nagasaki", 0xb44f0582 },
		{ "Injection", "BF", 0x432aa566 },
		{ "Insurgent", "HVY", 0x7b7e56f0 },
		{ "Insurgent Pick-Up", "HVY", 0x9114eada },
		{ "Insurgent Pick-Up Custom", "HVY", 0x8d4b7a8a },
		{ "Kalahari", "Canis", 0x5852838 },
		{ "Kamacho", "Canis", 0xf8c2e0e7 },
		{ "Liberator", "Vapid", 0xcd93a7db },
		{ "Marshall", "Cheval", 0x49863e9c },
		{ "Menacer", "HVY", 0x79dd18ae },
		{ "Mesa", "Canis", 0x84f42e51 },
		{ "Nightshark", "HVY", 0x19dd9ed1 },
		{ "Ramp Buggy", "", 0xceb28249 },
		{ "Ramp Buggy Open Cage", "", 0xed62bfa9 },
		{ "Rancher XL", "Declasse", 0x6210cbb0 },
		{ "Rancher XL Snow", "Declasse", 0x7341576b },
		{ "Rebel", "Karin", 0x8612b64b },
		{ "Riata", "Vapid", 0xa4a4e453 },
		{ "Rusty Rebel", "Karin", 0xb802dd46 },
		{ "Sandking SWB", "Vapid", 0x3af8c345 },
		{ "Sandking XL", "Vapid", 0xb9210fd0 },
		{ "Space Docker", "", 0x1fd824af },
		{ "Street Blazer", "Nagasaki", 0xe5ba6858 },
		{ "Technical", "Karin", 0x83051506 },
		{ "Technical Aqua", "Karin", 0x4662bcbb },
		{ "Technical Custom", "Karin", 0x50d4d19f },
		{ "Trophy Truck", "Vapid", 0x612f4b6 },
	}
	},
	{ "Industrial",
	{
		{ "Cutter", "HVY", 0xc3fba120 },
		{ "Dock Handler", "", 0x1a7fcefa },
		{ "Dozer", "HVY", 0x7074f39d },
		{ "Dump", "HVY", 0x810369e2 },
		{ "Flatbed", "MTL", 0x50b0215a },
		{ "Guardian", "Vapid", 0x825a9f4c },
		{ "Mixer", "HVY", 0xd138a6bb },
		{ "Mixer Support Wheel", "HVY", 0x1c534995 },
		{ "Rubble", "JoBuilt", 0x9a5b1dcc },
		{ "Tipper", "", 0xc7824e5e },
		{ "Tipper", "Brute", 0x2e19879 },
	}
	},
	{ "Utility",
	{
		{ "Airtug", "", 0x5d0aac8f },
		{ "Army Trailer Flatbed", "", 0xa7ff33f5 },
		{ "Army Trailer Flatbed Driller", "", 0x9e6b14d6 },
		{ "Army Trailer Tanker", "", 0xb8081009 },
		{ "Baletrailer", "", 0xe82ae656 },
		{ "Boat Trailer", "", 0x1f3d44b5 },
		{ "Boat Trailer and Boat", "", 0x6a59902d },
		{ "Caddy Bunker", "", 0xd227bdbb },
		{ "Caddy Civilian", "", 0xdff0594c },
		{ "Caddy Fairway", "", 0x44623884 },
		{ "Docktug", "", 0xcb44b1ca },
		{ "Forklift", "HVY", 0x58e49664 },
		{ "Lawn Mower", "", 0x6a4bd8f6 },
		{ "Mobile Operations Center", "", 0x5993f939 },
		{ "Ripley", "", 0xcd935ef9 },
		{ "Sadler", "Vapid", 0xdc434e51 },
		{ "Sadler Snow", "Vapid", 0x2bc345d1 },
		{ "Scrap Truck", "", 0x9a9fd3df },
		{ "Towtruck", "", 0xe5a2d6c6 },
		{ "Towtruck Heavy Duty", "", 0xb12314e0 },
		{ "Tractor Antique", "", 0x61d6ba8c },
		{ "Tractor Fieldmaster", "Stanley", 0x843b73de },
		{ "Tractor Fieldmaster Snow", "Stanley", 0x562a97bd },
		{ "Trailer Car Transport", "", 0x7cab34d0 },
		{ "Trailer Car Transport Empty", "", 0x7be032c6 },
		{ "Trailer Container", "", 0x806efbee },
		{ "Trailer Container/Curtain-side)", "", 0xcbb2be0e },
		{ "Trailer Fame or Shame TV", "", 0x967620be },
		{ "Trailer Flatbed tri axle", "", 0xd1abb666 },
		{ "Trailer Flatbed twin axle", "", 0xaf62f6b2 },
		{ "Trailer Freight", "", 0xa1da3c91 },
		{ "Trailer Freight Canvas Side", "", 0x8548036d },
		{ "Trailer Grain", "", 0x3cc7f596 },
		{ "Trailer Logging", "", 0x782a236d },
		{ "Trailer Meth Lab", "", 0x153e1b0a },
		{ "Trailer Scarifier Rake", "", 0x174cb172 },
		{ "Trailer Shipping Container", "", 0xbe66f5aa },
		{ "Trailer Tanker", "", 0x74998082 },
		{ "Trailer Tanker Ron", "", 0xd46f4737 },
		{ "Trailer Tradesman", "", 0x2a72beab },
		{ "Utility Truck", "", 0x7f2153df },
		{ "Utility Truck Plumbing", "", 0x34e6bf6b },
		{ "Utility Truck Telephone", "", 0x1ed0a534 },
	}
	},
	{ "Vans",
	{
		{ "Bison", "Bravado", 0xfefd644f },
		{ "Bison Construction", "Bravado", 0x7b8297c5 },
		{ "Bison Ladder Rack", "Bravado", 0x67b3f020 },
		{ "Bobcat XL", "Vapid", 0x3fc5d440 },
		{ "Boxville", "", 0x898eccea },
		{ "Boxville Armored", "", 0x28ad20e1 },
		{ "Boxville Go Postal", "", 0xf21b33be },
		{ "Boxville Humane Labs", "Brute", 0x7405e08 },
		{ "Boxville Post OP", "Brute", 0x1a79847a },
		{ "Brute Pony (Business)", "Brute", 0xf8de29a8 },
		{ "Brute Pony Cannabis Shop", "Brute", 0x38408341 },
		{ "Burrito", "Declasse", 0x98171bd3 },
		{ "Burrito Bug Stars", "Declasse", 0xc9e8ff76 },
		{ "Burrito Construction", "Declasse", 0x353b561d },
		{ "Burrito Gang", "Declasse", 0x11aa0e14 },
		{ "Burrito Graphics", "Declasse", 0xafbb2ca4 },
		{ "Burrito Snow", "Declasse", 0x437cf2a0 },
		{ "Burrito The Lost MC", "Declasse", 0x97fa4f36 },
		{ "Camper", "Brute", 0x6fd95f68 },
		{ "Clown Van", "Vapid", 0x2b6dc64a },
		{ "Journey", "Zirconium", 0xf8d48e7a },
		{ "Minivan", "Vapid", 0xed7eada4 },
		{ "Minivan Custom", "Vapid", 0xbcde91f0 },
		{ "Paradise", "Bravado", 0x58b3979c },
		{ "Rumpo Custom", "Bravado", 0x57f682af },
		{ "Rumpo Deludamol", "Bravado", 0x961afef7 },
		{ "Rumpo Plain/Weazel", "Bravado", 0x4543b74d },
		{ "Speedo", "Vapid", 0xcfb3870c },
		{ "Speedo Custom", "Vapid", 0xd17099d },
		{ "Surfer", "BF", 0x29b0da97 },
		{ "Surfer Rusty", "BF", 0xb1d80e06 },
		{ "Taco Van", "", 0x744ca80d },
		{ "Youga", "Bravado", 0x3e5f6b8 },
		{ "Youga Classic", "Bravado", 0x3d29cd2b },
	}
	},
	{ "Cycles",
	{
		{ "BMX", "", 0x43779c54 },
		{ "Cruiser", "", 0x1aba13b5 },
		{ "Endurex Race Bike", "", 0xb67597ec },
		{ "Fixter", "", 0xce23d3bf },
		{ "Scorcher", "", 0xf4e1aa15 },
		{ "Tri-Cycles Race Bike", "", 0xe823fb48 },
		{ "Whippet Race Bike", "", 0x4339cd69 },
	}
	},
	{ "Boats",
	{
		{ "Dinghy 2 Seat", "Nagasaki", 0x107f392c },
		{ "Dinghy 4 Seat, Black", "Nagasaki", 0x1e5e54ea },
		{ "Dinghy 4 Seat, Red", "Nagasaki", 0x3d961290 },
		{ "Dinghy 4 Seat, Yacht", "Nagasaki", 0x33b47f96 },
		{ "Jetmax", "Shitzu", 0x33581161 },
		{ "Kraken", "", 0xc07107ee },
		{ "Marquis", "Dinka", 0xc1ce1183 },
		{ "Police Predator", "", 0xe2e7d4ab },
		{ "Seashark", "Speedophile", 0xc2974024 },
		{ "Seashark Lifeguard", "Speedophile", 0xdb4388e4 },
		{ "Seashark Yacht", "Speedophile", 0xed762d49 },
		{ "Speeder", "Pegassi", 0xdc60d2b },
		{ "Speeder Yacht", "Pegassi", 0x1a144f2a },
		{ "Squalo", "Shitzu", 0x17df5ec2 },
		{ "Submersible", "", 0x2dff622f },
		{ "Suntrap", "Shitzu", 0xef2295c9 },
		{ "Toro", "Lampadati", 0x3fd5aa2f },
		{ "Toro Yacht", "Lampadati", 0x362cac6d },
		{ "Tropic", "Shitzu", 0x1149422f },
		{ "Tropic Yacht", "Shitzu", 0x56590fe9 },
		{ "Tug", "", 0x82cac433 },
	}
	},
	{ "Helicopters",
	{
		{ "Akula", "", 0x46699f47 },
		{ "Annihilator", "", 0x31f0b376 },
		{ "Buzzard", "", 0x2c75f0dd },
		{ "Buzzard Attack Chopper", "", 0x2f03547b },
		{ "Cargobob Camo", "", 0xfcfcb68b },
		{ "Cargobob Camo Closed", "", 0x78bc1a3c },
		{ "Cargobob Camo TPE", "", 0x53174eef },
		{ "Cargobob Jetsam", "", 0x60a7ea10 },
		{ "FH-1 Hunter", "", 0xfd707ede },
		{ "Frogger", "", 0x2c634fbd },
		{ "Frogger TPE/FIB", "", 0x742e9ac0 },
		{ "Havok", "Nagasaki", 0x89ba59f5 },
		{ "Maverick", "", 0x9d0450ca },
		{ "Police Maverick", "", 0x1517d4d9 },
		{ "Savage", "", 0xfb133a17 },
		{ "Sea Sparrow", "", 0xd4ae63d9 },
		{ "Skylift", "", 0x3e48bf23 },
		{ "SuperVolito", "Buckingham", 0x2a54c47d },
		{ "SuperVolito Carbon", "Buckingham", 0x9c5e5644 },
		{ "Swift", "Buckingham", 0xebc24df2 },
		{ "Swift Deluxe", "Buckingham", 0x4019cb4c },
		{ "Valkyrie", "", 0xa09e15fd },
		{ "Valkyrie MOD.0", "", 0x5bfa5c4b },
		{ "Volatus", "Buckingham", 0x920016f1 },
	}
	},
	{ "Planes",
	{
		{ "Alpha-Z1", "Buckingham", 0xa52f6866 },
		{ "Atomic Blimp", "", 0xf7004c86 },
		{ "Avenger", "Mammoth", 0x81bd2ed0 },
		{ "Avenger Custom", "Mammoth", 0x18606535 },
		{ "B-11 Strikeforce", "", 0x64de07a1 },
		{ "Besra", "Western", 0x6cbd1d6d },
		{ "Blimp", "", 0xeda4ed97 },
		{ "Cargo Plane", "", 0x15f27762 },
		{ "Cuban 800", "", 0xd9927fe3 },
		{ "Dodo", "Mammoth", 0xca495705 },
		{ "Duster", "", 0x39d6779e },
		{ "Howard NX-25", "Buckingham", 0xc3f25753 },
		{ "Hydra", "Mammoth", 0x39d6e83f },
		{ "Jet", "", 0x3f119114 },
		{ "LF-22 Starling", "", 0x9a9eb7de },
		{ "Luxor", "Buckingham", 0x250b0c5e },
		{ "Luxor Deluxe", "Buckingham", 0xb79f589e },
		{ "Mallard", "", 0x81794c70 },
		{ "Mammatus", "", 0x97e55d11 },
		{ "Miljet", "Buckingham", 0x9d80f93 },
		{ "Mogul", "Mammoth", 0xd35698ef },
		{ "Nimbus", "Buckingham", 0xb2cf7250 },
		{ "P-45 Nokota", "", 0x3dc92356 },
		{ "P-996 LAZER", "", 0xb39b0ae6 },
		{ "Pyro", "Buckingham", 0xad6065c0 },
		{ "RM-10 Bombushka", "", 0xfe0a508c },
		{ "Rogue", "Western", 0xc5dd6967 },
		{ "Seabreeze", "Western", 0xe8983f9f },
		{ "Shamal", "Buckingham", 0xb79c1bf5 },
		{ "Titan", "", 0x761e2ad3 },
		{ "Tula", "Mammoth", 0x3e2e4f8a },
		{ "Ultralight", "Nagasaki", 0x96e24857 },
		{ "V-65 Molotok", "", 0x5d56f01b },
		{ "Velum", "", 0x9c429b6a },
		{ "Velum 5-Seater", "", 0x403820e8 },
		{ "Vestra", "Buckingham", 0x4ff77e37 },
		{ "Volatol", "", 0x1aad0ded },
		{ "Xero Blimp", "", 0xdb6b4924 },
	}
	},
	{ "Service",
	{
		{ "Airport Bus", "", 0x4c80eb0e },
		{ "Brickade", "MTL", 0xedc6f847 },
		{ "Bus", "", 0xd577c962 },
		{ "Dashound", "", 0x84718d34 },
		{ "Dune", "MTL", 0x829a3c44 },
		{ "Festival Bus", "", 0x149bd32a },
		{ "Rental Shuttle Bus", "", 0xbe819c63 },
		{ "Taxi", "", 0xc703db5f },
		{ "Tourbus", "", 0x73b1c3cb },
		{ "Trashmaster Butt Lovers", "", 0xb527915c },
		{ "Trashmaster Little Pricks", "", 0x72435a19 },
		{ "Wastelander", "MTL", 0x8e08ec82 },
	}
	},
	{ "Emergency",
	{
		{ "Ambulance", "", 0x45d56ada },
		{ "FIB Buffalo", "", 0x432ea949 },
		{ "FIB Granger", "", 0x9dc66994 },
		{ "Fire Truck", "", 0x73920f8e },
		{ "Lifeguard", "", 0x1bf8d381 },
		{ "Park Ranger", "", 0x2c33b46e },
		{ "Police Bike", "", 0xfdefaec3 },
		{ "Police Cruiser", "", 0x79fbb0c5 },
		{ "Police Cruiser Buffalo", "", 0x9f05f101 },
		{ "Police Cruiser Interceptor", "", 0x71fa16ea },
		{ "Police Rancher", "", 0xa46462f7 },
		{ "Police Riot", "", 0xb822a1aa },
		{ "Police Roadcruiser", "", 0x95f4c618 },
		{ "Police Transporter", "", 0x1b38e955 },
		{ "Prison Bus", "", 0x885f3671 },
		{ "RCV", "", 0x9b16a3b4 },
		{ "Sheriff Cruiser", "", 0x9baa707c },
		{ "Sheriff SUV", "", 0x72935408 },
		{ "Unmarked Cruiser", "", 0x8a63c7b9 },
	}
	},
	{ "Military",
	{
		{ "APC", "HVY", 0x2189d250 },
		{ "Anti-Aircraft Trailer", "Vom Feuer", 0x8fd54ebb },
		{ "Barracks", "", 0xceea3f4b },
		{ "Barracks Semi", "HVY", 0x4008eabb },
		{ "Barracks short canvas", "", 0x2592b5cf },
		{ "Barrage", "", 0xf34dfb25 },
		{ "Chernobog", "", 0xd6bc7523 },
		{ "Crusader", "Canis", 0x132d5a1a },
		{ "Half-track", "Bravado", 0xfe141da6 },
		{ "Rhino Tank", "", 0x2ea68690 },
		{ "TM-02 Khanjali", "", 0xaa6f980a },
		{ "Thruster", "Mammoth", 0x58cdaf30 },
	}
	},
	{ "Commercial",
	{
		{ "Benson", "Vapid", 0x7a61b330 },
		{ "Biff", "HVY", 0x32b91ae8 },
		{ "Hauler", "JoBuilt", 0x5a82f9ae },
		{ "Hauler Custom", "JoBuilt", 0x171c92c4 },
		{ "Mule Custom", "Maibatsu", 0x73f4110e },
		{ "Mule Graphics 1", "Maibatsu", 0x35ed670b },
		{ "Mule Graphics 2", "Maibatsu", 0xc1632beb },
		{ "Mule Plain", "Maibatsu", 0x85a5b471 },
		{ "Packer", "MTL", 0x21eee87d },
		{ "Phantom", "JoBuilt", 0x809aa4cb },
		{ "Phantom Custom", "JoBuilt", 0xa90ed5c },
		{ "Phantom Wedge", "JoBuilt", 0x9dae1398 },
		{ "Pounder", "MTL", 0x7de35e7d },
		{ "Pounder Custom", "MTL", 0x6290f15b },
		{ "Stockade", "Brute", 0x6827cf72 },
		{ "Stockade Snow", "Brute", 0xf337ab36 },
		{ "Terrorbyte", "Benefactor", 0x897afc65 },
	}
	},
	{ "Trains",
	{
		{ "Cable Car", "", 0xc6c3242d },
		{ "Container Car", "", 0x36dcff98 },
		{ "Container Car 2", "", 0xe512e79 },
		{ "Flatbed Car", "", 0xafd22a6 },
		{ "Freight Train", "", 0x3d6aaa9b },
		{ "Grain Car", "", 0x264d9262 },
		{ "Metro Passenger Train", "", 0x33c9e158 },
		{ "Oil Tanker Car", "", 0x22eddc30 },
	}
	},
};

// Updates all features that can be turned off by the game, being called each game frame
void update_features() 
{
	update_status_text();

	while (!g_nativeQueue.empty())
	{
		g_nativeQueue.front()();
		g_nativeQueue.pop_front();
	}

	if (ProcessPlayerButton > -1) ProcessPlayerButtons();

	if (ProcessWeaponButton > -1) ProcessWeaponButtons();

	if (ProcessVehicleButton > -1) ProcessVehicleButtons();

	if (ProcessWorldButton > -1) ProcessWorldButtons();

	if (ProcessTimeButton > -1) ProcessTimeButtons();

	if (ProcessWeatherButton > -1) ProcessWeatherButtons();

	if (ProcessMiscButton > -1) ProcessMiscButtons();

	// changing player model if died/arrested while being in another skin, since it can cause inf loading loop
	if (skinchanger_used)
		check_player_model(NULL);

	// wait until player is ready, basicly to prevent using the trainer while player is dead or arrested
	while (ENTITY::IS_ENTITY_DEAD(PLAYER::PLAYER_PED_ID(), 0) || PLAYER::IS_PLAYER_BEING_ARRESTED(PLAYER::PLAYER_ID(), TRUE))
		WAIT(0);

	update_vehicle_guns();

	// read default feature values from the game
	featureWorldRandomCops = PED::CAN_CREATE_RANDOM_COPS() == TRUE;

	// common variables
	Vehicle playerVeh = NULL;
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();	
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);
	BOOL bPlayerInVeh = FALSE;
	const int PED_FLAG_CAN_FLY_THRU_WINDSCREEN = 32;
	if (bPlayerExists)
	{
		bPlayerInVeh = PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0);
		bPlayerInVeh ? playerVeh = PED::GET_VEHICLE_PED_IS_USING(playerPed) : playerVeh = NULL;

		/* PLAYER */

		if (featurePlayerInvincibleUpdated)
		{
			if (!featurePlayerInvincible)
				PLAYER::SET_PLAYER_INVINCIBLE(player, FALSE);
			featurePlayerInvincibleUpdated = false;
		}
		if (featurePlayerInvincible)
		{
			PLAYER::SET_PLAYER_INVINCIBLE(player, TRUE);
		}

		if (featurePlayerNeverWanted)
		{
			PLAYER::CLEAR_PLAYER_WANTED_LEVEL(player);
		}

		if (featurePlayerIgnoredUpdated)
		{
			PLAYER::SET_POLICE_IGNORE_PLAYER(player, featurePlayerIgnored);
			featurePlayerIgnoredUpdated = false;
		}

		if (featurePlayerUnlimitedAbility)
		{
			PLAYER::SPECIAL_ABILITY_FILL_METER(player, 1);
		}

		if (featurePlayerNoNoiseUpdated)
		{
			if (!featurePlayerNoNoise)
				PLAYER::SET_PLAYER_NOISE_MULTIPLIER(player, 1.0);
			featurePlayerNoNoiseUpdated = false;
		}
		if (featurePlayerNoNoise)
			PLAYER::SET_PLAYER_NOISE_MULTIPLIER(player, 0.0);

		if (featurePlayerFastSwimUpdated)
		{
			if (!featurePlayerFastSwim)
				PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(player, 1.0);
			featurePlayerFastSwimUpdated = false;
		}
		if (featurePlayerFastSwim)
			PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(player, 1.49);

		if (featurePlayerFastRunUpdated)
		{
			if (!featurePlayerFastRun)
				PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, 1.0);
			featurePlayerFastRunUpdated = false;
		}
		if (featurePlayerFastRun)
			PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, 1.49);

		if (featurePlayerSuperJump)
		{
			GAMEPLAY::SET_SUPER_JUMP_THIS_FRAME(player);
		}

		/* WEAPON */

		if (featureWeaponFireAmmo)
		{
			GAMEPLAY::SET_FIRE_AMMO_THIS_FRAME(player);
		}
		if (featureWeaponExplosiveAmmo)
		{
			GAMEPLAY::SET_EXPLOSIVE_AMMO_THIS_FRAME(player);
		}
		if (featureWeaponExplosiveMelee)
		{
			GAMEPLAY::SET_EXPLOSIVE_MELEE_THIS_FRAME(player);
		}
		if (featureWeaponTpImpact)
		{
			if (PED::IS_PED_SHOOTING(playerPed))
			{
				Vector3 iCoord;
				if (WEAPON::GET_PED_LAST_WEAPON_IMPACT_COORD(playerPed, &iCoord))
				{
					ENTITY::SET_ENTITY_COORDS(playerPed, iCoord.x, iCoord.y, iCoord.z + 1, 0, 0, 1, 1);
					set_status_text("X:" + std::to_string(iCoord.x) + "\tY:" + std::to_string(iCoord.y) + "\tZ:" + std::to_string(iCoord.z));
				}
			}
		}
		if (featureWeaponNoReload)
		{
			Hash cur;
			if (WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &cur, 1))
			{
				if (WEAPON::IS_WEAPON_VALID(cur))
				{
					int maxAmmo;
					if (WEAPON::GET_MAX_AMMO(playerPed, cur, &maxAmmo))
					{
						WEAPON::SET_PED_AMMO(playerPed, cur, maxAmmo, 0);

						maxAmmo = WEAPON::GET_MAX_AMMO_IN_CLIP(playerPed, cur, 1);
						if (maxAmmo > 0)
							WEAPON::SET_AMMO_IN_CLIP(playerPed, cur, maxAmmo);
					}
				}
			}
		}

		/* VEHICLE */
		if (bPlayerInVeh)
		{
			if (featureVehInvincibleUpdated)
			{
				if (!featureVehInvincible)
				{
					ENTITY::SET_ENTITY_INVINCIBLE(playerVeh, FALSE);
					ENTITY::SET_ENTITY_PROOFS(playerVeh, 0, 0, 0, 0, 0, 0, 0, 0);
					VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(playerVeh, 1);
					VEHICLE::SET_VEHICLE_WHEELS_CAN_BREAK(playerVeh, 1);
					VEHICLE::SET_VEHICLE_CAN_BE_VISIBLY_DAMAGED(playerVeh, 1);
				}
				featureVehInvincibleUpdated = false;
			}
			if (featureVehInvincible)
			{
				ENTITY::SET_ENTITY_INVINCIBLE(playerVeh, TRUE);
				ENTITY::SET_ENTITY_PROOFS(playerVeh, 1, 1, 1, 1, 1, 1, 1, 1);
				VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(playerVeh, 0);
				VEHICLE::SET_VEHICLE_WHEELS_CAN_BREAK(playerVeh, 0);
				VEHICLE::SET_VEHICLE_CAN_BE_VISIBLY_DAMAGED(playerVeh, 0);
			}

			if (featureVehInvincibleWheelsUpdated)
			{
				if (!featureVehInvincibleWheels)
				{
					VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(playerVeh, TRUE);
					VEHICLE::SET_VEHICLE_WHEELS_CAN_BREAK(playerVeh, TRUE);
					VEHICLE::SET_VEHICLE_HAS_STRONG_AXLES(playerVeh, FALSE);
				}
				featureVehInvincibleWheelsUpdated = false;
			}
			if (featureVehInvincibleWheels)
			{
				VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(playerVeh, FALSE);
				VEHICLE::SET_VEHICLE_WHEELS_CAN_BREAK(playerVeh, FALSE);
				VEHICLE::SET_VEHICLE_HAS_STRONG_AXLES(playerVeh, TRUE);
			}

			if (featureVehSpeedBoost)
			{
				static int NosTimer = 0;
				float speed = ENTITY::GET_ENTITY_SPEED(playerVeh);
				bool boostKey = IsKeyDown(VK_NUMPAD9) || CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleHorn);
				bool stopKey = IsKeyDown(VK_NUMPAD3) || CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleBrake);
				if (!VEHICLE::_HAS_VEHICLE_ROCKET_BOOST(playerVeh))
				{
					if (boostKey)
					{
						NosTimer = GAMEPLAY::GET_GAME_TIMER() + 1000;
						if (speed < 3.0f) speed = 3.0f; speed += speed * 0.025f;
						VEHICLE::SET_VEHICLE_FORWARD_SPEED(playerVeh, speed);
						AUDIO::SET_VEHICLE_BOOST_ACTIVE(playerVeh, true);
						GRAPHICS::_START_SCREEN_EFFECT("RaceTurbo", 0, 0);
					}
					else
					{
						NosTimer = 0;
						AUDIO::SET_VEHICLE_BOOST_ACTIVE(playerVeh, false);
					}

					if (GAMEPLAY::GET_GAME_TIMER() <= NosTimer)
					{
						AUDIO::SET_VEHICLE_BOOST_ACTIVE(playerVeh, 0);
						GRAPHICS::_STOP_SCREEN_EFFECT("RaceTurbo");
					}
				}
				else 
				{	// Voltic2 infinity rockets
					if (boostKey)
					{
						VEHICLE::_SET_VEHICLE_ROCKET_BOOST_REFILL_TIME(playerVeh, 0);
						VEHICLE::_SET_VEHICLE_ROCKET_BOOST_PERCENTAGE(playerVeh, 2.5);
						if (!VEHICLE::_IS_VEHICLE_ROCKET_BOOST_ACTIVE(playerVeh)) VEHICLE::_SET_VEHICLE_ROCKET_BOOST_ACTIVE(playerVeh, true);
					}   else VEHICLE::_SET_VEHICLE_ROCKET_BOOST_ACTIVE(playerVeh, false);
				}

				if (stopKey)
				{
					if (ENTITY::IS_ENTITY_IN_AIR(playerVeh) || speed > 5.0)
					{
						VEHICLE::SET_VEHICLE_FORWARD_SPEED(playerVeh, 0.0);
					}
				}
			}

			if (featureVehSeatbelt)
			{

				if (PED::GET_PED_CONFIG_FLAG(playerPed, PED_FLAG_CAN_FLY_THRU_WINDSCREEN, TRUE))
					PED::SET_PED_CONFIG_FLAG(playerPed, PED_FLAG_CAN_FLY_THRU_WINDSCREEN, FALSE);
			}
		}
		if (featureVehSeatbeltUpdated)
		{
			if (!featureVehSeatbelt)
				PED::SET_PED_CONFIG_FLAG(playerPed, PED_FLAG_CAN_FLY_THRU_WINDSCREEN, TRUE);
			featureVehSeatbeltUpdated = false;
		}
	}

	/* MISC */
		
	if (featureTimePausedUpdated)
	{
		TIME::PAUSE_CLOCK(featureTimePaused);
		featureTimePausedUpdated = false;
	}

	if (featureTimeSynced)
	{
		time_t now = time(0);
		tm t;
		localtime_s(&t, &now);
		TIME::SET_CLOCK_TIME(t.tm_hour, t.tm_min, t.tm_sec);
	}

	if (featureMiscHideHud)
		UI::HIDE_HUD_AND_RADAR_THIS_FRAME();
}

void reset_globals()
{
	featurePlayerInvincible				=
	featurePlayerInvincibleUpdated		=
	featurePlayerNeverWanted			=
	featurePlayerIgnored				=
	featurePlayerIgnoredUpdated			=
	featurePlayerUnlimitedAbility		=
	featurePlayerNoNoise				=
	featurePlayerNoNoiseUpdated			=
	featurePlayerFastSwim				=
	featurePlayerFastSwimUpdated		=
	featurePlayerFastRun				=
	featurePlayerFastRunUpdated			=
	featurePlayerSuperJump				=
	featureWeaponNoReload				=
	featureWeaponFireAmmo				=
	featureWeaponExplosiveAmmo			=
	featureWeaponExplosiveMelee			=
	featureWeaponVehRockets				=
	featureVehInvincible				=
	featureVehInvincibleUpdated			=
	featureVehInvincibleWheels			=
	featureVehInvincibleWheelsUpdated	=
	featureVehSeatbelt					=
	featureVehSeatbeltUpdated			=
	featureVehSpeedBoost				=
	featureVehWrapInSpawned				=
	featureWorldMoonGravity				=
	featureTimePaused					=
	featureTimePausedUpdated			=
	featureTimeSynced					=
	featureWeatherWind					=
	featureWeatherPers					=
	featureMiscLockRadio				=
	featureMiscHideHud					=	false;

	featureWorldRandomCops		=
	featureWorldRandomTrains	=
	featureWorldRandomBoats		=
	featureWorldGarbageTrucks	=	true;

	skinchanger_used			=
	vehicleSpawnWarpInto		=
	vehicleSpawnUpgrade			= false;

	ProcessPlayerButton			=
	ProcessWeaponButton			=
	ProcessVehicleButton		=
	ProcessWorldButton			=
	ProcessTimeButton			=
	ProcessWeatherButton		=
	ProcessMiscButton			= -1;

	*reinterpret_cast<__int64*>(getGlobalPtr(4266042)) = 1;//Disable Unspawn SP
}

void main()
{	
	reset_globals();

	while (true)
	{
		if (trainer_switch_pressed())
		{
			menu_beep();
			is_menu_open ^= 1;
		}

		for (auto it = g_callResults.begin(); it != g_callResults.end(); )
		{
			if (std::get<0>(*it)())
			{
				std::get<1>(*it)();

				it = g_callResults.erase(it);
			}
			else
			{
				it++;
			}
		}

		update_features();

		WAIT(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}

void RenderDirect3d()
{
	try
	{
		// start draw
		ImGui_ImplDX11_NewFrame();

		static bool no_titlebar = false;
		//static bool no_border = true;
		static bool no_resize = false;
		static bool auto_resize = false;
		static bool no_move = false;
		static bool no_scrollbar = false;
		static bool no_collapse = false;
		static bool no_menu = true;
		static bool start_pos_set = false;

		// various window flags. Typically you would just use the default.
		ImGuiWindowFlags	window_flags = 0;
		if (no_titlebar)	window_flags |= ImGuiWindowFlags_NoTitleBar;
		//if (!no_border)		window_flags |= ImGuiWindowFlags_ShowBorders;
		if (no_resize)		window_flags |= ImGuiWindowFlags_NoResize;
		if (auto_resize)	window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
		if (no_move)		window_flags |= ImGuiWindowFlags_NoMove;
		if (no_scrollbar)	window_flags |= ImGuiWindowFlags_NoScrollbar;
		if (no_collapse)	window_flags |= ImGuiWindowFlags_NoCollapse;
		if (!no_menu)		window_flags |= ImGuiWindowFlags_MenuBar;

		ImGui::GetIO().MouseDrawCursor = is_menu_open || show_test_window || show_another_window;

		NQ_ setGameInputToEnabled(ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard || playerControl); _NQ
		
		ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiSetCond_FirstUseEver);
		if (!start_pos_set) { ImGui::SetNextWindowPos(ImVec2(10, 10)); start_pos_set = true; }

		if (is_menu_open)
		{
			ImGui::Begin("ImGUI Trainer", &is_menu_open, window_flags);

			//ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);    // 2/3 of the space for widget and 1/3 for labels
			ImGui::PushItemWidth(-140);                                 // Right align, keep 140 pixels for labels
			ImGui::TextColored(ImColor(0, 128, 255), "example implementation"); ImGui::SameLine();
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate / 2);

			if (ImGui::CollapsingHeader("Player Options"))
			{
				const int lineCount = 13;

				static struct 
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Fix Player", nullptr, nullptr },
					{ "Reset Skin", nullptr, nullptr },
					{ "Add Cash", nullptr, nullptr },
					{ "Wanted Up", nullptr, nullptr },
					{ "Wanted Down", nullptr, nullptr },
					{ "Never Wanted", &featurePlayerNeverWanted },
					{ "Invincible", &featurePlayerInvincible, &featurePlayerInvincibleUpdated },
					{ "Police Ignore You", &featurePlayerIgnored, &featurePlayerIgnoredUpdated },
					{ "Unlimited Ability", &featurePlayerUnlimitedAbility, nullptr },
					{ "Noiseless", &featurePlayerNoNoise, &featurePlayerNoNoiseUpdated },
					{ "Fast Swim", &featurePlayerFastSwim, &featurePlayerFastSwimUpdated },
					{ "Fast Run", &featurePlayerFastRun, &featurePlayerFastRunUpdated },
					{ "Super Jump", &featurePlayerSuperJump, nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessPlayerButton = i;
						ImGui::PopItemWidth();
					}
				}

				if (ImGui::TreeNode("Skin Changer"))
				{
					if (ImGui::TreeNode("Player Skins"))
					{
						static int item = -1;
						ImGui::Combo("PlayerSkins", &item, &get_labels(skins_players)[0], (int)std::size(skins_players)); ImGui::SameLine();
						if (ImGui::Button("Apply"))
						{
							NQ_ check_player_model(std::get<1>(skins_players[item])); _NQ
						}
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Animal Skins"))
					{
						static int item = -1;
						ImGui::Combo("AnimalSkins", &item, &get_labels(skins_animals)[0], (int)std::size(skins_animals)); ImGui::SameLine();
						if (ImGui::Button("Apply"))
						{
							NQ_ check_player_model(std::get<1>(skins_animals[item])); _NQ
						}
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("General Skins"))
					{
						static int item = -1;
						ImGui::Combo("GeneralSkins", &item, &get_labels(skins_general)[0], (int)std::size(skins_general)); ImGui::SameLine();
						if (ImGui::Button("Apply"))
						{
							NQ_ check_player_model(std::get<1>(skins_general[item])); _NQ
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Teleport Locations"))
				{
					if (ImGui::TreeNode("Safe Houses"))
					{
						static int item = -1;
						ImGui::Combo("SafeHouses", &item, &get_labels(locations_safe)[0], (int)std::size(locations_safe)); ImGui::SameLine();
						if (ImGui::Button("Teleport"))
						{
							NQ_ teleport_to_location(std::get<1>(locations_safe[item])); _NQ
						}
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Landmarks "))
					{
						static int item = -1;
						ImGui::Combo("Landmarks", &item, &get_labels(locations_landmarks)[0], (int)std::size(locations_landmarks)); ImGui::SameLine();
						if (ImGui::Button("Teleport"))
						{
							NQ_ teleport_to_location(std::get<1>(locations_landmarks[item])); _NQ
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
			}

			if (ImGui::CollapsingHeader("Weapon Options"))
			{
				const int lineCount = 7;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Get All Weapons", nullptr, nullptr },
					{ "No Reload", &featureWeaponNoReload, nullptr },
					{ "Teleport To Impact",	&featureWeaponTpImpact,	nullptr },
					{ "Fire Ammo", &featureWeaponFireAmmo, nullptr },
					{ "Explosive Ammo", &featureWeaponExplosiveAmmo, nullptr },
					{ "Explosive Melee", &featureWeaponExplosiveMelee, nullptr },
					{ "Vehicle Rockets", &featureWeaponVehRockets, nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessWeaponButton = i;
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("Vehicle Options"))
			{
				const int lineCount = 8;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Paint Random", nullptr, nullptr },
					{ "Upgrade Everything", nullptr, nullptr },
					{ "Fix", nullptr, nullptr },
					{ "Seat Belt", &featureVehSeatbelt, &featureVehSeatbeltUpdated },
					{ "Warp Into Spawned", &featureVehWrapInSpawned, nullptr },
					{ "Invincible", &featureVehInvincible, &featureVehInvincibleUpdated },
					{ "Strong Wheels", &featureVehInvincibleWheels, &featureVehInvincibleWheelsUpdated },
					{ "Speed Boost", &featureVehSpeedBoost, nullptr }
				};

				//Spawning
				if (ImGui::TreeNode("Spawning##vehicle"))
				{
					if (ImGui::TreeNode("Spawning Options"))
					{
						ImGui::Checkbox("Warp into", &vehicleSpawnWarpInto); ImGui::SameLine();
						ImGui::Checkbox("Full Tuned", &vehicleSpawnUpgrade);
						ImGui::Separator();
						ImGui::TreePop();
					}

					
					if (ImGui::TreeNode("Enter Model Manually"))
					{
						static char input[16] = ""; ImGui::InputText("Model", input, 16, ImGuiInputTextFlags_CharsNoBlank);
						ImGui::SameLine();
						if (ImGui::Button("Spawn"))
						{
							std::string result = input;
							if (!result.empty())
							{
								auto model = utils::joaatc(result.c_str(), result.size());
								if (STREAMING::IS_MODEL_A_VEHICLE(model)) //This seems ok on direct X thread
								{
									CallOnResult([=]()
									{
										return LoadModel(model, -1);
									}, [=]()
									{
										SpawnVehicle(model);
									});
								}

								else
								{
									set_status_text("Not A Valid Model!");
									char input[64] = "";
								}
							}

						}

						ImGui::TreePop();
					}
					

					for (const auto& types : Vehicles)
					{
						if (ImGui::TreeNode(types.first.c_str()))
						{
							auto list = types.second;
							
							ImGui::Columns(2, NULL, false);

							bool selected = false;

							for (int i = 0; i < list.size(); i++)
							{
								if (ImGui::Selectable(std::get<0>(list[i]), selected))
								{
									Hash model = std::get<2>(list[i]);
									CallOnResult([=]()
									{
										return LoadModel(model, -1);
									}, [=]()
									{
										SpawnVehicle(model);
									});
								}
								ImGui::NextColumn();
							}
							ImGui::Separator();
							ImGui::Columns(1);

							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessVehicleButton = i;
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("World Options"))
			{
				const int lineCount = 5;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Moon Gravity",	&featureWorldMoonGravity,	nullptr },
					{ "Random Cops",	&featureWorldRandomCops,	nullptr },
					{ "Random Trains",	&featureWorldRandomTrains,	nullptr },
					{ "Random Boats",	&featureWorldRandomBoats,	nullptr },
					{ "Garbage Trucks",	&featureWorldGarbageTrucks,	nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessWorldButton = i;
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("Time Options"))
			{
				const int lineCount = 4;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Hour Forward",	 nullptr,				 nullptr },
					{ "Hour Backward",	 nullptr,				 nullptr },
					{ "Clock Paused",	 &featureTimePaused, &featureTimePausedUpdated },
					{ "Sync With System", &featureTimeSynced, nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessTimeButton = i;
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("Weather Options"))
			{
				const int lineCount = 17;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Wind",	 	 &featureWeatherWind,	nullptr },
					{ "Set persist",  &featureWeatherPers,	nullptr },
					{ "ExtraSunny",	 nullptr,					nullptr },
					{ "Clear",		 nullptr,					nullptr },
					{ "Clouds",		 nullptr,					nullptr },
					{ "Smog",		 nullptr,					nullptr },
					{ "Foggy",	 	 nullptr,					nullptr },
					{ "Overcast",	 nullptr,					nullptr },
					{ "Rain",		 nullptr,					nullptr },
					{ "Thunder",	 nullptr,					nullptr },
					{ "Clearing",	 nullptr,					nullptr },
					{ "Halloween",	 nullptr,					nullptr },
					{ "Neutral",	 nullptr,					nullptr },
					{ "Snow",		 nullptr,					nullptr },
					{ "Blizzard",	 nullptr,					nullptr },
					{ "Snowlight",	 nullptr,					nullptr },
					{ "Xmas",		 nullptr,					nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						
						if (ImGui::Button(lines[i].text))
						{
							currentWeatherStatus = std::string(lines[i].text);
							ProcessWeatherButton = i;
						}
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("Misc Options"))
			{
				const int lineCount = 2;

				static struct
				{
					LPCSTR					text;
					bool					*pState;
					bool					*pUpdated;
				} lines[lineCount] = {
					{ "Next Radio Track",	nullptr, nullptr },
					{ "Hide Hud",			&featureMiscHideHud, nullptr }
				};

				// draw menu
				for (int i = 0; i < lineCount; i++)
				{
					if (lines[i].pState != nullptr)
					{
						ImGui::Checkbox(lines[i].text, lines[i].pState);
					}
					else
					{
						ImGui::PushItemWidth(-140);
						if (ImGui::Button(lines[i].text)) ProcessMiscButton = i;
						ImGui::PopItemWidth();
					}
				}
			}

			if (ImGui::CollapsingHeader("Settings"))
			{
				if (ImGui::TreeNode("Gameplay"))
				{
					ImGui::Checkbox("Game Controls Off While Menu Open", &playerControl);
					ImGui::Checkbox("Hide HUD", &featureMiscHideHud);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Style Editor"))
				{
					ImGui::ShowStyleEditor();
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Debug"))
				{
					if (ImGui::Button("Test Window")) show_test_window ^= 1;
					if (ImGui::Button("Another Window")) show_another_window ^= 1;
					ImGui::ColorEdit3("clear color", (float*)&clear_col);
					ImGui::TreePop();
				}
			}

			ImGui::End();
		}

		// Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (show_test_window)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
			ImGui::ShowTestWindow(&show_test_window);
		}

		if (show_another_window)
		{
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello Another Window");
			ImGui::End();
		}

		ImGui::Render();
	}
	catch (const std::exception &e)
	{
		set_status_text(e.what());
	}
}

void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	pDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &pRenderTargetView);
	pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (pRenderTargetView) { pRenderTargetView->Release(); pRenderTargetView = NULL; }
}

void CleanupDeviceD3D()
{
	ImGui_ImplDX11_Shutdown();
	CleanupRenderTarget();
	if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
	if (pDeviceContext) { pDeviceContext->Release(); pDeviceContext = nullptr; }
	if (pDevice) { pDevice->Release(); pDevice = nullptr; }
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OnWndProcMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(windowHandle, uMsg, wParam, lParam);
}

void PresentHook(void *swapChain)
{
	if (pSwapChain == nullptr)
	{
		windowHandle = FindWindow("grcWindow", NULL);
		pSwapChain = static_cast<IDXGISwapChain *>(swapChain);
		pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
		pDevice->GetImmediateContext(&pDeviceContext);
		ImGui_ImplDX11_Init(windowHandle, pDevice, pDeviceContext);
		CreateRenderTarget();
	}
	
	RenderDirect3d();
}