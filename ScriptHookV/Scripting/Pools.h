#pragma once
#include "..\..\SDK\inc\types.h"
#include "..\ScriptHookV.h"

enum EntityPoolType : uint16_t
{
	PoolTypeVehicle = 1,
	PoolTypePed = 2,
	PoolTypeObject = 4,
	PoolTypePickup = 8,
	PoolTypeCamera = 16
};

namespace rage 
{
	std::string GetEntityPoolStats();

	int GetAllWorld(EntityPoolType type, int max, int *arr);

	std::vector<Entity> GetAllWorld(EntityPoolType type, int max);
};
