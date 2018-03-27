#include "General.h"
#include <Tlhelp32.h>

static HMODULE ourModule;

namespace Utility {

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

	void SetOurModuleHanlde(const HMODULE module) {

		ourModule = module;
	}

	const HMODULE GetOurModuleHandle() {

		return ourModule;
	}

	bool fileExists(const char* name) {

		struct stat buffer;
		return (stat(name, &buffer) == 0);
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

	std::uint32_t joaat(const char* string)
	{
		return joaatc(string, std::strlen(string));
	}

	std::uint32_t joaat(const std::string& string)
	{
		return Utility::joaatc(string.c_str(), string.size());
	}
}
