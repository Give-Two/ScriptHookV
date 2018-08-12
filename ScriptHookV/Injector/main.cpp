#include "Injection.h"
#include "registry.h"

#define BREAK_WITH_ERROR( e ) { LOG_ERROR("%s. Code=%d", e, GetLastError()); return EXIT_FAILURE; }

int main()
{
	/* Configurable Settings */

	//DLL to inject
	LPCSTR dllName = "ScriptHookV.dll";

	//Hijack an existing thread ? false = create one
	bool ThreadHijacking = false;

	//DLL Injction methods - IM_LoadLibrary | IM_LdrLoadDll | IM_ManualMap
	DWORD InjectionMethod = IM_LoadLibrary;

	bool Unlink = false; // true = INJ_UNLINK_FROM_PEB

	//DLL Header Options - INJ_ERASE_HEADER | INJ_FAKE_HEADER  | INJ_UNLINK_FROM_PEB | INJ_FLAGS_ALL (All of these Options)
	DWORD HeaderOption = NULL;

	HWND hWindow = NULL;
	HANDLE hProcess = NULL;
	DWORD dwProcessId = 0;
	char * cpDllFile = nullptr;
	std::string sExecutablePath;
	bool isRetailKey = false;
	bool isSteamKey = false;

	// first truncated entry
	LOG_PRINT("\n");

	//Fuck off the ugly console;
	FreeConsole();

	Utility::SetOurModuleHandle(GetModuleHandle(NULL));

	if (dwProcessId = Utility::GetProcessIDByName("GTA5.exe")) goto DO_INJECT;

	else
	{
		Registry reg;	

		isSteamKey = reg.isSteamKey();

		isRetailKey = reg.isRetailKey();

		if (isRetailKey)//RETAIL
		{
			LOG_PRINT("Detected the retail version of GTAV.");
			if (reg.GetValue().empty())
			{
				BREAK_WITH_ERROR("Reading GTA V install path.");
			}
			else
			{
				sExecutablePath = reg.GetValue() + "\\GTA5.exe";
				if (!Utility::DoesFileExist(sExecutablePath.c_str()))
					BREAK_WITH_ERROR("GTA5.exe not found");

				LOG_DEBUG("Starting %s", "GTAVLauncher.exe");
				Utility::StartProcess((reg.GetValue() + "\\GTAVLauncher.exe").c_str());
			}
		}
		//STEAM
		else if (isSteamKey)
		{
			LOG_PRINT("Detected the steam version of GTAV.");
			if (reg.GetValue(true).empty())
			{
				BREAK_WITH_ERROR("Reading GTA V install path.");
			}
			else
			{
				sExecutablePath = reg.GetValue() + "\\GTA5.exe";
				if (!Utility::DoesFileExist(sExecutablePath.c_str()))
					BREAK_WITH_ERROR("GTA5.exe not found");

				LOG_DEBUG("Starting GTA V - steam id:271590");
				ShellExecute(NULL, "open", "steam://run/271590", NULL, NULL, SW_SHOWNORMAL);	
			}
		}

		// Wait while target process is unavailable
		LOG_PRINT("Waiting for GTA5.exe process to become available...");
		while (!dwProcessId) 
		{
			dwProcessId = Utility::GetProcessIDByName("GTA5.exe"); 
			
			if (isRetailKey)
			{
				if (!Utility::IsProcessRunning("GTAVLauncher.exe"))
				{
					BREAK_WITH_ERROR("GTAVLauncher.exe Is no longer running, aborting injection");
				}
			}

			Sleep(100);
		}
	}
	
	DO_INJECT:

	// Get full file path
	cpDllFile = new char[MAX_PATH];
	if (!GetFullPathName(dllName, MAX_PATH, cpDllFile, 0))
		BREAK_WITH_ERROR("Unable to find DLL to inject");

	Utility::SetPrivilege("SeDebugPrivilege", true);

	INJECTION_MODE im = (INJECTION_MODE)InjectionMethod;
	DWORD Flags = 0;
	if (Unlink)
		Flags = INJ_UNLINK_FROM_PEB;
	Flags |= HeaderOption;
	DWORD Err = 0;
	DWORD Ret = 0;

	LOG_PRINT("Found GTA5.exe, PID: %lu\n", dwProcessId);

	while (!hWindow)
	{
		hWindow = FindWindow("grcWindow", NULL);
		Sleep(100);
	}

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	
	if (!hProcess)
		BREAK_WITH_ERROR("Opening GTA5.exe for injection.");

	// Check if our dll is already injected
	if (!GetModuleInProcess(hProcess, cpDllFile))
	{
		InjectDLL(cpDllFile, hProcess, im, ThreadHijacking, Flags, &Err);
		CloseHandle(hProcess);
		if (Ret)
		{
			char szRet[9]{ 0 };
			char szAdv[9]{ 0 };
			_ultoa_s(Ret, szRet, 0x10);
			_ultoa_s(Err, szAdv, 0x10);
			std::string Msg = "Error code: 0x";
			Msg += szRet;
			Msg += "\nAdvanced info: 0x";
			Msg += szAdv;
			MessageBoxA(0, Msg.c_str(), "Injection failed", MB_ICONERROR);
		}
	}

	return ERROR_SUCCESS;
}