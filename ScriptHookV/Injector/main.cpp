#include "Injection.h"
#include "registry.h"
#include "Utilities.h"

std::ofstream LOG;

#define BREAK_WITH_ERROR( e ) { LOG <<( FMT("[-] %s. Error=%d", e, GetLastError()) );  LOG.close(); return 0; }

int main()
{
	/* Configurable Settings */

	//x64 process to inject our DLL
	DWORD dwProcessId = GetProcessIDByName("GTA5.exe");

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
	char * cpDllFile = nullptr;

	//Fuck off the ugly console;
	FreeConsole();
	
	// Get Log path
	static std::string logFileName("");
	char *cpLogFile = new char[MAX_PATH];
	if (GetFullPathNameA("ScriptHookV.log", MAX_PATH, cpLogFile, 0))
	{
		logFileName = cpLogFile; LOG.open(logFileName, std::ofstream::trunc); LOG.close();
	}	LOG.open(logFileName, std::ofstream::out | std::ofstream::app );

	if (!dwProcessId)
	{
		Registry reg;
		if (reg.isRetailKey() == true)//RETAIL
		{
			LOG << (" Retail GTA V version detected.\n");
			if (reg.GetValue().empty())
			{
				LOG << (" reading GTA V install path.\n");
				return EXIT_FAILURE;
			}
			else
			{
				std::string launcher = reg.GetValue() + "\\GTAVLauncher.exe";
				if (!DoesFileExist(launcher.c_str()))
					BREAK_WITH_ERROR("GTAVLauncher.exe not found");

				LOG << FMT(" Starting %s\n", launcher.c_str());
				startup(launcher.c_str());
				LOG << (" Waiting for GTA5.exe process to become available...\n");
			}
		}
		//STEAM
		else if (reg.isSteamKey() == true)
		{
			LOG << (" Steam GTA V version detected.\n");
			if (reg.GetValue(true).empty())
			{
				LOG << (" reading GTA V install path.\n");
				return EXIT_FAILURE;
			}
			else
			{
				LOG << (" Starting GTA V - steam id:271590\n");
				ShellExecute(NULL, "open", "steam://run/271590", NULL, NULL, SW_SHOWNORMAL);
				LOG << (" Waiting for GTA5.exe process to become available...\n");
			}
		}

		// Wait while target process is unavailable
		while (!dwProcessId)
		{
			dwProcessId = GetProcessIDByName("GTA5.exe");
			if (reg.isRetailKey() == true)
			{
				if (!IsProcessRunning("GTAVLauncher.exe"))
				{
					BREAK_WITH_ERROR("GTAVLauncher.exe Is no longer running");
				}
			}
			Sleep(100);
		}
	}	LOG << FMT(" Found GTA5.exe, PID: %lu\n", dwProcessId);

	// Wait for window to reappear
	while (!hWindow)
	{
		hWindow = FindWindow("grcWindow", NULL);
	}

	// Get full file path
	cpDllFile = new char[MAX_PATH];
	if (cpDllFile == 0) return 0;
	if (!GetFullPathNameA(dllName, MAX_PATH, cpDllFile, 0))
		BREAK_WITH_ERROR("Unable to find DLL to inject");

	INJECTION_MODE im = (INJECTION_MODE)InjectionMethod;
	DWORD Flags = 0;
	if (Unlink)
		Flags = INJ_UNLINK_FROM_PEB;
	Flags |= HeaderOption;

	SetPrivilege("SeDebugPrivilege", true);
	DWORD Err = 0;
	DWORD Ret = 0;
	if (dwProcessId)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
		if (!hProcess)
			return 0;

		// Check if our dll is already injected
		if (get_module_address(hProcess, cpDllFile))
		{
			return EXIT_FAILURE;
		}

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

	LOG.close();

	return ERROR_SUCCESS;
}