#include "General.h"

static HMODULE ourModule;

namespace Utility 
{
	/* String */

	std::wstring str_to_wstr(const std::string& string)
	{
		std::wstring wstring;

		return MultiByteToWideChar(CP_UTF8, 0, string.data(), static_cast<int>(string.size()), &wstring[0], (int)wstring.size()) ? wstring : std::wstring();
	}

	std::string wstr_to_str(const std::wstring& wstring)
	{
		std::string string;

		return WideCharToMultiByte(CP_UTF8, 0, wstring.data(), static_cast<int>(wstring.size()), &string[0], static_cast<int>(string.size()), nullptr, nullptr) ? string : std::string();
	}

	/* Hash */

	std::uint32_t joaat(const char* string)
	{
		return joaatc(string, std::strlen(string));
	}

	std::uint32_t joaat(const std::string& string)
	{
		return Utility::joaatc(string.c_str(), string.size());
	}

	/* File / Folder */

	bool DoesFileExist(const char* name) {

		struct stat buffer;
		return (stat(name, &buffer) == 0);
	}

	const std::string GetRunningExecutableFolder() {

		char fileName[MAX_PATH];
		GetModuleFileNameA(NULL, fileName, MAX_PATH);

		std::string currentPath = fileName;
		return currentPath.substr(0, currentPath.find_last_of("\\"));
	}

	const std::string GetOurModuleFolder() {

		char fileName[MAX_PATH];
		GetModuleFileNameA(ourModule, fileName, MAX_PATH);

		std::string currentPath = fileName;
		return currentPath.substr(0, currentPath.find_last_of("\\"));
	}

	/* Module / Process Related */

	void SetOurModuleHandle(const HMODULE module) {

		ourModule = module;
	}

	const HMODULE GetOurModuleHandle() {

		return ourModule;
	}

	const std::string GetModuleName(const HMODULE module) {

		char fileName[MAX_PATH];
		GetModuleFileNameA(module, fileName, MAX_PATH);

		std::string fullPath = fileName;

		size_t lastIndex = fullPath.find_last_of("\\") + 1;
		return fullPath.substr(lastIndex, fullPath.length() - lastIndex);
	}

	const std::string GetModuleNameWithoutExtension(const HMODULE module) {

		const std::string fileNameWithExtension = GetModuleName(module);

		size_t lastIndex = fileNameWithExtension.find_last_of(".");
		if (lastIndex == -1) {
			return fileNameWithExtension;
		}

		return fileNameWithExtension.substr(0, lastIndex);
	}

	DWORD GetProcessIDByName(const std::string& processName)
	{
		PROCESSENTRY32 processInfo;
		processInfo.dwSize = sizeof(processInfo);

		HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (processesSnapshot == INVALID_HANDLE_VALUE)
			return 0;

		Process32First(processesSnapshot, &processInfo);
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}

		while (Process32Next(processesSnapshot, &processInfo))
		{
			if (!processName.compare(processInfo.szExeFile))
			{
				CloseHandle(processesSnapshot);
				return processInfo.th32ProcessID;
			}
		}

		CloseHandle(processesSnapshot);
		return 0;
	}

	void Startup(LPCTSTR lpApplicationName)
	{
		// additional information
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		CreateProcess(lpApplicationName,   // the path
			NULL,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi);           // Pointer to PROCESS_INFORMATION structure
							// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	void create_thread(LPTHREAD_START_ROUTINE thread)
	{
		DWORD myThreadID;
		HANDLE myHandle = CreateThread(0, 0, thread, 0, 0, &myThreadID);
		CloseHandle(myHandle);
	}

	void killProcessByName(const char *filename)
	{
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		PROCESSENTRY32 pEntry;
		pEntry.dwSize = sizeof(pEntry);
		BOOL hRes = Process32First(hSnapShot, &pEntry);
		while (hRes)
		{
			if (strcmp(pEntry.szExeFile, filename) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
					(DWORD)pEntry.th32ProcessID);
				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 9);
					CloseHandle(hProcess);
				}
			}
			hRes = Process32Next(hSnapShot, &pEntry);
		}
		CloseHandle(hSnapShot);
	}

	bool SetPrivilege(const char * szPrivilege, bool bState)
	{
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
			return false;

		TOKEN_PRIVILEGES TokenPrivileges = { 0 };
		TokenPrivileges.PrivilegeCount = 1;
		TokenPrivileges.Privileges[0].Attributes = bState ? SE_PRIVILEGE_ENABLED : 0;

		if (!LookupPrivilegeValueA(nullptr, szPrivilege, &TokenPrivileges.Privileges[0].Luid))
		{
			CloseHandle(hToken);
			return false;
		}

		if (!AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
		{
			CloseHandle(hToken);
			return false;
		}

		CloseHandle(hToken);

		return true;
	}

	bool Is64BitProcess(HANDLE hProc)
	{
		bool Is64BitWin = false;
		BOOL Out = 0;
		IsWow64Process(GetCurrentProcess(), &Out);
		if (Out)
			Is64BitWin = true;

		if (!IsWow64Process(hProc, &Out))
			return false;

		if (Is64BitWin && !Out)
			return true;

		return false;
	}

	bool IsProcessRunning(const char *filename)
	{
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		PROCESSENTRY32 pEntry;
		pEntry.dwSize = sizeof(pEntry);
		BOOL hRes = Process32First(hSnapShot, &pEntry);
		while (hRes)
		{
			if (strcmp(pEntry.szExeFile, filename) == 0)
			{
				return true;
			}
			hRes = Process32Next(hSnapShot, &pEntry);
		}
		CloseHandle(hSnapShot);
		return false;
	}

	/* General Misc */

	void playwindowsSound(const char* sound)
	{
		char windir[MAX_PATH];
		const char* delim = "\\";
		GetWindowsDirectoryA(windir, MAX_PATH);
		std::string currentPath = windir;
		currentPath.substr(0, currentPath.find_last_of("\\"));
		currentPath.append(delim).append("Media").append(delim).append(sound);
		PlaySoundA(NULL, NULL, SND_NODEFAULT); // cancel the current sound playing
		PlaySoundA(currentPath.c_str(), NULL, SND_ASYNC | SND_NODEFAULT);
	}
}
