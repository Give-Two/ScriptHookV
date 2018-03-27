#include "ASILoader.h"
#include "..\ScriptHookV.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\Utility\PEImage.h"

using namespace Utility;

void LoadPlugin(const std::string& asiSearchFolder) {

	const std::string asiSearchQuery = asiSearchFolder + "\\*.asi";
	WIN32_FIND_DATAA fileData;
	HANDLE fileHandle = FindFirstFileA(asiSearchQuery.c_str(), &fileData);
	if (fileHandle != INVALID_HANDLE_VALUE) {

		do {

			const std::string pluginPath = asiSearchFolder + "\\" + fileData.cFileName;

			LOG_PRINT("Loading \"%s\"", pluginPath.c_str());
			
			// Load Image
			HMODULE module = LoadLibraryA(pluginPath.c_str());
			if (module) {
				LOG_PRINT("\tLoaded \"%s\" => 0x%p", fileData.cFileName, module);
				PlaySound("C:\\WINDOWS\\Media\\ding.wav", NULL, SND_ASYNC);
			}
			else {
				DWORD errorMessageID = ::GetLastError();
				if (errorMessageID)
				{
					LPSTR messageBuffer = nullptr;
					size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
					std::string message(messageBuffer, size);
					//Free the buffer.
					LocalFree(messageBuffer);
					LOG_ERROR("\tFailed to load: %s", message.c_str());
				}
				else LOG_ERROR("\tFailed to load");
				PlaySound("C:\\WINDOWS\\Media\\chord.wav", NULL, SND_ASYNC);
			}

		} while (FindNextFileA(fileHandle, &fileData));

		FindClose(fileHandle);
	}
}

void ASILoader::Initialize() {

	LOG_PRINT( "Loading *.asi plugins" );
	std::string moduleFolder = GetOurModuleFolder();
	std::string gtavFolder = GetRunningExecutableFolder();
	LoadPlugin(moduleFolder + "\\asi");
	LoadPlugin(gtavFolder);
	LOG_PRINT( "Finished loading *.asi plugins" );
}
