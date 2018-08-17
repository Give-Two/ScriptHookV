#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Input\InputHook.h"
#include "Hooking\Hooking.h"
#include "DirectX\D3d11Hook.h"
#include "Utility\Versioning.h"
#include "..\SDK\inc\enums.h"

using namespace Utility;

std::uint32_t g_ThreadHash = "main_persistent"_joaat;

std::deque<std::function<void()>> g_Stack;

BOOL APIENTRY DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID /*lpvReserved*/ ) 
{
	switch ( dwReason ) 
	{
		case DLL_PROCESS_ATTACH: 
		{
			SetOurModuleHandle(hModule);
			//Memory::pattern("48 8D 4D AF E8 ? ? ? ? 84 C0 74 1F").count(1).get(0).get<void>(0x4B);
			// no launcher check
			if (auto p_launcherCheck = "E8 ? ? ? ? 84 C0 75 0C B2 01 B9 2F"_Scan) p_launcherCheck.nop(21);
			
			// no legals
			if (auto p_gameLegals = "72 1F E8 ? ? ? ? 8B 0D"_Scan) p_gameLegals.nop(2);

			// no annoying movie
			// this doesn't save any loadup time just plays nothing
			//if (auto p_preMovie = "70 6C 61 74 66 6F 72 6D 3A 2F 6D 6F 76"_Scan) p_preMovie.nop(13);

			static auto& versionTool = GTAVersion::GetInstance();
			g_GameVersion = versionTool.GameVersion();
			auto gta5directory = versionTool.GameDirectory();
			auto versionString = versionTool.VersionString();

			if (g_GameVersion == -1)
			{
				g_GameVersion = -1;
				auto file = GetOurModuleFolder() + "\\Native_Registration.txt";
				if (DoesFileExist(file.c_str())) remove(file.c_str());
				LOG_FILE("Registration", "%s", versionString.c_str());
				CreateElevatedThread([](LPVOID)->DWORD
				{ 
					Hooking::HookFunctions();
					while (true)
					{
						static eGameState* gameState = "83 3D ? ? ? ? ? 8A D9 74 0A"_Scan.add(2).rip(5).as<decltype(gameState)>();
						if (*gameState == GameStateMainMenu)
						{
							Hooking::UnHookFunctions();
							MessageBoxA(NULL, FMT("New Game Version %s", versionTool.VersionString().c_str()).c_str(), "ScriptHookV Incompatible", MB_OK | MB_TOPMOST);
							FreeLibraryAndExitThread(Utility::GetOurModuleHandle(), ERROR_SUCCESS);
							break;
						}
					}
					return TRUE;
				});
			}
			else
			{
				if (!DoesFileExist((gta5directory + "\\steam_api64.dll").c_str()))
				{
					g_GameVersion += 1;
				}

				LOG_DEBUG("found GTA5 directory %s", gta5directory.c_str());
				LOG_DEBUG("detected GTA5 version %s (SHV patch %d)", versionString.c_str(), g_GameVersion);

				// incompatible with versions prior to 1.0.1493.0 with current hashmap.
				if (g_GameVersion < VER_1_0_1493_0_STEAM)
				{
					MessageBoxA(NULL, "Update to Version 1.0.1493.0", "Game Version Incompatible!",  MB_OK | MB_TOPMOST);
					FreeLibraryAndExitThread(hModule, 0);
					return TRUE;
				}

				CreateElevatedThread([](LPVOID)->DWORD
				{
					if (!ScriptEngine::Initialize())
					{
						LOG_ERROR("Failed to initialize ScriptEngine");
						return FALSE;
					}	return TRUE;
				});
				
			}
			break;
		}
		case DLL_PROCESS_DETACH: 
		{
			g_HookState = HookStateExiting;
			break;
		}
	}

	return TRUE;
}
