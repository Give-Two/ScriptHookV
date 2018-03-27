/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com			
			(C) Alexander Blade 2015
*/

#include "script.h"
#include <vector>

DWORD	vehUpdateTime;
DWORD	pedUpdateTime;

void update()
{
	// we don't want to mess with missions in this example
	if (GAMEPLAY::GET_MISSION_FLAG())
		return;

	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();

	// check if player ped exists and control is on (e.g. not in a cutscene)
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player))
		return;

	// get all vehicles
	const int ARR_SIZE = 1024;
	Vehicle vehicles[ARR_SIZE];
	int count = worldGetAllVehicles(vehicles, ARR_SIZE);

	// randomize all vehicle colours every 200 ms
	// setting only primary or secondary color per update
	if (vehUpdateTime + 200 < GetTickCount())
	{
		vehUpdateTime = GetTickCount();
		for (int i = 0; i < count; i++)
		{
			int primary = 0, secondary = 0;
			VEHICLE::GET_VEHICLE_COLOURS(vehicles[i], &primary, &secondary);
			if (rand() % 2)
				VEHICLE::SET_VEHICLE_COLOURS(vehicles[i], rand() % (VehicleColorBrushedGold + 1), secondary);
			else
				VEHICLE::SET_VEHICLE_COLOURS(vehicles[i], primary, rand() % (VehicleColorBrushedGold + 1));
		}		
	}

	/*	
	// delete all vehicles
	for (int i = 0; i < count; i++)
	{		
		if (!ENTITY::IS_ENTITY_A_MISSION_ENTITY(vehicles[i]))
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(vehicles[i], TRUE, TRUE);
		VEHICLE::DELETE_VEHICLE(&vehicles[i]);
	}
	*/

	// let's track all exising helis and planes and other vehicles in air
	for (int i = 0; i < count; i++)
	{
		Hash model = ENTITY::GET_ENTITY_MODEL(vehicles[i]);
		if (VEHICLE::IS_THIS_MODEL_A_HELI(model) || VEHICLE::IS_THIS_MODEL_A_PLANE(model) || ENTITY::IS_ENTITY_IN_AIR(vehicles[i]))
		{
			Vector3 v = ENTITY::GET_ENTITY_COORDS(vehicles[i], TRUE);
			float x, y;
			if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(v.x, v.y, v.z, &x, &y))
			{
				Vector3 plv	= ENTITY::GET_ENTITY_COORDS(playerPed, TRUE);
				float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(plv.x, plv.y, plv.z, v.x, v.y, v.z, TRUE);
				// draw text if vehicle isn't close to the player
				if (dist > 15.0)
				{
					int health = ENTITY::GET_ENTITY_HEALTH(vehicles[i]);
					const char* name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model);
					// print text in a box
					char text[256];					
					sprintf_s(text, "^\n%s\n| Height %.02f\n| Distance %.02f\n| Health %d", name, v.z, dist, health);
					UI::SET_TEXT_FONT(0);
					UI::SET_TEXT_SCALE(0.2, 0.2);
					UI::SET_TEXT_COLOUR(255, 255, 255, 255);
					UI::SET_TEXT_WRAP(0.0, 1.0);
					UI::SET_TEXT_CENTRE(0);
					UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
					UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
					UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
					UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
					UI::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
					// box
					GRAPHICS::DRAW_RECT(x + 0.027f, y + 0.043f, 0.058f, 0.056f, 75, 75, 75, 75, 0);
				}
			}
		}
	}

	// get all peds
	Ped peds[ARR_SIZE];
	count = worldGetAllPeds(peds, ARR_SIZE);

	// randmoize peds
	if (pedUpdateTime + 200 < GetTickCount())
	{
		pedUpdateTime = GetTickCount();
		for (int i = 0; i < count; i++)
		{
			// if (rand() % 2 != 0) continue;
			if (peds[i] != playerPed && PED::IS_PED_HUMAN(peds[i]) && !ENTITY::IS_ENTITY_DEAD(peds[i], 0))
			{
				for (int component = 0; component < 12; component++)
				{
					if (rand() % 2 != 0) continue;
					for (int j = 0; j < 100; j++)
					{
						int drawable = rand() % 10;
						int texture = rand() % 10;
						if (PED::IS_PED_COMPONENT_VARIATION_VALID(peds[i], component, drawable, texture))
						{
							PED::SET_PED_COMPONENT_VARIATION(peds[i], component, drawable, texture, 0);
							break;
						}
					}
				}
			}
		}
	}
		
	// get all objects
	Object objects[ARR_SIZE];
	count = worldGetAllObjects(objects, ARR_SIZE);

	// mark objects on the screen around the player

	// there are lots of objects in some places so we need to
	// remove possibilty of text being drawn on top of another text
	// thats why we will check distance between text on screen
	std::vector<std::pair<float, float>> coordsOnScreen;
	for (int i = 0; i < count; i++)
	{
		Hash model = ENTITY::GET_ENTITY_MODEL(objects[i]);
		Vector3 v = ENTITY::GET_ENTITY_COORDS(objects[i], TRUE);
		float x, y;
		if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(v.x, v.y, v.z, &x, &y))
		{
			// select objects only around
			Vector3 plv = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE);
			float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(plv.x, plv.y, plv.z, v.x, v.y, v.z, TRUE);
			if (dist < 200.0)
			{
				// check if the text fits on screen
				bool bFitsOnscreen = true;
				for (auto iter = coordsOnScreen.begin(); iter != coordsOnScreen.end(); iter++)
				{
					float textDist = sqrtf((iter->first - x)*(iter->first - x) + (iter->second - y)*(iter->second - y));
					if (textDist < 0.05)
					{
						bFitsOnscreen = false;
						break;
					}
				}
				// if text doesn't fit then skip draw
				if (!bFitsOnscreen) continue;

				// add text coords to the vector
				coordsOnScreen.push_back({ x, y });

				// print text in a box
				char text[256];
				sprintf_s(text, "^\n%08X\n%.02f", model, dist);
				UI::SET_TEXT_FONT(0);
				UI::SET_TEXT_SCALE(0.2, 0.2);
				UI::SET_TEXT_COLOUR(200, 255, 200, 255);
				UI::SET_TEXT_WRAP(0.0, 1.0);
				UI::SET_TEXT_CENTRE(0);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
				UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
				UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
				UI::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
				// box
				GRAPHICS::DRAW_RECT(x + 0.017f, y + 0.029f, 0.04f, 0.032f, 20, 20, 20, 75, 0);
			}
		}
	}

	// get all pickups
	Pickup pickups[ARR_SIZE];
	count = worldGetAllPickups(pickups, ARR_SIZE);

	// move pickups around the player
	for (int i = 0; i < count; i++)
	{
		Vector3 v = ENTITY::GET_ENTITY_COORDS(pickups[i], TRUE);	
		Vector3 plv = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE);
		float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(plv.x, plv.y, plv.z, v.x, v.y, v.z, TRUE);
		if (dist > 5.0)
		{
			ENTITY::SET_ENTITY_COORDS(pickups[i], plv.x, plv.y, plv.z, 0, 0, 0, FALSE);
		}

		else GRAPHICS::DRAW_MARKER(2, plv.x, plv.y, plv.z + 5.f, 0.0f, 0.0f, 0.0f, 180.0f, 0.0f, 0.0f, 0.75f, 0.75f, 0.75f, 204, 204, 1, 100, false, true, 2, false, false, false, false);
	}

	// let's add explosions to grenades in air, looks awesome !
	DWORD mhash = 0x1152354B; // for grenade launcher use 0x741FD3C4
	for (int i = 0; i < count; i++)
	{
		Hash model = ENTITY::GET_ENTITY_MODEL(objects[i]);
		if (model == mhash && ENTITY::IS_ENTITY_IN_AIR(objects[i]))
		{
			Vector3 v = ENTITY::GET_ENTITY_COORDS(objects[i], TRUE);
			Vector3 plv = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE);
			// make sure that explosion won't hurt the player
			float dist = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(plv.x, plv.y, plv.z, v.x, v.y, v.z, TRUE);
			if (dist > 10.0)
			{
				// only 1/3 of expolsions will have the sound
				FIRE::ADD_EXPLOSION(v.x, v.y, v.z, ExplosionTypeGrenadeL, 1.0, rand() % 3 == 0, FALSE, 0.f, FALSE);
			}
		}
	}
}

void main()
{		
	while (true)
	{
		update();
		WAIT(0);
	}
}

void ScriptMain()
{	
	srand(GetTickCount());
	main();
}
