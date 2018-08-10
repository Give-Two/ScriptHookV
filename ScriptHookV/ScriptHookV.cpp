#include "..\SDK\inc\enums.h"
#include "Scripting\ScriptEngine.h"
#include "Scripting\ScriptManager.h"
#include "Input\InputHook.h"
#include "Hooking\Hooking.h"
#include "DirectX\D3d11Hook.h"
#include "Utility\General.h"
#include "Utility\Versioning.h"

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

			// no launcher check
			if (auto p_launcherCheck = "E8 ? ? ? ? 84 C0 75 0C B2 01 B9 2F"_Scan) p_launcherCheck.nop(21);
			
			// no legals
			if (auto p_gameLegals = "72 1F E8 ? ? ? ? 8B 0D"_Scan) p_gameLegals.nop(2);

			// no annoying movie
			// this doesn't save any loadup time just plays nothing
			//if (auto p_preMovie = "70 6C 61 74 66 6F 72 6D 3A 2F 6D 6F 76"_Scan) p_preMovie.nop(13);

			auto& versionTool = GTAVersion::GetInstance();

			g_GameVersion = versionTool.GameVersion();

			if (g_GameVersion > -1)
			{
				auto gta5directory = versionTool.GameDirectory();
				auto versionString = versionTool.VersionString();

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

				CloseHandle(CreateThread(NULL, NULL, [](LPVOID) -> DWORD
				{
					LOG_PRINT("Initializing...");

					Utility::killProcessByName("GTAVLauncher.exe");
					LOG_DEBUG("Killed GTAVLauncher.exe");

					if (!ScriptEngine::Initialize()) 
					{
						LOG_ERROR("Failed to initialize ScriptEngine");
						return TRUE;
					}

					return TRUE;
				}, NULL, NULL, NULL));
				
			}
			break;
		}
		case DLL_PROCESS_DETACH: 
		{
			break;
		}
	}

	return TRUE;
}
