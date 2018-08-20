#include "Injection.h"
#include "registry.h"

#define BREAK_WITH_ERROR( fmt, ...)	Utility::GetLog()->LogToFile( Utility::eLogType::LogTypeError, FMT( fmt, ##__VA_ARGS__ ) + FMT( " Code=%d", GetLastError() ))

int main()
{
	/* Configurable Settings */

	//Target process
	const std::string exeName = "GTA5.exe";

	//DLL to inject
	const std::string dllName = "ScriptHookV.dll";

	//Hijack an existing thread ? false = create one
	bool ThreadHijacking = false;

	//DLL Injction methods - IM_LoadLibrary | IM_LdrLoadDll | IM_ManualMap
	INJECTION_MODE InjectionMethod = IM_LoadLibrary;

	//DLL Header Options - INJ_ERASE_HEADER | INJ_FAKE_HEADER  | INJ_UNLINK_FROM_PEB | INJ_FLAGS_ALL (All of these Options)
	DWORD HeaderFlags = NULL;

	// Registry entry
	std::string sExecutablePath;
	bool isRetailKey = false;
	bool isSteamKey = false;

	// first truncated entry
	LOG_PRINT("\n");

	// console window removal;
	FreeConsole();

	Utility::SetOurModuleHandle(GetModuleHandle(NULL));

	HANDLE hProcess = NULL;

	if (Utility::GetProcess(exeName, hProcess)) goto DO_INJECT;

	if (!hProcess)
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
					BREAK_WITH_ERROR("GTA5.exe not found.");

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
					BREAK_WITH_ERROR("GTA5.exe not found.");

				LOG_DEBUG("Starting GTA V - steam id:271590");
				ShellExecute(NULL, "open", "steam://run/271590", NULL, NULL, SW_SHOWNORMAL);	
			}
		}

		// Wait while target process is unavailable
		LOG_DEBUG("Waiting for %s process to become available...", exeName.c_str());

		while (!Utility::GetProcess(exeName, hProcess))
		{	
			if (isRetailKey)
			{
				if (!Utility::GetProcessID("GTAVLauncher.exe"))
				{
					BREAK_WITH_ERROR("GTAVLauncher.exe Is no longer running, aborting injection.");
				}
			}

			Sleep(100);
		}
	}
	
	DO_INJECT:

	// Ensure we have the process open
	if (!hProcess)
		BREAK_WITH_ERROR("Opening %s.", exeName.c_str());

	LOG_DEBUG("Found %s, PID: %lu", exeName.c_str(), GetProcessId(hProcess));

	// Get full file path
	const char* cpDllFile = Utility::GetNamedModuleFolder(dllName, true).c_str();
	if (!cpDllFile)
		BREAK_WITH_ERROR("Unable to find %s to inject", dllName.c_str());

	// Wait for the window
	while (GetForegroundWindow() != FindWindow("grcWindow", NULL))
	{
		doOnce(LOG_DEBUG("Waiting for %s window to become visible...", exeName.c_str()));
		Sleep(100);
	}

	Sleep(2000); //Seems to need a sleep here sometimes else errors
	
	DWORD Err = 0;
	auto Ret = InjectDLL(cpDllFile, hProcess, InjectionMethod, ThreadHijacking, HeaderFlags, &Err);
	if (Ret)
	{
		std::string Msg = "Error code: ";
		Msg += eInjectionErrorNames[Ret];
		Msg += "\nAdvanced info: ";
		Msg += eInjectionAdvErrorNames[Err];
		MessageBox(0, Msg.c_str(), "Injection failed", MB_ICONERROR);
	}

	CloseHandle(hProcess);
	
	return ERROR_SUCCESS;
}