#include "ScriptEngine.h"
#include "ScriptManager.h"
#include "NativeInvoker.h"
#include "Pools.h"

#include "..\ASI Loader\ASILoader.h"
#include "..\Input\InputHook.h"
#include "..\DirectX\D3d11Hook.h"
#include "..\Hooking\Hooking.h"

#include "..\..\SDK\inc\enums.h"

struct GlobalTable 
{
	PINT64* GlobalBasePtr;
	PINT64 AddressOf(int index) const { return &GlobalBasePtr[index >> 18 & 0x3F][index & 0x3FFFF]; }
	bool IsInitialised()const { return *GlobalBasePtr; }
};	

GlobalTable		globalTable;
eGameState *	gameState;
eHookState		g_HookState;
int				g_GameVersion;
bool			g_IsRetail;

bool ScriptEngine::Initialize() 
{
	LOG_PRINT("Initializing...");

	// init Direct3d hook
	if (!g_D3DHook.InitializeHooks())
	{
		LOG_ERROR("Failed to Initialize Direct3d Hooks");
		return false;
	}

	// kill this data snoop ( must  be done after D3D )
	if (Utility::GetProcessID("GTAVLauncher.exe"))
	{
		Utility::killProcessByName("GTAVLauncher.exe");
		LOG_DEBUG("Killed %s", "GTAVLauncher.exe");
	}

	// init Winproc hook
	if (!InputHook::Initialize()) 
	{
		LOG_ERROR("Failed to Initialize InputHook");
		return false;
	}

	// Get game state
	if (auto gameStatePattern = "83 3D ? ? ? ? ? 8A D9 74 0A"_Scan)
	{
		gameState = gameStatePattern.add(2).rip(5).as<decltype(gameState)>();
		LOG_ADDRESS("gameState", gameState);
	}
	else
	{
		LOG_ERROR("Unable to find gameState");
		return false;
	}

	// Get global table
	if (auto globalTablePattern = "4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11"_Scan)
	{
		globalTable.GlobalBasePtr = globalTablePattern.add(3).rip(4).as<PINT64*>();
		while (!globalTable.IsInitialised()) Sleep(100);
		LOG_ADDRESS("globalTable", globalTable.GlobalBasePtr);
	}
	else
	{
		LOG_ERROR("Unable to find globalTable");
		return false;
	}

	while (GetGameState() != GameStatePlaying) Sleep(100);

	LOG_PRINT("Performing function hooking...");
	
	if (Hooking::HookFunctions())
	{
		ASILoader::Initialize();
		
		LOG_PRINT("Initialization finished");

		g_HookState = HookStateRunning;

		return true;
	}

	g_HookState = HookStateExiting;

	return false;
}

eGameState ScriptEngine::GetGameState() 
{
	return *gameState;
}

PUINT64 ScriptEngine::getGlobal(int globalId)
{
	return reinterpret_cast<PUINT64>(globalTable.AddressOf(globalId));
}

uint32_t ScriptEngine::RegisterFile(const char* fullpath, const char* filename)
{
	uint32_t index;
	rage::FileRegister(&index, fullpath, true, filename, false);
	return index;
}
