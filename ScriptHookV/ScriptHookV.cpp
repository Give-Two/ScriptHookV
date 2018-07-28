#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Input\InputHook.h"
#include "Hooking\Hooking.h"
#include "DirectX\D3d11Hook.h"
#include "Utility\Console.h"
#include "Utility\General.h"
#include "Utility\Versioning.h"

using namespace Utility;

std::uint32_t g_ThreadHash = "main_persistent"_joaat;

std::deque<std::function<void()>> g_Stack;

void Cleanup() {

	if ( IsThreadAFiber() )
	{
		ConvertFiberToThread();
		CloseHandle(g_MainFiber);
	}

	CloseHandle(CreateThread(NULL, NULL, [](LPVOID) -> DWORD 
	{
		if (GetConsole()->IsAllocated()) GetConsole()->DeAllocate();

		InputHook::Remove();

		for (auto && pair : g_hooks) Hooking::RemoveDetour(pair.first, pair.second);

		FreeLibraryAndExitThread(Utility::GetOurModuleHandle(), 0); 

	}, NULL, NULL, NULL)); 
}

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID /*lpvReserved*/ ) {

	switch ( dwReason ) {
		case DLL_PROCESS_ATTACH: {

			SetOurModuleHanlde(hModule);

#ifdef _CONSOLE
			GetConsole()->Allocate();
#endif
			// Clear LogFile
			GetLog()->Clean();

			auto& versionTool = GTAVersion::GetInstance();

			g_GameVersion = versionTool.GameVersion();

			if (g_GameVersion > -1)
			{
				auto gta5directory = versionTool.GameDirectory();
				auto versionString = versionTool.VersionString();

				if (!fileExists((gta5directory + "\\steam_api64.dll").c_str()))
				{
					g_GameVersion += 1;
				}

				LOG_DEBUG("found GTA5 directory %s", gta5directory.c_str());
				LOG_DEBUG("detected GTA5 version %s (SHV patch %d)", versionString.c_str(), g_GameVersion);

				// incompatible with versions prior to 1.0.1393.0 with current hashmap.
				if (g_GameVersion < VER_1_0_1493_0_STEAM)
				{
					LOG_MESSAGE("ERROR", "Game Version Incompatible");
					Cleanup();
					return TRUE;
				}

				CloseHandle(CreateThread(NULL, NULL, [](LPVOID) -> DWORD
				{
					LOG_PRINT("Initializing...");

					Utility::killProcessByName("GTAVLauncher.exe");
					LOG_DEBUG("Killed GTAVLauncher.exe");

					if (!ScriptEngine::Initialize()) {

						LOG_ERROR("Failed to initialize ScriptEngine");
						return 0;
					}

					return 1;
				}, NULL, NULL, NULL));
				
			}
			break;
		}
		case DLL_PROCESS_DETACH: {

			Cleanup();
			break;
		}
	}

	return TRUE;
}
